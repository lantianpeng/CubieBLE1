/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "uart_acts.h"

int uart_acts_suspend_device(struct device *dev)
{
	struct uart_acts_dev_data_t *dev_data = DEV_DATA(dev);
	
	dev_data->ctl_reg_val = sys_read32(DEV_CFG(dev)->regs + UART_CTRL);
	dev_data->baud_reg_val = sys_read32(DEV_CFG(dev)->regs + UART_BAUDRATE);
	acts_clock_peripheral_disable(dev_data->clock_id);
	
	return 0;
}

int uart_acts_resume_device_from_suspend(struct device *dev)
{
	struct uart_acts_dev_data_t *dev_data = DEV_DATA(dev);
	
	acts_reset_peripheral(dev_data->reset_id);
	acts_clock_peripheral_enable(dev_data->clock_id);

	sys_write32(dev_data->baud_reg_val, DEV_CFG(dev)->regs + UART_BAUDRATE);
	sys_write32(dev_data->ctl_reg_val , DEV_CFG(dev)->regs + UART_CTRL);

	return 0;
}

static int uart_device_ctrl(struct device *dev, u32_t ctrl_command,
			   void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return uart_acts_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return uart_acts_resume_device_from_suspend(dev);
		}
	}

	return 0;
}

#ifdef CONFIG_UART_0
/* Forward declare function */
static void uart_acts_irq_config_0(struct device *port);

static const struct uart_device_config uart_acts_dev_cfg_0 = {
	.base = (u8_t *)UART0_BASE,
	.irq_config_func = uart_acts_irq_config_0,
};

static struct uart_acts_dev_data_t uart_acts_dev_data_0 = {
	.baud_rate = 115200,
	.clock_id = 5,
	.reset_id = 5,
};

DEVICE_DEFINE(uart_acts_0, "UART_0", &uart_acts_init,
		      uart_device_ctrl, &uart_acts_dev_data_0, &uart_acts_dev_cfg_0, PRE_KERNEL_1, 
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static __init_once_text void uart_acts_irq_config_0(struct device *port)
{
	IRQ_CONNECT(UART0_IRQn,
		    0,
		    uart_acts_isr, DEVICE_GET(uart_acts_0),
		    0);
	irq_enable(UART0_IRQn);
}
#endif

#ifdef CONFIG_UART_1

/* Forward declare function */
static void uart_acts_irq_config_1(struct device *port);

static const struct uart_device_config uart_acts_dev_cfg_1 = {
	.base = (u8_t *)UART1_BASE,
	.irq_config_func = uart_acts_irq_config_1,
};

static struct uart_acts_dev_data_t uart_acts_dev_data_1 = {
	.baud_rate = 9600,
	.clock_id = 6,
	.reset_id = 6,
};

DEVICE_DEFINE(uart_acts_1, "UART_1", &uart_acts_init,
		      uart_device_ctrl, &uart_acts_dev_data_1, &uart_acts_dev_cfg_1, PRE_KERNEL_1, 
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void uart_acts_irq_config_1(struct device *port)
{
	IRQ_CONNECT(UART1_IRQn,
		    0,
		    uart_acts_isr, DEVICE_GET(uart_acts_1),
		    0);
	irq_enable(UART1_IRQn);
}
#endif

#ifdef CONFIG_UART_2
/* Forward declare function */
static void uart_acts_irq_config_2(struct device *port);

static const struct uart_device_config uart_acts_dev_cfg_2 = {
	.base = (u8_t *)UART2_BASE,
	.irq_config_func = uart_acts_irq_config_2,
};

static struct uart_acts_dev_data_t uart_acts_dev_data_2= {
	.baud_rate = 9600,
	.clock_id = 7,
	.reset_id = 7,
};

DEVICE_DEFINE(uart_acts_2, "UART_2", &uart_acts_init,
	    uart_device_ctrl, &uart_acts_dev_data_2, &uart_acts_dev_cfg_2, PRE_KERNEL_1, 
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void uart_acts_irq_config_2(struct device *port)
{
	IRQ_CONNECT(UART2_IRQn,
		    2,
		    uart_acts_isr, DEVICE_GET(uart_acts_2),
		    0);
	irq_enable(UART2_IRQn);
}
#endif
