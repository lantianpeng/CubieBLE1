/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <init.h>
#include <soc.h>

#define CONFIG_NUM_IRQS 32

struct _isr_table_entry _sw_isr_table[CONFIG_NUM_IRQS];

int main(void)
{
	debug_uart_put_char('M');

	_PrepC(&_sw_isr_table, app_main, _sys_device_do_config_level, device_get_binding, kernel_config_init);

	return 0;
}
