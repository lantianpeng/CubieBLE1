/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int prng_init(void);

extern int use_hci_le_encrypt;

int SecCmac(const u8_t *key, const u8_t *in, size_t len, u8_t *out);

int SecAes(const u8_t key[16], const u8_t plaintext[16], u8_t enc_data[16]);

