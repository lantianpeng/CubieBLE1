/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "atb110x.h"

void debug_uart_init(void)
{
	CMU->DEVRST &= ~(0x1 << 5);
	CMU->DEVRST |= (0x1 << 5);

	CMU->DEVCLKEN |= (0x1 << 5);

	/* uart1 tx mfp config*/
	GPIO_MFP->IO_CTL[3] &= ~0xf;
	GPIO_MFP->IO_CTL[3] |= 0x3;
	GPIO_MFP->IO_CTL[3] |= (0x1 << 11);

	/* uart1 config*/
	UART0->BAUDDIV = 0x8b0000;
	UART0->STA = 0xffffffff;
	UART0->CTRL = (1 << 30) | (1 << 23) | (1 << 15) | 0x3;
}

void debug_uart_put_char(char data)
{
	UART0->TXDAT = data;
	UART0->TXDAT = 0x0a;
	UART0->TXDAT = 0x0d;

	while ((UART0->STA & UART_STATE_TXEMPTY) == 0)
		;
}

void SystemInit(void)
{
	WD->CTL = 0;
	debug_uart_init();
}
