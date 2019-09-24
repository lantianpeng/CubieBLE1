/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <app_input.h>
#include <at_cmd.h>
#include <soc.h>
#include <misc.h>

enum gpio_trigger{
	LOW_EDGE,
	HIGH_EDGE,
};

const struct input_config input_configs[] = {
	INPUT_CONFIG
};

#define INPUT_TOTAL_NUM ARRAY_SIZE(input_configs)

struct gpio_callback input_cb[INPUT_TOTAL_NUM];
static struct device *input_dev;

/* for enter deepsleep */
void clear_device_busy(void)
{
	device_busy_clear(input_dev);
}

/* for exit deepsleep */
void set_device_busy(void)
{
	device_busy_set(input_dev);	
}

void input_handler(struct device *gpiob, struct gpio_callback *cb,
		u32_t pins)
{
	u32_t edge = 0;

	u8_t pin = find_lsb_set(pins) - 1;	
	if ((pins & (1 << CONN_INT_PIN)) && 
			(pins & (1 << POWM_MODE_PIN))) {
		return;
	}
	
	printk("pin=%d interrupt\n", pin); 
	
	switch (pin) {
	case CONN_INT_PIN:
		printk("send disconnect msg\n");
		send_async_msg(MSG_DISCONNECT);	
		break;
	case POWM_MODE_PIN:
		gpio_pin_read(input_dev, pin, &edge);
		if (edge == HIGH_EDGE){
			printk("exit low power mode\n");
			set_device_busy();
		} else {
			printk("enter low power mode\n");
			clear_device_busy();
			printk("send disconnect msg\n");
			send_async_msg(MSG_DISCONNECT);	
		}	
		break;
	}
}

bool input_manager_init(void)
{
	int i;	

	input_dev = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);
	if (!input_dev) {
		printk("cannot found input device\n");
		return false;
	}

#if CONFIG_BLE_ROLE_MASTER
	for (i = 0; i < INPUT_TOTAL_NUM; i++) {
		gpio_pin_configure(input_dev, input_configs[i].pin,
				GPIO_INT_DEBOUNCE | GPIO_DIR_IN | GPIO_INT | input_configs[i].pull_state | input_configs[i].trigger_mode);
		gpio_init_callback(&input_cb[i], input_handler, BIT(input_configs[i].pin));
		gpio_add_callback(input_dev, &input_cb[i]);
		gpio_pin_enable_callback(input_dev, input_configs[i].pin);
	}
#endif
		return true;
}

