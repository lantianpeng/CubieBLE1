/**
 ****************************************************************************************
 *
 * @file qpps.c
 *
 * @brief Quintic Private Profile Server implementation.
 *
 * Copyright(C) 2015 NXP Semiconductors N.V.
 * All rights reserved.
 *
 ****************************************************************************************
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

#define SYS_LOG_DOMAIN "qpps"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

/* Quintic private Profile Service */
static struct bt_uuid_128 qpps_uuid = BT_UUID_INIT_128(
	0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,
	0x00,0x10,0x00,0x00,0xe9,0xfe,0x00,0x00);
	
static struct bt_uuid_128 qpps_tx_uuid = BT_UUID_INIT_128(
	0x00,0x96,0x12,0x16,0x54,0x92,0x75,0xb5,
	0xa2,0x45,0xfd,0xab,0x39,0xc4,0x4b,0xd4);

static struct bt_uuid_128 qpps_rx_uuid = BT_UUID_INIT_128(
	0x01,0x96,0x12,0x16,0x54,0x92,0x75,0xb5,
	0xa2,0x45,0xfd,0xab,0x39,0xc4,0x4b,0xd4);

static struct bt_gatt_ccc_cfg qpps_rx_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t qpps_rx_ccc;

static u8_t tx_value[23] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static u32_t tx_count;

static void qpps_rx_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	qpps_rx_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("qpps_rx_ccc : %d\n", qpps_rx_ccc);
}

extern int act_data_send_data(const u8_t *data, int len);
static ssize_t write_qpps_tx(struct bt_conn *conn,
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
	
	tx_count += len;

	printk("received: %u\n", tx_count);

	return len;
}

/* Quintic private Profile Service Declaration */
static struct bt_gatt_attr qpps_attrs[] = {
	/* Quintic Private Profile Service */
	BT_GATT_PRIMARY_SERVICE(&qpps_uuid),

	/* TX */
	BT_GATT_CHARACTERISTIC(&qpps_tx_uuid.uuid, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&qpps_tx_uuid.uuid, BT_GATT_PERM_WRITE, NULL, write_qpps_tx, tx_value),
	BT_GATT_CUD("TX", BT_GATT_PERM_READ),

	/* RX */
	BT_GATT_CHARACTERISTIC(&qpps_rx_uuid.uuid, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(&qpps_rx_uuid.uuid, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(qpps_rx_ccc_cfg, qpps_rx_ccc_cfg_changed),
};


static struct bt_gatt_service qpps_svc = BT_GATT_SERVICE(qpps_attrs);

__init_once_text void qpps_init(void)
{
	bt_gatt_service_register(&qpps_svc);
}

int qpps_send_input_report(struct bt_conn *conn, u16_t len, u8_t *p_value)
{
	/* rx notify enabled */
	if (!qpps_rx_ccc)
		return -EIO;

	return bt_gatt_notify(conn, &qpps_attrs[5], p_value, len);
}
