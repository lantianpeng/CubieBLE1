/** @file
 *  @brief DIS Service sample
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

struct bt_gatt_dis {
	char *manufacturer;
	char *model;
	char *serial_number;
	char *hardware_version;
	char *software_version;
	char *firmware_version;
	uint64_t system_id;      // Manufacturer Identifie(LSO uint40), Organizationally Unique Identifier(MSO uint24)
	uint64_t ieee_regulatory_certification; //see IEEE Std 11073-20601 2008 Health Informatics
	uint64_t pnp_id;	     // Vendor ID Source(LSO uin8), Vendor ID(uin16), Product ID(uin16), Product Version(MSO uin16)
};

static struct bt_gatt_dis gatt_dis = {
	.manufacturer = "actions",
	.model = "acts_ble",
	.serial_number = "ACTS20180315",
	.hardware_version = "V1.0",
	.software_version = "BLE V1.0",
	.firmware_version = "BLE V1.0.0",
	.system_id = 0x123456FFFE9ABCDE,
	.ieee_regulatory_certification = 0x110732061,
	.pnp_id = 0x111111222222,
};

static ssize_t read_serial_number(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, gatt_dis.serial_number,
				 strlen(gatt_dis.serial_number));
}

static ssize_t read_hardware_version(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, gatt_dis.hardware_version,
				 strlen(gatt_dis.hardware_version));
}

static ssize_t read_software_version(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, gatt_dis.software_version,
				 strlen(gatt_dis.software_version));
}

static ssize_t read_firmware_version(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, gatt_dis.firmware_version,
				 strlen(gatt_dis.firmware_version));
}

static ssize_t read_system_id(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_dis.system_id,
				 sizeof(gatt_dis.system_id));
}

static ssize_t read_ieee_regulatory_certification(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_dis.ieee_regulatory_certification,
				 sizeof(gatt_dis.ieee_regulatory_certification));
}

static ssize_t read_pnp_id(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_dis.pnp_id,
				 7);
}

static ssize_t read_model(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, gatt_dis.model,
				 strlen(gatt_dis.model));
}

static ssize_t read_manufacturer(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, gatt_dis.manufacturer,
				 strlen(gatt_dis.manufacturer));
}

/* Device Information Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_PERM_READ, read_model, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_PERM_READ, read_manufacturer, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_PERM_READ, read_serial_number, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_PERM_READ, read_hardware_version, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SOFTWARE_REVISION, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_SOFTWARE_REVISION, BT_GATT_PERM_READ, read_software_version, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_PERM_READ, read_firmware_version, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SYSTEM_ID, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_SYSTEM_ID, BT_GATT_PERM_READ, read_system_id, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_IEEE_REGULATORY_CERTIFICATION_DATA_LIST, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_IEEE_REGULATORY_CERTIFICATION_DATA_LIST, BT_GATT_PERM_READ, read_ieee_regulatory_certification, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_DIS_PNP_ID, BT_GATT_PERM_READ, read_pnp_id, NULL, NULL),
};

static struct bt_gatt_service dis_svc = BT_GATT_SERVICE(attrs);

void dis_init(const char *model, const char *manuf)
{
	if (model) {
		gatt_dis.model = model;
	}

	if (manuf) {
		gatt_dis.manufacturer= manuf;
	}

	bt_gatt_service_register(&dis_svc);
}
