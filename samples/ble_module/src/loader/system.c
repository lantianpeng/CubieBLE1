/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "atb110x.h"
#include "string.h"

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

#define BROM_GLOBAL_ADDR 0x20009f98
#define BROM_GL0BAL_LEN  0x10
#define BROM_GLOBAL_RELOCATION_ADDR 0x2000bf98

void SystemInit(void)
{
	/* disable watchdog */
	WD->CTL = 0;

	/* If NOT enable cache, mv BROM_GLOBAL_ADDR to tail of 48K ram to load app image larger than 0x9f98 */
	memcpy((void *)BROM_GLOBAL_RELOCATION_ADDR, (void *)BROM_GLOBAL_ADDR, BROM_GL0BAL_LEN);
	RTC_BAK->BAK0 = BROM_GLOBAL_RELOCATION_ADDR;

	/* enable uart0 for debug */
	debug_uart_init();
}
