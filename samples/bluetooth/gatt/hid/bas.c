/** @file
 *  @brief BAS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
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
#include <bas.h>

#define BT_GATT_CCC_INTERNAL(_ccc)					\
{									\
	.uuid = BT_UUID_GATT_CCC,					\
	.perm = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,			\
	.read = bt_gatt_attr_read_ccc,					\
	.write = bt_gatt_attr_write_ccc,				\
	.user_data = _ccc,\
}

hw_read_batt_func_t hw_read_batt_func;

struct _bt_gatt_ccc blvl_ccc;

static u8_t simulate_blvl;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	simulate_blvl = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u8_t battery = 100;

	if (hw_read_batt_func)
		(hw_read_batt_func)(&battery);

	const char *value = (const char *)&battery;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Battery Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_PERM_READ,
			   read_blvl, NULL, NULL),
	BT_GATT_CCC_INTERNAL(&blvl_ccc),
};

struct bt_gatt_service bas_svc = BT_GATT_SERVICE(attrs);

__init_once_text void bas_init(hw_read_batt_func_t func, void	*ccc_cfg, size_t cfg_len)
{
	bt_gatt_service_register(&bas_svc);
	hw_read_batt_func = func;
	blvl_ccc.cfg = (struct bt_gatt_ccc_cfg	*)ccc_cfg;
	blvl_ccc.cfg_len = cfg_len;
	blvl_ccc.cfg_changed = blvl_ccc_cfg_changed;
}

void bas_notify(void)
{
	u8_t battery = 100;

	if (!simulate_blvl)
		return;

	if (hw_read_batt_func)
		(hw_read_batt_func)(&battery);

	bt_gatt_notify(NULL, &attrs[2], &battery, sizeof(battery));
}
