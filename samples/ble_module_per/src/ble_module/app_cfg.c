/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>
#include <net/buf.h>
#include <stdlib.h>
#include "soc.h"
#include "soc_pm.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/storage.h>
#include "keys.h"
#include <input_dev.h>

#include "hci_core.h"
#include "hci_core_patch.h"

#include "nvram_config.h"

#include "at_cmd.h"
#include "act_data.h"
#include "app_ble.h"
#include "app_cfg.h"
#include "msg_manager.h"

extern char *sys_version_get(void);

extern struct app_cb_t app_cb;
extern u8_t adv_user_data[];
extern struct bt_le_adv_param app_adv_cfg;
extern u8_t adv_name_data[];
extern struct bt_data app_scan_data[];
extern struct bt_le_conn_param app_conn_update_cfg;
extern struct bt_data app_adv_data[];
extern struct bt_uuid_128 data_srv_uuid;
extern struct bt_uuid_128 rx_char_uuid;
extern struct bt_uuid_128 tx_char_uuid;
extern struct bt_uuid_128 drv_srv_uuid;
extern struct bt_uuid_128 adc_char_uuid;
extern struct bt_uuid_128 pwm_char_uuid;
extern u8_t rts_func_enable;

char app_bt_addr_str[] = "11:22:33:44:55:66";
u16_t adv_interval_array[] = {20, 50, 100, 200, 300, 500, 1000, 2000}; /* ms: adv interval */
u16_t conn_interval_array[] = {20, 30, 50, 100, 200, 300, 500, 1000, 2000};  /* ms: conn interval */
u8_t tx_power_level_index = BLE_TX_POWER_DEF;
extern u8_t tx_power_level[];

int baud_index = UART_BAUDRATE_DEF;
u32_t uart_baudrate[] = {9600, 19200, 38400, 57600, 115200};
#define	UART0_BASE 0X4000D000
#define	UART1_BASE 0X4000E000
#define	UARTX_BR   0x0010
#define UART_BR_TXBRDIV_SHIFT   16
#define UART_BR_RXBRDIV_SHIFT   0
void uart_baudrate_set(u32_t baudrate)
{
	u32_t uart_clock = 16000000;
	u32_t reg_val = ((uart_clock / baudrate) << UART_BR_TXBRDIV_SHIFT) |
					((uart_clock / baudrate) << UART_BR_RXBRDIV_SHIFT);

	if (!strcmp(CONFIG_UART_AT_ON_DEV_NAME, "UART_0")) {
		sys_write32(reg_val, UART0_BASE + UARTX_BR);
	} else {
		sys_write32(reg_val, UART1_BASE + UARTX_BR);
	}
}

void load_uuid_cfg_from_nv(char *name, struct bt_uuid_128 *uuid)
{
	int err;
	u16_t param_16;

	err = nvram_config_get(name, &param_16, sizeof(u16_t));
	if (err >= 0) {
		uuid->val[13] = (param_16 >> 8) & 0xFF;
		uuid->val[12] = param_16 & 0xFF;
		printk("load %s 0x%02x%02x\n", name, uuid->val[13], uuid->val[12]);
	}
}

void load_app_cfg_from_nv(void)
{
	int err;
	u8_t param_8;
	u16_t param_16;
	u8_t param[30];

	err = nvram_config_get_factory("BT_ADDR", app_bt_addr_str, strlen(app_bt_addr_str));
	if (err >= 0) {
		printk("load BT_ADDR %s\n", app_bt_addr_str);
	}
	
	err = nvram_config_get("ADV_STATE", &param_8, sizeof(u8_t));
	if (err >= 0) {
		app_cb.adv_state = param_8;
		printk("load ADV_STATE %d\n", app_cb.adv_state);
	}
	
	err = nvram_config_get("ADV_INTERVAL", &param_16, sizeof(u16_t));
	if (err >= 0) {
		app_adv_cfg.interval_min = param_16;
		app_adv_cfg.interval_max = param_16;
		printk("load ADV_INTERVAL 0x%04x(%d ms)\n", app_adv_cfg.interval_min, app_adv_cfg.interval_min * 625 / 1000);
	}

	err = nvram_config_get("ADV_NAME", param, BLE_ADV_NAME_DATA_MAX+1);
	if (err >= 0) {
		param_8 = param[BLE_ADV_NAME_DATA_MAX];
		if (param_8 > BLE_ADV_NAME_DATA_MAX) 
			param_8 = BLE_ADV_NAME_DATA_MAX;
		adv_name_data[BLE_ADV_NAME_DATA_MAX] = param_8;
		memcpy(adv_name_data, param, param_8);
		app_scan_data[0].data = (const u8_t *)adv_name_data;
		app_scan_data[0].data_len = param_8;
		param[param_8] = '\0';
		printk("load ADV_NAME %s(%d)\n", (char*)param, app_scan_data[0].data_len);
	}

	err = nvram_config_get("UART_BAUDRATE", &param_8, sizeof(u8_t));
	if (err >= 0) {
		baud_index = param_8;
		uart_baudrate_set(uart_baudrate[baud_index]);
		printk("load UART_BAUDRATE 0x%02x(%d)\n", baud_index, uart_baudrate[baud_index]);
	} else {
		uart_baudrate_set(uart_baudrate[baud_index]);
		printk("Set default UART_BAUDRATE 0x%02x(%d)\n", baud_index, uart_baudrate[baud_index]);
	}
	
	err = nvram_config_get("CONN_INTERVAL", &param_16, sizeof(u16_t));
	if (err >= 0) {
		app_conn_update_cfg.interval_min = param_16;
		app_conn_update_cfg.interval_max = param_16;
		printk("load CONN_INTERVAL 0x%04x(%d ms)\n", app_conn_update_cfg.interval_min, app_conn_update_cfg.interval_min * 125 / 100);
	}	
	
	printk("load TX_POWER 0x%04x runtime_tx %d dm\n", tx_power_level_index, (s8_t)tx_power_level[tx_power_level_index+8]);
	
	err = nvram_config_get("ADV_USER_DATA", param, BLE_ADV_USER_DATA_LEN);
	if (err >= 0) {
		memcpy(adv_user_data, param, BLE_ADV_USER_DATA_LEN);
		app_adv_data[1].data = (const u8_t *)adv_user_data;
		app_adv_data[1].data_len = BLE_ADV_USER_DATA_LEN;
		{
			int i = 0;
			printk("load ADV_USER_DATA ");
			for (i = BLE_ADV_USER_DATA_LEN - 1; i >=0 ; i--)
				printk("%02x",adv_user_data[i]);
			printk("\n");
		}
	}

	load_uuid_cfg_from_nv("DATA_SRV_UUID", &data_srv_uuid);
	load_uuid_cfg_from_nv("TX_UUID", &tx_char_uuid);
	load_uuid_cfg_from_nv("RX_UUID", &rx_char_uuid);
	load_uuid_cfg_from_nv("DRV_SRV_UUID", &drv_srv_uuid);
	load_uuid_cfg_from_nv("ADC_UUID", &adc_char_uuid);
	load_uuid_cfg_from_nv("PWM_UUID", &pwm_char_uuid);
	
	err = nvram_config_get("RSSI_MODE", &param_8, sizeof(u8_t));
	if (err >= 0) {
		app_cb.rssi_mode = param_8;
		printk("load RSSI_MODE %d\n", app_cb.rssi_mode);
	}
	
	err = nvram_config_get("RTS", &param_8, sizeof(u8_t));
	if (err >= 0) {
		rts_func_enable = param_8;
		printk("load RTS %d\n", rts_func_enable);
	}
}

void set_app_cfg_to_default(void)
{
	u8_t param_8;
	u16_t param_16;
	u8_t name_data[BLE_ADV_NAME_DATA_MAX+1] = {BLE_ADV_NAME_DATA_DEF};
	u8_t user_data[BLE_ADV_USER_DATA_LEN] = {BLE_ADV_USER_DATA_DEF};

	param_8 = BLE_ADV_STATE_DEF;
	nvram_config_set("ADV_STATE", &param_8, sizeof(u8_t)); 
	
	param_16 = BLE_ADV_INTERVAL_DEF;
	nvram_config_set("ADV_INTERVAL", &param_16, sizeof(u16_t));

	name_data[BLE_ADV_NAME_DATA_MAX] = BLE_ADV_NAME_LEN_DEF;
	nvram_config_set("ADV_NAME", name_data, BLE_ADV_NAME_DATA_MAX+1);

	param_8 = UART_BAUDRATE_DEF;
	nvram_config_set("UART_BAUDRATE", &param_8, sizeof(u8_t));
	
	param_16 = BLE_CONN_INTERVAL_DEF;
	nvram_config_set("CONN_INTERVAL", &param_16, sizeof(u16_t));
	
	param_8 = BLE_TX_POWER_DEF;
	nvram_config_set("TX_POWER", &param_8, sizeof(u8_t));
	
	nvram_config_set("ADV_USER_DATA", user_data, BLE_ADV_USER_DATA_LEN);
	
	param_16 = (DATA_SRV_UUID_H_DEF << 8) + DATA_SRV_UUID_L_DEF;
	nvram_config_set("DATA_SRV_UUID", &param_16, sizeof(u16_t));
	param_16 = (TX_UUID_H_DEF << 8) + TX_UUID_L_DEF;
	nvram_config_set("TX_UUID", &param_16, sizeof(u16_t));
	param_16 = (RX_UUID_H_DEF << 8) + RX_UUID_L_DEF;
	nvram_config_set("RX_UUID", &param_16, sizeof(u16_t));
	param_16 = (DRV_SRV_UUID_H_DEF << 8) + DRV_SRV_UUID_L_DEF;
	nvram_config_set("DRV_SRV_UUID", &param_16, sizeof(u16_t));
	param_16 = (ADC_UUID_H_DEF << 8) + ADC_UUID_L_DEF;
	nvram_config_set("ADC_UUID", &param_16, sizeof(u16_t));
	param_16 = (PWM_UUID_H_DEF << 8) + PWM_UUID_L_DEF;
	nvram_config_set("PWM_UUID", &param_16, sizeof(u16_t));
	
	param_8 = BLE_RSSI_MODE_DEF;
	nvram_config_set("RSSI_MODE", &param_8, sizeof(u8_t));
	
	param_8 = GPIO_RTS_ENABLE_DEF;
	nvram_config_set("RTS", &param_8, sizeof(u8_t));
}

/* 1. soft reset */
int at_cmd_reset(void)
{
	set_app_cfg_to_default();
	at_cmd_response("OK+STRS\r\n");
	printk("reset\n");
	
	k_sleep(100);
	sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	return 0;
}

/* 2. state of adv */
int at_cmd_adv_set(const char *p_para)
{
	u8_t state= atoi(p_para);
	
	if(!is_decimal_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	
	switch (state) {
	case ADV_STOP:
	case CONN_ADV_START:
	case NON_CONN_ADV_START:
		ble_exec_adv_op(state);
		break;
	default:
		at_cmd_response_error();
		return 0;
	}
	app_cb.adv_state = state;
	nvram_config_set("ADV_STATE", &state, sizeof(u8_t));
	
	at_cmd_response("OK+ADST-");
	at_cmd_response(p_para);
	at_cmd_response("\r\n");
	return 0;
}

int at_cmd_adv_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+ADST-%d\r\n", app_cb.adv_state);
	at_cmd_response(ret);
	return 0;
}

/* 3. interval of adv */
int at_cmd_adv_interval_set(const char *p_para)
{
	u16_t interval;
	u8_t inteval_index = atoi(p_para);
	
	if (!is_decimal_string(p_para) ||
		(inteval_index > 7)) {
		at_cmd_response_error();
		return 0;
	}

	interval = adv_interval_array[inteval_index] * 1000 / 625;
	printk("interval : %d, %d\n", interval, adv_interval_array[inteval_index]);

	app_adv_cfg.interval_min = interval;
	app_adv_cfg.interval_max = interval;
	nvram_config_set("ADV_INTERVAL", &interval, sizeof(u16_t));
	
	/* update adv interval based on current adv state */
	ble_exec_adv_op(app_cb.adv_state);

	at_cmd_response("OK+ADIT-");
	at_cmd_response(p_para);
	at_cmd_response("\r\n");
	return 0;
}

int at_cmd_adv_interval_get(void)
{
	char ret[30];
	u8_t i;
	u16_t interval = app_adv_cfg.interval_min * 625 / 1000;

	for(i = 0; i < ARRAY_SIZE(adv_interval_array); i++) {
		if(interval == adv_interval_array[i]) 
			break;
	}
	snprintf_rom(ret, 30, "OK+ADIT-%d\r\n", i);
	at_cmd_response(ret);
	return 0;
}

/* 4. name data of adv */
int at_cmd_adv_name_set(const char *p_para)
{
	int len = strlen(p_para);
	u8_t cp_len = (len > BLE_ADV_NAME_DATA_MAX) ? BLE_ADV_NAME_DATA_MAX : len;
	memcpy(adv_name_data, p_para, cp_len);
	adv_name_data[BLE_ADV_NAME_DATA_MAX] = cp_len;
	app_scan_data[0].data = (const u8_t *)adv_name_data;
	app_scan_data[0].data_len = cp_len;
	
	nvram_config_set("ADV_NAME", adv_name_data, BLE_ADV_NAME_DATA_MAX+1);

	/* update adv interval based on current adv state */
	ble_exec_adv_op(app_cb.adv_state);
	
	at_cmd_response("OK+NAME\r\n");
	return 0;
}

/* 5. baudrate of uart */
int at_cmd_adv_baudrate_set(const char *p_para)
{
	u8_t index = atoi(p_para);
	u32_t baudrate;
	
	if (!is_decimal_string(p_para) ||
		(index > 4)) {
		at_cmd_response_error();
		return 0;	
	}
	
	baud_index = index;
	baudrate = uart_baudrate[index];
	printk("baud_index=%d, baudrate=%d\n", baud_index, baudrate);
	nvram_config_set("UART_BAUDRATE", &baud_index, sizeof(u8_t));
	at_cmd_response("OK+BAUD-");
	at_cmd_response(p_para);
	at_cmd_response("\r\n");
	
	/* set buadrate after send response */
	k_sleep(100);
	uart_baudrate_set(baudrate);
	return 0;
}

int at_cmd_adv_baudrate_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+BAUD-%d\r\n", baud_index);
	at_cmd_response(ret);
	return 0;
}

/* 6. interval of conn */
int at_cmd_conn_interval_set(const char *p_para)
{
	u16_t interval;
	u8_t inteval_index = atoi(p_para);
	if (!is_decimal_string(p_para) || 
		(inteval_index > 8)) {
		at_cmd_response_error();
		return 0;
	}

	interval = conn_interval_array[inteval_index] * 100 / 125;
	
	printk("interval : %d, %d\n", interval, conn_interval_array[inteval_index]);

	app_conn_update_cfg.interval_min = interval;
	app_conn_update_cfg.interval_max = interval;
	nvram_config_set("CONN_INTERVAL", &interval, sizeof(u16_t));
	at_cmd_response("OK+CNIT-");
	at_cmd_response(p_para);
	at_cmd_response("\r\n");
	return 0;
}

int at_cmd_conn_interval_get(void)
{
	char ret[30];
	u8_t i;
	u16_t interval = app_conn_update_cfg.interval_min * 125 / 100;

	for(i = 0; i < ARRAY_SIZE(conn_interval_array); i++) {
		if(interval == conn_interval_array[i]) 
			break;
	}
	snprintf_rom(ret, 30, "OK+CNIT-%d\r\n", i);
	at_cmd_response(ret);
	return 0;
}

/* 7. tx power of conn */
int at_cmd_tx_power_set(const char *p_para)
{
	u8_t tx_power_index = atoi(p_para);
	
	if (!is_decimal_string(p_para) ||
		(tx_power_index > 7)) {
		at_cmd_response_error();
		return 0;
	}
		
	tx_power_level_index = tx_power_index;
	printk("tx_power_level_index=%d\n", tx_power_level_index);
	nvram_config_set("TX_POWER", &tx_power_level_index, sizeof(u8_t));
		
	at_cmd_response("OK+TXPW-");
	at_cmd_response(p_para);
	at_cmd_response("\r\n");
	
	k_sleep(100);
	sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	return 0;
}

int at_cmd_tx_power_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+TXPW-%d\r\n", tx_power_level_index);
	at_cmd_response(ret);
	return 0;
}

/* 8. bt addr */
int at_cmd_bt_addr(void)
{
	at_cmd_response(app_bt_addr_str);
	at_cmd_response("\r\n");
	
	return 0;
}

/* 9. data of adv */
int at_cmd_adv_data_set(const char *p_para)
{
	int len = strlen(p_para);
	int cp_len = (len > (BLE_ADV_USER_DATA_LEN * 2)) ? (BLE_ADV_USER_DATA_LEN * 2) : len;
	int i, j;

	memset(adv_user_data, 0, BLE_ADV_USER_DATA_LEN);
	for (i = 0, j = 0; i < (cp_len - 1); i += 2) {
		u8_t tmp0, tmp1;
		char2hex(&p_para[i],&tmp0);
		char2hex(&p_para[i+1],&tmp1);
		adv_user_data[j++] = (tmp0 << 4) + tmp1;
	}
	app_adv_data[1].data = (const u8_t *)adv_user_data;
	app_adv_data[1].data_len = BLE_ADV_USER_DATA_LEN;
	nvram_config_set("ADV_USER_DATA", adv_user_data, BLE_ADV_USER_DATA_LEN);
	
	/* update adv interval based on current adv state */
	ble_exec_adv_op(app_cb.adv_state);
	
	at_cmd_response("OK+MAFD\r\n");
	return 0;
}

int at_cmd_adv_data_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "%02x%02x%02x%02x%02x%02x\r\n", 
									app_adv_data[1].data[5],app_adv_data[1].data[4],
									app_adv_data[1].data[3],app_adv_data[1].data[2],
									app_adv_data[1].data[1],app_adv_data[1].data[0]);
	at_cmd_response(ret);
	return 0;
}

/* 10. uuid of data srv */
int at_cmd_data_srv_uuid_set(const char *p_para)
{
	u16_t uuid_val = strtoul(p_para, NULL, 16);

	if(!is_hex_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	data_srv_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	data_srv_uuid.val[12] = uuid_val & 0xFF;
	nvram_config_set("DATA_SRV_UUID", &uuid_val, sizeof(u16_t));
	at_cmd_response("OK+SERN\r\n");
	return 0;
}

int at_cmd_data_srv_uuid_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+SERN-%02x%02x\r\n", data_srv_uuid.val[13], data_srv_uuid.val[12]);
	at_cmd_response(ret);
	return 0;
}

/* 11. uuid of drv srv */
int at_cmd_drv_srv_uuid_set(const char *p_para)
{
	u16_t uuid_val = strtoul(p_para, NULL, 16);

	if(!is_hex_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	drv_srv_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	drv_srv_uuid.val[12] = uuid_val & 0xFF;
	nvram_config_set("DRV_SRV_UUID", &uuid_val, sizeof(u16_t));
	at_cmd_response("OK+SERD\r\n");
	return 0;
}

int at_cmd_drv_srv_uuid_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+SERD-%02x%02x\r\n", drv_srv_uuid.val[13], drv_srv_uuid.val[12]);
	at_cmd_response(ret);
	return 0;
}

/* 12. uuid of data tx char */
int at_cmd_data_tx_char_uuid_set(const char *p_para)
{
	u16_t uuid_val = strtoul(p_para, NULL, 16);
	
	if(!is_hex_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	tx_char_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	tx_char_uuid.val[12] = uuid_val & 0xFF;
	nvram_config_set("TX_UUID", &uuid_val, sizeof(u16_t));
	at_cmd_response("OK+CHAT\r\n");
	return 0;
}

int at_cmd_data_tx_char_uuid_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+CHAT-%02x%02x\r\n", tx_char_uuid.val[13], tx_char_uuid.val[12]);
	at_cmd_response(ret);
	return 0;
}

/* 13. uuid of data rx char */
int at_cmd_data_rx_char_uuid_set(const char *p_para)
{
	u16_t uuid_val = strtoul(p_para, NULL, 16);

	if(!is_hex_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	rx_char_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	rx_char_uuid.val[12] = uuid_val & 0xFF;
	nvram_config_set("RX_UUID", &uuid_val, sizeof(u16_t));
	at_cmd_response("OK+CHAR\r\n");
	return 0;
}

int at_cmd_data_rx_char_uuid_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+CHAR-%02x%02x\r\n", rx_char_uuid.val[13], rx_char_uuid.val[12]);
	at_cmd_response(ret);
	return 0;
}

/* 14. uuid of drv adc char */
int at_cmd_drv_adc_char_uuid_set(const char *p_para)
{
	u16_t uuid_val = strtoul(p_para, NULL, 16);

	if(!is_hex_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	adc_char_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	adc_char_uuid.val[12] = uuid_val & 0xFF;
	drv_srv_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	drv_srv_uuid.val[12] = uuid_val & 0xFF;
	nvram_config_set("ADC_UUID", &uuid_val, sizeof(u16_t));
	at_cmd_response("OK+CHAD\r\n");
	return 0;
}

int at_cmd_drv_adc_char_uuid_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+CHAD-%02x%02x\r\n", adc_char_uuid.val[13], adc_char_uuid.val[12]);
	at_cmd_response(ret);
	return 0;
}

/* 15. uuid of drv pwm char */
int at_cmd_drv_pwm_char_uuid_set(const char *p_para)
{
	u16_t uuid_val = strtoul(p_para, NULL, 16);

	if(!is_hex_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	pwm_char_uuid.val[13] = (uuid_val >> 8) & 0xFF;
	pwm_char_uuid.val[12] = uuid_val & 0xFF;
	nvram_config_set("PWM_UUID", &uuid_val, sizeof(u16_t));
	at_cmd_response("OK+CHAP\r\n");
	return 0;
}

int at_cmd_drv_pwm_char_uuid_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+CHAP-%02x%02x\r\n", pwm_char_uuid.val[13], pwm_char_uuid.val[12]);
	at_cmd_response(ret);
	return 0;
}

/* 16. version information */
int at_cmd_ver_info(void)
{
	at_cmd_response("OK+CODV-");
	at_cmd_response(sys_version_get());
	at_cmd_response("\r\n");
	return 0;
}

/* 17. rssi  */
int at_cmd_rssi_set(const char *p_para)
{
	u8_t param = atoi(p_para);
	char ret[30];
	s8_t rssi = 0;
	struct app_msg  msg = {0};
	
	if(!is_decimal_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	switch(param) {
	case 0:	
		app_cb.rssi_mode = 0;
		nvram_config_set("RSSI_MODE", &app_cb.rssi_mode, sizeof(u8_t));
		rssi = 0;
		msg.type = MSG_EXIT_RSSI_MODE;
		msg.value = 0;
		send_msg(&msg, K_MSEC(100));
	break;
	case 1:
		app_cb.rssi_mode = 1;
		nvram_config_set("RSSI_MODE", &app_cb.rssi_mode, sizeof(u8_t));
		rssi = 1;
	break;	
	case 2:
		if (app_cb.pconn != NULL) {
		rssi = hci_read_rssi(app_cb.pconn);
		}
		break;
	default:
		at_cmd_response_error();
		return 0;
	}
	
	snprintf_rom(ret, 30, "OK+RSSI-%d\r\n", rssi);
	at_cmd_response(ret);
	return 0;
}

int at_cmd_rssi_get(void)
{
	char ret[30];
	s8_t rssi = 0;

	if (app_cb.pconn != NULL) {
		rssi = hci_read_rssi(app_cb.pconn);
	}
	snprintf_rom(ret, 30, "OK+RSSI-%d\r\n", rssi);
	at_cmd_response(ret);
	return 0;
}

/* 18. RTS func choose */
int at_cmd_rts_func_set(const char *p_para)
{
	u8_t enable = atoi(p_para);

	if(!is_decimal_string(p_para)) {
		at_cmd_response_error();
		return 0;
	}
	if (enable)
		rts_func_enable = 1;
	else
		rts_func_enable = 0;
	nvram_config_set("RTS", &rts_func_enable, sizeof(u8_t));
	at_cmd_response("OK+CRTS-");
	at_cmd_response(p_para);
	at_cmd_response("\r\n");
	return 0;
}

int at_cmd_rts_func_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "OK+CRTS-%d\r\n", rts_func_enable);
	at_cmd_response(ret);
	return 0;
}

void app_cfg_event_handle(u32_t event)
{
	/* if exit rssi mode and current is conn state, enter into data mode */
	if ((app_cb.rssi_mode == 0) && (app_cb.pconn != NULL)) { 
		exit_out_at_cmd_mode();
		enter_into_data_mode();
	}
}
