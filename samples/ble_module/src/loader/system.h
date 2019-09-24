/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

void SystemInit(void);
void debug_uart_init(void);
void debug_uart_put_char(char data);

#endif /* _SYSTEM_H_ */
