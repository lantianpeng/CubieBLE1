/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <init.h>
#include <uart.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>

#include "common/log.h"

static struct device *hci_uart_dev;
static BT_STACK_NOINIT(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;

/* HCI command buffers */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(cmd_tx_pool, CONFIG_BT_HCI_CMD_COUNT, CMD_BUF_SIZE,
		    BT_BUF_USER_DATA_MIN, NULL);

#if defined(CONFIG_BT_CTLR)
#define BT_L2CAP_MTU (CONFIG_BT_CTLR_TX_BUFFER_SIZE - BT_L2CAP_HDR_SIZE)
#else
#define BT_L2CAP_MTU 65 /* 64-byte public key + opcode */
#endif /* CONFIG_BT_CTLR */

/** Data size needed for ACL buffers */
#define BT_BUF_ACL_SIZE BT_L2CAP_BUF_SIZE(BT_L2CAP_MTU)

#if defined(CONFIG_BT_CTLR_TX_BUFFERS)
#define TX_BUF_COUNT CONFIG_BT_CTLR_TX_BUFFERS
#else
#define TX_BUF_COUNT 6
#endif

NET_BUF_POOL_DEFINE(acl_tx_pool, TX_BUF_COUNT, BT_BUF_ACL_SIZE,
		    BT_BUF_USER_DATA_MIN, NULL);

static K_FIFO_DEFINE(tx_queue);
#define H4_NONE 0x00
#define H4_CMD 0x01
#define H4_ACL 0x02
#define H4_SCO 0x03
#define H4_EVT 0x04

/* Length of a discard/flush buffer.
 * This is sized to align with a BLE HCI packet:
 * 1 byte H:4 header + 32 bytes ACL/event data
 * Bigger values might overflow the stack since this is declared as a local
 * variable, smaller ones will force the caller to call into discard more
 * often.
 */
#define H4_DISCARD_LEN 33


static struct {
	struct net_buf *buf;

	u16_t    remaining;
	u16_t    discard;

	bool     have_hdr;
	u8_t     hdr_len;

	u8_t     type;
	union {
		struct bt_hci_cmd_hdr cmd;
		struct bt_hci_acl_hdr acl;
		u8_t   hdr[BT_HCI_ACL_HDR_SIZE];
	};

} rx;

static void reset_rx(void)
{
	rx.type = H4_NONE;
	rx.remaining = 0;
	rx.have_hdr = false;
	rx.hdr_len = 0;
}

static void h4_get_type(void)
{
	/* Get packet type */
	if (uart_fifo_read(hci_uart_dev, &rx.type, 1) != 1) {
		printk("Unable to read packet type\n");
		rx.type = H4_NONE;
		return;
	}

	rx.remaining = 0;
	switch (rx.type) {
	case H4_CMD:
		rx.hdr_len = BT_HCI_CMD_HDR_SIZE;
		break;
	case H4_ACL:
		rx.hdr_len = BT_HCI_ACL_HDR_SIZE;
		break;
	default:
		/* discard byte; reset state machine */
		rx.type = H4_NONE;
	}
}

static void h4_get_hdr(void)
{
	rx.remaining += uart_fifo_read(hci_uart_dev, &rx.hdr[rx.remaining], rx.hdr_len - rx.remaining);

    /* see if entire header has been read */
	if (rx.remaining == rx.hdr_len) {
		if (rx.type == H4_CMD) {
			rx.buf = net_buf_alloc(&cmd_tx_pool, K_NO_WAIT);
			if (rx.buf) {
				bt_buf_set_type(rx.buf, BT_BUF_CMD);
				net_buf_add_mem(rx.buf, rx.hdr, rx.hdr_len);
			} else {
				printk("No available CMD buffers!\n");
				rx.discard = rx.cmd.param_len;
				reset_rx();
				return;
			}
			/* save number of bytes left to read */
			rx.remaining = rx.cmd.param_len;
		} else if (rx.type == H4_ACL) {
			rx.buf = net_buf_alloc(&acl_tx_pool, K_NO_WAIT);
			if (rx.buf) {
				bt_buf_set_type(rx.buf, BT_BUF_ACL_OUT);
				net_buf_add_mem(rx.buf, rx.hdr, rx.hdr_len);
			} else {
				printk("No available ACL buffers!\n");
				rx.discard = rx.acl.len;
				reset_rx();
				return;
			}
			/* save number of bytes left to read */
			rx.remaining = rx.acl.len;
		}

		if (rx.remaining > net_buf_tailroom(rx.buf)) {
			printk("Not enough space in buffer\n");
			net_buf_destroy(rx.buf);
			rx.buf = NULL;
			rx.discard = rx.remaining;
			reset_rx();
			return;
		}

		if (rx.remaining == 0) {
			/* deliver data */
			if (rx.buf) {
				/* Put buffer into TX queue, thread will dequeue */
				net_buf_put(&tx_queue, rx.buf);
				rx.buf = NULL;
			}
			/* reset state machine */
			reset_rx();
		} else {
			rx.have_hdr = true;
		}
	}
}

static size_t h4_discard(struct device *uart, size_t len)
{
	u8_t buf[33];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

static void read_payload(void)
{
	int read;
	/* write incoming byte to allocated buffer */
	read = uart_fifo_read(hci_uart_dev, net_buf_tail(rx.buf), rx.remaining);
	net_buf_add(rx.buf, read);
	/* determine if entire packet has been read */
	rx.remaining -= read;

	if (rx.remaining == 0) {
		if (rx.buf) {
			/* Put buffer into TX queue, thread will dequeue */
			net_buf_put(&tx_queue, rx.buf);
			rx.buf = NULL;
		}
		/* reset state machine */
		reset_rx();
	}
}

static void read_header(void)
{
	switch (rx.type) {
	case H4_NONE:
		h4_get_type();
		return;
	case H4_CMD:
	case H4_ACL:
		h4_get_hdr();
		return;
	default:
		return;
	}
}

static void process_rx(void)
{
	if (rx.discard) {
		rx.discard -= h4_discard(hci_uart_dev, rx.discard);
		return;
	}

	if (rx.have_hdr) {
		read_payload();
	} else {
		read_header();
	}
}

static void bt_uart_isr(struct device *unused)
{
	ARG_UNUSED(unused);

	while (uart_irq_update(hci_uart_dev) && uart_irq_is_pending(hci_uart_dev)) {
		if (uart_irq_rx_ready(hci_uart_dev)) {
			process_rx();
		}
	}
}

static void tx_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct net_buf *buf;
		int err;

		/* Wait until a buffer is available */
		buf = net_buf_get(&tx_queue, K_FOREVER);
		/* Pass buffer to the stack */
		err = bt_send_raw(buf);
		if (err) {
			SYS_LOG_ERR("Unable to send (err %d)", err);
			net_buf_unref(buf);
		}

		/* Give other threads a chance to run if tx_queue keeps getting
		 * new data all the time.
		 */
		k_yield();
	}
}

static int h4_send(struct net_buf *buf)
{
	SYS_LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf),
		    buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		uart_poll_out(hci_uart_dev, H4_ACL);
		break;
	case BT_BUF_EVT:
		uart_poll_out(hci_uart_dev, H4_EVT);
		break;
	default:
		SYS_LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}

	while (buf->len) {
		uart_poll_out(hci_uart_dev, net_buf_pull_u8(buf));
	}

	net_buf_unref(buf);

	return 0;
}

static int hci_uart_init(struct device *unused)
{
	SYS_LOG_DBG("");
	hci_uart_dev = device_get_binding("UART_0");
	if (!hci_uart_dev) {
		return -EINVAL;
	}

	uart_irq_rx_disable(hci_uart_dev);
	uart_irq_tx_disable(hci_uart_dev);

	uart_irq_callback_set(hci_uart_dev, bt_uart_isr);

	uart_irq_rx_enable(hci_uart_dev);

	return 0;
}

DEVICE_INIT(hci_uart, "hci_uart", &hci_uart_init, NULL, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

u8_t BUF_POWER_MODE[] = {0x02, 0xFE, 0x01, 0x02}; /*!< Set Power Management Mode command. */
u8_t BUF_CONTINUOUS_RX[] = {0x0E, 0xFE, 0x01, 0x13};
u8_t BUF_CONTINUOUS_TX[] = {0x0F, 0xFE, 0x03, 0x13, 0x0, 0x00};
u8_t BUF_RESET[] = {0x03, 0x0c, 0x00};
u8_t BUF_LE_LE_RECEIVER_TEST[] = {0x1D, 0x20, 0x01, 0x13};
u8_t BUF_LE_TRANSMITTER_TEST[] = {0x1E, 0x20, 0x03, 0x13, 0x25, 0x00};

u8_t BUF_LE_TEST_END[] = {0x1F, 0x20, 0x00};
void send_cmd_buf(u8_t *cmdBuf, u8_t cmdLen)
{
	struct net_buf *tx_buf = NULL;

	tx_buf = net_buf_alloc(&cmd_tx_pool, K_FOREVER);
	if (tx_buf) {
		bt_buf_set_type(tx_buf, BT_BUF_CMD);
		net_buf_add_mem(tx_buf, cmdBuf, cmdLen);
	} else {
		printk("No available CMD buffers!\n");
	}
	net_buf_put(&tx_queue, tx_buf);
}

void send_single_tone(void)
{
		printk("start send_single_tone\n");
		send_cmd_buf(BUF_RESET, sizeof(BUF_RESET));

		send_cmd_buf(BUF_POWER_MODE, sizeof(BUF_POWER_MODE));
#if 1
		send_cmd_buf(BUF_CONTINUOUS_TX, sizeof(BUF_CONTINUOUS_TX));
#else
		send_cmd_buf(BUF_CONTINUOUS_RX, sizeof(BUF_CONTINUOUS_RX));
#endif

		printk("End send_single_tone\n");
}

/* TODO */
u32_t bt_ready_flag;

void app_main(void)
{
	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);

	SYS_LOG_DBG("Start");

	/* Enable the raw interface, this will in turn open the HCI driver */
	bt_enable_raw(&rx_queue);
	/* Spawn the TX thread and start feeding commands and data to the
	 * controller
	 */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack), tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	bt_ready_flag = 1;
#ifdef CONFIG_SEND_SINGLE_TONE
	send_single_tone();
	while (1)
		;
#else
	while (1) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);
		err = h4_send(buf);
		if (err) {
			SYS_LOG_ERR("Failed to send");
		}
	}
#endif
}
