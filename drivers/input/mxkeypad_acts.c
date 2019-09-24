/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief matrix keyboard driver for Actions SoC
 */

#include <kernel.h>
#include <init.h>
#include <input_dev.h>
#include <soc.h>
#include "mxkeypad_acts.h"

#define SYS_LOG_DOMAIN "MXKEY"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_INPUT_DEV_LEVEL
#include <logging/sys_log.h>

static u32_t save_scan_key[3];
int mxkeypad_acts_suspend_device(struct device *dev)
{
	irq_enable(IRQ_ID_KEY_WAKEUP);
	return 0;
}

static int mxkeypad_devcie_ctrl(struct device *dev, u32_t ctrl_command,
			   void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return mxkeypad_acts_suspend_device(dev);
		}
	}

	return 0;
}

 void mxkeypad_acts_scan_key_new(const struct acts_mxkeypad_config *cfg,
				     struct acts_mxkeypad_data *keypad)
{
	u32_t key_down[3];
	u16_t keycode;
	int i;
	
	/* get maxkeypad value */
	for (i = 0; i < 3; i++) {
		key_down[i] = save_scan_key[i];
		save_scan_key[i] = 0;
	}
	
	/* get keycode */
	keycode = mxkeypad_acts_get_keycode(cfg, (u32_t*)(&key_down[0]));
	
	SYS_LOG_DBG("scan_key [0x%08x][0x%08x][0x%08x], key_code [0x%x]\n",key_down[0],key_down[1],key_down[2],keycode);
	
	/* previous key is released? */
	if (keypad->prev_stable_keycode != 0) {
		if ((keypad->last_key_down[0] != key_down[0]) 
			|| (keypad->last_key_down[1] != key_down[1]) 
			|| (keypad->last_key_down[2] != key_down[2])) {
			mxkeypad_acts_report_key(cfg, keypad, keypad->prev_stable_keycode, 0);
		}
	}

	/* a new key press? */
	if (keycode != 0) {
		mxkeypad_acts_report_key(cfg, keypad, keycode, 1);
	}

	keypad->prev_stable_keycode = keycode;
	/* update last key down state */
	for (i = 0; i < 3; i++) {
		keypad->last_key_down[i] = key_down[i];
	}

	if (key_down[0] ||
	    key_down[1] ||
	    key_down[2]) {
		/* start next poll timer */
		k_timer_stop(&keypad->timer);
		k_timer_start(&keypad->timer, cfg->poll_interval_ms, 0);
	}
}

 void mxkeypad_acts_poll_new(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	const struct acts_mxkeypad_config *cfg = dev->config->config_info;
	struct acts_mxkeypad_data *keypad = dev->driver_data;

	mxkeypad_acts_scan_key_new(cfg, keypad);
}

void mxkeypad_acts_isr_new(struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config->config_info;
	struct acts_mxkeypad_data *keypad = dev->driver_data;
	struct acts_mxkeypad_controller *base = cfg->base;
	

	SYS_LOG_DBG("enter mxkeypad isr\n");

	int i;
	
	/* clear pending */
	base->ctrl |= KEY_CTL_IRQ_PD;

	/* get maxkeypad value */
	for (i = 0; i < 3; i++) {
		save_scan_key[i] = base->info[i];
	}

	/* report key */
	mxkeypad_acts_scan_key_new(cfg, keypad);
}

int mxkeypad_acts_init_new(struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config->config_info;
	struct acts_mxkeypad_data *keypad = dev->driver_data;

	cfg->irq_config();

	k_timer_init(&keypad->timer, mxkeypad_acts_poll_new, NULL);
	k_timer_user_data_set(&keypad->timer, dev);
	return 0;
}

const struct mxkeypad_map acts_mxkeypad_maps[] = {
	MXKEYPAD_MAPS
};

 const struct acts_mxkeypad_config mxkeypad_acts_cdata = {
	.base = (struct acts_mxkeypad_controller *)KEY_REG_BASE,
	.irq_config = mxkeypad_acts_irq_config,
	.clock_id = CLOCK_ID_KEY,
	.reset_id = RESET_ID_KEY,
	.pin_mask = MXKEYPAD_MASK,

	.sample_wait_ms = 32,
	.sample_debounce_ms = 20,
	.poll_interval_ms = 100,

	.use_low_frequency = 0,
	.key_cnt = ARRAY_SIZE(acts_mxkeypad_maps),
	.key_maps = &acts_mxkeypad_maps[0],
};


DEVICE_DEFINE(mxkeypad_acts, CONFIG_INPUT_DEV_ACTS_MARTRIX_KEYPAD_NAME,
		    mxkeypad_acts_init_new, mxkeypad_devcie_ctrl,
		    &mxkeypad_acts_ddata, &mxkeypad_acts_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mxkeypad_acts_driver_api);

void mxkeypad_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_KEY, CONFIG_INPUT_DEV_ACTS_MARTRIX_KEYPAD_NAME_IRQ_PRI,
		    mxkeypad_acts_isr_new, DEVICE_GET(mxkeypad_acts), 0);
	irq_enable(IRQ_ID_KEY);
  
	IRQ_CONNECT(IRQ_ID_KEY_WAKEUP, 1,
		    mxkeypad_acts_wakeup_isr, DEVICE_GET(mxkeypad_acts), 0);
}

