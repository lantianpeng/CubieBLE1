/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include "errno.h"
#include <misc/printk.h>
#include <console/uart_pipe.h>
#include "act_data.h"
#include "at_cmd.h"
#include "app_ble.h"
#include "nvram_config.h"
#include "misc.h"
#include <uart.h>
#include "uart_pipe.h"

#define STACKSIZE 1024
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);

static struct k_thread cmd_thread;
static struct k_timer send_partial_timer;

#define DATA_QUEUED 60
struct act_data_buf {
    u32_t _reserved;
	u16_t data_len;
    union {
        u8_t data[ACT_DATA_MTU];
    };
};

static struct act_data_buf data_buf[DATA_QUEUED];

static struct k_fifo datas_queue;
static struct k_fifo avail_queue;

static u8_t *recv_buf;
static size_t recv_buf_len;
static uart_pipe_recv_cb rx_recv_cb;
static size_t recv_off;

void act_data_rx_init(u8_t *buf, size_t len, uart_pipe_recv_cb cb)
{
	recv_buf = buf;
	recv_buf_len = len;
	rx_recv_cb = cb;
	recv_off = 0;
}

void act_data_rx_abort(void)
{
	if (rx_recv_cb) {
		recv_buf = rx_recv_cb(recv_buf, &recv_off);	
	}
}

static void act_data_rx_isr(struct device *dev)
{
	uart_irq_update(dev);
	if (uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			/* As per the API, the interrupt may be an edge so keep
			 * reading from the FIFO until it's empty.
			 */
			for (;;) {
				int avail = recv_buf_len - recv_off;
				int got;
				got = uart_fifo_read(dev, recv_buf + recv_off, avail);
				if (got <= 0) {
					break;
				}
				
				/*
				 * Call application callback with received data. Application
				 * may provide new buffer or alter data offset.
				 */
				recv_off += got;
				recv_buf = rx_recv_cb(recv_buf, &recv_off);
			};
		}
	}
}

int act_data_send_data(const u8_t *data, int len)
{
	uart_pipe_send(data, len);
	return 0;
}

static u8_t at_disconn_match_cnt;
u8_t at_disconn_match(u8_t *data, u16_t len)
{
	int i = 0;
	u8_t index = at_disconn_match_cnt;
	char at_disconn_cmd[] = "AT+DISCONNECT\r\n";

	for (i = 0; i < len; i++) {
		if (data[i] == at_disconn_cmd[index++]) {
			at_disconn_match_cnt++;
			if (at_disconn_match_cnt == strlen(at_disconn_cmd)) {
				at_disconn_match_cnt = 0;
				send_async_msg(MSG_DISCONNECT);
				return len;   // drop the reset data
			}
		} else {
			/* only match AT+DISCON, disconnect anyway */
			if(at_disconn_match_cnt >= strlen("AT+DISCON")) {
				send_async_msg(MSG_DISCONNECT);
				at_disconn_match_cnt = 0;
				return len;   
			}
			at_disconn_match_cnt = 0;
			return 0;
		}	
	}

	/* match all, but not completed */
	return len;
}

u8_t at_ota_match(u8_t *data, u16_t len) 
{
	int i = 0;
	u8_t match_len;
	char at_ota_cmd[] = "AT+OTA\r\n";

	if (len < 7) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		if (data[i] == at_ota_cmd[i]) {
			match_len++;
			if (match_len == strlen(at_ota_cmd)) {
				extern void at_cmd_ota(void);
				at_cmd_ota();
				return len;
			}
		} else {
			return 0;
		}	
	}

	return 0;
}

bool at_name_match(u8_t *data, u16_t len)
{
	int i = 0;
	u8_t index = 0;
#if !CONFIG_AT_CMD_NO_EQUAL_SIGN
	char at_name_cmd[] = "AT+NAME=";
#else
	char at_name_cmd[] = "AT+NAME";
#endif
	
	do {
		if (data[i++] == at_name_cmd[index++]) {
			if (index == strlen(at_name_cmd))
				return true;
		} else {
			return false;
		}
	} while(i < len);
	
	return false;
}

static char ad_name[31];
static bool at_name_getting = false;
static bool at_name_get_r = false;

void at_name_set(char *name)
{
	u8_t name_len = strlen(name);
	u8_t cp_len = (name_len > BLE_NAME_MAX_LEN -1) ? (BLE_NAME_MAX_LEN-1) : name_len;

	memset(ble_param.adv_name, 0 , sizeof(ble_param.adv_name));
	memcpy(ble_param.adv_name, name, cp_len);
	ble_param.adv_name[BLE_NAME_MAX_LEN -1] = '\0';
	
	nvram_config_set_factory("BT_NAME", ble_param.adv_name, cp_len);
}

int at_name_get(u8_t *data, u16_t len)
{
	int i;
	u8_t name_len = strlen(ad_name);
	u8_t cp_len = sizeof(ad_name) - name_len - 1;
	
	for (i = 0; i < len; i++) {
		if (at_name_get_r) {
			if (data[i] == '\n') {
				break;
			}		
		} else {
			if (data[i] == '\r' || data[i] == '\n') {
				break;
			}		
		}
	}

	/* not found \r or \n */
	if (i == len) {
		if (at_name_get_r) {
			/* found \r before, but there is no \n, receive finish */
			at_name_getting = false;
			at_name_set(ad_name);
			return 0;
		} else {
			/* when \r or \n is not found, copy all data, i=len, i start from 0*/
			cp_len = (cp_len > i) ? i: cp_len;
			memcpy(&ad_name[name_len], data, cp_len);
			ad_name[30] = '\0';
			if (cp_len == 0) {
				/* found AT+NAME, but no \r or \n found in next data, drop the data, finish receive */
				at_name_getting = false;
				at_name_set(ad_name);
			}
			return i;
		}
	} else {// found \r or \n
		/* when found \n or \n, copy i bytes data, i <= len - 2, i start from 0 */
		cp_len = (cp_len > i) ? i: cp_len;
		memcpy(&ad_name[name_len], data, cp_len);
		ad_name[30] = '\0';
	
		if (data[i] == '\r') {
			at_name_get_r = true;
			if (i < len - 1) {
				if (data[i+1] == '\n') {
					i++;
					at_name_getting = false;
				} else {
					/* \r is not the last byte, but there is no \n, receive finish */
					at_name_getting = false;
				}
				at_name_set(ad_name);
			} else {
				/* \r is the last byte, maybe receive not finish */
			}
		} else {
			/* found \n£¬finish */
			at_name_getting = false;
			at_name_set(ad_name);
		}

		return i + 1;		
	}	
}

extern struct app_cb_t app_cb;
static void data_handler(void *p1, void *p2, void *p3)
{
	while (1) {
		struct act_data_buf *data;
		data = k_fifo_get(&datas_queue, K_FOREVER);
		if (data) {
			if (app_cb.pconn) {
				u8_t data_start = 0;	
				/* slave need receive AT+NAME command in data transport mode */
				if (ble_param.role == BLE_ROLE_SLAVE) {
					if (!at_name_getting) {
	#if !CONFIG_AT_CMD_NO_EQUAL_SIGN
						u8_t at_cmd_name_len = 8;
	#else
						u8_t at_cmd_name_len = 7;
	#endif
						/* AT+NAME command is not found */
						if (data->data_len >= at_cmd_name_len) {
							if (at_name_match(data->data, data->data_len)) {
								at_name_getting = true;
								at_name_get_r = false;
								memset(ad_name, 0, sizeof(ad_name));
								/* get at name */
								data_start = at_name_get(&data->data[at_cmd_name_len], data->data_len - at_cmd_name_len) + at_cmd_name_len;
							}			
						}
					} else {
						/* AT+NAME command found, get name before AT+NAME end */
						data_start = at_name_get(data->data, data->data_len);
					}
				} else {
					data_start = at_ota_match(data->data, data->data_len);
					if (!data_start) {
						/* get at disconnect */				
						data_start = at_disconn_match(data->data, data->data_len);				
					}
				}

				if (data_start < data->data_len ) {
					/* handle data */
					int err = ble_send_data(&data->data[data_start], data->data_len - data_start);
					if (err < 0) {
						printk("ble_send_data err %d\n", err);
						k_sleep(100); /* when noitfy err, yeild cpu */
					}
				}
			}

			memset(data, 0, sizeof(*data));
			k_fifo_put(&avail_queue, data);				
		}
    }
}

volatile u8_t send_partial_timer_started = 0;
volatile u8_t rx_abort = 0;

void send_partial_handler(struct k_timer *timer)
{
	rx_abort = 1;
	act_data_rx_abort();
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
	
	cur_buf =  CONTAINER_OF(buf, struct act_data_buf, data);
	cur_buf->data_len = *off;
	k_fifo_put(&datas_queue, cur_buf);

	*off = 0;
	return new_buf->data;
}

void enter_into_data_mode(void)
{
	/* 1. set isr callback for data mode */
	uart_pipe_irq_callback_set(act_data_rx_isr);
	
	/* 2. clear data */
	recv_off = 0;
}

void act_data_init(void)
{
	int i;
	struct act_data_buf *buf;

	k_fifo_init(&avail_queue);
	k_fifo_init(&datas_queue);

	for (i = 0; i < DATA_QUEUED; i++) {
		k_fifo_put(&avail_queue, &data_buf[i]);
	}

	buf = k_fifo_get(&avail_queue, K_NO_WAIT);

	act_data_rx_init(buf->data, ACT_DATA_MTU, recv_cb);

	k_timer_init(&send_partial_timer, send_partial_handler, NULL);

	k_thread_create(&cmd_thread, stack, STACKSIZE, data_handler,
			NULL, NULL, NULL, 7, 0, K_NO_WAIT);
}
