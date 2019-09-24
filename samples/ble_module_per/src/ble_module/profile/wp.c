/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "msg_manager.h"

int wp_send_input_report(struct bt_conn *conn, u8_t reportId, u16_t len, u8_t *pValue);
/* Custom Service Variables */
struct bt_uuid_128 data_srv_uuid = BT_UUID_INIT_128(
	0x99, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
	0x93, 0xf3, 0xa3, 0xb5, DATA_SRV_UUID_L_DEF, DATA_SRV_UUID_H_DEF, 0x40, 0x69);

struct bt_uuid_128 rx_char_uuid = BT_UUID_INIT_128(
	0x99, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
	0x93, 0xf3, 0xa3, 0xb5, TX_UUID_L_DEF, TX_UUID_H_DEF, 0x40, 0x69);

struct bt_uuid_128 tx_char_uuid = BT_UUID_INIT_128(
	0x99, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
	0x93, 0xf3, 0xa3, 0xb5, RX_UUID_L_DEF, RX_UUID_H_DEF, 0x40, 0x69);

struct bt_uuid_128 drv_srv_uuid = BT_UUID_INIT_128(
	0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
	0x93, 0xf3, 0xa3, 0xb5, DRV_SRV_UUID_L_DEF, DRV_SRV_UUID_H_DEF, 0x51, 0x7f);

struct bt_uuid_128 adc_char_uuid = BT_UUID_INIT_128(
	0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
	0x93, 0xf3, 0xa3, 0xb5, ADC_UUID_L_DEF, ADC_UUID_H_DEF, 0x51, 0x7f);

struct bt_uuid_128 pwm_char_uuid = BT_UUID_INIT_128(
	0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
	0x93, 0xf3, 0xa3, 0xb5, PWM_UUID_L_DEF, PWM_UUID_H_DEF, 0x51, 0x7f);

static struct bt_gatt_ccc_cfg tx_char_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg adc_char_ccc_cfg[BT_GATT_CCC_MAX] = {};

static u8_t data_value[256] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static struct bt_gatt_attr data_attrs[];
static struct bt_gatt_attr drv_attrs[];

static u8_t rx_ntf_enable;
static u8_t drv_adc_ntf_enable;

static void data_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	uint8_t id = attr - data_attrs;
	rx_ntf_enable = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("data_attr %p(id=%d) changed value: %d\n", attr, id, value);
}

static void drv_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	uint8_t id = attr - drv_attrs;
	drv_adc_ntf_enable = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("drv_attr %p(id=%d) changed value: %d\n", attr, id, value);
}
extern int ble_send_data(u8_t *data, u16_t len);
extern int act_data_send_data(const u8_t *data, int len);

static ssize_t rx_write_cb(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	if (offset + len > sizeof(data_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	act_data_send_data(buf, len);

	printk("received data: %u\n", len);

	return len;
}

static ssize_t adc_write_cb(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	struct app_msg  msg = {0};
	
	if (offset + len > sizeof(data_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	if ((len == 2) && (((u8_t*)buf)[0] == 0xAD)) {
			msg.type = MSG_ADC;
			msg.value = ((u8_t*)buf)[1];
			send_msg(&msg, K_NO_WAIT);
	}

	return len;
}

static ssize_t pwm_write_cb(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	if (offset + len > sizeof(data_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	if ((len == 6) && (((u8_t*)buf)[0] == 0x97)) {
				void pwm_cmd_handle(u8_t *cmd_buf);
				pwm_cmd_handle((u8_t*)buf);
	}

	return len;
}


/* Vendor Primary Service Declaration */
static struct bt_gatt_attr data_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&data_srv_uuid),

	BT_GATT_CHARACTERISTIC(&rx_char_uuid.uuid, BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&rx_char_uuid.uuid, BT_GATT_PERM_WRITE, NULL, rx_write_cb, NULL),

	BT_GATT_CHARACTERISTIC(&tx_char_uuid.uuid, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(&tx_char_uuid.uuid, BT_GATT_PERM_READ, NULL, NULL, data_value),
	BT_GATT_CCC(tx_char_ccc_cfg, data_ccc_cfg_changed),
};

static struct bt_gatt_attr drv_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&drv_srv_uuid),

	BT_GATT_CHARACTERISTIC(&adc_char_uuid.uuid, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&adc_char_uuid.uuid, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, adc_write_cb, NULL),
	BT_GATT_CCC(adc_char_ccc_cfg, drv_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(&pwm_char_uuid.uuid,BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&pwm_char_uuid.uuid, BT_GATT_PERM_WRITE, NULL, pwm_write_cb, data_value),
};


static struct bt_gatt_service data_svc = BT_GATT_SERVICE(data_attrs);
static struct bt_gatt_service drv_svc = BT_GATT_SERVICE(drv_attrs);

__init_once_text void wp_init(void)
{
	bt_gatt_service_register(&data_svc);
	bt_gatt_service_register(&drv_svc);
}

#define DRV_ADC_REPORT_ID	1
#define DATA_TX_REPORT_ID	2
int wp_send_input_report(struct bt_conn *conn, u8_t reportId, u16_t len, u8_t *pValue)
{
	if (reportId == DRV_ADC_REPORT_ID) {
		/* drv data notify enabled */
		if (!drv_adc_ntf_enable)
			return -EIO;

		return bt_gatt_notify(conn, &drv_attrs[2], pValue, len);
	} else if (reportId == DATA_TX_REPORT_ID) {
		/* rx notify enabled */
		if (!rx_ntf_enable)
			return -EIO;

		return bt_gatt_notify(conn, &data_attrs[4], pValue, len);
	} else
		return -EIO;
}
