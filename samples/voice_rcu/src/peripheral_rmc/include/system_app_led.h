/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SYSTEM_APP_LED_H_
#define _SYSTEM_APP_LED_H_

#include <led_manager.h>

/*low power indicator */
#define LOW_POWER_IND_BLINK_PERIOD	100		/* 100 ms	*/
#define LOW_POWER_IND_TIMEOUT				3000	/* 3000 ms*/

#define IR_BTN_IND_BLINK_PERIOD			200			/* 200 ms	*/
#define IR_BTN_IND_TIMEOUT					400			/* 400 ms	*/
#ifdef CONFIG_IRC_SW_RX
#define TV_IR_BTN_IND_BLINK_PERIOD      300			/* 300 ms	*/
#define IR_LEARN_RESULT_IND_BLINK_PERIOD		250			/* 250 ms	*/
#define IR_LEARN_SUCCESS_IND_TIMEOUT				500			/* 500 ms	*/
#define IR_LEARN_FAIL_IND_TIMEOUT						1250		/* 1250 ms	*/
#endif

/* ble paring indicator */
#define PAIRING_IND_BLINK_PERIOD		400			/* 400 ms	*/

#ifdef CONFIG_IRC_SW_RX
/* irc learning indicator */
#define LEARNING_IND_BLINK_PERIOD		800			/* 800 ms	*/
#endif

/* ble pair success indicator */
#define PAIRED_IND_BLINK_PERIOD			1000			/* 1000 ms	*/
#define PAIRED_IND_TIMEOUT					2000			/* 2000 ms	*/

/* led indication event */
#define BTN_DOWN	LED_ON
#define BTN_UP		LED_OFF

/* pairing state */
enum {
	BLE_PAIRING,
	BLE_PAIRED,
	BLE_PAIRING_TIMEOUT,
};

/**
 * @brief led indicates at which stage the current pairing is
 *
 * @param state BLE_PAIRING/BLE_PAIRED/BLE_PAIRING_TIMEOUT .
 */

void pair_led_indication(u8_t state);

/**
 * @brief led indicates that system enter into low power mode
 *
 */

void low_power_led_indication(void);

/**
 * @brief led indicate whether ir key is triggered
 *
 * @param state key_up or key_down.
 */

void ir_btn_led_indication(void);

#ifdef CONFIG_IRC_SW_RX
void tv_ir_btn_led_indication(u8_t state);
void ir_learn_led_indication(u8_t state);
void ir_learn_result_led_indication(u16_t state_time);
#endif

/**
 * @brief led indicate whether key is up or down
 *
 * @param state key_up or key_down.
 */

void btn_led_indication(u8_t state);

/**
 * @brief application led initialization
 *
 */

__init_once_text void system_led_handle_init(void);


#endif    /* _SYSTEM_APP_LED_H_ */

