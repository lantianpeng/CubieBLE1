/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for PWM
 */

#ifndef __PWM_ACTS_H__
#define __PWM_ACTS_H__

#include <zephyr/types.h>
#include <pwm.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CMU_PWMCLK_CLKSEL_MASK		(0x1 << 3)
#define CMU_PWMCLK_CLKSEL_32M		(0x0 << 3)
#define CMU_PWMCLK_CLKSEL_32K		(0x1 << 3)
#define CMU_PWMCLK_CLKDIV_MASK		(0x7 << 0)
#define CMU_PWMCLK_CLKDIV_SHIFT		0
#define CMU_PWMCLK_CLKDIV(x)		((x) << 0)

#define PWM_CTRL_CHAN_EN		(0x1 << 3)
#define PWM_CTRL_POL_SEL_MASK		(0x1 << 2)
#define PWM_CTRL_POL_SEL_ACTIVE_LOW	(0x0 << 2)
#define PWM_CTRL_POL_SEL_ACTIVE_HIGH	(0x1 << 2)
#define PWM_CTRL_MODE_SEL_MASK		(0x3 << 0)
#define PWM_CTRL_MODE_SEL_FIXED		(0x0 << 0)
#define PWM_CTRL_MODE_SEL_BREATH	(0x1 << 0)
#define PWM_CTRL_MODE_SEL_PROGRAMABLE	(0x2 << 0)

/* dma global registers */
struct acts_pwm_chan {
	volatile u32_t pwm_ctrl;
	volatile u32_t pwm_qhl;
	volatile u32_t pwm_dutymax;
	volatile u32_t pwm_duty;
};

struct pwm_acts_config {
	u32_t	base;
	u32_t	pwmclk_reg;
	u32_t	num_chans;
	u8_t	clock_id;
	u8_t	reset_id;
};

int pwm_acts_clk_enable(const struct pwm_acts_config *cfg, u32_t chan, bool enable);
int pwm_acts_get_cycles_per_sec(struct device *dev, u32_t chan, u64_t *cycles);
int pwm_acts_init(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif	/* __PWM_ACTS_H__ */
