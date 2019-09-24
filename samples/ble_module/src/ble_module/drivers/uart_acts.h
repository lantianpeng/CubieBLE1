/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for GPIO
 */

#ifndef __UART_ACTS_H__
#define __UART_ACTS_H__

#include <zephyr/types.h>
#include <uart.h>

#ifdef __cplusplus
extern "C" {
#endif


#define UART0_IRQn 15
#define UART1_IRQn 16
#define UART2_IRQn 17

#define	UART0_BASE	0x4000D000
#define	UART1_BASE	0x4000E000
#define	UART2_BASE	0x4000F000

#define UART_CTRL 0x0
#define UART_BAUDRATE	0x10


/* Device data structure */
struct uart_acts_dev_data_t {
	u32_t baud_rate;	        /**< Baud rate */
	u32_t clock_id;
	u32_t reset_id;
	u32_t ctl_reg_val;
	u32_t baud_reg_val;

	uart_irq_callback_t     cb;     /**< Callback function pointer */
};

#define DEV_DATA(dev) \
	((struct uart_acts_dev_data_t * )(dev)->driver_data)

#define DEV_CFG(dev) \
	((const struct uart_device_config *)(dev)->config->config_info)
		
int uart_acts_init(struct device *dev);
void uart_acts_isr(void *arg);

#ifdef __cplusplus
}
#endif

#endif	/* __UART_ACTS_H__ */
