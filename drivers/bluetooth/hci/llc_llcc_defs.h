/* llc_llcc_defs.h - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LLC_LLCC_DEFS_H
#define LLC_LLCC_DEFS_H


/**************************************************************************************************
  Peripheral memory map
**************************************************************************************************/

/*! \brief      Base of LLCC registers. */
#define LLC_LLCC_REGS_BASE  0x40020000

#ifndef LLCC_CTRL_BASE
/*! \brief      Base of blocks within LLCC. */
#define LLCC_CTRL_BASE            (LLC_LLCC_REGS_BASE + 0x0000)
#define DMAC_CONT_BASE            (LLC_LLCC_REGS_BASE + 0x1000)
#define LLCC_RXD_BASE             (LLC_LLCC_REGS_BASE + 0x2000)
#define LLCC_TXD_BASE             (LLC_LLCC_REGS_BASE + 0x3000)

/*! \brief      DMA registers base. */
#define DMAC_DMARH_BASE           (DMAC_CONT_BASE+0x00)
#define DMAC_DMARL_BASE           (DMAC_CONT_BASE+0x40)
#define DMAC_DMAWH_BASE           (DMAC_CONT_BASE+0x80)
#define DMAC_DMAWL_BASE           (DMAC_CONT_BASE+0xC0)
#define DMAC_HCIR_BASE            DMAC_DMARL_BASE
#define DMAC_HCIW_BASE            DMAC_DMAWL_BASE

/**************************************************************************************************
  Peripheral declaration
**************************************************************************************************/

#define LLCC_CTL             ((LLCC_CTL_TypeDef *)  LLCC_CTRL_BASE)
#define LLCC_RXD             ((LLCC_RXD_TypeDef *)  LLCC_RXD_BASE)
#define LLCC_TXD             ((LLCC_TXD_TypeDef *)  LLCC_TXD_BASE)
#define DMAC_DMARH           ((DMAC_CHAN_TypeDef *) DMAC_DMARH_BASE)
#define DMAC_DMARL           ((DMAC_CHAN_TypeDef *) DMAC_DMARL_BASE)
#define DMAC_DMAWH           ((DMAC_CHAN_TypeDef *) DMAC_DMAWH_BASE)
#define DMAC_DMAWL           ((DMAC_CHAN_TypeDef *) DMAC_DMAWL_BASE)
#define DMAC_HCIR            DMAC_DMAWL
#define DMAC_HCIW            DMAC_DMARL

/**************************************************************************************************
  LLCC/DMAC v1
**************************************************************************************************/

#ifndef __IO
#define __IO  volatile
#endif

#ifndef __I
#define __I   volatile const
#endif

#ifndef __O
#define __O   volatile
#endif

typedef struct
{
  __IO u32_t BUF_STATE;        // +0x00
  __I  u32_t STATUS;           // +0x00
  __IO u32_t PTR_ADDR;         // +0x08
  __IO u32_t PTR_CTRL;         // +0x0c
  __O  u32_t NXT_ADDR;         // +0x10
  __O  u32_t NXT_CTRL;         // +0x14
  __I  u32_t rsvd_18[2];       // +0x18
  __IO u32_t BUF0_ADDR;        // +0x20
  __IO u32_t BUF0_CTRL;        // +0x24
  __I  u32_t rsvd_28[2];       // +0x28
  __IO u32_t BUF1_ADDR;        // +0x30
  __IO u32_t BUF1_CTRL;        // +0x34
  __IO u32_t INTEN;            // +0x38
  __IO u32_t IRQSTATUS;        // +0x3c
} DMAC_CHAN_TypeDef;

/* \brief       DMA buffer control state machine. */
#define DMAC_BUFSTATE_MT          0
#define DMAC_BUFSTATE_A           1
#define DMAC_BUFSTATE_AB          5
#define DMAC_BUFSTATE_B           2
#define DMAC_BUFSTATE_BA          6
#define DMAC_BUFSTATE_FULL_IDX    2

/*! \brief      DMA Control structure MASKs. */
#define DMAC_CHAN_ADDR_MASK       0xfffffffc
#define DMAC_CHAN_COUNT_MASK      0x0000ffff
#define DMAC_CHAN_SIZE_MASK       0x00030000
#define DMAC_CHAN_AFIX_MASK       0x00040000
#define DMAC_CHAN_LOOP_MASK       0x00080000
#define DMAC_CHAN_ATTR_MASK       0xfff00000
#define DMAC_CHAN_COUNT_IDX_LO    0
#define DMAC_CHAN_COUNT_IDX_HI    15
#define DMAC_CHAN_SIZE_IDX_LO     16
#define DMAC_CHAN_SIZE_IDX_HI     17
#define DMAC_CHAN_AFIX_IDX        18
#define DMAC_CHAN_LOOP_IDX        19
#define DMAC_CHAN_TRIG_IDX_LO     20
#define DMAC_CHAN_TRIG_IDX_HI     23
#define DMAC_CHAN_ATTR_IDX_LO     24
#define DMAC_CHAN_ATTR_IDX_HI     31
#define DMAC_CHAN_IRQ_IDX         0
#define DMAC_CHAN_ERR_IDX         1

typedef struct
{
  __I  u32_t ID_MAIN;          // +0x0000
  __I  u32_t ID_REV;           // +0x0004
  __I  u32_t rsvd_0008[30];    // +0x0008
  __IO u32_t STANDBY_CTRL;     // +0x0080
} LLCC_CTL_TypeDef;

typedef struct
{
  __I  u32_t CMD_DATA0;        // +0x2000
  __I  u32_t CMD_DATA1;        // +0x2004
  __I  u32_t rsvd_008[14];     // +0x2008
  __I  u32_t DMAH_DATA0;       // +0x2040
  __I  u32_t DMAH_DATA1;       // +0x2044
  __I  u32_t rsvd_048[6];      // +0x2048
  __I  u32_t DMAL_DATA0;       // +0x2060
  __I  u32_t DMAL_DATA1;       // +0x2064
  __I  u32_t rsvd_068[6];      // +0x2068
  __I  u32_t EVT_DATA0;        // +0x2080
  __I  u32_t EVT_DATA1;        // +0x2084
  __I  u32_t rsvd_088[14];     // +0x2088
  __I  u32_t INTERRUPT;        // +0x20c0
  __IO u32_t INTENMASK;        // +0x20c4
  __IO u32_t INTENMASK_SET;    // +0x20c8
  __IO u32_t INTENMASK_CLR;    // +0x20cc
  __I  u32_t REQUEST;          // +0x20d0
  __I  u32_t rsvd_0d4[3];      // +0x20d4
  __I  u32_t XFERREQ;          // +0x20e0
  __I  u32_t XFERACK;          // +0x20e4
  __I  u32_t rsvd_0e8[6];      // +0x20e8
} LLCC_RXD_TypeDef;

typedef struct
{
  __IO u32_t CMD_DATA0;        // +0x3000
  __IO u32_t CMD_DATA1;        // +0x3004
  __I  u32_t rsvd_008[14];     // +0x3008
  __IO u32_t DMAH_DATA0;       // +0x3040
  __IO u32_t DMAH_DATA1;       // +0x3044
  __I  u32_t rsvd_048[6];      // +0x3048
  __IO u32_t DMAL_DATA0;       // +0x3060
  __IO u32_t DMAL_DATA1;       // +0x3064
  __I  u32_t rsvd_068[6];      // +0x3068
  __IO u32_t EVT_DATA0;        // +0x3080
  __IO u32_t EVT_DATA1;        // +0x3084
  __I  u32_t rsvd_088[14];     // +0x3088
  __I  u32_t INTERRUPT;        // +0x30c0
  __IO u32_t INTENMASK;        // +0x30c4
  __IO u32_t INTENMASK_SET;    // +0x30c8
  __IO u32_t INTENMASK_CLR;    // +0x30cc
  __I  u32_t REQUEST;          // +0x30d0
  __I  u32_t ACTIVE;           // +0x30d4
  __I  u32_t VCREADY;          // +0x30d8
  __I  u32_t rsvd_0dc;         // +0x30dc
  __IO u32_t XFERREQ;          // +0x30e0
  __I  u32_t XFERACK;          // +0x30e4
  __I  u32_t rsvd_0e8[6];      // +0x30e8
} LLCC_TXD_TypeDef;

/*! \brief      TX/RX buffer handshake/interrupt fields. */
#define LLCC_CMD0_MASK            0x01
#define LLCC_CMD1_MASK            0x02
#define LLCC_CMD_MASK             0x03
#define LLCC_CMD_IRQ_MASK         LLCC_CMD_MASK
#define LLCC_DMAH1_MASK           0x04
#define LLCC_DMAH2_MASK           0x08
#define LLCC_DMAH_MASK            0x0c
#define LLCC_DMAL1_MASK           0x10
#define LLCC_DMAL2_MASK           0x20
#define LLCC_DMAL_MASK            0x30
#define LLCC_EVT0_MASK            0x40
#define LLCC_EVT1_MASK            0x80
#define LLCC_EVT_IRQ_MASK         LLCC_EVT1_MASK
#define LLCC_EVT_MASK             0xc0
#define LLCC_CMD0_IDX             0
#define LLCC_CMD1_IDX             1
#define LLCC_CMD_IDX              LLCC_CMD1_IDX
#define LLCC_CMD_IRQ_IDX          1
#define LLCC_DMAH1_IDX            2
#define LLCC_DMAH2_IDX            3
#define LLCC_DMAL1_IDX            4
#define LLCC_DMAL2_IDX            5
#define LLCC_EVT0_IDX             6
#define LLCC_EVT1_IDX             7
#define LLCC_EVT_IDX              LLCC_EVT1_IDX

#endif /* LLCC_CTRL_BASE */

#endif /* LLC_LLCC_DEFS_H */
