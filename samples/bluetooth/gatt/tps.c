/** @file
 *  @brief TPS Service sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

struct bt_gatt_tps {
	/* The Transmit Power Level characteristic represents the current transmit power level in dBm, 
	 *  and the level ranges from -100 dBm to +20 dBm to a resolution of 1 dBm.
	 */
	s8_t tx_power_level;
 } __packed;

static struct bt_gatt_tps gatt_tps;
static struct k_timer tps_tx_power_level_timer;

void tps_tx_power_level_timer_handler(struct k_timer *dummy)
{
	gatt_tps.tx_power_level++;
	if (gatt_tps.tx_power_level == 20) {
		gatt_tps.tx_power_level = -100;
	}
}

void tps_tx_power_level_timer_init(void)
{
	k_timer_init(&tps_tx_power_level_timer, tps_tx_power_level_timer_handler, NULL);
	k_timer_start(&tps_tx_power_level_timer, K_SECONDS(1), K_SECONDS(1));
} 

static ssize_t tps_tx_power_level_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_tps.tx_power_level,
			sizeof(gatt_tps.tx_power_level));
}

/* Tx Power Service Declaration */
static struct bt_gatt_attr tps_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_TPS),
	BT_GATT_CHARACTERISTIC(BT_UUID_TPS_TX_POWER_LEVEL, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_TPS_TX_POWER_LEVEL, BT_GATT_PERM_READ, tps_tx_power_level_read, NULL, NULL),
};

static struct bt_gatt_service tps_svc = BT_GATT_SERVICE(tps_attrs);

void tps_init(void)
{
	gatt_tps.tx_power_level = -100;
	bt_gatt_service_register(&tps_svc);
	tps_tx_power_level_timer_init();
}

