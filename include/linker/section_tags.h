/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _section_tags__h_
#define _section_tags__h_

#include <toolchain.h>

#if !defined(_ASMLANGUAGE)

#define __noinit		__in_section_unique(NOINIT)
#define __irq_vector_table	_GENERIC_SECTION(IRQ_VECTOR_TABLE)
#define __sw_isr_table		_GENERIC_SECTION(SW_ISR_TABLE)

#define __init_once_text		__in_section_unique(INIT_ONCE_TEXT)
#define __init_once_data		__in_section_unique(INIT_ONCE_DATA)
#define __data_overlay		  __in_section_unique(DATA_OVERLAY)

#ifdef CONFIG_SPI0_XIP
#define __ramfunc _GENERIC_SECTION(RAMFUNC)
#else
#define __ramfunc
#endif

#define NO_INLINE __attribute__ ( (noinline) )

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
