/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include "app_gpio.h"
#include "at_cmd.h"
#include "msg_manager.h"
#include "app_ble.h"
#include "app_cfg.h"
#include "soc.h"

extern void acts_delay_us(uint32_t us);

static struct device *gpio_dev;
struct gpio_callback en_pin_cb;
#ifdef MODULE_RTS_PIN
struct gpio_callback rts_pin_cb;
#endif
struct gpio_callback reset_pin_cb;
u8_t rts_func_enable = GPIO_RTS_ENABLE_DEF;
#define ENTER_LOWPOWER_MODE 0
#define EXIT_LOWPOWER_MODE 1

void app_gpio_callback_handler(struct device *gpiob, struct gpio_callback *cb,
		u32_t pins)
{
	u8_t pin = find_lsb_set(pins) - 1;	
	u32_t val;

	if (pin == MODULE_EN_PIN) {
		gpio_pin_read(gpiob, pin, &val);
		if (val == 0) {
			struct app_msg  msg = {0};
			printk("1exit low power mode\n");
			device_busy_set(gpiob);
			msg.type = MSG_APP_GPIO;
			msg.value = EXIT_LOWPOWER_MODE;
			send_msg(&msg, K_NO_WAIT);
		} else {
			struct app_msg  msg = {0};
			printk("1enter low power mode\n");
			msg.type = MSG_APP_GPIO;
			msg.value = ENTER_LOWPOWER_MODE;
			send_msg(&msg, K_NO_WAIT);
			device_busy_clear(gpiob);
		}
	} 
#ifdef MODULE_RTS_PIN
	if (rts_func_enable && (pin == MODULE_RTS_PIN)) {
		gpio_pin_read(gpiob, pin, &val);
		if (val == 0) {
			printk("rts low level\n");
			device_busy_set(gpiob);
		} else {
			printk("rts high level\n");
			device_busy_clear(gpiob);
		}
	}
#endif
	if (pin == MODULE_RESET_PIN) {
		printk("reset pin trigger\n");
		set_app_cfg_to_default();
		acts_delay_us(10000);
		sys_pm_reboot(REBOOT_TYPE_GOTO_APP);
	}
}

void gpio_output_wakeup_host(u8_t enable)
{
	if (gpio_dev != NULL) {
		if (enable) {
			gpio_pin_write(gpio_dev, MODULE_CTS_PIN, 1);
			acts_delay_us(500);
		} else {
			acts_delay_us(1000);
			gpio_pin_write(gpio_dev, MODULE_CTS_PIN, 0);
		}
	}
}

void gpio_output_notify_state(u8_t enable)
{
	if (gpio_dev != NULL) {
		if (enable)
			gpio_pin_write(gpio_dev, MODULE_NTF_PIN, 1);
		else
			gpio_pin_write(gpio_dev, MODULE_NTF_PIN, 0);
	}
}

void gpio_output_flow_ctrl(u8_t enable)
{
	if (gpio_dev != NULL) {
		if (enable)
			gpio_pin_write(gpio_dev, MODULE_FLOW_PIN, 1);
		else
			gpio_pin_write(gpio_dev, MODULE_FLOW_PIN, 0);
	}
}

void gpio_event_handle(u32_t event)
{
	switch (event) {
	case ENTER_LOWPOWER_MODE:
		clear_ble_state_before_suspend();
		break;
	case EXIT_LOWPOWER_MODE:
		set_ble_state_after_resume();
		break;
	}
}

bool app_gpio_init(void)
{

	gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);
	if (!gpio_dev) {
		printk("cannot found gpio device\n");
		return false;
	}

	/* init intput pin */
	gpio_pin_configure(gpio_dev, MODULE_EN_PIN,
			GPIO_INT_DEBOUNCE | GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE);
	gpio_init_callback(&en_pin_cb, app_gpio_callback_handler, BIT(MODULE_EN_PIN));
	gpio_add_callback(gpio_dev, &en_pin_cb);
	gpio_pin_enable_callback(gpio_dev, MODULE_EN_PIN);

	/* if use rts, power is smaller than use en only */
#ifdef MODULE_RTS_PIN	
	gpio_pin_configure(gpio_dev, MODULE_RTS_PIN,
			GPIO_INT_DEBOUNCE | GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE);
	gpio_init_callback(&rts_pin_cb, app_gpio_callback_handler, BIT(MODULE_RTS_PIN));
	gpio_add_callback(gpio_dev, &rts_pin_cb);
	gpio_pin_enable_callback(gpio_dev, MODULE_RTS_PIN);
#endif
	
	/* reset pin */
	gpio_pin_configure(gpio_dev, MODULE_RESET_PIN,
			GPIO_INT_DEBOUNCE | GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);
	gpio_init_callback(&reset_pin_cb, app_gpio_callback_handler, BIT(MODULE_RESET_PIN));
	gpio_add_callback(gpio_dev, &reset_pin_cb);
	gpio_pin_enable_callback(gpio_dev, MODULE_RESET_PIN);
	
	/* init output pin */
	gpio_pin_configure(gpio_dev, MODULE_CTS_PIN, GPIO_DIR_OUT | GPIO_POL_INV);
	gpio_pin_configure(gpio_dev, MODULE_NTF_PIN, GPIO_DIR_OUT | GPIO_POL_INV);
	gpio_pin_configure(gpio_dev, MODULE_FLOW_PIN, GPIO_DIR_OUT | GPIO_POL_INV);
	gpio_output_wakeup_host(false);
	gpio_output_notify_state(false);
	gpio_output_flow_ctrl(false);
	
	return true;
}
