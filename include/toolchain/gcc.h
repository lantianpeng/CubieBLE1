/*
 * Copyright (c) 2010-2014,2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GCC toolchain abstraction
 *
 * Macros to abstract compiler capabilities for GCC toolchain.
 */

#include <toolchain/common.h>

#define ALIAS_OF(of) __attribute__((alias(#of)))

#define FUNC_ALIAS(real_func, new_alias, return_type) \
	return_type new_alias() ALIAS_OF(real_func)

#define CODE_UNREACHABLE __builtin_unreachable()
#define FUNC_NORETURN    __attribute__((__noreturn__))

/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define _GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define ___in_section(a, b, c) \
	__attribute__((section("." _STRINGIFY(a)			\
				"." _STRINGIFY(b)			\
				"." _STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define __in_section_unique(seg) ___in_section(seg, __FILE__, __COUNTER__)


#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif
#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif
#define __may_alias     __attribute__((__may_alias__))
#ifndef __printf_like
#define __printf_like(f, a)   __attribute__((format (printf, f, a)))
#endif
#define __used		__attribute__((__used__))
#define __deprecated	__attribute__((deprecated))
#define ARG_UNUSED(x) (void)(x)

#define likely(x)   __builtin_expect((long)!!(x), 1L)
#define unlikely(x) __builtin_expect((long)!!(x), 0L)

#define popcount(x) __builtin_popcount(x)

#define __weak __attribute__((__weak__))
#define __unused __attribute__((__unused__))


#define compiler_barrier() do { \
	__asm__ volatile ("dmb"); \
} while ((0))
