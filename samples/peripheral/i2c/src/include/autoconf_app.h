/* common config */
#include "autoconf.h"

#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 0

#undef CONFIG_BT_LLCC

#define CONFIG_UART_0 1

#define CONFIG_I2C_0 1

#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{8, 7 | (3<<12) | (1<<11) | (1<<8) },\
	{9, 7 | (3<<12) | (1<<11) | (1<<8) }

