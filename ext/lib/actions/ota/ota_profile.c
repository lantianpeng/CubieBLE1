/** @file
 *  @brief ota Service sample
 */

/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_OTA_WITH_APP

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

#include "ota_profile.h"

#define SYS_LOG_DOMAIN "ota"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

/* Custom Service Variables */
static struct bt_uuid_128 ota_uuid = BT_UUID_INIT_128(
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
0x00, 0x10, 0x00, 0x00, 0xf6, 0xfe,	0x00, 0x00);

static struct bt_uuid_128 ota_dc_uuid = BT_UUID_INIT_128(
	0x65, 0x78, 0x61, 0x63, 0x74, 0x4c, 0x45, 0xb0,
0xd5, 0x4e, 0xf2, 0x2f, 0x02, 0x00,	0x5f, 0x00);

static struct bt_uuid_128 ota_ftc_uuid = BT_UUID_INIT_128(
	0x65, 0x78, 0x61, 0x63, 0x74, 0x4c, 0x45, 0xb0,
0xd5, 0x4e, 0xf2, 0x2f, 0x03, 0x00,	0x5f, 0x00);

static struct bt_uuid_128 ota_ftd_uuid = BT_UUID_INIT_128(
	0x65, 0x78, 0x61, 0x63, 0x74, 0x4c, 0x45, 0xb0,
0xd5, 0x4e, 0xf2, 0x2f, 0x04, 0x00,	0x5f, 0x00);

static const struct bt_uuid_128 ota_au_uuid = BT_UUID_INIT_128(
	0x65, 0x78, 0x61, 0x63, 0x74, 0x4c, 0x45, 0xb0,
0xd5, 0x4e, 0xf2, 0x2f, 0x05, 0x00,	0x5f, 0x00);

static struct bt_gatt_ccc_cfg ota_dc_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg ota_ftc_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg ota_ftd_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg ota_au_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t ota_dc;
static u8_t ota_ftc;
static u8_t ota_ftd;
static u8_t ota_au;

static u8_t bt_attr_get_id(const struct bt_gatt_attr *attr);

static void ota_dc_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	ota_dc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	SYS_LOG_DBG("ota_dc : %d", ota_dc);
}

static void ota_ftc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			u16_t value)
{
	ota_ftc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	SYS_LOG_DBG("ota_ftc : %d", ota_ftc);
}

static void ota_ftd_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			u16_t value)
{
	ota_ftd = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	SYS_LOG_DBG("ota_ftd : %d", ota_ftd);
}
static void ota_au_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			u16_t value)
{
	ota_au = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	SYS_LOG_DBG("ota_au : %d", ota_au);
}
typedef u8_t (*atts_write_cback_t)(struct bt_conn *conn, u16_t handle, u8_t operation,
															u16_t offset, u16_t len, u8_t *p_value,
																const struct bt_gatt_attr *attr);
atts_write_cback_t p_write_cback;
static u8_t is_need_writeCb(u8_t attr_id)
{
	switch (attr_id) {
	case OTA_DC_HDL:
	case OTA_FTC_HDL:
	case OTA_FTD_HDL:
	case OTA_AU_HDL:
			return true;
	}
	return false;
}
static ssize_t write_ota(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t attr_id = bt_attr_get_id(attr);

	if (offset + len > (bt_gatt_get_mtu(conn) - 3))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	if (p_write_cback && is_need_writeCb(attr_id))
		p_write_cback(conn, attr_id, 1, offset, len, (u8_t *)buf, attr);

	return len;
}

/* Vendor Primary Service Declaration */
static struct bt_gatt_attr ota_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&ota_uuid),

	BT_GATT_CHARACTERISTIC(&ota_dc_uuid.uuid, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(&ota_dc_uuid.uuid, BT_GATT_PERM_WRITE, NULL, write_ota, NULL),
	BT_GATT_CCC(ota_dc_ccc_cfg, ota_dc_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(&ota_ftc_uuid.uuid, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&ota_ftc_uuid.uuid, BT_GATT_PERM_WRITE, NULL, write_ota, NULL),
	BT_GATT_CCC(ota_ftc_ccc_cfg, ota_ftc_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(&ota_ftd_uuid.uuid, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(&ota_ftd_uuid.uuid, BT_GATT_PERM_WRITE, NULL, write_ota, NULL),
	BT_GATT_CCC(ota_ftd_ccc_cfg, ota_ftd_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(&ota_au_uuid.uuid, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(&ota_au_uuid.uuid, BT_GATT_PERM_WRITE, NULL, write_ota, NULL),
	BT_GATT_CCC(ota_au_ccc_cfg, ota_au_ccc_cfg_changed),
};

static u8_t bt_attr_get_id(const struct bt_gatt_attr *attr)
{
	return attr - ota_attrs;
}

static struct bt_gatt_service ota_svc = BT_GATT_SERVICE(ota_attrs);

void ota_init(void)
{
	bt_gatt_service_register(&ota_svc);
}

u8_t ota_ccc_enabled(struct bt_conn *conn, u8_t handle)
{
		switch (handle) {
		case OTA_DC_HDL:
			if (!ota_dc)
				return 0;
		break;
		case OTA_FTC_HDL:
			if (!ota_ftc)
				return 0;
		break;
		case OTA_FTD_HDL:
			if (!ota_ftd)
				return 0;
		break;
		case OTA_AU_HDL:
			if (!ota_au)
				return 0;
		break;
	}
	return 1;
}

void ota_send_notify(struct bt_conn *conn, u8_t handle, u16_t len, u8_t *p_value)
{
	bt_gatt_notify(conn, &ota_attrs[handle], p_value, len);
}

void ota_register(atts_write_cback_t write_cback)
{
	p_write_cback = write_cback;
}
#endif
