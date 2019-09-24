/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <kernel.h>
#include <key_hal.h>
#include <adckey_hal.h>
#include "msg_manager.h"
#include "input_manager.h"
#include "system_app_input.h"
#include <misc/printk.h>

#define SYS_LOG_DOMAIN "input"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

struct input_manager_info {
	event_trigger event_cb;
	is_need_report_hold_key_t is_need_report_hold_key;
	u32_t press_type;
	u32_t press_code;
	u32_t report_key_value;
	bool input_event_lock;
};

static struct input_manager_info input_manager_info_entity;
struct input_manager_info *input_manager = &input_manager_info_entity;

void report_key_event(void)
{
	struct app_msg  msg = {0};

	msg.type = MSG_KEY_INPUT;
	msg.value = input_manager->report_key_value;

	send_msg(&msg, K_NO_WAIT);
}

bool is_need_report_hold_defalt(uint32_t key_code)
{
	return true;
}

void key_event_handle(struct device *dev, struct input_value *val)
{
	bool need_report = false;

	if (val->type != EV_KEY) {
		SYS_LOG_INF("input type not support val->type %d", val->type);
		return;
	}

	switch (val->value) {
	case KEY_VALUE_UP:
	{
		input_manager->press_code = val->code;
		input_manager->press_type = KEY_TYPE_UP;
		need_report = true;
		break;
	}
	case KEY_VALUE_DOWN:
	{
		if ((val->code != input_manager->press_code) ||
			(input_manager->press_type != KEY_TYPE_DOWN)) {
			input_manager->press_code = val->code;
			input_manager->press_type = KEY_TYPE_DOWN;
			need_report = true;
		} else {
			if (input_manager->is_need_report_hold_key &&
				input_manager->is_need_report_hold_key(input_manager->press_code))
				need_report = true;
		}
		break;
	}
	default:
		break;
	}

	if (need_report) {
		input_manager->report_key_value = input_manager->press_type
									| input_manager->press_code;

		if (input_manager->event_cb)
			input_manager->event_cb(input_manager->report_key_value);

		if (!input_event_islock())
			report_key_event();
	}
}

/*init manager*/
__init_once_text bool input_manager_init(event_trigger event_cb,
	is_need_report_hold_key_t is_need_report_hold_key)
{
	if (key_device_open(&key_event_handle, CONFIG_INPUT_DEV_ACTS_MARTRIX_KEYPAD_NAME) == NULL) {
		SYS_LOG_ERR("open mxkeyapd devices failed");
		return false;
	}

	if (adckey_device_open(key_event_handle, CONFIG_INPUT_DEV_ACTS_ADCKEY_0_NAME) == NULL) {
		SYS_LOG_ERR("open adckey0 devices failed");
		return false;
	}
	
	input_manager->event_cb = event_cb;
	input_manager->is_need_report_hold_key = (is_need_report_hold_key != NULL) ?
	is_need_report_hold_key : is_need_report_hold_defalt;

	SYS_LOG_INF("input_manager_init success");
	input_event_lock();
	return true;
}
bool input_event_lock(void)
{
	unsigned int key;

	key = irq_lock();
	input_manager->input_event_lock = true;
	irq_unlock(key);
	return true;
}

bool input_event_unlock(void)
{
	unsigned int key;

	key = irq_lock();
	input_manager->input_event_lock = false;
	irq_unlock(key);
	return true;
}

bool input_event_islock(void)
{
	bool result = false;
	unsigned int key;

	key = irq_lock();
	result = input_manager->input_event_lock;
	irq_unlock(key);
	return result;
}
