/** @file
 *  @brief BLE Central
 */

/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include "errno.h"
#include <zephyr.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "msg_manager.h"

static struct bt_conn *default_conn;

/* Custom Service Variables */
static struct bt_uuid_128 wp_uuid = BT_UUID_INIT_128(
	0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
	0x55, 0x44, 0x33, 0x22, 0xA0, 0x20, 0x11, 0x00);

static struct bt_uuid_128 wp_rx_uuid = BT_UUID_INIT_128(
	0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
	0x55, 0x44, 0x33, 0x22, 0xA1, 0x20, 0x11, 0x00);


static struct bt_uuid_128 wp_tx_uuid = BT_UUID_INIT_128(
	0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
	0x55, 0x44, 0x33, 0x22, 0xA2, 0x20, 0x11, 0x00);

static const struct bt_uuid_128 wp_ctrl_uuid = BT_UUID_INIT_128(
	0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
	0x55, 0x44, 0x33, 0x22, 0xA3, 0x20, 0x11, 0x00);

static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);
static struct bt_uuid_16 uuid_16 = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static u8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] data %p length %u\n", data, length);

	return BT_GATT_ITER_CONTINUE;
}

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (!memcmp(BT_UUID_128(discover_params.uuid)->val, wp_uuid.val, sizeof(uuid.val))) {
		memcpy(uuid.val, wp_rx_uuid.val, sizeof(uuid.val));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!memcmp(BT_UUID_128(discover_params.uuid)->val, wp_rx_uuid.val, sizeof(uuid.val))) {
		memcpy(uuid.val, wp_tx_uuid.val, sizeof(uuid.val));
		uuid_16.val = BT_UUID_GATT_CCC_VAL;
		discover_params.uuid = &uuid_16.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct app_msg msg = {0};
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);
	
	/* send a message to main thread */
	msg.type = BLE_CONN_OPEN_IND;
	msg.value = conn;
	send_msg(&msg, K_MSEC(1000));
}

static bool datc_found(u8_t type, const u8_t *data, u8_t data_len,
		      void *user_data)
{
	bt_addr_le_t *addr = user_data;
	char name[31] = {0};

	printk("[AD]: %u data_len %u\n", type, data_len);

	switch (type) {
	case BT_DATA_NAME_COMPLETE:	
		memcpy(name, data, data_len);
		printk("Device found name: %s\n", name); 
		if (!strcmp(name, "Actions DATS")){
			int err;
			
			printk("Actions DATS found.\n");
			err = bt_le_scan_stop();
			if (err) {
				printk("Stop LE scan failed (err %d)\n", err);
				break;
			}

			default_conn = bt_conn_create_le(addr,
							 BT_LE_CONN_PARAM_DEFAULT);
			return false;
		}
	}

	return true;
}

static void ad_parse(struct net_buf_simple *ad,
		     bool (*func)(u8_t type, const u8_t *data,
				  u8_t data_len, void *user_data),
		     void *user_data)
{
	while (ad->len > 1) {
		u8_t len = net_buf_simple_pull_u8(ad);
		u8_t type;

		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > ad->len || ad->len < 1) {
			printk("AD malformed\n");
			return;
		}

		type = net_buf_simple_pull_u8(ad);

		if (!func(type, ad->data, len - 1, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_SCAN_RSP) {
		/* TODO: Move this to a place it can be shared */
		ad_parse(ad, datc_found, (void *)addr);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct app_msg msg = {0};
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* send a message to main thread */
	msg.type = BLE_CONN_CLOSE_IND;
	msg.value = conn;
	send_msg(&msg, K_MSEC(1000));
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void app_main(void)
{
	int err;
	struct app_msg msg = {0};
	int result = 0;
	
	err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);

	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	
	while (1) {
	if (receive_msg(&msg, K_FOREVER)) {
		switch (msg.type) {
		case BLE_CONN_OPEN_IND:
		{
			if (msg.value == default_conn) {
				memcpy(uuid.val, wp_uuid.val, sizeof(uuid.val));
				discover_params.uuid = &uuid.uuid;
				discover_params.func = discover_func;
				discover_params.start_handle = 0x0001;
				discover_params.end_handle = 0xffff;
				discover_params.type = BT_GATT_DISCOVER_PRIMARY;

				err = bt_gatt_discover(default_conn, &discover_params);
				if (err) {
					printk("Discover failed(err %d)\n", err);
					return;
				}
			}
		}
		break;
		case BLE_CONN_CLOSE_IND:
		{
				/* This demo require active scan for reconnecting */
				err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
				if (err) {
					printk("Scanning failed to start (err %d)\n", err);
				}
		}
		default:
			printk("error message type msg.type %d\n", msg.type);
			break;
		}
		if (msg.callback != NULL)
			msg.callback(&msg, result, NULL);
	}
	}
}
