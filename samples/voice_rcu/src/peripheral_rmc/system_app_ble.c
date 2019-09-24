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

#include <dis.h>
#include <bas.h>
#include <hog.h>
#include <wp.h>
#include "msg_manager.h"
#include "system_app_sm.h"
#include "system_app_input.h"
#include "system_app_audio.h"
#include "system_app_timer.h"
#include "system_app_led.h"
#include "ota.h"
#include "soc.h"
#include "soc_pm.h"
#include "system_app_batt.h"
#include "system_app_audio.h"
#include "system_app_ble.h"

#define SYS_LOG_DOMAIN "ble"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

#define MAX_RECONN_TIMES 3

struct hid_app_cb_t {
	struct bt_conn *pconn;
	bt_addr_le_t direct_addr;
	u8_t reconn_times;
	u8_t enter_pairing_mode;
	u8_t rmc_term_conn_flag;
	u8_t rmc_security_ready_flag;
};

struct hid_app_cb_t hid_app_cb;

struct bt_le_conn_param hid_app_update_cfg = {
	.interval_min = (10),
	.interval_max = (10),
	.latency = (44),
	.timeout = (400),
};

#define BT_LE_ADV_CONN_UNDIRECT BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
								BT_LE_ADV_OPT_ONE_TIME, \
								BT_GAP_ADV_FAST_INT_MIN_1, \
								BT_GAP_ADV_FAST_INT_MAX_1)

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x12, 0x18, /* HID Service */
		      0x0f, 0x18), /* Battery Service */
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x80, 0x01),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0xe0, 0x03),
};

/* The input report fits in one byte */
#define HIDAPP_VOICE_INPUT_REPORT_LEN			20
#define HIDAPP_MOUSE_INPUT_REPORT_LEN			3
#define HIDAPP_REMOTE_INPUT_REPORT_LEN		3
#define HIDAPP_OUTPUT_REPORT_LEN					1
#define HIDAPP_FEATURE_REPORT_LEN					1

/* HID Report IDs */
#define HIDAPP_REMOTE_REPORT_ID						1
#define HIDAPP_VOICE_REPORT_ID						CONFIG_HID_VOICE_REPORT_ID
#define HIDAPP_MOUSE_REPORT_ID						3


/*! HID Report Type/ID and attribute handle map */
static const struct hid_report_id_map_t hid_app_report_id_set[] = {
	/* type                       ID                            handle */
	{HID_REPORT_TYPE_INPUT,       HIDAPP_REMOTE_REPORT_ID,      HID_INPUT_REPORT_1_HDL},     /* Remote Input Report */
	{HID_REPORT_TYPE_INPUT,       HIDAPP_VOICE_REPORT_ID,       HID_INPUT_REPORT_2_HDL},     /* Keyboard Input Report */
	{HID_REPORT_TYPE_OUTPUT,      HIDAPP_VOICE_REPORT_ID,       HID_OUTPUT_REPORT_HDL},      /* Keyboard Output Report */
	{HID_REPORT_TYPE_FEATURE,     HIDAPP_VOICE_REPORT_ID,       HID_FEATURE_REPORT_HDL},     /* Keyboard Feature Report */
	{HID_REPORT_TYPE_INPUT,       HIDAPP_MOUSE_REPORT_ID,       HID_INPUT_REPORT_3_HDL},     /* Mouse Input Report */
	{HID_REPORT_TYPE_INPUT,       HID_KEYBOARD_BOOT_ID,         HID_KEYBOARD_BOOT_IN_HDL},   /* Boot Keyboard Input Report */
	{HID_REPORT_TYPE_OUTPUT,      HID_KEYBOARD_BOOT_ID,         HID_KEYBOARD_BOOT_OUT_HDL},  /* Boot Keyboard Output Report */
	{HID_REPORT_TYPE_INPUT,       HID_MOUSE_BOOT_ID,            HID_MOUSE_BOOT_IN_HDL},      /* Boot Mouse Input Report */
};

const u8_t hid_report_map[] = {
	0x05, 0x0c, /*	Usage Page (Consumer Devices) */
	0x09, 0x01, /*	Usage (Consumer Control) */
	0xa1, 0x01, /*	Collection (Application) */
	0x85, HIDAPP_REMOTE_REPORT_ID, /*	report ID (0x01) */
	0x19, 0x00, /*	USAGE_MINIMUM (0) */
	0x2a, 0x9c, 0x02, /*	USAGE_MINIMUM (0x29c) */
	0x15, 0x00, /*	Logical Minimum (0) */
	0x26, 0x9c, 0x02, /*	Logical Maximum (0x29c) */
	0x95, 0x01, /*	Report Count (1) */
	0x75, 0x10, /*	Report Size (16) */
	0x81, 0x00, /*	Input (Data, array, Absolute) */
	0x09, 0x02, /*	Usage (Numeric Key Pad ) */
	0xa1, 0x02, /*	Collection (Application) */
	0x05, 0x09, /*	Usage Page (button) */
	0x19, 0x01, /*	Usage Minimum (1) */
	0x29, 0x0a, /*	Usage Maximum (10) */
	0x15, 0x01, /*	Logical Minimum (1) */
	0x25, 0x0a, /*	Logical Maximum (10) */
	0x95, 0x01, /*	Report Count (1) */
	0x75, 0x08, /*	Report Size (8)  */
	0x81, 0x40, /*	Input (Data, Variable, Relative) */
	0xc0,    /*	End Collection */
	0xc0,    /*	End Collection */
#if CONFIG_HID_MOUSE_USAGE 
	0x05, 0x01, /*	Usage Page (Generic Desktop) */
	0x09, 0x02, /*	USAGE (Mouse) */
	0xa1, 0x01, /*	COLLECTION (Application) */
	0x09, 0x01, /*	USAGE (Pointer) */
	0xa1, 0x00, /*	COLLECTION (Physical) */
	0x85, HIDAPP_MOUSE_REPORT_ID, /*	report ID (HIDAPP_MOUSE_REPORT_ID) */
	0x05, 0x01, /*	USAGE_PAGE (Generic Desktop) */
	0x09, 0x30, /*	USAGE (X) */
	0x09, 0x31, /*	USAGE (Y) */
	0x15, 0x80, /*	LOGICAL_MINIMUM (-127) */
	0x25, 0x7f, /*	LOGICAL_MAXIMUM (127) */
	0x75, 0x08, /*	REPORT_SIZE (8) */
	0x95, 0x02, /*	REPORT_COUNT (2) */
	0x81, 0x06, /*	INPUT (Data, Variable, Relative) */
	0x05, 0x09, /*	USAGE_PAGE (Button) */
	0x19, 0x01, /*	USAGE_MINIMUM (Button 1) */
	0x29, 0x05, /*	USAGE_MAXIMUM (Button 5) */
	0x15, 0x00, /*	LOGICAL_MINIMUM (0) */
	0x25, 0x01, /*	LOGICAL_MAXIMUM (1) */
	0x95, 0x05, /*	REPORT_COUNT (5) */
	0x75, 0x01, /*	REPORT_SIZE (1) */
	0x81, 0x02, /*	INPUT () */
	0x95, 0x01, /*	REPORT_COUNT (1) */
	0x75, 0x03, /*	REPORT_SIZE (3) */
	0x81, 0x03, /*	INPUT () */
	0xc0, /*	End Collection (Physical) */
	0xc0, /*	End Collection (Application) */
#endif
	0x06, 0x01, 0xff,
	0x09, 0x01, /*	Usage (Consumer Control) */
	0xa1, 0x02, /*	COLLECTION () */
	0x85, HIDAPP_VOICE_REPORT_ID, /*	report ID (HIDAPP_KEYBOARD_REPORT_ID) */
	0x09, 0x14,
	0x75, 0x08, /*	Report Size (8) */
	0x95, 0x14, /*	Report Count (20) */
	0x15, 0x80, /*	Logical Minimum (0x80) */
	0x25, 0x7f, /*	Logical Minimum (0x7f) */
	0x81, 0x22, /*	Input () */
	0x85, 0x04, /*	report ID () */
	0x09, 0x04, /*	USAGE () */
	0x75, 0x08, /*	Report Size (8) */
	0x95, 0x01, /*	Report Count (1) */
	0x91, 0x02, /*	Output */
	0xc0,
};

static struct bt_gatt_ccc_cfg hid_ccc_cfg[BT_GATT_CCC_MAX*5] = {};

/*! HID Profile Configuration */
static struct hid_config_t hid_app_hid_config = {
	(struct hid_report_id_map_t *) hid_app_report_id_set,                  /* Report ID to Attribute Handle map */
	sizeof(hid_app_report_id_set)/sizeof(struct hid_report_id_map_t),     /* Size of Report ID to Attribute Handle map */
	NULL,
	NULL,                                     /* Output Report Callback */
	NULL,                                    /* Feature Report Callback */
	NULL,                                        /* Info Callback */
	hid_ccc_cfg,
	BT_GATT_CCC_MAX,
	hid_report_map,
	sizeof(hid_report_map),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct app_msg  msg = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		SYS_LOG_ERR("Failed to connect to %s (%u)", addr, err);
		return;
	}

	hid_app_cb.pconn = conn;
	SYS_LOG_INF("Connected %s", addr);

	msg.type = MSG_BLE_STATE;
	msg.value = RMC_MSG_BLE_CONNECTED;
	send_msg(&msg, K_MSEC(100));

	if (hid_app_cb.enter_pairing_mode == 1) {
		hid_app_cb.enter_pairing_mode = 0;
		pair_led_indication(BLE_PAIRED);
	}

	update_battry_value();
	rmc_timer_start(ID_ADC_BATTERY_TIMEOUT);
	rmc_timer_start(ID_RMC_SEC_TIMEOUT);
}

static void le_identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				 const bt_addr_le_t *identity)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(rpa, addr, sizeof(addr));
	SYS_LOG_INF("le_identity_resolved rpa %s", addr);
	bt_addr_le_to_str(identity, addr, sizeof(addr));
	SYS_LOG_DBG("le_identity_resolved identity %s", addr);
}
extern u8_t ota_need_reset;
static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct app_msg  msg = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	hid_app_cb.pconn = NULL;
	SYS_LOG_INF("Disconnected from %s (reason %u)", addr, reason);
	rx_stack_analyze();
	if (reason == BT_HCI_ERR_REMOTE_USER_TERM_CONN)
		hid_app_cb.rmc_term_conn_flag = 1;

	msg.type = MSG_BLE_STATE;
	if (hid_app_cb.rmc_term_conn_flag) {
		hid_app_cb.rmc_term_conn_flag = 0;
		msg.value = RMC_MSG_BLE_USER_DISCONNECTED;
	} else {
		msg.value = RMC_MSG_BLE_NON_USER_DISCONNECTED;
	}
	
	/*pair_comb_key_timer_start is set, maybe need to reconnect */
	if (pair_comb_key_timer_start) 
		msg.value = RMC_MSG_BLE_NON_USER_DISCONNECTED;

	send_msg(&msg, K_MSEC(100));

	rmc_timer_stop(ID_ADC_BATTERY_TIMEOUT);
	hid_app_cb.rmc_security_ready_flag = 0;

	/* reset when ota need */
	#ifdef CONFIG_OTA_WITH_APP
	if (ota_need_reset) {
		sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	}
	#endif
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct app_msg  msg = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	SYS_LOG_INF("Security changed: %s level %u", addr, level);

	/* bonded but hid is not ready */
	if (is_paired() && !all_hid_ccc_is_enabled()) {
		SYS_LOG_INF("bonded but hid is not ready\n");
		rmc_clear_pair_info();
		rmc_sm_execute(RMC_MSG_CLEAR_PIAR_KEY);
		return ;
	}

	rmc_timer_stop(ID_RMC_SEC_TIMEOUT);

	/* set security flags */
	hid_app_cb.rmc_security_ready_flag = 1;

	msg.type = MSG_BLE_STATE;
	msg.value = RMC_MSG_BLE_SEC_CONNECTED;
	send_msg(&msg, K_MSEC(100));
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	SYS_LOG_INF("LE conn param updated: int 0x%04x lat %d to %d", interval, latency, timeout);
}

bool hid_app_ready_update(void)
{
	if (all_hid_ccc_is_enabled() && hid_app_cb.rmc_security_ready_flag)
		return true;
	else
		return false;
}

void hid_app_remote_report_channel_enable(void)
{
	SYS_LOG_INF("input channel enable\n");
	if (!hid_app_cb.pconn)
		return ;
	struct app_msg  msg = {0};
	msg.type = MSG_BLE_STATE;
	msg.value = RMC_MSG_BLE_INPUT_ENABLED;
	send_msg(&msg, K_MSEC(100));
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
static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[BT_GATT_CCC_MAX] = {};
__init_once_text void bt_ready(int err)
{
	/* don't enter into deepsleep when bt stack init */
	app_release_wake_lock();

	if (err) {
		SYS_LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	/* force to use legacy pair for compitable and performance of pair */
	sc_supported = false;
	SYS_LOG_INF("force to disable sc");
	
	/* disable LE conn param req feature for compitable such as ap6212 */
	hci_set_le_local_feat(BT_LE_FEAT_BIT_CONN_PARAM_REQ, false);

	SYS_LOG_INF("Bluetooth initialized");
	memset(&hid_app_cb, 0, sizeof(hid_app_cb));

	/* init bas service */
	bas_init(adc_batt_read, blvl_ccc_cfg, BT_GATT_CCC_MAX);

	/* init dis service */
	dis_init(CONFIG_DIS_MODEL, CONFIG_DIS_MANUFACTURER_NAME, dis_val_pnp);

	/* init hid service */
	hid_init(&hid_app_hid_config);
	hog_init();

	/* init ota profile */
#if CONFIG_OTA_WITH_APP
	ota_profile_init();
#endif

	/* set conn update param for bt stack */
	bt_set_conn_update_param(K_SECONDS(1), &hid_app_update_cfg);
	register_app_ready_update_cback(hid_app_ready_update);

	/* register conn callbacks into bt stack */
	bt_conn_cb_register(&conn_callbacks);

	/* NOTE Make Sure All BLE profile have register into stack db then load ccc */
	bt_gatt_load_ccc();

	/* tell rmc state machine event of system power up */
	rmc_sm_execute(RMC_MSG_POWER_UP);

	/* set some flag */
	bt_ready_flag = 1;
	overlay_ready_flag = 1;
}

int hid_app_remote_report_event(u16_t button)
{
	/* beofre send key check whether connection is connected, we need encryption conn for hid profile */
	if ((hid_app_cb.pconn != NULL) && hid_app_cb.rmc_security_ready_flag) {
		SYS_LOG_INF("button is 0x%04x", button);
		u8_t btnData[HIDAPP_REMOTE_INPUT_REPORT_LEN];

		/* record key data */
		btnData[0] = button & 0xFF;
		btnData[1] = (button >> 8) & 0xFF;
		btnData[2] = 0;

		/* send the data */
		return hid_send_input_report(hid_app_cb.pconn, HIDAPP_REMOTE_REPORT_ID, HIDAPP_REMOTE_INPUT_REPORT_LEN, btnData);
	} else
		return -EIO;
}

int hid_app_voice_report_event(u8_t *buffer)
{
	/* beofre send key check whether connection is connected, we need encryption conn for hid profile */
	if ((hid_app_cb.pconn != NULL) && hid_app_cb.rmc_security_ready_flag) {
		return hid_send_input_report(hid_app_cb.pconn, HIDAPP_VOICE_REPORT_ID, HIDAPP_VOICE_INPUT_REPORT_LEN, buffer);
	} else
		return -EIO;
}

void rmc_sm_act_none(u8_t event, void *msg)
{
	/* nothing for rmc state machine */
}
extern u16_t comb_key_code;
void set_sd_data(void)
{
	u8_t i;

	for (i = 0; i < ARRAY_SIZE(ad); i++) {

		if(ad[i].type != BT_DATA_NAME_COMPLETE)
			continue;

		switch (comb_key_code) {
		case REMOTE_COMB_KEY_OK_BACK:
			ad[i].data = DEVICE_NAME;
			ad[i].data_len = DEVICE_NAME_LEN;
		break;
		case REMOTE_COMB_KEY_TEST_ONE:
			ad[i].data = CONFIG_BT_DEVICE_NAME_1;
			ad[i].data_len = (sizeof(CONFIG_BT_DEVICE_NAME_1) - 1);
			break;
		case REMOTE_COMB_KEY_TEST_TWO:
			ad[i].data = CONFIG_BT_DEVICE_NAME_2;
			ad[i].data_len = (sizeof(CONFIG_BT_DEVICE_NAME_2) - 1);
			break;
		case REMOTE_COMB_KEY_TEST_THREE:
			ad[i].data = CONFIG_BT_DEVICE_NAME_3;
			ad[i].data_len = (sizeof(CONFIG_BT_DEVICE_NAME_3) - 1);
		break;
		case REMOTE_COMB_KEY_TEST_FOUR:
			ad[i].data = CONFIG_BT_DEVICE_NAME_4;
			ad[i].data_len = (sizeof(CONFIG_BT_DEVICE_NAME_4) - 1);
		break;
		}
		SYS_LOG_DBG("ad name %s", ad[i].data);
	}
	comb_key_code = REMOTE_COMB_KEY_OK_BACK;
}

void rmc_sm_act_start_adv(u8_t event, void *msg)
{
	/* enter pairing mode (undirected advertising) */
	int err;

	/* pconn != NULL, don't start adv */
	if (hid_app_cb.pconn)
		return ;

	bt_le_adv_stop();
	err = bt_le_adv_start(BT_LE_ADV_CONN_UNDIRECT, ad, ARRAY_SIZE(ad), (const struct bt_data *)sd, ARRAY_SIZE(sd));
	if (err) {
		SYS_LOG_ERR("Undirect Advertising failed to start (err %d)", err);
		return;
	}
	pair_led_indication(BLE_PAIRING);
	hid_app_cb.enter_pairing_mode = 1;
	SYS_LOG_INF("Undirect Advertising successfully started");

	/* stop direct adv timer if direct adv timer started */
	rmc_timer_stop(ID_DIRECT_ADV_TIMEOUT);
	rmc_timer_start(ID_ADV_TIMEOUT);
}

void rmc_sm_act_connected(u8_t event, void *msg)
{
	SYS_LOG_INF("rmc_sm_act_connected");
	if (hid_app_cb.enter_pairing_mode == 1) {
		hid_app_cb.enter_pairing_mode = 0;
		pair_led_indication(BLE_PAIRED);	
	} 
}
bool is_paired(void)
{
	if (bt_addr_le_cmp(&hid_app_cb.direct_addr, BT_ADDR_LE_ANY)) {
		return true;
	}

	if (bt_storage) {
		struct bt_keys tmp_bt_keys;

		/* try to read bt_keys from NVRAM */
		ssize_t ret = bt_storage->read(NULL, BT_STORAGE_BT_KEYS, (void *)&tmp_bt_keys, sizeof(struct bt_keys));

		/* check whether there is bt_keys and then get addr from bt_keys */
		if ((ret == sizeof(struct bt_keys)) &&
			(bt_addr_le_cmp(&tmp_bt_keys.addr, BT_ADDR_LE_ANY)) &&
		(!bt_addr_le_is_rpa(&tmp_bt_keys.addr))) {
			bt_addr_le_copy(&hid_app_cb.direct_addr, &tmp_bt_keys.addr);
			return true;
		}
	}
	return false;
}

void rmc_sm_act_start_direct_adv(u8_t event, void *msg)
{
		if (is_paired() && hid_app_cb.reconn_times < MAX_RECONN_TIMES) {
			/* try to reconnect with reconnecting advertising */
			int err;

			bt_le_adv_stop();
#if CONFIG_USE_UNDIRECT_ADV_FOR_RECONN
			/* ADV_DIRECT_IND should not be used for IOS */
			err = bt_le_adv_start(BT_LE_ADV_CONN_UNDIRECT, ad, ARRAY_SIZE(ad), (const struct bt_data *)sd, ARRAY_SIZE(sd));
#else
			err = bt_le_direct_adv_start(&hid_app_cb.direct_addr);
#endif
			if (err) {
				SYS_LOG_ERR("reconnecting Advertising failed to start (err %d)", err);
				return;
			}
			hid_app_cb.reconn_times++;
			SYS_LOG_INF("reconnecting Advertising successfully started");
			rmc_timer_start(ID_DIRECT_ADV_TIMEOUT);
		} else {
			hid_app_cb.reconn_times = 0;
			
			if (!pair_comb_key_timer_start) 
				rmc_sm_execute(RMC_MSG_RECONN_FAIL);
			else {
				/* pair_comb_key_timer_start is set, start paring adv */
				rmc_sm_execute(RMC_MSG_CLEAR_PIAR_KEY);
			}
		}
}


void rmc_sm_act_stop_adv(u8_t event, void *msg)
{
	SYS_LOG_INF("rmc_sm_act_stop_adv");
	bt_le_adv_stop();
	if (hid_app_cb.enter_pairing_mode == 1) {
		hid_app_cb.enter_pairing_mode = 0;
		pair_led_indication(BLE_PAIRING_TIMEOUT);
	}
}

void rmc_sm_act_disconnect_conn(u8_t event, void *msg)
{
	SYS_LOG_INF("rmc_sm_act_disconnect_conn");

	/* disconnect connection when No Actions for a while */
	if (hid_app_cb.pconn != NULL) {
		bt_conn_disconnect(hid_app_cb.pconn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		hid_app_cb.rmc_term_conn_flag = 1;
	}
}

void rmc_clear_pair_info(void)
{
	int err;

	/* set scan response data */
	set_sd_data();

	/* clear ccc in nvram */
	bt_gatt_clear_ccc();

	/* disconnect connection and clear ltk from NVRAM */
	err = bt_storage_clear(NULL);
	if (err) {
		SYS_LOG_ERR("Failed to clear storage (err %d)", err);
	} else {
		bt_addr_le_copy(&hid_app_cb.direct_addr, BT_ADDR_LE_ANY);
		SYS_LOG_INF("rmc_clear_pair_info");
	}

}

void rmc_sm_act_send_key(u8_t event, void *msg)
{
	/* send pending key which is pended before reconnected */
	send_pending_keycode();
}

void system_ble_event_handle(u32_t ble_event)
{
		hid_app_cb.reconn_times = 0;
		rmc_sm_execute(ble_event);
}

bool rmc_security_ready(void)
{
	return hid_app_cb.rmc_security_ready_flag;
}
