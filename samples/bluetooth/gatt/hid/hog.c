/** @file
 *  @brief HoG Service sample
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

#include <hog.h>

#define SYS_LOG_DOMAIN "hog"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>


#define BT_GATT_CCC_INTERNAL(_ccc)					\
{									\
	.uuid = BT_UUID_GATT_CCC,					\
	.perm = BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,			\
	.read = bt_gatt_attr_read_ccc,					\
	.write = bt_gatt_attr_write_ccc,				\
	.user_data = _ccc,\
}

/*! HID control block */
struct hid_cb_t {
	const struct hid_config_t *p_config;
};

struct hid_cb_t hid_cb;

enum {
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
	u16_t version; /* version number of base USB HID Specification */
	u8_t code; /* country HID Device hardware is localized for. */
	u8_t flags;
} __packed;

struct hids_report {
	u8_t id; /* report id */
	u8_t type; /* report type */
} __packed;

/* HID Info Value: HID Spec version, country code, flags */
const struct hids_info hid_info_val = {
	.version = HID_VERSION,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

/* HID Control Point Value */
u8_t hid_cp_val;

struct _bt_gatt_ccc hid_ccc[5];

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep1_id_map = {
	.id = 0x01,
	.type = HID_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep2_id_map = {
	.id = CONFIG_HID_VOICE_REPORT_ID,
	.type = HID_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep3_id_map = {
	.id = 0x03,
	.type = HID_REPORT_TYPE_INPUT,
};

/* HID Output Report Reference - ID, Type */
const struct hids_report hid_val_orep_id_map = {
	.id = 0x00,
	.type = HID_REPORT_TYPE_OUTPUT,
};

/* HID Feature Report Reference - ID, Type */
const struct hids_report hid_val_frep_id_map = {
	.id = 0x00,
	.type = HID_REPORT_TYPE_FEATURE,
};

/* HID Protocol Mode Value */
static u8_t hid_pm_val = HID_PROTOCOL_MODE_REPORT;
static const u16_t hid_len_pm_val = sizeof(hid_pm_val);

static u8_t bt_attr_get_id(const struct bt_gatt_attr *attr);
static struct hid_report_id_map_t *hid_get_report_id_map(u16_t handle);

static ssize_t read_info(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_cb.p_config->hid_report_map,
				 hid_cb.p_config->hid_report_map_len);
}

static ssize_t read_ext_report(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       u16_t len, u16_t offset)
{
	u8_t hid_ext_report[] = {((u8_t) (BT_UUID_BAS_BATTERY_LEVEL_VAL)), ((u8_t)((BT_UUID_BAS_BATTERY_LEVEL_VAL) >> 8))};
	u16_t hid_len_ext_report = sizeof(hid_ext_report);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_ext_report,
				 hid_len_ext_report);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	if (offset + len > 1)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	/* notify the application */
	if (hid_cb.p_config->info_cback != NULL)
		hid_cb.p_config->info_cback(conn, HID_INFO_CONTROL_POINT, *((u8_t *)buf));

	return len;
}

static ssize_t read_report_reference(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static ssize_t read_report_value(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	struct hid_report_id_map_t *p_id_map;

	if (offset > HID_MAX_REPORT_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	/* notify the application */
	if (hid_cb.p_config->input_cback != NULL) {
		p_id_map = hid_get_report_id_map(bt_attr_get_id(attr));

		if (p_id_map != NULL)
			hid_cb.p_config->input_cback(conn, p_id_map->id, len, (u8_t *)buf);
	}

	return len;
}

static ssize_t write_report_value(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	struct hid_report_id_map_t *p_id_map;

	if (offset + len > HID_MAX_REPORT_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	/* notify the application */
	if (hid_cb.p_config->output_cback != NULL) {
		p_id_map = hid_get_report_id_map(bt_attr_get_id(attr));

		if (p_id_map != NULL)
			hid_cb.p_config->output_cback(conn, p_id_map->id, len, (u8_t *)buf);
	}

	return len;
}

static void hid_ccc_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	hid_set_ccc_table_value(bt_attr_get_id(attr), value);
	SYS_LOG_DBG("handle: %d, ccc_value : %d", bt_attr_get_id(attr), value);
}

static ssize_t read_protocol_mode(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 hid_len_pm_val);
}


static ssize_t write_protocol_mode(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	if (offset + len > 1)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	if (hid_cb.p_config->info_cback != NULL)
		hid_cb.p_config->info_cback(conn, HID_INFO_PROTOCOL_MODE, *((u8_t *)buf));

	return len;
}

/* HID Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_INFO, BT_GATT_PERM_READ_ENCRYPT,
			   read_info, NULL, (void *)&hid_info_val),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_MAP, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_map, NULL, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_EXT_REPORT, BT_GATT_PERM_READ_ENCRYPT,
			   read_ext_report, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_CTRL_POINT, BT_GATT_PERM_WRITE_ENCRYPT,
			   NULL, write_ctrl_point, &hid_cp_val),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KEYBOARD_IN,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_BOOT_KEYBOARD_IN, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_value, NULL, NULL),
	BT_GATT_CCC_INTERNAL(&hid_ccc[0]),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KEYBOARD_OUT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_BOOT_KEYBOARD_OUT, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			   read_report_value, write_report_value, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_MOUSE_IN,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_BOOT_MOUSE_IN, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_value, NULL, (void *)NULL),
	BT_GATT_CCC_INTERNAL(&hid_ccc[1]),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_value, NULL, NULL),
	BT_GATT_CCC_INTERNAL(&hid_ccc[2]),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep1_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_value, NULL, NULL),
	BT_GATT_CCC_INTERNAL(&hid_ccc[3]),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep2_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_value, NULL, NULL),
	BT_GATT_CCC_INTERNAL(&hid_ccc[4]),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep3_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			   read_report_value, write_report_value, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_orep_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			   read_report_value, write_report_value, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_frep_id_map),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_PROTOCOL_MODE, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
				 read_protocol_mode, write_protocol_mode, &hid_pm_val),
};

static u8_t bt_attr_get_id(const struct bt_gatt_attr *attr)
{
	return attr - attrs;
}

static struct bt_gatt_service hog_svc = BT_GATT_SERVICE(attrs);

__init_once_text void hog_init(void)
{
	bt_gatt_service_register(&hog_svc);
}

struct hids_ccc {
	u16_t handle; /* ccc handle */
	u8_t value;    /* ccc value */
} __packed;


struct hids_ccc hids_ccc_table[] = {
	/* handle                           value*/
{HID_KEYBOARD_BOOT_IN_CH_CCC_HDL, 0},
{HID_MOUSE_BOOT_IN_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_1_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_2_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_3_CH_CCC_HDL, 0},
};

void hid_set_ccc_table_value(u16_t handle, u8_t value)
{
	u8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].handle == handle) {
			hids_ccc_table[i].value = value;
			break;
		}
	}
}

u8_t hid_get_ccc_table_value(u16_t handle)
{
	u8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].handle == handle)
			return hids_ccc_table[i].value;
	}
	return 0;
}

u8_t hid_ccc_is_enabled(u16_t handle)
{
	return (hid_get_ccc_table_value(handle) == BT_GATT_CCC_NOTIFY);
}

u8_t all_hid_ccc_is_enabled(void)
{
	u8_t ret = true;
	u8_t i = 0;
		
	for (i = 2; i < ARRAY_SIZE(hids_ccc_table); i++) {
			if (hids_ccc_table[i].value != BT_GATT_CCC_NOTIFY) {
				ret = false;
				break;
			}
	}
	
	return ret;
}

u16_t hid_get_report_handle(u8_t type, u8_t id)
{
	struct hid_report_id_map_t *p_map;
	u8_t count;
	s8_t i;

	if (hid_cb.p_config == NULL) {
		SYS_LOG_ERR("hid_cb.p_config is not init!");
		return 0;
	}

	if (hid_cb.p_config->p_report_id_map == NULL) {
		SYS_LOG_ERR("hid_cb.p_config->p_report_id_map is not init");
		return 0;
	}

	p_map = hid_cb.p_config->p_report_id_map;
	count = hid_cb.p_config->report_id_map_size;

	for (i = 0; i < count; i++) {
		if (p_map[i].type == type && p_map[i].id == id)
			return p_map[i].handle;
	}

	return 0;
}

static struct hid_report_id_map_t *hid_get_report_id_map(u16_t handle)
{
	struct hid_report_id_map_t *p_map;
	u8_t count;
	s8_t i;

	if (hid_cb.p_config == NULL) {
		SYS_LOG_ERR("hid_cb.p_config is not init!");
		return NULL;
	}

	if (hid_cb.p_config->p_report_id_map == NULL) {
		SYS_LOG_ERR("hid_cb.p_config->p_report_id_map is not init");
		return NULL;
	}

	p_map = hid_cb.p_config->p_report_id_map;
	count = hid_cb.p_config->report_id_map_size;

	for (i = 0; i < count; i++) {
		if (p_map[i].handle == handle)
			return &p_map[i];
	}

	return NULL;
}

u8_t hid_get_protocol_mode(void)
{
	return hid_pm_val;
}

void hid_set_protocol_mode(u8_t protocol_mode)
{
	hid_pm_val = protocol_mode;
}

int hid_send_input_report(struct bt_conn *conn, u8_t report_id, u16_t len, u8_t *p_value)
{
	u16_t handle = hid_get_report_handle(HID_REPORT_TYPE_INPUT, report_id);

	if ((handle != 0) && hid_ccc_is_enabled(handle+1))
		return bt_gatt_notify(conn, &attrs[handle], p_value, len);
	else
		return -EIO;
}

__init_once_text void hid_init(const struct hid_config_t *p_config)
{
	s8_t i;
	struct bt_gatt_ccc_cfg	    *p_ccc = NULL;

	if (p_config == NULL) {
		SYS_LOG_ERR("hid_init failed!");
		return;
	}

	p_ccc = p_config->p_ccc_cfg;
	for (i = 0; i < 5; i++) {
		hid_ccc[i].cfg = p_ccc;
		p_ccc += p_config->cfg_len;
		hid_ccc[i].cfg_len = p_config->cfg_len;
		hid_ccc[i].cfg_changed = hid_ccc_changed;
	}

	/* Store the configuration */
	hid_cb.p_config = p_config;
}


