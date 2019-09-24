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

#include <pwm.h>

#define PWM_CHANNEL	1

/* in micro second */
#define MIN_PERIOD	(USEC_PER_SEC / 16)

/* in micro second */
#define MAX_PERIOD	(USEC_PER_SEC * 2)

void app_main(void)
{
	struct device *pwm_dev;
	u32_t period = MAX_PERIOD;
	u8_t dir = 0;

	printk("PWM demo app-blink LED\n");

	pwm_dev = device_get_binding(CONFIG_PWM_ACTS_DEV_NAME);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", CONFIG_PWM_ACTS_DEV_NAME);
		goto out;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				     period, period / 2)) {
			printk("pwm pin set fails\n");
			goto out;
		}

		if (dir) {
			period *= 2;

			if (period > MAX_PERIOD) {
				dir = 0;
				period = MAX_PERIOD;
			}
		} else {
			period /= 2;

			if (period < MIN_PERIOD) {
				dir = 1;
				period = MIN_PERIOD;
			}
		}
		printk("pwm, dir:%d, period:%d\n", dir, period);
		k_sleep(MSEC_PER_SEC * 4);

	}

out:
	while (1)
		;
}
