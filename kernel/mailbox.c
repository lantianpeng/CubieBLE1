/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Mailboxes.
 */

#include <kernel.h>
#include <init.h>

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)


int init_mbox_module(struct device *dev);
SYS_INIT(init_mbox_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

int init_static_pools(struct device *unused);
SYS_INIT(init_static_pools, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS > 0 */


