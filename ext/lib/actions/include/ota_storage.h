/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ota storage interface
 */

#ifndef __OTA_STORAGE_H__
#define __OTA_STORAGE_H__

#include <kernel.h>
#include <device.h>


#define OTA_ERASE_ALIGN_SIZE		0x10000

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) ((x) & (uint32_t)(-((int32_t)(a))))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(x, a) (-((int32_t)((uint32_t)(-((int32_t)(x))) & (uint32_t)(-((int32_t)(a))))))
#endif

/**
 * @brief OTA storage initialization
 *
 *
 * @return storage device which will be operated later
 */

struct device *ota_storage_init(void);

/**
 *  @brief  Read data from ota storage
 *
 *  @param  dev  : ota storage dev
 *  @param  addr : start addr to read
 *  @param  buf  : Buffer to store read data
 *  @param  len  : Number of bytes to read.
 *
 *  @return  0 on success, negative errno code on fail.
 */

int ota_storage_read(struct device *dev, uint32_t addr,
			      void *buf, int32_t len);

/**
 *  @brief  write data into ota storage.
 *
 *
 *  @param  dev    	: ota storage dev
 *  @param  addr 		: starting addr for the write
 *  @param  buf			: data to write
 *  @param  len    	: Number of bytes to write
 *
 *  @return  0 on success, negative errno code on fail.
 */

int ota_storage_write(struct device *dev, uint32_t addr,
			       const void *buf, int32_t len);

/**
 *  @brief  Erase part or all of ota storage
 *
 *
 *  @param  dev			: ota storage dev
 *  @param  addr		: erase area starting addr
 *  @param  size		: size of area to be erased
 *
 *  @return  0 on success, negative errno code on fail.
 */

int ota_storage_erase(struct device *dev, uint32_t addr, int32_t size);

#endif/* #define __OTA_STORAGE_H__ */
