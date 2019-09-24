/* bstream.h - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BSTREAM_H
#define BSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!
 * Macros for converting a little endian byte stream to integers, with increment.
 */
#define BSTREAM_TO_INT8(n, p)     {n = (int8_t)(*(p)++);}
#define BSTREAM_TO_UINT8(n, p)    {n = (u8_t)(*(p)++);}
#define BSTREAM_TO_UINT16(n, p)   {BYTES_TO_UINT16(n, p); p += 2;}
#define BSTREAM_TO_UINT24(n, p)   {BYTES_TO_UINT24(n, p); p += 3;}
#define BSTREAM_TO_UINT32(n, p)   {BYTES_TO_UINT32(n, p); p += 4;}
#define BSTREAM_TO_UINT40(n, p)   {BYTES_TO_UINT40(n, p); p += 5;}

/*!
 * Macros for converting integers to a little endian byte stream, with increment.
 */
#define UINT8_TO_BSTREAM(p, n)    {*(p)++ = (u8_t)(n);}
#define UINT16_TO_BSTREAM(p, n)   {*(p)++ = (u8_t)(n); *(p)++ = (u8_t)((n) >> 8);}
#define UINT24_TO_BSTREAM(p, n)   {*(p)++ = (u8_t)(n); *(p)++ = (u8_t)((n) >> 8); \
                                  *(p)++ = (u8_t)((n) >> 16);}
#define UINT32_TO_BSTREAM(p, n)   {*(p)++ = (u8_t)(n); *(p)++ = (u8_t)((n) >> 8); \
                                  *(p)++ = (u8_t)((n) >> 16); *(p)++ = (u8_t)((n) >> 24);}
#define UINT40_TO_BSTREAM(p, n)   {*(p)++ = (u8_t)(n); *(p)++ = (u8_t)((n) >> 8); \
                                  *(p)++ = (u8_t)((n) >> 16); *(p)++ = (u8_t)((n) >> 24); \
                                  *(p)++ = (u8_t)((n) >> 32);}

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

#ifdef __cplusplus
};
#endif

#endif /* BSTREAM_H */
