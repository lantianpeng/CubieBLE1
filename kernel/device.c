/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "errno.h"
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <soc.h>

#define SYS_LOG_DOMAIN "device"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

static struct device *config_levels[] = {
	Image$$RW_IRAM_DEVICE_PRE_KERNEL_1$$Base,
	Image$$RW_IRAM_DEVICE_PRE_KERNEL_2$$Base,
	Image$$RW_IRAM_DEVICE_POST_KERNEL$$Base,
	Image$$RW_IRAM_DEVICE_APPLICATION$$Base,
	/* End marker */
	Image$$RW_IRAM_DEVICE_APPLICATION$$Limit,
};


/**
 * @brief Execute all the device initialization functions at a given level
 *
 * @details Invokes the initialization routine for each device object
 * created by the DEVICE_INIT() macro using the specified level.
 * The linker script places the device objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
void _sys_device_do_config_level(int level)
{
	struct device *info;

	for (info = config_levels[level]; info < config_levels[level+1];
								info++) {
		struct device_config *device = info->config;

		device->init(info);
	}
}

struct device *device_get_binding(const char *name)
{
	struct device *info;

	for (info = Image$$RW_IRAM_DEVICE_PRE_KERNEL_1$$Base;
				info != Image$$RW_IRAM_DEVICE_APPLICATION$$Limit; info++) {
		if (info->driver_api && !strcmp(name, info->config->name))
			return info;
	}

	return NULL;
}

void set_devices_state(u32_t state)
{
	struct device *info;

	for (info = Image$$RW_IRAM_DEVICE_PRE_KERNEL_1$$Base;
				info != Image$$RW_IRAM_DEVICE_APPLICATION$$Limit; info++) {
		if (info->config->device_pm_control != device_pm_control_nop) {
			SYS_LOG_DBG("device_name:%s", info->config->name);
			SYS_LOG_DBG(",%p", info->config->device_pm_control);
			device_set_power_state(info, state);
		}
	}
}

/*
 * void dump_all_devices(void)
 * {
 *	int i;
 *	struct device *info;
 *
 *	SYS_LOG_INF("All devices:");
 *	for (info = Image$$RW_IRAM_DEVICE_PRE_KERNEL_1$$Base, i = 0;
 *				info != Image$$RW_IRAM_DEVICE_APPLICATION$$Limit; info++, i++) {
 *					SYS_LOG_INF("device[%d]_name:%s", i, info->config->name);
 *					if (info->config->device_pm_control != device_pm_control_nop)
 *						SYS_LOG_INF(",%p", info->config->device_pm_control);
 *					else
 *						SYS_LOG_INF(", nop");
 *	}
 * }
 */

#define DEVICE_NUM_MAX 64
__attribute__((used, section(".device_BUSY")))  u8_t device_busy[(DEVICE_NUM_MAX + 8 - 1)/8];

void device_busy_set_new(struct device *busy_dev)
{
	atomic_set_bit((atomic_t *) device_busy,
				 (busy_dev - Image$$RW_IRAM_DEVICE_PRE_KERNEL_1$$Base));
}

void device_busy_clear_new(struct device *busy_dev)
{
	atomic_clear_bit((atomic_t *) device_busy,
				 (busy_dev - Image$$RW_IRAM_DEVICE_PRE_KERNEL_1$$Base));
}

FUNCTION_PATCH_REGISTER(device_busy_set, device_busy_set_new, device_busy_set);
FUNCTION_PATCH_REGISTER(device_busy_clear, device_busy_clear_new, device_busy_clear);

int device_any_busy_check(void)
{
	int i = 0;

	for (i = 0; i < DEVICE_NUM_MAX; i++) {
		if (atomic_test_bit((const atomic_t *)device_busy, i))
		return -EBUSY;
	}

	return 0;
}
