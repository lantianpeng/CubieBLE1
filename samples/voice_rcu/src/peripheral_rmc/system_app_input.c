/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <input_dev.h>

#include "system_app_input.h"
#include "msg_manager.h"
#include "input_manager.h"
#include "system_app_sm.h"
#include "system_app_timer.h"
#include "system_app_led.h"
#include "system_app_ble.h"
#include "system_app_audio.h"
#include "irc_hal.h"
#include "system_app_storage.h"
#define SYS_LOG_DOMAIN "input"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>
#include <gpio.h>
struct irc_handle *irc_tx_handle;
static u16_t previous_ir_key_code = KEY_RESERVED;
extern void irc_input_acts_register_notify(struct device *dev, input_notify_t notify);
extern void irc_input_acts_enable(struct device *dev);

#ifdef CONFIG_IRC_SW_RX
struct learn_key_map {
	u16_t key_code;
	u16_t learn_key_code;
};

const struct learn_key_map acts_learn_key_maps[] = {
	LEARN_KEY_MAPS
};

/* key_code -> learn_key_code */
u16_t acts_get_learn_keycode(int key_code)
{
	int i;
	const struct learn_key_map *map = &acts_learn_key_maps[0];

	for (i = 0; i < ARRAY_SIZE(acts_learn_key_maps); i++) {

		if (key_code == map->key_code)
			return map->learn_key_code;

		map++;
	}

	return KEY_RESERVED;
}

u8_t learn_ready = 0;
void irc_input_handle(struct device *dev, struct input_value *val)
{
	if (learn_ready) {
		ir_learn_led_indication(BTN_UP);
		learn_ready = 0;
	}
	
	printk("irc_dev->cur_learn_state : %d\n", irc_tx_handle->cur_learn_state);
	switch (irc_tx_handle->cur_learn_state) {
	case IR_LEARN_SUCCESS:
		ir_learn_result_led_indication(IR_LEARN_SUCCESS_IND_TIMEOUT);
		break;
	case IR_LEARN_FAILED:
		ir_learn_result_led_indication(IR_LEARN_FAIL_IND_TIMEOUT);
		break;
	case IR_LEARN_TIMEOUT:
		btn_led_indication(BTN_UP);
		break;
	case IR_LEARN_READY:
		ir_learn_led_indication(BTN_DOWN);
		learn_ready = 1;
		break;
	}
}

void enter_into_learn_mode(void)
{
	if ((irc_tx_handle != NULL) && (!irc_tx_handle->sw_ir_learn_mode)) {
		tv_ir_btn_led_indication(BTN_UP);
		irc_device_enable_rx(irc_tx_handle, irc_input_handle);
		btn_led_indication(BTN_DOWN);
	}
}
#endif
void send_ir_keycode(u32_t key_value)
{
		if (irc_tx_handle != NULL) {
			u16_t ir_key_code = acts_get_ir_keycode(key_value & 0xFFFF);

			if (ir_key_code == KEY_RESERVED)
				return;
			if (rmc_sm_state_is_activing() || !rmc_security_ready()) {
				if (key_value & KEY_TYPE_DOWN) {
					if (previous_ir_key_code != ir_key_code)
						ir_btn_led_indication();
					irc_device_send_key(irc_tx_handle, ir_key_code, 1);
					previous_ir_key_code = ir_key_code;
				} else {
					irc_device_send_key(irc_tx_handle, ir_key_code, 0);
					previous_ir_key_code = KEY_RESERVED;
				}
			} else if (irc_device_state_get(irc_tx_handle)) {
				irc_device_send_key(irc_tx_handle, ir_key_code, 0);
				previous_ir_key_code = KEY_RESERVED;
			}
		}
}

u32_t pending_key;
void send_ble_keycode(u32_t key_value)
{
	u8_t ret;

	if (rmc_sm_state_is_activing() || !rmc_security_ready()) {
		pending_key = key_value;
		SYS_LOG_INF("pending_key : 0x%x", pending_key);
	} else {
		u16_t ble_key_code = acts_get_ble_keycode(key_value & 0xFFFF);

		if (ble_key_code == KEY_RESERVED)
			return;
		pending_key = 0;
		if (key_value & KEY_TYPE_DOWN) {
			ret = hid_app_remote_report_event(ble_key_code);
			if (ret == 0)
				SYS_LOG_INF("Send ble_key_code :0x%x down!", ble_key_code);

		} else {
			ret = hid_app_remote_report_event(0x0000);
			if (ret == 0)
				SYS_LOG_INF("Send ble_key_code :0x%x up!", ble_key_code);
			
		}
	}
}

void send_pending_keycode(void)
{
	u32_t key_value = pending_key;

	if (key_value & KEY_TYPE_DOWN) {
		send_ble_keycode(key_value);
	} else {
		u16_t ble_key_code = acts_get_ble_keycode(key_value & 0xFFFF);

		if (ble_key_code == KEY_RESERVED)
			return;
		hid_app_remote_report_event(ble_key_code);
		hid_app_remote_report_event(0x0000);
	}
}

void _system_key_event_cb(u32_t key_value)
{
#ifdef CONFIG_IRC_SW_RX
		if (overlay_ready_flag == 0)
			return;
		if (irc_tx_handle != NULL) {
			u16_t ir_learn_key_code = acts_get_learn_keycode(key_value & 0xFFFF);
			if (ir_learn_key_code != KEY_RESERVED) {
					if (key_value & KEY_TYPE_DOWN) {
						if (!irc_tx_handle->sw_ir_learn_mode)
							tv_ir_btn_led_indication(BTN_DOWN);
						irc_device_send_learn_key(irc_tx_handle, ir_learn_key_code, 1);
					} else {
						tv_ir_btn_led_indication(BTN_UP);
						irc_device_send_learn_key(irc_tx_handle, ir_learn_key_code, 0);
					}
				return;
			}
			
			/* exit learn mode when other key send */
			irc_device_exit_learn_mode(irc_tx_handle);
		}
#endif
	if (key_value & KEY_TYPE_DOWN) {
		btn_led_indication(BTN_DOWN);
		if (acts_get_ble_keycode(key_value & 0xFFFF) == REMOTE_KEY_VOICE_COMMAND)
			start_audio_capture();
	} else if (key_value & KEY_TYPE_UP) {
		btn_led_indication(BTN_UP);
		if (acts_get_ble_keycode(key_value & 0xFFFF) == REMOTE_KEY_VOICE_COMMAND)
			stop_audio_capture();
	}
	send_ir_keycode(key_value);
}

bool is_need_report_hold(u32_t key_code)
{
#if CONFIG_NEED_REPORT_REPEAT_KEY
	u16_t app_key_code = acts_get_ble_keycode(key_code & 0xFFFF);

	switch (app_key_code) {
	case REMOTE_KEY_VOICE_COMMAND:
	case REMOTE_COMB_KEY_OK_BACK:
	case REMOTE_COMB_KEY_TEST_ONE:
	case REMOTE_COMB_KEY_TEST_TWO:
	case REMOTE_COMB_KEY_TEST_THREE:
	case REMOTE_COMB_KEY_TEST_FOUR:
	case REMOTE_COMB_KEY_HCI_MODE:
			return false;
	default:
			return true;
	}
#else
	return false;
#endif
}

u16_t comb_key_code = REMOTE_COMB_KEY_OK_BACK;
u8_t pair_comb_key_timer_start;
u8_t hci_mode_comb_key_timer_start;
static bool is_need_deliver_to_app(u32_t event)
{
	bool need_deliver = true;
	u16_t app_key_code = acts_get_ble_keycode(event & 0xFFFF);

	switch (app_key_code) {
	case REMOTE_KEY_VOICE_COMMAND:
	{
		if (event & KEY_TYPE_DOWN) {
			if (!rmc_sm_state_is_activing() && rmc_security_ready())
				audio_send_start_flag();
		} else {
			if (!rmc_sm_state_is_activing() && rmc_security_ready())
				audio_send_stop_flag();
		}
		need_deliver = false;
		break;
	}
	case REMOTE_COMB_KEY_OK_BACK:
	case REMOTE_COMB_KEY_TEST_ONE:
	case REMOTE_COMB_KEY_TEST_TWO:
	case REMOTE_COMB_KEY_TEST_THREE:
	case REMOTE_COMB_KEY_TEST_FOUR:
	{
		if (event & KEY_TYPE_DOWN) {
			rmc_timer_start(ID_PAIR_COMB_KEY_TIMEOUT);
			comb_key_code = app_key_code;
			pair_comb_key_timer_start = 1;
			SYS_LOG_INF("start comb key timer 0x%x", app_key_code);
		} else {
			rmc_timer_stop(ID_PAIR_COMB_KEY_TIMEOUT);
			pair_comb_key_timer_start = 0;
			SYS_LOG_INF("stop comb key timer 0x%x", app_key_code);
		}
		need_deliver = false;
		break;
	}
	case REMOTE_COMB_KEY_HCI_MODE:
	{
		if (event & KEY_TYPE_DOWN) {
			rmc_timer_start(ID_HCI_MODE_COMB_KEY_TIMEOUT);
			hci_mode_comb_key_timer_start = 1;
			SYS_LOG_INF("start hci mode timer 0x%x", app_key_code);
		} else {
			rmc_timer_stop(ID_HCI_MODE_COMB_KEY_TIMEOUT);
			hci_mode_comb_key_timer_start = 0;
			SYS_LOG_INF("stop hci mode timer 0x%x", app_key_code);
		}
		need_deliver = false;
		break;
	}
	default:
		/* exception handle for comb key up is not coming, maybe input msg buffer is not enough */
	if (pair_comb_key_timer_start) {
		rmc_timer_stop(ID_PAIR_COMB_KEY_TIMEOUT);
		pair_comb_key_timer_start = 0;
		SYS_LOG_INF("stop comb key timer for 0x%x", app_key_code);
	}
	if (hci_mode_comb_key_timer_start) {
		rmc_timer_stop(ID_HCI_MODE_COMB_KEY_TIMEOUT);
		hci_mode_comb_key_timer_start = 0;
		SYS_LOG_INF("stop hci mode timer for 0x%x", app_key_code);
	}
	break;
	}
	return need_deliver;
}

struct key_map {
	u16_t key_code;
	u16_t ble_key_code;
	u16_t ir_key_code;
};

const struct key_map acts_key_maps[] = {
	KEY_MAPS
};

/* key_code -> ble_key_code */
u16_t acts_get_ble_keycode(int key_code)
{
	int i;
	const struct key_map *map = &acts_key_maps[0];

	for (i = 0; i < ARRAY_SIZE(acts_key_maps); i++) {

		if (key_code == map->key_code)
			return map->ble_key_code;

		map++;
	}

	return KEY_RESERVED;
}

/* key_code -> ir_key_code */
u16_t acts_get_ir_keycode(int key_code)
{
	int i;
	const struct key_map *map = &acts_key_maps[0];

	for (i = 0; i < ARRAY_SIZE(acts_key_maps); i++) {

		if (key_code == map->key_code)
			return map->ir_key_code;

		map++;
	}

	return KEY_RESERVED;
}

void system_input_event_handle(u32_t key_event)
{
#ifdef CONFIG_IRC_SW_RX
	u16_t ir_learn_key_code = acts_get_learn_keycode(key_event & 0xFFFF);
	if (ir_learn_key_code != KEY_RESERVED) {
		switch (ir_learn_key_code) {
			case TV_KEY_POWER:
			{
				if (key_event & KEY_TYPE_DOWN) {
					rmc_timer_start(ID_IR_LEARN_KEY_TIMEOUT);
					SYS_LOG_INF("start learn key 0x%x", ir_learn_key_code);
				} else {
					rmc_timer_stop(ID_IR_LEARN_KEY_TIMEOUT);
					SYS_LOG_INF("stop learn key 0x%x", ir_learn_key_code);
				}
				break;
			}
		}
		return ;
	}
#endif
	/* 1. state switch */
	rmc_sm_execute(RMC_MSG_INPUT_KEY);

	if (is_need_deliver_to_app(key_event)) {
		/* 2. send key */
		send_ble_keycode(key_event);
	}
}

__init_once_text void system_input_handle_init(void)
{
	input_manager_init(_system_key_event_cb, is_need_report_hold);
	/* input init is locked ,so we must unlock */
	input_event_unlock();

	irc_tx_handle = irc_device_open(CONFIG_IRC_ACTS_DEV_NAME);
	if (irc_tx_handle == NULL)
		SYS_LOG_ERR("open irc fail");
}

#ifdef CONFIG_IRC_RX
void test_irc_input_notify(struct device *dev, struct input_value *val)
{
 struct device *gpio_dev;
	
	gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);
	if (!gpio_dev) {
		printk("cannot found device \'%s\'\n",
			    CONFIG_GPIO_ACTS_DRV_NAME);
	}

	/* set PIN_OUT as writer */
	gpio_pin_configure(gpio_dev, LED_IR_TX_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(gpio_dev, LED_IRC_RX_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio_dev, LED_IR_TX_PIN, 1);
	gpio_pin_write(gpio_dev, LED_IRC_RX_PIN, 1);
	k_sleep(50);
	gpio_pin_write(gpio_dev, LED_IR_TX_PIN, 0);
	gpio_pin_write(gpio_dev, LED_IRC_RX_PIN, 0);
}

void IRC_rx_test(void)
{
	struct device *dev2 = device_get_binding(CONFIG_IRC_ACTS_DEV_NAME);
	if (!dev2) {
		printk("can't get device IRC\n");
		return;
	}
	irc_input_acts_enable(dev2);
	irc_input_acts_register_notify(dev2,test_irc_input_notify);
}
#endif
