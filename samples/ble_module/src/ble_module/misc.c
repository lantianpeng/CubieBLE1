/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>
#include <zephyr.h>
#include <stdlib.h>
#include <misc/printk.h>
#include <bluetooth/hci.h>
#include <misc.h>
#include <msg_manager.h>

void send_async_msg(u8_t msg_type)
{
	struct app_msg  msg = {0};
	msg.type = msg_type;
	send_msg(&msg, K_NO_WAIT);
}

void bubble_sort(int A[], int n)
{
	int i, j, temp;

    for (j = 0; j < n - 1; j++) {
        for (i = 0; i < n - 1 - j; i++) {
            if (A[i] < A[i + 1]) {
				temp = A[i];
				A[i] = A[i + 1];
				A[i + 1] = temp;
            }
        }
    }
}

int average(s8_t array[], char num)
{
    int i, sum = 0;
	printk("array[%d] = {", num);
    for(i = 0; i < num; i++) {
		printk(" %d,", array[i]);
        sum += array[i];
    }
	printk("}\n");
    return sum/num;
}

bool is_hex_string(char *str)
{
    int i, len = strlen(str);

    if (!len)
        return false;

    for (i = 0; i < len; i++) {
        if (!((str[i] >= '0' && str[i] <= '9') ||
			(str[i] >= 'a' && str[i] <= 'f') ||
			(str[i] >= 'A' && str[i] <= 'F')))
            return false;
    }

    return true;
}

bool is_decimal_string(char *str)
{
    int i, len = strlen(str);

    if (!len)
        return false;

    for (i = 0; i < len; i++) {
        if (!(str[i] >= '0' && str[i] <= '9'))
            return false;
    }

    return true;
}

int at_addr_le_to_str(const bt_addr_le_t *addr, char *str,
		size_t len)
{
	return snprintk(str, len, "%02X%02X%02X%02X%02X%02X",
		addr->a.val[5], addr->a.val[4], addr->a.val[3],
		addr->a.val[2], addr->a.val[1], addr->a.val[0]);
}

int at_addr_le_to_str2(const bt_addr_le_t *addr, char *str,
		size_t len)
{
	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
		addr->a.val[5], addr->a.val[4], addr->a.val[3],
		addr->a.val[2], addr->a.val[1], addr->a.val[0]);
}

void at_str_to_le_addr(char *bt_addr_str, int len, bt_addr_le_t *addr, 
		char *str)
{
	addr->type = BT_ADDR_LE_PUBLIC;
	snprintf_rom(bt_addr_str, len, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
		str[0], str[1],
		str[2], str[3],
		str[4], str[5],
		str[6], str[7],
		str[8], str[9],
		str[10], str[11]);
	str2bt_addr(bt_addr_str, &addr->a);
}

void show_data(u8_t *data, u16_t len)
{
	if (len) {
		int i;
		printk("#: ");
		for (i = 0; i < len; i++) {
			printk("%02x ", ((u8_t *)data)[i]);
		}
		printk("\n");
	}
}