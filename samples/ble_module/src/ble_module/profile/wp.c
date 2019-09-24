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

static u8_t tx_value[23] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static u8_t ctrl_value[23] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static u32_t RxCount;

static void wp_rx_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	rx_wp = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("rx_wp : %d\n", rx_wp);
}

static void wp_ctrl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			u16_t value)
{
	ctrl_wp = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("rx_wp : %d\n", ctrl_wp);
}
extern int act_data_send_data(const u8_t *data, int len);
static ssize_t write_wp_tx(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(tx_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

#if 0
	memcpy(value + offset, buf, len);
#else
	act_data_send_data(buf, len);
#endif

	RxCount += len;

	printk("received data: %u\n", RxCount);

	return len;
}
static ssize_t write_wp_ctrl(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t k;
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	memcpy(value + offset, buf, len);
	return len;
}

/* Vendor Primary Service Declaration */
static struct bt_gatt_attr wp_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&wp_uuid),
	BT_GATT_CHARACTERISTIC(&wp_rx_uuid.uuid, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(&wp_rx_uuid.uuid,BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(wp_rx_ccc_cfg, wp_rx_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(&wp_tx_uuid.uuid,BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&wp_tx_uuid.uuid,BT_GATT_PERM_WRITE, NULL, write_wp_tx, tx_value),
	BT_GATT_CHARACTERISTIC(&wp_ctrl_uuid.uuid,BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&wp_ctrl_uuid.uuid,BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL,write_wp_ctrl, &ctrl_value),
	BT_GATT_CCC(wp_ctrl_ccc_cfg, wp_ctrl_ccc_cfg_changed),
};


static struct bt_gatt_service wp_svc = BT_GATT_SERVICE(wp_attrs);

__init_once_text void wp_init(void)
{
	bt_gatt_service_register(&wp_svc);
}

/* HID Report IDs */
#define HIDAPP_REMOTE_REPORT_ID	1
#define HIDAPP_VOICE_REPORT_ID	2
#define HIDAPP_MOUSE_REPORT_ID	3
int wp_send_input_report(struct bt_conn *conn, u8_t reportId, u16_t len, u8_t *pValue)
{
	if (reportId == HIDAPP_REMOTE_REPORT_ID) {
		/* rx notify enabled */
		if (!ctrl_wp)
			return -EIO;

		return bt_gatt_notify(conn, &wp_attrs[7], pValue, len);
	} else if (reportId == HIDAPP_VOICE_REPORT_ID) {
		/* rx notify enabled */
		if (!rx_wp)
			return -EIO;

		return bt_gatt_notify(conn, &wp_attrs[2], pValue, len);
	} else
		return -EIO;
}

