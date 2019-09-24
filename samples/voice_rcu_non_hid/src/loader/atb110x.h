#ifndef __ATB110X_H
#define __ATB1101_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------  Interrupt Number Definition  -------------------- */

typedef enum IRQn
{
/* ---------------  Cortex-M0 Processor Exceptions Numbers  --------------- */
  NonMaskableInt_IRQn           = -14,      /*!<  2 Non Maskable Interrupt  */
  HardFault_IRQn                = -13,      /*!<  3 HardFault Interrupt     */



  SVCall_IRQn                   =  -5,      /*!< 11 SV Call Interrupt       */

  PendSV_IRQn                   =  -2,      /*!< 14 Pend SV Interrupt       */
  SysTick_IRQn                  =  -1,      /*!< 15 System Tick Interrupt   */

/* ------------------  ATB110X Specific Interrupt Numbers  ---------------- */
} IRQn_Type;

/* ======================================================================== */
/* ============      Processor and Core Peripheral Section     ============ */
/* ======================================================================== */

/* ----  Configuration of the Cortex-M0 Processor and Core Peripherals  --- */
#define __CM0_REV             0x0001    /*!< Core revision r0p1         */
#define __NVIC_PRIO_BITS          2         /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0         /*!< Set to 1 if different SysTick Config is used */

#include <core_cm0.h>                 /* Processor and core peripherals */


/* ======================================================================== */
/* ============       Device Specific Peripheral Section       ============ */
/* ======================================================================== */

/* ---------------  Start of section using anonymous unions  -------------- */
#if defined(__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined(__ICCARM__)
  #pragma language=extended
#elif defined(__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined(__TMS470__)
/* anonymous unions are enabled by default */
#elif defined(__TASKING__)
  #pragma warning 586
#else
  #warning Not supported compiler type
#endif

/* ======================================================================== */
/* ============                     CMU                        ============ */
/* ======================================================================== */
typedef struct
{
	__IO	uint32_t BLE_32K_CTL;       /* Offset: 0x000 (R/W)   */
	__IO  uint32_t BLE_32M_CTL;      /* Offset: 0x004 (R/W)   */
	__IO 	uint32_t ACT_3M_CTL;    /* Offset: 0x008 (R/W)   */

	__IO  uint32_t SYSCLK;     /* Offset: 0x00C (R/W)   */

	__IO  uint32_t DEVRST;        /* Offset: 0x010 (R/W)   */
	__IO  uint32_t DEVCLKEN;        /* Offset: 0x014 (R/W)   */

	__IO 	uint32_t SPI0CLK;     /* Offset: 0x018 (R/W)   */
	__IO 	uint32_t SPI1CLK;			/* Offset: 0x01C (R/W)   */
	__IO 	uint32_t SPI2CLK;			/* Offset: 0x01C (R/W)   */

	__IO 	uint32_t PWM0CLK;    /* Offset: 0x02C (R/W)   */
	__IO 	uint32_t PWM1CLK;    /* Offset: 0x02C (R/W)   */
	__IO 	uint32_t PWM2CLK;    /* Offset: 0x02C (R/W)   */
	__IO 	uint32_t PWM3CLK;    /* Offset: 0x02C (R/W)   */
	__IO 	uint32_t PWM4CLK;    /* Offset: 0x02C (R/W)   */

	__IO 	uint32_t AUDIOCLK;    /* Offset: 0x02C (R/W)   */

	__IO 	uint32_t TIMER0CLK;		/* Offset: 0x024 (R/W)   */
	__IO  uint32_t TIMER1CLK;		/* Offset: 0x028 (R/W)   */
	__IO  uint32_t TIMER2CLK;		/* Offset: 0x028 (R/W)   */
	__IO  uint32_t TIMER3CLK;		/* Offset: 0x028 (R/W)   */

	__IO  uint32_t EXTICLK;     /* Offset: 0x030 (R/W)   */
	__IO  uint32_t MEMCLKEN;        /* Offset: 0x034 (R/W)   */
	__IO  uint32_t DIGITALDEBUG;        /* Offset: 0x038 (R/W)   */
	__IO  uint32_t CHIP_VER;        /* Offset: 0x03C (R/)   */
	__IO  uint32_t RMU_RSTSTA;        /* Offset: 0x040 (R/)   */
	__IO  uint32_t XTAL_32M_REQ;        /* Offset: 0x044 (R/W)   */
} CMU_TypeDef;

/* ======================================================================== */
/* ============            MEMORY-CONTROLLER-MAPPING           ============ */
/* ======================================================================== */
typedef struct
{
  __IO	uint32_t MAPPING_ADDR0;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR0_ENTRY;         /* Offset: 0x004 (R/W)   */
  
  __IO	uint32_t MAPPING_ADDR1;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR1_ENTRY;         /* Offset: 0x004 (R/W)   */
  
  __IO	uint32_t MAPPING_ADDR2;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR2_ENTRY;         /* Offset: 0x004 (R/W)   */

  __IO	uint32_t MAPPING_ADDR3;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR3_ENTRY;         /* Offset: 0x004 (R/W)   */
  
  __IO	uint32_t MAPPING_ADDR4;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR4_ENTRY;         /* Offset: 0x004 (R/W)   */

  __IO	uint32_t MAPPING_ADDR5;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR5_ENTRY;         /* Offset: 0x004 (R/W)   */
  
  __IO	uint32_t MAPPING_ADDR6;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR6_ENTRY;         /* Offset: 0x004 (R/W)   */
  
  __IO	uint32_t MAPPING_ADDR7;       /* Offset: 0x000 (R/W)   */
  __IO  uint32_t ADDR7_ENTRY;         /* Offset: 0x004 (R/W)   */
} MAPPING_TypeDef;

/* ======================================================================== */
/* ==========                UART                                 ========= */
/* ======================================================================== */
typedef struct
{
	__IO uint32_t CTRL;             /* Offset: 0x000 (R/W)  control */
	__I uint32_t RXDAT;            /* Offset: 0x004 (R)  fifo data */
	__IO uint32_t TXDAT;            /* Offset: 0x008 (R/W)  fifo data */
	__IO uint32_t STA;             /* Offset: 0x00c (R/W)  status */
	__IO uint32_t BAUDDIV;          /* Offset: 0x010 (R/W)  Baud rate divisor */
} UART_TypeDef;

#define UART_CTRL_RX_EN    ((uint32_t)1 << 31)
#define UART_CTRL_TX_EN    (1 << 30)

#define UART_CTL_TX_FIFO_EN   (1 << 23)
#define UART_CTL_RX_FIFO_EN   (1 << 22)

#define UART_CTRL_EN   (1 << 15)

#define UART_STATE_TXEMPTY  (1 << 10)
#define UART_STATE_RXFULL  (1 << 9)
#define UART_STATE_TXFULL  (1 << 6)
#define UART_STATE_RXEMPTY  (1 << 5)

/* ======================================================================== */
/* ==========                     GPIO&MFP                        ========= */
/* ======================================================================== */
#define GPIO_NUM_MAX 30

typedef struct
{
	__IO uint32_t JTAG_EN;             /* Offset: 0x000 (R/W)  GPIO Output Enable */
	__IO uint32_t IO_CTL[GPIO_NUM_MAX];				/* Offset: 0x014 (R/W) GPIO0 Multi-Function Control */
	__IO uint32_t MICIN_CTL;					/* Offset: 0x064 (R/W) PAD Drive Control Register 0 */
	__IO uint32_t IO_ODAT;		/* Offset: 0x068 (R/W) PAD Drive Control Register 0 * */
	__IO uint32_t IO_BSR;		/* Offset: 0x06c (R/W) GPIO 1K Pull-up resistance Enable */
	__IO uint32_t IO_BRR;	/* Offset: 0x070 (R/W) reserved */
	__IO uint32_t IO_IDAT;	/* Offset: 0x074 (R/W) reserved */
	__IO uint32_t IO_IRQ_PD;	/* Offset: 0x07c (R/W) WAKE up source pending */
} GPIO_MFP_TypeDef;

/* ======================================================================== */
/* ==========                     Watch Dog                        ========= */
/* ======================================================================== */
typedef struct
{
	__IO uint32_t CTL;             /* Offset: 0x000 (R/W)  Control */
} WD_TypeDef;


/* ======================================================================== */
/* ==========                     RTC BAK                        ========= */
/* ======================================================================== */
typedef struct
{
	__IO uint32_t BAK0;             /* Offset: 0x000 (R/W)  Control */
	__IO uint32_t BAK1;             /* Offset: 0x000 (R/W)  Control */
	__IO uint32_t BAK2;             /* Offset: 0x000 (R/W)  Control */
	__IO uint32_t BAK3;             /* Offset: 0x000 (R/W)  Control */
} RTC_BAK_TypeDef;

/* =================================================== */
/* ==========  SPI ========= */
/* =================================================== */
typedef struct
{
	__IO uint32_t CTL;             /* Offset: 0x00 (R/W)  Control */
	__IO uint32_t STA;             /* Offset: 0x04 (R/W)  STATUS */
	__O  uint32_t TXDAT;              /* Offset: 0x08 (/W)  TXDATA */
	__I  uint32_t RXDAT;              /* Offset: 0x0C (R/ )  RXDATA */
	__IO uint32_t BC;            /* Offset: 0x10 (R/)  Byte Count */
	__IO uint32_t SEED;            /* Offset: 0x14 (R/W)  Seed */
} SPI0_TypeDef;

typedef struct
{
	__IO uint32_t CTL;             /* Offset: 0x00 (R/W)  Control */
	__IO uint32_t STA;             /* Offset: 0x04 (R/W)  STATUS */
	__O  uint32_t TXDAT;              /* Offset: 0x08 (/W)  TXDATA */
	__I  uint32_t RXDAT;              /* Offset: 0x0C (R/ )  RXDATA */
	__IO uint32_t BC;            /* Offset: 0x10 (R/)  Byte Count */
} SPI1_TypeDef;

#define SPI_CTRL_SEL ((uint32_t)1 << 31)
#define SPI_CTL_TX_FIFO_EN (1<<5)
#define SPI_CTL_RX_FIFO_EN (1<<4)
#define SPI_CTL_SS (1<<3)
#define SPI_CTL_SPI_WR_SHIFT 0
#define SPI_CTL_SPI_WR_MASK (0x3 << SPI_CTL_SPI_WR_SHIFT)

#define SPI0_STATE_TXFULL (1<<5)
#define SPI0_STATE_TXEMPTY (1<<4)
#define SPI0_STATE_RXFULL (1<<3)
#define SPI0_STATE_RXEM (1<<2)

#define SPI1_STATE_RXFULL (1<<7)
#define SPI1_STATE_RXEM (1<<6)
#define SPI1_STATE_TXFULL (1<<5)
#define SPI1_STATE_TXEMPTY (1<<4)

/* ----------------  End of section using anonymous unions  --------------- */
#if defined(__CC_ARM)
  #pragma pop
#elif defined(__ICCARM__)
  /* leave anonymous unions enabled */
#elif defined(__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined(__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined(__TASKING__)
  #pragma warning restore
#else
  #warning Not supported compiler type
#endif

/* ======================================================================== */
/* ============              Peripheral memory map             ============ */
/* ======================================================================== */

/* --------------------------  CPU memory map  ---------------------------- */
/* APB Peripherals */
#define CMU_BASE					(0x40002000)
#define MAPPING_BASE      (0x40009100)
#define UART0_BASE       	(0x4000d000)
#define UART1_BASE       	(0x4000e000)

#define SPI0_BASE       	(0x40011000)
#define SPI1_BASE       	(0x40012000)

#define GPIO_MFP_BASE		  (0x40016000)

#define WD_BASE		  			(0x4000401c)

#define RTC_BAK_BASE      (0X40004030)
/* ======================================================================== */
/* ============             Peripheral declaration             ============ */
/* ======================================================================== */

#define CMU         			((CMU_TypeDef *) CMU_BASE)

#define MAPPING         	((MAPPING_TypeDef *) MAPPING_BASE)

#define UART0         		((UART_TypeDef *) UART0_BASE)
#define UART1         		((UART_TypeDef *) UART1_BASE)

#define SPI0         		((SPI0_TypeDef *) SPI0_BASE)
#define SPI1         		((SPI1_TypeDef *) SPI1_BASE)

#define GPIO_MFP					((GPIO_MFP_TypeDef *) GPIO_MFP_BASE)
#define WD 		 						((WD_TypeDef *) WD_BASE)
#define RTC_BAK 		 			((RTC_BAK_TypeDef *) RTC_BAK_BASE)
#ifdef __cplusplus
}
#endif

#endif  /* __ATB1101_H */

