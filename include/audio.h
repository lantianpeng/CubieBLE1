/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio public API header file.
 */

#ifndef __INCLUDE_AUDIO_H__
#define __INCLUDE_AUDIO_H__

#include <stdint.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PCMBUF interrupt control flag */
#define PCMBUF_IP_HE		(1 << 0)
#define PCMBUF_IP_EP		(1 << 1)
#define PCMBUF_IP_HF		(1 << 2)
#define PCMBUF_IP_FU		(1 << 3)
typedef void (*pcmbuf_irq_cbk)(uint32_t pending);

typedef struct {
	/* PCMBUF interrupt callback function */
	pcmbuf_irq_cbk irq_cbk;

	/* PCMBUF half empy threshold, must be an even number */
	uint16_t half_empty_thres;

	/* PCMBUF half full threshold
	 * must be an even number and bigger than half_empty_thres
	 */
	uint16_t half_full_thres;

	/* enable PCMBUF half empty interrupt */
	bool he_ie;

	/* enable PCMBUF empty interrupt */
	bool ep_ie;

	/* enable PCMBUF half full interrupt */
	bool hf_ie;

	/* enable PCMBUF full interrupt */
	bool fu_ie;
} pcmbuf_setting_t;


typedef enum {
	AIN_SOURCE_AUX = 0,
	AIN_SOURCE_FM = 1,
	AIN_SOURCE_MIC = 2,
} ain_source_type_e;

typedef enum {
	MIC_OP_0DB = 0,
	MIC_OP_3DB = 1,
	MIC_OP_6DB = 2,
	MIC_OP_9DB = 3,
	MIC_OP_12DB = 4,
	MIC_OP_15DB = 5,
	MIC_OP_18DB = 6,
	MIC_OP_21DB = 7,
	MIC_OP_15DB_1 = 8,
	MIC_OP_18DB_1 = 9,
	MIC_OP_21DB_1 = 10,
	MIC_OP_24DB = 11,
	MIC_OP_27DB = 12,
	MIC_OP_30DB = 13,
	MIC_OP_33DB = 14,
	MIC_OP_36DB = 15,
} mic_op_gain_e;

typedef enum {
	AIN_OP_M12DB = 0,
	AIN_OP_M3DB = 1,
	AIN_OP_0DB = 2,
	AIN_OP_1DB5 = 3,
	AIN_OP_3DB = 4,
	AIN_OP_4DB5 = 5,
	AIN_OP_6DB = 6,
	AIN_OP_7DB5 = 7,
} ain_op_gain_e;

typedef union {
	mic_op_gain_e mic_op_gain;
	ain_op_gain_e ain_op_gain;
} op_gain_u;

typedef struct {
	ain_source_type_e ain_src;
	op_gain_u op_gain;
	u8_t     ain_input_mode;
} ain_setting_t;

typedef enum {
	ADC_GAIN_0DB = 0,
	ADC_GAIN_0DB0 = 1,
	ADC_GAIN_6DB = 2,
	ADC_GAIN_12DB = 3,
} adc_gain_e;

typedef struct {
	/* sample rate: KHz */
	u8_t sample_rate;

	/* ADC gain */
	adc_gain_e gain;
} adc_setting_t;

/* audio input */

struct audio_dev_driver_api {
	void (*enable)(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *setting);
	void (*disable)(struct device *dev);
};

/**
 * @brief Enables adc channel and starts adc transfer, the pcmbuf must be
 *        configured beforehand.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param ain_setting	config of adc analog
 * @param adc_setting_t	config of adc digital
 *
 */
static inline void audio_in_start(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *adc_setting)
{
	const struct audio_dev_driver_api *api = dev->driver_api;

	api->enable(dev, ain_setting, adc_setting);
}

/**
 * @brief Stops the adc transfer and disables adc channel.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 *
 */

static inline  void audio_in_stop(struct device *dev)
{
	const struct audio_dev_driver_api *api = dev->driver_api;

	api->disable(dev);
}


/**
 * @brief Enables i2s channel and starts i2s transfer, the pcmbuf must be
 *        configured beforehand.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param ain_setting	config of adc analog
 * @param adc_setting_t	config of adc digital
 *
 */
static inline void audio_out_start(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *adc_setting)
{
	const struct audio_dev_driver_api *api = dev->driver_api;

	api->enable(dev, ain_setting, adc_setting);
}

/**
 * @brief Stops the i2s transfer and disables i2s channel.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 *
 */

static inline  void audio_out_stop(struct device *dev)
{
	const struct audio_dev_driver_api *api = dev->driver_api;

	api->disable(dev);
}

/**
 * @brief Configure pcm_buf for ADC DMA transfer.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param setting config for adc dma
 * @param pcm_buf buffer for ac dma from adc to pcm_buf
 * @param data_cnt buffer size of pcm_buf
 *
 */

void audio_in_pcmbuf_config(struct device *dev, pcmbuf_setting_t *setting, uint16_t *pcm_buf, u32_t data_cnt);

/**
 * @brief Configure ADC DMA transfer for a specific channel that has been
 * configured.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pcm_buf buffer for ac dma from adc to pcm_buf
 * @param data_cnt buffer size of pcm_buf
 *
 */

void audio_in_pcmbuf_transfer_config(struct device *dev, uint16_t *pcm_buf, u32_t data_cnt);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_AUDIO_H__ */
