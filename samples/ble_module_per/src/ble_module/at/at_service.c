/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Shell framework services
 *
 * This module initiates shell.
 * Includes also adding basic framework commands to shell commands.
 */

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>

#define AT_PROMPT "at> "

int at_run(struct device *dev)
{
	ARG_UNUSED(dev);

	at_cmd_init(AT_PROMPT);
	return 0;
}

SYS_INIT(at_run, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

