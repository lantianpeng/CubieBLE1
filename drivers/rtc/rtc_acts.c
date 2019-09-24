/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <rtc.h>
#include <soc.h>
#include "rtc_acts.h"

#define RTC_HMS_H_SHIFT		(16)
#define RTC_HMS_H_MASK		(0x1f << RTC_HMS_H_SHIFT)
#define RTC_HMS_M_SHIFT		(8)
#define RTC_HMS_M_MASK		(0x3f << RTC_HMS_M_SHIFT)
#define RTC_HMS_S_SHIFT		(0)
#define RTC_HMS_S_MASK		(0x3f << RTC_HMS_S_SHIFT)

#define RTC_HMS_H(ymd)		(((ymd) & RTC_HMS_H_MASK) >> RTC_HMS_H_SHIFT)
#define RTC_HMS_M(ymd)		(((ymd) & RTC_HMS_M_MASK) >> RTC_HMS_M_SHIFT)
#define RTC_HMS_S(ymd)		(((ymd) & RTC_HMS_S_MASK) >> RTC_HMS_S_SHIFT)
#define RTC_HMS_VAL(h, m, s)	(((h) << RTC_HMS_H_SHIFT) | ((m) << RTC_HMS_M_SHIFT) | ((s) << RTC_HMS_S_SHIFT))

extern void rtc_acts_enable(struct device *dev);
extern void rtc_acts_disable(struct device *dev);
extern uint32_t rtc_acts_read(struct device *dev);
extern int rtc_acts_set_config(struct device *dev, struct rtc_config *cfg);
extern uint32_t rtc_acts_get_pending_int(struct device *dev);
extern void rtc_acts_set_alarm_interrupt(struct acts_rtc_controller *base, bool enable);

struct acts_rtc_data rtc_acts_ddata;

const struct acts_rtc_config rtc_acts_cdata = {
	.base = (struct acts_rtc_controller *)RTC_REG_BASE,
	.irq_config = rtc_acts_irq_config,
};

int rtc_acts_set_alarm_time_new(struct acts_rtc_data *rtcd, struct rtc_time *tm)
{
	struct acts_rtc_controller *base = rtcd->base;

	/* disable alarm interrupt */
	rtc_acts_set_alarm_interrupt(base, false);

	base->hms_alarm = RTC_HMS_VAL(tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* enable alarm interrupt */
	rtc_acts_set_alarm_interrupt(base, true);

	return 0;
}

int rtc_acts_set_alarm_new(struct device *dev, const uint32_t alarm_val)
{
	struct acts_rtc_data *rtcd = dev->driver_data;
	struct rtc_time tm;

	rtc_time_to_tm(alarm_val, &tm);

	return rtc_acts_set_alarm_time_new(rtcd, &tm);
}

const struct rtc_driver_api rtc_acts_driver_api_new = {
	.enable = rtc_acts_enable,
	.disable = rtc_acts_disable,
	.read = rtc_acts_read,
	.set_config = rtc_acts_set_config,
	.set_alarm = rtc_acts_set_alarm_new,
	.get_pending_int = rtc_acts_get_pending_int,
};

DEVICE_AND_API_INIT(rtc_acts, CONFIG_RTC_0_NAME, rtc_acts_init,
		    &rtc_acts_ddata, &rtc_acts_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &rtc_acts_driver_api_new);

void rtc_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_RTC, CONFIG_RTC_0_IRQ_PRI,
		    rtc_acts_isr, DEVICE_GET(rtc_acts), 0);
	irq_enable(IRQ_ID_RTC);
}
