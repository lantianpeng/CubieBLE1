/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "errno.h"
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <net/buf.h>


#include "msg_manager.h"
#include "system_app_audio.h"
#include "asc_i_encoder.h"
#include "asc_ii_encoder.h"
#include "asc_iii_encoder.h"
#include "audioin_hal.h"
#include "system_app_sm.h"
#include "system_app_input.h"
#include "soc_pm.h"
#include "system_app_ble.h"

#define SYS_LOG_DOMAIN "audio"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

static struct audioin_handle *audioin;

struct audio_cb_t {
	short   codec;
	short   sample_len;
	u8_t    send_start_flag;
	u8_t    send_stop_flag;
	u8_t    need_drop_buffer_count;
	u8_t    encoder_init_flag;
};

static struct audio_cb_t audio_cb;

short (*encode_init_func)(short codec);
short (*encode_func)(short *indata, short *outdata, short len);

#ifdef USE_AL_ENCODE_1
int global_encode_buffer[4*1024];
#endif

#if (defined(USE_AL_ENCODE_1) || defined(USE_AL_ENCODE_2_8_1) || defined(USE_AL_ENCODE_2_8_1_2))
#define CONFIG_AUDIO_SAMPLE_RATE 16
#define PCM_BUF_LEN  640
#else
#define CONFIG_AUDIO_SAMPLE_RATE 8
#define PCM_BUF_LEN  320
#endif
#define PCM_BUF_NUM  5

NET_BUF_POOL_DEFINE(pcm_buffer_pool, PCM_BUF_NUM, PCM_BUF_LEN, 4, NULL);
u16_t audio_encode(void *in_buffer, u8_t *out_buffer)
{
	u8_t i, times;
	u16_t out_len = 0;
	struct net_buf *buf = (struct net_buf *)in_buffer;

	s16_t *p_in_buffer = (s16_t *)buf->data;

	u16_t pcm_buffer_len = buf->len;
	s16_t *p_out_buffer = (s16_t *)out_buffer;

	times = pcm_buffer_len/(audio_cb.sample_len * 2);

	for (i = 0; i < times; i++) {
		out_len = (encode_func)(p_in_buffer, p_out_buffer, audio_cb.sample_len);
		p_in_buffer += audio_cb.sample_len;
		p_out_buffer += out_len;
	}

	return (out_len * times * 2);
}

void audio_send_start_flag(void)
{
	if (audio_cb.encoder_init_flag) {
		audio_cb.sample_len = (encode_init_func)(audio_cb.codec);
		audio_cb.encoder_init_flag = 0;
		SYS_LOG_INF("sample_len : %d", audio_cb.sample_len);
	}

	if (audio_cb.send_start_flag) {
		/* Make sure start flag send completed */
		if ((hid_app_remote_report_event(REMOTE_KEY_VOICE_COMMAND) == 0)
#ifdef CONFIG_ANDROID_TV_BOX
			&& (hid_app_remote_report_event(0x0000) == 0)
#endif
		) {
			audio_cb.send_start_flag = 0;
			SYS_LOG_INF("send audio_start flag");
		}
	}
}

void audio_send_stop_flag(void)
{
	if (audio_cb.send_stop_flag) {
		/* Make sure end flag send completed */
		if ((hid_app_remote_report_event(REMOTE_KEY_VOICE_COMMAND_END) == 0)
#ifdef CONFIG_ANDROID_TV_BOX
			&& (hid_app_remote_report_event(0x0000) == 0)
#endif
		) {
			audio_cb.send_stop_flag = 0;
			SYS_LOG_INF("send audio_stop flag");
		}
	}
}

bool audio_need_drop_buffer(void)
{
	if (audio_cb.need_drop_buffer_count) {
		audio_cb.need_drop_buffer_count--;
		SYS_LOG_DBG("dummy buffer %d", audio_cb.need_drop_buffer_count);
		return true;
	}
	return false;
}

void pcm_buffer_free(void *ptr)
{
	struct net_buf *buf = (struct net_buf *)ptr;

	if (buf != NULL)
		net_buf_unref(buf);
}

__data_overlay u8_t encode_out_buffer[80];
u32_t count;
void system_audio_event_handle(void *ptr)
{
	u8_t *out_buffer = encode_out_buffer;
	u16_t i, out_len = 0;

	if (rmc_sm_state_is_activing()) {
		pcm_buffer_free(ptr);
		return;
	}

	/*  */
	audio_send_start_flag();

	if (audio_need_drop_buffer()) {
		pcm_buffer_free(ptr);
		return;
	}

	out_len = audio_encode((void *)ptr, out_buffer);
	count += out_len;
	SYS_LOG_DBG("send (%d) data", count);
	pcm_buffer_free(ptr);

	for (i = 0; i < out_len; i = i+20)
		hid_app_voice_report_event(&encode_out_buffer[i]);

	audio_send_stop_flag();
}

void start_audio_capture(void)
{
	/* audio overlay buffer is used after other module init*/
	if (overlay_ready_flag == 0)
		return;

	audio_cb.encoder_init_flag = 1;
	audio_cb.send_start_flag = 1;
	audio_cb.send_stop_flag = 0;
	audio_cb.need_drop_buffer_count = 4;
	audioin_start_record(audioin);
}

void stop_audio_capture(void)
{
	/* audio overlay buffer is used after other module init*/
	if (overlay_ready_flag == 0)
		return;

	audio_cb.send_stop_flag = 1;
	audio_cb.send_start_flag = 0;
	audioin_stop_record(audioin);
}

__init_once_text int8_t audio_init(void)
{
	memset(&audio_cb, 0, sizeof(audio_cb));
#if defined(USE_AL_ENCODE_1)
	audio_cb.codec = 10;
	encode_init_func = ASC_I_Encoder_Init;
	encode_func = ASC_I_Encoder;
	ASC_I_Encoder_Set_Buffer(global_encode_buffer);
#elif defined(USE_AL_ENCODE_2_8_1)
	audio_cb.codec = 20;
	encode_init_func = ASC_II_Encoder_Init;
	encode_func = ASC_II_Encoder;
#elif defined(USE_AL_ENCODE_2_8_1_2)
	audio_cb.codec = 21;
	encode_init_func = ASC_II_B_Encoder_Init;
	encode_func = ASC_II_B_Encoder;
#elif defined(USE_AL_ENCODE_3_4_1)
	audio_cb.codec = 30;
	encode_init_func = ASC_III_Encoder_Init;
	encode_func = ASC_III_Encoder;
#elif defined(USE_AL_ENCODE_3_4_1_2)
	audio_cb.codec = 31;
	encode_init_func = ASC_III_Encoder_Init;
	encode_func = ASC_III_Encoder;
#else
	audio_cb.codec = 30;
	encode_init_func = ASC_III_Encoder_Init;
	encode_func = ASC_III_Encoder;
#endif

	audioin = (struct audioin_handle *)audioin_device_open(CONFIG_AUDIO_SAMPLE_RATE);
	audioin->pcm_pool = &pcm_buffer_pool;
	return 0;
}

int8_t audio_exit(void)
{
	SYS_LOG_DBG("audio_exit");
	audioin_device_close(audioin);
	return 0;
}
