/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief Kernel initialization module
 *
 * This module contains routines that are used to initialize the kernel.
 */

#include <kernel.h>
#include <ksched.h>
#include "kernel_config_init.h"

#define CONFIG_MAIN_THREAD_PRIORITY 0
#define CONFIG_SYSTEM_WORKQUEUE_PRIORITY -1

#define IDLE_STACK_SIZE CONFIG_IDLE_STACK_SIZE

#if CONFIG_MAIN_STACK_SIZE & (STACK_ALIGN - 1)
    #error "MAIN_STACK_SIZE must be a multiple of the stack alignment"
#endif

#if IDLE_STACK_SIZE & (STACK_ALIGN - 1)
    #error "IDLE_STACK_SIZE must be a multiple of the stack alignment"
#endif

#define MAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE

K_THREAD_STACK_DEFINE(_main_stack, MAIN_STACK_SIZE);
__attribute__((used, section(".ram_retention_data"))) char _idle_stack[IDLE_STACK_SIZE];

#if CONFIG_ISR_STACK_SIZE & (STACK_ALIGN - 1)
    #error "ISR_STACK_SIZE must be a multiple of the stack alignment"
#endif

K_THREAD_STACK_DEFINE(sys_work_q_stack, CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);


struct k_mbox_async {
	struct _thread_base thread;		/* dummy thread object */
	struct k_mbox_msg tx_msg;	/* transmit message descriptor */
};

/* array of asynchronous message descriptors */
static struct k_mbox_async __noinit async_msg_array[CONFIG_NUM_MBOX_ASYNC_MSGS];
/* stack of unused asynchronous message descriptors */
K_STACK_DEFINE(async_msg_free, CONFIG_NUM_MBOX_ASYNC_MSGS);

__init_once_text int kernel_config_init(void)
{
	int err;

	err = main_thread_init(_main_stack, MAIN_STACK_SIZE, CONFIG_MAIN_THREAD_PRIORITY);
	if (err)
		return err;

	err = idle_thread_init((k_thread_stack_t)_idle_stack, IDLE_STACK_SIZE, K_LOWEST_THREAD_PRIO);
	if (err)
		return err;

	err = interrupt_stack_init((k_thread_stack_t)&_interrupt_stack, CONFIG_ISR_STACK_SIZE);
	if (err)
		return err;

	err = sys_work_queue_init(sys_work_q_stack, K_THREAD_STACK_SIZEOF(sys_work_q_stack),
														CONFIG_SYSTEM_WORKQUEUE_PRIORITY);
	if (err)
		return err;

	_outer_net_buf_pool_list = Image$$RW_IRAM__net_buf_pool_area$$Base;

	_outer_k_mem_pool_list_start = Image$$RW_IRAM__mem_pool_area$$Base;
	_outer_k_mem_pool_list_end = Image$$RW_IRAM_SHELL_CMD_START$$Base;
	async_msg = async_msg_array;
	p_async_msg_free = &async_msg_free;
	return 0;
}
