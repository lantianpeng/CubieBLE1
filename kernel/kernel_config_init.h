/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel initialization module
 *
 * This module contains routines that are used to initialize the kernel.
 */

#ifndef _kernel_config_init_h_
#define _kernel_config_init_h_


extern struct net_buf_pool Image$$RW_IRAM__net_buf_pool_area$$Base[];
extern struct net_buf_pool *_outer_net_buf_pool_list;

extern struct k_mem_pool *_outer_k_mem_pool_list_start;
extern struct k_mem_pool *_outer_k_mem_pool_list_end;

extern struct k_mem_pool Image$$RW_IRAM__mem_pool_area$$Base[];
extern struct k_mem_pool Image$$RW_IRAM_SHELL_CMD_START$$Base[];

extern struct k_mbox_async *async_msg;
extern struct k_stack *p_async_msg_free;

extern int (*pkernel_config_init)(void);

int main_thread_init(k_thread_stack_t stack, size_t stack_size, int prio);
int idle_thread_init(k_thread_stack_t stack, size_t stack_size, int prio);
int interrupt_stack_init(k_thread_stack_t stack, size_t stack_size);
int sys_work_queue_init(k_thread_stack_t stack, size_t stack_size, int prio);

#endif /* _kernel_config_init_h_ */
