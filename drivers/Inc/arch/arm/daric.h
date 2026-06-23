/*
 * Copyright 2024-2026 CrossBar, Inc.
 * Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 * This file has been modified by CrossBar, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DARIC_H__
#define __DARIC_H__

#include <stddef.h>
#include <stdint.h>

/* ========================================================================= */
/* ============           Interrupt Number Definition           ============ */
/* ========================================================================= */
typedef enum IRQn
{
    /* ================ Cortex-M Core Exception Numbers ================ */
    Reset_IRQn            = -15,
    NonMaskableInt_IRQn   = -14,
    HardFault_IRQn        = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn         = -11,
    UsageFault_IRQn       = -10,

    SVCall_IRQn = -5,

    PendSV_IRQn  = -2,
    SysTick_IRQn = -1,

    /* =============== Device Specific Interrupt Numbers ================ */
    QFCIRQ_IRQn          = 16,
    MDMAIRQ_IRQn         = 17,
    MBOX_AVAILABLE_IRQn  = 18,
    MBOX_ABORT_INIT_IRQn = 19,
    MBOX_ABORT_DONE_IRQn = 20,
    MBOX_ERROR_IRQn      = 21,
    ATIMER0_IRQn         = 27,
    ATIMER1_IRQn         = 28,
    WDT_IRQn             = 29,
    AOINT_IRQn           = 30,
    AOWKUPINT_IRQn       = 31,
    TRNG_DONE_IRQn       = 32,
    AES_DONE_IRQn        = 33,
    PKE_DONE_IRQn        = 34,
    HASH_DONE_IRQn       = 35,
    ALU_DONE_IRQn        = 36,
    SDMA_ICHDONE_IRQn    = 37,
    SDMA_SCHDONE_IRQn    = 38,
    SDMA_XCHDONE_IRQn    = 39,

    UART0_0_IRQn = 64,
    UART0_1_IRQn = 65,
    UART0_2_IRQn = 66,
    UART0_3_IRQn = 67,

    UART1_0_IRQn = 68,
    UART1_1_IRQn = 69,
    UART1_2_IRQn = 70,
    UART1_3_IRQn = 71,

    UART2_0_IRQn = 72,
    UART2_1_IRQn = 73,
    UART2_2_IRQn = 74,
    UART2_3_IRQn = 75,

    UART3_0_IRQn = 76,
    UART3_1_IRQn = 77,
    UART3_2_IRQn = 78,
    UART3_3_IRQn = 79,

    SPIM0_0_IRQn = 80,
    SPIM0_1_IRQn = 81,
    SPIM0_2_IRQn = 82,
    SPIM0_3_IRQn = 83,

    SPIM1_0_IRQn = 84,
    SPIM1_1_IRQn = 85,
    SPIM1_2_IRQn = 86,
    SPIM1_3_IRQn = 87,

    SPIM2_0_IRQn = 88,
    SPIM2_1_IRQn = 89,
    SPIM2_2_IRQn = 90,
    SPIM2_3_IRQn = 91,

    SPIM3_0_IRQn = 92,
    SPIM3_1_IRQn = 93,
    SPIM3_2_IRQn = 94,
    SPIM3_3_IRQn = 95,

    I2C0_0_IRQn = 96,
    I2C0_1_IRQn = 97,
    I2C0_2_IRQn = 98,
    I2C0_3_IRQn = 99,

    I2C1_0_IRQn = 100,
    I2C1_1_IRQn = 101,
    I2C1_2_IRQn = 102,
    I2C1_3_IRQn = 103,

    I2C2_0_IRQn = 104,
    I2C2_1_IRQn = 105,
    I2C2_2_IRQn = 106,
    I2C2_3_IRQn = 107,

    I2C3_0_IRQn = 108,
    I2C3_1_IRQn = 109,
    I2C3_2_IRQn = 110,
    I2C3_3_IRQn = 111,

    SDIO_0_IRQn = 112,
    SDIO_1_IRQn = 113,
    SDIO_2_IRQn = 114,
    SDIO_3_IRQn = 115,

    I2S_0_IRQn = 116,
    I2S_1_IRQn = 117,
    I2S_2_IRQn = 118,
    I2S_3_IRQn = 119,

    CAMIF_0_IRQn = 120,
    CAMIF_1_IRQn = 121,
    CAMIF_2_IRQn = 122,
    CAMIF_3_IRQn = 123,

    FILTER_0_IRQn = 124,
    FILTER_1_IRQn = 125,
    FILTER_2_IRQn = 126,
    FILTER_3_IRQn = 127,

    SCIF_0_IRQn = 128,
    SCIF_1_IRQn = 129,
    SCIF_2_IRQn = 130,
    SCIF_3_IRQn = 131,

    SPIS0_0_IRQn = 132,
    SPIS0_1_IRQn = 133,
    SPIS0_2_IRQn = 134,
    SPIS0_3_IRQn = 135,

    SPIS1_0_IRQn = 136,
    SPIS1_1_IRQn = 137,
    SPIS1_2_IRQn = 138,
    SPIS1_3_IRQn = 139,

    PWM_0_IRQn = 140,
    PWM_1_IRQn = 141,
    PWM_2_IRQn = 142,
    PWM_3_IRQn = 143,

    GPIO_IRQn = 144,
    USB_IRQn  = 145,
    SDDC_IRQn = 146,
    PIO0_IRQn = 147,
    PIO1_IRQn = 148,

    I2C0_NACK_IRQn = 184,
    I2C1_NACK_IRQn = 185,
    I2C2_NACK_IRQn = 186,
    I2C3_NACK_IRQn = 187,
    I2C0_ERR_IRQn  = 188,
    I2C1_ERR_IRQn  = 189,
    I2C2_ERR_IRQn  = 190,
    I2C3_ERR_IRQn  = 191,

    ERROR_0_IRQn  = 192,
    ERROR_1_IRQn  = 193,
    ERROR_2_IRQn  = 194,
    ERROR_3_IRQn  = 195,
    ERROR_4_IRQn  = 196,
    ERROR_5_IRQn  = 197,
    ERROR_6_IRQn  = 198,
    ERROR_7_IRQn  = 199,
    ERROR_8_IRQn  = 200,
    ERROR_9_IRQn  = 201,
    ERROR_10_IRQn = 202,
    ERROR_11_IRQn = 203,
    ERROR_12_IRQn = 204,
    ERROR_13_IRQn = 205,
    ERROR_14_IRQn = 206,
    ERROR_15_IRQn = 207,
    ERROR_16_IRQn = 208,
    ERROR_17_IRQn = 209,
    ERROR_18_IRQn = 210,
    ERROR_19_IRQn = 211,
    ERROR_20_IRQn = 212,
    ERROR_21_IRQn = 213,
    ERROR_22_IRQn = 214,
    ERROR_23_IRQn = 215,
    ERROR_24_IRQn = 216,
    ERROR_25_IRQn = 217,
    ERROR_26_IRQn = 218,
    ERROR_27_IRQn = 219,
    ERROR_28_IRQn = 220,
    ERROR_29_IRQn = 221,
    ERROR_30_IRQn = 222,
    ERROR_31_IRQn = 223,

    TOTAL_IRQ_NUMS = 224
} IRQn_Type;

/* ========================================================================= */
/* ============      Processor and Core Peripheral Section      ============ */
/* ========================================================================= */

/* ================ Start of section using anonymous unions ================ */
#if defined(__CC_ARM)
#pragma push
#pragma anon_unions
#elif defined(__ICCARM__)
#pragma language = extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc11-extensions"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined(__GNUC__)
/* anonymous unions are enabled by default */
#elif defined(__TMS470__)
/* anonymous unions are enabled by default */
#elif defined(__TASKING__)
#pragma warning 586
#elif defined(__CSMC__)
/* anonymous unions are enabled by default */
#else
#warning Not supported compiler type
#endif

/* ================    Configuration of Core Peripherals    ================ */
/* Set to 1 if different SysTick Config is used */
#define __Vendor_SysTickConfig 1U
/* Number of Bits used for Priority Levels */
#define __NVIC_PRIO_BITS 3U
/* Set to 1 if VTOR is present */
#define __VTOR_PRESENT 1U
/* Set to 1 if MPU is present */
#define __MPU_PRESENT 1U
/* Set to 1 if FPU is present */
#define __FPU_PRESENT 1U
/* Set to 1 if FPU is double precision FPU (default is single precision FPU) */
#define __FPU_DP 0U
/* Set to 1 if DSP extension are present */
#define __DSP_PRESENT 0U
/* Set to 1 if SAU regions are present */
#define __SAUREGION_PRESENT 0U
/* Set to 1 if PMU is present */
#define __PMU_PRESENT 0U
/* Set to 1 if I-Cache is present */
#define __ICACHE_PRESENT 1U
/* Set to 1 if D-Cache is present */
#define __DCACHE_PRESENT 1U
/* Set to 1 if DTCM is present */
#define __DTCM_PRESENT 1U

#include "system_daric.h" /* System Header */
#include <core_cm7.h>     /* Processor and core peripherals */

/* Daric execution mode definition */
#define SRAM_MODE  0 // Execution in SRAM
#define ITCM_MODE  1 // Execution in ITCM
#define RERAM_MODE 2 // Execution in RERAM

#if CONFIG_EXECUTION_MODE == SRAM_MODE
#include "config_mpu_sram.h"
#else
#include "config_mpu_default.h"
#endif

/* ========================================================================= */
/* ============       Device Specific Peripheral Section        ============ */
/* ========================================================================= */

/* ToDo: Add here your device specific peripheral access structure typedefs
         including bit definitions for Pos/Msk macros
         following is an example for a timer */

/** @addtogroup Peripheral_registers_structures
 * @{
 */
typedef struct
{
    volatile uint32_t cgusec; // 0x0000
    volatile uint32_t cgulp;  // 0x0004
    volatile uint32_t dummy_00[2];
    volatile uint32_t cgusel0; // 0x0010
    volatile uint32_t fdfclk;  // 0x0014
    volatile uint32_t fdaclk;  // 0x0018
    volatile uint32_t fdhclk;  // 0x001c
    volatile uint32_t fdiclk;  // 0x0020
    volatile uint32_t fdpclk;  // 0x0024
    // volatile uint32_t dummy_01;
    volatile uint32_t fdaoclk; // 0x0028
    volatile uint32_t cguset;  // 0x002c
    volatile uint32_t cgusel1; // 0x0030
    // volatile uint32_t dummy_32[3];
    volatile uint32_t fdpke;      // 0x0034
    volatile uint32_t fdaoram;    // 0x0038
    volatile uint32_t fdper;      // 0x003c
    volatile uint32_t cgufsfreq0; // 0x0040
    volatile uint32_t cgufsfreq1; // 0x0044
    volatile uint32_t cgufsfreq2; // 0x0048
    volatile uint32_t cgufsfreq3; // 0x004c
    volatile uint32_t cgufsvld;   // 0x0050
    volatile uint32_t cgufscr;    // 0x0054
    volatile uint32_t dummy_56[2];
    volatile uint32_t cguaclkgr; // 0x0060
    volatile uint32_t cguhclkgr; // 0x0064
    volatile uint32_t cguiclkgr; // 0x0068
    volatile uint32_t cgupclkgr; // 0x006c
} DARIC_SYSCTRL_CGU_TypeDef;

typedef struct
{
    volatile uint32_t ar;
    volatile uint32_t en;
    volatile uint32_t lpen;
    volatile uint32_t osc;
    volatile uint32_t pll_mn;
    volatile uint32_t pll_f;
    volatile uint32_t pll_q;
    volatile uint32_t ipc;
} DARIC_SYSCTRL_IPC_TypeDef;

typedef struct
{
    volatile uint32_t CFG_CG;
    volatile uint32_t CFG_EVENT;
    volatile uint32_t CFG_RST;
} DARIC_UDMACORE_TypeDef;

typedef struct
{
    volatile uint8_t TX;
    volatile uint8_t DUMMY1[3];
    volatile uint8_t EN;
    volatile uint8_t DUMMY5[3];
    volatile uint8_t BUSY;
    volatile uint8_t DUMMY9[3];
    volatile uint8_t ETU;
    volatile uint8_t DUMMYD[3];
} DARIC_DUART_TypeDef;

typedef struct
{
    volatile uint32_t cache;  // 00
    volatile uint32_t itcm;   // 04
    volatile uint32_t dtcm;   // 08
    volatile uint32_t sram0;  // 0c
    volatile uint32_t sram1;  // 10
    volatile uint32_t vexram; // 14
    volatile uint32_t DUMMY08[2];
    volatile uint32_t srambankerr; // 20
    volatile uint32_t DUMMY24[3];
    volatile uint32_t ramsec; // 30
} DARIC_CORE_SRAMCFG_TypeDef;

typedef union
{
    struct
    {
        uint32_t key_access_err : 1;
        uint32_t data_access_err : 1;
        uint32_t code_access_err : 1;
        uint32_t cfg_access_err : 1;
        uint32_t info_access_err : 1;
        uint32_t rfu : 27;
    } bits;
    uint32_t rrcFrVal;
} DARIC_URRCFRREG_Union;

typedef struct
{
    volatile uint32_t RRC_CR; // ReRAM control register
    volatile uint32_t CFG_FD; // ReRAM Configure clock frequency
    volatile uint32_t CFG_SR; // ReRAM status indicator
    DARIC_URRCFRREG_Union RRC_FR;
} DARIC_RERAM_TypeDef;

typedef struct
{
    volatile uint32_t cr; // 00
    volatile uint32_t sr; // 04
    volatile uint32_t ar; // 08
} DARIC_SRAMTRIM_TypeDef;

/**
 * @brief General Purpose I/O
 */
// clang-format off
typedef struct {
  __IO uint32_t OCR;             /*< GPIO output control register,              Address offset: 0x130                */
  __IO uint32_t RESERVED2[5];    /*< RESERVED,                                  Address offset: 0x134 ~ 0x144        */
  __IO uint32_t OER;             /*< GPIO output enable register,               Address offset: 0x148                */
  __IO uint32_t RESERVED3[5];    /*< RESERVED,                                  Address offset: 0x14c ~ 0x15c        */
  __IO uint32_t PUCR;            /*< GPIO pull up control register,             Address offset: 0x160                */
  __IO uint32_t RESERVED4[5];    /*< RESERVED,                                  Address offset: 0x164 ~ 0x174        */
  __IO uint32_t ISR;             /*< GPIO input status register,                Address offset: 0x178                */
  __IO uint32_t RESERVED5[45];   /*< RESERVED,                                  Address offset: 0x17c ~ 0x22c        */
  __IO uint32_t CISTER;          /*< Input schmitter trigger enable register,   Address offset: 0x230                */
  __IO uint32_t RESERVED6[5];    /*< RESERVED,                                  Address offset: 0x234 ~ 0x244        */
  __IO uint32_t COSRCER;         /*< Output slew rate control enable register,  Address offset: 0x248                */
  __IO uint32_t RESERVED7[5];    /*< RESERVED,                                  Address offset: 0x24c ~ 0x25c        */
  __IO uint32_t CDSR;            /*< Drv Selection Register,                    Address offset: 0x260                */
} GPIO_TypeDef;

typedef struct {
  __IO uint32_t PORTS[12];
} GPIO_AFSEL_Array_TypeDef;

typedef struct {
  __IO uint32_t INTCR[8];
} GPIO_INTCR_Array_TypeDef;

typedef struct {
  __IO uint32_t INTFR;
} GPIO_INTFR_TypeDef;
// clang-format on
/**
 * @}
 */

/** @addtogroup Peripheral_memory_map
 * @{
 */

/*!< Peripheral memory map */
#define GPIO_BASE       (0x5012F000)
#define GPIO_AFSEL_BASE (GPIO_BASE + 0x000UL)
#define GPIO_INTCR_BASE (GPIO_BASE + 0x100UL)
#define GPIO_INTFR_BASE (GPIO_BASE + 0x120UL)

#define GPIOA_BASE (GPIO_BASE + 0x130UL)
#define GPIOB_BASE (GPIO_BASE + 0x134UL)
#define GPIOC_BASE (GPIO_BASE + 0x138UL)
#define GPIOD_BASE (GPIO_BASE + 0x13CUL)
#define GPIOE_BASE (GPIO_BASE + 0x140UL)
#define GPIOF_BASE (GPIO_BASE + 0x144UL)

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF ((GPIO_TypeDef *)GPIOF_BASE)

#define GPIO_AFSEL_ARRAY ((GPIO_AFSEL_Array_TypeDef *)GPIO_AFSEL_BASE)
#define GPIO_INTCR_ARRAY ((GPIO_INTCR_Array_TypeDef *)GPIO_INTCR_BASE)
#define GPIO_INTFR       ((GPIO_INTFR_TypeDef *)GPIO_INTFR_BASE)
/**
 * @}
 */

/** @addtogroup Peripheral_Registers_Bits_Definition
 * @{
 */
/******************************************************************************/
/*                                                                            */
/*                            General Purpose I/O                             */
/*                                                                            */
/******************************************************************************/
#define GPIO_INTCR_NUMBER 8U

/******************  Bits definition for GPIOPU register  *****************/
#define GPIO_PUCR_PUPD0_Pos (0U)
#define GPIO_PUCR_PUPD0_Msk (0x1UL << GPIO_PUCR_PUPD0_Pos) /*!< 0x00000001 */
#define GPIO_PUCR_PUPD0     GPIO_PUCR_PUPD0_Msk

#define GPIO_PUCR_PUPD1_Pos (1U)
#define GPIO_PUCR_PUPD1_Msk (0x1UL << GPIO_PUCR_PUPD1_Pos) /*!< 0x00000002 */
#define GPIO_PUCR_PUPD1     GPIO_PUCR_PUPD1_Msk

#define GPIO_PUCR_PUPD2_Pos (2U)
#define GPIO_PUCR_PUPD2_Msk (0x1UL << GPIO_PUCR_PUPD2_Pos) /*!< 0x00000004 */
#define GPIO_PUCR_PUPD2     GPIO_PUCR_PUPD2_Msk

#define GPIO_PUCR_PUPD3_Pos (3U)
#define GPIO_PUCR_PUPD3_Msk (0x1UL << GPIO_PUCR_PUPD3_Pos) /*!< 0x00000008 */
#define GPIO_PUCR_PUPD3     GPIO_PUCR_PUPD3_Msk

#define GPIO_PUCR_PUPD4_Pos (4U)
#define GPIO_PUCR_PUPD4_Msk (0x1UL << GPIO_PUCR_PUPD4_Pos) /*!< 0x00000010 */
#define GPIO_PUCR_PUPD4     GPIO_PUCR_PUPD4_Msk

#define GPIO_PUCR_PUPD5_Pos (5U)
#define GPIO_PUCR_PUPD5_Msk (0x1UL << GPIO_PUCR_PUPD5_Pos) /*!< 0x00000020 */
#define GPIO_PUCR_PUPD5     GPIO_PUCR_PUPD5_Msk

#define GPIO_PUCR_PUPD6_Pos (6U)
#define GPIO_PUCR_PUPD6_Msk (0x1UL << GPIO_PUCR_PUPD6_Pos) /*!< 0x00000040 */
#define GPIO_PUCR_PUPD6     GPIO_PUCR_PUPD6_Msk

#define GPIO_PUCR_PUPD7_Pos (7U)
#define GPIO_PUCR_PUPD7_Msk (0x1UL << GPIO_PUCR_PUPD7_Pos) /*!< 0x00000080 */
#define GPIO_PUCR_PUPD7     GPIO_PUCR_PUPD7_Msk

#define GPIO_PUCR_PUPD8_Pos (8U)
#define GPIO_PUCR_PUPD8_Msk (0x1UL << GPIO_PUCR_PUPD8_Pos) /*!< 0x00000100 */
#define GPIO_PUCR_PUPD8     GPIO_PUCR_PUPD8_Msk

#define GPIO_PUCR_PUPD9_Pos (9U)
#define GPIO_PUCR_PUPD9_Msk (0x1UL << GPIO_PUCR_PUPD9_Pos) /*!< 0x00000200 */
#define GPIO_PUCR_PUPD9     GPIO_PUCR_PUPD9_Msk

#define GPIO_PUCR_PUPD10_Pos (10U)
#define GPIO_PUCR_PUPD10_Msk (0x1UL << GPIO_PUCR_PUPD10_Pos) /*!< 0x00000400 */
#define GPIO_PUCR_PUPD10     GPIO_PUCR_PUPD10_Msk

#define GPIO_PUCR_PUPD11_Pos (11U)
#define GPIO_PUCR_PUPD11_Msk (0x1UL << GPIO_PUCR_PUPD11_Pos) /*!< 0x00000800 */
#define GPIO_PUCR_PUPD11     GPIO_PUCR_PUPD11_Msk

#define GPIO_PUCR_PUPD12_Pos (12U)
#define GPIO_PUCR_PUPD12_Msk (0x1UL << GPIO_PUCR_PUPD12_Pos) /*!< 0x00001000 */
#define GPIO_PUCR_PUPD12     GPIO_PUCR_PUPD12_Msk

#define GPIO_PUCR_PUPD13_Pos (13U)
#define GPIO_PUCR_PUPD13_Msk (0x1UL << GPIO_PUCR_PUPD13_Pos) /*!< 0x00002000 */
#define GPIO_PUCR_PUPD13     GPIO_PUCR_PUPD13_Msk

#define GPIO_PUCR_PUPD14_Pos (14U)
#define GPIO_PUCR_PUPD14_Msk (0x1UL << GPIO_PUCR_PUPD14_Pos) /*!< 0x00004000 */
#define GPIO_PUCR_PUPD14     GPIO_PUCR_PUPD14_Msk

#define GPIO_PUCR_PUPD15_Pos (15U)
#define GPIO_PUCR_PUPD15_Msk (0x1UL << GPIO_PUCR_PUPD15_Pos) /*!< 0x00008000 */
#define GPIO_PUCR_PUPD15     GPIO_PUCR_PUPD15_Msk

/***************** Bit definition for GPIO_AFSELL register ********************/
#define GPIO_AFSELL_PX0_Pos  (0U)
#define GPIO_AFSELL_PX0_Msk  (0x3UL << GPIO_AFSELL_PX0_Pos) /*!< 0x00000003 */
#define GPIO_AFSELL_PX0      GPIO_AFSELL_PX0_Msk
#define GPIO_AFSELL_PX0_GPIO (0x0UL << GPIO_AFSELL_PX0_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX0_AF1  (0x1UL << GPIO_AFSELL_PX0_Pos) /*!< 0x00000001 */
#define GPIO_AFSELL_PX0_AF2  (0x2UL << GPIO_AFSELL_PX0_Pos) /*!< 0x00000002 */
#define GPIO_AFSELL_PX0_AF3  (0x3UL << GPIO_AFSELL_PX0_Pos) /*!< 0x00000003 */

#define GPIO_AFSELL_PX1_Pos  (2U)
#define GPIO_AFSELL_PX1_Msk  (0x3UL << GPIO_AFSELL_PX1_Pos) /*!< 0x0000000C */
#define GPIO_AFSELL_PX1      GPIO_AFSELL_PX1_Msk
#define GPIO_AFSELL_PX1_GPIO (0x0UL << GPIO_AFSELL_PX1_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX1_AF1  (0x1UL << GPIO_AFSELL_PX1_Pos) /*!< 0x00000004 */
#define GPIO_AFSELL_PX1_AF2  (0x2UL << GPIO_AFSELL_PX1_Pos) /*!< 0x00000008 */
#define GPIO_AFSELL_PX1_AF3  (0x3UL << GPIO_AFSELL_PX1_Pos) /*!< 0x0000000C */

#define GPIO_AFSELL_PX2_Pos  (4U)
#define GPIO_AFSELL_PX2_Msk  (0x3UL << GPIO_AFSELL_PX2_Pos) /*!< 0x00000030 */
#define GPIO_AFSELL_PX2      GPIO_AFSELL_PX2_Msk
#define GPIO_AFSELL_PX2_GPIO (0x0UL << GPIO_AFSELL_PX2_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX2_AF1  (0x1UL << GPIO_AFSELL_PX2_Pos) /*!< 0x00000010 */
#define GPIO_AFSELL_PX2_AF2  (0x2UL << GPIO_AFSELL_PX2_Pos) /*!< 0x00000020 */
#define GPIO_AFSELL_PX2_AF3  (0x3UL << GPIO_AFSELL_PX2_Pos) /*!< 0x00000030 */

#define GPIO_AFSELL_PX3_Pos  (6U)
#define GPIO_AFSELL_PX3_Msk  (0x3UL << GPIO_AFSELL_PX3_Pos) /*!< 0x000000C0 */
#define GPIO_AFSELL_PX3      GPIO_AFSELL_PX3_Msk
#define GPIO_AFSELL_PX3_GPIO (0x0UL << GPIO_AFSELL_PX3_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX3_AF1  (0x1UL << GPIO_AFSELL_PX3_Pos) /*!< 0x00000040 */
#define GPIO_AFSELL_PX3_AF2  (0x2UL << GPIO_AFSELL_PX3_Pos) /*!< 0x00000080 */
#define GPIO_AFSELL_PX3_AF3  (0x3UL << GPIO_AFSELL_PX3_Pos) /*!< 0x000000C0 */

#define GPIO_AFSELL_PX4_Pos  (8U)
#define GPIO_AFSELL_PX4_Msk  (0x3UL << GPIO_AFSELL_PX4_Pos) /*!< 0x00000300 */
#define GPIO_AFSELL_PX4      GPIO_AFSELL_PX4_Msk
#define GPIO_AFSELL_PX4_GPIO (0x0UL << GPIO_AFSELL_PX4_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX4_AF1  (0x1UL << GPIO_AFSELL_PX4_Pos) /*!< 0x00000100 */
#define GPIO_AFSELL_PX4_AF2  (0x2UL << GPIO_AFSELL_PX4_Pos) /*!< 0x00000200 */
#define GPIO_AFSELL_PX4_AF3  (0x3UL << GPIO_AFSELL_PX4_Pos) /*!< 0x00000300 */

#define GPIO_AFSELL_PX5_Pos  (10U)
#define GPIO_AFSELL_PX5_Msk  (0x3UL << GPIO_AFSELL_PX5_Pos) /*!< 0x00000C00 */
#define GPIO_AFSELL_PX5      GPIO_AFSELL_PX5_Msk
#define GPIO_AFSELL_PX5_GPIO (0x0UL << GPIO_AFSELL_PX5_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX5_AF1  (0x1UL << GPIO_AFSELL_PX5_Pos) /*!< 0x00000400 */
#define GPIO_AFSELL_PX5_AF2  (0x2UL << GPIO_AFSELL_PX5_Pos) /*!< 0x00000800 */
#define GPIO_AFSELL_PX5_AF3  (0x3UL << GPIO_AFSELL_PX5_Pos) /*!< 0x00000C00 */

#define GPIO_AFSELL_PX6_Pos  (12U)
#define GPIO_AFSELL_PX6_Msk  (0x3UL << GPIO_AFSELL_PX6_Pos) /*!< 0x00003000 */
#define GPIO_AFSELL_PX6      GPIO_AFSELL_PX6_Msk
#define GPIO_AFSELL_PX6_GPIO (0x0UL << GPIO_AFSELL_PX6_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX6_AF1  (0x1UL << GPIO_AFSELL_PX6_Pos) /*!< 0x00001000 */
#define GPIO_AFSELL_PX6_AF2  (0x2UL << GPIO_AFSELL_PX6_Pos) /*!< 0x00002000 */
#define GPIO_AFSELL_PX6_AF3  (0x3UL << GPIO_AFSELL_PX6_Pos) /*!< 0x00003000 */

#define GPIO_AFSELL_PX7_Pos  (14U)
#define GPIO_AFSELL_PX7_Msk  (0x3UL << GPIO_AFSELL_PX7_Pos) /*!< 0x0000C000 */
#define GPIO_AFSELL_PX7      GPIO_AFSELL_PX7_Msk
#define GPIO_AFSELL_PX7_GPIO (0x0UL << GPIO_AFSELL_PX7_Pos) /*!< 0x00000000 */
#define GPIO_AFSELL_PX7_AF1  (0x1UL << GPIO_AFSELL_PX7_Pos) /*!< 0x00004000 */
#define GPIO_AFSELL_PX7_AF2  (0x2UL << GPIO_AFSELL_PX7_Pos) /*!< 0x00008000 */
#define GPIO_AFSELL_PX7_AF3  (0x3UL << GPIO_AFSELL_PX7_Pos) /*!< 0x0000C000 */

/****************** Bit definition for GPIO_AFSELH register
 * ********************/
#define GPIO_AFSELH_PX8_Pos  (0U)
#define GPIO_AFSELH_PX8_Msk  (0x3UL << GPIO_AFSELH_PX8_Pos) /*!< 0x00000003 */
#define GPIO_AFSELH_PX8      GPIO_AFSELH_PX8_Msk
#define GPIO_AFSELH_PX8_GPIO (0x0UL << GPIO_AFSELH_PX8_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX8_AF1  (0x1UL << GPIO_AFSELH_PX8_Pos) /*!< 0x00000001 */
#define GPIO_AFSELH_PX8_AF2  (0x2UL << GPIO_AFSELH_PX8_Pos) /*!< 0x00000002 */
#define GPIO_AFSELH_PX8_AF3  (0x3UL << GPIO_AFSELH_PX8_Pos) /*!< 0x00000003 */

#define GPIO_AFSELH_PX9_Pos  (2U)
#define GPIO_AFSELH_PX9_Msk  (0x3UL << GPIO_AFSELH_PX9_Pos) /*!< 0x0000000C */
#define GPIO_AFSELH_PX9      GPIO_AFSELH_PX9_Msk
#define GPIO_AFSELH_PX9_GPIO (0x0UL << GPIO_AFSELH_PX9_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX9_AF1  (0x1UL << GPIO_AFSELH_PX9_Pos) /*!< 0x00000004 */
#define GPIO_AFSELH_PX9_AF2  (0x2UL << GPIO_AFSELH_PX9_Pos) /*!< 0x00000008 */
#define GPIO_AFSELH_PX9_AF3  (0x3UL << GPIO_AFSELH_PX9_Pos) /*!< 0x0000000C */

#define GPIO_AFSELH_PX10_Pos  (4U)
#define GPIO_AFSELH_PX10_Msk  (0x3UL << GPIO_AFSELH_PX10_Pos) /*!< 0x00000030 */
#define GPIO_AFSELH_PX10      GPIO_AFSELH_PX10_Msk
#define GPIO_AFSELH_PX10_GPIO (0x0UL << GPIO_AFSELH_PX10_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX10_AF1  (0x1UL << GPIO_AFSELH_PX10_Pos) /*!< 0x00000010 */
#define GPIO_AFSELH_PX10_AF2  (0x2UL << GPIO_AFSELH_PX10_Pos) /*!< 0x00000020 */
#define GPIO_AFSELH_PX10_AF3  (0x3UL << GPIO_AFSELH_PX10_Pos) /*!< 0x00000030 */

#define GPIO_AFSELH_PX11_Pos  (6U)
#define GPIO_AFSELH_PX11_Msk  (0x3UL << GPIO_AFSELH_PX11_Pos) /*!< 0x000000C0 */
#define GPIO_AFSELH_PX11      GPIO_AFSELH_PX11_Msk
#define GPIO_AFSELH_PX11_GPIO (0x0UL << GPIO_AFSELH_PX11_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX11_AF1  (0x1UL << GPIO_AFSELH_PX11_Pos) /*!< 0x00000040 */
#define GPIO_AFSELH_PX11_AF2  (0x2UL << GPIO_AFSELH_PX11_Pos) /*!< 0x00000080 */
#define GPIO_AFSELH_PX11_AF3  (0x3UL << GPIO_AFSELH_PX11_Pos) /*!< 0x000000C0 */

#define GPIO_AFSELH_PX12_Pos  (8U)
#define GPIO_AFSELH_PX12_Msk  (0x3UL << GPIO_AFSELH_PX12_Pos) /*!< 0x00000300 */
#define GPIO_AFSELH_PX12      GPIO_AFSELH_PX12_Msk
#define GPIO_AFSELH_PX12_GPIO (0x0UL << GPIO_AFSELH_PX12_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX12_AF1  (0x1UL << GPIO_AFSELH_PX12_Pos) /*!< 0x00000100 */
#define GPIO_AFSELH_PX12_AF2  (0x2UL << GPIO_AFSELH_PX12_Pos) /*!< 0x00000200 */
#define GPIO_AFSELH_PX12_AF3  (0x3UL << GPIO_AFSELH_PX12_Pos) /*!< 0x00000300 */

#define GPIO_AFSELH_PX13_Pos  (10U)
#define GPIO_AFSELH_PX13_Msk  (0x3UL << GPIO_AFSELH_PX13_Pos) /*!< 0x00000C00 */
#define GPIO_AFSELH_PX13      GPIO_AFSELH_PX13_Msk
#define GPIO_AFSELH_PX13_GPIO (0x0UL << GPIO_AFSELH_PX13_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX13_AF1  (0x1UL << GPIO_AFSELH_PX13_Pos) /*!< 0x00000400 */
#define GPIO_AFSELH_PX13_AF2  (0x2UL << GPIO_AFSELH_PX13_Pos) /*!< 0x00000800 */
#define GPIO_AFSELH_PX13_AF3  (0x3UL << GPIO_AFSELH_PX13_Pos) /*!< 0x00000C00 */

#define GPIO_AFSELH_PX14_Pos  (12U)
#define GPIO_AFSELH_PX14_Msk  (0x3UL << GPIO_AFSELH_PX14_Pos) /*!< 0x00003000 */
#define GPIO_AFSELH_PX14      GPIO_AFSELH_PX14_Msk
#define GPIO_AFSELH_PX14_GPIO (0x0UL << GPIO_AFSELH_PX14_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX14_AF1  (0x1UL << GPIO_AFSELH_PX14_Pos) /*!< 0x00001000 */
#define GPIO_AFSELH_PX14_AF2  (0x2UL << GPIO_AFSELH_PX14_Pos) /*!< 0x00002000 */
#define GPIO_AFSELH_PX14_AF3  (0x3UL << GPIO_AFSELH_PX14_Pos) /*!< 0x00003000 */

#define GPIO_AFSELH_PX15_Pos  (14U)
#define GPIO_AFSELH_PX15_Msk  (0x3UL << GPIO_AFSELH_PX15_Pos) /*!< 0x0000C000 */
#define GPIO_AFSELH_PX15      GPIO_AFSELH_PX15_Msk
#define GPIO_AFSELH_PX15_GPIO (0x0UL << GPIO_AFSELH_PX15_Pos) /*!< 0x00000000 */
#define GPIO_AFSELH_PX15_AF1  (0x1UL << GPIO_AFSELH_PX15_Pos) /*!< 0x00004000 */
#define GPIO_AFSELH_PX15_AF2  (0x2UL << GPIO_AFSELH_PX15_Pos) /*!< 0x00008000 */
#define GPIO_AFSELH_PX15_AF3  (0x3UL << GPIO_AFSELH_PX15_Pos) /*!< 0x0000C000 */

/******************  Bits definition for GPIOOE register  *****************/
#define GPIO_OE_D0_Pos (0U)
#define GPIO_OE_D0_Msk (0x1UL << GPIO_OE_D0_Pos) /*!< 0x00000001 */
#define GPIO_OE_D0     GPIO_OE_D0_Msk

#define GPIO_OE_D1_Pos (1U)
#define GPIO_OE_D1_Msk (0x1UL << GPIO_OE_D1_Pos) /*!< 0x00000002 */
#define GPIO_OE_D1     GPIO_OE_D1_Msk

#define GPIO_OE_D2_Pos (2U)
#define GPIO_OE_D2_Msk (0x1UL << GPIO_OE_D2_Pos) /*!< 0x00000004 */
#define GPIO_OE_D2     GPIO_OE_D2_Msk

#define GPIO_OE_D3_Pos (3U)
#define GPIO_OE_D3_Msk (0x1UL << GPIO_OE_D3_Pos) /*!< 0x00000008 */
#define GPIO_OE_D3     GPIO_OE_D3_Msk

#define GPIO_OE_D4_Pos (4U)
#define GPIO_OE_D4_Msk (0x1UL << GPIO_OE_D4_Pos) /*!< 0x00000010 */
#define GPIO_OE_D4     GPIO_OE_D4_Msk

#define GPIO_OE_D5_Pos (5U)
#define GPIO_OE_D5_Msk (0x1UL << GPIO_OE_D5_Pos) /*!< 0x00000020 */
#define GPIO_OE_D5     GPIO_OE_D5_Msk

#define GPIO_OE_D6_Pos (6U)
#define GPIO_OE_D6_Msk (0x1UL << GPIO_OE_D6_Pos) /*!< 0x00000040 */
#define GPIO_OE_D6     GPIO_OE_D6_Msk

#define GPIO_OE_D7_Pos (7U)
#define GPIO_OE_D7_Msk (0x1UL << GPIO_OE_D7_Pos) /*!< 0x00000080 */
#define GPIO_OE_D7     GPIO_OE_D7_Msk

#define GPIO_OE_D8_Pos (8U)
#define GPIO_OE_D8_Msk (0x1UL << GPIO_OE_D8_Pos) /*!< 0x00000100 */
#define GPIO_OE_D8     GPIO_OE_D8_Msk

#define GPIO_OE_D9_Pos (9U)
#define GPIO_OE_D9_Msk (0x1UL << GPIO_OE_D9_Pos) /*!< 0x00000200 */
#define GPIO_OE_D9     GPIO_OE_D9_Msk

#define GPIO_OE_D10_Pos (10U)
#define GPIO_OE_D10_Msk (0x1UL << GPIO_OE_D10_Pos) /*!< 0x00000400 */
#define GPIO_OE_D10     GPIO_OE_D10_Msk

#define GPIO_OE_D11_Pos (11U)
#define GPIO_OE_D11_Msk (0x1UL << GPIO_OE_D11_Pos) /*!< 0x00000800 */
#define GPIO_OE_D11     GPIO_OE_D11_Msk

#define GPIO_OE_D12_Pos (12U)
#define GPIO_OE_D12_Msk (0x1UL << GPIO_OE_D12_Pos) /*!< 0x00001000 */
#define GPIO_OE_D12     GPIO_OE_D12_Msk

#define GPIO_OE_D13_Pos (13U)
#define GPIO_OE_D13_Msk (0x1UL << GPIO_OE_D13_Pos) /*!< 0x00002000 */
#define GPIO_OE_D13     GPIO_OE_D13_Msk

#define GPIO_OE_D14_Pos (14U)
#define GPIO_OE_D14_Msk (0x1UL << GPIO_OE_D14_Pos) /*!< 0x00004000 */
#define GPIO_OE_D14     GPIO_OE_D14_Msk

#define GPIO_OE_D15_Pos (15U)
#define GPIO_OE_D15_Msk (0x1UL << GPIO_OE_D15_Pos) /*!< 0x00008000 */
#define GPIO_OE_D15     GPIO_OE_D15_Msk

/******************  Bits definition for GPIOIN register  *****************/
#define GPIO_IN_D0_Pos (0U)
#define GPIO_IN_D0_Msk (0x1UL << GPIO_IN_D0_Pos) /*!< 0x00000001 */
#define GPIO_IN_D0     GPIO_IN_D0_Msk

#define GPIO_IN_D1_Pos (1U)
#define GPIO_IN_D1_Msk (0x1UL << GPIO_IN_D1_Pos) /*!< 0x00000002 */
#define GPIO_IN_D1     GPIO_IN_D1_Msk

#define GPIO_IN_D2_Pos (2U)
#define GPIO_IN_D2_Msk (0x1UL << GPIO_IN_D2_Pos) /*!< 0x00000004 */
#define GPIO_IN_D2     GPIO_IN_D2_Msk

#define GPIO_IN_D3_Pos (3U)
#define GPIO_IN_D3_Msk (0x1UL << GPIO_IN_D3_Pos) /*!< 0x00000008 */
#define GPIO_IN_D3     GPIO_IN_D3_Msk

#define GPIO_IN_D4_Pos (4U)
#define GPIO_IN_D4_Msk (0x1UL << GPIO_IN_D4_Pos) /*!< 0x00000010 */
#define GPIO_IN_D4     GPIO_IN_D4_Msk

#define GPIO_IN_D5_Pos (5U)
#define GPIO_IN_D5_Msk (0x1UL << GPIO_IN_D5_Pos) /*!< 0x00000020 */
#define GPIO_IN_D5     GPIO_IN_D5_Msk

#define GPIO_IN_D6_Pos (6U)
#define GPIO_IN_D6_Msk (0x1UL << GPIO_IN_D6_Pos) /*!< 0x00000040 */
#define GPIO_IN_D6     GPIO_IN_D6_Msk

#define GPIO_IN_D7_Pos (7U)
#define GPIO_IN_D7_Msk (0x1UL << GPIO_IN_D7_Pos) /*!< 0x00000080 */
#define GPIO_IN_D7     GPIO_IN_D7_Msk

#define GPIO_IN_D8_Pos (8U)
#define GPIO_IN_D8_Msk (0x1UL << GPIO_IN_D8_Pos) /*!< 0x00000100 */
#define GPIO_IN_D8     GPIO_IN_D8_Msk

#define GPIO_IN_D9_Pos (9U)
#define GPIO_IN_D9_Msk (0x1UL << GPIO_IN_D9_Pos) /*!< 0x00000200 */
#define GPIO_IN_D9     GPIO_IN_D9_Msk

#define GPIO_IN_D10_Pos (10U)
#define GPIO_IN_D10_Msk (0x1UL << GPIO_IN_D10_Pos) /*!< 0x00000400 */
#define GPIO_IN_D10     GPIO_IN_D10_Msk

#define GPIO_IN_D11_Pos (11U)
#define GPIO_IN_D11_Msk (0x1UL << GPIO_IN_D11_Pos) /*!< 0x00000800 */
#define GPIO_IN_D11     GPIO_IN_D11_Msk

#define GPIO_IN_D12_Pos (12U)
#define GPIO_IN_D12_Msk (0x1UL << GPIO_IN_D12_Pos) /*!< 0x00001000 */
#define GPIO_IN_D12     GPIO_IN_D12_Msk

#define GPIO_IN_D13_Pos (13U)
#define GPIO_IN_D13_Msk (0x1UL << GPIO_IN_D13_Pos) /*!< 0x00002000 */
#define GPIO_IN_D13     GPIO_IN_D13_Msk

#define GPIO_IN_D14_Pos (14U)
#define GPIO_IN_D14_Msk (0x1UL << GPIO_IN_D14_Pos) /*!< 0x00004000 */
#define GPIO_IN_D14     GPIO_IN_D14_Msk

#define GPIO_IN_D15_Pos (15U)
#define GPIO_IN_D15_Msk (0x1UL << GPIO_IN_D15_Pos) /*!< 0x00008000 */
#define GPIO_IN_D15     GPIO_IN_D15_Msk

/******************  Bits definition for INTCR register  *****************/
#define GPIO_INTCR_INTSEL_Pos (0U)
#define GPIO_INTCR_INTSEL_Msk (0x7FUL << GPIO_INTCR_INTSEL_Pos) /*!< 0x0000007F */
#define GPIO_INTCR_INTSEL     GPIO_INTCR_INTSEL_Msk

#define GPIO_INTCR_INTMOD_Pos        (7U)
#define GPIO_INTCR_INTMOD_Msk        (0x3UL << GPIO_INTCR_INTMOD_Pos) /*!< 0x00000000 */
#define GPIO_INTCR_INTMOD            GPIO_INTCR_INTMOD_Msk
#define GPIO_INTCR_INTMOD_RISE_EDGE  (0x0UL << GPIO_INTCR_INTMOD_Pos) /*!< 0x00000000 */
#define GPIO_INTCR_INTMOD_FALL_EDGE  (0x1UL << GPIO_INTCR_INTMOD_Pos) /*!< 0x00000080 */
#define GPIO_INTCR_INTMOD_HIGH_LEVEL (0x2UL << GPIO_INTCR_INTMOD_Pos) /*!< 0x00000100 */
#define GPIO_INTCR_INTMOD_LOW_LEVEL  (0x3UL << GPIO_INTCR_INTMOD_Pos) /*!< 0x00000180 */

#define GPIO_INTCR_INTEN_Pos (9U)
#define GPIO_INTCR_INTEN_Msk (0x1UL << GPIO_INTCR_INTEN_Pos) /*!< 0x000000200 */
#define GPIO_INTCR_INTEN     GPIO_INTCR_INTEN_Msk

#define GPIO_INTCR_WKUPE_Pos (10U)
#define GPIO_INTCR_WKUPE_Msk (0x1UL << GPIO_INTCR_WKUPE_Pos) /*!< 0x00000400 */
#define GPIO_INTCR_WKUPE     GPIO_INTCR_WKUPE_Msk

/**
 * @}
 */
/** @addtogroup Exported_macros
 * @{
 */

/******************************* GPIO Instances *******************************/
#define IS_GPIO_ALL_INSTANCE(INSTANCE) \
    (((INSTANCE) == GPIOA) || ((INSTANCE) == GPIOB) || ((INSTANCE) == GPIOC) || ((INSTANCE) == GPIOD) || ((INSTANCE) == GPIOE) || ((INSTANCE) == GPIOF))

/******************************* GPIO AF **************************************/
#define IS_GPIO_AF(INSTANCE) (((INSTANCE) == (uint16_t)0x0001) || ((INSTANCE) == (uint16_t)0x0002) || ((INSTANCE) == (uint16_t)0x0003))

/**
 * @}
 */
/* ================  End of section using anonymous unions  ================ */
#if defined(__CC_ARM)
#pragma pop
#elif defined(__ICCARM__)
/* leave anonymous unions enabled */
#elif (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
#pragma clang diagnostic pop
#elif defined(__GNUC__)
/* anonymous unions are enabled by default */
#elif defined(__TMS470__)
/* anonymous unions are enabled by default */
#elif defined(__TASKING__)
#pragma warning restore
#elif defined(__CSMC__)
/* anonymous unions are enabled by default */
#else
#warning Not supported compiler type
#endif

#endif /* DARIC_H */
