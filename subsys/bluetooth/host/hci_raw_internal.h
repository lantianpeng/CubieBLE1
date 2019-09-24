/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HCI_RAW_INTERNAL_H
#define __BT_HCI_RAW_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

struct bt_dev_raw_t {
	/* Registered HCI driver */
	const struct bt_hci_driver *drv;
};

extern struct bt_dev_raw_t bt_dev_raw;

extern u8_t *llc_hci_buf_alloc(u16_t len, u8_t type);

extern int bt_hci_driver_register(const struct bt_hci_driver *drv);

extern struct net_buf *bt_buf_get_cmd_complete(s32_t timeout);

extern int bt_recv(struct net_buf *buf);

extern int bt_recv_prio(struct net_buf *buf);

extern int bt_rx_thread_init(k_thread_stack_t stack, size_t stack_size, int prio);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HCI_RAW_INTERNAL_H */
