/** @file
 *  @brief Bluetooth UUID handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_UUID_H
#define __BT_UUID_H

/**
 * @brief UUIDs
 * @defgroup bt_uuid UUIDs
 * @ingroup bluetooth
 * @{
 */

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bluetooth UUID types */
enum {
	BT_UUID_TYPE_16,
	BT_UUID_TYPE_32,
	BT_UUID_TYPE_128,
};

/** @brief This is a 'tentative' type and should be used as a pointer only */
struct bt_uuid {
	u8_t type;
};

struct bt_uuid_16 {
	struct bt_uuid uuid;
	u16_t val;
};

struct bt_uuid_32 {
	struct bt_uuid uuid;
	u32_t val;
};

struct bt_uuid_128 {
	struct bt_uuid uuid;
	u8_t val[16];
};

#define BT_UUID_INIT_16(value)		\
{					\
	.uuid.type = BT_UUID_TYPE_16,	\
	.val = (value),			\
}

#define BT_UUID_INIT_32(value)		\
{					\
	.uuid.type = BT_UUID_TYPE_32,	\
	.val = (value),			\
}

#define BT_UUID_INIT_128(value...)	\
{					\
	.uuid.type = BT_UUID_TYPE_128,	\
	.val = { value },		\
}

#define BT_UUID_DECLARE_16(value) \
	((struct bt_uuid *) (&(struct bt_uuid_16) BT_UUID_INIT_16(value)))
#define BT_UUID_DECLARE_32(value) \
	((struct bt_uuid *) (&(struct bt_uuid_32) BT_UUID_INIT_32(value)))
#define BT_UUID_DECLARE_128(value...) \
	((struct bt_uuid *) (&(struct bt_uuid_128) BT_UUID_INIT_128(value)))

#define BT_UUID_16(__u) CONTAINER_OF(__u, struct bt_uuid_16, uuid)
#define BT_UUID_32(__u) CONTAINER_OF(__u, struct bt_uuid_32, uuid)
#define BT_UUID_128(__u) CONTAINER_OF(__u, struct bt_uuid_128, uuid)

/** @def BT_UUID_GAP
 *  @brief Generic Access
 */
#define BT_UUID_GAP                       BT_UUID_DECLARE_16(0x1800)
#define BT_UUID_GAP_VAL                   0x1800
/** @def BT_UUID_GATT
 *  @brief Generic Attribute
 */
#define BT_UUID_GATT                      BT_UUID_DECLARE_16(0x1801)
#define BT_UUID_GATT_VAL                  0x1801
/** @def BT_UUID_ANS
 *  @brief GATT Alert Notification Service
 */
#define BT_UUID_ANS                  BT_UUID_DECLARE_16(0x1811)
#define BT_UUID_ANS_VAL              0x1811
/** @def BT_UUID_CTS
 *  @brief Current Time Service
 */
#define BT_UUID_CTS                       BT_UUID_DECLARE_16(0x1805)
#define BT_UUID_CTS_VAL                   0x1805
/** @def BT_UUID_DIS
 *  @brief Device Information Service
 */
#define BT_UUID_DIS                       BT_UUID_DECLARE_16(0x180a)
#define BT_UUID_DIS_VAL                   0x180a
/** @def BT_UUID_HRS
 *  @brief Heart Rate Service
 */
#define BT_UUID_HRS                       BT_UUID_DECLARE_16(0x180d)
#define BT_UUID_HRS_VAL                   0x180d
/** @def BT_UUID_RTUS
 *  @brief Reference Time Update Service
 */
#define BT_UUID_RTUS                       BT_UUID_DECLARE_16(0x1806)
#define BT_UUID_RTUS_VAL                   0x1806
/** @def BT_UUID_NDCS
 *  @brief Next DST Change Service
 */
#define BT_UUID_NDCS                       BT_UUID_DECLARE_16(0x1807)
#define BT_UUID_NDCS_VAL               0x1807
/** @def BT_UUID_BAS
 *  @brief Battery Service
 */
#define BT_UUID_BAS                       BT_UUID_DECLARE_16(0x180f)
#define BT_UUID_BAS_VAL                   0x180f
/** @def BT_UUID_BCS
 *  @brief Body Composition Service
 */
#define BT_UUID_BCS                                                                                    BT_UUID_DECLARE_16(0x181b)
#define BT_UUID_BCS_VAL                                                                            0x181b
/** @def BT_UUID_BLS
 *  @brief Blood Pressure Service
 */
#define BT_UUID_BLS                       BT_UUID_DECLARE_16(0x1810)
#define BT_UUID_BLS_VAL                   0x1810
/** @def BT_UUID_GLS
 *  @brief Glucose Service
 */
#define BT_UUID_GLS                                  BT_UUID_DECLARE_16(0x1808)
#define BT_UUID_GLS_VAL                              0x1808
/** @def BT_UUID_HIDS
 *  @brief HID Service
 */
#define BT_UUID_HIDS                      BT_UUID_DECLARE_16(0x1812)
#define BT_UUID_HIDS_VAL                  0x1812
/** @def BT_UUID_IAS
 *  @brief Immediate Alert Service
 */
#define BT_UUID_IAS                       BT_UUID_DECLARE_16(0x1802)
#define BT_UUID_IAS_VAL                   0x1802
/** @def BT_UUID_LLS
 *  @brief Link Loss Service
 */
#define BT_UUID_LLS                       BT_UUID_DECLARE_16(0x1803)
#define BT_UUID_LLS_VAL                   0x1803
/** @def BT_UUID_LNS
 *  @brief Location and Navigation Service
 */
#define BT_UUID_LNS                       BT_UUID_DECLARE_16(0x1819)
#define BT_UUID_LNS_VAL                   0x1819
/** @def BT_UUID_PASS
 *  @brief Phone Alert Status Service
 */
#define BT_UUID_PASS                       BT_UUID_DECLARE_16(0x180e)
#define BT_UUID_PASS_VAL               0x180e
/** @def BT_UUID_SCPS
 *  @brief SCPS Scan Parameters Service
 */
#define BT_UUID_SCPS                       BT_UUID_DECLARE_16(0x1813)
#define BT_UUID_SCPS_VAL                   0x1813
/** @def BT_UUID_TPS
 *  @brief Tx Power Service
 */
#define BT_UUID_TPS                       BT_UUID_DECLARE_16(0x1804)
#define BT_UUID_TPS_VAL                   0x1804
/** @def BT_UUID_UDS
 *  @brief User Data Service
 */
#define BT_UUID_UDS             BT_UUID_DECLARE_16(0x181C)
#define BT_UUID_UDS_VAL     0x181C
/** @def BT_UUID_CSC
 *  @brief Cycling Speed and Cadence Service
 */
#define BT_UUID_CSC                       BT_UUID_DECLARE_16(0x1816)
#define BT_UUID_CSC_VAL                   0x1816
/** @def BT_UUID_CGMS
 *  @brief Running Speed and Cadence
 */
#define BT_UUID_CGMS                       BT_UUID_DECLARE_16(0x181f)
#define BT_UUID_CGMS_VAL                   0x181f
/** @def BT_UUID_ESS
 *  @brief Environmental Sensing Service
 */
#define BT_UUID_ESS                       BT_UUID_DECLARE_16(0x181a)
#define BT_UUID_ESS_VAL                   0x181a
/** @def BT_UUID_IPSS
 *  @brief IP Support Service
 */
#define BT_UUID_IPSS                      BT_UUID_DECLARE_16(0x1820)
#define BT_UUID_IPSS_VAL                  0x1820
/** @def BT_UUID_MESH_PROV
 *  @brief Mesh Provisioning Service
 */
#define BT_UUID_MESH_PROV                 BT_UUID_DECLARE_16(0x1827)
#define BT_UUID_MESH_PROV_VAL             0x1827
/** @def BT_UUID_MESH_PROXY
 *  @brief Mesh Proxy Service
 */
#define BT_UUID_MESH_PROXY                BT_UUID_DECLARE_16(0x1828)
#define BT_UUID_MESH_PROXY_VAL            0x1828
/** @def BT_UUID_GATT_PRIMARY
 *  @brief GATT Primary Service
 */
#define BT_UUID_GATT_PRIMARY              BT_UUID_DECLARE_16(0x2800)
#define BT_UUID_GATT_PRIMARY_VAL          0x2800
/** @def BT_UUID_GATT_SECONDARY
 *  @brief GATT Secondary Service
 */
#define BT_UUID_GATT_SECONDARY            BT_UUID_DECLARE_16(0x2801)
#define BT_UUID_GATT_SECONDARY_VAL        0x2801
/** @def BT_UUID_GATT_INCLUDE
 *  @brief GATT Include Service
 */
#define BT_UUID_GATT_INCLUDE              BT_UUID_DECLARE_16(0x2802)
#define BT_UUID_GATT_INCLUDE_VAL          0x2802
/** @def BT_UUID_GATT_CHRC
 *  @brief GATT Characteristic
 */
#define BT_UUID_GATT_CHRC                 BT_UUID_DECLARE_16(0x2803)
#define BT_UUID_GATT_CHRC_VAL             0x2803
/** @def BT_UUID_GATT_CEP
 *  @brief GATT Characteristic Extended Properties
 */
#define BT_UUID_GATT_CEP                  BT_UUID_DECLARE_16(0x2900)
#define BT_UUID_GATT_CEP_VAL              0x2900
/** @def BT_UUID_GATT_CUD
 *  @brief GATT Characteristic User Description
 */
#define BT_UUID_GATT_CUD                  BT_UUID_DECLARE_16(0x2901)
#define BT_UUID_GATT_CUD_VAL              0x2901
/** @def BT_UUID_GATT_CCC
 *  @brief GATT Client Characteristic Configuration
 */
#define BT_UUID_GATT_CCC                  BT_UUID_DECLARE_16(0x2902)
#define BT_UUID_GATT_CCC_VAL              0x2902
/** @def BT_UUID_GATT_SCC
 *  @brief GATT Server Characteristic Configuration
 */
#define BT_UUID_GATT_SCC                  BT_UUID_DECLARE_16(0x2903)
#define BT_UUID_GATT_SCC_VAL              0x2903
/** @def BT_UUID_GATT_CPF
 *  @brief GATT Characteristic Presentation Format
 */
#define BT_UUID_GATT_CPF                  BT_UUID_DECLARE_16(0x2904)
#define BT_UUID_GATT_CPF_VAL              0x2904
/** @def BT_UUID_WSS
 *  @brief Weight Scale Service
 */
#define BT_UUID_WSS                       BT_UUID_DECLARE_16(0x181D)
#define BT_UUID_WSS_VAL                   0x181D
/** @def BT_UUID_VALID_RANGE
 *  @brief Valid Range Descriptor
 */
#define BT_UUID_VALID_RANGE               BT_UUID_DECLARE_16(0x2906)
#define BT_UUID_VALID_RANGE_VAL           0x2906
/** @def BT_UUID_HIDS_EXT_REPORT
 *  @brief HID External Report Descriptor
 */
#define BT_UUID_HIDS_EXT_REPORT           BT_UUID_DECLARE_16(0x2907)
#define BT_UUID_HIDS_EXT_REPORT_VAL       0x2907
/** @def BT_UUID_HIDS_REPORT_REF
 *  @brief HID Report Reference Descriptor
 */
#define BT_UUID_HIDS_REPORT_REF           BT_UUID_DECLARE_16(0x2908)
#define BT_UUID_HIDS_REPORT_REF_VAL       0x2908
/** @def BT_UUID_IAS_ALERT_LEVEL
 *  @brief IAS Alert Level
 */
#define BT_UUID_IAS_ALERT_LEVEL           BT_UUID_DECLARE_16(0x2A06)
#define BT_UUID_IAS_ALERT_LEVEL_VAl       0x2A06
/** @def BT_UUID_TPS_TX_POWER_LEVEL
 *  @brief TPS Tx Power Level
 */
#define BT_UUID_TPS_TX_POWER_LEVEL                       BT_UUID_DECLARE_16(0x2a07)
#define BT_UUID_TPS_TX_POWER_LEVEL_VAL                   0x2a07
/** @def BT_UUID_NDCS_TIME_WITH_DST
 *  @brief NDCS Time With DST
 */
#define BT_UUID_NDCS_TIME_WITH_DST                       BT_UUID_DECLARE_16(0x2A11)
#define BT_UUID_NDCS_TIME_WITH_DST_VAL               0x2A11
/** @def BT_UUID_HTS
 *  @brief Health Thermometer Service
 */
#define BT_UUID_HTS                       BT_UUID_DECLARE_16(0x1809)
#define BT_UUID_HTS_VAL                   0x1809
/** @def BT_UUID_ES_CONFIGURATION
 *  @brief Environmental Sensing Configuration Descriptor
 */
#define BT_UUID_ES_CONFIGURATION          BT_UUID_DECLARE_16(0x290b)
#define BT_UUID_ES_CONFIGURATION_VAL      0x290b
/** @def BT_UUID_ES_MEASUREMENT
 *  @brief Environmental Sensing Measurement Descriptor
 */
#define BT_UUID_ES_MEASUREMENT            BT_UUID_DECLARE_16(0x290c)
#define BT_UUID_ES_MEASUREMENT_VAL        0x290c
/** @def BT_UUID_ES_TRIGGER_SETTING
 *  @brief Environmental Sensing Trigger Setting Descriptor
 */
#define BT_UUID_ES_TRIGGER_SETTING        BT_UUID_DECLARE_16(0x290d)
#define BT_UUID_ES_TRIGGER_SETTING_VAL    0x290d
/** @def BT_UUID_GAP_DEVICE_NAME
 *  @brief GAP Characteristic Device Name
 */
#define BT_UUID_GAP_DEVICE_NAME           BT_UUID_DECLARE_16(0x2a00)
#define BT_UUID_GAP_DEVICE_NAME_VAL       0x2a00
/** @def BT_UUID_GAP_APPEARANCE
 *  @brief GAP Characteristic Appearance
 */
#define BT_UUID_GAP_APPEARANCE            BT_UUID_DECLARE_16(0x2a01)
#define BT_UUID_GAP_APPEARANCE_VAL        0x2a01
/** @def BT_UUID_GAP_PPCP
 *  @brief GAP Characteristic Peripheral Preferred Connection Parameters
 */
#define BT_UUID_GAP_PPCP                  BT_UUID_DECLARE_16(0x2a04)
#define BT_UUID_GAP_PPCP_VAL              0x2a04
/** @def BT_UUID_GATT_SC
 *  @brief GATT Characteristic Service Changed
 */
#define BT_UUID_GATT_SC                   BT_UUID_DECLARE_16(0x2a05)
#define BT_UUID_GATT_SC_VAL               0x2a05
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_BAS_BATTERY_LEVEL         BT_UUID_DECLARE_16(0x2a19)
#define BT_UUID_BAS_BATTERY_LEVEL_VAL     0x2a19
/** @def BT_UUID_BCS_BODY_COMPOSITION_FEATURE
 *  @brief BCS Body Composition Feature
 */
#define BT_UUID_BCS_BODY_COMPOSITION_FEATURE                                 BT_UUID_DECLARE_16(0x2a9b)
#define BT_UUID_BCS_BODY_COMPOSITION_FEATURE_VAL                         0x2a9b
/** @def BT_UUID_BCS_BODY_COMPOSITION_MEASUREMENT
 *  @brief BCS Body Composition Measurement
 */
#define BT_UUID_BCS_BODY_COMPOSITION_MEASUREMENT                        BT_UUID_DECLARE_16(0x2a9c)
#define BT_UUID_BCS_BODY_COMPOSITION_MEASUREMENT_VAL                0x2a9c
/** @def BT_UUID_RSC_MEASUREMENT
 *  @brief CGMS CGM Measurement
 */
#define BT_UUID_CGMS_CGM_MEASUREMENT           BT_UUID_DECLARE_16(0x2aa7)
#define BT_UUID_CGMS_CGM_MEASUREMENT_VAL       0x2aa7
/** @def BT_UUID_CGMS_CGM_FEATURE
 *  @brief CGMS CGM Feature
 */
#define BT_UUID_CGMS_CGM_FEATURE           BT_UUID_DECLARE_16(0x2aa8)
#define BT_UUID_CGMS_CGM_FEATURE_VAL       0x2aa8
/** @def BT_UUID_CGMS_CGM_STATUS
 *  @brief CGMS CGM Status
 */
#define BT_UUID_CGMS_CGM_STATUS           BT_UUID_DECLARE_16(0x2aa9)
#define BT_UUID_CGMS_CGM_STATUS_VAL       0x2aa9
/** @def BT_UUID_CGMS_CGM_SESSION_START_TIME
 *  @brief CGMS CGM Session Start Time
 */
#define BT_UUID_CGMS_CGM_SESSION_START_TIME           BT_UUID_DECLARE_16(0x2aaa)
#define BT_UUID_CGMS_CGM_SESSION_START_TIME_VAL       0x2aaa
/** @def BT_UUID_CGMS_CGM_SESSION_RUN_TIME
 *  @brief CGMS CGM Session Run Time
 */
#define BT_UUID_CGMS_CGM_SESSION_RUN_TIME           BT_UUID_DECLARE_16(0x2aab)
#define BT_UUID_CGMS_CGM_SESSION_RUN_TIME_VAL       0x2aab
/** @def BT_UUID_CGMS_RECORD_ACCESS_CONTROL_POINT
 *  @brief CGMS Record Access Control Point
 */
#define BT_UUID_CGMS_RECORD_ACCESS_CONTROL_POINT      BT_UUID_DECLARE_16(0x2a52)
#define BT_UUID_CGMS_RECORD_ACCESS_CONTROL_POINT_VAL  0x2a52
/** @def BT_UUID_CGMS_CGM_SPECIFIC_OPS_CONTROL_POINT
 *  @brief CGMS  CGM Specific Ops Control Point
 */
#define BT_UUID_CGMS_CGM_SPECIFIC_OPS_CONTROL_POINT          BT_UUID_DECLARE_16(0x2aac)
#define BT_UUID_CGMS_CGM_SPECIFIC_OPS_CONTROL_POINT_VAL       0x2aac
/** @def BT_UUID_DIS_SYSTEM_ID
 *  @brief DIS Characteristic System ID
 */
#define BT_UUID_DIS_SYSTEM_ID             BT_UUID_DECLARE_16(0x2a23)
#define BT_UUID_DIS_SYSTEM_ID_VAL         0x2a23
/** @def BT_UUID_DIS_MODEL_NUMBER
 *  @brief DIS Characteristic Model Number String
 */
#define BT_UUID_DIS_MODEL_NUMBER          BT_UUID_DECLARE_16(0x2a24)
#define BT_UUID_DIS_MODEL_NUMBER_VAL      0x2a24
/** @def BT_UUID_DIS_SERIAL_NUMBER
 *  @brief DIS Characteristic Serial Number String
 */
#define BT_UUID_DIS_SERIAL_NUMBER         BT_UUID_DECLARE_16(0x2a25)
#define BT_UUID_DIS_SERIAL_NUMBER_VAL     0x2a25
/** @def BT_UUID_DIS_FIRMWARE_REVISION
 *  @brief DIS Characteristic Firmware Revision String
 */
#define BT_UUID_DIS_FIRMWARE_REVISION     BT_UUID_DECLARE_16(0x2a26)
#define BT_UUID_DIS_FIRMWARE_REVISION_VAL 0x2a26
/** @def BT_UUID_DIS_HARDWARE_REVISION
 *  @brief DIS Characteristic Hardware Revision String
 */
#define BT_UUID_DIS_HARDWARE_REVISION     BT_UUID_DECLARE_16(0x2a27)
#define BT_UUID_DIS_HARDWARE_REVISION_VAL 0x2a27
/** @def BT_UUID_DIS_SOFTWARE_REVISION
 *  @brief DIS Characteristic Software Revision String
 */
#define BT_UUID_DIS_SOFTWARE_REVISION     BT_UUID_DECLARE_16(0x2a28)
#define BT_UUID_DIS_SOFTWARE_REVISION_VAL 0x2a28
/** @def BT_UUID_DIS_MANUFACTURER_NAME
 *  @brief DIS Characteristic Manufacturer Name String
 */
#define BT_UUID_DIS_MANUFACTURER_NAME     BT_UUID_DECLARE_16(0x2a29)
#define BT_UUID_DIS_MANUFACTURER_NAME_VAL 0x2a29
/** @def BT_UUID_DIS_PNP_ID
 *  @brief DIS Characteristic PnP ID
 */
#define BT_UUID_DIS_PNP_ID                BT_UUID_DECLARE_16(0x2a50)
#define BT_UUID_DIS_PNP_ID_VAL            0x2a50
/** @def BT_UUID_DIS_IEEE_REGULATORY_CERTIFICATION_DATA_LIST
 *  @brief DIS Characteristic IEEE 11073-20601 Regulatory Certification Data List
 */
#define BT_UUID_DIS_IEEE_REGULATORY_CERTIFICATION_DATA_LIST  BT_UUID_DECLARE_16(0x2A2A)
#define BT_UUID_DIS_IEEE_REGULATORY_CERTIFICATION_DATA_LIST_VAL  0x2A2A
/** @def BT_UUID_LLS_ALERT_LEVEL
 *  @brief LLS Alert Level
 */
#define BT_UUID_LLS_ALERT_LEVEL           BT_UUID_DECLARE_16(0x2A06)
#define BT_UUID_LLS_ALERT_LEVEL_VAl       0x2A06
/** @def BT_UUID_LN_FEAT
 *  @brief LNS LN Feature
 */
#define BT_UUID_LNS_LN_FEATURE                       BT_UUID_DECLARE_16(0x2a6a)
#define BT_UUID_LNS_LN_FEATURE_VAL                   0x2a6a
/** @def BT_UUID_LOCATION_AND_SPEED
 *  @brief LNS Location and Speed
 */
#define BT_UUID_LNS_LOCATION_AND_SPEED                       BT_UUID_DECLARE_16(0x2a67)
#define BT_UUID_LNS_LOCATION_AND_SPEED_VAL                   0x2a67
/** @def BT_UUID_LNS_POSITION_QUALITY
 *  @brief LNS Position Quality
 */
#define BT_UUID_LNS_POSITION_QUALITY                      BT_UUID_DECLARE_16(0x2a69)
#define BT_UUID_LNS_POSITION_QUALITY_VAL                   0x2a69
/** @def BT_UUID_LNS_LN_CONTROL_POINT
 *  @brief LNS LN Control Point
 */
#define BT_UUID_LNS_LN_CONTROL_POINT                      BT_UUID_DECLARE_16(0x2a6b)
#define BT_UUID_LNS_LN_CONTROL_POINT_VAL                   0x2a6b
/** @def BT_UUID_LNS_NAVIGATION
 *  @brief LNS Navigation
 */
#define BT_UUID_LNS_NAVIGATION                      BT_UUID_DECLARE_16(0x2a68)
#define BT_UUID_LNS_NAVIGATION_VAL                   0x2a68
/** @def BT_UUID_ANS_SUPPORTED_NEW_ALERT_CATEGORY
 *  @brief ANS Supported New Alert Category
 */
#define BT_UUID_ANS_SUPPORTED_NEW_ALERT_CATEGORY                  BT_UUID_DECLARE_16(0x2A47)
#define BT_UUID_ANS_SUPPORTED_NEW_ALERT_CATEGORY_VAL              0x2A47
/** @def BT_UUID_ANS_NEW_ALERT
 *  @brief  ANS Supported New Alert Category 
 */
#define BT_UUID_ANS_NEW_ALERT                  BT_UUID_DECLARE_16(0x2A46)
#define BT_UUID_ANS_NEW_ALERT_VAL              0x2A46
/** @def BT_UUID_ANS_SUPPORTED_NEW_ALERT_CATEGORY
 *  @brief ANS Supported Unread Alert Category
 */
#define BT_UUID_ANS_SUPPORTED_UNREAD_ALERT_CATEGORY                  BT_UUID_DECLARE_16(0x2A48)
#define BT_UUID_ANS_SUPPORTED_UNREAD_ALERT_CATEGORY_VAL              0x2A48
/** @def BT_UUID_ANS_UNREAD_ALERT_STATUS
 *  @brief ANS Unread Alert Status
 */
#define BT_UUID_ANS_UNREAD_ALERT_STATUS                  BT_UUID_DECLARE_16(0x2A45)
#define BT_UUID_ANS_UNREAD_ALERT_STATUS_VAL              0x2A45
/** @def BT_UUID_ANS_ALERT_NOTIFICATION_CONTROL_POINT
 *  @brief ANS Alert Notification Control Point
 */
#define BT_UUID_ANS_ALERT_NOTIFICATION_CONTROL_POINT                 BT_UUID_DECLARE_16(0x2A44)
#define BT_UUID_ANS_ALERT_NOTIFICATION_CONTROL_POINT_VAL              0x2A44
/** @def BT_UUID_BPM
 *  @brief BLS Blood Pressure Measurement
 */
#define BT_UUID_BPM                       BT_UUID_DECLARE_16(0x2A35)
#define BT_UUID_BPM_VAL                   0x2A35
/** @def BT_UUID_INTERMEDIATE_CUFF_PRESSURE
 *  @brief BLS Intermediate Cuff Pressure
 */
#define BT_UUID_INTERMEDIATE_CUFF_PRESSURE                       BT_UUID_DECLARE_16(0x2A36)
#define BT_UUID_INTERMEDIATE_CUFF_PRESSURE_VAL                   0x2A36
/** @def BT_UUID_BLOOD_PRESSURE_FEATURE
 *  @brief BLS Blood Pressure Feature
 */
#define BT_UUID_BLOOD_PRESSURE_FEATURE                       BT_UUID_DECLARE_16(0x2A49)
#define BT_UUID_BLOOD_PRESSURE_FEATURE_VAL                   0x2A49
/** @def BT_UUID_HTS_TEMPERATURE_MEASUREMENT
 *  @brief HTS Characteristic Temperature Measurement
 */
#define BT_UUID_HTS_TEMPERATURE_MEASUREMENT     BT_UUID_DECLARE_16(0x2A1C)
#define BT_UUID_HTS_TEMPERATURE_MEASUREMENT_VAL 0x2A1C
/** @def BT_UUID_HTS_TEMPERATURE_TYPE
 *  @brief HTS Characteristic Temperature Type
 */
#define BT_UUID_HTS_TEMPERATURE_TYPE     BT_UUID_DECLARE_16(0x2A1D)
#define BT_UUID_HTS_TEMPERATURE_TYPE_VAL 0x2A1D
/** @def BT_UUID_HTS_INTERMEDIATE_TEMPERATURE
 *  @brief HTS Characteristic Intermediate Temperature
 */
#define BT_UUID_HTS_INTERMEDIATE_TEMPERATURE     BT_UUID_DECLARE_16(0x2A1E)
#define BT_UUID_HTS_INTERMEDIATE_TEMPERATURE_VAL 0x2A1E
/** @def BT_UUID_HTS_MEASUREMENT_INTERVAL
 *  @brief HTS Characteristic Measurement Interval
 */
#define BT_UUID_HTS_MEASUREMENT_INTERVAL     BT_UUID_DECLARE_16(0x2A21)
#define BT_UUID_HTS_MEASUREMENT_INTERVAL_VAL 0x2A21
/** @def BT_UUID_HTS_VALID_RANGE
 *  @brief HTS Characteristic Valid Range
 */
#define BT_UUID_HTS_VALID_RANGE     BT_UUID_DECLARE_16(0x2906)
#define BT_UUID_HTS_VALID_RANGE_VAL 0x2906
/** @def BT_UUID_GLS_GLUCOSE_MEASUREMENT
 *  @brief GLS Glucose Measurement
 */
#define BT_UUID_GLS_GLUCOSE_MEASUREMENT              BT_UUID_DECLARE_16(0x2a18)
#define BT_UUID_GLS_GLUCOSE_MEASUREMENT_VAL          0x2a18

/** @def BT_UUID_GLS_GLUCOSE_MEASUREMENT_CONTEXT
 *  @brief GLS Glucose Measurement Context
 */
#define BT_UUID_GLS_GLUCOSE_MEASUREMENT_CONTEXT      BT_UUID_DECLARE_16(0x2a34)
#define BT_UUID_GLS_GLUCOSE_MEASUREMENT_CONTEXT_VAL  0x2a34
/** @def BT_UUID_GLS_GLUCOSE_FEATURE
 *  @brief GLS Glucose Feature
 */
#define BT_UUID_GLS_GLUCOSE_FEATURE                  BT_UUID_DECLARE_16(0x2a51)
#define BT_UUID_GLS_GLUCOSE_FEATURE_VAL              0x2a51

/** @def BT_UUID_GLS_RECORD_ACCESS_CONTROL_POINT
 *  @brief GLS Record Access Control Point
 */
#define BT_UUID_GLS_RECORD_ACCESS_CONTROL_POINT      BT_UUID_DECLARE_16(0x2a52)
#define BT_UUID_GLS_RECORD_ACCESS_CONTROL_POINT_VAL  0x2a52
/** @def BT_UUID_CTS_CURRENT_TIME
 *  @brief CTS Characteristic Current Time
 */
#define BT_UUID_CTS_CURRENT_TIME          BT_UUID_DECLARE_16(0x2a2b)
#define BT_UUID_CTS_CURRENT_TIME_VAL      0x2a2b
/** @def BT_UUID_CTS_CURRENT_TIME
 *  @brief CTS Characteristic Local Time Info
 */
#define BT_UUID_CTS_LOCAL_TIMR_INFO                     BT_UUID_DECLARE_16(0x2a0f)
#define BT_UUID_CTS_LOCAL_TIMR_INFO_VAL             0x2a0f
/** @def BT_UUID_CTS_CURRENT_TIME
 *  @brief CTS Reference Time Info
 */
#define BT_UUID_CTS_REFERENCE_TIME_INFO              BT_UUID_DECLARE_16(0x2a14)
#define BT_UUID_CTS_REFERENCE_TIME_INFO_VAL      0x2a14
/** @def BT_UUID_MAGN_DECLINATION
 *  @brief Magnetic Declination Characteristic
 */
#define BT_UUID_MAGN_DECLINATION          BT_UUID_DECLARE_16(0x2a2c)
#define BT_UUID_MAGN_DECLINATION_VAL      0x2a2c
/** @def BT_UUID_HRS_MEASUREMENT
 *  @brief HRS Characteristic Measurement Interval
 */
#define BT_UUID_HRS_MEASUREMENT           BT_UUID_DECLARE_16(0x2a37)
#define BT_UUID_HRS_MEASUREMENT_VAL       0x2a37
/** @def BT_UUID_HRS_BODY_SENSOR
 *  @brief HRS Characteristic Body Sensor Location
 */
#define BT_UUID_HRS_BODY_SENSOR           BT_UUID_DECLARE_16(0x2a38)
#define BT_UUID_HRS_BODY_SENSOR_VAL       0x2a38
/** @def BT_UUID_HRS_CONTROL_POINT
 *  @brief HRS Characteristic Control Point
 */
#define BT_UUID_HRS_CONTROL_POINT         BT_UUID_DECLARE_16(0x2a39)
#define BT_UUID_HRS_CONTROL_POINT_VAL     0x2a39


/** @def BT_UUID_HIDS_BOOT_KEYBOARD_IN
 *  @brief HID HID Boot Keyboard In Characteristic 
 */
#define BT_UUID_HIDS_BOOT_KEYBOARD_IN         BT_UUID_DECLARE_16(0x2a22)
#define BT_UUID_HIDS_BOOT_KEYBOARD_IN_VAL     0x2a22


/** @def BT_UUID_HIDS_BOOT_KEYBOARD_OUT
 *  @brief HID HID Boot Keyboard Out Characteristic 
 */
#define BT_UUID_HIDS_BOOT_KEYBOARD_OUT         BT_UUID_DECLARE_16(0x2a32)
#define BT_UUID_HIDS_BOOT_KEYBOARD_OUT_VAL     0x2a32


/** @def BT_UUID_HIDS_BOOT_MOUSE_IN
 *  @brief HID Boot Mouse In Characteristic 
 */
#define BT_UUID_HIDS_BOOT_MOUSE_IN         BT_UUID_DECLARE_16(0x2a33)
#define BT_UUID_HIDS_BOOT_MOUSE_IN_VAL     0x2a33

/** @def BT_UUID_HIDS_INFO
 *  @brief HID Information Characteristic
 */
#define BT_UUID_HIDS_INFO                 BT_UUID_DECLARE_16(0x2a4a)
#define BT_UUID_HIDS_INFO_VAL             0x2a4a
/** @def BT_UUID_HIDS_REPORT_MAP
 *  @brief HID Report Map Characteristic
 */
#define BT_UUID_HIDS_REPORT_MAP           BT_UUID_DECLARE_16(0x2a4b)
#define BT_UUID_HIDS_REPORT_MAP_VAL       0x2a4b
/** @def BT_UUID_HIDS_CTRL_POINT
 *  @brief HID Control Point Characteristic
 */
#define BT_UUID_HIDS_CTRL_POINT           BT_UUID_DECLARE_16(0x2a4c)
#define BT_UUID_HIDS_CTRL_POINT_VAL       0x2a4c
/** @def BT_UUID_HIDS_REPORT
 *  @brief HID Report Characteristic
 */
#define BT_UUID_HIDS_REPORT               BT_UUID_DECLARE_16(0x2a4d)
#define BT_UUID_HIDS_REPORT_VAL           0x2a4d
/** @def BT_UUID_HIDS_PROTOCOL_MODE
 *  @brief HID Protocol mode Characteristic
 */
#define BT_UUID_HIDS_PROTOCOL_MODE        BT_UUID_DECLARE_16(0x2a4e)
#define BT_UUID_HIDS_PROTOCOL_MODE_VAL    0x2a4e
/** @def BT_UUID_PASS_ALERT_STATUS
 *  @brief PASS Alert Status
 */
#define BT_UUID_PASS_ALERT_STATUS                       BT_UUID_DECLARE_16(0x2a3f)
#define BT_UUID_PASS_ALERT_STATUS_VAL               0x2a3f
/** @def BT_UUID_PASS_RINGER_SETTING
 *  @brief PASS Ringer_Setting
 */
#define BT_UUID_PASS_RINGER_SETTING                       BT_UUID_DECLARE_16(0x2a41)
#define BT_UUID_PASS_RINGER_SETTING_VAL               0x2a41
/** @def BT_UUID_PASS_RINGER_CONTROL_POINT
 *  @brief PASS Ringer Control point
 */
#define BT_UUID_PASS_RINGER_CONTROL_POINT                       BT_UUID_DECLARE_16(0x2a40)
#define BT_UUID_PASS_RINGER_CONTROL_POINT_VAL               0x2a40
/** @def BT_UUID_SCPS_SCAN_INTERVAL_WINDOW
 *  @brief SCPS Scan Interval Window
 */
#define BT_UUID_SCPS_SCAN_INTERVAL_WINDOW                       BT_UUID_DECLARE_16(0x2a4f)
#define BT_UUID_SCPS_SCAN_INTERVAL_WINDOW_VAL                   0x2a4f
 /** @def BT_UUID_SCPS_SCAN_INTERVAL_WINDOW
 *  @brief SCPS Scan Refresh
 */
#define BT_UUID_SCPS_SCAN_REFRESH                       BT_UUID_DECLARE_16(0x2a31)
#define BT_UUID_SCPS_SCAN_REFRESH_VAL                   0x2a31
/** @def BT_UUID_UDS_FIRST_NAME
 *  @brief UDS Characteristic First Name
 */
#define BT_UUID_UDS_FIRST_NAME             BT_UUID_DECLARE_16(0x2a8a)
#define BT_UUID_UDS_FIRST_NAME_VAL     0x2a8a
/** @def BT_UUID_UDS_LAST_NAME
 *  @brief UDS Characteristic Last Name
 */
#define BT_UUID_UDS_LAST_NAME             BT_UUID_DECLARE_16(0x2a90)
#define BT_UUID_UDS_LAST_NAME_VAL     0x2a90
/** @def BT_UUID_UDS_EMAIL_ADDRESS
 *  @brief UDS Characteristic Email Address
 */
#define BT_UUID_UDS_EMAIL_ADDRESS             BT_UUID_DECLARE_16(0x2a87)
#define BT_UUID_UDS_EMAIL_ADDRESS_VAL     0x2a87
#define BT_UUID_UDS_AGE             BT_UUID_DECLARE_16(0x2a80)
#define BT_UUID_UDS_AGE_VAL     0x2a80
/** @def BT_UUID_UDS_DATE_OF_BIRTH
 *  @brief UDS Date Of Birth
 */
#define BT_UUID_UDS_DATE_OF_BIRTH             BT_UUID_DECLARE_16(0x2a85)
#define BT_UUID_UDS_DATE_OF_BIRTH_VAL     0x2a85
/** @def BT_UUID_UDS_GENDER
 *  @brief UDS Gender
 */
#define BT_UUID_UDS_GENDER             BT_UUID_DECLARE_16(0x2a8c)
#define BT_UUID_UDS_GENDER_VAL     0x2a8c
/** @def BT_UUID_UDS_DATABASE_CHANGE_INCREMENT
 *  @brief UDS Characteristic Database change increment
 */
#define BT_UUID_UDS_DATABASE_CHANGE_INCREMENT             BT_UUID_DECLARE_16(0x2a99)
#define BT_UUID_UDS_DATABASE_CHANGE_INCREMENT_VAL  0x2a99
/** @def BT_UUID_UDS_USER_INDEX
 *  @brief UDS Characteristic User Index
 */
#define BT_UUID_UDS_USER_INDEX             BT_UUID_DECLARE_16(0x2a9a)
#define BT_UUID_UDS_USER_INDEX_VAL  0x2a9a
/** @def BT_UUID_UDS_USER_CONTROL_POINT
 *  @brief UDS Characteristic User Control Ponit
 */
#define BT_UUID_UDS_USER_CONTROL_POINT             BT_UUID_DECLARE_16(0x2A9F)
#define BT_UUID_UDS_USER_CONTROL_POINT_VAL 0x2A9F
/** @def BT_UUID_CSC_MEASUREMENT
 *  @brief CSC Measurement Characteristic
 */
#define BT_UUID_CSC_MEASUREMENT           BT_UUID_DECLARE_16(0x2a5b)
#define BT_UUID_CSC_MEASUREMENT_VAL       0x2a5b
/** @def BT_UUID_CSC_FEATURE
 *  @brief CSC Feature Characteristic
 */
#define BT_UUID_CSC_FEATURE               BT_UUID_DECLARE_16(0x2a5c)
#define BT_UUID_CSC_FEATURE_VAL           0x2a5c
/** @def BT_UUID_RTUS_TIME_UPDATE_CONTROL_POINT
 *  @brief RTUS Time Update Control Point
 */
#define BT_UUID_RTUS_TIME_UPDATE_CONTROL_POINT           BT_UUID_DECLARE_16(0x2A16)
#define BT_UUID_RTUS_TIME_UPDATE_CONTROL_POINT_VAl       0x2A16
/** @def BT_UUID_RTUS_TIME_UPDATE_STATE
 *  @brief RTUS Time Update State
 */
#define BT_UUID_RTUS_TIME_UPDATE_STATE           BT_UUID_DECLARE_16(0x2A17)
#define BT_UUID_RTUS_TIME_UPDATE_STATE_VAl       0x2A17
/** @def BT_UUID_SENSOR_LOCATION
 *  @brief Sensor Location Characteristic
 */
#define BT_UUID_SENSOR_LOCATION           BT_UUID_DECLARE_16(0x2a5d)
#define BT_UUID_SENSOR_LOCATION_VAL       0x2a5d
/** @def BT_UUID_SC_CONTROL_POINT
 *  @brief SC Control Point Characteristic
 */
#define BT_UUID_SC_CONTROL_POINT          BT_UUID_DECLARE_16(0x2a55)
#define BT_UUID_SC_CONTROL_POINT_VAl      0x2a55
/** @def BT_UUID_ELEVATION
 *  @brief Elevation Characteristic
 */
#define BT_UUID_ELEVATION                 BT_UUID_DECLARE_16(0x2a6c)
#define BT_UUID_ELEVATION_VAL             0x2a6c
/** @def BT_UUID_PRESSURE
 *  @brief Pressure Characteristic
 */
#define BT_UUID_PRESSURE                  BT_UUID_DECLARE_16(0x2a6d)
#define BT_UUID_PRESSURE_VAL              0x2a6d
/** @def BT_UUID_TEMPERATURE
 *  @brief Temperature Characteristic
 */
#define BT_UUID_TEMPERATURE               BT_UUID_DECLARE_16(0x2a6e)
#define BT_UUID_TEMPERATURE_VAL           0x2a6e
/** @def BT_UUID_HUMIDITY
 *  @brief Humidity Characteristic
 */
#define BT_UUID_HUMIDITY                  BT_UUID_DECLARE_16(0x2a6f)
#define BT_UUID_HUMIDITY_VAL              0x2a6f
/** @def BT_UUID_TRUE_WIND_SPEED
 *  @brief True Wind Speed Characteristic
 */
#define BT_UUID_TRUE_WIND_SPEED           BT_UUID_DECLARE_16(0x2a70)
#define BT_UUID_TRUE_WIND_SPEED_VAL       0x2a70
/** @def BT_UUID_TRUE_WIND_DIR
 *  @brief True Wind Direction Characteristic
 */
#define BT_UUID_TRUE_WIND_DIR             BT_UUID_DECLARE_16(0x2a71)
#define BT_UUID_TRUE_WIND_DIR_VAL         0x2a71
/** @def BT_UUID_APPARENT_WIND_SPEED
 *  @brief Apparent Wind Speed Characteristic
 */
#define BT_UUID_APPARENT_WIND_SPEED       BT_UUID_DECLARE_16(0x2a72)
#define BT_UUID_APPARENT_WIND_SPEED_VAL   0x2a72
/** @def BT_UUID_APPARENT_WIND_DIR
 *  @brief Apparent Wind Direction Characteristic
 */
#define BT_UUID_APPARENT_WIND_DIR         BT_UUID_DECLARE_16(0x2a73)
#define BT_UUID_APPARENT_WIND_DIR_VAL     0x2a73
/** @def BT_UUID_GUST_FACTOR
 *  @brief Gust Factor Characteristic
 */
#define BT_UUID_GUST_FACTOR               BT_UUID_DECLARE_16(0x2a74)
#define BT_UUID_GUST_FACTOR_VAL           0x2a74
/** @def BT_UUID_POLLEN_CONCENTRATION
 *  @brief Pollen Concentration Characteristic
 */
#define BT_UUID_POLLEN_CONCENTRATION      BT_UUID_DECLARE_16(0x2a75)
#define BT_UUID_POLLEN_CONCENTRATION_VAL  0x2a75
/** @def BT_UUID_UV_INDEX
 *  @brief UV Index Characteristic
 */
#define BT_UUID_UV_INDEX                  BT_UUID_DECLARE_16(0x2a76)
#define BT_UUID_UV_INDEX_VAL              0x2a76
/** @def BT_UUID_IRRADIANCE
 *  @brief Irradiance Characteristic
 */
#define BT_UUID_IRRADIANCE                BT_UUID_DECLARE_16(0x2a77)
#define BT_UUID_IRRADIANCE_VAL            0x2a77
/** @def BT_UUID_RAINFALL
 *  @brief Rainfall Characteristic
 */
#define BT_UUID_RAINFALL                  BT_UUID_DECLARE_16(0x2a78)
#define BT_UUID_RAINFALL_VAL              0x2a78
/** @def BT_UUID_WSS_WSF
 *  @brief WSS Weight Scale Feature
 */
#define BT_UUID_WSS_WSF                       BT_UUID_DECLARE_16(0x2A9E)
#define BT_UUID_WSS_WSF_VAL                   0x2A9E
/** @def BT_UUID_WSS_WM
 *  @brief WSS Weight Measurement
 */
#define BT_UUID_WSS_WM                       BT_UUID_DECLARE_16(0x2A9D)
#define BT_UUID_WSS_WM_VAL                   0x2A9D
/** @def BT_UUID_WIND_CHILL
 *  @brief Wind Chill Characteristic
 */
#define BT_UUID_WIND_CHILL                BT_UUID_DECLARE_16(0x2a79)
#define BT_UUID_WIND_CHILL_VAL            0x2a79
/** @def BT_UUID_HEAT_INDEX
 *  @brief Heat Index Characteristic
 */
#define BT_UUID_HEAT_INDEX                BT_UUID_DECLARE_16(0x2a7a)
#define BT_UUID_HEAT_INDEX_VAL            0x2a7a
/** @def BT_UUID_DEW_POINT
 *  @brief Dew Point Characteristic
 */
#define BT_UUID_DEW_POINT                 BT_UUID_DECLARE_16(0x2a7b)
#define BT_UUID_DEW_POINT_VAL             0x2a7b
/** @def BT_UUID_DESC_VALUE_CHANGED
 *  @brief Descriptor Value Changed Characteristic
 */
#define BT_UUID_DESC_VALUE_CHANGED        BT_UUID_DECLARE_16(0x2a7d)
#define BT_UUID_DESC_VALUE_CHANGED_VAL    0x2a7d
/** @def BT_UUID_MAGN_FLUX_DENSITY_2D
 *  @brief Magnetic Flux Density - 2D Characteristic
 */
#define BT_UUID_MAGN_FLUX_DENSITY_2D      BT_UUID_DECLARE_16(0x2aa0)
#define BT_UUID_MAGN_FLUX_DENSITY_2D_VAL  0x2aa0
/** @def BT_UUID_MAGN_FLUX_DENSITY_3D
 *  @brief Magnetic Flux Density - 3D Characteristic
 */
#define BT_UUID_MAGN_FLUX_DENSITY_3D      BT_UUID_DECLARE_16(0x2aa1)
#define BT_UUID_MAGN_FLUX_DENSITY_3D_VAL  0x2aa1
/** @def BT_UUID_BAR_PRESSURE_TREND
 *  @brief Barometric Pressure Trend Characteristic
 */
#define BT_UUID_BAR_PRESSURE_TREND        BT_UUID_DECLARE_16(0x2aa3)
#define BT_UUID_BAR_PRESSURE_TREND_VAL    0x2aa3
/** @def BT_UUID_MESH_PROV_DATA_IN
 *  @brief Mesh Provisioning Data In
 */
#define BT_UUID_MESH_PROV_DATA_IN         BT_UUID_DECLARE_16(0x2adb)
#define BT_UUID_MESH_PROV_DATA_IN_VAL     0x2adb
/** @def BT_UUID_MESH_PROV_DATA_OUT
 *  @brief Mesh Provisioning Data Out
 */
#define BT_UUID_MESH_PROV_DATA_OUT        BT_UUID_DECLARE_16(0x2adc)
#define BT_UUID_MESH_PROV_DATA_OUT_VAL    0x2adc
/** @def BT_UUID_MESH_PROXY_DATA_IN
 *  @brief Mesh Proxy Data In
 */
#define BT_UUID_MESH_PROXY_DATA_IN        BT_UUID_DECLARE_16(0x2add)
#define BT_UUID_MESH_PROXY_DATA_IN_VAL    0x2add
/** @def BT_UUID_MESH_PROXY_DATA_OUT
 *  @brief Mesh Proxy Data Out
 */
#define BT_UUID_MESH_PROXY_DATA_OUT       BT_UUID_DECLARE_16(0x2ade)
#define BT_UUID_MESH_PROXY_DATA_OUT_VAL   0x2ade

/*
 * Protocol UUIDs
 */
#define BT_UUID_SDP                       BT_UUID_DECLARE_16(0x0001)
#define BT_UUID_SDP_VAL                   0x0001
#define BT_UUID_UDP                       BT_UUID_DECLARE_16(0x0002)
#define BT_UUID_UDP_VAL                   0x0002
#define BT_UUID_RFCOMM                    BT_UUID_DECLARE_16(0x0003)
#define BT_UUID_RFCOMM_VAL                0x0003
#define BT_UUID_TCP                       BT_UUID_DECLARE_16(0x0004)
#define BT_UUID_TCP_VAL                   0x0004
#define BT_UUID_TCS_BIN                   BT_UUID_DECLARE_16(0x0005)
#define BT_UUID_TCS_BIN_VAL               0x0005
#define BT_UUID_TCS_AT                    BT_UUID_DECLARE_16(0x0006)
#define BT_UUID_TCS_AT_VAL                0x0006
#define BT_UUID_ATT                       BT_UUID_DECLARE_16(0x0007)
#define BT_UUID_ATT_VAL                   0x0007
#define BT_UUID_OBEX                      BT_UUID_DECLARE_16(0x0008)
#define BT_UUID_OBEX_VAL                  0x0008
#define BT_UUID_IP                        BT_UUID_DECLARE_16(0x0009)
#define BT_UUID_IP_VAL                    0x0009
#define BT_UUID_FTP                       BT_UUID_DECLARE_16(0x000a)
#define BT_UUID_FTP_VAL                   0x000a
#define BT_UUID_HTTP                      BT_UUID_DECLARE_16(0x000c)
#define BT_UUID_HTTP_VAL                  0x000c
#define BT_UUID_BNEP                      BT_UUID_DECLARE_16(0x000f)
#define BT_UUID_BNEP_VAL                  0x000f
#define BT_UUID_UPNP                      BT_UUID_DECLARE_16(0x0010)
#define BT_UUID_UPNP_VAL                  0x0010
#define BT_UUID_HIDP                      BT_UUID_DECLARE_16(0x0011)
#define BT_UUID_HIDP_VAL                  0x0011
#define BT_UUID_HCRP_CTRL                 BT_UUID_DECLARE_16(0x0012)
#define BT_UUID_HCRP_CTRL_VAL             0x0012
#define BT_UUID_HCRP_DATA                 BT_UUID_DECLARE_16(0x0014)
#define BT_UUID_HCRP_DATA_VAL             0x0014
#define BT_UUID_HCRP_NOTE                 BT_UUID_DECLARE_16(0x0016)
#define BT_UUID_HCRP_NOTE_VAL             0x0016
#define BT_UUID_AVCTP                     BT_UUID_DECLARE_16(0x0017)
#define BT_UUID_AVCTP_VAL                 0x0017
#define BT_UUID_AVDTP                     BT_UUID_DECLARE_16(0x0019)
#define BT_UUID_AVDTP_VAL                 0x0019
#define BT_UUID_CMTP                      BT_UUID_DECLARE_16(0x001b)
#define BT_UUID_CMTP_VAL                  0x001b
#define BT_UUID_UDI                       BT_UUID_DECLARE_16(0x001d)
#define BT_UUID_UDI_VAL                   0x001d
#define BT_UUID_MCAP_CTRL                 BT_UUID_DECLARE_16(0x001e)
#define BT_UUID_MCAP_CTRL_VAL             0x001e
#define BT_UUID_MCAP_DATA                 BT_UUID_DECLARE_16(0x001f)
#define BT_UUID_MCAP_DATA_VAL             0x001f
#define BT_UUID_L2CAP                     BT_UUID_DECLARE_16(0x0100)
#define BT_UUID_L2CAP_VAL                 0x0100


/** @brief Compare Bluetooth UUIDs.
 *
 *  Compares 2 Bluetooth UUIDs, if the types are different both UUIDs are
 *  first converted to 128 bits format before comparing.
 *
 *  @param u1 First Bluetooth UUID to compare
 *  @param u2 Second Bluetooth UUID to compare
 *
 *  @return negative value if @a u1 < @a u2, 0 if @a u1 == @a u2, else positive
 */
int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2);

/** @brief Convert Bluetooth UUID to string.
 *
 *  Converts Bluetooth UUID to string. UUID has to be in 16 bits or 128 bits
 *  format.
 *
 *  @param uuid Bluetooth UUID
 *  @param str pointer where to put converted string
 *  @param len length of str
 *
 *  @return N/A
 */
void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len);

/** @brief Convert Bluetooth UUID to string in place.
 *
 *  Converts Bluetooth UUID to string in place. UUID has to be in 16 bits or
 *  128 bits format.
 *
 *  @param uuid Bluetooth UUID
 *
 *  @return String representation of the UUID given
 */
const char *bt_uuid_str(const struct bt_uuid *uuid);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_UUID_H */
