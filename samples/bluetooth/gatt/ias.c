/** @file
 *  @brief IAS Service sample
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

#define IAS_ALERT_LEVEL_NO_ALERT    0
#define IAS_ALERT_LEVEL_MILD_ALERT  1
#define IAS_ALERT_LEVEL_HIGH_ALERT  2

struct bt_gatt_ias {
	u8_t alert_level;
 } __packed;

static struct bt_gatt_ias gatt_ias;
static struct k_timer ias_alert_timer;

static void ias_alert_on(void) 
{
	k_timer_start(&ias_alert_timer, K_SECONDS(2),  K_SECONDS(2));
}

void ias_alert_off(void)
{
	gatt_ias.alert_level = IAS_ALERT_LEVEL_NO_ALERT;
	k_timer_stop(&ias_alert_timer);
}

static void ias_alert_handler(struct k_timer *dummy)
{
	if (IAS_ALERT_LEVEL_MILD_ALERT == gatt_ias.alert_level) {
		printk("Warning: milddle alert!\n");
	} else  if (IAS_ALERT_LEVEL_HIGH_ALERT == gatt_ias.alert_level) {
		printk("Warning: high alert!\n");
	}
}

static void ias_alert_timer_init(void)
{
	k_timer_init(&ias_alert_timer, ias_alert_handler, NULL);
} 

static ssize_t ias_alert_level_write(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(gatt_ias.alert_level)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	if (gatt_ias.alert_level == IAS_ALERT_LEVEL_MILD_ALERT
		|| gatt_ias.alert_level == IAS_ALERT_LEVEL_HIGH_ALERT) {
		ias_alert_on();
	} else {
		ias_alert_off();
	}
	return len;
}

/* Immediate Alert Service Declaration */
static struct bt_gatt_attr ias_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_IAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_IAS_ALERT_LEVEL, BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(BT_UUID_IAS_ALERT_LEVEL, BT_GATT_PERM_WRITE, NULL, ias_alert_level_write, &gatt_ias.alert_level),
};

static struct bt_gatt_service ias_svc = BT_GATT_SERVICE(ias_attrs);

void ias_init(void)
{
	gatt_ias.alert_level = IAS_ALERT_LEVEL_NO_ALERT;
	
	bt_gatt_service_register(&ias_svc);
	
	ias_alert_timer_init();
}

