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
#include <device.h>
#include <rtc.h>
#include "rtc_acts.h"

#define ALARM_INTERVAL 10
#define ALARM_COUNT 3

void rtc_interrupt_fn(struct device *rtc_dev)
{
	static u8_t alarm_count;

	if (alarm_count++ < ALARM_COUNT) {
		printk("Alarm\n");
		u32_t now = rtc_read(rtc_dev);

		rtc_set_alarm(rtc_dev, now + ALARM_INTERVAL);
	}
}

void app_main(void)
{
	struct device *rtc;
	struct rtc_config config;
	struct rtc_time tm;
	u32_t now;

	printk("\n\n=======Test RTC=======\n");
	rtc = device_get_binding(CONFIG_RTC_0_NAME);

	now = mktime(2018, 8, 21, 19, 39, 0);
	rtc_time_to_tm(now, &tm);
	printk("NOW: ");
	print_rtc_time(&tm);

	config.init_val = now;
	config.alarm_enable = 1;
	config.alarm_val = now + ALARM_INTERVAL;
	config.cb_fn = rtc_interrupt_fn;

	rtc_enable(rtc);
	rtc_set_config(rtc, &config);

	while (1) {
		now = rtc_read(rtc);
		rtc_time_to_tm(now, &tm);
		print_rtc_time(&tm);
		k_sleep(1000);
	}
}
