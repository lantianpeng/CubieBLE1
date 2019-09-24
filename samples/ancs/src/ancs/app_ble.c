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
#include "att_internal.h"
#include "keys.h"
#include <input_dev.h>

#include "hci_core.h"
#include "hci_core_patch.h"

#include <dis.h>
#include <bas.h>
#include <ota.h>
#include "msg_manager.h"
#include "soc.h"
#include "soc_pm.h"
#include "nvram_config.h"
#include "app_ble.h"
#include "app_disc.h"
#include "ancs_service_disc.h"
#include "ancs_service.h"

/* Magic value to check the sanity of NVM region used by the application */
#define NVM_SANITY_MAGIC               (0xAB1B)


struct app_cb_t app_cb = {
	.pconn = NULL,
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

struct bt_data app_adv_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18),
	BT_DATA_BYTES(BT_DATA_SOLICIT128,
	0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4,
	0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79),
};

u8_t adv_name_data[BLE_ADV_NAME_DATA_MAX+1] = {BLE_ADV_NAME_DATA_DEF};
struct bt_data app_scan_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, adv_name_data, BLE_ADV_NAME_LEN_DEF),
};

extern void send_async_msg(u8_t msg_type);
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

	printk("Connected %s", addr);
	if (bt_conn_security(conn, BT_SECURITY_MEDIUM)) {
		printk("Failed to set security");
	}

	/* send a message to main thread */
	app_ble_send_msg_to_app(BLE_CONN_OPEN_IND, err);
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
	/* send a message to main thread */
	app_ble_send_msg_to_app(BLE_CONN_SEC_OPEN_IND, level);
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval,
	       latency, timeout);
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
static void read_persistent_store(void);
static void app_data_init(void);
extern int bt_att_paramter_init(int att_mtu, int tx_limit);
__init_once_text void bt_ready(int err)
{
	/* don't enter into deepsleep when bt stack init */
	app_release_wake_lock();

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	/* config mtu for bt host att layer */
	bt_att_paramter_init(BT_ATT_DEFAULT_LE_MTU, CONFIG_BT_ATT_TX_MAX);

	/* init dis service */
	dis_init(CONFIG_DIS_MODEL, CONFIG_DIS_MANUFACTURER_NAME, dis_val_pnp);

	/* init ota profile */
#if CONFIG_OTA_WITH_APP
	/* init bas service */
	bas_init(NULL, blvl_ccc_cfg, BT_GATT_CCC_MAX);
	ota_profile_init();
#endif

	/* set conn update param for bt stack */
	bt_set_conn_update_param(K_SECONDS(1), &app_conn_update_cfg);

	/* register conn callbacks into bt stack */
	bt_conn_cb_register(&conn_callbacks);

	/* NOTE Make Sure All BLE profile have register into stack db */
	bt_gatt_load_ccc();

	/* Read persistent storage */
	read_persistent_store();

	/* Initialise  application data structure */
	app_data_init();

	/* Initialise ANCS application State
	 * Always the application state should be initialised before the
	 * GATT Database functions as GATT_ADD_DB_CFM gets invoked
	 * prior to executing the code after GattAddDatabaseReq()
	 */
	app_cb.state = app_init;

	/* set some flag */
	bt_ready_flag = 1;
	overlay_ready_flag = 1;

	err = bt_le_adv_start(&app_adv_cfg, app_adv_data, ARRAY_SIZE(app_adv_data), (const struct bt_data *)app_scan_data, ARRAY_SIZE(app_scan_data));
	if (err) {
		printk("Undirect Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Undirect Advertising successfully started\n");
}

static void app_data_init(void)
{
	/* Make connection Id invalid */
	app_cb.pconn = NULL;

	/* Reset the authentication failure flag */
	app_cb.auth_failure = false;
}


static void read_persistent_store(void)
{
	u16_t nvm_sanity = 0xffff;

	/* Read persistent storage to know if the device was last bonded.
	 * If the device was bonded, trigger fast undirected advertisements by
	 * setting the white list for bonded host. If the device was not bonded,
	 * trigger undirected advertisements for any host to connect.
	 */
	nvram_config_get("sanity", (u16_t *)&nvm_sanity, sizeof(nvm_sanity));
	printk("nvm_sanity : 0x%x\n", nvm_sanity);

	if (nvm_sanity == NVM_SANITY_MAGIC) {
		/* Read bonded flag from NVM */
		nvram_config_get("bond_flag", (u16_t *)&app_cb.bonded, sizeof(app_cb.bonded));
		if (app_cb.bonded) {

			/* Bonded host BD address will only be stored if bonded flag
			 * is set to true.
			 */
			nvram_config_get("bond_bd_addr", (u16_t *)&app_cb.bonded_bd_addr, sizeof(app_cb.bonded_bd_addr));
		} else {
			/* Case when we have only written NVM_SANITY_MAGIC to NVM but
			 * application didn't get bonded to any host in the last powered
			 * session.
			 */
			app_cb.bonded = false;
		}


		/* Check if there are any valid discovered handles stored in NVM */
		nvram_config_get("disc_h_present", (u16_t *)&app_cb.remote_gatt_handles_present, sizeof(app_cb.remote_gatt_handles_present));
		printk("remote_gatt_handles_present : 0x%x\n", app_cb.remote_gatt_handles_present);

		/* If there are any valid handles stored in NVM, the following function
		 * will read those handles.
		 * If there are no valid handles present in NVM, the following function
		 * will only initialise the NVM offsets
		 */
		read_discovered_gatt_database_from_nvm(app_cb.remote_gatt_handles_present);

		/* Read device name and length from NVM */
		/* GapReadDataFromNVM(&nvm_offset); */
	} else {
		/* NVM sanity check failed means either the device is being brought up
		 * for the first time or memory has got corrupted in which case
		 * discard the data and start fresh.
		 */
		nvm_sanity = NVM_SANITY_MAGIC;

		/* Write NVM sanity word to the NVM */
		nvram_config_set("sanity", (u16_t *)&nvm_sanity, sizeof(nvm_sanity));

		/* The device will not be bonded as it is coming up for the first time
		 */
		app_cb.bonded = false;

		/* Write bonded status to NVM */
		nvram_config_set("bond_flag", (u16_t *)&app_cb.bonded, sizeof(app_cb.bonded));

		/* The application is being initialised for the first time. There are
		 * no discovered handles.
		 */
		app_cb.remote_gatt_handles_present = false;
		nvram_config_set("disc_h_present", (u16_t *)&app_cb.remote_gatt_handles_present, sizeof(app_cb.remote_gatt_handles_present));
		/* Initialise the NVM offsets for the discovered remote database
		 * attribute handles.
		 */
		read_discovered_gatt_database_from_nvm(app_cb.remote_gatt_handles_present);
	}
}

static void appConfigureNotifications(struct bt_conn *conn, bool datasource);
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
	case BLE_CONN_SEC_OPEN_IND:
	{
		if (app_cb.pconn != NULL) {
			extern void start_gatt_database_discovery(struct bt_conn *conn);

			/*Initiate configuring ANCS notification handle */
			/* if( get_ancs_notification_ccd_handle() != INVALID_ATT_HANDLE) */
			if (0) {
				if (!app_cb.notif_configuring) {
					app_cb.notif_configuring = true;
					appConfigureNotifications(app_cb.pconn, true);
					appConfigureNotifications(app_cb.pconn, false);
				}
			} else {
				/* Start Gatt Database discovery */
				start_gatt_database_discovery(app_cb.pconn);
			}
		}
	}
	break;
	case BLE_CONN_CLOSE_IND:
	{
		app_cb.notif_configuring = false;
		/* clear ancs service state */
		ancs_service_data_init();

		/* This demo require active scan for reconnecting */
		err = bt_le_adv_start(&app_adv_cfg, app_adv_data, ARRAY_SIZE(app_adv_data), (const struct bt_data *)app_scan_data, ARRAY_SIZE(app_scan_data));
		if (err) {
			printk("Undirect Advertising failed to start (err %d)\n", err);
			return;
		}

		printk("Undirect Advertising successfully started\n");
	}
	break;
	case BLE_DISC_PRIM_SERV_IND:
	{
		if (app_cb.pconn != NULL) {
			extern void handle_service_discovery_primary_service_by_uuid_cfm(struct bt_conn *conn, u8_t status);
			handle_service_discovery_primary_service_by_uuid_cfm(app_cb.pconn, (int)event_data);
		}
	}
	break;
	case BLE_DISC_SERVICE_CHAR_IND:
	{
		if (app_cb.pconn != NULL) {
			extern void handle_gatt_discover_service_characteristic_cfm(struct bt_conn *conn, u8_t status);
			handle_gatt_discover_service_characteristic_cfm(app_cb.pconn, (int)event_data);
		}
	}
	break;
	case BLE_DISC_SERVICE_CHAR_DESC_IND:
	{
		if (app_cb.pconn != NULL) {
			extern void handle_gatt_characteristic_descriptor_cfm(struct bt_conn *conn, u8_t status);
			handle_gatt_characteristic_descriptor_cfm(app_cb.pconn, (int)event_data);
		}
	}
	break;
	case BLE_DISCOVERY_COMPELTE_IND:
	{
		if (app_cb.pconn != NULL) {
			/* Service discovery is complete. Set the boolean flag
			 * remote_gatt_handles_present to true and store it in NVM
			 */
			app_cb.remote_gatt_handles_present = true;
			nvram_config_set("disc_h_present", (u16_t *)&app_cb.remote_gatt_handles_present, sizeof(app_cb.remote_gatt_handles_present));

			/* Initiate configuring ANCS Notification Handle */
			if (get_ancs_notification_ccd_handle() != INVALID_ATT_HANDLE) {
				if (!app_cb.notif_configuring) {
					app_cb.notif_configuring = true;
					appConfigureNotifications(app_cb.pconn, true);
					appConfigureNotifications(app_cb.pconn, false);
				}
			}
		}
	}
	break;
	default:
		printk("unknow ble event %d\n", event_code);
		break;
	}
}

extern void appConfigureNotifications(struct bt_conn *conn, bool datasource)
{
	if (!datasource) {
		/* Configure for ANCS Notification */
		configure_ancs_notifications(conn);
	} else {
		/* Configure for ANCS Data Source Notification */
		configure_ancs_data_source_notification(conn);
	}
}

