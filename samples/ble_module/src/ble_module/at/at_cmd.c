/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AT CMD handler implementation of at_cmd.h API
 */
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <misc/printk.h>
#include <misc/util.h>
#include "errno.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <conn_internal.h>
#include <bluetooth/storage.h>
#include "soc.h"
#include "at_cmd.h"
#include "nvram_config.h"
#include "led_manager.h"
#include "app_ble.h"
#include "misc.h"
#include <uart.h>
#include <uart_pipe.h>
#include <console/console.h>

#define BT_LE_CONN_PARAM_MODULE \
	BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, \
	BT_GAP_INIT_CONN_INT_MAX, 0, 100)

#define ARGC_MAX 10
#define AT_CMD_MAX_LINE_LEN 256

struct at_cmd_input {
	/** FIFO uses first 4 bytes itself, reserve space */
	int _unused;
	/** Buffer where the input line is recorded */
	char line[AT_CMD_MAX_LINE_LEN];
};

#define STACKSIZE 1024
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread at_cmd_thread;

#define MAX_CMD_QUEUED 3
static struct at_cmd_input at_cmd_buf[MAX_CMD_QUEUED];

static struct k_fifo avail_queue;
static struct k_fifo cmds_queue;

extern struct app_cb_t app_cb;
struct k_timer at_scan_timer;

static u8_t input_cur;
static struct console_input *input_cmd;

void at_cmd_rx_init(struct k_fifo *avail, struct k_fifo *lines)
{
	input_cur = 0;
	input_cmd = NULL;
}

void at_cmd_port_print(const char *str)
{
	int len = strlen(str);
	
	uart_pipe_send(str, len);
}

static atomic_t cmd_queue_ref;
void at_cmd_rx_isr(struct device *dev)
{
	while (uart_irq_update(dev) &&
	    uart_irq_is_pending(dev)) {
		u8_t byte;
		int rx;	 
		if (!uart_irq_rx_ready(dev)) {
			continue;
		}

		/* Character(s) have been received */
		rx = uart_fifo_read(dev, &byte, 1);
		if (rx < 0) {
			/* Overrun issue. Stop the UART */
			uart_irq_rx_disable(dev);
			return;
		}

		if (!input_cmd) {
			input_cmd = k_fifo_get(&avail_queue, K_NO_WAIT);
			if (!input_cmd) {
				return;
			}
			atomic_inc(&cmd_queue_ref);
		}

		/* Handle special control characters */
		if (!isprint(byte)) {
			switch (byte) {
			case '\n':
				input_cmd->line[input_cur] = '\0';
				if (input_cur-1 >= 0 && input_cmd->line[input_cur-1] == '\r') {
					input_cmd->line[input_cur-1] = '\0';	
				}
				input_cur = 0;
				k_fifo_put(&cmds_queue, input_cmd);
				atomic_dec(&cmd_queue_ref);
				input_cmd = NULL;
				break;
			default:
				break;
			}

			continue;
		}

		/* Ignore characters if there's no more buffer space */
		if (input_cur < sizeof(input_cmd->line) - 1) {
			input_cmd->line[input_cur++] = byte;
		}
	}
}

void at_cmd_response_ok(void)
{
	at_cmd_port_print("OK\r\n");
}

void at_cmd_response_error(void)
{
	at_cmd_port_print("ERROR\r\n");
}

void at_cmd_response(const char *str)
{
	at_cmd_port_print(str);
}

int at_cmd_AT(int argc, char *argv[])
{
	at_cmd_response_ok();
	return 0;
}

void ble_state_update(u8_t state) {
	ble_param.state = state;
}

void switch_role(u8_t new_role)
{	
	if (new_role == BLE_ROLE_SLAVE) {
		if (ble_param.state == INQUIRING
#if CONFIG_AUTO_RECONNECT		
		|| ble_param.state == RECONNTING
#endif
		) {
			stop_scan();
		}
		
		start_adv();
	} else {
		if (ble_param.state == ADVERTISING) {
			bt_le_adv_stop();
		}
	}
	
	if (ble_param.state == CONNECTING) {
		disconnect();		
	}
	
	ble_param.role = new_role;
}

int at_cmd_role_set(char *p_para)
{
	u8_t role = atoi(p_para);
	
	if (!is_decimal_string(p_para) || 
			(role != BLE_ROLE_SLAVE && role != BLE_ROLE_MASTER)) {
		at_cmd_response_error();
		return 0;	
	}
		
	if (role != ble_param.role) {
		switch_role(role);
		ble_state_update(READY);
	}	

	at_cmd_response("+ROLE=");
	at_cmd_response(p_para);
	at_cmd_response("\r\nOK\r\n");	
	
	nvram_config_set_factory("BT_ROLE", &role, 1);

	return 0;
}

int at_cmd_role_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "+ROLE=%d\r\n", ble_param.role);
	at_cmd_response(ret);
	return 0;
}

void at_scan_timer_handler(struct k_timer *dummy)
{
	send_async_msg(MSG_SCAN_TIME_OUT);
}

void at_cmd_start_scan(u8_t time_out)
{
	int err;

	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_ACTIVE,  
		.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_INTERVAL
	};
	
	err = start_scan(&scan_param);
	if (err && err != -EALREADY) {
		printk("Bluetooth set active scan failed (err %d)\n", err);
		at_cmd_response_error();
		return;
	} else {
		printk("Bluetooth active scan enabled\n");
	}
	
	ble_state_update(INQUIRING);		

	if (time_out) {
		k_timer_start(&at_scan_timer, K_SECONDS(time_out), 0);
	}
}

int at_cmd_inq(void)
{
	if (BLE_ROLE_MASTER != ble_param.role) {
		at_cmd_response_error();
		return 0;
	}
	
	at_cmd_response_ok();
	
	memset(ble_param.dev_list, 0, sizeof(ble_param.dev_list));
	ble_param.dev_cnt = 0;
	
	at_cmd_start_scan(1);
	
	return 0;
}

struct bt_conn *conn_tmp = NULL;
int at_conn_create(bt_addr_le_t * addr)
{
	struct bt_conn *conn = bt_le_conn_lookup_addr(addr);
	if (conn) {
		if (conn->state == BT_CONN_CONNECT || conn->state == BT_CONN_CONNECTED) {
			return 0;
		}
	} else {
		if (conn_tmp) {
			if (conn_tmp->state == BT_CONN_CONNECT_SCAN) {
				bt_conn_unref(conn_tmp);
			} else { 
				bt_conn_disconnect(conn_tmp, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				return -1;		
			}
		}
	}

	conn_tmp = bt_conn_create_le(addr, BT_LE_CONN_PARAM_MODULE);
	if (!conn_tmp) {
		printk("Connection failed\n");
		at_cmd_response_error();
		return -1;
	} else {
		printk("Connection pending\n");
		bt_conn_unref(conn_tmp);
		ble_state_update(CONNECTING);
		return 0;		
	}
}

#if CONFIG_QUICK_CONNECT
int at_cona_create(bt_addr_le_t * addr)
{
	struct bt_conn *conn = bt_le_conn_lookup_addr(addr);
	if (conn) {
		/* the same address or first cona */
		if (conn->state == BT_CONN_CONNECT || conn->state == BT_CONN_CONNECTED) {
			return 0;
		}
	} else {
		/* new address */
		if (conn_tmp) {
			if (conn_tmp->state == BT_CONN_CONNECT_SCAN) {
				 /* last connection still passive scan */
				bt_conn_unref(conn_tmp);			
			} else { 
				/* almost connected, just disconnect last connection */
				bt_conn_disconnect(conn_tmp, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				return -1;		
			}
		}
	}

	conn_tmp = bt_conn_create_le_new(addr, BT_LE_CONN_PARAM_MODULE);
	if (!conn_tmp) {
		printk("Connection failed\n");
		at_cmd_response_error();
		return -1;
	} else {
		printk("Connection pending\n");
		bt_conn_unref(conn_tmp);
		ble_state_update(CONNECTING);
		return 0;		
	}
}
#endif

int at_cmd_conn_set(char *p_para)
{
	u8_t dev_index = atoi(p_para) - 1;
	bt_addr_le_t *addr;
	
	if (ble_param.role != BLE_ROLE_MASTER || !ble_param.dev_cnt || dev_index > ble_param.dev_cnt) {
		at_cmd_response_error();
		return 0;	
	}

	if (ble_param.state == INQUIRING
#if CONFIG_AUTO_RECONNECT
		|| ble_param.state == RECONNTING
#endif
	) {
		k_timer_stop(&at_scan_timer);
		stop_scan();
	}
	
	addr = (bt_addr_le_t *) &ble_param.dev_list[dev_index].addr;
	if (at_conn_create(addr)) {
		at_cmd_response_error();
	}
	
	return 0;
}

int at_cmd_cona_set(char *p_para)
{
	char bt_addr_str[18] = {0};
	char *p_addr = p_para + 2;
	
	bt_addr_le_t addr;
	u8_t dev_index = atoi(p_para) - 1;
	if (ble_param.role != BLE_ROLE_MASTER) {
		at_cmd_response_error();
		return 0;	
	}

	if ((p_para[0] != '0' || (p_para[1] != 'x' && p_para[1] != 'X')) || 
			strlen(p_addr) != 12 || !is_hex_string(p_addr)) {
		at_cmd_response_error();
		return 0;		
	}

	at_str_to_le_addr(bt_addr_str, sizeof(bt_addr_str), &addr, p_addr);

	if (!bt_addr_le_cmp(&addr, BT_ADDR_LE_ANY)) {
		at_cmd_response_error();
		return 0;	
	}

	if (ble_param.state == INQUIRING
#if CONFIG_AUTO_RECONNECT
		|| ble_param.state == RECONNTING
#endif
	) {
		k_timer_stop(&at_scan_timer);
		stop_scan();
	}

#if CONFIG_QUICK_CONNECT
	if (at_cona_create(&addr)) {
#else
	if (at_conn_create(&addr)) {
#endif
		at_cmd_response_error();
	}

	return 0;
}

int at_cmd_auto_conn_set(char *p_para)
{
	u8_t auto_conn = atoi(p_para);
	
	if ((ble_param.role != BLE_ROLE_MASTER) || !is_decimal_string(p_para)) {
		at_cmd_response_error();
		return 0;	
	}
	
	switch (auto_conn) {
	case 0:
		ble_param.auto_conn = false;
#if CONFIG_AUTO_RECONNECT	
		if (ble_param.state == RECONNTING) {
			stop_scan();
		}
#endif
		break;
	case 1:
		ble_param.auto_conn = true;
		break;
	default:
		at_cmd_response_error();
		return 0;
	}

	at_cmd_response("+AUTOCONN=");
	at_cmd_response(p_para);
	at_cmd_response("\r\nOK\r\n");
	
	nvram_config_set_factory("BT_AUTOCONN", &auto_conn, 1);

	return 0;
}

int at_cmd_auto_conn_get(void)
{
	char ret[30];
	
	if (ble_param.role != BLE_ROLE_MASTER) {
		at_cmd_response_error();
		return 0;	
	}

	snprintf_rom(ret, 30, "+AUTOCONN=%d\r\n", ble_param.auto_conn);
	at_cmd_response(ret);
	return 0;
}

int at_cmd_clear(void)
{
	at_cmd_response_ok();

#if CONFIG_AUTO_RECONNECT		
	if (ble_param.state == RECONNTING) {
		stop_scan();
	}	
#endif

	extern int bt_storage_clear(const bt_addr_le_t *addr);	
	bt_storage_clear(NULL);
	
	bt_addr_le_copy(&ble_param.targed_addr, BT_ADDR_LE_ANY);
	
	nvram_config_set_factory("BT_TGT_ADDR", BLE_TARGET_ADDR_DFLT, strlen(BLE_TARGET_ADDR_DFLT));

	return 0;
}

int at_cmd_reset(void)
{
	at_cmd_response_ok();
	k_sleep(100);
	sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	return 0;
}

int at_cmd_default(void)
{
	/* default values */
	u8_t role = BLE_ROLE_DFLT;
	u8_t auto_conn = BLE_AUTOCONN_DFLT;
	char adv_name[] = BLE_ADV_NAME_DFLT;

	/* bt address */
	nvram_config_set_factory("BT_ADDR", BLE_DEVICE_ADDR_DFLT, strlen(BLE_DEVICE_ADDR_DFLT));

	/* role */
	nvram_config_set_factory("BT_ROLE", &role, 1);

	/* autoconn */
	nvram_config_set_factory("BT_AUTOCONN", &auto_conn, 1);

	/* name */
	nvram_config_set_factory("BT_NAME", adv_name, strlen(adv_name));

	/* target address */
	nvram_config_set_factory("BT_TGT_ADDR", BLE_TARGET_ADDR_DFLT, strlen(BLE_TARGET_ADDR_DFLT));

	/* keys */
	extern int bt_storage_clear(const bt_addr_le_t *addr);	
	bt_storage_clear(NULL);
	
	/* target address */
	bt_addr_le_copy(&ble_param.targed_addr, BT_ADDR_LE_ANY);

#if CONFIG_AUTO_RECONNECT
	/* stop scan */
	if (ble_param.state == RECONNTING) {
		stop_scan();
	}
#endif

	at_cmd_response_ok();
	
	return 0;
}

int at_cmd_adv_name_set(char *p_para)
{
	int len = strlen(p_para);
	int cp_len = (len > BLE_NAME_MAX_LEN -1) ? (BLE_NAME_MAX_LEN-1) : len;
	
	if (!len) {
		at_cmd_response_error();
		return 0;
	}
	
#ifdef CONFIG_DEVICE_NAME_PREFIX
	if (strncmp(p_para, CONFIG_DEVICE_NAME_PREFIX, strlen(CONFIG_DEVICE_NAME_PREFIX))) {
		at_cmd_response_error();
		return 0;
	}
#endif

	memset(ble_param.adv_name, 0 , sizeof(ble_param.adv_name));
	memcpy(ble_param.adv_name, p_para, cp_len);
	ble_param.adv_name[BLE_NAME_MAX_LEN -1] = '\0';
	ad_data_set(BT_DATA_NAME_SHORTENED, (u8_t *)ble_param.adv_name,
			cp_len);
	
	at_cmd_response("+NAME=");
	at_cmd_response(ble_param.adv_name);
	at_cmd_response("\r\nOK\r\n");

	if (ble_param.role == BLE_ROLE_SLAVE) {
		start_adv();
	}

	nvram_config_set_factory("BT_NAME", p_para, cp_len);

	return 0;
}

int at_cmd_adv_name_get(void)
{
	char ret[30];
	snprintf_rom(ret, 30, "+NAME=%s\r\n", ble_param.adv_name);
	at_cmd_response(ret);
	
	return 0;
}

int at_cmd_laddr_set(char *p_para)
{	
	bt_addr_le_t addr;
	char bt_addr_str[18] = {0};
	char *p_addr = p_para + 2;
		
	if ((p_para[0] != '0' || (p_para[1] != 'x' && p_para[1] != 'X')) || 
			strlen(p_addr) != 12 || !is_hex_string(p_addr)) {
		at_cmd_response_error();
		return 0;
	}

	at_str_to_le_addr(bt_addr_str, sizeof(bt_addr_str), &addr, p_addr);
	nvram_config_set_factory("BT_ADDR", bt_addr_str, strlen(bt_addr_str));
	
	/* update to ble_param.dev_addr */
	bt_addr_le_copy(&ble_param.dev_addr, &addr);

	at_cmd_response("+LADDR=");
	at_cmd_response(p_para);
	at_cmd_response("\r\nOK\r\n");
	return 0;
}

int at_cmd_laddr_get(void)
{
	char ret[30];
	char bt_addr_str[18] = {0};
	
	at_addr_le_to_str(&ble_param.dev_addr, bt_addr_str, sizeof(bt_addr_str));

	snprintf_rom(ret, 30, "+LADDR=0x%s\r\n", bt_addr_str);
	at_cmd_response(ret);

	return 0;
}

int at_cmd_disconnect(void)
{	
	send_async_msg(MSG_DISCONNECT);
	at_cmd_response_ok();
	return 0;
}

int at_cmd_ota(void)
{	
	ble_param.role = BLE_ROLE_SLAVE;
	send_async_msg(MSG_DISCONNECT);
	at_cmd_response_ok();
	return 0;
}

struct at_cmd at_cmd_commands[] = {
	{ "ROLE",        at_cmd_role_set,      NULL,  at_cmd_role_get},
	{ "INQ",         NULL,                 NULL,  at_cmd_inq},
	{ "CONN",        at_cmd_conn_set,      NULL,  NULL},
	{ "CONA",        at_cmd_cona_set,      NULL,  NULL},
	{ "AUTOCONN",    at_cmd_auto_conn_set, NULL,  at_cmd_auto_conn_get},
	{ "CLEAR",       NULL,                 NULL,  at_cmd_clear},
	{ "RESET",       NULL,                 NULL,  at_cmd_reset},
	{ "DEFAULT",     NULL,                 NULL,  at_cmd_default},
	{ "NAME",        at_cmd_adv_name_set,  NULL,  at_cmd_adv_name_get},
	{ "LADDR",       at_cmd_laddr_set,     NULL,  at_cmd_laddr_get},
	{ "DISCONNECT",  NULL,                 NULL,  at_cmd_disconnect},
	{ "OTA",         NULL,                 NULL,  at_cmd_ota},
};

#if !CONFIG_AT_CMD_NO_EQUAL_SIGN
static int line2argv(char *str, char *argv[], size_t size)
{
	int argc = 0;

	if (!strlen(str)) {
		return 0;
	}

	while (*str && *str == ' ') {
		str++;
	}

	if (!*str) {
		return 0;
	}

	argv[argc++] = str;
	
	while ((str = strchr(str, '+'))) {
		*str++ = '\0';

		while (*str && *str == ' ') {
			str++;
		}

		if (!*str) {
			return -EINVAL;
		}

		argv[argc++] = str;

		if (argc == size) {
			printk("Too many parameters (max %zu)\n", size - 1);
			return -EINVAL;
		}
		while ((str = strchr(str, '='))) {			
			*str++ = '\0';

			while (*str && *str == ' ') {
				str++;
			}

			if (!*str) {
				return -EINVAL;
			}

			argv[argc++] = str;

			if (argc == size) {
				printk("Too many parameters (max %zu)\n", size - 1);
				return -EINVAL;
			}
		}
	}

	/* keep it POSIX style where argv[argc] is required to be NULL */
	argv[argc] = NULL;

	return argc;
}
#else
struct at_cmd_struct {
	char *name;
	char *cmd;
	u8_t cmd_len;
	u8_t param_len;
} at_cmds_hj[] = {
     /* name */       /* cmd */ /* cmd_len */ /* param_len */
	{ "AT",            "",            2,           0,   },
	{ "AT+ROLE",       "ROLE",        7,           1    },
	{ "AT+INQ",        "INQ",         6,           0,   },
	{ "AT+CONN",       "CONN",        7,           1,   },
	{ "AT+CONA",       "CONA",        7,           14,  },
	{ "AT+AUTOCONN",   "AUTOCONN",    11,          1,   },
	{ "AT+CLEAR",      "CLEAR",       8,           0,   },
	{ "AT+RESET",      "RESET",       8,           0,   },
	{ "AT+DEFAULT",    "DEFAULT",     10,          0,   },
	{ "AT+NAME",       "NAME",        7,           0xff,},
	{ "AT+LADDR",      "LADDR",       8,           14,  },
	{ "AT+DISCONNECT", "DISCONNECT", 13,           0,   },
	{ "AT+OTA",        "OTA",         5,           0,   },
};

#define AT_CMD_SIZE   ARRAY_SIZE(at_cmds_hj)

static int line2argv(char *str, char *argv[], size_t size)
{
	int i = 0;
	char *str_cmd = NULL;
	int argc = 0;

	printk("line: %s\n", str);

	if (!strlen(str)) {
		return 0;
	}

	while (*str && *str == ' ') {
		str++;
	}

	if (!*str) {
		return 0;
	}

	if (!strcmp(str, at_cmds_hj[i].name)) {
		argv[argc++] = "AT";
		return argc;
	}

	for (i = 1; i < AT_CMD_SIZE; i++) {
		str_cmd = strstr(str, at_cmds_hj[i].name);
		if (str_cmd) {
			break;
		}
	}
	
	if (i == AT_CMD_SIZE) {
		return -EINVAL;
	}

	argv[argc++] = "AT";
	argv[argc++] = at_cmds_hj[i].cmd;
	if (at_cmds_hj[i].param_len) {
		if (strlen(str_cmd) > at_cmds_hj[i].cmd_len) {
			argv[argc++] = &str_cmd[at_cmds_hj[i].cmd_len];
		}
	} else {
		if (strcmp(str_cmd, at_cmds_hj[i].name)) {
			return -EINVAL;
		}
	}

	/* keep it POSIX style where argv[argc] is required to be NULL */
	argv[argc] = NULL;

	return argc;
}

#endif

static void exec_cb(int *argc, char *argv[])
{
	const char *first_string = argv[0];
	const char *command = argv[1];
	int i;
	
	if (!first_string || first_string[0] == '\0') {
		at_cmd_response_error();
		return;
	}

	if (strcmp(first_string, "AT")) {
		at_cmd_response_error();
		return;
	}

	if (*argc == 1) {
		at_cmd_response_ok();
		return;
	}

	/* Execute callback with arguments */
	/* Ajust parameters to point to the actual command */
	for (i = 0; i < ARRAY_SIZE(at_cmd_commands); i++) {
		if (!strcmp(command, at_cmd_commands[i].at_cmd_name)) {
			if (*argc == 2) {
				if (at_cmd_commands[i].exec_cb) {
					at_cmd_commands[i].exec_cb();
				} else {
					goto out;
				}
			} else if (*argc == 3) {
				const char *param = argv[2];
				if (at_cmd_commands[i].set_cb) {
					at_cmd_commands[i].set_cb(param);					
				} else {
					goto out;
				}
			}	
			return ;
		}
	}

out:
	at_cmd_response_error();
}

/* argc: 0: no exec, no error response, <0: no exec, response error, >0: exec */
int at_cmd_exec(char *line)
{
	char *argv[ARGC_MAX + 1];
	int argc;
	int err;
	
	argc = line2argv(line, argv, ARRAY_SIZE(argv));
	if (argc <= 0) {
		return argc;
	}

	err = argc;

	exec_cb(&argc, argv);

	return err;
}

void at_cmd_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct at_cmd_input *cmd;
		cmd = k_fifo_get(&cmds_queue, K_FOREVER);
		if (cmd) {
			if (!app_cb.pconn) {
				if (at_cmd_exec(cmd->line) < 0) {
					at_cmd_response_error();
				}
			}
			k_fifo_put(&avail_queue, cmd);
		}
	}
}

void enter_into_at_cmd_mode(void)
{
	/* 1. check and clear command queue reference */
	atomic_val_t queue_ref = atomic_clear(&cmd_queue_ref);
	if (queue_ref && input_cmd) {
		input_cmd->line[0] = '\0';
		k_fifo_put(&cmds_queue, input_cmd);
	}

	/* 2. reset command input */
	input_cur = 0;
	input_cmd = NULL;

	/* 3. set isr callback for cmd mode */
	uart_pipe_irq_callback_set(at_cmd_rx_isr);
}

void at_cmd_init(void)
{	
	int i;
	
	k_fifo_init(&avail_queue);
	k_fifo_init(&cmds_queue);

	for (i = 0; i < MAX_CMD_QUEUED; i++) {
		k_fifo_put(&avail_queue, &at_cmd_buf[i]);
	}

	at_cmd_rx_init(&avail_queue, &cmds_queue);

	k_timer_init(&at_scan_timer, at_scan_timer_handler, NULL);
	
	k_thread_create(&at_cmd_thread, stack, STACKSIZE, at_cmd_handler, NULL, NULL,
			NULL, 7, 0, K_NO_WAIT);
}
