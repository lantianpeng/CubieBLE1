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
#include <timer.h>
 
void timer_irq_callback(struct device *dev)
{
	printk("timer irq callback\n");
}

void app_main(void)
{
	struct device *timer;

	printk("timer test\n");

	timer = device_get_binding(CONFIG_TIMER1_ACTS_DRV_NAME);
	if (!timer) {
		printk("Cannot find %s!\n", CONFIG_TIMER1_ACTS_DRV_NAME);
		return;
	}

	timer_set_callback(timer, timer_irq_callback);
	timer_start(timer, 1000, 1000);
	
	k_sleep(5000);
	timer_stop(timer);
}
