/** @file
 *  @brief BLE Perhieral
 */

/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
//#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "conn_internal.h"
#include "smp.h"

/*! Configurable parameters for security */
struct appSecCfg_t {
	u8_t	auth;
	u8_t	iKeyDist;
	u8_t	rKeyDist;
	u8_t	oob;
	u8_t	initiateSec;
};

static struct bt_le_conn_param datsUpdateCfg = {
	.interval_min = (BT_GAP_INIT_CONN_INT_MIN),
	.interval_max = (BT_GAP_INIT_CONN_INT_MAX),
	.latency = (0),
	.timeout = (2000),
};

static struct appSecCfg_t datsSecCfg = {
	BT_SMP_AUTH_BONDING | BT_SMP_AUTH_MITM,
	BT_SMP_DIST_ID_KEY,
	BT_SMP_DIST_ENC_KEY | BT_SMP_DIST_ID_KEY,
	BT_SMP_OOB_NOT_PRESENT,
	false
};

extern void bt_set_sec_cfg(u8_t auth, u8_t sendKeyDist, u8_t recvKeyDist, u8_t oobFlag);
extern void bt_set_smp_cfg(u8_t minKeyLen, u8_t maxKeyLen);
extern int bt_set_conn_update_param(int idlePeriod,
			struct bt_le_conn_param *param);


#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

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

static struct bt_gatt_ccc_cfg wp_rx_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg wp_ctrl_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t rx_wp;
static u8_t ctrl_wp;

static u8_t tx_value[256] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static u8_t ctrl_value[256] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static u8_t rx[256];

static u32_t RxCount;
static u32_t TxCount;

static void wp_rx_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	rx_wp = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("rx_wp is %d\n",rx_wp);
}

static void wp_ctrl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			u16_t value)
{
	ctrl_wp = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("ctrl_wp is %d\n",ctrl_wp);
}

static ssize_t write_wp_tx(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(tx_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	RxCount += len;

	printk("received data: %u from conn[%p]\n", RxCount,conn);

	return len;
}
static ssize_t write_wp_ctrl(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t k;
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	printk("ctrl received data:0x");
	for (k = 0; k < len; k++)
		printk("%02X", ((u8_t *)buf)[k]);
	printk("\n");

	return len;
}


/* Vendor Primary Service Declaration */
static struct bt_gatt_attr wp_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&wp_uuid),
	BT_GATT_CHARACTERISTIC(&wp_rx_uuid.uuid, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(&wp_rx_uuid.uuid,
	BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(wp_rx_ccc_cfg, wp_rx_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(&wp_tx_uuid.uuid,
	BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&wp_tx_uuid.uuid,
	BT_GATT_PERM_WRITE, NULL, write_wp_tx, tx_value),
	BT_GATT_CHARACTERISTIC(&wp_ctrl_uuid.uuid,
	BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&wp_ctrl_uuid.uuid,
	BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL,
	write_wp_ctrl, &ctrl_value),
	BT_GATT_CCC(wp_ctrl_ccc_cfg, wp_ctrl_ccc_cfg_changed),
};

void wp_rx_notify(struct bt_conn *conn)
{
	u8_t k = 0;
	u16_t mtu = 23 - 3;

	/* rx notify enabled */
	if (!rx_wp) {
		return;
	}

	if (conn) {
		mtu = bt_gatt_get_mtu(conn) - 3;
	}

	for (k = 0; k < mtu; k++)
		rx[k] = k;

	bt_gatt_notify(conn, &wp_attrs[2], &rx, mtu);
	TxCount += mtu;
	printk("send data: %u to conn[%p]\n",TxCount,conn);

}


static struct bt_gatt_service wp_svc = BT_GATT_SERVICE(wp_attrs);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
	if (datsSecCfg.initiateSec) {
		if (bt_conn_security(conn, BT_SECURITY_MEDIUM)) {
			printk("Failed to set security\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval,
	       latency, timeout);
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Security changed: %s level %u\n", addr, level);
}


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
	.security_changed = security_changed,
};

u8_t bt_ready_flag;
__init_once_text static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready_flag = 1;
	printk("Bluetooth initialized\n");
	bt_gatt_service_register(&wp_svc);

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

void app_main(void)
{
	int err;

	err = bt_set_conn_update_param(K_SECONDS(5), &datsUpdateCfg);

	/*bt_set_smp_cfg(BT_SMP_MIN_ENC_KEY_SIZE, BT_SMP_MAX_ENC_KEY_SIZE);*/

	/*bt_set_sec_cfg(datsSecCfg.auth, datsSecCfg.iKeyDist, datsSecCfg.rKeyDist, datsSecCfg.oob);*/

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		int i;
		
		for (i = 0; i < BT_GATT_CCC_MAX; i++) {
			struct bt_conn *conn;

			if (wp_rx_ccc_cfg[i].value != BT_GATT_CCC_NOTIFY) {
				continue;
			}

			conn = bt_conn_lookup_state_le(&wp_rx_ccc_cfg[i].peer, BT_CONN_CONNECTED);
			if (!conn) {
				continue;
			}

			/* Vendor rx simulation */
			wp_rx_notify(conn);	

			bt_conn_unref(conn);
		}
	}
}
