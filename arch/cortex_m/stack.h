/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-M CPUs
 *
 * Stack helper functions.
 */

#ifndef _ARM_CORTEXM_STACK__H_
#define _ARM_CORTEXM_STACK__H_


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern void * _interrupt_stack;
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXM_STACK__H_ */
