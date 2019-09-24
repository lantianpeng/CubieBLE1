/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <soc.h>

void app_main(void)
{
	static int time;

	while (1) {
		printk("Hello World 0!\n");
		k_sleep(1000);
		time++;
		if (time == 30)
			sys_pm_reboot(REBOOT_TYPE_GOTO_DTM);
	}
}
