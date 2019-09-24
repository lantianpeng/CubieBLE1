/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <uart.h>
#include <misc/printk.h>

static struct device *uart_pipe_dev;

int uart_pipe_send(const u8_t *data, int len)
{
	while (len--)  {
		uart_poll_out(uart_pipe_dev, *data++);
	}

	return 0;
}

void uart_pipe_irq_callback_set(uart_irq_callback_t cb)
{
	u8_t c;

	if(!uart_pipe_dev) 
		return;

	uart_irq_rx_disable(uart_pipe_dev);
	
	/* Drain the fifo */
	while (uart_irq_rx_ready(uart_pipe_dev)) {
		uart_fifo_read(uart_pipe_dev, &c, 1);
	}

	uart_irq_callback_set(uart_pipe_dev, cb);

	uart_irq_rx_enable(uart_pipe_dev);	
}

void uart_pipe_unregister(void)
{
	if (uart_pipe_dev) {
		uart_irq_rx_disable(uart_pipe_dev);
	}
}

void uart_pipe_init(void)
{
	uart_pipe_dev = device_get_binding(CONFIG_UART_PIPE_ON_DEV_NAME);
	if (!uart_pipe_dev) {
		uart_irq_tx_disable(uart_pipe_dev);
	}
}

