/* main.c - uart pipe demo */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <misc/printk.h>
#include <string.h>
#include <zephyr.h>
#include <console/uart_pipe.h>

/***************************************************
 * Brief:
 *	 This example will send data back when uart
 *    receive 20(SEND_DATA_LEN) byte data.
 *
 * Communication: uart0
 *    #define CONFIG_UART_PIPE_ON_DEV_NAME "UART_0"
 *
 * Debug: uart1
 *    #define CONFIG_UART_CONSOLE_ON_DEV_NAME "UART_1"
 *****************************************************/

#define STACKSIZE 2048
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread cmd_thread;

#define DATA_QUEUED  10
#define ACT_DATA_MTU 256

#define SEND_DATA_LEN 20

struct uart_data_buf {
	u32_t _reserved;
	u16_t data_len;
	union {
		u8_t data[ACT_DATA_MTU];
	};
};

static struct uart_data_buf data_buf[DATA_QUEUED];

static K_FIFO_DEFINE(datas_queue);
static K_FIFO_DEFINE(avail_queue);

static void data_handler(void *p1, void *p2, void *p3)
{
	int i;

	while (1) {
		struct uart_data_buf *data;

		data = k_fifo_get(&datas_queue, K_FOREVER);

		/* TODO: handle data */
		printk("received data: ");
		for (i = 0; i < data->data_len; i++) {
			printk("%02x ", data->data[i]);
		}
		printk("\n");

		/* send back */
		uart_pipe_send(data->data, data->data_len);

		memset(data, 0, sizeof(*data));
		k_fifo_put(&avail_queue, data);
	}
}

/* Attention: recv_cb is called form uart interruption */
static u8_t *recv_cb(u8_t *buf, size_t *off)
{
	struct uart_data_buf *new_buf;
	struct uart_data_buf *cur_buf;

	if (*off < SEND_DATA_LEN) {
		return buf;
	}

	new_buf =  k_fifo_get(&avail_queue, K_NO_WAIT);
	if (!new_buf) {
		*off = 0;
		return buf;
	}

	cur_buf = CONTAINER_OF(buf, struct uart_data_buf, data);
	cur_buf->data_len = *off;
	k_fifo_put(&datas_queue, cur_buf);

	*off = 0;
	return new_buf->data;
}

void app_main(void)
{
	int i;
	struct uart_data_buf *buf;
	u32_t val;

	printk("uart pipe test\n");

	k_fifo_init(&avail_queue);
	k_fifo_init(&datas_queue);

	for (i = 0; i < DATA_QUEUED; i++) {
		k_fifo_put(&avail_queue, &data_buf[i]);
	}

	k_thread_create(&cmd_thread, stack, STACKSIZE, data_handler,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	buf = k_fifo_get(&avail_queue, K_NO_WAIT);

	uart_pipe_register(buf->data, ACT_DATA_MTU, recv_cb);
}

