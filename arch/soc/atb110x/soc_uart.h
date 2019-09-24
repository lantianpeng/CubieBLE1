/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ADC configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_UART_H_
#define	_ACTIONS_SOC_UART_H_

#include "soc_regs.h"

#define     UART0_CTL                                                       (UART0_REG_BASE+0x000)
#define     UART0_RXDAT                                                     (UART0_REG_BASE+0x004)
#define     UART0_TXDAT                                                     (UART0_REG_BASE+0x008)
#define     UART0_STA                                                       (UART0_REG_BASE+0x00C)
#define     UART0_BR                                                        (UART0_REG_BASE+0x010)

#define     UART1_CTL                                                       (UART1_REG_BASE+0x000)
#define     UART1_RXDAT                                                     (UART1_REG_BASE+0x004)
#define     UART1_TXDAT                                                     (UART1_REG_BASE+0x008)
#define     UART1_STA                                                       (UART1_REG_BASE+0x00C)
#define     UART1_BR                                                        (UART1_REG_BASE+0x010)

#define     UART2_CTL                                                       (UART2_REG_BASE+0x000)
#define     UART2_RXDAT                                                     (UART2_REG_BASE+0x004)
#define     UART2_TXDAT                                                     (UART2_REG_BASE+0x008)
#define     UART2_STA                                                       (UART2_REG_BASE+0x00C)
#define     UART2_BR                                                        (UART2_REG_BASE+0x010)

#ifndef _ASMLANGUAGE

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_UART_H_	*/
