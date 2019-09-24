/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <misc/printk.h>
#include <led_manager.h>
#include "system_app_led.h"

struct k_delayed_work low_power_ind_timer;
struct k_delayed_work paired_ind_timer;
struct k_delayed_work ir_btn_ind_timer;

void low_power_led_indication(void)
{
	k_delayed_work_submit(&low_power_ind_timer, K_MSEC(LOW_POWER_IND_TIMEOUT));
	led_set_state(LED_LPOWER_PIN, LED_ON, LOW_POWER_IND_BLINK_PERIOD);
}

static void low_power_ind_timeout_event(struct k_work *work)
{
	led_set_state(LED_LPOWER_PIN, LED_OFF, LOW_POWER_IND_BLINK_PERIOD);
}

static void paired_indication(void)
{
	led_set_state(LED_PAIR_PIN, LED_OFF, PAIRING_IND_BLINK_PERIOD);
	k_delayed_work_submit(&paired_ind_timer, K_MSEC(PAIRED_IND_TIMEOUT));
	led_set_state(LED_PAIR_PIN, LED_ON, PAIRED_IND_BLINK_PERIOD);
}

static void paired_ind_timeout_event(struct k_work *work)
{
	led_set_state(LED_PAIR_PIN, LED_OFF, PAIRED_IND_BLINK_PERIOD);
}

void ir_btn_led_indication(void)
{
	led_set_state(LED_IR_BTN_PIN, LED_ON, IR_BTN_IND_BLINK_PERIOD);
	k_delayed_work_submit(&ir_btn_ind_timer, K_MSEC(IR_BTN_IND_TIMEOUT));
}

static void ir_btn_ind_timeout_event(struct k_work *work)
{
	led_set_state(LED_IR_BTN_PIN, LED_OFF, IR_BTN_IND_BLINK_PERIOD);
}

#ifdef CONFIG_IRC_SW_RX
static u16_t learn_result_flash_time = 0;
struct k_delayed_work delay_timer;
struct k_delayed_work tv_ir_btn_ind_timer;
void tv_ir_btn_led_indication(u8_t state)
{
	led_set_state(LED_IR_BTN_PIN, state, TV_IR_BTN_IND_BLINK_PERIOD);
}
void ir_learn_led_indication(u8_t state)
{
	led_set_state(LED_IR_BTN_PIN, state, LEARNING_IND_BLINK_PERIOD);
}

static void tv_ir_btn_ind_timeout_event(struct k_work *work)
{
	led_set_state(LED_IR_BTN_PIN, LED_OFF, IR_LEARN_RESULT_IND_BLINK_PERIOD);
}
void ir_learn_result_led_indication(u16_t state_time)
{
	k_delayed_work_submit(&delay_timer, K_MSEC(LEARNING_IND_BLINK_PERIOD));
	learn_result_flash_time = state_time;
}

static void delay_timeout_event(struct k_work *work)
{
	led_set_state(LED_IR_BTN_PIN, LED_ON, IR_LEARN_RESULT_IND_BLINK_PERIOD);
	k_delayed_work_submit(&tv_ir_btn_ind_timer, K_MSEC(learn_result_flash_time));
}
#endif

void btn_led_indication(u8_t state)
{
	led_set_state(LED_BTN_PIN, state, 0);
}

void pair_led_indication(u8_t state)
{
	/* hand led indication event */
	switch (state) {
	case BLE_PAIRING:
		led_set_state(LED_PAIR_PIN, LED_ON, PAIRING_IND_BLINK_PERIOD);
	break;
	case BLE_PAIRED:
		paired_indication();
	break;
	case BLE_PAIRING_TIMEOUT:
		led_set_state(LED_PAIR_PIN, LED_OFF, PAIRING_IND_BLINK_PERIOD);
	break;
	}
}

__init_once_text void system_led_handle_init(void)
{
	k_delayed_work_init(&low_power_ind_timer, low_power_ind_timeout_event);
	k_delayed_work_init(&paired_ind_timer, paired_ind_timeout_event);
	k_delayed_work_init(&ir_btn_ind_timer, ir_btn_ind_timeout_event);
#ifdef CONFIG_IRC_SW_RX
	k_delayed_work_init(&delay_timer, delay_timeout_event);
	k_delayed_work_init(&tv_ir_btn_ind_timer, tv_ir_btn_ind_timeout_event);
#endif
}
