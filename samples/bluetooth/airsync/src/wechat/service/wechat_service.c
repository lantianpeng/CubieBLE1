/** @file
 *  @brief Wecaht Service sample
 */

/*
 * Copyright (c) 2017-2018 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: lipeng<lipeng@actions-semi.com>
 *
 * Change log:
 *	2017/5/5: Created by lipeng.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "wechat_service"
#define SYS_LOG_LEVEL 4//3		/* 4, debug */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <logging/sys_log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


#include "wechat_service.h"

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

#define BT_UUID_WECHAT		BT_UUID_DECLARE_16(0xFEE7)
#define BT_UUID_WECHAT_WR_CH	BT_UUID_DECLARE_16(0xFEC7)
#define BT_UUID_WECHAT_IND_CH	BT_UUID_DECLARE_16(0xFEC8)
#define BT_UUID_WECHAT_RD_CH	BT_UUID_DECLARE_16(0xFEC9)

#define MANUFACTURER_DATA_SIZE	(8)

#define WECHAT_MAX_PKT		(20)

static struct bt_gatt_ccc_cfg	g_ind_ch_ccc_cfg[CONFIG_BT_MAX_PAIRED];
static uint8_t			g_indicating;
static struct bt_gatt_indicate_params g_ind_params;

static wechat_ind_status_cb_t	g_ind_status_cb;

/* manufacturer data, will be filled in wechat_init() */
static uint8_t			g_manufacturer_data[MANUFACTURER_DATA_SIZE];

static wechat_recv_cb_t		g_recv_cb;

static struct bt_conn		*g_cur_conn;

static ssize_t wr_ch_write_cb(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	SYS_LOG_DBG("buf %p, len %d, offset %d\n", buf, len, offset);

	if (g_recv_cb)
		g_recv_cb((void *)buf, len);

	return len;
}

static void ind_ch_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	bool status = (value == BT_GATT_CCC_INDICATE);

	SYS_LOG_INF("indicate started? %d\n", status);

	/* reset g_indicating flag anycase! */
	g_indicating = 0;

	if (g_ind_status_cb)
		g_ind_status_cb(status);
}

static ssize_t rd_ch_read_cb(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	SYS_LOG_DBG("buf %p, len %d, offset %d\n", buf, len, offset);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &g_manufacturer_data[2], 6);
}


static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_WECHAT),

	BT_GATT_CHARACTERISTIC(BT_UUID_WECHAT_WR_CH, BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_WECHAT_WR_CH, BT_GATT_PERM_WRITE,
			   NULL, wr_ch_write_cb, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_WECHAT_IND_CH, BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_WECHAT_IND_CH, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   NULL, NULL, NULL),
	BT_GATT_CCC(g_ind_ch_ccc_cfg, ind_ch_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_WECHAT_RD_CH, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_WECHAT_RD_CH, BT_GATT_PERM_READ,
			   rd_ch_read_cb, NULL, NULL),
};

static struct bt_gatt_service wechat_svc = BT_GATT_SERVICE(attrs);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),

	/* wechat serviceUUID */
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xe7, 0xfe),

	/* manufacturer data */
	BT_DATA(BT_DATA_MANUFACTURER_DATA, g_manufacturer_data,
		MANUFACTURER_DATA_SIZE),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		SYS_LOG_ERR("Connection failed (err %u)\n", err);
		g_cur_conn = NULL;
	} else {
		SYS_LOG_INF("Connected\n");
		g_cur_conn = conn;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	SYS_LOG_INF("Disconnected (reason %u)\n", reason);

	g_cur_conn = NULL;

	/* ensure advertiser is on */
	wechat_advertise(true);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

int wechat_init(wechat_recv_cb_t recv_cb, wechat_ind_status_cb_t status_cb)
{
	int err = 0;

	struct bt_le_oob oob;

	struct _bt_gatt_ccc *ccc;

	SYS_LOG_DBG("init\n");

	g_recv_cb = recv_cb;
	g_ind_status_cb = status_cb;

	err = bt_le_oob_get_local(&oob);
	if (err) {
		SYS_LOG_ERR("get local oob failed (err %d)\n", err);
		return err;
	}

	/* mac address(bd address) which is big endian */
	g_manufacturer_data[2] = oob.addr.a.val[5];
	g_manufacturer_data[3] = oob.addr.a.val[4];
	g_manufacturer_data[4] = oob.addr.a.val[3];
	g_manufacturer_data[5] = oob.addr.a.val[2];
	g_manufacturer_data[6] = oob.addr.a.val[1];
	g_manufacturer_data[7] = oob.addr.a.val[0];

	/* reset ccc value */
	ccc = (struct _bt_gatt_ccc *)attrs[5].user_data;
	ccc->value = 0;

	err = bt_gatt_service_register(&wechat_svc);
	if (err) {
		SYS_LOG_ERR("bt_gatt_register failed (err %d)\n", err);
		return err;
	}

	bt_conn_cb_register(&conn_callbacks);

	return 0;
}

int wechat_advertise(bool on)
{
	int err;

	SYS_LOG_INF("%s\n", on ? "on" : "off");

	if (on)
		err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
				      sd, ARRAY_SIZE(sd));
	else
		err = bt_le_adv_stop();

	if (err) {
		SYS_LOG_INF("err %d\n", err);
		return err;
	}

	return 0;
}

void wechat_disconnect(void)
{
	if (g_cur_conn)
		bt_conn_disconnect(g_cur_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	/* wait for discconected event */
	while (g_cur_conn)
		k_sleep(100);
}

static void __indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  uint8_t err)
{
	SYS_LOG_DBG("Indication %s\n", err != 0 ? "fail" : "success");
	g_indicating = 0;
}

static void __send_and_wait(void)
{
	int ret;

	if ((ret = bt_gatt_indicate(g_cur_conn, &g_ind_params)) == 0)
		g_indicating = 1;
	else
		SYS_LOG_ERR("bt_gatt_indicate failed(%d)\n", ret);

	while (g_indicating)
		k_sleep(100);
}

int wechat_send_data(void *buf, int len)
{
	int sent = 0;

	char tmp_buf[WECHAT_MAX_PKT];

	SYS_LOG_DBG("buf %p, len %d\n", buf, len);

	g_ind_params.attr = &attrs[4];
	g_ind_params.func = __indicate_cb;
	g_ind_params.len = WECHAT_MAX_PKT;

	while (len > WECHAT_MAX_PKT) {
		g_ind_params.data = buf + sent;

		__send_and_wait();

		len -= WECHAT_MAX_PKT;
		sent += WECHAT_MAX_PKT;
	}

	if (len > 0) {
		memset(tmp_buf, 0, WECHAT_MAX_PKT);
		memcpy(tmp_buf, buf + sent, len);

		g_ind_params.data = tmp_buf;

		__send_and_wait();
	}

	return len;
}
