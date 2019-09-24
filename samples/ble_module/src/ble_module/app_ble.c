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
#include <conn_internal.h>
#include "keys.h"
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
#include "led_manager.h"
#include "app_ble.h"
#include "misc.h"
#include "qpps.h"

struct app_cb_t app_cb;
struct ble_params ble_param;

static struct bt_uuid_128 wp_uuid = BT_UUID_INIT_128(
	0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
	0x55, 0x44, 0x33, 0x22, 0xA0, 0x20, 0x11, 0x00);

static struct bt_uuid_128 qpps_uuid = BT_UUID_INIT_128(
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xe9, 0xfe, 0x00, 0x00);

struct bt_le_conn_param app_conn_update_cfg = {
	.interval_min = (BT_GAP_INIT_CONN_INT_MIN),
	.interval_max = (BT_GAP_INIT_CONN_INT_MAX),
	.latency = (0),
	.timeout = (100),
};

#define BT_LE_GAP_ADV_FAST_INT_MIN      0x0060  /* 60 ms */
#define BT_LE_GAP_ADV_FAST_INT_MAX      0x0068  /* 65 ms */

struct bt_le_adv_param app_adv_cfg = {
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
	.interval_min = BT_LE_GAP_ADV_FAST_INT_MIN,
	.interval_max = BT_LE_GAP_ADV_FAST_INT_MAX,
};

struct bt_le_scan_param app_scan_cfg = {
	.type       = BT_HCI_LE_SCAN_ACTIVE,  
	.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
	.interval   = BT_GAP_SCAN_FAST_INTERVAL,
	.window     = BT_GAP_SCAN_FAST_WINDOW
};

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_SHORTENED, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_SOME, 
#if !CONFIG_USE_QPPS_PROFILE
		0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
		0x55, 0x44, 0x33, 0x22, 0xA0, 0x20, 0x11, 0x00),
#else
		0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
		0x00, 0x10, 0x00, 0x00, 0xe9, 0xfe, 0x00, 0x00),
#endif
};

int ad_data_set(u8_t type, u8_t *data, u8_t data_len) 
{
	int i;
	int size = ARRAY_SIZE(ad);
	
	for (i = 0; i < size; i++) {
		if (ad[i].type == type) {
			break;
		}
	}
	
	if (i == size) {
		return -1;
	}

	ad[i].data = (const u8_t *)data;
	ad[i].data_len = data_len;

	return 0;
}

extern struct bt_conn *conn_tmp;
static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char ret[30];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	conn_tmp = NULL;
	
	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		at_cmd_response_error();
#if CONFIG_AUTO_RECONNECT		
		int ret;
		char target_addr_str[] = BLE_TARGET_ADDR_DFLT;
		printk("Failed to connect to %s (%u)\n", addr, err);
		ret = nvram_config_get_factory("BT_TGT_ADDR", target_addr_str, strlen(target_addr_str));
		if (ret >= 0 && strcmp(target_addr_str, BLE_TARGET_ADDR_DFLT)) {
			send_async_msg(MSG_RECONNECTING);
		}
#endif		
		return;
	}

	app_cb.pconn = conn;
	app_cb.work_state = 1;

	/* enter into data mode */
	enter_into_data_mode();
	printk("Connected %s\n", addr);
	
	ble_state_update(CONNECTED);
	if (ble_param.role == BLE_ROLE_SLAVE) {
		at_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
		snprintf_rom(ret, 30, "+CONNECTED>>0x%s\r\n", addr);
		at_cmd_response(ret);	
	} else {
		at_cmd_response("+CONNECTED\r\n");
	}
	
	if (BLE_ROLE_MASTER == ble_param.role) {
		at_addr_le_to_str2(bt_conn_get_dst(conn), addr, sizeof(addr));
		bt_addr_le_copy(&ble_param.targed_addr, bt_conn_get_dst(conn));
		nvram_config_set_factory("BT_TGT_ADDR", addr, strlen(addr));	
		send_async_msg(MSG_CONNECTED);
	}

	/* indicate connected state with always on led after close disconnected state led */
	led_set_state(LED_STATE_IND_PIN, LED_OFF, 300);
	led_set_state(LED_STATE_IND_PIN, LED_ON, 0);

	/* output high pluse to pin */
	led_set_state(CONN_STATE_IND_PIN, LED_ON, 0);	
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
void reset_subscribe_params(void);
static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	app_cb.pconn = NULL;
	printk("Disconnected from %s (reason %u)\n", addr, reason);
	rx_stack_analyze();
	
	app_cb.work_state = 0;
	conn_tmp = NULL;

	/* enter into at_cmd mode */
	enter_into_at_cmd_mode();

	at_cmd_response("+DISCONNECTED\r\n");
	
	ble_state_update(READY);
	if (BLE_ROLE_MASTER == ble_param.role) {
#if CONFIG_AUTO_RECONNECT		
		if (bt_addr_le_cmp(&ble_param.targed_addr, BT_ADDR_LE_ANY)){
			printk("Reconnecting...\n");
			send_async_msg(MSG_RECONNECTING);
		}
#endif		
		reset_subscribe_params();
	} else {
		if (!(app_adv_cfg.options & BT_LE_ADV_OPT_ONE_TIME)) {
			ble_state_update(ADVERTISING);
		}
	}

	/* indicate disconnected state with 300ms flash after close connected state led */
	led_set_state(LED_STATE_IND_PIN, LED_OFF, 0);
	led_set_state(LED_STATE_IND_PIN, LED_ON, 300);

	/* output low pluse to pin */
	led_set_state(CONN_STATE_IND_PIN, LED_OFF, 0);	
	
#if CONFIG_OTA_WITH_APP
	/* reset when ota need */
	if (ota_need_reset) {
		sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	}
#endif	

	send_async_msg(MSG_DISCONNECTED);
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

void load_ble_cfg_from_nv(void);

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
	memset(&app_cb, 0, sizeof(app_cb));

	/* init dis service */
	dis_init(CONFIG_DIS_MODEL, CONFIG_DIS_MANUFACTURER_NAME, dis_val_pnp);

#if CONFIG_USE_QPPS_PROFILE
	/* QPPS */
	qpps_init();
#else
	/* WP */
	wp_init();
#endif

	/* init ota profile */
#if CONFIG_OTA_WITH_APP
	/* init bas service */
	bas_init(NULL, blvl_ccc_cfg, BT_GATT_CCC_MAX);
	ota_profile_init();
#endif
	
	/* there is no necessary for module to set conn update param */
	//bt_set_conn_update_param(K_SECONDS(5), &app_conn_update_cfg);

	/* register conn callbacks into bt stack */
	bt_conn_cb_register(&conn_callbacks);

	/* NOTE Make Sure All BLE profile have register into stack db */
	//bt_gatt_load_ccc();

	/* load configuration form nvram */
	load_ble_cfg_from_nv();
	
	/* tell system enter into at_cmd mode */
	enter_into_at_cmd_mode();

	/* set ble state */
	ble_state_update(INITIALIZED);

	/* set some flag */
	bt_ready_flag = 1;
	overlay_ready_flag = 1;

#if CONFIG_BLE_ROLE_MASTER
	ble_param.role = BLE_ROLE_MASTER;
#else
	ble_param.role = BLE_ROLE_SLAVE;
#endif

	if (ble_param.role == BLE_ROLE_SLAVE) {
		start_adv();
	}else {
#if CONFIG_AUTO_RECONNECT		
		if (ble_param.auto_conn  && 	
				bt_addr_le_cmp(&ble_param.targed_addr, BT_ADDR_LE_ANY)){
			send_async_msg(MSG_RECONNECTING);
		}
#endif
	}	
}

/* if get cfg from nvram failed, the value will set to default */
void load_ble_cfg_from_nv(void)
{
	int err;
	u8_t role, auto_conn;;
	char adv_name[BLE_NAME_MAX_LEN] = {0};
	
	bt_addr_le_t bt_addr;
	bt_addr.type = BT_ADDR_LE_PUBLIC;
	
	/* set to default value */
	char bt_addr_str[] = BLE_DEVICE_ADDR_DFLT;
	char target_addr_str[] = BLE_TARGET_ADDR_DFLT;
	ble_param.role = BLE_ROLE_DFLT;
	memcpy(ble_param.adv_name, BLE_ADV_NAME_DFLT, sizeof(BLE_ADV_NAME_DFLT));

	/* restore value form nvram */
	/* role */
	err = nvram_config_get_factory("BT_ROLE", &role, 1);
	if (err >= 0) {
		printk("role=%d\n",  role);		
		if (role == BLE_ROLE_SLAVE || role == BLE_ROLE_MASTER) {
			ble_param.role = role;		
		}	
	}
	
	/* autoconn */
	err = nvram_config_get_factory("BT_AUTOCONN", &auto_conn, 1);
	if (err >= 0) {
		printk("auto_conn=%d\n", auto_conn);		
		if (auto_conn == 0 || auto_conn == 1) {
			ble_param.auto_conn = auto_conn;
		}	
	}
	
	/* adv name */
	err = nvram_config_get_factory("BT_NAME", adv_name, BLE_NAME_MAX_LEN);
	adv_name[BLE_NAME_MAX_LEN -1] = '\0';
	if (err >= 0) {
		printk("adv_name=%s\n", adv_name);		
		memcpy(ble_param.adv_name, adv_name, sizeof(adv_name));
		ad_data_set(BT_DATA_NAME_SHORTENED, (u8_t *)ble_param.adv_name,
				strlen(adv_name));
	}
	
	/* bt addr */
	err = nvram_config_get_factory("BT_ADDR", bt_addr_str, strlen(bt_addr_str));
	if (err < 0) {
		nvram_config_set_factory("BT_ADDR", bt_addr_str, strlen(bt_addr_str));
	} else {
		printk("bt_addr=%s\n", bt_addr_str);
	}
	str2bt_addr(bt_addr_str, &bt_addr.a);
	bt_addr_le_copy(&ble_param.dev_addr, &bt_addr);
	
	/* target addr */
	err = nvram_config_get_factory("BT_TGT_ADDR", target_addr_str, strlen(target_addr_str));
	if (err >= 0) {
		printk("target_addr=%s\n", target_addr_str);
	}	
	str2bt_addr(target_addr_str, &bt_addr.a);
	bt_addr_le_copy(&ble_param.targed_addr, &bt_addr);
}

void leds_status_reset(void)
{
	/* indicate disconnected state with 300ms flash */
	led_set_state(LED_STATE_IND_PIN, LED_ON, 300);

	/* output low to pin */
	led_set_state(CONN_STATE_IND_PIN, LED_OFF, 0);	
}

struct bt_conn *bt_le_conn_lookup_addr(const bt_addr_le_t *peer)
{
	extern struct bt_conn *p_conns;
	extern u16_t bt_conn_num;
	int i;

	for (i = 0; i < bt_conn_num; i++) {
		if (!atomic_get(&p_conns[i].ref)) {
			continue;
		}

		if (p_conns[i].type != BT_CONN_TYPE_LE) {
			continue;
		}

		if (!bt_conn_addr_le_cmp(&p_conns[i], peer)) {
			return &p_conns[i];
		}
	}

	return NULL;
}

extern int start_le_scan(u8_t scan_type, u16_t interval, u16_t window);
extern int set_le_scan_enable(u8_t enable);
extern void bt_conn_set_param_le(struct bt_conn *conn,
				 const struct bt_le_conn_param *param);

#define BT_CREATE_LE_TIMEOUT    K_SECONDS(1)
#define CONFIG_BT_CENTRAL       1

static int bt_le_scan_update_new(bool fast_scan)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return 0;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		err = set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		u16_t interval, window;
		struct bt_conn *conn;

		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT_SCAN);
		if (!conn) {
			return 0;
		}

		atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);

		bt_conn_unref(conn);

		if (fast_scan) {
			interval = BT_GAP_SCAN_FAST_INTERVAL;
			window = BT_GAP_SCAN_FAST_INTERVAL;
		} else {
			interval = BT_GAP_SCAN_SLOW_INTERVAL_1;
			window = BT_GAP_SCAN_SLOW_WINDOW_1;
		}
		return start_le_scan(BT_HCI_LE_SCAN_PASSIVE, interval, window);
	}

	return 0;
}

void create_le_timeout(void)
{
	set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
}

void create_le_timer_handler(struct k_timer *timer)
{
	send_async_msg(MSG_CREATE_LE_TIME_OUT);
}

K_TIMER_DEFINE(create_le_timer, create_le_timer_handler, NULL);

struct bt_conn *bt_conn_create_le_new(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param)
{
	struct bt_conn *conn;

	if (!bt_le_conn_params_valid(param)) {
		return NULL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return NULL;
	}
	conn = bt_conn_lookup_addr_le(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_CONNECT_SCAN:
			if (!atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
				bt_le_scan_update_new(true);
			}
			k_timer_start(&create_le_timer, BT_CREATE_LE_TIMEOUT, 0);
			bt_conn_set_param_le(conn, param);
			return conn;
		case BT_CONN_CONNECT:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}
	conn = bt_conn_add_le(peer);
	if (!conn) {
		return NULL;
	}

	/* Set initial address - will be updated later if necessary. */
	bt_addr_le_copy(&conn->le.resp_addr, peer);

	bt_conn_set_param_le(conn, param);

	bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);

	bt_le_scan_update_new(true);
	
	k_timer_start(&create_le_timer, BT_CREATE_LE_TIMEOUT, 0);

	return conn;
}

void start_adv(void)
{
	int err;

	bt_le_adv_stop();
	err = bt_le_adv_start(&app_adv_cfg, ad, ARRAY_SIZE(ad), (const struct bt_data *)sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Undirect Advertising failed to start (err %d)\n", err);
		return;
	}

	ble_state_update(ADVERTISING);
	printk("Undirect Advertising successfully started\n");
}

void disconnect(void)
{
	int err;
	if (app_cb.pconn) {
		err = bt_conn_disconnect(app_cb.pconn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			printk("Disconnect failed (err %d)\n", err);
		}
	} else {
		if (conn_tmp) {
			bt_conn_disconnect(conn_tmp, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			send_async_msg(MSG_CREATE_LE_TIME_OUT);
		} else {
			at_cmd_response("+DISCONNECTED\r\n");
		}
		/* make sure the disconnected msg can be send */
		send_async_msg(MSG_DISCONNECTED);
	}
}

static int get_dev_index(const bt_addr_le_t *addr)
{
	int i;
	for (i = 0; i < ble_param.dev_cnt; i++) {
		if (!bt_addr_le_cmp(&ble_param.dev_list[i].addr, addr)) {
			return i;
		}
	}
	
	return -1;
}

int get_max_riss_dev_index(void)
{
	int i;
	int rssis[DEV_TOTAL] = {0};
	
	for (i = 0; i < ble_param.dev_cnt; i++) {
		ble_param.dev_list[i].rssi_av = average(ble_param.dev_list[i].rssi, ble_param.dev_list[i].rssi_cnt);
		rssis[i] = ble_param.dev_list[i].rssi_av;
		printk("rssis[%d] = %d\n", i, rssis[i]);
	}
	
	bubble_sort(rssis, ble_param.dev_cnt);
	
	for (i = 0; i < ble_param.dev_cnt; i++) {
		if (rssis[0] == ble_param.dev_list[i].rssi_av) {
			return i;
		}
	}
	
	return -1;
}

/* return true if device get a name */
bool add_scan_dev_list(const bt_addr_le_t *addr, s8_t rssi, char *name, u8_t name_len)
{
	char dev[BT_ADDR_LE_STR_LEN];
	int index;
	u8_t rssi_cnt;
	
	at_addr_le_to_str(addr, dev, sizeof(dev));

	index = get_dev_index(addr);
	if (index < 0) {
		if (ble_param.dev_cnt < DEV_TOTAL -1) {
			//printk("add new device: %s rssi %d\n", dev, rssi);
			
			bt_addr_le_copy(&ble_param.dev_list[ble_param.dev_cnt].addr, addr);
			rssi_cnt = ble_param.dev_list[ble_param.dev_cnt].rssi_cnt;
			
			ble_param.dev_list[ble_param.dev_cnt].rssi[rssi_cnt] = rssi;
			ble_param.dev_list[ble_param.dev_cnt].rssi_cur_index = 0;
			ble_param.dev_list[ble_param.dev_cnt].rssi_cnt++;
			
			if (name_len) {
				memcpy(ble_param.dev_list[ble_param.dev_cnt].name, name, BLE_NAME_MAX_LEN);
				ble_param.dev_list[ble_param.dev_cnt].name[BLE_NAME_MAX_LEN - 1] = '\0';
				ble_param.dev_cnt++;
				return true;
			}
			
			ble_param.dev_cnt++;
			
			return false;		
		} else {
			printk("device list full\n");
			return false;		
		}
	} else {
		/* add rssi to rssi array */
//		printk("update device %d: rssi_cnt %d, new rssi %d\n", 
//				index, ble_param.dev_list[index].rssi_cnt, rssi);
		
		/* update rssi_cur_index */
		if (++ble_param.dev_list[index].rssi_cur_index == RSSI_NUM) {
			ble_param.dev_list[index].rssi_cur_index = 0;
		}
		
		/* insert rssi to rssi array */
		ble_param.dev_list[index].rssi[ble_param.dev_list[index].rssi_cur_index] = rssi;
			
		/* update rssi_cnt */
		if (ble_param.dev_list[index].rssi_cnt < RSSI_NUM) {
			ble_param.dev_list[index].rssi_cnt++;
		}
		
		if (name_len && !strlen(ble_param.dev_list[index].name)) {
			memcpy(ble_param.dev_list[index].name, name, BLE_NAME_MAX_LEN);
			ble_param.dev_list[index].name[BLE_NAME_MAX_LEN - 1] = '\0';
			return true;
		}
	}
	
	return false;
}

extern int at_cmd_conn_set(char *p_para);
extern struct k_timer at_scan_timer;

static bool uuid_match(const u8_t *data, u8_t len)
{
	u8_t uuid_len= 16;
	
	while (len >= 16) {
		if (
#if !CONFIG_USE_QPPS_PROFILE
			!memcmp(data, wp_uuid.val, uuid_len)
#else
			!memcmp(data, qpps_uuid.val, uuid_len)
#endif
		) {
			return true;
		}

		len -= uuid_len;
		data += uuid_len;
	}

	return false;
}

static bool ad_parse(struct net_buf_simple *ad, 
		const bt_addr_le_t *addr, char *name, u8_t name_len)
{	
	while (ad->len > 1) {
		u8_t len = net_buf_simple_pull_u8(ad);
		u8_t type;

		/* Check for early termination */
		if (len == 0) {
			return false;
		}

		if (len > ad->len || ad->len < 1) {
			return false;
		}

		type = net_buf_simple_pull_u8(ad);
		switch(type){
//		case BT_DATA_UUID128_SOME:
//		case BT_DATA_UUID128_ALL:
//			if (uuid_match(ad->data, len - 1)) {
//				return true;
//			}
//			break;
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE: 
			{
				if (strstr(ad->data, CONFIG_DEVICE_NAME_PREFIX)) {
					u8_t cp_len = (len - 1) > name_len ? 
							name_len : (len - 1);
					memcpy(name, ad->data, cp_len);	
//					if (get_dev_index(addr) >= 0) {
						return true;
//					}			
				}
			}
			break;
		}

		net_buf_simple_pull(ad, len - 1);
	}

    return false;
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t evtype,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];
	char buf[50] = {0};
	bool get_name;
	char name[BLE_NAME_MAX_LEN] = {0};
	
	at_addr_le_to_str(addr, dev, sizeof(dev));

	/* connectable undirected event type and connectable directed event type */ 
	if (addr->type != BT_ADDR_LE_PUBLIC || (evtype != BT_LE_ADV_IND && evtype != BT_LE_ADV_SCAN_RSP) 
		|| !ad_parse(ad, addr, name, BLE_NAME_MAX_LEN - 1)) { 
//		printk("Drop device: %s, addr type %d, AD event type %u, AD data len %u, RSSI %i\n",
//				dev, addr->type, evtype, ad->len, rssi);
		return;
	}
	
	//printk("Device: %s, Name %s RSSI %i\n", dev, name, rssi);

#if CONFIG_AUTO_RECONNECT
	if (ble_param.state != RECONNTING) {
#endif
		get_name = add_scan_dev_list(addr, rssi, name, strlen(name));
		if (get_name) {
			snprintf_rom(buf, 50, "+INQ:%d 0x%s %s %d\r\n", 
					ble_param.dev_cnt, dev, name, rssi);  // index actual = ble_param.dev_cnt - 1
			printk("%s", buf);
			at_cmd_response(buf);	
		}				

		if (ble_param.auto_conn) {
			if (ble_param.dev_cnt == DEV_TOTAL) {
				int index = get_max_riss_dev_index();
				if (index >= 0) {
					char index_str = index + '0';
					at_cmd_conn_set(&index_str);
				}		
			}
		} else {
			if (ble_param.dev_cnt == DEV_TOTAL) {
				k_timer_start(&at_scan_timer, K_MSEC(10), 0);
			}	
		}
#if CONFIG_AUTO_RECONNECT
	} else {
		if (rssi >= -92 && !bt_addr_le_cmp(&ble_param.targed_addr, addr)) {
			extern int at_conn_create(bt_addr_le_t * addr);
			stop_scan();
			if (at_conn_create((bt_addr_le_t *) addr)) {
				send_async_msg(MSG_RECONNECTING);
			}
		}
	}
#endif
}

int start_scan(struct bt_le_scan_param *scan_param)
{
	return bt_le_scan_start(scan_param, device_found);
}

void stop_scan(void)
{
	bt_le_scan_stop();
	ble_state_update(READY);
	printk("stop scan\n");
}

/* WP Handles */
enum wp_handles {
	WP_SVC_HDL = 0,
	WP_RX_CH_HDL,
	WP_RX_HDL,
	WP_RX_CH_CCC_HDL,
	WP_TX_CH_HDL,
	WP_TX_HDL,
	WP_CTRL_CH_HDL,
	WP_CTRL_HDL,
	WP_CTRL_CH_CCC_HDL,	
	WP_MAX_HDL
};

/* QPPS Handles */
enum qpps_handles {
	QPPS_SVC_HDL = 0,
	QPPS_TX_CH_HDL,
	QPPS_TX_HDL,
	QPPS_TX_CUD_HDL,
	QPPS_RX_CH_HDL,
	QPPS_RX_HDL,
	QPPS_RX_CH_CCC_HDL,	
	QPPS_MAX_HDL
};

#if !CONFIG_USE_QPPS_PROFILE
#define MODULE_HANDLE_COUNT     WP_MAX_HDL
#define MODULE_HANDLE_START     WP_SVC_HDL
#define MODULE_HANDLE_END       WP_CTRL_CH_CCC_HDL
#define MODULE_SERVICE_UUID_VAL wp_uuid.val
#define MODULE_TX_HANDLE        WP_TX_HDL
#else
#define MODULE_HANDLE_COUNT     QPPS_MAX_HDL
#define MODULE_HANDLE_START     QPPS_SVC_HDL
#define MODULE_HANDLE_END       QPPS_RX_CH_CCC_HDL
#define MODULE_SERVICE_UUID_VAL qpps_uuid.val
#define MODULE_TX_HANDLE        QPPS_TX_HDL
#endif

u16_t module_handle[MODULE_HANDLE_COUNT];
static struct bt_gatt_discover_params discover_params;
static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);

struct ccc_info {
	char *alias;
	u16_t handle_idx;
	struct bt_gatt_subscribe_params subscribe_params;
};

struct ccc_info module_ccc[] = {
#if !CONFIG_USE_QPPS_PROFILE
	{"rx", WP_RX_CH_CCC_HDL, {0}},
//	{"ctrl", WP_CTRL_CH_CCC_HDL, {0}},
#else
	{"rx", QPPS_RX_CH_CCC_HDL, {0}},
#endif
};

#define MODULE_CCC_COUNT  sizeof(module_ccc)/sizeof(module_ccc[0])

#if CONFIG_QUICK_CONNECT
void quick_set_handles(void)
{
    u8_t i;

	for (i = 0; i <= MOUDULE_SVC_HANDLE_END - MOUDULE_SVC_HANDLE_START; i++) {
		module_handle[i] = MOUDULE_SVC_HANDLE_START + i;
	} 		
}
#endif

#if CONFIG_MODULE_WRITE_WITH_RESPONSE
static struct bt_gatt_write_params write_params;
static void write_func(struct bt_conn *conn, u8_t err,
		       struct bt_gatt_write_params *params)
{
	memset(&write_params, 0, sizeof(write_params));
}

int module_write(u8_t *data, u16_t len)
{
	int err;
	
	if(!app_cb.pconn) {
		return -1;	
	}

	if (write_params.func) {
		printk("Write ongoing\n");
		return 0;
	}

	write_params.data = data;
	write_params.length = len;
	write_params.handle = module_handle[MODULE_TX_HANDLE];
	write_params.offset = 0;
	write_params.func = write_func;

	err = bt_gatt_write(app_cb.pconn, &write_params);
	if (err) {
		printk("Write failed (err %d)\n", err);
	} else {
		printk("Write pending\n");
	}

	return 0;
}
#else
int module_write(u8_t *data, u16_t len)
{
    int err = 0;

 	if(!app_cb.pconn) {
		return -1;	
	}

	err = bt_gatt_write_without_response(app_cb.pconn, 
			module_handle[MODULE_TX_HANDLE],data, len, 0);
    if (err) {
        printk("write failed (err %d)\n", err);
    }
	
    return err;
}
#endif

u8_t notify_func(struct bt_conn *conn,
		   struct bt_gatt_subscribe_params *params,
		   const void *data, u16_t length)
{
	static u32_t recv_cnt = 0;
	if (length) {
		if (length == 6) {
			if (((u8_t *)data)[0] == 'A' && ((u8_t *)data)[1] == 'T' && 
				((u8_t *)data)[2] == '+' && ((u8_t *)data)[3] == 'O' &&
				((u8_t *)data)[4] == 'T' && ((u8_t *)data)[5] == 'A') {
				ble_param.role = BLE_ROLE_SLAVE;
				send_async_msg(MSG_DISCONNECT);
				return BT_GATT_ITER_CONTINUE;
			}
		}
#if 0			
		int i;
		printk("%s len %d: ", module_ccc_alias(params->ccc_handle), length);
		if (length) {
			for (i = 0; i < length; i++) {
				printk("%02x ", ((u8_t *)data)[i]);
			}
			printk("\n ");
		}
#endif		
		recv_cnt += length;
		printk("notification: %d\n", recv_cnt);
		act_data_send_data(data, length);
	}

	if (!data) {
		SYS_LOG_DBG("Unsubscribed");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

void module_ccc_init(void)
{
	u16_t i;
	for(i = 0; i < MODULE_CCC_COUNT; i++) {
		module_ccc[i].subscribe_params.ccc_handle = 0;
		module_ccc[i].subscribe_params.value_handle = 0;
		module_ccc[i].subscribe_params.notify = notify_func;
		module_ccc[i].subscribe_params.value = BT_GATT_CCC_NOTIFY;
		module_ccc[i].subscribe_params.flags = BT_GATT_SUBSCRIBE_FLAG_VOLATILE;
	}
}

char *module_ccc_alias(u16_t handle) 
{
	u16_t i, ccc_handle;
	
	for(i = 0; i < MODULE_CCC_COUNT; i++) {
		ccc_handle = module_handle[module_ccc[i].handle_idx];
		if (ccc_handle == handle) {
			return module_ccc[i].alias;
		}
	}
	
	return NULL;
}

void reset_subscribe_params(void)
{
	int i = 0;
	for (i = 0; i < MODULE_CCC_COUNT; i++)
		module_ccc[i].subscribe_params.value_handle = 0;
}

struct bt_gatt_subscribe_params *module_ccc_subscribe_params(
		u16_t handle) 
{
	u16_t i, ccc_handle;

	for(i = 0; i < MODULE_CCC_COUNT; i++) {
		ccc_handle = module_handle[module_ccc[i].handle_idx];
		if (ccc_handle == handle) {
			return &module_ccc[i].subscribe_params;
		}
	}
	return NULL;
}

void nofity_enable(void)
{
	int i, err =0;
	u16_t ccc_handle;
	struct bt_gatt_subscribe_params *subscribe_params;

	if (!app_cb.pconn) {
		printk("Not connected\n");
		return;
	}
	
	/* set subscribe params */
	if ((module_ccc[0].subscribe_params.flags 
			& BT_GATT_SUBSCRIBE_FLAG_VOLATILE) 
			|| !module_ccc[0].subscribe_params.ccc_handle) {
		for(i = 0; i < MODULE_CCC_COUNT; i++) {
			module_ccc[i].subscribe_params.ccc_handle = module_handle[module_ccc[i].handle_idx];
			module_ccc[i].subscribe_params.value_handle = 0;
			module_ccc[i].subscribe_params.value = BT_GATT_CCC_NOTIFY;
		}
	}

	for(i = 0; i < MODULE_CCC_COUNT; i++) {
		ccc_handle = module_ccc[i].subscribe_params.ccc_handle;	
		subscribe_params = module_ccc_subscribe_params(ccc_handle);
		if (subscribe_params->value_handle) {
			printk("Cannot subscribe: subscription to %x already exists\n",
				subscribe_params->value_handle);
			send_async_msg(MSG_DISCONNECT);
		}
		
		subscribe_params->value_handle = ccc_handle -1;
		err = bt_gatt_subscribe(app_cb.pconn, subscribe_params);		
		if (err) {
			printk("%s subscribe failed (err %d)\n", module_ccc_alias(ccc_handle), err);
			send_async_msg(MSG_DISCONNECT);
		} else {
			printk("%s subscribed\n", module_ccc_alias(ccc_handle));
		}
	}	
}

static void parse_service(const struct bt_gatt_service_val *gatt_service,
		const struct bt_gatt_attr *attr)
{
    u8_t i, module_handle_start, module_handle_end;

	if (!memcmp(BT_UUID_128(gatt_service->uuid)->val, 
			MODULE_SERVICE_UUID_VAL,sizeof(MODULE_SERVICE_UUID_VAL))) 
	{
		module_handle_start = MODULE_HANDLE_START;
		module_handle_end = MODULE_HANDLE_END;
		for (i = module_handle_start; i <= module_handle_end; i++) {
			module_handle[i] = attr->handle + i - module_handle_start;
		} 
	}
}

static u8_t discover_func(struct bt_conn *conn,
		const struct bt_gatt_attr *attr,
		struct bt_gatt_discover_params *params)
{

	if (!attr) {		
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		send_async_msg(MSG_DISCOVER_COMPLETE);
		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		parse_service((struct bt_gatt_service_val *)attr->user_data, attr);
	}

	return BT_GATT_ITER_CONTINUE;
}

void start_discover(void)
{
	int err;

	if (!app_cb.pconn) {
		printk("Not connected\n");
		return;
	}
	
	if (module_handle[0]) {
		printk("Handle discovered.\n");
		send_async_msg(MSG_DISCOVER_COMPLETE);
		return;
	}

	memcpy(uuid.val, MODULE_SERVICE_UUID_VAL, sizeof(MODULE_SERVICE_UUID_VAL));

	discover_params.uuid = &uuid.uuid;	
	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	err = bt_gatt_discover(app_cb.pconn, &discover_params);
	if (err)  {
		printk("Discover failed (err %d)\n", err);
		send_async_msg(MSG_DISCONNECT);
	} else {
		printk("Discover pending\n");
	}	
}

extern int wp_send_input_report(struct bt_conn *conn, u8_t reportId, u16_t len, u8_t *pValue);
extern int qpps_send_input_report(struct bt_conn *conn, u16_t len, u8_t *pValue);
int module_write(u8_t *data, u16_t len);
static int count = 0;
int ble_send_data(u8_t *data, u16_t len)
{	
	u8_t *p_data = data;
	int err = 0;
	if (app_cb.pconn != NULL) {
		while (len > 0) {
			u8_t send_len = (len > 20 ) ? 20 : len;
			if (ble_param.role == BLE_ROLE_MASTER) {
				err = module_write(p_data, send_len);  // uart to ble
			} else {
#if !CONFIG_USE_QPPS_PROFILE
				err = wp_send_input_report(app_cb.pconn, 2, send_len, p_data); // ble to uart
#else
				err = qpps_send_input_report(app_cb.pconn, send_len, p_data); // ble to uart
#endif
			}

			if (err) {
				return err;
			}

			len -= send_len;
			p_data += send_len;
			count += send_len;
			printk("send count: %d\n",count);
		}					
	}
	return err;
}
