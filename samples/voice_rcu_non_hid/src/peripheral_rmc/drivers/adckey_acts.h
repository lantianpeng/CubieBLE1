/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for adc key
 */

#ifndef __ADCKEY_ACTS_H__
#define __ADCKEY_ACTS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif


extern int adckey_acts_init(struct device *dev);

struct adckey_map {
	u16_t adc_div;
	u16_t adc_val;
	u16_t key_code;
};

struct acts_adckey_data {
	struct k_timer timer;

	struct device *adc_dev;
	struct adc_seq_table seq_tbl;
	struct adc_seq_entry seq_entrys;
	u8_t adc_buf[4];

	int scan_count;
	u16_t prev_keycode;
	u16_t prev_stable_keycode;

	input_notify_t notify;
};

struct acts_adckey_config {
	char *adc_name;
	u16_t adc_chan;

	u16_t poll_interval_ms;
	u16_t sample_filter_dep;

	u16_t key_cnt;
	struct adckey_map *key_maps;
	
	void (*irq_config)(void);
};

extern const struct input_dev_driver_api adckey_acts_driver_api;

#ifdef __cplusplus
}
#endif

#endif	/* __ADCKEY_ACTS_H__ */
