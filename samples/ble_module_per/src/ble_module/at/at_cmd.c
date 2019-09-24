/*
 * Copyright (c) 2015 Intel Corporation
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

#include <misc/printk.h>
#include <misc/util.h>

#include "at_cmd.h"
#include "errno.h"
#include "app_cfg.h"
#include "app_gpio.h"

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
static struct k_fifo *p_at_avail_queue =&avail_queue;
static struct k_fifo *p_at_cmds_queue = &cmds_queue;

char print_buf[256];

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

bool is_hex_string(const char *str)
{
    int i, len = strlen(str);

    if (!len)
        return false;

    for (i = 0; i < len; i++) {
        if (!((str[i] >= '0' && str[i] <= '9') ||
			(str[i] >= 'a' && str[i] <= 'f') ||
			(str[i] >= 'A' && str[i] <= 'F')))
            return false;
    }

    return true;
}

bool is_decimal_string(const char *str)
{
    int i, len = strlen(str);

    if (!len)
        return false;

    for (i = 0; i < len; i++) {
        if (!(str[i] >= '0' && str[i] <= '9'))
            return false;
    }

    return true;
}

int char2hex(const char *c, u8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

struct at_cmd at_cmd_commands[] = {
	{ "STRS", NULL, NULL, at_cmd_reset},
	{ "ADST", at_cmd_adv_set, at_cmd_adv_get, NULL},
	{ "ADIT", at_cmd_adv_interval_set, at_cmd_adv_interval_get, NULL},
	{ "NAME", at_cmd_adv_name_set, NULL, NULL},
	{ "BAUD", at_cmd_adv_baudrate_set, at_cmd_adv_baudrate_get, NULL},
	{ "CNIT", at_cmd_conn_interval_set, at_cmd_conn_interval_get, NULL},
	{ "TXPW", at_cmd_tx_power_set, at_cmd_tx_power_get, NULL},
	{ "GMAC", NULL, NULL, at_cmd_bt_addr},
	{	"MAFD", at_cmd_adv_data_set, at_cmd_adv_data_get, NULL},
	{	"SERN", at_cmd_data_srv_uuid_set, at_cmd_data_srv_uuid_get, NULL},
	{	"SERD", at_cmd_drv_srv_uuid_set, at_cmd_drv_srv_uuid_get, NULL},
	{	"CHAT", at_cmd_data_tx_char_uuid_set, at_cmd_data_tx_char_uuid_get, NULL},
	{	"CHAR", at_cmd_data_rx_char_uuid_set, at_cmd_data_rx_char_uuid_get, NULL},
	{	"CHAD", at_cmd_drv_adc_char_uuid_set, at_cmd_drv_adc_char_uuid_get, NULL},
	{	"CHAP", at_cmd_drv_pwm_char_uuid_set, at_cmd_drv_pwm_char_uuid_get, NULL},
	{	"CODV", NULL, NULL, at_cmd_ver_info},
	{	"RSSI", at_cmd_rssi_set, at_cmd_rssi_get, NULL},
	{	"CRTS", at_cmd_rts_func_set, at_cmd_rts_func_get, NULL},
};


void at_cmd_line_queue_init(void)
{
	int i;

	for (i = 0; i < MAX_CMD_QUEUED; i++) {
		k_fifo_put(p_at_avail_queue, &at_cmd_buf[i]);
	}
}

static size_t line2argv(char *str, char *argv[], size_t size)
{
	size_t argc = 0;

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
			break;
		}

		argv[argc++] = str;

		if (argc == size) {
			printk("Too many parameters (max %zu)\n", size - 1);
			return 0;
		}
		
		while ((str = strchr(str, '-'))) {
			*str++ = '\0';

			while (*str && *str == ' ') {
				str++;
			}

			if (!*str) {
				break;
			}

			argv[argc++] = str;

			if (argc == size) {
				printk("Too many parameters (max %zu)\n", size - 1);
				return 0;
			}
		}
	}
#if 0	
	for (i = 0; i < argc; i++) {
		printk("argv[%d] %s\n", i, argv[i]);
	}
#endif
	/* keep it POSIX style where argv[argc] is required to be NULL */
	argv[argc] = NULL;

	return argc;
}

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
					at_cmd_commands[i].exec_cb();// exec,нч'?'╨м'-'
				} else {
					goto out;
				}
			} else if (*argc == 3) {
				const char *param = argv[2];
				if (param[0] == '?') { // '?', get
					if (at_cmd_commands[i].get_cb) {
						at_cmd_commands[i].get_cb();
					} else {
						goto out;
					}				

				} else {               // '-', set
					if (at_cmd_commands[i].set_cb) {
						at_cmd_commands[i].set_cb(param);					
					} else {
						goto out;
					}
				}
			}	
			return ;
		}
	}
	
out:
	at_cmd_response_error();
}

int at_cmd_exec(char *line)
{
	char *argv[ARGC_MAX + 1];
	int argc;
	int err;
	
	argc = line2argv(line, argv, ARRAY_SIZE(argv));
	if (!argc) {
		return -EINVAL;
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

		cmd = k_fifo_get(p_at_cmds_queue, K_FOREVER);

		at_cmd_exec(cmd->line);

		k_fifo_put(p_at_avail_queue, cmd);
	}
}

static k_tid_t at_cmd_thread_id;
extern void uart_at_register_input(struct k_fifo *avail, struct k_fifo *lines);
extern void uart_at_unregister_input(struct k_fifo *avail, struct k_fifo *lines,
			 u8_t (*completion)(char *str, u8_t len));
void enter_into_at_cmd_mode(void)
{
	k_fifo_init(p_at_cmds_queue);
	k_fifo_init(p_at_avail_queue);

	at_cmd_line_queue_init();
#ifdef MODULE_FLOW_PIN
	gpio_output_flow_ctrl(false);
#endif
	
	at_cmd_thread_id = k_thread_create(&at_cmd_thread, stack, STACKSIZE, at_cmd_handler, NULL, NULL,
			NULL, 7, 0, K_NO_WAIT);
	
	/* Register serial at cmd handler */
	uart_at_register_input(p_at_avail_queue, p_at_cmds_queue);
}

void exit_out_at_cmd_mode(void)
{
	uart_at_unregister_input(NULL, NULL, NULL);
	if (at_cmd_thread_id != 0) {
		k_thread_abort(at_cmd_thread_id);
		at_cmd_thread_id = 0;
	}
}

extern void uart_at_int(void);
void at_cmd_init(void)
{
		uart_at_int();
}

