/* common config */
#include "autoconf.h"

/* DEEPSLEEP */
#undef CONFIG_DEEPSLEEP
#define CONFIG_DEEPSLEEP 0

#define CONFIG_UART_0 1

/* PIN */
#define BOARD_PIN_CONFIG \
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},
