/* common config */
#include "autoconf.h"

/* application config */

/* msg type */
enum {
	MSG_NULL,

	MSG_KEY_INPUT,
	MSG_LOW_POWER,

	MSG_BLE_EVENT,
	MSG_OTA_EVENT,
	
	MSG_ANCS_SERVICE,
};

/* BT */
#undef CONFIG_BT_DEVICE_NAME
#undef CONFIG_BT_MAX_CONN
#undef CONFIG_BT_MAX_PAIRED
#undef CONFIG_BT_RX_BUF_COUNT
#undef CONFIG_BT_L2CAP_TX_BUF_COUNT

extern unsigned char adv_name_data[];
#define CONFIG_BT_DEVICE_NAME ((char*)adv_name_data)

#define CONFIG_BT_MAX_CONN 1
#define CONFIG_BT_MAX_PAIRED 1
#define CONFIG_BT_RX_BUF_COUNT 3
#define CONFIG_BT_L2CAP_TX_BUF_COUNT 4

/* STACK */
#undef CONFIG_IDLE_STACK_SIZE
#undef CONFIG_BT_RX_STACK_SIZE

#define CONFIG_IDLE_STACK_SIZE (256+256)
#define CONFIG_BT_RX_STACK_SIZE (1280 - 300)

/* SHELL */
#undef CONFIG_KERNEL_SHELL
#define CONFIG_KERNEL_SHELL 0

#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 0

#define CONFIG_UART_0 1

#undef CONFIG_OTA_WITH_APP
#define CONFIG_OTA_WITH_APP 1

/* Manufacturer Name */
#define CONFIG_DIS_MANUFACTURER_NAME       "Actions corp."

/* Model Number String */
#define CONFIG_DIS_MODEL                   "ATB110x"

/* PNP ID */
#define CONFIG_DIS_PNP_COMPANY_ID_TYPE    0x01
#define CONFIG_DIS_PNP_VENDOR_ID          0xe0, 0x03
#define CONFIG_DIS_PNP_PRODUCT_ID         0x00, 0x11
#define CONFIG_DIS_PNP_PRODUCT_VERSION    0x00, 0x0

#include "app_atb1109.h"
