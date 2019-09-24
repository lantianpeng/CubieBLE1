/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_adfu.h"

#define UART_ADFU_RETRY_TIMES 1

#define CMU_DEVRST 0x40002010
#define RESET_ID_SPICACHE 1

__attribute__((section(".ramfunc"))) void check_uart_adfu(void)
{
	int ret = -1;
	int retry_times = UART_ADFU_RETRY_TIMES;

	while (retry_times != 0) {
		if (uart_connect()) {
			if (uart_open(UART_PROTOCOL_TYPE_ADFU) == UART_PROTOCOL_OK)
				ret = 0;
				break;
		}
		retry_times--;
	}

	if (ret != 0)
		return;

#ifdef CONFIG_SPI0_XIP
	/* reset spicache */
	*(volatile unsigned int *)CMU_DEVRST |= (1<<RESET_ID_SPICACHE);
	*(volatile unsigned int *)CMU_DEVRST &= ~(1<<RESET_ID_SPICACHE);
#endif
	/* debug_uart_put_char('A');
	 * debug_uart_put_char('D');
	 * debug_uart_put_char('F');
	 * debug_uart_put_char('U');
	 */
	uart_adfu_server();
	uart_adfu_close();

	while (1)
		;
}
