/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI XIP controller helper for Actions SoC
 */

#ifndef __XSPI_NOR_ACTS_H__
#define __XSPI_NOR_ACTS_H__

#include <zephyr/types.h>
#include <flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/* spinor controller */
#define XSPI_CTL			0x00
#define XSPI_STATUS			0x04
#define XSPI_TXDAT			0x08
#define XSPI_RXDAT			0x0c
#define XSPI_BC				0x10
#define XSPI_SEED			0x14
#define XSPI_CACHE_ERR_ADDR		0x18
#define XSPI_NO_CACHE_ADDR		0x1C

#define XSPI_CTL_DUAL_QUAD_SEL_MASK				(1 << 27)
#define XSPI_CTL_DUAL_QUAD_SEL_DUAL_QUAD	(1 << 27)
#define XSPI_CTL_DUAL_QUAD_SEL_2X_4X			(0 << 27)

struct xspi_info {
	/* spi controller address */
	u32_t base;
	u32_t flag;

	u16_t cs_gpio;
	u8_t bus_width;
	u8_t delay_chain;

	u32_t spiclk_reg;
	u32_t freq_khz;

	u8_t clock_id;
	u8_t reset_id;
	u8_t dma_id;
	s8_t dma_chan;

	void (*set_cs)(struct xspi_info *si, int value);
	void (*set_clk)(struct xspi_info *si, unsigned int freq_khz);
	void (*prepare_hook)(struct xspi_info *si);

};

struct xspi_nor_info {
	struct xspi_info spi;
	u32_t chipid;
};

#ifdef CONFIG_SPI0_XIP
static inline u32_t xspi_read(struct xspi_info *si, u32_t reg)
{
	return sys_read32(si->base + reg);
}

static inline void xspi_write(struct xspi_info *si, u32_t reg,
				    u32_t value)
{
	sys_write32(value, si->base + reg);
}

static inline void xspi_set_bits(struct xspi_info *si, u32_t reg,
				       u32_t mask, u32_t value)
{
	xspi_write(si, reg, (xspi_read(si, reg) & ~mask) | value);
}

static ALWAYS_INLINE void xspi_set_clk(struct xspi_info *si, u32_t freq_khz)
{
	u32_t pclk_freq_khz = 32000;
	u32_t div;

	if (!freq_khz)
		freq_khz = 1000; 
	
	div = find_lsb_set(pclk_freq_khz / freq_khz) - 1;

	if (div > 5)
		div = 5;

	sys_write32(div, si->spiclk_reg);
}

#endif

#ifndef CONFIG_SPI0_XIP
int acts_xspi_nor_init(struct device *dev);
#endif

int acts_xspi_nor_suspend(struct device *dev);
int acts_xspi_nor_resume(struct device *dev);
void xspi_nor_write_data(struct xspi_nor_info *sni, u8_t cmd,
			u32_t addr, s32_t addr_len, const u8_t *buf, s32_t len);

#ifdef CONFIG_SPI0_XIP
extern  const struct flash_driver_api xspi_nor_api;
u32_t xspi_nor_read_chipid(struct xspi_nor_info *sni);
void acts_xspi_nor_init_internal(struct xspi_nor_info *sni);
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __XSPI_NOR_ACTS_H__ */
