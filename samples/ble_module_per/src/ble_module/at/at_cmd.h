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


struct device;

typedef int (*at_cmd_exec_t)(void);
typedef int (*at_cmd_get_t)(void);
typedef int (*at_cmd_set_t)(const char *p_para);

struct at_cmd {
	const char *at_cmd_name;
	at_cmd_set_t set_cb;
	at_cmd_get_t get_cb;
	at_cmd_exec_t exec_cb;
};

extern char print_buf[];

extern int snprintf_rom(char * s, size_t len, const char * format, ...);

extern void at_cmd_port_print(const char *str);

#define AT_CMD_DBG(fmt, ...) \
	do { \
		snprintf_rom(print_buf, 256, fmt, ##__VA_ARGS__); \
		at_cmd_port_print(print_buf);\
	} while(0);

void at_cmd_init(void);
	
void enter_into_at_cmd_mode(void);
	
void exit_out_at_cmd_mode(void);

int at_cmd_exec(char *line);

void at_cmd_line_queue_init(void);

void at_cmd_response_ok(void);

void at_cmd_response_error(void);

void at_cmd_response(const char *str);
	
bool is_hex_string(const char *str);

bool is_decimal_string(const char *str);
	
int char2hex(const char *c, u8_t *x);

#ifdef __cplusplus
}
#endif

#endif /* _AT_CMD_H_ */
