/** @file
 *  @brief DIS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief dis service initialization
 *
 * @param device model.
 * @param device manufactory.
 * @param PNP company id/vendor id/product id/product version.
 *
 */

void dis_init(const char *model, const char *manuf, const u8_t *pnp);

#ifdef __cplusplus
}
#endif
