/*
 * Copyright (c) 2011-2012, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UART-driven at
 *
 *
 * Serial at driver.
 * Hooks into the printk and fputc (for printf) modules. Poll driven.
 */

#include <kernel.h>
#include <arch/cpu.h>

#include <stdio.h>
#include <zephyr/types.h>
#include "errno.h"
#include <ctype.h>

#include <device.h>
#include <init.h>

#include <uart.h>
#include <console/console.h>
#include <console/uart_console.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <atomic.h>
#include <misc/printk.h>
#include <string.h>

#include "errno.h"
#include "app_gpio.h"

static struct device *uart_at_dev;

static int at_out(int c)
{
	if(!uart_at_dev) return -1;
	
	uart_poll_out(uart_at_dev, c);

	return c;
}

/**
  * @brief  print string to at cmd port
  * @param  string
  * @retval None
  */
void at_cmd_port_print(const char *str)
{
	int i;
	int len = strlen(str);
#ifdef MODULE_CTS_PIN
	gpio_output_wakeup_host(true);
#endif
	for(i = 0; i< len; i++) {
		at_out(str[i]);
	}
#ifdef MODULE_CTS_PIN
	gpio_output_wakeup_host(false);
#endif
}

static struct k_fifo *avail_queue;
static struct k_fifo *lines_queue;

static int read_uart(struct device *uart, u8_t *buf, unsigned int size)
{
	int rx;

	rx = uart_fifo_read(uart, buf, size);
	if (rx < 0) {
		/* Overrun issue. Stop the UART */
		uart_irq_rx_disable(uart);

		return -EIO;
	}

	return rx;
}

static u8_t cur;
static struct console_input *cmd;
void uart_at_isr(struct device *unused)
{
	ARG_UNUSED(unused);

	while (uart_irq_update(uart_at_dev) &&
	       uart_irq_is_pending(uart_at_dev)) {
		u8_t byte;
		int rx;
					 
		if (!uart_irq_rx_ready(uart_at_dev)) {
			continue;
		}

		/* Character(s) have been received */

		rx = read_uart(uart_at_dev, &byte, 1);
		if (rx < 0) {
			return;
		}

		if (!cmd) {
			cmd = k_fifo_get(avail_queue, K_NO_WAIT);
			if (!cmd) {
				return;
			}
		}

		/* Handle special control characters */
		if (!isprint(byte)) {
			switch (byte) {
			case '\r':
				cmd->line[cur] = '\0';
				cur = 0;
				k_fifo_put(lines_queue, cmd);
				cmd = NULL;
				break;
			default:
				break;
			}

			continue;
		}

		/* Ignore characters if there's no more buffer space */
		if (cur < sizeof(cmd->line) - 1) {
			cmd->line[cur++] = byte;
		}
	}
}

static void at_input_init(uart_irq_callback_t cb)
{
	u8_t c;

	uart_irq_rx_disable(uart_at_dev);
	uart_irq_tx_disable(uart_at_dev);

	uart_irq_callback_set(uart_at_dev, cb);

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart_at_dev)) {
		uart_fifo_read(uart_at_dev, &c, 1);
	}

	uart_irq_rx_enable(uart_at_dev);
}

void uart_at_register_input(struct k_fifo *avail, struct k_fifo *lines)
{
	avail_queue = avail;
	lines_queue = lines;
	cur = 0;
	cmd = NULL;
	
	uart_at_dev = device_get_binding(CONFIG_UART_AT_ON_DEV_NAME);

	at_input_init(uart_at_isr);
}


void uart_at_unregister_input(struct k_fifo *avail, struct k_fifo *lines)
{
	uart_at_dev = device_get_binding(CONFIG_UART_AT_ON_DEV_NAME);

	if (uart_at_dev)
		uart_irq_rx_disable(uart_at_dev);
}

void uart_at_int(void)
{
	uart_at_dev = device_get_binding(CONFIG_UART_AT_ON_DEV_NAME);
}
