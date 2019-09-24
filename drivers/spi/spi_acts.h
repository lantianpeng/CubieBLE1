/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for spi
 */

#ifndef __SPI_ACTS_H__
#define __SPI_ACTS_H__

#include <zephyr/types.h>
#include <spi.h>
#include <dma.h>

#ifdef __cplusplus
extern "C" {
#endif


struct acts_spi_controller
{
	volatile uint32_t ctrl;  
	volatile uint32_t status;  
	volatile uint32_t txdat;
	volatile uint32_t rxdat;
	volatile uint32_t bc;   
} ;

struct acts_spi_config {
	struct acts_spi_controller *spi;
	u32_t spiclk_reg;
	u8_t txdma_id;
	u8_t rxdma_id;
	s8_t dma_chan0;
	s8_t dma_chan1;
	u8_t clock_id;
	u8_t reset_id;
	u8_t delay_chain;
	u8_t reserved;
};

struct acts_spi_data {
	struct spi_context ctx;
	struct device *dma_dev;
	struct k_sem dma_sync;
};
	
extern int spi_acts_transceive(struct spi_config *config,
			     const struct spi_buf *tx_bufs, size_t tx_count,
			     struct spi_buf *rx_bufs,  size_t rx_count);
extern int spi_acts_release(struct spi_config *config);

extern const struct spi_driver_api acts_spi_driver_api;
extern int spi_acts_init(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif	/* __SPI_ACTS_H__ */
