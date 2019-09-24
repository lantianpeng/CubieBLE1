/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "pwm_acts.h"

static u8_t pwm_chan_pol[5];

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct pwm_acts_config *)(dev)->config->config_info)
#define PWM_CHAN(base, chan_id)							\
	(((struct acts_pwm_chan *)(base)) + chan_id)

static int pwm_acts_clk_enable_new(const struct pwm_acts_config *cfg, u32_t chan, bool enable)
{
	unsigned int key;
	int clock_id;
	int clock_id_chan_0;

	/* pwm4 depend on pwm3 clock */
	if (chan == 4)
		chan = 3;

	clock_id = cfg->clock_id + chan;
	clock_id_chan_0 = cfg->clock_id;

	key = irq_lock();

	if (enable) {
		/* all pwm depend on pwm0 clock */
		acts_clock_peripheral_enable(clock_id_chan_0);
		acts_clock_peripheral_enable(clock_id);
	} else {
		if (chan == 3) {
			if ((PWM_CHAN(cfg->base, chan)->pwm_ctrl == 0) && (PWM_CHAN(cfg->base, chan + 1)->pwm_ctrl == 0))
				acts_clock_peripheral_disable(clock_id);
		}

		if ((PWM_CHAN(cfg->base, 0)->pwm_ctrl == 0) && (PWM_CHAN(cfg->base, 1)->pwm_ctrl == 0)
			&& (PWM_CHAN(cfg->base, 2)->pwm_ctrl == 0) && (PWM_CHAN(cfg->base, 3)->pwm_ctrl == 0)
			&& (PWM_CHAN(cfg->base, 4)->pwm_ctrl == 0))
			acts_clock_peripheral_disable(clock_id_chan_0);
	}

	irq_unlock(key);

	return 0;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 *
 * return 0, or negative errno code
 */
int pwm_acts_pin_set_new(struct device *dev, u32_t chan,
			     u32_t period_cycles, u32_t pulse_cycles)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	struct acts_pwm_chan *pwm_chan;

	if (chan >= cfg->num_chans)
		return -EIO;

	pwm_chan = PWM_CHAN(cfg->base, chan);
	
	if (pulse_cycles != 0) {
		pwm_acts_clk_enable_new(cfg, chan, true);

		pwm_chan->pwm_dutymax = period_cycles;
		pwm_chan->pwm_duty = pulse_cycles;	
		pwm_chan->pwm_ctrl = PWM_CTRL_CHAN_EN | (pwm_chan_pol[chan] << 2) |
				 PWM_CTRL_MODE_SEL_FIXED;
	} else {
		pwm_chan->pwm_ctrl = 0;
		pwm_acts_clk_enable_new(cfg, chan, false);
	}

	return 0;
}

int pwm_acts_pol_set(struct device *dev, u32_t chan, u8_t pol)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);

	if (chan >= cfg->num_chans)
		return -EIO;

	pwm_chan_pol[chan] = pol;
	return 0;
}

const struct pwm_driver_api pwm_acts_drv_api_funcs_new = {
	.pin_set = pwm_acts_pin_set_new,
	.get_cycles_per_sec = pwm_acts_get_cycles_per_sec,
	.pol_set = pwm_acts_pol_set,
};

int pwm_acts_init_new(struct device *dev)
{
	const struct pwm_acts_config *cfg = DEV_CFG(dev);
	int i;

	/* reset pwm controller */
	acts_reset_peripheral(cfg->reset_id);

	/* init PWM clock */
	for (i = 0; i < cfg->num_chans; i++) {
		/* clock source: 32M, div= / 32, pwm clock is 1MHz */
		sys_write32(CMU_PWMCLK_CLKSEL_32M | CMU_PWMCLK_CLKDIV(5),
			cfg->pwmclk_reg + 4 * i);
	}

	return 0;
}

const struct pwm_acts_config pwm_acts_dev_cfg_1 = {
	.base = PWM_REG_BASE,
	.pwmclk_reg = CMU_PWM0CLK,
	.num_chans = 5,
	.clock_id = CLOCK_ID_PWM0,
	.reset_id = RESET_ID_PWM,
};

DEVICE_AND_API_INIT(pwm_acts_1, CONFIG_PWM_ACTS_DEV_NAME,
		    pwm_acts_init_new,
		    NULL, &pwm_acts_dev_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_acts_drv_api_funcs_new);

