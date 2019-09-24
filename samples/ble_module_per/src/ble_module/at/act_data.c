/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include "errno.h"
#include <zephyr.h>
#include <misc/printk.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/storage.h>
#include <console/uart_pipe.h>
#include "act_data.h"
#include "app_ble.h"
#include "app_gpio.h"

#define STACKSIZE 1024
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread cmd_thread;

static struct k_timer send_partial_timer;

#define DATA_QUEUED 60
#define DATA_QUEUE_HIGH_LEVEL 50
#define DATA_QUEUE_LOW_LEVEL 10

#ifdef MODULE_FLOW_PIN	
static u16_t avail_buf_count;
static u8_t  rx_flow_ctr_flag;
#endif

struct act_data_buf {
    u32_t _reserved;
	u16_t data_len;
    union {
        u8_t data[ACT_DATA_MTU];
    };
};

static struct act_data_buf data_buf[DATA_QUEUED];

static K_FIFO_DEFINE(datas_queue);
static K_FIFO_DEFINE(avail_queue);

int act_data_send_data(const u8_t *data, int len)
{
#ifdef MODULE_CTS_PIN
	gpio_output_wakeup_host(true);
#endif
	uart_pipe_send(data, len);
#ifdef MODULE_CTS_PIN
	gpio_output_wakeup_host(false);
#endif
	return 0;
}

static void data_handler(void *p1, void *p2, void *p3)
{
	while (1) {
		int err ;
		struct act_data_buf *data;
		data = k_fifo_get(&datas_queue, K_FOREVER);

		/* TODO
		 * verify if service is registered before calling handler
		 */
		extern int ble_send_data(u8_t *data, u16_t len);
		err = ble_send_data(data->data, data->data_len);
		if (err < 0) {
			printk("ble_send_data err %d\n", err);
			k_sleep(100); /* when noitfy err, yeild cpu */
		}

		memset(data, 0, sizeof(*data));
		k_fifo_put(&avail_queue, data);

#ifdef MODULE_FLOW_PIN		
		do {
			u32_t key = irq_lock();
			avail_buf_count++;
			irq_unlock(key);
		} while (0);
		
		/* disable flow control if avil_buf_count is bigger than high_level */
		if (rx_flow_ctr_flag && (avail_buf_count > DATA_QUEUE_HIGH_LEVEL)) {
			rx_flow_ctr_flag = false;
			gpio_output_flow_ctrl(false);
			//printk("close flowctrl %d\n", avail_buf_count);
		}
#endif
	}
}

volatile u8_t send_partial_timer_started = 0;
volatile u8_t rx_abort = 0;

extern void uart_pipe_rx_abort(void);
void send_partial_handler(struct k_timer *timer)
{
	rx_abort = 1;
	uart_pipe_rx_abort();
}

/* Attention: recv_cb is called form uart interruption */
static u8_t *recv_cb(u8_t *buf, size_t *off)
{
	struct act_data_buf *new_buf;
	struct act_data_buf *cur_buf;
	
	if ((*off > 0) && (!send_partial_timer_started)) {
		send_partial_timer_started = 1;
		k_timer_start(&send_partial_timer, K_MSEC(20), 0);
	}
	
	if ((*off < ACT_DATA_MTU) && (!rx_abort)) {
		return buf;
	}
	
	if (send_partial_timer_started && (!rx_abort)) {
		k_timer_stop(&send_partial_timer);
	}
	send_partial_timer_started = 0;
	rx_abort = 0;

	new_buf =  k_fifo_get(&avail_queue, K_NO_WAIT);
	if (!new_buf) {
		*off = 0;
		return buf;
	}
#ifdef MODULE_FLOW_PIN	
	avail_buf_count--;
	/* enable flow control after avail_buf_count lower than low level */
	if ((avail_buf_count < DATA_QUEUE_LOW_LEVEL) && (!rx_flow_ctr_flag)) {
		rx_flow_ctr_flag = true; 
		gpio_output_flow_ctrl(true);
	}
#endif
	cur_buf =  CONTAINER_OF(buf, struct act_data_buf, data);
	cur_buf->data_len = *off;
	k_fifo_put(&datas_queue, cur_buf);

	*off = 0;
	return new_buf->data;
}

static k_tid_t data_thread_id;

void enter_into_data_mode(void)
{
	int i;
	struct act_data_buf *buf;

	k_fifo_init(&avail_queue);
	k_fifo_init(&datas_queue);

	for (i = 0; i < DATA_QUEUED; i++) {
		k_fifo_put(&avail_queue, &data_buf[i]);
	}

#ifdef MODULE_FLOW_PIN
	avail_buf_count = DATA_QUEUED;
	rx_flow_ctr_flag = false;
	gpio_output_flow_ctrl(false);
#endif

	data_thread_id = k_thread_create(&cmd_thread, stack, STACKSIZE, data_handler,
			NULL, NULL, NULL, 7, 0, K_NO_WAIT);

	buf = k_fifo_get(&avail_queue, K_NO_WAIT);

	uart_pipe_register(buf->data, ACT_DATA_MTU, recv_cb);
}

extern void uart_pipe_unregister(u8_t *buf, size_t len, uart_pipe_recv_cb cb);
void exit_out_data_mode(void)
{
	uart_pipe_unregister(NULL, NULL, NULL);
	if (data_thread_id != 0) {
		k_thread_abort(data_thread_id);
		data_thread_id = 0;
	}
}

extern void uart_pipe_int(void);
void act_data_init(void)
{
		uart_pipe_int();
		k_timer_init(&send_partial_timer, send_partial_handler, NULL);
}


