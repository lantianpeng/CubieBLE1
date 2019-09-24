/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPINOR Flash driver for Actions SoC
 */
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "xspi_nor_acts.h"

#define JEDEC_ID_ATB1103_MGD	0x1364c8

/* system XIP spinor */
struct xspi_nor_info system_xspi_nor = {
	.spi = {
		.base = SPI0_REG_BASE,
		.spiclk_reg = CMU_SPI0CLK,
		.bus_width = 2,
		.delay_chain = 8,
		.flag = 0,

		.freq_khz = 2000,

		.clock_id = CLOCK_ID_SPI0,
		.reset_id = RESET_ID_SPI0,
		.dma_id = DMA_ID_SPI0,
		.dma_chan = 0,
	}
};

static int acts_xspi_nor_device_ctrl(struct device *dev, u32_t ctrl_command,
			   void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
#ifdef CONFIG_XSPI_NOR_SUSPEND_RESUME
			acts_xspi_nor_suspend(dev);
#endif
#ifndef CONFIG_SPI0_XIP
			acts_clock_peripheral_disable(CLOCK_ID_SPI0);
#endif
			return 0;
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
#ifndef CONFIG_SPI0_XIP
			acts_clock_peripheral_enable(CLOCK_ID_SPI0);
#endif
#ifdef CONFIG_XSPI_NOR_SUSPEND_RESUME
			acts_xspi_nor_resume(dev);
#endif
			return 0;
		}
	}

	return 0;
}

#ifdef CONFIG_XSPI_NOR_LOCK

/*
 * BP2 BP1 BP0 Blocks Addresses Density Portion
 * 0 0 0 NONE NONE NONE NONE
 * 0 0 1 Sector 0 to 125 000000H-07DFFFH 504KB Lower 126/128
 * 0 1 0 Sector 0 to 123 000000H-07BFFFH 496KB Lower 124/128
 * 0 1 1 Sector 0 to 119 000000H-077FFFH 480KB Lower 120/128
 * 1 0 0 Sector 0 to 111 000000H-06FFFFH 448KB Lower 112/128
 * 1 0 1 Sector 0 to 95  000000H-05FFFFH 384KB Lower 96/128
 * 1 1 0 Sector 0 to 63  000000H-03FFFFH 256KB Lower 64/128
 * 1 1 1 All             000000H-07FFFFH 512KB ALL
 */
#define	SR_BP0			4	/* Block protect 0 */
#define	SR_BP1			8	/* Block protect 1 */
#define	SR_BP2			0x10	/* Block protect 2 */

__ramfunc int xspi_nor_lock(struct device *dev, off_t offset, size_t len)
{
	u8_t status;
	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;

	if (sni->chipid == JEDEC_ID_ATB1103_MGD) {
		if (offset == 0) {
			/* lock all region*/
			status = SR_BP2 | SR_BP1 | SR_BP0;
			xspi_nor_write_data(sni, 0x1, 0, 0, &status, 1);
			/* erase 1st 64K block to test whether lock is valid
			 * xspi_nor_write_data(sni, 0xd8, 0, 3, NULL, 0);
			 */
		}
	}

	return 0;
}

__ramfunc int xspi_nor_unlock(struct device *dev, off_t offset, size_t len)
{
	u8_t status;
	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;

	if (sni->chipid == JEDEC_ID_ATB1103_MGD) {
		if (offset == 0) {
			/* unlock all region */
			status = 0;
			xspi_nor_write_data(sni, 0x1, 0, 0, &status, 1);
		} else if (offset == 0x70000) {
			/* unlock region over NVRAM_FACTORY_REGION_BASE_ADDR */
			status = SR_BP2;
			xspi_nor_write_data(sni, 0x1, 0, 0, &status, 1);
		}
	}

	return 0;
}

extern int xspi_nor_read(struct device *dev, off_t addr,
			      void *data, size_t len);
extern int xspi_nor_write(struct device *dev, off_t addr,
			       const void *data, size_t len);
extern int xspi_nor_erase(struct device *dev, off_t addr, size_t size);
extern int xspi_nor_write_protection(struct device *dev, bool enable);

static const struct flash_driver_api xspi_nor_api_new = {
	.read = xspi_nor_read,
	.write = xspi_nor_write,
	.erase = xspi_nor_erase,
	.write_protection = xspi_nor_write_protection,
	.lock = xspi_nor_lock,
	.unlock = xspi_nor_unlock,
};
#endif

#ifdef CONFIG_SPI0_XIP
static __ramfunc int acts_xspi_nor_init_new(struct device *dev)
{
	struct xspi_nor_info *sni = (struct xspi_nor_info *)dev->driver_data;
	struct xspi_info *si = &sni->spi;
	u32_t key;

	/* set spi clock */
	xspi_set_clk(si, si->freq_khz);

	sni->chipid = xspi_nor_read_chipid(sni);

	key = irq_lock();
	/* set spi bus_width */
	acts_xspi_nor_init_internal(sni);

	/* selct dual/quad mode because mcp flash only support dual mode, NOT support 2xIO mode */
	xspi_set_bits(si, XSPI_CTL, XSPI_CTL_DUAL_QUAD_SEL_MASK, XSPI_CTL_DUAL_QUAD_SEL_DUAL_QUAD);
	irq_unlock(key);

#ifdef CONFIG_XSPI_NOR_LOCK
	dev->driver_api = &xspi_nor_api_new;
	xspi_nor_lock(dev, 0, (size_t)-1);
#else
	dev->driver_api = &xspi_nor_api;
#endif

	return 0;
}
#else
static int acts_xspi_nor_init_new(struct device *dev)
{
	int ret;

	ret = acts_xspi_nor_init(dev);
#ifdef CONFIG_XSPI_NOR_LOCK
	dev->driver_api = &xspi_nor_api_new;
	xspi_nor_lock(dev, 0, (size_t)-1);
#endif

	return ret;
}
#endif

DEVICE_DEFINE(xspi_nor_acts, CONFIG_XSPI_NOR_ACTS_DEV_NAME, acts_xspi_nor_init_new,
	     acts_xspi_nor_device_ctrl, &system_xspi_nor, NULL, PRE_KERNEL_1,
	     CONFIG_XSPI_NOR_ACTS_DEV_INIT_PRIORITY, NULL);
