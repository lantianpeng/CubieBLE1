/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MISC_H
#define __MISC_H

#include <zephyr/types.h>
#include <bluetooth/hci.h>

void send_async_msg(u8_t msg_type);

void bubble_sort(int A[], int n);

int average(s8_t array[], char num);

bool is_hex_string(char *str);

bool is_decimal_string(char *str);

int at_addr_le_to_str(const bt_addr_le_t *addr, char *str,
		size_t len);

int at_addr_le_to_str2(const bt_addr_le_t *addr, char *str,
		size_t len);

void at_str_to_le_addr(char *bt_addr_str, int len, bt_addr_le_t *addr, 
		char *str);

void show_data(u8_t *data, u16_t len);

extern int str2bt_addr(const char *str, bt_addr_t *addr);

extern int snprintf_rom(char * s, size_t len, const char * format, ...);

#endif    /* _MISC_H */
