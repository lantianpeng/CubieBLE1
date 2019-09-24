/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LED_MANAGER_H__
#define __LED_MANAGER_H__

/**
 * @defgroup led_manager_apis App Led Manager APIs
 * @ingroup lib_app_apis
 * @{
 */

/** led status */
enum {
	/** led status off */
	LED_OFF = 1,
	/** led status on */
	LED_ON = 0,
};

/**
 * @brief set target led states
 *
 * @param led_id target led id
 * @param state led status LED_ON , LED_OFF ,LED_FAST_BLINK , LED_SLOW_BLINK
 *
 *
 * @return N/A
 */
void led_set_state(int led_id, int state, int blink_period);

/**
 * @brief led manager init funcion
 *
 * This routine calls init app manager ,called by main
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool led_manager_init(void);

/**
 * @} end defgroup led_manager_apis
 */

#endif
