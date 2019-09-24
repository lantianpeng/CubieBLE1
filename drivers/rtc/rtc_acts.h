/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for adc key
 */

#ifndef __RTC_ACTS_H__
#define __RTC_ACTS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct acts_rtc_controller {
	volatile u32_t ctrl;
	volatile u32_t hms_alarm;
	volatile u32_t hms;
	volatile u32_t ymd;
	volatile u32_t access;
};

struct acts_rtc_data {
	struct acts_rtc_controller *base;
	void (*alarm_cb_fn)(struct device *dev);
	bool alarm_en;
};

struct acts_rtc_config {
	struct acts_rtc_controller *base;
	void (*irq_config)(void);
};

extern const struct rtc_driver_api rtc_acts_driver_api;

void rtc_acts_isr(struct device *dev);
int rtc_acts_init(struct device *dev);
void rtc_acts_irq_config(void);

#ifdef __cplusplus
}
#endif

#endif	/* __RTC_ACTS_H__ */
