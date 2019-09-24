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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/storage.h>
#include "conn_internal.h"
#include "keys.h"
#include <input_dev.h>

#include "hci_core.h"
#include "hci_core_patch.h"

#include <dis.h>
#include <bas.h>
#include <wp.h>
#include <ota.h>
#include "msg_manager.h"
#include "soc.h"
#include "soc_pm.h"

#include "at_cmd.h"
#include "act_data.h"
#include "nvram_config.h"
#include <led_manager.h>
#include "app_ble.h"
#include "app_gpio.h"
#include "app_batt.h"

struct app_cb_t app_cb = {
	.pconn = NULL,
	.rssi_mode = BLE_RSSI_MODE_DEF, 
	.adv_state = BLE_ADV_STATE_DEF,
	.user_force_to_sleep_flag = 0,
};

struct bt_le_conn_param app_conn_update_cfg = {
	.interval_min = (BLE_CONN_INTERVAL_DEF),
	.interval_max = (BLE_CONN_INTERVAL_DEF),
	.latency = (0),
	.timeout = (2000),
};

struct bt_le_adv_param app_adv_cfg = {
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
	.interval_min = BLE_ADV_INTERVAL_DEF,
	.interval_max = BLE_ADV_INTERVAL_DEF,
};

u8_t adv_user_data[BLE_ADV_USER_DATA_LEN] = {BLE_ADV_USER_DATA_DEF};
struct bt_data app_adv_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, adv_user_data, sizeof(adv_user_data)),
};

u8_t adv_name_data[BLE_ADV_NAME_DATA_MAX+1] = {BLE_ADV_NAME_DATA_DEF};
struct bt_data app_scan_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, adv_name_data, BLE_ADV_NAME_LEN_DEF),
};


int at_addr_le_to_str2(const bt_addr_le_t *addr, char *str,
			size_t len);

/* send a message to main thread */
static void app_ble_send_msg_to_app(u8_t event_code, int status)
{
	struct app_msg msg = {0};

	msg.type = MSG_BLE_EVENT;
	msg.cmd = event_code;
	msg.value = status;
	send_msg(&msg, K_MSEC(1000));
	printk("app_ble send event_code %d\n", event_code);
}
static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	app_cb.pconn = conn;

	/* exit at_cmd mode and enter into data mode */
	if (!app_cb.rssi_mode) { /* if current state is rssi mode, don't enter into data mode */
		exit_out_at_cmd_mode();
		enter_into_data_mode();
	}
	printk("Connected %s", addr);

	/* output high pluse to pin */
	gpio_output_notify_state(true);
}

static void le_identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				 const bt_addr_le_t *identity)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(rpa, addr, sizeof(addr));
	printk("le_identity_resolved rpa %s\n", addr);
	bt_addr_le_to_str(identity, addr, sizeof(addr));
	printk("le_identity_resolved identity %s\n", addr);
}

extern u8_t ota_need_reset;
static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	app_cb.pconn = NULL;
	printk("Disconnected from %s (reason %u)\n", addr, reason);
	rx_stack_analyze();
	
	/* exit data mode and enter into at_cmd mode */
	exit_out_data_mode();
	enter_into_at_cmd_mode();

	/* output low pluse to pin */
	gpio_output_notify_state(false);
	
	/* send a message to main thread */
	app_ble_send_msg_to_app(BLE_CONN_CLOSE_IND, reason);

#if CONFIG_OTA_WITH_APP
	/* reset when ota need */
	if (ota_need_reset) {
		sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	}
#endif	
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Security changed: %s level %u\n", addr, level);

}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval, latency, timeout);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.identity_resolved = le_identity_resolved,
	.security_changed = security_changed,
	.le_param_updated = le_param_updated,
};

u8_t bt_ready_flag;
u8_t overlay_ready_flag;
const u8_t dis_val_pnp[7] = {
	CONFIG_DIS_PNP_COMPANY_ID_TYPE,
	CONFIG_DIS_PNP_VENDOR_ID,
	CONFIG_DIS_PNP_PRODUCT_ID,
	CONFIG_DIS_PNP_PRODUCT_VERSION
};

#if CONFIG_OTA_WITH_APP
struct bt_gatt_ccc_cfg  blvl_ccc_cfg[BT_GATT_CCC_MAX];	
#endif

__init_once_text void bt_ready(int err)
{
	/* don't enter into deepsleep when bt stack init */
	app_release_wake_lock();

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* init dis service */
	dis_init(CONFIG_DIS_MODEL, CONFIG_DIS_MANUFACTURER_NAME, dis_val_pnp);

	/* init hid/wp service */
	wp_init();
	
	/* init ota profile */
#if CONFIG_OTA_WITH_APP
	/* init bas service */
	bas_init(adc_batt_read, blvl_ccc_cfg, BT_GATT_CCC_MAX);
	ota_profile_init();
#endif
	
	/* set conn update param for bt stack */
	bt_set_conn_update_param(K_SECONDS(1), &app_conn_update_cfg);

	/* register conn callbacks into bt stack */
	bt_conn_cb_register(&conn_callbacks);

	/* NOTE Make Sure All BLE profile have register into stack db */
	bt_gatt_load_ccc();
	
	extern void load_app_cfg_from_nv(void);
	load_app_cfg_from_nv();
	
	/* tell system enter into at_cmd mode */
	enter_into_at_cmd_mode();
	/* set some flag */
	bt_ready_flag = 1;
	overlay_ready_flag = 1;
	
	/* resume adv state according adv_state */
	ble_exec_adv_op(app_cb.adv_state);
}

void ble_exec_adv_op(u8_t adv_state)
{
	int err;

	bt_le_adv_stop();
	switch (adv_state) {
	case ADV_STOP:
		printk("Advertising stop\n");
		return;
	case CONN_ADV_START:
		app_adv_cfg.options |= BT_LE_ADV_OPT_CONNECTABLE;
		break;
	case NON_CONN_ADV_START:
		app_adv_cfg.options &= (~BT_LE_ADV_OPT_CONNECTABLE);
		break;
	default:
		break;
	}
	
	err = bt_le_adv_start(&app_adv_cfg, app_adv_data, ARRAY_SIZE(app_adv_data), (const struct bt_data *)app_scan_data, ARRAY_SIZE(app_scan_data));
	if (err) {
		printk("Undirect Advertising failed to start (err %d)\n", err);
		return;
	}
	
	printk("Undirect Advertising successfully started\n");
}

extern int wp_send_input_report(struct bt_conn *conn, u8_t reportId, u16_t len, u8_t *pValue);
int wp_write(u8_t *data, u16_t len);
static int count = 0;
int ble_send_data(u8_t *data, u16_t len)
{	
	int err;
	u8_t *p_data = data;
	if (app_cb.pconn != NULL) {
		while (len > 0) {
			u8_t send_len = (len > 20 ) ? 20 : len;
			err = wp_send_input_report(app_cb.pconn, 2, send_len, p_data);
			if (err != 0) return err;
			
			len -= send_len;
			p_data += send_len;
			count += send_len;
			printk("send data count %d\n",count);
		}
	}
	return 0;
}

void ble_send_drv_data(u8_t *data, u16_t len)
{	
	u8_t *p_data = data;
	if (app_cb.pconn != NULL) {
		while (len > 0) {
			u8_t send_len = (len > 20 ) ? 20 : len;
			wp_send_input_report(app_cb.pconn, 1, send_len, p_data);
			len -= send_len;
			p_data += send_len;
		}
	}
}

s8_t hci_read_rssi(struct bt_conn *conn)
{

	int err;
	struct bt_hci_cp_read_rssi *cp;
	struct bt_hci_rp_read_rssi *rp;
	struct net_buf *buf;
	struct net_buf *rsp;
	s8_t rssi;

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI,
				sizeof(*cp));
	if (!buf) {
		return 0;
	}
	
	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = conn->handle;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf,
				   &rsp);
	if (err) {
		return 0;
	}
	
	rp = (void *)rsp->data;
	
	rssi = rp->status ? 0 : rp->rssi;
	
	net_buf_unref(rsp);
	
	return rssi;
}

void clear_ble_state_before_suspend(void)
{
	int err;

	if (app_cb.pconn) {
		err = bt_conn_disconnect(app_cb.pconn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			printk("Disconnect failed (err %d)\n", err);
		}			
	} else {
		if (app_cb.adv_state != ADV_STOP) {
			bt_le_adv_stop();
			printk("stop adv for suspend\n");
		}
	}
	app_cb.user_force_to_sleep_flag = 1;
}

void set_ble_state_after_resume(void)
{
	app_cb.user_force_to_sleep_flag = 0;
	if ((app_cb.pconn == NULL) && (app_cb.adv_state != ADV_STOP)) {
		printk("start adv for resume\n");
		/* resume adv state according adv_state */
		ble_exec_adv_op(app_cb.adv_state);
	}
}

void app_ble_event_handle(u8_t event_code, void *event_data)
{
		int err;
		printk("app_ble_event_handle %d\n", event_code);
		switch (event_code) {
		case BLE_CONN_OPEN_IND:
		{
			/* TODO */
		}
		break;
		case BLE_CONN_CLOSE_IND:
		{
			/* resume adv state according adv_state except sleep_state == 0*/
			if (!app_cb.user_force_to_sleep_flag)
				ble_exec_adv_op(app_cb.adv_state);
		}
		break;

		default:
			printk("unknow ble event %d\n", event_code);
			break;
		}
}

