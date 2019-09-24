/* common config */
#include "autoconf.h"

/* application config */

/* msg type */
enum {
	MSG_NULL,

	BLE_ADV_START_IND,		/*! Advertising started */
	BLE_ADV_STOP_IND,			/*! Advertising stopped */
	BLE_SCAN_START_IND,		/*! Scanning started */
	BLE_SCAN_STOP_IND,		/*! Scanning stopped */
	BLE_SCAN_REPORT_IND,	/*! Scan data received from peer device */
	BLE_CONN_OPEN_IND,		/*! Connection opened */
	BLE_CONN_CLOSE_IND,		/*! Connection closed */
};

/* NVRAM */
#undef CONFIG_NVRAM_CONFIG
#define CONFIG_NVRAM_CONFIG 0

/* BT */
#undef CONFIG_BT_DEVICE_NAME
#undef CONFIG_BT_MAX_CONN
#undef CONFIG_BT_MAX_PAIRED
#undef CONFIG_BT_RX_BUF_COUNT
#undef CONFIG_BT_L2CAP_TX_BUF_COUNT

#define CONFIG_BT_DEVICE_NAME "Actions DATC"
#define CONFIG_BT_MAX_CONN 4
#define CONFIG_BT_MAX_PAIRED 2
#define CONFIG_BT_RX_BUF_COUNT 3
#define CONFIG_BT_L2CAP_TX_BUF_COUNT 4

/* STACK */
#undef CONFIG_IDLE_STACK_SIZE
#undef CONFIG_BT_RX_STACK_SIZE
#undef CONFIG_CONSOLE_SHELL_STACKSIZE

#define CONFIG_IDLE_STACK_SIZE (256+256)
#define CONFIG_BT_RX_STACK_SIZE (1280 - 300)
#define CONFIG_CONSOLE_SHELL_STACKSIZE 1024

/* DEEPSLEEP */
#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 0

/* BOARD */
#define CONFIG_UART_0 1
#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}
