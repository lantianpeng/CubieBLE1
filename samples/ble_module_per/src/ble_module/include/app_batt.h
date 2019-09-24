/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_BATT_H__
#define __APP_BATT_H__

/**
 * @brief battery manager initialization
 *
 */

void batt_manager_init(void);

/**
 * @brief read battery level from battery manager
 *
 * @param pLevel store battery level.
 */

void adc_batt_read(u8_t *pLevel);

/**
 * @brief update battery through read adc channel
 *
 */

void update_battry_value(void);

#endif   /* __APP_BATT_H__ */

