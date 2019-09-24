/** @file
 *  @brief encode/decode data functions
 */
 
/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#ifndef _TYPES_ENCODING_H
#define _TYPES_ENCODING_H

#include <zephyr/types.h>

static inline u8_t u16_encode(u16_t value, u8_t *encoded_data)
{
    encoded_data[0] = (u8_t) ((value & 0x00FF) >> 0);
    encoded_data[1] = (u8_t) ((value & 0xFF00) >> 8);
    return sizeof(u16_t);
}

static inline u8_t u24_encode(u32_t value, u8_t *encoded_data)
{
    encoded_data[0] = (u8_t) ((value & 0x000000FF) >> 0);
    encoded_data[1] = (u8_t) ((value & 0x0000FF00) >> 8);
    encoded_data[2] = (u8_t) ((value & 0x00FF0000) >> 16);
    return 3;
}

static inline u8_t u32_encode(u32_t value, u8_t *encoded_data)
{
    encoded_data[0] = (u8_t) ((value & 0x000000FF) >> 0);
    encoded_data[1] = (u8_t) ((value & 0x0000FF00) >> 8);
    encoded_data[2] = (u8_t) ((value & 0x00FF0000) >> 16);
    encoded_data[3] = (u8_t) ((value & 0xFF000000) >> 24);
    return sizeof(u32_t);
}

static inline u8_t u48_encode(u64_t value, u8_t *encoded_data)
{
    encoded_data[0] = (u8_t) ((value & 0x0000000000FF) >> 0);
    encoded_data[1] = (u8_t) ((value & 0x00000000FF00) >> 8);
    encoded_data[2] = (u8_t) ((value & 0x000000FF0000) >> 16);
    encoded_data[3] = (u8_t) ((value & 0x0000FF000000) >> 24);
    encoded_data[4] = (u8_t) ((value & 0x00FF00000000) >> 32);
    encoded_data[5] = (u8_t) ((value & 0xFF0000000000) >> 40);
    return 6;
}

static inline u16_t u16_decode(const u8_t *encoded_data)
{
        return ( (((u16_t)((u8_t *)encoded_data)[0])) |
                 (((u16_t)((u8_t *)encoded_data)[1]) << 8 ));
}

static inline u16_t u16_big_decode(const u8_t *encoded_data)
{
        return ( (((u16_t)((u8_t *)encoded_data)[0]) << 8 ) |
                 (((u16_t)((u8_t *)encoded_data)[1])) );
}


static inline u32_t u24_decode(const u8_t *encoded_data)
{
    return ( (((u32_t)((u8_t *)encoded_data)[0]) << 0)  |
             (((u32_t)((u8_t *)encoded_data)[1]) << 8)  |
             (((u32_t)((u8_t *)encoded_data)[2]) << 16));
}


static inline u32_t u32_decode(const u8_t *encoded_data)
{
    return ( (((u32_t)((u8_t *)encoded_data)[0]) << 0)  |
             (((u32_t)((u8_t *)encoded_data)[1]) << 8)  |
             (((u32_t)((u8_t *)encoded_data)[2]) << 16) |
             (((u32_t)((u8_t *)encoded_data)[3]) << 24 ));
}

static inline u32_t u32_big_decode(const u8_t *encoded_data)
{
    return ( (((u32_t)((u8_t *)encoded_data)[0]) << 24) |
             (((u32_t)((u8_t *)encoded_data)[1]) << 16) |
             (((u32_t)((u8_t *)encoded_data)[2]) << 8)  |
             (((u32_t)((u8_t *)encoded_data)[3]) << 0) );
}

static inline u8_t u32_big_encode(u32_t value, u8_t *encoded_data)
{
    encoded_data[0] = (u8_t) ((value & 0xFF000000) >> 24);
    encoded_data[1] = (u8_t) ((value & 0x00FF0000) >> 16);
    encoded_data[2] = (u8_t) ((value & 0x0000FF00) >> 8);
    encoded_data[3] = (u8_t) ((value & 0x000000FF) >> 0);
    return sizeof(u32_t);
}

static inline u64_t u48_decode(const u8_t *encoded_data)
{
    return ( (((u64_t)((u8_t *)encoded_data)[0]) << 0)  |
             (((u64_t)((u8_t *)encoded_data)[1]) << 8)  |
             (((u64_t)((u8_t *)encoded_data)[2]) << 16) |
             (((u64_t)((u8_t *)encoded_data)[3]) << 24) |
             (((u64_t)((u8_t *)encoded_data)[4]) << 32) |
             (((u64_t)((u8_t *)encoded_data)[5]) << 40 ));
}

#endif    /* _TYPES_ENCODING_H */

