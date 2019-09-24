/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AUDIO_OUT_HAL_H__
#define __AUDIO_OUT_HAL_H__

#define MAX_CHANNEL_IN  2

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#endif

#include <logging/sys_log.h>
/**
 * @defgroup audio_out_apis Audio In APIs
 * @ingroup lib_system_apis
 * @{
 */

/** audio in source from */
typedef enum
{
	/** audio in source from line in*/
    AUDIOIN_SOURCE_LINE_IN,
	/** audio in source from MIC*/
    AUDIOIN_SOURCE_MIC
}audioin_source_e;

typedef void (*get_pcm_t)(u8_t *p_out, u16_t p_out_len);

/** audio out  handle*/
struct audio_out_handle
{
	/** device of audio in driver*/
	struct device *dev;
	/** samples audio in get*/
	int samples;
	/** sample rate of audio in*/
	int sample_rate;
	/** read offset for pcm buffer*/
	int rofs;
	/** read offset for pcm buffer*/
	int pcm_off;
	/**flag is audio in running*/
	bool running;
	/**pointer of pcm buffer*/
	char * pcm_buf;
	/**pcm buffer len*/
	int pcm_buf_len;

	struct net_buf_pool *pcm_pool;
	struct k_fifo *pcm_queue;
	get_pcm_t get_pcm;
};

/**
 * @brief get pcm data from audio in driver
 *
 * This routine provides get audio pcm data from
 * audio in driver, if audio in record not start
 * this routine will retun err.
 *
 * @param handle audio handle , return by audio_out_device_open
 * @param pcm pointer of pcm data
 * @param len len of pcm data user want
 *
 * @return samples get from audio in
 */
int audio_out_get_pcm_data(void *handle, char **pcm, int len);
/**
 * @brief start to recording
 *
 * This routine provides start to recording , mic or linein
 * is enable by this routine.
 *
 * @param handle  audio handle , return by audio_out_device_open
 * @param parama create stream parama
 *
 * @return 0 start succes , others is error
 */
int audio_out_start_playback(void *handle);
/**
 * @brief stop  record
 *
 * This routine provides stop recording , mic or linein
 * is disbale by this routine.
 *
 * @param handle  audio handle , return by audio_out_device_open
 *
 * @return 0 stop succes , others is error
 */
int audio_out_stop_playback(void *handle);
/**
 * @brief open audio in device
 *
 * This routine provides open audio in device, and create a audio
 * in handle .
 *
 * @param sample_rate sample rate of audio in devices set
 *
 * @return NULL if open failed, otherwise return audio in hanlde
 */
void* audio_out_device_open(uint8_t sample_rate);
/**
 * @brief close audio in handle
 *
 * This routine provides close audio in hanlde ,and release resouce for
 * audio in.
 *
 * @param handle  audio handle , return by audio_out_device_open
 *
 * @return N/A
 */
void audio_out_device_close(void *handle);

/**
 * @} end defgroup audio_out_apis
 */
 
void audio_out_disable(struct device *dev);

#endif
