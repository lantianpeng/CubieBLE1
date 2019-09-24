/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 *
 * @brief Actions ADC header file
 */

#ifndef ACTS_ADC_H_
#define ACTS_ADC_H_

#include <zephyr/types.h>
#include <adc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct acts_adc_config {
	u32_t base;
	u8_t clock_id;
};
struct acts_adc_data {
	u32_t enable_cnt;
};

extern const struct adc_driver_api adc_acts_driver_api;
extern int adc_acts_init(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif  /*  ACTS_ADC_H_ */
