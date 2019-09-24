/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>

#include "irc_hal.h"
#include "irc.h"
#include "input_dev.h"
#ifdef CONFIG_IRC_SW_RX
#include "sw_irc_acts.h"
#endif
#define SYS_LOG_DOMAIN "irc"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>
#include "nvram_config.h"
#include <string.h>
#include "irc_protocol.h"


struct irc_handle irc_handle_entity;

#ifdef CONFIG_IRC_SW_RX

#define DONT_SAVE_REPEAT_KEY 1

#define CONFIG_IRC_LEARN_MODE_TIMEOUT 15

struct nv_key_map acts_nv_key_maps[] = {
	NV_KEY_MAPS
};

struct flash_pulse_tab_t {
	u16_t flash_pulse_width_tab[MAX_PULSE_LEN];
	u16_t flash_pulse_width_tab_len;
};

static struct flash_pulse_tab_t flash_pulse_tab;

u8_t check_irc_wave(u16_t *pulse_tab, u16_t tab_len)
{
	if (is_nec_wave(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;
	else if (is_9012_protocol(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;
	else if (is_rc5_protocol(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;
	else if (is_rc6_protocol(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;
	else if (is_7461_protocol(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;
	else if (is_50560_protocol(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;
	else if (is_50462_protocol(pulse_tab, tab_len))
		return IR_LEARN_SUCCESS;

	return IR_LEARN_FAILED;
}

/* learn_key_code -> nv_key */
char* acts_get_nv_keycode(int key_code)
{
	int i;
	const struct nv_key_map *map = &acts_nv_key_maps[0];

	for (i = 0; i < ARRAY_SIZE(acts_nv_key_maps); i++) {

		if (key_code == map->learn_key_code)
			return map->nv_key_name;

		map++;
	}

	return NULL;
}
void irc_hal_event_handle(struct device *dev, struct input_value *ir_val)
{
	struct irc_handle *handle = &irc_handle_entity;
	struct ir_input_value *val = (struct ir_input_value *)ir_val;
	printk("type: 0x%x, code: 0x%x, val: 0x%x\n", val->type, val->code, val->value);
	char * nvram_key_name = NULL;
	
	/* report event when learn_key has set and curruent state is learn mode */
	if ((!handle->has_set_learn_key) || (!handle->sw_ir_learn_mode))
		return ;
	
	if (val->type == IR_FORMAT_PULSE_TAB) {
		u16_t *p_pulse_tab = (u16_t *)val->pluse_tab;
		u16_t p_pulse_width = val->pluse_tab_len;
		
#if 1
		int i;
		printk("bit_wth %d\n", p_pulse_width);
		printk("pluse_wth: ");
		for (i = 0; i < p_pulse_width; i++)
			printk("%04d ", p_pulse_tab[i]);
		printk("\n");
#endif
		
		nvram_key_name = acts_get_nv_keycode(handle->cur_learn_keycode);
		if (nvram_key_name == NULL) 
			return;

#if DONT_SAVE_REPEAT_KEY			
		if (p_pulse_width < MIN_FIRST_CODE_NUM) {
			return ;
		}
#endif
		
		k_timer_stop(&handle->learn_mode_timer);
		k_timer_start(&handle->learn_mode_timer, K_SECONDS(CONFIG_IRC_LEARN_MODE_TIMEOUT), 0);
		handle->cur_learn_state = check_irc_wave(p_pulse_tab, p_pulse_width);
		
#if 1
		printk("bit_wth %d\n", p_pulse_width);
		printk("pluse_wth_new: ");
		for (i = 0; i < p_pulse_width; i++)
			printk("%04d ", p_pulse_tab[i]);
		printk("\n");
#endif
		
		/* save to nvram */
		flash_pulse_tab.flash_pulse_width_tab_len = p_pulse_width;
		memcpy(flash_pulse_tab.flash_pulse_width_tab, p_pulse_tab, p_pulse_width * sizeof(u16_t));
	#ifdef CONFIG_NVRAM_CONFIG
	if (nvram_config_set(nvram_key_name, &flash_pulse_tab, sizeof(struct flash_pulse_tab_t)) >= 0)
		SYS_LOG_INF("Storing flash_pulse_tab %s: len 0x%04x ", nvram_key_name, flash_pulse_tab.flash_pulse_width_tab_len);
	#endif
	
		printk("###handle->cur_learn_state %d\n", handle->cur_learn_state);
		/* notify app learn result */
		handle->has_set_learn_key = 0;
		if (handle->event_cb) {
			handle->event_cb(dev, ir_val);
		}
	} else {
		/* */
	}

}

static void __irc_learn_timeout_event(struct k_timer *timer)
{
	struct irc_handle *handle = CONTAINER_OF(timer, struct irc_handle, learn_mode_timer);
	
	printk("__irc_learn_timeout_event\n");
	
	/* disable rx learn when timeout */
	if (handle->sw_ir_learn_mode) 
		irc_device_disable_rx(handle);
	
	/* notify app learn timeout */
	if (handle->event_cb) {
		struct ir_input_value val;
		handle->cur_learn_state = IR_LEARN_TIMEOUT;
		handle->event_cb(handle->sw_irc_dev, (struct input_value*)&val);
	}

}
#endif


void *irc_device_open(char *dev_name)
{
	struct irc_handle *handle = &irc_handle_entity;

	handle->irc_dev = device_get_binding(dev_name);

	if (!handle->irc_dev) {
		SYS_LOG_ERR("cannot found irc dev");
		return NULL;
	}
#ifdef CONFIG_IRC_SW_RX
	handle->sw_irc_dev = device_get_binding(CONFIG_SW_IRC_ACTS_DEV_NAME);

	if (!handle->sw_irc_dev) {
		SYS_LOG_ERR("cannot found irc dev");
		return NULL;
	}
	
	k_timer_init(&handle->learn_mode_timer, __irc_learn_timeout_event, NULL);
#endif
	return handle;
}

void irc_device_enable_rx(void *handle, input_notify_t cb)
{
#ifdef CONFIG_IRC_RX
	struct irc_handle *irc = (struct irc_handle *)handle;

	input_dev_enable(irc->irc_dev);
	input_dev_register_notify(irc->irc_dev, cb);
#endif
	
#ifdef CONFIG_IRC_SW_RX
	struct irc_handle *irc = (struct irc_handle *)handle;

	input_dev_enable(irc->sw_irc_dev);
	irc->event_cb = cb;
	input_dev_register_notify(irc->sw_irc_dev, irc_hal_event_handle);
	irc->sw_ir_learn_mode = 1;
	irc->cur_learn_keycode = 0;
	irc->has_set_learn_key = 0;
	SYS_LOG_INF("irc_device_enable_rx sw");
	k_timer_start(&irc->learn_mode_timer, K_SECONDS(CONFIG_IRC_LEARN_MODE_TIMEOUT), 0);
#endif
}

void irc_device_disable_rx(void *handle)
{
#ifdef CONFIG_IRC_RX
	struct irc_handle *irc = (struct irc_handle *)handle;

	input_dev_disable(irc->irc_dev);
	input_dev_unregister_notify(irc->irc_dev, NULL);
#endif
	
#ifdef CONFIG_IRC_SW_RX
	struct irc_handle *irc = (struct irc_handle *)handle;

	input_dev_disable(irc->sw_irc_dev);
	input_dev_unregister_notify(irc->sw_irc_dev, NULL);
	irc->sw_ir_learn_mode = 0;
	irc->cur_learn_keycode = 0;
	irc->has_set_learn_key = 0;
	SYS_LOG_INF("irc_device_disable_rx sw");
#endif

}

void irc_device_send_key(void *handle, u32_t key_code, u8_t key_state)
{
	struct irc_handle *irc = (struct irc_handle *)handle;
	struct input_value val;
	
	val.type = EV_KEY;
	val.code = key_code;
	val.value = key_state;
	
	irc_output_key(irc->irc_dev, &val);
	irc->state = key_state;
}

void irc_device_send_learn_key(void *handle, u32_t key_code, u8_t key_state)
{
#ifdef CONFIG_IRC_SW_RX
	struct irc_handle *irc = (struct irc_handle *)handle;
	struct ir_input_value ir_val;

	char * nvram_key_name = acts_get_nv_keycode(key_code);

	if (nvram_key_name == NULL) 
			return ;
	
	ir_val.type = IR_FORMAT_PULSE_TAB;
	ir_val.code = key_code;
	ir_val.value = key_state;
	
	if (irc->sw_ir_learn_mode) {
		if (key_state == IR_KEY_DOWN) {
			if (irc->has_set_learn_key == 0) {
				k_timer_stop(&irc->learn_mode_timer);
				k_timer_start(&irc->learn_mode_timer, K_SECONDS(CONFIG_IRC_LEARN_MODE_TIMEOUT), 0);
			}
			irc->has_set_learn_key = 1;
			irc->cur_learn_keycode = key_code;
			irc->cur_learn_state = IR_LEARN_READY;
			irc->event_cb(irc->sw_irc_dev, (struct input_value*)&ir_val);
			printk("set key 0x%x as learn_key\n",key_code);
		}
		return ;
	}
	
	if (key_state == IR_KEY_DOWN) {
#ifdef CONFIG_NVRAM_CONFIG
		if (nvram_config_get(nvram_key_name, &flash_pulse_tab, sizeof(struct flash_pulse_tab_t)) >= 0)
			SYS_LOG_INF("geting %s flash_pulse_tab: len 0x%04x ", nvram_key_name, flash_pulse_tab.flash_pulse_width_tab_len);
#endif
		ir_val.pluse_tab = flash_pulse_tab.flash_pulse_width_tab;
		ir_val.pluse_tab_len = flash_pulse_tab.flash_pulse_width_tab_len;
	}

	irc_output_key(irc->sw_irc_dev, (struct input_value*)&ir_val);
	irc->state = key_state;
#endif
}

void irc_device_exit_learn_mode(void *handle)
{
#ifdef CONFIG_IRC_SW_RX
	struct irc_handle *irc = (struct irc_handle *)handle;

	/* exit to learn mode when other key press up */
	if (irc->sw_ir_learn_mode) {
		irc_device_disable_rx(handle);
		k_timer_stop(&irc->learn_mode_timer);
		/* notify app learn timeout */
		if (irc->event_cb) {
			struct ir_input_value val;
			irc->cur_learn_state = IR_LEARN_TIMEOUT;
			irc->event_cb(irc->sw_irc_dev, (struct input_value*)&val);
		}
		printk("exit learn mode when non learn key press up\n");
	}	
#endif
}

u8_t irc_device_state_get(void *handle)
{
	struct irc_handle *irc = (struct irc_handle *)handle;

	return irc->state;
}

void irc_device_close(void *handle)
{

}
