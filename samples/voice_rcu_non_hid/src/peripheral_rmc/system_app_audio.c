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
#include "system_app_storage.h"
#include "audio_out_hal.h"
#include "audioin_hal.h"
#include "msg_manager.h"
#include "system_app_audio.h"
#include "asc_i_encoder.h"
#include "asc_ii_encoder.h"
#include "asc_iii_encoder.h"

#include "system_app_sm.h"
#include "system_app_input.h"
#include "soc_pm.h"
#include "system_app_ble.h"
#include <flash.h>
#include "audioin_hal.h"
//#define SYS_LOG_DOMAIN "audio"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>


extern int audioin_start_record(void *handle);
extern int audioin_stop_record(void *handle);
extern void *audioin_device_open(u8_t sample_rate);
extern void audioin_device_close(void *handle);
static struct audioin_handle *audioin;

typedef int off_t;
static struct audio_out_handle * audio_out = NULL;

static u16_t Bitstreamlen = 0;
static u32_t audio_file_offset = 0;

extern u32_t audio_file_size;
extern u32_t audio_file_addr;
extern struct device *flash_dev;

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
static u8_t p_in_data[PCM_BUF_LEN];
static u8_t p_out_data[PCM_BUF_LEN];

NET_BUF_POOL_DEFINE(pcm_buffer_pool, PCM_BUF_NUM, PCM_BUF_LEN, 4, NULL);

static K_FIFO_DEFINE(pcm_queue);

__data_overlay u16_t dma_pcm_buf[PCM_BUF_LEN];

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
	{
		hid_app_voice_report_event(&encode_out_buffer[i]);
		write_data_to_storage(&encode_out_buffer[i], 20);
	}

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

#define USE_STEREO 1
#if USE_STEREO
void convert_mono_to_stereo(u8_t *p_in, u16_t *p_out, u16_t p_out_len)
{
	u32_t i = 0;
	/* p_out_len/2 for convert mono to stereo*/
	u32_t read_len = p_out_len/2;
	
	u16_t *p_mono = (u16_t*)p_in;
	u16_t *p_stereo = p_out;
	
	/* read_len/2 for 16bit convert*/
	for(i = 0; i < read_len/2; i++) {
		*p_stereo++ = *p_mono;
		*p_stereo++ = *p_mono++;
	}
}
#endif
void stop_audio_playback(void)
{
	audio_out_stop_playback(audio_out);
	audio_file_offset = 0;
}

void send_pcm_to_i2s(u8_t *p_out, u16_t p_out_len)
{
	u8_t times = 0;
	u32_t i = 0;
	u32_t cur_addr;
	u32_t out_len = 0;
	u32_t read_len = 0;
	
	int16_t *p_in_buffer = (int16_t *)p_in_data;
#if USE_STEREO
	int16_t *p_out_buffer = (int16_t *)p_out_data; 
#else
	int16_t *p_out_buffer = (int16_t *)p_out; /* use the first dma buf of */
#endif	
	if (audio_file_size > audio_file_offset) {
		/* 1. read data from flash */
		read_len = p_out_len/4;
#if USE_STEREO
		/* for convert_stereo_to_mono */
		read_len = read_len/2;
#endif
		if(flash_dev) {
			cur_addr = audio_file_addr + audio_file_offset;
			flash_read(flash_dev, (off_t)(cur_addr&0xFFFFF), p_in_data, (size_t)read_len);
		}
		audio_file_offset += read_len;
	  //printk("audio_file_offset is %d\n",audio_file_offset);
		/* 2. decode */
		times = read_len / (Bitstreamlen * 2);
		//printk("times is %d\n",times);
		for (i = 0; i < times; i++) {
			out_len = ASC_III_Decoder(p_in_buffer, p_out_buffer, Bitstreamlen);
			p_in_buffer += Bitstreamlen;
			p_out_buffer += out_len;
		}
#if USE_STEREO	
		/* 3. convert_mono_to_stereo */
		convert_mono_to_stereo(p_out_data, p_out, p_out_len);
#endif
	}
	else {
		//printk("stop audio\n");
		stop_audio_playback();
	}
}
void start_audio_playback(void)
{
	/* init decoder */
	Bitstreamlen = ASC_III_Decoder_Init(8000, 16, 1, 32000);
	if (Bitstreamlen == 0) {
		printk("ASC_Decoder_Init error !\n");
		return;
	}
	
	send_pcm_to_i2s(dma_pcm_buf, PCM_BUF_LEN);
	audio_out_start_playback(audio_out);
}

extern void *audioin_device_open(u8_t sample_rate);
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
	
#ifdef CONFIG_AUDIO_OUT
	audio_out = (struct audio_out_handle *)audio_out_device_open(CONFIG_AUDIO_SAMPLE_RATE);
	audio_out->pcm_pool = &pcm_buffer_pool;
	audio_out->pcm_queue = &pcm_queue;
	audio_out->pcm_buf = (char *)dma_pcm_buf;
	audio_out->pcm_buf_len = PCM_BUF_LEN * 2;
#endif
  audioin = (struct audioin_handle *)audioin_device_open(CONFIG_AUDIO_SAMPLE_RATE);
	audioin->pcm_pool = &pcm_buffer_pool;
	return 0;
}

int8_t audio_exit(void)
{
	SYS_LOG_DBG("audio_exit");
	audioin_device_close(audioin);
	#ifdef CONFIG_AUDIO_OUT
	audio_out_device_close(audio_out);
	#endif
	return 0;
}
