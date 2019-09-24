/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __APP_CFG_H__
#define __APP_CFG_H__

#include <kernel.h>
#include <zephyr/types.h>
#include <misc/printk.h>

void load_app_cfg_from_nv(void);

void set_app_cfg_to_default(void);

/* 1. soft reset */
int at_cmd_reset(void);

/* 2. state of adv */
int at_cmd_adv_set(const char *p_para);
int at_cmd_adv_get(void);

/* 3. interval of adv */
int at_cmd_adv_interval_set(const char *p_para);
int at_cmd_adv_interval_get(void);

/* 4. name data of adv */
int at_cmd_adv_name_set(const char *p_para);

/* 5. baudrate of uart */
int at_cmd_adv_baudrate_set(const char *p_para);
int at_cmd_adv_baudrate_get(void);

/* 6. interval of conn */
int at_cmd_conn_interval_set(const char *p_para);
int at_cmd_conn_interval_get(void);

/* 7. tx power of conn */
int at_cmd_tx_power_set(const char *p_para);
int at_cmd_tx_power_get(void);

/* 8. bt addr */
int at_cmd_bt_addr(void);

/* 9. data of adv */
int at_cmd_adv_data_set(const char *p_para);
int at_cmd_adv_data_get(void);

/* 10. uuid of data srv */
int at_cmd_data_srv_uuid_set(const char *p_para);
int at_cmd_data_srv_uuid_get(void);

/* 11. uuid of drv srv */
int at_cmd_drv_srv_uuid_set(const char *p_para);
int at_cmd_drv_srv_uuid_get(void);

/* 12. uuid of data tx char */
int at_cmd_data_tx_char_uuid_set(const char *p_para);
int at_cmd_data_tx_char_uuid_get(void);

/* 13. uuid of data rx char */
int at_cmd_data_rx_char_uuid_set(const char *p_para);
int at_cmd_data_rx_char_uuid_get(void);

/* 14. uuid of drv adc char */
int at_cmd_drv_adc_char_uuid_set(const char *p_para);
int at_cmd_drv_adc_char_uuid_get(void);

/* 15. uuid of drv pwm char */
int at_cmd_drv_pwm_char_uuid_set(const char *p_para);
int at_cmd_drv_pwm_char_uuid_get(void);

/* 16. version information */
int at_cmd_ver_info(void);

/* 17. rssi  */
int at_cmd_rssi_set(const char *p_para);
int at_cmd_rssi_get(void);

/* 18. RTS func choose */
int at_cmd_rts_func_set(const char *p_para);
int at_cmd_rts_func_get(void);

void app_cfg_event_handle(u32_t event);

#endif
