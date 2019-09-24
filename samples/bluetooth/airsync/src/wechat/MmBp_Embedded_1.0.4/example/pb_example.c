//  pb_example.c
//  WeChat Embedded
//
//  Created by harlliu on 14-03-12.
//  Copyright 2014 Tencent. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include "epb_MmBp.h"
#include "crc32.h"
#include "aes_crypt.h"

typedef struct 
{
	uint32_t random;
	uint32_t seq;
	uint32_t crc;
} auth_buf_t;

uint8_t md5_type_id[16] = {0x79, 0xC1, 0x92, 0xD6, 0xE7, 0xEA, 0x6B, 0xB9, 0x7E, 0x1C, 0x2C, 0x44, 0xD3, 0xAB, 0x92, 0x43};
uint8_t device_key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
char *device_name = "WeChat_Device";
char *device_id = "Dev_001";
char *timezone = "UTC+8";
char *language = "zh-CN";

int auth_seq = 0;

void pack_auth_request()
{
	AuthRequest *request = (AuthRequest *)malloc(sizeof(AuthRequest));
	request->md5_device_type_and_device_id.data = md5_type_id;
	request->md5_device_type_and_device_id.len = sizeof(md5_type_id);
	request->proto_version = 0x010000;
	request->auth_proto = 1;
	request->auth_method = 1;

	auth_buf_t auth_buf;
    srand((unsigned)time(0));
    int auth_rand = rand(); //srand()和rand()可能需要另外实现
	auth_buf.random = htonl(auth_rand); //htonl()可能需要另外实现
	auth_buf.seq = htonl(auth_seq);
	auth_seq++;
	uint32_t crc = 0;
	crc = crc32(crc, (uint8_t *)device_id, strlen(device_id));
	crc = crc32(crc, (uint8_t *)&auth_buf.random, 4);
	crc = crc32(crc, (uint8_t *)&auth_buf.seq, 4);
	auth_buf.crc = htonl(crc);

	AES_CTX c;
	int out_len = 0;
	uint8_t temp_buf[32];
	aes_cbc_encrypt_init(&c, device_key);
	aes_cbc_encrypt_final(&c, (uint8_t *)&auth_buf, sizeof(auth_buf), temp_buf, &out_len);

	request->aes_sign.data = temp_buf;
	request->aes_sign.len = out_len;

	request->has_time_zone = true;
	request->time_zone.str = timezone;
	request->time_zone.len = strlen(timezone);

	request->has_language = true;
	request->language.str = language;
	request->language.len = strlen(language);

	request->has_device_name = true;
	request->device_name.str = device_name;
	request->device_name.len = strlen(device_name);

	int len = epb_auth_request_pack_size(request);
	uint8_t *buf = (uint8_t *)malloc(len);
	epb_pack_auth_request(request, buf, len);

	printf("auth request len: %d\n", len); //75
	for(int i=0; i<len; i++) {
		printf("%02x", buf[i]);
	} 
	printf("\n");

	free(request);
	free(buf);
	//0a00121079c192d6e7ea6bb97e1c2c44d3ab9243
	//18808004200128013210b6b88f081ee0f230d1f4
	//71949dae1ed052055554432b385a057a682d434e
	//620d5765436861745f446576696365 //每次运行会有不一样
}

const uint8_t auth_response_buf[] = {0x0a, 0x1d, 0x08, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
	0xff, 0xff, 0x01, 0x12, 0x10, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x54, 0x69, 0x6d, \
	0x65, 0x20, 0x4f, 0x75, 0x74, 0x12, 0x00};

void unpack_auth_response()
{
	AuthResponse *response = epb_unpack_auth_response(auth_response_buf, sizeof(auth_response_buf));

	if(response != NULL) {
		printf("error code: %d\n", response->base_response->err_code); //EEC_sessionTimeout = -3
		if (response->base_response->has_err_msg) {
			for(int i=0; i<response->base_response->err_msg.len; i++) {//字符串不是以'\0'结尾，不能直接使用printf打印整个字符串
				printf("%c", response->base_response->err_msg.str[i]); //"Session Time Out"
			}
			printf("\n");
		}

		if (response->aes_session_key.len > 0) { //response->aes_session_key.len = 0
			for(int i=0; i<response->aes_session_key.len; i++) {
				printf("%02x", response->aes_session_key.data[i]);
			}
			printf("\n");
		}

		epb_unpack_auth_response_free(response);
	}
}


int main(int argc, char **argv)
{
	pack_auth_request();
	unpack_auth_response();
	return 0;
}
