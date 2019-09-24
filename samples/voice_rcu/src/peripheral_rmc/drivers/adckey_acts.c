/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC Keyboard driver for Actions SoC
 */

#include "errno.h"
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <adc.h>
#include <input_dev.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <soc.h>
#include "adckey_acts.h"
#include <misc/printk.h>
#include <system_app_batt.h>
#include <timer.h>

#define SYS_LOG_DOMAIN "ADCKEY"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_INPUT_DEV_LEVEL
#include <logging/sys_log.h>

 /************************************************************************************
 * BATADC( Connected IOVCC)
 * Input Voltage range set: 0~VBAT
 *     1 LSB = (3.6/(2^10)) * (Vref/1.2) , attention: Vref/1.2 is alawys 1 
 *     When the battery voltage equals V, the ADC data n = V / ((3.6/(2^10)) * (Vref/1.2))
 *     Where Vref is the value of the reference voltage actually value.
 *     For example: Vref=1.2V, 1LSB=3.515625mV, So the corresponding data from 0v to 0.003515625mV is 000h .
 *     V=3600/1024*DATA(mV) 
 *     ADC data = V / ((3.6/(2^10)) * (Vref/1.2)) = V / 0.00351562
 *     eg: V = 2.828V£¬ ADC data = 779 
 *     eg: V = 3.1V, ADC data = 854
 * Input Voltage range set: 0~1.2V
 * 	   1 LSB = (1.2/(2^10)) * (Vref/1.2) , attention: Vref/1.2 is alawys 1 
 * 	   When the battery voltage equals V, the ADC data n = V / ((1.2/(2^10)) * (Vref/1.2))
 * 	   Where Vref is the value of the reference voltage actually value.
*************************************************************************************/

/* KEY_RESERVED rom value is 0 */
#undef KEY_RESERVED
#define KEY_RESERVED    0

#define PMUADC_CTL0     0x0010
#define PMUADC_CTL1     0x0014
#define PMUADC_CTL2     0x0018
#define PMUADC_INT_CTL  0x001c
#define PMUADC_INT_PD   0x0020
#define WAKE_PD         0x0054

#define CONFIG_ADCKEY_TIMER_INTERVER       30
#define CONFIG_ADCKEY_SAMPLE_FILTER_DEP    2
#define CONFIG_ADCKEY_CH_NUM               2
#define CONFIG_ADCKEY_VAL_MARGIN(BAT_VAL)  (u16_t)(((float)BAT_VAL/3600)*60)
#define CONFIG_ADCKEY_POLL_NO_KEY_CNT      8

struct adckey_dev {
	struct device *dev;
	u16_t adc_chan;
	bool avalible;
};

struct adckey_dev adckey_devs[CONFIG_ADCKEY_CH_NUM];
struct device *adckey_timer;
bool adckey_timer_start_flag = false;

void adckey_timer_start(struct device *timer)
{
	if (!adckey_timer_start_flag) {
		adckey_timer_start_flag = true;
		timer_start(timer, 1, CONFIG_ADCKEY_TIMER_INTERVER);
	}
}

void adckey_timer_stop(struct device *timer)
{
	if (adckey_timer_start_flag) {
		adckey_timer_start_flag = false;
		timer_stop(timer);
	}
}

void update_adckey_map(u16_t bat_val, struct adckey_map *map, u16_t key_cnt)
{
	int i;
	for (i = 0; i < key_cnt; i++) {
		map[i].adc_val = (map[i].adc_div * bat_val) / 1000;
	}
}

/* adc_val -> key_code */
u16_t adckey_acts_get_keycode_new(const struct acts_adckey_config *cfg,
				     struct acts_adckey_data *adckey,
				     int adc_val)
{
	struct adckey_map *map = cfg->key_maps;
	int i;
	u16_t bat_val = adc_batt_val_read();
	update_adckey_map(bat_val, map, cfg->key_cnt);
	//printk("adc_val=%d, margin=%d, adc[0]=%d, adc[1]=%d, adc[2]=%d, adc[3]=%d, adc[4]=%d, adc[5]=%d\n", adc_val, CONFIG_ADCKEY_VAL_MARGIN(bat_val), 
	//		map[0].adc_val, map[1].adc_val, map[2].adc_val ,map[3].adc_val, map[4].adc_val, map[5].adc_val);

	for (i = 0; i < cfg->key_cnt; i++) {
		if (adc_val < (map[i].adc_val + CONFIG_ADCKEY_VAL_MARGIN(bat_val))) {
			//printk("adc_val=%d, map[i].adc_val=%d, map[i].key_code=%d\n", adc_val, map[i].adc_val, map[i].key_code);
			return map[i].key_code;
		}
	}

	return KEY_RESERVED;
}

void adckey_acts_report_key_new(const struct acts_adckey_config *cfg, struct acts_adckey_data *adckey,
				   u16_t adcval, int key_code, int value)
{
	struct input_value val;

//	struct adckey_map *map = cfg->key_maps;
	
	if (adckey->notify) {
		val.type = EV_KEY;
		val.code = key_code;
		val.value = value;
		adckey->notify(NULL, &val);
	}
}

void adckey_acts_poll_new(struct device *timer)
{
	int i, j;
	static u8_t no_key_cnt = 0;
	struct adckey_dev *adckeys = (struct adckey_dev *)timer_user_data_get(timer);
	
	for (i = 0; i < CONFIG_ADCKEY_CH_NUM; i++) {
		if (adckeys[i].avalible) {
			struct device *dev = adckeys[i].dev;
			const struct acts_adckey_config *cfg = dev->config->config_info;
			struct acts_adckey_data *adckey = dev->driver_data;			
			u16_t keycode, adcval;

			/* adc enable */
			adc_enable(adckey->adc_dev);
			
			/* get adc value */
			adc_read(adckey->adc_dev, &adckey->seq_tbl);

			adcval = sys_get_le16(adckey->seq_entrys.buffer);

			/* adc disable */
			adc_disable(adckey->adc_dev);
			
			/* get keycode */
			keycode = adckey_acts_get_keycode_new(cfg, adckey, adcval);
			if (keycode != KEY_RESERVED) {
				no_key_cnt = 0;
			}

			/* no key is pressed or releasing */
			if (keycode == KEY_RESERVED &&
				adckey->prev_stable_keycode == KEY_RESERVED) {
				if (++no_key_cnt >= CONFIG_ADCKEY_POLL_NO_KEY_CNT) {
					no_key_cnt = 0;
					for (j = 0; j < CONFIG_ADCKEY_CH_NUM; j++) {
						device_busy_clear(adckeys[j].dev);
					}
				}
				continue;				
			}

			if (keycode == adckey->prev_keycode) {
				adckey->scan_count++;
				if (adckey->scan_count == cfg->sample_filter_dep) {
					/* previous key is released? */
					if (adckey->prev_stable_keycode != KEY_RESERVED
						&& keycode != adckey->prev_stable_keycode) {
							adckey_acts_report_key_new(cfg, adckey, adcval, adckey->prev_stable_keycode, 0);
						}

					/* a new key press? */
					if (keycode != KEY_RESERVED) {
						adckey_acts_report_key_new(cfg, adckey, adcval, keycode, 1);
					}

					/* clear counter for new checking */
					adckey->prev_stable_keycode = keycode;
					adckey->scan_count = 0;
				}
			} else {
				/* new key pressed? */
				adckey->prev_keycode = keycode;
				adckey->scan_count = 1;
			}
		}
	}
}

static void add_adckey(struct device *dev, u16_t adc_chan)
{
	int i;
	struct adckey_dev *adckey = NULL;
	for (i = 0; i < CONFIG_ADCKEY_CH_NUM; i++) {
		if (!adckey_devs[i].avalible) {
			adckey = &adckey_devs[i];
			adckey->dev = dev;
			adckey->adc_chan = adc_chan;
			adckey->avalible = true;
			break;
		}
	}

	if (adckey) {
		timer_user_data_set(adckey_timer, adckey_devs);
		adckey_timer_start(adckey_timer);
	}
}

static void remove_adckey(struct device *dev)
{
	int i;
	for (i = 0; i < CONFIG_ADCKEY_CH_NUM; i++) {
		if (adckey_devs[i].dev == dev) {
			adckey_devs[i].avalible = false;
			break;
		}
	}
}

void adckey_acts_enable_new(struct device *dev)
{
	const struct acts_adckey_config *cfg = dev->config->config_info;
//	struct acts_adckey_data *adckey = dev->driver_data;

	if (cfg->adc_chan >= ADC_ID_CH0)
		add_adckey(dev, cfg->adc_chan);
}

void adckey_acts_disable_new(struct device *dev)
{
	struct acts_adckey_data *adckey = dev->driver_data;

	adc_disable(adckey->adc_dev);

	adckey->notify = NULL;
	
	remove_adckey(dev);
}

void adckey_acts_register_notify_new(struct device *dev, input_notify_t notify)
{
	struct acts_adckey_data *adckey = dev->driver_data;

	adckey->notify = notify;
}

void adckey_acts_unregister_notify_new(struct device *dev, input_notify_t notify)
{
	struct acts_adckey_data *adckey = dev->driver_data;

	adckey->notify = NULL;
}

int adckey_acts_suspend_device(struct device *dev)
{
	/* stop timer */
	adckey_timer_stop(adckey_timer);

	irq_enable(IRQ_ID_SARADC_WAKEUP);

	return 0;
}

int adckey_acts_resume_device_from_suspend(struct device *dev)
{	
	/* start timer */
	adckey_timer_start(adckey_timer);
	
	device_busy_set(dev);

	return 0;
}

static int adckey_device_ctrl(struct device *dev, u32_t ctrl_command,
			   void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return adckey_acts_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return adckey_acts_resume_device_from_suspend(dev);
		}
	}

	return 0;
}

const struct input_dev_driver_api adckey_acts_driver_api_new = {
	.enable = adckey_acts_enable_new,
	.disable = adckey_acts_disable_new,
	.register_notify = adckey_acts_register_notify_new,
	.unregister_notify = adckey_acts_unregister_notify_new,
};


int adckey_acts_init_new(struct device *dev)
{
    static bool adc_init = false;
	const struct acts_adckey_config *cfg = dev->config->config_info;
	struct acts_adckey_data *adckey = dev->driver_data;
	u32_t reg_val;

	adckey->adc_dev = device_get_binding(cfg->adc_name);
	if (!adckey->adc_dev) {
		SYS_LOG_ERR("cannot found adc dev %s\n", cfg->adc_name);
		return -ENODEV;
	}

	adckey->seq_entrys.sampling_delay = 0;
	adckey->seq_entrys.buffer = &adckey->adc_buf[0];
	adckey->seq_entrys.buffer_length = sizeof(adckey->adc_buf);
	adckey->seq_entrys.channel_id = cfg->adc_chan;

	adckey->seq_tbl.entries = &adckey->seq_entrys;
	adckey->seq_tbl.num_entries = 1;

	if (!adc_init) {
		adc_init = true;

		/* clear IRQ pending */
		sys_write32(0xffffffff, PMU_REG_BASE + PMUADC_INT_PD);
		
		/*clear SARADC WAKE IRQ pending */
		sys_write32(0x0C, PMU_REG_BASE + WAKE_PD);
	
		adckey_timer = device_get_binding(CONFIG_TIMER1_ACTS_DRV_NAME);
		if (!adckey_timer) {
			printk("Cannot find %s!\n", CONFIG_TIMER1_ACTS_DRV_NAME);
			return -1;
		}

		timer_set_callback(adckey_timer, adckey_acts_poll_new);
	}
	
	/* if adc wakeup is enable, adc must enable */
	reg_val = sys_read32(PMU_REG_BASE + PMUADC_CTL0);
	sys_write32(reg_val | (1 << cfg->adc_chan), PMU_REG_BASE + PMUADC_CTL0);	
	
	/* enable adc wakeup */
	reg_val = sys_read32(PMU_REG_BASE + PMUADC_CTL0);
	sys_write32(reg_val | (1 << (24 + cfg->adc_chan)), PMU_REG_BASE + PMUADC_CTL0);

	/* irq config, only adc ch0 */
	if (cfg->irq_config) {
		cfg->irq_config();	
	}

	return 0;
}

void adckey_acts_irq_config(void);

static struct acts_adckey_data adckey_0_acts_ddata;

static struct adckey_map adckey_0_acts_keymaps[] = {
	ADCKEY_0_MAPS
};

void adckey_acts_wakeup_isr(struct device *dev)
{
	u32_t reg_val;

	irq_disable(IRQ_ID_SARADC_WAKEUP);

    /* clear all interrupt pending */
	reg_val = sys_read32(PMU_REG_BASE + PMUADC_INT_PD);
	sys_write32(reg_val | 0xfc, PMU_REG_BASE + PMUADC_INT_PD);

    /* clear wakeup interrupt pending */
	reg_val = sys_read32(PMU_REG_BASE + WAKE_PD);
	sys_write32(reg_val | (1 << 3), PMU_REG_BASE + WAKE_PD);
}

static const struct acts_adckey_config adckey_0_acts_cdata = {
	.adc_name = "ADC_0",
	.adc_chan = ADC_ID_CH0,

	.poll_interval_ms = CONFIG_ADCKEY_TIMER_INTERVER,
	.sample_filter_dep = CONFIG_ADCKEY_SAMPLE_FILTER_DEP,

	.key_cnt = ARRAY_SIZE(adckey_0_acts_keymaps),
	.key_maps = &adckey_0_acts_keymaps[0],
	.irq_config = adckey_acts_irq_config,
};

DEVICE_DEFINE(adckey_0_acts, CONFIG_INPUT_DEV_ACTS_ADCKEY_0_NAME, 
		adckey_acts_init_new, adckey_device_ctrl, &adckey_0_acts_ddata, &adckey_0_acts_cdata, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &adckey_acts_driver_api_new);

void adckey_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_SARADC_WAKEUP, 0,
		    adckey_acts_wakeup_isr, NULL, 0);
	irq_enable(IRQ_ID_SARADC_WAKEUP);
}

//static struct acts_adckey_data adckey_1_acts_ddata;

//static struct adckey_map adckey_1_acts_keymaps[] = {
//	ADCKEY_1_MAPS
//};

//static const struct acts_adckey_config adckey_1_acts_cdata = {
//	.adc_name = "ADC_0",
//	.adc_chan = ADC_ID_CH1,

//	.poll_interval_ms = CONFIG_ADCKEY_TIMER_INTERVER,
//	.sample_filter_dep = CONFIG_ADCKEY_SAMPLE_FILTER_DEP,

//	.key_cnt = ARRAY_SIZE(adckey_1_acts_keymaps),
//	.key_maps = &adckey_1_acts_keymaps[0],
//};

//DEVICE_DEFINE(adckey_1_acts, CONFIG_INPUT_DEV_ACTS_ADCKEY_1_NAME, 
//		&adckey_acts_init_new, adckey_device_ctrl, &adckey_1_acts_ddata, &adckey_1_acts_cdata, POST_KERNEL,
//		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &adckey_acts_driver_api_new);
