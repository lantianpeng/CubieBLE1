/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <misc/printk.h>
#include "adc.h"
#include "soc_adc.h"
#include "app_batt.h"

#define SYS_LOG_DOMAIN "battery"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

/* battery level */
static u8_t batt_level = 100;
static u16_t batt_val = 3000;

#define VERY_LOW_BATT_VAL	    1800	/* mv */
#define LOW_BATT_VAL			2000	/* mv */
#define FULL_BATT_VAL			3250	/* mv */

#define CONFIG_RMC_ADC_BATTERY_TIMEOUT K_SECONDS(3)

static struct k_timer batt_timer;

void low_power_led_indication(void)
{
	/* TODO: handle low power led indication  */
}

void very_low_power_handler(void)
{
	/* TODO: handle very low power  */
}

void update_battry_value(void)
{
	struct device *adc_dev;
	struct adc_seq_table seq_tbl;
	struct adc_seq_entry seq_entrys;
	u8_t adcval[4];

	adc_dev = device_get_binding(CONFIG_ADC_0_NAME);
	if (!adc_dev) {
		SYS_LOG_ERR("cannot found adc dev");
		return;
	}

	seq_entrys.sampling_delay = 0;
	seq_entrys.buffer = (u8_t *)&adcval;
	seq_entrys.buffer_length = sizeof(adcval);
	seq_entrys.channel_id = ADC_ID_BATV;

	seq_tbl.entries = &seq_entrys;
	seq_tbl.num_entries = 1;

	//adc_enable(adc_dev);

	/* get adc value */
	adc_read(adc_dev, &seq_tbl);

	batt_val = (seq_entrys.buffer[0] + (seq_entrys.buffer[1] << 8)) * 3600 / 1024; /* mv */

	/* low power warning handler */
	if (batt_val <= LOW_BATT_VAL)
		low_power_led_indication();

	/* very low power handler */
	if (batt_val <= VERY_LOW_BATT_VAL)
		very_low_power_handler();

	if (batt_val <= VERY_LOW_BATT_VAL)
		batt_val = VERY_LOW_BATT_VAL;
	else if (batt_val >= FULL_BATT_VAL)
		batt_val = FULL_BATT_VAL;

	batt_level =  (batt_val - VERY_LOW_BATT_VAL) * 100 / (FULL_BATT_VAL - VERY_LOW_BATT_VAL);

	printk("batt_val %d mv, batt_level %d, adc_val %d\n", batt_val, batt_level, seq_entrys.buffer[0] + (seq_entrys.buffer[1] << 8));

	//adc_disable(adc_dev);
}

void adc_batt_read(u8_t *pLevel)
{
	*pLevel = batt_level;
}

u16_t adc_batt_val_read(void)
{
	return batt_val;
}

void batt_timer_handler(struct k_timer *dummy)
{
	update_battry_value();
}

void batt_manager_init(void)
{
	update_battry_value();
	k_timer_init(&batt_timer, batt_timer_handler, NULL);
	k_timer_start(&batt_timer, CONFIG_RMC_ADC_BATTERY_TIMEOUT, CONFIG_RMC_ADC_BATTERY_TIMEOUT);
}


