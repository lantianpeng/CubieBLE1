/* common config */
#include "autoconf.h"

/* application config */

/* msg type */
enum {
	MSG_NULL,
	MSG_LOW_POWER,
	MSG_OTA_EVENT,
	
	MSG_SCAN_TIME_OUT,
	MSG_CREATE_LE_TIME_OUT,
	MSG_CONNECTED,
	MSG_DISCOVER_COMPLETE,
	MSG_RECONNECTING,
	MSG_DISCONNECT,
	MSG_DISCONNECTED,
};

/* BT */
#undef CONFIG_BT_DEVICE_NAME
#undef CONFIG_BT_MAX_CONN
#undef CONFIG_BT_MAX_PAIRED
#undef CONFIG_BT_RX_BUF_COUNT
#undef CONFIG_BT_L2CAP_TX_BUF_COUNT

#define CONFIG_BT_DEVICE_NAME "ZRS1020810"

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
#define CONFIG_DEEPSLEEP 1

#define CONFIG_UART_0 1
#define CONFIG_UART_1 1

#define CONFIG_UART_PIPE_ON_DEV_NAME "UART_1"
#define CONFIG_UART_AT_ON_DEV_NAME "UART_1"
#undef CONFIG_UART_CONSOLE_ON_DEV_NAME
#define CONFIG_UART_CONSOLE_ON_DEV_NAME "UART_0"

#undef CONFIG_OTA_WITH_APP
#define CONFIG_OTA_WITH_APP 1

#define CONFIG_BT_CENTRAL 1

/* use qpps profile */
#define CONFIG_USE_QPPS_PROFILE 1

/* use fix handles to quick connect */
#define CONFIG_QUICK_CONNECT 1

/* ATTENTION: the service/profile register order will change the handle value */
#if CONFIG_USE_QPPS_PROFILE
#define MOUDULE_SVC_HANDLE_START 16
#define MOUDULE_SVC_HANDLE_END   22
#else
#define MOUDULE_SVC_HANDLE_START 16
#define MOUDULE_SVC_HANDLE_END   24
#endif

/* role master */
#define CONFIG_BLE_ROLE_MASTER 1

/* device name prefix */
#define CONFIG_DEVICE_NAME_PREFIX   "ZRS"

/* slave disable deepsleep */
#if !CONFIG_BLE_ROLE_MASTER
#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 0
#endif

/* at command has no equal sign */
#define CONFIG_AT_CMD_NO_EQUAL_SIGN 1

/* auto reconnect */
#define CONFIG_AUTO_RECONNECT 0

/* LED & INPUT PIN */	
#define LED_STATE_IND_PIN   6   // pin 14 (PWM0 before)
#define CONN_STATE_IND_PIN  7   // pin 8  (NTF before)
#define CONN_INT_PIN        8   // pin 11 (RESET before)
#define POWM_MODE_PIN       18  // pin 1  (EN before)

/* LED */
#define CONFIG_USE_GPIO_LED  1

#if CONFIG_USE_GPIO_LED
#define LED_PIN_CONFIG \
		{LED_STATE_IND_PIN, 0xff, 1},\
		{CONN_STATE_IND_PIN, 0xff, 1},
#else
#define LED_PIN_CONFIG \
		{LED_STATE_IND_PIN, 1, 1},\
		{CONN_STATE_IND_PIN, 2, 1},
#endif	

/* PIN */
#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{4, 4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{5, 4 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, 

/* INPUT */
#define CONFIG_INPUT_TRIGGER_LOW_EDGE      (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)  
#define CONFIG_INPUT_TRIGGER_DOUBLE_EDGE      (GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE)  
#define INPUT_CONFIG \
		{CONN_INT_PIN, GPIO_PUD_PULL_UP, CONFIG_INPUT_TRIGGER_LOW_EDGE}, \
		{POWM_MODE_PIN, GPIO_PUD_PULL_DOWN, CONFIG_INPUT_TRIGGER_DOUBLE_EDGE},

/* Manufacturer Name */
#define CONFIG_DIS_MANUFACTURER_NAME       "Actions corp."

/* Model Number String */
#define CONFIG_DIS_MODEL                   "ATB110x"

/* PNP ID */
#define CONFIG_DIS_PNP_COMPANY_ID_TYPE    0x01
#define CONFIG_DIS_PNP_VENDOR_ID          0xe0, 0x03
#define CONFIG_DIS_PNP_PRODUCT_ID         0x00, 0x11
#define CONFIG_DIS_PNP_PRODUCT_VERSION    0x00, 0x0

