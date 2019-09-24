/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_ACTIONS_BOARD_H_
#define	_ACTIONS_BOARD_H_

#ifndef _ASMLANGUAGE

extern int acts_init(struct device *arg);

extern uint8_t Image$$ER_IROM$$Base[];
#define REMAP_VECTOR_START (uint32_t)(Image$$ER_IROM$$Base)

extern void timer_irq_handler(void *unused);

extern void acts_set_xtal_32k(void);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_BOARD_H_	*/
