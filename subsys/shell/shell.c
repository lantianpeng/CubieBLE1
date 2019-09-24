/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Console handler implementation of shell.h API
 */


#if CONFIG_KERNEL_SHELL

#include <zephyr.h>
#include <stdio.h>
#include <string.h>

#include <console/console.h>
#include <misc/printk.h>
#include <misc/util.h>

#include <console/uart_console.h>

#include <shell/shell.h>

#define ARGC_MAX 10
#define COMMAND_MAX_LEN 50
#define MODULE_NAME_MAX_LEN 20
/* additional chars are " >" (include '\0' )*/
#define PROMPT_SUFFIX 3
#define PROMPT_MAX_LEN (MODULE_NAME_MAX_LEN + PROMPT_SUFFIX)


static char keil_default_module_prompt[PROMPT_MAX_LEN];

#define STACKSIZE CONFIG_CONSOLE_SHELL_STACKSIZE
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread shell_thread;

#define MACRO_MAX_CMD_QUEUED CONFIG_CONSOLE_SHELL_MAX_CMD_QUEUED
static struct console_input keil_shell_buf[MACRO_MAX_CMD_QUEUED];

static struct k_fifo avail_queue;
static struct k_fifo cmds_queue;

__init_once_text void shell_rom_init(void)
{
	p_avail_queue = &avail_queue;
	p_cmds_queue = &cmds_queue;

	MAX_CMD_QUEUED = CONFIG_CONSOLE_SHELL_MAX_CMD_QUEUED;
	shell_buf = keil_shell_buf;

	default_module_prompt = keil_default_module_prompt;

	__shell_cmd_start = Image$$RW_IRAM_SHELL_CMD_START$$Base;
	__shell_cmd_end = Image$$RW_IRAM_SHELL_CMD_START$$Limit;
	NUM_OF_SHELL_ENTITIES = ((uint32_t)__shell_cmd_end - (uint32_t)__shell_cmd_start) / sizeof(struct shell_module);
}

__init_once_text void shell_init(const char *str)
{
	shell_rom_init();

	k_fifo_init(p_cmds_queue);
	k_fifo_init(p_avail_queue);

	line_queue_init();

	prompt = str ? str : "";

	k_thread_create(&shell_thread, stack, STACKSIZE, shell, NULL, NULL,
			NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Register serial console handler */
	uart_register_input(p_avail_queue, &cmds_queue, completion);

}
#endif
