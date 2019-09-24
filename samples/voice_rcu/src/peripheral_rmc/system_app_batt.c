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
#include "system_app_sm.h"

#include "adc.h"
#include "soc_adc.h"
#include "flash.h"
#include "msg_manager.h"

#define SYS_LOG_DOMAIN "battery"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

/* battery level */
static u8_t batt_level = 100;

/* use 3.0v as default */
static u16_t batt_val = 3000;

#define VERY_LOW_BATT_VAL	1800	/* mv */
#define LOW_BATT_VAL			2000	/* mv */
#define FULL_BATT_VAL			3250	/* mv */

static int dummy_nor_read(struct device *dev, int addr, void *data, size_t len) { return 0; };
static int dummy_nor_write(struct device *dev, int addr, const void *data, size_t len) { return 0; };
static int dummy_nor_erase(struct device *dev, int addr, size_t size) { return 0; };
static int dummy_nor_write_protection(struct device *dev, bool enable) { return 0; };

const struct flash_driver_api dummy_nor_api = {
	.read = dummy_nor_read,
	.write = dummy_nor_write,
	.erase = dummy_nor_erase,
	.write_protection = dummy_nor_write_protection,
};

void very_low_power_handler(void)
{
	struct device *dev;
	struct app_msg  msg = {0};

	dev = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (!dev) {
		SYS_LOG_ERR("cannot found device \'%s\'",
			    CONFIG_XSPI_NOR_ACTS_DEV_NAME);
		return;
	}

	SYS_LOG_INF("enter into very_low_power_handler");
	/* 1. disable Nor operation */
	dev->driver_api = &dummy_nor_api;

	/* 2. notify app to do something */
	msg.type = MSG_LOW_POWER;
	send_msg(&msg, K_MSEC(100));
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

		adc_enable(adc_dev);

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

		SYS_LOG_INF("adcval %d, %d", batt_val, batt_level);

		adc_disable(adc_dev);
}

void adc_batt_read(u8_t *pLevel)
{
	*pLevel = batt_level;
}

u16_t adc_batt_val_read(void)
{
	return batt_val;
}

void batt_manager_init(void)
{
	update_battry_value();
}
