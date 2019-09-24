/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC Keyboard driver for Actions SoC
 */

#include <errno.h>
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

static struct acts_adckey_data adckey_acts_ddata;

static const struct adckey_map adckey_acts_keymaps[] = {
	{10,  KEY_NEXTSONG},
	{450, KEY_PREVIOUSSONG},
	{750, KEY_EXIT},
};

static const struct acts_adckey_config adckey_acts_cdata = {
	.adc_name = "ADC_0",
	.adc_chan = ADC_ID_CH1,

	.poll_interval_ms = 50,
	.sample_filter_dep = 3,

	.key_cnt = ARRAY_SIZE(adckey_acts_keymaps),
	.key_maps = &adckey_acts_keymaps[0],
};

DEVICE_AND_API_INIT(adckey_acts, CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME,
		    adckey_acts_init,
		    &adckey_acts_ddata, &adckey_acts_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adckey_acts_driver_api);
