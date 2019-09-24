/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AUDIOIN_HAL_H__
#define __AUDIOIN_HAL_H__

/**
 * @defgroup audioin_apis Audio In APIs
 * @ingroup lib_system_apis
 * @{
 */

/** audio in  handle*/
struct audioin_handle {
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

	struct net_buf *working_net_buf;
	struct net_buf *completed_net_buf;

	struct net_buf_pool *pcm_pool;
};

/**
 * @brief start to recording
 *
 * This routine provides start to recording , mic or linein
 * is enable by this routine.
 *
 * @param handle  audio handle , return by audioin_device_open
 * @param parama create stream parama
 *
 * @return 0 start succes , others is error
 */
int audioin_start_record(void *handle);
/**
 * @brief stop  record
 *
 * This routine provides stop recording , mic or linein
 * is disbale by this routine.
 *
 * @param handle  audio handle , return by audioin_device_open
 *
 * @return 0 stop succes , others is error
 */
int audioin_stop_record(void *handle);
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
void *audioin_device_open(u8_t sample_rate);
/**
 * @brief close audio in handle
 *
 * This routine provides close audio in hanlde ,and release resouce for
 * audio in.
 *
 * @param handle  audio handle , return by audioin_device_open
 *
 * @return N/A
 */
void audioin_device_close(void *handle);

/**
 * @} end defgroup audioin_apis
 */

#endif
