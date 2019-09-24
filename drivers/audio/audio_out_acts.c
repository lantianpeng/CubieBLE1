/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <init.h>
#include <device.h>
#include <irq.h>
#include <soc.h>
#include <string.h>
#include <audio.h>
#include "errno.h"
#include "dma.h"
#include "soc.h"

#define SYS_LOG_DOMAIN "audio"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

#define		DISABLE			0
#define		ENABLE			1

#define		MASTER			0
#define		SLAVE				1

#define		MCLK_INT		0
#define		MCLK_EXT		1

#define		OSR64				0
#define		OSR32				1

#define		WL24BIT			2
#define		WL20BIT			1
#define		WL16BIT			0

#define		I2S_FORMAT	0
#define		LJ_FORMAT		1
#define		RJ_FORMAT		2

#define		TX_INTCLK									0
#define		TX_EXTCLK									1

#define		TXFIFO_CPU_ACCESS		0
#define		TXFIFO_DMA_ACCESS		1

#define		FS_16K_I2S		16000
#define		FS_8K_I2S			8000

#define     IO_MFP_BASE                                                       0x40016000
#define     IO22_CTL                                                          (IO_MFP_BASE+0x5C)
#define     IO23_CTL                                                          (IO_MFP_BASE+0x60)
#define     IO24_CTL                                                          (IO_MFP_BASE+0x64)
#define     IO25_CTL                                                          (IO_MFP_BASE+0x68)

#define     DAC_CONTROL_REGISTER_BASE                                          0x40015000
#define     I2S_TX_CTL                                                        (DAC_CONTROL_REGISTER_BASE+0x00)
#define     I2S_RX_CTL                                                        (DAC_CONTROL_REGISTER_BASE+0x04)
#define     I2S_TXFIFOCTL                                                     (DAC_CONTROL_REGISTER_BASE+0x08)
#define     I2S_TXFIFOSTAT                                                    (DAC_CONTROL_REGISTER_BASE+0x0c)
#define     I2S_DAT_FIFO                                                      (DAC_CONTROL_REGISTER_BASE+0x10)

#define     CMU_ANALOG_BASE                                                   0x40002100
#define     AUDPLL_CTL                                                        (CMU_ANALOG_BASE+0x0004)



void mfp_cfg_i2stx()
{
	int func=10;

	sys_write32((0x3<<12)|(func),IO22_CTL);//TXMCLK
	sys_write32((0x3<<12)|(func),IO23_CTL);//TXBCLK
	sys_write32((0x3<<12)|(func),IO24_CTL);//TXLRCLK
	sys_write32((0x3<<12)|(func),IO25_CTL);//TXDATA

}

void i2stx0_if_init(	u32_t mode_sel,
											u32_t mclk_sel,
											u32_t word_len,
											u32_t bclk_rat,
											u32_t format_sel,
											u32_t reserved)
{
	sys_write32(0,I2S_TX_CTL);
	sys_write32(sys_read32(I2S_TX_CTL) | mode_sel<<7, I2S_TX_CTL);
	sys_write32(sys_read32(I2S_TX_CTL) | mclk_sel<<6, I2S_TX_CTL);
	sys_write32(sys_read32(I2S_TX_CTL) | bclk_rat<<3, I2S_TX_CTL);
	sys_write32(sys_read32(I2S_TX_CTL) | format_sel<<1, I2S_TX_CTL);

}

void i2stx_if_op(int operation)
{
	if(operation!=DISABLE)
		sys_write32(sys_read32(I2S_TX_CTL) | 0x1, I2S_TX_CTL);
	else
		sys_write32(sys_read32(I2S_TX_CTL) & (~0x1), I2S_TX_CTL);
}

void i2stx_init()
{
	/* reset i2s controller */
	//acts_reset_peripheral(RESET_ID_AUDIO);
	/* enable i2s TX clock */
	acts_clock_peripheral_enable(CLOCK_ID_I2STX);
}

void i2stx0_clk_cfg(u32_t pll,u32_t src,u32_t fs,u32_t osr)
{
	int CLKDIV;

	i2stx_init();

	if(fs!=FS_8K_I2S)
		CLKDIV=4;
	else
		CLKDIV=6;

	sys_write32(0x16, AUDPLL_CTL);
	sys_write32(sys_read32(CMU_AUDIOCLK) & (~(0xf<<8)), CMU_AUDIOCLK);
	sys_write32(sys_read32(CMU_AUDIOCLK) | (CLKDIV<<8), CMU_AUDIOCLK);

}

void i2stx_fifo_cfg()
{
	sys_write32(0x13, I2S_TXFIFOCTL);

}

struct acts_audio_out_config {
	u8_t dma_id;
	s8_t dma_chan;
};

struct acts_audio_out_data {
	struct device *dma_dev;
	u32_t backup_old_vdd;
};

//struct audio_dev_driver_api {
//	void (*enable)(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *setting);
//	void (*disable)(struct device *dev);
//};


static pcmbuf_irq_cbk audio_out_pcmbuf_irq_cbk;

void audio_out_dma_isr(struct device *dev, u32_t callback_data, int error_code)
{
	u32_t pending = 0;
	if (error_code == 1)
		pending |= PCMBUF_IP_HF;
	
	if (error_code == 0)
		pending |= PCMBUF_IP_FU;
	
	if (pending && audio_out_pcmbuf_irq_cbk)
		audio_out_pcmbuf_irq_cbk(pending);
}

void audio_out_pcmbuf_transfer_config(struct device *dev,uint16_t *pcm_buf, u32_t data_cnt)
{
		struct dma_transfer_config dma_transfer_cfg = {0};
		const struct acts_audio_out_config *cfg = dev->config->config_info;
		struct acts_audio_out_data *data = dev->driver_data;
		
		dma_transfer_cfg.block_size = (u32_t)data_cnt;
		dma_transfer_cfg.source_address = (u32_t*)pcm_buf;
		dma_transfer_cfg.destination_address = (u32_t*)I2S_DAT_FIFO;
		if (dma_transfer_config(data->dma_dev, cfg->dma_chan, &dma_transfer_cfg)) {
			SYS_LOG_ERR("dma%d transfer_config error", cfg->dma_chan);
			return;
		}
}


void audio_out_pcmbuf_config(struct device *dev, pcmbuf_setting_t *setting, uint16_t *pcm_buf, u32_t data_cnt)
{ 
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	const struct acts_audio_out_config *cfg = dev->config->config_info;
	struct acts_audio_out_data *data = dev->driver_data;

	if (setting->hf_ie || setting->fu_ie) {
		if (setting->irq_cbk) {
			audio_out_pcmbuf_irq_cbk = setting->irq_cbk;
			dma_cfg.dma_callback = audio_out_dma_isr;
			dma_cfg.callback_data = NULL;
		}
		if (setting->hf_ie)
			dma_cfg.half_complete_callback_en = 1;
		
		if (setting->fu_ie)
			dma_cfg.complete_callback_en = 1;
		
		dma_cfg.source_data_size = 2;
		dma_cfg.dest_data_size = 2;
		dma_cfg.block_count = 1;
		dma_cfg.head_block = &dma_block_cfg;

		dma_cfg.dma_slot = cfg->dma_id;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_block_cfg.source_address = (u32_t)pcm_buf;
		dma_block_cfg.dest_address = (u32_t)I2S_DAT_FIFO;
		dma_block_cfg.block_size = data_cnt;
		dma_block_cfg.source_reload_en = 1; 
		dma_block_cfg.dest_reload_en = 1;
		if (dma_config(data->dma_dev, cfg->dma_chan, &dma_cfg)) {
			SYS_LOG_ERR("dma%d config error", cfg->dma_chan);
			return;
		}
		printk("audio_out_pcmbuf_config \n");
	}
}

void start_i2s_tx_dma(struct device *dev)
{
	const struct acts_audio_out_config *cfg = dev->config->config_info;
	struct acts_audio_out_data *data = dev->driver_data;
	if (dma_start(data->dma_dev, cfg->dma_chan)) {
		SYS_LOG_ERR("dma%d start error", cfg->dma_chan);
		return ;
	}
	printk("start_i2s_tx_dma \n");
}

void stop_i2s_tx_dma(struct device *dev)
{
	const struct acts_audio_out_config *cfg = dev->config->config_info;
	struct acts_audio_out_data *data = dev->driver_data;
	dma_stop(data->dma_dev, cfg->dma_chan);
	printk("stop_i2s_tx_dma \n");
}

void audio_out_enable(struct device *dev, ain_setting_t *ain_setting,
		     adc_setting_t *adc_setting)
{
	ARG_UNUSED(dev);

	int fs;
	fs = FS_8K_I2S;
	
	/* 1. config mfp */
	mfp_cfg_i2stx();
	
	/* 2. config i2s tx clock */
	i2stx0_clk_cfg(0,TX_INTCLK,fs,OSR64);
	
	/* 3. config i2s tx ctrl */
	i2stx0_if_init(MASTER,MCLK_INT,WL24BIT,OSR64,I2S_FORMAT,0);
	
	/* 4.  config i2s fifo */
	i2stx_fifo_cfg();
	
	/* 5. start dma */
	start_i2s_tx_dma(dev);

	/* 6. enable i2s tx */
	i2stx_if_op(ENABLE);
	
	printk("audio_out_enable \n");
}

void audio_out_disable(struct device *dev)
{
	ARG_UNUSED(dev);

	/* 1. disable i2s tx */
	i2stx_if_op(DISABLE);
	
	/* 2. stop dma */
	stop_i2s_tx_dma(dev);
}

void acts_audio_out_enable(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *setting)
{
	audio_out_enable(dev, ain_setting, setting);
	device_busy_set(dev);
}

void acts_audio_out_disable(struct device *dev)
{
	audio_out_disable(dev);
	acts_reset_peripheral(RESET_ID_AUDIO);
	device_busy_clear(dev);
}

int audio_out_init(struct device *dev)
{
	struct acts_audio_out_data *data = dev->driver_data;
	
	data->dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!data->dma_dev)
		return -ENODEV;

	return 0;
}

struct acts_audio_out_data audio_out_acts_data;

const struct acts_audio_out_config audio_out_acts_cfg = {
	.dma_id = DMA_ID_I2S,
	.dma_chan = 2,
};

const struct audio_dev_driver_api audio_out_acts_driver_api = {
	.enable = acts_audio_out_enable,
	.disable = acts_audio_out_disable,
};

DEVICE_AND_API_INIT(audio_out, CONFIG_AUDIO_OUT_ACTS_NAME, audio_out_init, &audio_out_acts_data, &audio_out_acts_cfg,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &audio_out_acts_driver_api);
