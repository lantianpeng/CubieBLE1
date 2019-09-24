/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for Actions SoC
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "gpio_acts.h"
/**
 * @brief driver data
 */


const struct acts_gpio_config acts_gpio_config_info0 = {
	.base = GPIO_REG_BASE,
	.irq_num = IRQ_ID_GPIO,
	.config_func = gpio_acts_config_irq,
};

struct acts_gpio_data acts_gpio_data0;

DEVICE_AND_API_INIT(gpio_acts, CONFIG_GPIO_ACTS_DRV_NAME,
		    gpio_acts_init, &acts_gpio_data0, &acts_gpio_config_info0,
		    PRE_KERNEL_1, CONFIG_GPIO_ACTS_INIT_PRIORITY,
		    &gpio_acts_drv_api);

void gpio_acts_config_irq(struct device *dev)
{
	const struct acts_gpio_config *info = dev->config->config_info;

	IRQ_CONNECT(IRQ_ID_GPIO, 1, gpio_acts_isr,
		    DEVICE_GET(gpio_acts), 0);
	irq_enable(info->irq_num);
}
