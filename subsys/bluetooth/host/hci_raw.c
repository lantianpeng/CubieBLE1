/* hci_userchan.c - HCI user channel Bluetooth handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "errno.h"
#include <atomic.h>

#include <bluetooth/hci_driver.h>
#include <bluetooth/hci_raw.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#include "common/log.h"

#include "hci_ecc.h"
#include "monitor.h"
#include "hci_raw_internal.h"
#include "soc_patch.h"

#define CONFIG_BT_RX_PRIO 8

static struct k_fifo *raw_rx;

static BT_STACK_NOINIT(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);


NET_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
		    256, BT_BUF_USER_DATA_MIN, NULL);

struct bt_dev_raw_t bt_dev_raw;

int bt_hci_driver_register_raw(const struct bt_hci_driver *drv)
{
	if (bt_dev_raw.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	bt_dev_raw.drv = drv;

	BT_DBG("Registered %s", drv->name ? drv->name : "");

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, drv->bus,
					 BT_ADDR_ANY, drv->name ? drv->name : "bt0");

	return 0;
}

struct net_buf *bt_buf_get_rx_raw(enum bt_buf_type type,  u16_t len, s32_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&hci_rx_pool, timeout);

	if (buf) {
		bt_buf_set_type(buf, type);
	}

	return buf;
}


struct net_buf *bt_buf_get_cmd_complete_raw(s32_t timeout)
{
	return NULL;
}

int bt_recv_raw(struct net_buf *buf)
{
	BT_DBG("buf %p len %u", buf, buf->len);

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	/* Queue to RAW rx queue */
	net_buf_put(raw_rx, buf);

	return 0;
}

int bt_recv_prio_raw(struct net_buf *buf)
{
	return bt_recv(buf);
}

int bt_send_raw(struct net_buf *buf)
{
	BT_DBG("buf %p len %u type %u", buf, buf->len, bt_buf_get_type(buf));

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC))
		return bt_hci_ecc_send(buf);

	return bt_dev_raw.drv->send(buf);
}

int bt_enable_raw(struct k_fifo *rx_queue)
{
	const struct bt_hci_driver *drv = bt_dev_raw.drv;
	int err;

	BT_DBG("");

	err = bt_rx_thread_init(rx_thread_stack, K_THREAD_STACK_SIZEOF(rx_thread_stack), CONFIG_BT_RX_PRIO);
	if (err) {
		return err;
	}

	raw_rx = rx_queue;

	if (!bt_dev_raw.drv) {
		BT_ERR("No HCI driver registered");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_hci_ecc_init();
	}

	err = drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	BT_INFO("Bluetooth enabled in RAW mode");

	return 0;
}

#include <bluetooth/buf.h>
#define HCI_CMD_TYPE                                 1       /*!< HCI command packet */
#define HCI_ACL_TYPE                                 2       /*!< HCI ACL data packet */
#define HCI_EVT_TYPE                                 4       /*!< HCI event packet */


u8_t *llc_hci_buf_alloc_new(u16_t len, u8_t type)
{
	u8_t *pBuf;

	struct net_buf *rx_buf;

	if (type == HCI_ACL_TYPE) {
		rx_buf = bt_buf_get_rx_raw(BT_BUF_ACL_IN, len, K_NO_WAIT);
	} else if (type == HCI_EVT_TYPE) {
		rx_buf = bt_buf_get_rx_raw(BT_BUF_EVT, len, K_NO_WAIT);
	} else {
		return NULL;
	}
	if (rx_buf == NULL) {
		return NULL;
	}
	pBuf =  rx_buf->b.data;
	return (u8_t *)pBuf;
}

FUNCTION_PATCH_REGISTER(llc_hci_buf_alloc, llc_hci_buf_alloc_new, llc_hci_buf_alloc);


FUNCTION_PATCH_REGISTER(bt_hci_driver_register, bt_hci_driver_register_raw, bt_hci_driver_register);


FUNCTION_PATCH_REGISTER(bt_buf_get_cmd_complete, bt_buf_get_cmd_complete_raw, bt_buf_get_cmd_complete);


FUNCTION_PATCH_REGISTER(bt_recv, bt_recv_raw, bt_recv);


FUNCTION_PATCH_REGISTER(bt_recv_prio, bt_recv_prio_raw, bt_recv_prio);
