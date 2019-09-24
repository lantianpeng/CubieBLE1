/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA storage
 */

#if CONFIG_OTA_WITH_APP

#include "errno.h"
#include <kernel.h>
#include <init.h>
#include <device.h>
#include <flash.h>
#include <string.h>

#include "ota_storage.h"

#define SYS_LOG_DOMAIN "ota"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARNING
#include <logging/sys_log.h>

static void unlock_flash(struct device *dev)
{
#ifdef CONFIG_XSPI_NOR_LOCK
	static bool flash_unlock_flag;

	if (flash_unlock_flag == true)
		return;

	flash_unlock(dev, 0, (size_t)-1);
	flash_unlock_flag = true;
#endif
}

int ota_storage_write(struct device *dev, uint32_t addr,
				      const void *buf, int32_t len)
{
	SYS_LOG_DBG("write addr 0x%x len 0x%x", addr, len);
	if (!dev)
		return 0;

	unlock_flash(dev);
	return flash_write(dev, (off_t)addr, buf, (size_t)len);

}

int ota_storage_read(struct device *dev, uint32_t addr,
				     void *buf, int32_t len)
{
	SYS_LOG_DBG("read addr 0x%x len 0x%x", addr, len);
	if (!dev)
		return 0;

	return flash_read(dev, (off_t)addr, buf, (size_t)len);

}

int ota_storage_erase(struct device *dev, uint32_t addr,
				      int32_t size)
{
	SYS_LOG_DBG("erase addr 0x%x size 0x%x", addr, size);
	if (!dev)
		return 0;

	unlock_flash(dev);
	return flash_erase(dev, (off_t)addr, (size_t)size);

}

struct device *ota_storage_init(void)
{
	struct device *dev;

	SYS_LOG_DBG("init ota storage");

	dev = device_get_binding(CONFIG_NVRAM_STORAGE_DEV_NAME);
	if (!dev) {
		SYS_LOG_ERR("cannot found device \'%s\'",
			    CONFIG_NVRAM_STORAGE_DEV_NAME);
		return NULL;
	}

	return dev;
}
#endif
