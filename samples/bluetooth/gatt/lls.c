/** @file
 *  @brief LLS Service sample
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

#define LLS_ALERT_LEVEL_NO_ALERT    0
#define LLS_ALERT_LEVEL_MILD_ALERT  1
#define LLS_ALERT_LEVEL_HIGH_ALERT  2

struct bt_gatt_lls {
	u8_t alert_level;
 } __packed;

static struct bt_gatt_lls gatt_lls;
static struct k_timer lls_alert_timer;

void lls_alert_off(void)
{
	/* reset the alert level ? */
	gatt_lls.alert_level = LLS_ALERT_LEVEL_NO_ALERT;
	k_timer_stop(&lls_alert_timer);
}

static void lls_alert_handler(struct k_timer *dummy)
{
	if (LLS_ALERT_LEVEL_MILD_ALERT== gatt_lls.alert_level) {
		printk("link loss: milddle alert!\n");
	} else  if (LLS_ALERT_LEVEL_HIGH_ALERT == gatt_lls.alert_level) {
		printk("link loss: high alert!\n");
	}
}

void lls_alert_on(u8_t level) 
{
	if (gatt_lls.alert_level != LLS_ALERT_LEVEL_NO_ALERT) {
		k_timer_start(&lls_alert_timer, K_SECONDS(2),  K_SECONDS(2));
	}
}

static void lls_alert_timer_init(void)
{
	k_timer_init(&lls_alert_timer, lls_alert_handler, NULL);
} 

static ssize_t lls_alert_level_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_lls.alert_level,
				 sizeof(gatt_lls.alert_level));
}

static ssize_t lls_alert_level_write(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(gatt_lls.alert_level)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	return len;
}

/* Link Loss Service Declaration */
static struct bt_gatt_attr lls_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LLS),
	BT_GATT_CHARACTERISTIC(BT_UUID_LLS_ALERT_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_LLS_ALERT_LEVEL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE ,
		lls_alert_level_read, lls_alert_level_write, &gatt_lls.alert_level),
};

static struct bt_gatt_service lls_svc = BT_GATT_SERVICE(lls_attrs);

void lls_init(void)
{
	int err;

	gatt_lls.alert_level = LLS_ALERT_LEVEL_NO_ALERT;
	
	err = bt_gatt_service_register(&lls_svc);
	if (err) {
		printk("lls_init failed\n");
	}
	
	lls_alert_timer_init();
}

