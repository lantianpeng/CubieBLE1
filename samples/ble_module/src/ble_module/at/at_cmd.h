/* at_cmd.h - at_cmd header */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AT_CMD_H_
#define _AT_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

struct device;

typedef int (*at_cmd_exec_t)(void);
typedef int (*at_cmd_get_t)(void);
typedef int (*at_cmd_set_t)(char *p_para);

struct at_cmd {
	const char *at_cmd_name;
	at_cmd_set_t set_cb;
	at_cmd_get_t get_cb;
	at_cmd_exec_t exec_cb;
};

extern char print_buf[];

extern int snprintf_rom(char * s, size_t len, const char * format, ...);

extern void at_cmd_port_print(const char *str);

void at_cmd_init(void);
	
void enter_into_at_cmd_mode(void);

int at_cmd_exec(char *line);

void at_cmd_line_queue_init(void);

void at_cmd_response_ok(void);

void at_cmd_response_error(void);

void at_cmd_response(const char *str);

void ble_state_update(u8_t state);

#ifdef __cplusplus
}
#endif

#endif /* _AT_CMD_H_ */
