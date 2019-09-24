/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LED_HAL_H__
#define __LED_HAL_H__

#include "led_manager.h"

#define MAX_BLINK_NUM 4

#define NONE_PWM_CHAN 0xFF

struct led_config {
	u8_t led_pin;
	u8_t led_pwm;
	u8_t led_pol;
};

struct led_device {
	struct device *led_dev;
	struct k_timer blink_timer;
	u8_t blink_state;
	u8_t led_pin;
	u8_t led_pwm;
	u8_t led_pol;
	u8_t led_state;
	u32_t on_mask;
	u32_t blink_period[MAX_BLINK_NUM];
};

/**
 * @brief led device initialization
 *
 * @param led_device which will be init.
 * @param led_config for led_device initialization.
 *
 */

void led_device_init(struct led_device *led, struct led_config *led_config);

/**
 * @brief get led_device state
 *
 * @param led_device which will be operated.
 *
 */

u8_t get_led_status(struct led_device *led);

/**
 * @brief reset led_device state
 *
 * @param led_device which will be operated.
 *
 */

void reset_led_state(struct led_device *led);

/**
 * @brief set led_device state
 *
 * @param led_device which will be operated.
 * @param led_state(LED_ON/LED_OFF) of led_device will be set .
 * @param the blink period of led_device will be set.
 *
 */

void set_led_status(struct led_device *led, u8_t led_state, u32_t blink_period);

/**
 * @brief led device uninit
 *
 * @param led_device which will be uninit.
 *
 */

void led_device_uninit(struct led_device *led);

#endif
