/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for I2C
 */

#ifndef __I2C_ACTS_H__
#define __I2C_ACTS_H__

#include <zephyr/types.h>
#include <i2c.h>

#ifdef __cplusplus
extern "C" {
#endif


enum i2c_state {
	STATE_INVALID,
	STATE_READ_DATA,
	STATE_WRITE_DATA,
	STATE_TRANSFER_OVER,
	STATE_TRANSFER_ERROR,
};

/* I2C controller */
struct i2c_acts_controller {
	volatile u32_t ctl;
	volatile u32_t clkdiv;
	volatile u32_t stat;
	volatile u32_t addr;
	volatile u32_t txdat;
	volatile u32_t rxdat;
	volatile u32_t cmd;
	volatile u32_t fifoctl;
	volatile u32_t fifostat;
	volatile u32_t datcnt;
	volatile u32_t rcnt;
};

struct acts_i2c_config {
	struct i2c_acts_controller *base;
	void (*irq_config_func)(struct device *dev);
	union dev_config default_cfg;
	u32_t pclk_freq;
	u16_t clock_id;
	u16_t reset_id;
};

/* Device run time data */
struct acts_i2c_data {
	struct k_mutex mutex;
	struct k_sem complete_sem;
	union dev_config mode_config;

	struct i2c_msg *cur_msg;
	u32_t msg_buf_ptr;
	enum i2c_state state;
	u32_t clk_freq;

	u32_t device_power_state;
};

extern struct i2c_driver_api i2c_acts_driver_api;

extern int i2c_acts_init(struct device *dev);
extern void i2c_acts_isr(void *arg);
extern int i2c_device_ctrl(struct device *dev, u32_t ctrl_command,
			   void *context);

#ifdef __cplusplus
}
#endif

#endif	/* __I2C_ACTS_H__ */
