/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SYTEM_APP_AUDIO_H
#define _SYTEM_APP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief start audio data capture
 *
 */

void start_audio_capture(void);

/**
 * @brief function of audio coding
 *
 * @param in_buffer audio data buffer before encoding.
 * @param out_buffer encoded audio data buffer.
 *
 * @return encoded audio buffer size.
 */

u16_t audio_encode(void *in_buffer, u8_t *out_buffer);

/**
 * @brief stop audio data capture
 *
 */

void stop_audio_capture(void);
	
/**
 * @brief audio module initialzation
 *
 */

int8_t audio_init(void);

/**
 * @brief audio module exits and release resourses
 *
 */

int8_t audio_exit(void);

/**
 * @brief audio event handler
 *
 * @param audio data buffer pointer.
 */

void system_audio_event_handle(void *ptr);

/**
 * @brief send a start flag before sending audio data
 *
 */

void audio_send_start_flag(void);

void stop_audio_playback(void);
void start_audio_playback(void);

/**
 * @brief send an end flag after audio transmission
 *
 */

void audio_send_stop_flag(void);

#ifdef __cplusplus
};
#endif

#endif /* _SYTEM_APP_AUDIO_H */
