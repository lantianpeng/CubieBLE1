/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <gpio.h>
#include <pwm.h>
#include <soc.h>

#include "led_hal.h"

#define SYS_LOG_DOMAIN "led"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

#if CONFIG_USE_GPIO_LED
static inline int hal_gpio_pin_write(struct device *port, u32_t pin,
				 u32_t value)
{
	return gpio_pin_write(port, pin, value);
}
static void __led_blink_event(struct k_timer *timer)
{
	struct led_device *led = CONTAINER_OF(timer, struct led_device, blink_timer);

	led->blink_state = !led->blink_state;
	if (!led->led_pol)
		hal_gpio_pin_write(led->led_dev, led->led_pin, led->blink_state);
	else
		hal_gpio_pin_write(led->led_dev, led->led_pin, !led->blink_state);
}
#endif

void led_device_init(struct led_device *led, struct led_config *led_config)
{
	if (led_config->led_pwm != NONE_PWM_CHAN) {
#if CONFIG_USE_PWM_LED
		led->led_dev = device_get_binding(CONFIG_PWM_ACTS_DEV_NAME);
		if (!led->led_dev) {
			SYS_LOG_ERR("cannot found dev pwm");
			return;
		}

		led->led_pin = led_config->led_pin;
		led->led_pwm = led_config->led_pwm;
		led->led_pol = led_config->led_pol;
		pwm_pol_set(led->led_dev, led_config->led_pwm, led_config->led_pol);
		led->on_mask = 0;
#endif
	} else {
#if CONFIG_USE_GPIO_LED
		led->led_dev = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);

		if (!led->led_dev) {
			SYS_LOG_ERR("cannot found dev gpio");
			return;
		}

		k_timer_init(&led->blink_timer, __led_blink_event, NULL);

		led->led_pin = led_config->led_pin;
		led->led_pwm = led_config->led_pwm;
		led->led_pol = led_config->led_pol;
		led->on_mask = 0;

		if (!led->led_pol)
			gpio_pin_configure(led->led_dev, led->led_pin, GPIO_DIR_OUT | GPIO_POL_NORMAL);
		else
			gpio_pin_configure(led->led_dev, led->led_pin, GPIO_DIR_OUT | GPIO_POL_INV);

#endif
	}
	reset_led_state(led);
}

u8_t get_led_status(struct led_device *led)
{
	return led->led_state;
}


s8_t find_blink_period(struct led_device *led, u32_t blink_period)
{
	int i;

	for (i = 0; i < MAX_BLINK_NUM; i++) {
		if (blink_period == led->blink_period[i])
			return i;
	}
	return -1;
}

u32_t select_fast_blink_period(struct led_device *led)
{
	int i;
	u32_t ref_val = 0xFFFF;

	for (i = 0; i < MAX_BLINK_NUM; i++) {
		if (led->blink_period[i] && (ref_val > led->blink_period[i]))
			ref_val = led->blink_period[i];
	}
	if (ref_val == 0xFFFF)
		ref_val = 0;

	return ref_val;
}

void clear_on_state(struct led_device *led, u32_t blink_period)
{
		s8_t index = -1;

		if (blink_period == 0) {
			led->on_mask &= ~(1 << MAX_BLINK_NUM);
		} else {
			index = find_blink_period(led, blink_period);
			if (index >= 0) {
				led->blink_period[index] = 0;
				led->on_mask &= ~(1 << index);
			}
		}
}

void set_on_state(struct led_device *led, u32_t blink_period)
{
		int i;

		if (blink_period == 0) {
			led->on_mask |= (1 << MAX_BLINK_NUM);
		} else {
			if (find_blink_period(led, blink_period) < 0) {
				for (i = 0; i < MAX_BLINK_NUM; i++) {
					if (led->blink_period[i] == 0) {
						led->blink_period[i] = blink_period;
						led->on_mask |= (1 << i);
						break;
					}
				}
			}
	}
}

void reset_led_state(struct led_device *led)
{
		led->on_mask = 0;
		memset(led->blink_period, 0, MAX_BLINK_NUM);
		if (led->led_pwm == NONE_PWM_CHAN) {
#if CONFIG_USE_GPIO_LED
			k_timer_stop(&led->blink_timer);
#endif
		}
}

void set_led_status(struct led_device *led, u8_t led_state, uint32_t blink_period)
{
	unsigned int key;

	key = irq_lock();

	switch (led_state) {
	case LED_ON:
		set_on_state(led, blink_period);
	break;
	case LED_OFF:
		clear_on_state(led, blink_period);
		if (led->on_mask != 0)
		led_state = LED_ON;
	break;
	default:
		SYS_LOG_ERR("unkown state %d", led_state);
	return;
	}

	led->led_state = led_state;

	switch (led->led_state) {
	case LED_ON:
	{
		u32_t period = select_fast_blink_period(led);

		if (period == 0) {
			if (led->led_pwm != NONE_PWM_CHAN) {
#if CONFIG_USE_PWM_LED
				acts_pinmux_set(led->led_pin, 0x2);
				pwm_pin_set_cycles(led->led_dev, led->led_pwm, K_MSEC(10), K_MSEC(10-1));
#endif
			} else {
#if CONFIG_USE_GPIO_LED
				k_timer_stop(&led->blink_timer);
				hal_gpio_pin_write(led->led_dev, led->led_pin, LED_ON);
#endif
				}
		} else {
			led->blink_state = 0;
			if (led->led_pwm != NONE_PWM_CHAN) {
#if CONFIG_USE_PWM_LED
				acts_pinmux_set(led->led_pin, 0x2);
				pwm_pin_set_cycles(led->led_dev, led->led_pwm, K_MSEC(period), K_MSEC(period)/2);
#endif
			} else {
#if CONFIG_USE_GPIO_LED
				k_timer_stop(&led->blink_timer);
				k_timer_start(&led->blink_timer, K_MSEC(period)/2, K_MSEC(period)/2);
#endif
				}
		}
	}
	break;
	case LED_OFF:
	{
		if (led->led_pwm != NONE_PWM_CHAN) {
#if CONFIG_USE_PWM_LED
			pwm_pin_set_cycles(led->led_dev, led->led_pwm, K_MSEC(10), 0);
			if (led->led_pol)
				acts_pinmux_set(led->led_pin, GPIO_CTL_PULLDOWN);
			else
				acts_pinmux_set(led->led_pin, GPIO_CTL_PULLUP);
#endif
		} else {
#if CONFIG_USE_GPIO_LED
			k_timer_stop(&led->blink_timer);
			hal_gpio_pin_write(led->led_dev, led->led_pin, LED_OFF);
#endif
		}
	}
	break;
	}
	irq_unlock(key);
}

void led_device_uninit(struct led_device *led)
{
	/* input, enable pull up */
#if CONFIG_USE_GPIO_LED
	gpio_pin_configure(led->led_dev, led->led_pin, GPIO_DIR_IN | GPIO_PUD_PULL_UP);
#endif
}
