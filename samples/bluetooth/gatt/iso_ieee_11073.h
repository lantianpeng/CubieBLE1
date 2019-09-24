/** @file
*  @brief  Formats define in iso_ieee_11073-20601-2016.
*/

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#ifndef _ISO_IEEE_11073_H
#define _ISO_IEEE_11073_H
#include <zephyr/types.h>

#define IEEE_11073_SFLOAT_NAN      0x07FF
#define IEEE_11073_SFLOAT_NRES    0x0800
#define IEEE_11073_SFLOAT_POSITIVE_INFINITY    0x07FE
#define IEEE_11073_SFLOAT_NEGATIVE_INFINITY    0x0802
#define IEEE_11073_SFLOAT_RESERVED    0x0801

/**@brief FLOAT format (IEEE-11073 32-bit FLOAT, defined as a 32-bit value  
 *         with a 24-bit mantissa and an 8-bit exponent
 *  The number represented is mantissa*(10**exponent) */
typedef struct ieee_float_type {
	s8_t  exponent;       /* base 10 exponent */
	s32_t mantissa;       /* mantissa, should be using only 24 bits */
} ieee_float;

/**@brief SFLOAT format (IEEE-11073 16-bit SFLOAT, defined as a 16-bit value  
 *         with a signed 4-bit  exponent follow by a signed 12-bit mantissa.
 *  The number represented is mantissa*(10**exponent) */
typedef struct ieee_sfloat_type {
	s8_t  exponent;       /* base 10 exponent, should be using only 4 bits */
	s16_t mantissa;       /* mantissa, should be using only 12 bits */
} ieee_sfloat;


#endif    /* _ISO_IEEE_11073_H */

