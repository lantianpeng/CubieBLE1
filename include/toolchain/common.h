/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common toolchain abstraction
 *
 * Macros to abstract compiler capabilities (common to all toolchains).
 */

/* Abstract use of extern keyword for compatibility between C and C++ */
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

/* Use TASK_ENTRY_CPP to tag task entry points defined in C++ files. */

#ifdef __cplusplus
#define TASK_ENTRY_CPP  extern "C"
#endif

/*
 * Generate a reference to an external symbol.
 * The reference indicates to the linker that the symbol is required
 * by the module containing the reference and should be included
 * in the image if the module is in the image.
 *
 * The assembler directive ".set" is used to define a local symbol.
 * No memory is allocated, and the local symbol does not appear in
 * the symbol table.
 */

#ifdef _ASMLANGUAGE
  #define REQUIRES(sym) .set sym ## _Requires, sym
#else
  #define REQUIRES(sym) __asm__ (".set " # sym "_Requires, " # sym "\n\t");
#endif

#ifdef _ASMLANGUAGE
  #define SECTION .section
#endif


/* force inlining a function */

#if !defined(_ASMLANGUAGE)
  #define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#define _STRINGIFY(x) #x
#define STRINGIFY(s) _STRINGIFY(s)

/* Indicate that an array will be used for stack space. */

#if !defined(_ASMLANGUAGE)
  /* don't use this anymore, use K_DECLARE_STACK instead. Remove for 1.11 */
  #define __stack __aligned(STACK_ALIGN) __DEPRECATED_MACRO
#endif

/* concatenate the values of the arguments into one */
#define _DO_CONCAT(x, y) x ## y
#define _CONCAT(x, y) _DO_CONCAT(x, y)

#ifndef BUILD_ASSERT
/* compile-time assertion that makes the build fail */
#define BUILD_ASSERT(EXPR) typedef char __build_assert_failure[(EXPR) ? 1 : -1]
#endif
#ifndef BUILD_ASSERT_MSG
/* build assertion with message -- common implementation swallows message. */
#define BUILD_ASSERT_MSG(EXPR, MSG) BUILD_ASSERT(EXPR)
#endif
