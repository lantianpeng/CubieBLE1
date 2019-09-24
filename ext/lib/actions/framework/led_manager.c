/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include "led_hal.h"

#define SYS_LOG_DOMAIN "led"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

const struct led_config led_configs[] = {
	LED_PIN_CONFIG
};

#define LED_TOTAL_NUM ARRAY_SIZE(led_configs)

struct led_device led_mng[LED_TOTAL_NUM];

/*init manager*/
__init_once_text bool led_manager_init(void)
{
	int i = 0;

	for (i = 0; i < LED_TOTAL_NUM; i++)
		led_device_init((struct led_device *)(&led_mng[i]),
			(struct led_config *)(&led_configs[i]));

	return true;
}

/*uninit manager*/
bool led_manager_uninit(void)
{
	int i = 0;

	for (i = 0; i < LED_TOTAL_NUM; i++)
		led_device_uninit((struct led_device *)(&led_mng[i]));

	return true;
}

void led_set_state(int led_id, int state, int blink_period)
{
	struct led_device *led = NULL;
	int i = 0;

	for (i = 0; i < LED_TOTAL_NUM; i++) {
		if (led_mng[i].led_pin == led_id)
			led = (struct led_device *)(&led_mng[i]);
	}

	if (led != NULL)
		set_led_status(led, state, blink_period);
	else
		SYS_LOG_ERR("led not config");
}
