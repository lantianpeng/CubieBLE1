/** @file
 *  @brief Date Time characteristic type structure.
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#ifndef _DATE_TIME_H
#define _DATE_TIME_H

#include <zephyr/types.h>
#include "types_encode.h"

#pragma pack(1)
/* Date and Time structure. */
struct date_time {
    u16_t year;
    u8_t  month;
    u8_t  day;
    u8_t  hours;
    u8_t  minutes;
    u8_t  seconds;
};
#pragma pack()

static inline u8_t date_time_encode(const struct date_time *date_time, u8_t *encoded_data)
{
    u8_t len = u16_encode(date_time->year, encoded_data);
    
    encoded_data[len++] = date_time->month;
    encoded_data[len++] = date_time->day;
    encoded_data[len++] = date_time->hours;
    encoded_data[len++] = date_time->minutes;
    encoded_data[len++] = date_time->seconds;
    
    return len;
}

static inline u8_t date_time_decode(struct date_time *date_time, const u8_t *encoded_data)
{
    u8_t len = sizeof(u16_t);
    
    date_time->year    = u16_decode(encoded_data);
    date_time->month   = encoded_data[len++];
    date_time->day     = encoded_data[len++]; 
    date_time->hours   = encoded_data[len++];
    date_time->minutes = encoded_data[len++];
    date_time->seconds = encoded_data[len++];
    
    return len;
}

#endif    /* _DATE_TIME_H */

