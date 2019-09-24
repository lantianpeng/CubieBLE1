/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>

int uart_console_init(struct device *arg);
extern char *uart_console_on_dev_name;

int uart_console_init_new(struct device *arg)
{
	uart_console_on_dev_name = CONFIG_UART_CONSOLE_ON_DEV_NAME;
	return uart_console_init(arg);
}

SYS_INIT(uart_console_init_new,
			PRE_KERNEL_1,
			CONFIG_UART_CONSOLE_INIT_PRIORITY);
