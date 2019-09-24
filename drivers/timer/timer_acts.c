/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief timer driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <irq.h>
#include <soc_clock.h>
#include <soc_irq.h>
#include <timer.h>

#define CMU_TIMER2CLK             (CMU_DIGITAL_BASE+0X0044)
#define CMU_TIMER3CLK             (CMU_DIGITAL_BASE+0X0048)
                                  
#define CMU_TIMERCLK_TCSS         4
#define CMU_TIMERCLK_DIV_E        3
#define CMU_TIMERCLK_DIV_SHIFT    0
#define CMU_TIMERCLK_DIV_MASK     (0XF<<0)

#define TIMER_CLK_SEL_32M		  (0x0 << CMU_TIMERCLK_TCSS)
#define TIMER_CLK_SEL_32K		  (0x1 << CMU_TIMERCLK_TCSS)
#define TIMER_CLK_DIV(x)	      ((x & CMU_TIMERCLK_DIV_MASK) << CMU_TIMERCLK_DIV_SHIFT)

#define TIMER1_BASE               0x40004140
#define TIMER2_BASE               0x40004180
#define TIMER3_BASE               0x400041c0

#define TIMER_CTRL_OFFSET         0x00
#define TIMER_VAL_OFFSET          0x04
#define TIMER_CNT_OFFSET          0x08

#define TIMER_CTRL_ENABLE_MSK     (0x1 << 5)
#define TIMER_CTRL_RELOAD_MSK     (0x1 << 2)
#define TIMER_CTRL_TICKINT_MSK    (0x1 << 1)
#define TIMER_CTRL_COUNTFLAG_MSK  (0x1)

/**
 * @brief driver data
 */
struct acts_timer_data {
	/* user ISR cb */
	timer_irq_callback_t cb;
	/* user_data */
	void *user_data;
};

typedef void (*timer_config_irq_t)(struct device *dev);

struct acts_timer_config {
    u32_t base;
    u32_t clk;
    u32_t clk_id;
    u32_t clk_reg;
    u32_t div;
    u32_t irq_num;
    s32_t duration;
    s32_t period;
    timer_config_irq_t config_func;
};

u32_t _ms_to_cycle(struct device *dev, s32_t ms)
{
    const struct acts_timer_config *info = dev->config->config_info;
    u32_t clk;
    
    if (info->clk == TIMER_CLK_SEL_32M) {
        clk = 32000000;
    } else {
        clk = 32000;
    }

    clk = clk / (1 << info->div);
    
    u32_t timer_clock_cycle_per_ms = clk /(MSEC_PER_SEC);

    return  (u32_t)ms * timer_clock_cycle_per_ms;
}

static void timer_reload(struct device *dev, u32_t cnt)
{
    const struct acts_timer_config *info = dev->config->config_info;
    u32_t ctrl;

    /* write to val register */
    sys_write32(cnt, info->base + TIMER_VAL_OFFSET); 

    /* enable counter, interrupt */
    ctrl = TIMER_CTRL_ENABLE_MSK | TIMER_CTRL_TICKINT_MSK | TIMER_CTRL_RELOAD_MSK;

    /* countflag is cleared by this read */
    sys_read32(info->base + TIMER_CTRL_OFFSET);

    /* write to val register */
    sys_write32(cnt, info->base + TIMER_VAL_OFFSET); 

    /* write to control register */
    sys_write32(ctrl, info->base + TIMER_CTRL_OFFSET);  
}

/**
 * @brief timer start
 *
 * @param dev Device struct
 * @param duration  Initial timer duration (in milliseconds).
 * @param period    Timer period (in milliseconds).
 *
 * @return 0 if successful, failed otherwise
 */
int timer_acts_start(struct device *dev, s32_t duration, s32_t period)
{
    struct acts_timer_config *info = (struct acts_timer_config *)dev->config->config_info;
    volatile u32_t period_in_cycle, duration_in_cycle;
    u32_t key;

    info->duration = duration;
    info->period = period;

    duration_in_cycle = _ms_to_cycle(dev, duration);

    key = irq_lock();

    /* reload */
    timer_reload(dev, duration_in_cycle);

    irq_unlock(key);

    return 0;
}

/**
 * @brief stop timer
 *
 * @param dev Device struct
 *
 * @return 0 if successful, failed otherwise
 */
int timer_acts_stop(struct device *dev)
{
    const struct acts_timer_config *info = dev->config->config_info;
    u32_t key;

    ARG_UNUSED(dev);

    key = irq_lock();

    /* disable counter, interrupt and reload */
    sys_write32(0, info->base + TIMER_CTRL_OFFSET);  

    irq_unlock(key);

    return 0;
}

/**
 * @brief timer interrupt callback
 */
void timer_acts_isr(void *arg)
{
    struct device *dev = arg;
    struct acts_timer_config *info = (struct acts_timer_config *)dev->config->config_info;
    struct acts_timer_data *data = dev->driver_data;
    volatile u32_t period_in_cycle;
    u32_t val;

    period_in_cycle = _ms_to_cycle(dev, info->period);

    /* clear IRQ pending */
    val = sys_read32(info->base + TIMER_CTRL_OFFSET);
    sys_write32(val | TIMER_CTRL_COUNTFLAG_MSK, info->base + TIMER_CTRL_OFFSET);

    if (info->period) {
        /* reload */
        timer_reload(dev, period_in_cycle);
    } else {
		/* timer disable */
		sys_write32(0, info->base + TIMER_CTRL_OFFSET); 
	}

    if (data->cb) {
        data->cb(dev);
    }
}

/** Set the callback function */
void timer_acts_callback_set(struct device *dev, timer_irq_callback_t cb)
{
	struct acts_timer_data *data = dev->driver_data;

	data->cb = cb;
}

void timer_acts_user_data_set(struct device *dev, void *user_data)
{
	struct acts_timer_data *data = dev->driver_data;

	data->user_data = user_data;
}

void *timer_acts_user_data_get(struct device *dev)
{
	struct acts_timer_data *data = dev->driver_data;

	return data->user_data;
}


const struct timer_driver_api timer_acts_drv_api = {
	.irq_callback_set = timer_acts_callback_set,
	.start = timer_acts_start,
	.stop = timer_acts_stop,
	.user_data_set = timer_acts_user_data_set,
	.user_data_get = timer_acts_user_data_get,
};

/**
 * @brief Initialization function of timer
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
int timer_acts_init(struct device *dev)
{
    const struct acts_timer_config *info = dev->config->config_info;

    /* enable counter, interrupt and set clock src to system clock */
    u32_t ctrl = TIMER_CTRL_ENABLE_MSK | TIMER_CTRL_TICKINT_MSK;

    ARG_UNUSED(dev);

    /* enable timer1 clock */
    acts_clock_peripheral_enable(info->clk_id);
	
    /* timer1 clock source:32M, Div:1 */
    sys_write32(info->clk | info->div, info->clk_reg);    

    info->config_func(dev);

    /* write to control register */
    sys_write32(ctrl, info->base + TIMER_CTRL_OFFSET);    

    return 0;
}

void timer_acts_config_irq(struct device *dev);

struct acts_timer_config acts_timer_config_info1 = {
    .base = TIMER1_BASE,
    .clk_id = CLOCK_ID_TIMER1,
    .clk_reg = CMU_TIMER1CLK,
    .clk = TIMER_CLK_SEL_32M,
    .irq_num = IRQ_ID_TIMER1,
    .div = TIMER_CLK_DIV(0),
    .config_func = timer_acts_config_irq,
};

struct acts_timer_data acts_timer_data0;

DEVICE_AND_API_INIT(timer_acts, CONFIG_TIMER1_ACTS_DRV_NAME,
		    timer_acts_init, &acts_timer_data0, &acts_timer_config_info1,
		    POST_KERNEL, CONFIG_TIMER_ACTS_INIT_PRIORITY,
		    &timer_acts_drv_api);

void timer_acts_config_irq(struct device *dev)
{
	const struct acts_timer_config *info = dev->config->config_info;

	IRQ_CONNECT(info->irq_num, 1, timer_acts_isr,
		    DEVICE_GET(timer_acts), 0);
	irq_enable(info->irq_num);
}

