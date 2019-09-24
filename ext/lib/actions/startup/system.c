/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************************************************/
/*!
 *  \brief  Initialize the system.
 *
 *  \return None.
 */
/*************************************************************************************************/
#include <kernel.h>
#include "soc.h"
#include "uart_adfu.h"

void debug_uart_init(void)
{
	sys_write32(sys_read32(CMU_DEVRST) & ~(0x1 << 5), CMU_DEVRST);
	sys_write32(sys_read32(CMU_DEVRST) | (0x1 << 5), CMU_DEVRST);

	sys_write32(sys_read32(CMU_DEVCLKEN) | (0x1 << 5), CMU_DEVCLKEN);

	/* uart0 tx mfp config*/
	sys_write32(sys_read32(GPIO_REG_CTL(GPIO_REG_BASE, 3)) & ~0xf, GPIO_REG_CTL(GPIO_REG_BASE, 3));
	sys_write32(sys_read32(GPIO_REG_CTL(GPIO_REG_BASE, 3)) | 0x3, GPIO_REG_CTL(GPIO_REG_BASE, 3));
	sys_write32(sys_read32(GPIO_REG_CTL(GPIO_REG_BASE, 3)) | (0x1 << 11), GPIO_REG_CTL(GPIO_REG_BASE, 3));

	/* uart0 config*/
	sys_write32(0x8b0000, UART0_BR);
	sys_write32(0xffffffff, UART0_STA);
	/* uart0 enable, 8-bit data width, 1-bit stop bit */
	sys_write32((1 << 30) | (1 << 23) | (1 << 15) | 0x3, UART0_CTL);
}


void debug_uart_put_char(char data)
{
	sys_write32(data, UART0_TXDAT);
	sys_write32(0x0a, UART0_TXDAT);
	sys_write32(0x0d, UART0_TXDAT);

	while ((sys_read32(UART0_STA) & (1 << 10)) == 0)
		;
}

__init_once_text void SystemInit(void)
{
	/* clear watchdog */
	sys_write32(0, WD_CTL);

#ifdef CONFIG_ENABLE_UART_ADFU
	check_uart_adfu();
#endif

	debug_uart_init();
}
