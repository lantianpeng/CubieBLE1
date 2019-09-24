/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <soc.h>

void app_main(void)
{
	static int time;

	while (1) {
		printk("Hello World 1!\n");
		k_sleep(2000);
		time += 2;
		if (time == 30)
			sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	}
}
