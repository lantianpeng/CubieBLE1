/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2C master driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include "i2c_acts.h"

#ifdef CONFIG_I2C_0
static void i2c_acts_config_func_0(struct device *dev);

static const struct acts_i2c_config acts_i2c_config_0 = {
	.base = (struct i2c_acts_controller *)I2C0_REG_BASE,
	.irq_config_func = i2c_acts_config_func_0,
	.pclk_freq = 32000000,
	.clock_id = CLOCK_ID_I2C0,
	.reset_id = RESET_ID_I2C0,
};

static struct acts_i2c_data acts_i2c_data_0;

DEVICE_DEFINE(i2c_0, CONFIG_I2C_0_NAME, &i2c_acts_init,
	      i2c_device_ctrl, &acts_i2c_data_0, &acts_i2c_config_0,
	      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
	      &i2c_acts_driver_api);

static void i2c_acts_config_func_0(struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_I2C0, CONFIG_I2C_0_IRQ_PRI,
		    i2c_acts_isr, DEVICE_GET(i2c_0), 0);

	irq_enable(IRQ_ID_I2C0);
}
#endif

#ifdef CONFIG_I2C_1
static void i2c_acts_config_func_1(struct device *dev);

static const struct acts_i2c_config acts_i2c_config_1 = {
	.base = (struct i2c_acts_controller *)I2C1_REG_BASE,
	.irq_config_func = i2c_acts_config_func_1,
	.pclk_freq = 32000000,
	.clock_id = CLOCK_ID_I2C1,
	.reset_id = RESET_ID_I2C1,
};

static struct acts_i2c_data acts_i2c_data_1;

DEVICE_DEFINE(i2c_1, CONFIG_I2C_1_NAME, &i2c_acts_init,
	      i2c_device_ctrl, &acts_i2c_data_1, &acts_i2c_config_1,
	      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
	      &i2c_acts_driver_api);

static void i2c_acts_config_func_1(struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_I2C1, CONFIG_I2C_1_IRQ_PRI,
		    i2c_acts_isr, DEVICE_GET(i2c_1), 0);

	irq_enable(IRQ_ID_I2C1);
}
#endif

