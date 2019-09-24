//  aes_example.c
//  WeChat Embedded
//
//  Created by harlliu on 14-03-12.
//  Copyright 2014 Tencent. All rights reserved.
//

#include <stdio.h>
#include "aes_crypt.h"


uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
uint8_t key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

uint8_t encrypt_buf[128];
int encrypt_len = 0;
uint8_t decrypt_buf[128];
int decrypt_len = 0;

void encrypt_data()
{
	int len = sizeof(data);
	int offset = 0;

	AES_CTX c;
	aes_cbc_encrypt_init(&c, key);

	while(len > 16) {
		aes_cbc_encrypt_update(&c, data+offset, encrypt_buf+offset);
		offset += 16;
		len -= 16;
	}

	int out_len = 0;
	aes_cbc_encrypt_final(&c, data+offset, len, encrypt_buf+offset, &out_len);
	encrypt_len = offset + out_len;
	
	printf("encrypt len: %d\n", encrypt_len); //48
	for(int i=0; i<encrypt_len; i++) {
		printf("%02x", encrypt_buf[i]);
	} //c6a13b37878f5b826f4f8162a1c8d879b58a1064d8aca99bd9b0405b8545f5bbe0db23614cac28000000000088ac2800
	printf("\n");
}

void decrypt_data()
{
	int len = encrypt_len;
	int offset = 0;

	AES_CTX c;
	aes_cbc_decrypt_init(&c, key);

	while(len > 16) {
		aes_cbc_decrypt_update(&c, encrypt_buf+offset, decrypt_buf+offset);
		offset += 16;
		len -= 16;
	}

	int out_len = 0;
	aes_cbc_decrypt_final(&c, encrypt_buf+offset, decrypt_buf+offset, &out_len);
	decrypt_len = offset + out_len;
	
	printf("decrypt len: %d\n", decrypt_len); //40
	for(int i=0; i<decrypt_len; i++) {
		printf("%02x", decrypt_buf[i]);
	} //000102030405060708090a0b0c0d0e0f000102030405060708090a0b0c0d0e0f0001020304050607
	printf("\n");
}

int main(int argc, char **argv)
{
	encrypt_data();
 	decrypt_data();
	return 0;
}
