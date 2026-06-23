/**
 *******************************************************************************
 * @file    startup_daric.c
 * @author  Daric Team
 * @brief   Source file for startup_daric.c module.
 *******************************************************************************
 * @attention
 *
 * Copyright 2024-2026 CrossBar, Inc.
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
 *
 *******************************************************************************
 */
/*
 * Copyright (c) 2009-2020 Arm Limited. All rights reserved.
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
#include <stdint.h>
#include "daric.h"
#include "daric_util.h"
#include "daric_hal.h"

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;

#if defined(__ARMCC_VERSION)
extern __NO_RETURN void __PROGRAM_START(void);
#elif defined(__GNUC__)
// GCC wrapper to ensure proper scatter loading via __cmsis_start
static void __NO_RETURN GCC_Start(void)
{
    __cmsis_start();
}
#undef __PROGRAM_START
#define __PROGRAM_START GCC_Start
#else
#error "Unknown Compiler"
#endif

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler(void);
void             Default_Handler(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Exceptions */
void NMI_Handler(void) __attribute__((weak));
void HardFault_Handler(void) __attribute__((weak));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void);

void EXT_IRQ16_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ17_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ18_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ19_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ20_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ21_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ22_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ23_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ24_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ25_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ26_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ27_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ28_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ29_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ30_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ31_Handler(void) __attribute__((weak, alias("Default_Handler")));

// sce reserved
void EXT_IRQ32_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ33_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ34_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ35_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ36_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ37_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ38_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ39_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ40_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ41_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ42_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ43_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ44_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ45_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ46_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ47_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ48_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ49_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ50_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ51_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ52_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ53_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ54_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ55_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ56_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ57_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ58_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ59_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ60_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ61_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ62_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ63_Handler(void) __attribute__((weak, alias("Default_Handler")));

// Peripherals

// uart0
void UART0_RX_IRQ_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_TX_IRQ_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_POLL_IRQ_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_EOT_IRQ_Handler(void) __attribute__((weak, alias("Default_Handler")));

// uart1
void EXT_IRQ68_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ69_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ70_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ71_Handler(void) __attribute__((weak, alias("Default_Handler")));

// uart2
void EXT_IRQ72_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ73_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ74_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ75_Handler(void) __attribute__((weak, alias("Default_Handler")));

// uart3
void EXT_IRQ76_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ77_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ78_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ79_Handler(void) __attribute__((weak, alias("Default_Handler")));

// spim0
void EXT_IRQ80_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ81_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ82_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ83_Handler(void) __attribute__((weak, alias("Default_Handler")));

// spim1
void EXT_IRQ84_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ85_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ86_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ87_Handler(void) __attribute__((weak, alias("Default_Handler")));

// spim2
void EXT_IRQ88_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ89_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ90_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ91_Handler(void) __attribute__((weak, alias("Default_Handler")));

// spim3
void EXT_IRQ92_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ93_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ94_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ95_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c0
void EXT_IRQ96_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ97_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ98_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ99_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c1
void EXT_IRQ100_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ101_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ102_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ103_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c2
void EXT_IRQ104_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ105_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ106_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ107_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c3
void EXT_IRQ108_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ109_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ110_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ111_Handler(void) __attribute__((weak, alias("Default_Handler")));

// sdio
void EXT_IRQ112_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ113_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ114_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ115_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2s
void EXT_IRQ116_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ117_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ118_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ119_Handler(void) __attribute__((weak, alias("Default_Handler")));

// camif
void EXT_IRQ120_Handler(void) __attribute__((weak, alias("Default_Handler")));

// adc
void EXT_IRQ121_Handler(void) __attribute__((weak, alias("Default_Handler")));

void EXT_IRQ122_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ123_Handler(void) __attribute__((weak, alias("Default_Handler")));

// filter
void EXT_IRQ124_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ125_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ126_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ127_Handler(void) __attribute__((weak, alias("Default_Handler")));

// scif
void EXT_IRQ128_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ129_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ130_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ131_Handler(void) __attribute__((weak, alias("Default_Handler")));

// spis0
void EXT_IRQ132_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ133_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ134_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ135_Handler(void) __attribute__((weak, alias("Default_Handler")));

// spis1
void EXT_IRQ136_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ137_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ138_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ139_Handler(void) __attribute__((weak, alias("Default_Handler")));

// PWM
void EXT_IRQ140_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ141_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ142_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ143_Handler(void) __attribute__((weak, alias("Default_Handler")));

// gpio
void EXT_IRQ144_Handler(void) __attribute__((weak, alias("Default_Handler")));

// usb
void EXT_IRQ145_Handler(void) __attribute__((weak, alias("Default_Handler")));

// sdc
void EXT_IRQ146_Handler(void) __attribute__((weak, alias("Default_Handler")));

// pio
void EXT_IRQ147_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ148_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ149_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ150_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ151_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ152_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ153_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ154_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ155_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ156_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ157_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ158_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ159_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ160_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ161_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ162_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ163_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ164_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ165_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ166_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ167_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ168_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ169_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ170_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ171_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ172_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ173_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ174_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ175_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ176_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ177_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ178_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ179_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ180_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ181_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ182_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ183_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c0.nack
void EXT_IRQ184_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c1.nack
void EXT_IRQ185_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c2.nack
void EXT_IRQ186_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c3.nack
void EXT_IRQ187_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c0.err
void EXT_IRQ188_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c1.err
void EXT_IRQ189_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c2.err
void EXT_IRQ190_Handler(void) __attribute__((weak, alias("Default_Handler")));

// i2c3.err
void EXT_IRQ191_Handler(void) __attribute__((weak, alias("Default_Handler")));

// reserved
void EXT_IRQ192_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ193_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ194_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ195_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ196_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ197_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ198_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ199_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ200_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ201_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ202_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ203_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ204_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ205_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ206_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ207_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ208_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ209_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ210_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ211_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ212_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ213_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ214_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ215_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ216_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ217_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ218_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ219_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ220_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ221_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ222_Handler(void) __attribute__((weak, alias("Default_Handler")));
void EXT_IRQ223_Handler(void) __attribute__((weak, alias("Default_Handler")));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
extern const VECTOR_TABLE_Type __VECTOR_TABLE[240];
const VECTOR_TABLE_Type        __VECTOR_TABLE[240] __VECTOR_TABLE_ATTRIBUTE = {
    (VECTOR_TABLE_Type)(&__INITIAL_SP), /*     Initial Stack Pointer */
    Reset_Handler,                      /*     Reset Handler */
    NMI_Handler,                        /* -14 NMI Handler */
    HardFault_Handler,                  /* -13 Hard Fault Handler */
    MemManage_Handler,                  /* -12 MPU Fault Handler */
    BusFault_Handler,                   /* -11 Bus Fault Handler */
    UsageFault_Handler,                 /* -10 Usage Fault Handler */
    0,                                  /*     Reserved */
    0,                                  /*     Reserved */
    0,                                  /*     Reserved */
    0,                                  /*     Reserved */
    SVC_Handler,                        /*  -5 SVC Handler */
    DebugMon_Handler,                   /*  -4 Debug Monitor Handler */
    0,                                  /*     Reserved */
    PendSV_Handler,                     /*  -2 PendSV Handler */
    SysTick_Handler,                    /*  -1 SysTick Handler */

    // 16 ~ 47 reserved
    EXT_IRQ16_Handler, /*     EXT IRQ0 Reserved */
    EXT_IRQ17_Handler, /*     EXT IRQ1 Reserved */
    EXT_IRQ18_Handler, /*     EXT IRQ2 Reserved */
    EXT_IRQ19_Handler, /*     EXT IRQ3 Reserved */
    EXT_IRQ20_Handler, /*     EXT IRQ4 Reserved */
    EXT_IRQ21_Handler, /*     EXT IRQ5 Reserved */
    EXT_IRQ22_Handler, /*     EXT IRQ6 Reserved */
    EXT_IRQ23_Handler, /*     EXT IRQ7 Reserved */
    EXT_IRQ24_Handler, /*     EXT IRQ8 Reserved */
    EXT_IRQ25_Handler, /*     EXT IRQ9 Reserved */
    EXT_IRQ26_Handler, /*     EXT IRQ10 Reserved */
    EXT_IRQ27_Handler, /*     EXT IRQ11 Reserved */
    EXT_IRQ28_Handler, /*     EXT IRQ12 Reserved */
    EXT_IRQ29_Handler, /*     EXT IRQ13 Reserved */
    EXT_IRQ30_Handler, /*     EXT IRQ14 Reserved */
    EXT_IRQ31_Handler, /*     EXT IRQ15 Reserved */

    /* Interrupts of SCE 48~79*/
    EXT_IRQ32_Handler, /*     EXT IRQ32 Reserved */
    EXT_IRQ33_Handler, /*     EXT IRQ33 Reserved */
    EXT_IRQ34_Handler, /*     EXT IRQ34 Reserved */
    EXT_IRQ35_Handler, /*     EXT IRQ35 Reserved */
    EXT_IRQ36_Handler, /*     EXT IRQ36 Reserved */
    EXT_IRQ37_Handler, /*     EXT IRQ37 Reserved */
    EXT_IRQ38_Handler, /*     EXT IRQ38 Reserved */
    EXT_IRQ39_Handler, /*     EXT IRQ39 Reserved */
    EXT_IRQ40_Handler, /*     EXT IRQ40 Reserved */
    EXT_IRQ41_Handler, /*     EXT IRQ41 Reserved */
    EXT_IRQ42_Handler, /*     EXT IRQ42 Reserved */
    EXT_IRQ43_Handler, /*     EXT IRQ43 Reserved */
    EXT_IRQ44_Handler, /*     EXT IRQ44 Reserved */
    EXT_IRQ45_Handler, /*     EXT IRQ45 Reserved */
    EXT_IRQ46_Handler, /*     EXT IRQ46 Reserved */
    EXT_IRQ47_Handler, /*     EXT IRQ47 Reserved */
    EXT_IRQ48_Handler, /*     EXT IRQ48 Reserved */
    EXT_IRQ49_Handler, /*     EXT IRQ49 Reserved */
    EXT_IRQ50_Handler, /*     EXT IRQ50 Reserved */
    EXT_IRQ51_Handler, /*     EXT IRQ51 Reserved */
    EXT_IRQ52_Handler, /*     EXT IRQ52 Reserved */
    EXT_IRQ53_Handler, /*     EXT IRQ53 Reserved */
    EXT_IRQ54_Handler, /*     EXT IRQ54 Reserved */
    EXT_IRQ55_Handler, /*     EXT IRQ55 Reserved */
    EXT_IRQ56_Handler, /*     EXT IRQ56 Reserved */
    EXT_IRQ57_Handler, /*     EXT IRQ57 Reserved */
    EXT_IRQ58_Handler, /*     EXT IRQ58 Reserved */
    EXT_IRQ59_Handler, /*     EXT IRQ59 Reserved */
    EXT_IRQ60_Handler, /*     EXT IRQ60 Reserved */
    EXT_IRQ61_Handler, /*     EXT IRQ61 Reserved */
    EXT_IRQ62_Handler, /*     EXT IRQ62 Reserved */
    EXT_IRQ63_Handler, /*     EXT IRQ63 Reserved */

    /* Interrupts of Peripheral 80~207*/
    // uart0
    UART0_RX_IRQ_Handler,   /*     EXT IRQ64 Reserved */
    UART0_TX_IRQ_Handler,   /*     EXT IRQ65 Reserved */
    UART0_POLL_IRQ_Handler, /*     EXT IRQ66 Reserved */
    UART0_EOT_IRQ_Handler,  /*     EXT IRQ67 Reserved */
    // uart1
    EXT_IRQ68_Handler, /*     EXT IRQ68 Reserved */
    EXT_IRQ69_Handler, /*     EXT IRQ69 Reserved */
    EXT_IRQ70_Handler, /*     EXT IRQ70 Reserved */
    EXT_IRQ71_Handler, /*     EXT IRQ71 Reserved */
    // uart2
    EXT_IRQ72_Handler, /*     EXT IRQ72 Reserved */
    EXT_IRQ73_Handler, /*     EXT IRQ73 Reserved */
    EXT_IRQ74_Handler, /*     EXT IRQ74 Reserved */
    EXT_IRQ75_Handler, /*     EXT IRQ75 Reserved */
    // uart3
    EXT_IRQ76_Handler, /*     EXT IRQ76 Reserved */
    EXT_IRQ77_Handler, /*     EXT IRQ77 Reserved */
    EXT_IRQ78_Handler, /*     EXT IRQ78 Reserved */
    EXT_IRQ79_Handler, /*     EXT IRQ79 Reserved */
    // spim0
    EXT_IRQ80_Handler, /*     EXT IRQ80 Reserved */
    EXT_IRQ81_Handler, /*     EXT IRQ81 Reserved */
    EXT_IRQ82_Handler, /*     EXT IRQ82 Reserved */
    EXT_IRQ83_Handler, /*     EXT IRQ83 Reserved */
                       // spim1
    EXT_IRQ84_Handler, /*     EXT IRQ84 Reserved */
    EXT_IRQ85_Handler, /*     EXT IRQ85 Reserved */
    EXT_IRQ86_Handler, /*     EXT IRQ86 Reserved */
    EXT_IRQ87_Handler, /*     EXT IRQ87 Reserved */
    // spim2
    EXT_IRQ88_Handler, /*     EXT IRQ88 Reserved */
    EXT_IRQ89_Handler, /*     EXT IRQ89 Reserved */
    EXT_IRQ90_Handler, /*     EXT IRQ90 Reserved */
    EXT_IRQ91_Handler, /*     EXT IRQ91 Reserved */
                       // spim3
    EXT_IRQ92_Handler, /*     EXT IRQ92 Reserved */
    EXT_IRQ93_Handler, /*     EXT IRQ93 Reserved */
    EXT_IRQ94_Handler, /*     EXT IRQ94 Reserved */
    EXT_IRQ95_Handler, /*     EXT IRQ95 Reserved */
    // i2c0
    EXT_IRQ96_Handler, /*     EXT IRQ96 Reserved */
    EXT_IRQ97_Handler, /*     EXT IRQ97 Reserved */
    EXT_IRQ98_Handler, /*     EXT IRQ98 Reserved */
    EXT_IRQ99_Handler, /*     EXT IRQ99 Reserved */
    // i2c1
    EXT_IRQ100_Handler, /*     EXT IRQ100 Reserved */
    EXT_IRQ101_Handler, /*     EXT IRQ101 Reserved */
    EXT_IRQ102_Handler, /*     EXT IRQ102 Reserved */
    EXT_IRQ103_Handler, /*     EXT IRQ103 Reserved */
    // i2c2
    EXT_IRQ104_Handler, /*     EXT IRQ104 Reserved */
    EXT_IRQ105_Handler, /*     EXT IRQ105 Reserved */
    EXT_IRQ106_Handler, /*     EXT IRQ106 Reserved */
    EXT_IRQ107_Handler, /*     EXT IRQ107 Reserved */
    // i2c3
    EXT_IRQ108_Handler, /*     EXT IRQ108 Reserved */
    EXT_IRQ109_Handler, /*     EXT IRQ109 Reserved */
    EXT_IRQ110_Handler, /*     EXT IRQ110 Reserved */
    EXT_IRQ111_Handler, /*     EXT IRQ111 Reserved */
    // sdio
    EXT_IRQ112_Handler, /*     EXT IRQ112 Reserved */
    EXT_IRQ113_Handler, /*     EXT IRQ113 Reserved */
    EXT_IRQ114_Handler, /*     EXT IRQ114 Reserved */
    EXT_IRQ115_Handler, /*     EXT IRQ115 Reserved */
    // i2s
    EXT_IRQ116_Handler, /*     EXT IRQ116 Reserved */
    EXT_IRQ117_Handler, /*     EXT IRQ117 Reserved */
    EXT_IRQ118_Handler, /*     EXT IRQ118 Reserved */
    EXT_IRQ119_Handler, /*     EXT IRQ119 Reserved */

    // camif
    EXT_IRQ120_Handler, /*     EXT IRQ120 Reserved */

    // ADC
    EXT_IRQ121_Handler, /*     EXT IRQ121 Reserved */
    EXT_IRQ122_Handler, /*     EXT IRQ122 Reserved */
    EXT_IRQ123_Handler, /*     EXT IRQ123 Reserved */

    // filter
    EXT_IRQ124_Handler, /*     EXT IRQ124 Reserved */
    EXT_IRQ125_Handler, /*     EXT IRQ125 Reserved */
    EXT_IRQ126_Handler, /*     EXT IRQ126 Reserved */
    EXT_IRQ127_Handler, /*     EXT IRQ127 Reserved */

    // scif
    EXT_IRQ128_Handler, /*     EXT IRQ128 Reserved */
    EXT_IRQ129_Handler, /*     EXT IRQ129 Reserved */
    EXT_IRQ130_Handler, /*     EXT IRQ130 Reserved */
    EXT_IRQ131_Handler, /*     EXT IRQ131 Reserved */

    // spis0
    EXT_IRQ132_Handler, /*     EXT IRQ132 Reserved */
    EXT_IRQ133_Handler, /*     EXT IRQ133 Reserved */
    EXT_IRQ134_Handler, /*     EXT IRQ134 Reserved */
    EXT_IRQ135_Handler, /*     EXT IRQ135 Reserved */

    // spis1
    EXT_IRQ136_Handler, /*     EXT IRQ136 Reserved */
    EXT_IRQ137_Handler, /*     EXT IRQ137 Reserved */
    EXT_IRQ138_Handler, /*     EXT IRQ138 Reserved */
    EXT_IRQ139_Handler, /*     EXT IRQ139 Reserved */

    // PWM
    EXT_IRQ140_Handler, /*     EXT IRQ140 Reserved */
    EXT_IRQ141_Handler, /*     EXT IRQ141 Reserved */
    EXT_IRQ142_Handler, /*     EXT IRQ142 Reserved */
    EXT_IRQ143_Handler, /*     EXT IRQ143 Reserved */

    // gpio
    EXT_IRQ144_Handler, /*     EXT IRQ144 Reserved */

    // usb
    EXT_IRQ145_Handler, /*     EXT IRQ145 Reserved */

    // sdc
    EXT_IRQ146_Handler, /*     EXT IRQ146 Reserved */

    // pio
    EXT_IRQ147_Handler, /*     EXT IRQ147 Reserved */
    EXT_IRQ148_Handler, /*     EXT IRQ148 Reserved */
    EXT_IRQ149_Handler, /*     EXT IRQ149 Reserved */
    EXT_IRQ150_Handler, /*     EXT IRQ150 Reserved */
    EXT_IRQ151_Handler, /*     EXT IRQ151 Reserved */
    EXT_IRQ152_Handler, /*     EXT IRQ152 Reserved */
    EXT_IRQ153_Handler, /*     EXT IRQ153 Reserved */
    EXT_IRQ154_Handler, /*     EXT IRQ154 Reserved */
    EXT_IRQ155_Handler, /*     EXT IRQ155 Reserved */
    EXT_IRQ156_Handler, /*     EXT IRQ156 Reserved */
    EXT_IRQ157_Handler, /*     EXT IRQ157 Reserved */
    EXT_IRQ158_Handler, /*     EXT IRQ158 Reserved */
    EXT_IRQ159_Handler, /*     EXT IRQ159 Reserved */
    EXT_IRQ160_Handler, /*     EXT IRQ160 Reserved */
    EXT_IRQ161_Handler, /*     EXT IRQ161 Reserved */
    EXT_IRQ162_Handler, /*     EXT IRQ162 Reserved */
    EXT_IRQ163_Handler, /*     EXT IRQ163 Reserved */
    EXT_IRQ164_Handler, /*     EXT IRQ164 Reserved */
    EXT_IRQ165_Handler, /*     EXT IRQ165 Reserved */
    EXT_IRQ166_Handler, /*     EXT IRQ166 Reserved */
    EXT_IRQ167_Handler, /*     EXT IRQ167 Reserved */
    EXT_IRQ168_Handler, /*     EXT IRQ168 Reserved */
    EXT_IRQ169_Handler, /*     EXT IRQ169 Reserved */
    EXT_IRQ170_Handler, /*     EXT IRQ170 Reserved */
    EXT_IRQ171_Handler, /*     EXT IRQ171 Reserved */
    EXT_IRQ172_Handler, /*     EXT IRQ172 Reserved */
    EXT_IRQ173_Handler, /*     EXT IRQ173 Reserved */
    EXT_IRQ174_Handler, /*     EXT IRQ174 Reserved */
    EXT_IRQ175_Handler, /*     EXT IRQ175 Reserved */
    EXT_IRQ176_Handler, /*     EXT IRQ176 Reserved */
    EXT_IRQ177_Handler, /*     EXT IRQ177 Reserved */
    EXT_IRQ178_Handler, /*     EXT IRQ178 Reserved */
    EXT_IRQ179_Handler, /*     EXT IRQ179 Reserved */
    EXT_IRQ180_Handler, /*     EXT IRQ180 Reserved */
    EXT_IRQ181_Handler, /*     EXT IRQ181 Reserved */
    EXT_IRQ182_Handler, /*     EXT IRQ182 Reserved */
    EXT_IRQ183_Handler, /*     EXT IRQ183 Reserved */

    // i2c0.nack
    EXT_IRQ184_Handler, /*     EXT IRQ184 Reserved */

    // i2c1.nack
    EXT_IRQ185_Handler, /*     EXT IRQ185 Reserved */

    // i2c2.nack
    EXT_IRQ186_Handler, /*     EXT IRQ186 Reserved */

    // i2c3.nack
    EXT_IRQ187_Handler, /*     EXT IRQ187 Reserved */

    // i2c0.err
    EXT_IRQ188_Handler, /*     EXT IRQ188 Reserved */

    // i2c1.err
    EXT_IRQ189_Handler, /*     EXT IRQ189 Reserved */

    // i2c2.err
    EXT_IRQ190_Handler, /*     EXT IRQ190 Reserved */

    // i2c3.err
    EXT_IRQ191_Handler, /*     EXT IRQ191 Reserved */

    /* Interrupts of Error 208~239*/
    EXT_IRQ192_Handler, /*     EXT IRQ192 Reserved */
    EXT_IRQ193_Handler, /*     EXT IRQ193 Reserved */
    EXT_IRQ194_Handler, /*     EXT IRQ194 Reserved */
    EXT_IRQ195_Handler, /*     EXT IRQ195 Reserved */
    EXT_IRQ196_Handler, /*     EXT IRQ196 Reserved */
    EXT_IRQ197_Handler, /*     EXT IRQ197 Reserved */
    EXT_IRQ198_Handler, /*     EXT IRQ198 Reserved */
    EXT_IRQ199_Handler, /*     EXT IRQ199 Reserved */
    EXT_IRQ200_Handler, /*     EXT IRQ200 Reserved */
    EXT_IRQ201_Handler, /*     EXT IRQ201 Reserved */
    EXT_IRQ202_Handler, /*     EXT IRQ202 Reserved */
    EXT_IRQ203_Handler, /*     EXT IRQ203 Reserved */
    EXT_IRQ204_Handler, /*     EXT IRQ204 Reserved */
    EXT_IRQ205_Handler, /*     EXT IRQ205 Reserved */
    EXT_IRQ206_Handler, /*     EXT IRQ206 Reserved */
    EXT_IRQ207_Handler, /*     EXT IRQ207 Reserved */
    EXT_IRQ208_Handler, /*     EXT IRQ208 Reserved */
    EXT_IRQ209_Handler, /*     EXT IRQ209 Reserved */
    EXT_IRQ210_Handler, /*     EXT IRQ210 Reserved */
    EXT_IRQ211_Handler, /*     EXT IRQ211 Reserved */
    EXT_IRQ212_Handler, /*     EXT IRQ212 Reserved */
    EXT_IRQ213_Handler, /*     EXT IRQ213 Reserved */
    EXT_IRQ214_Handler, /*     EXT IRQ214 Reserved */
    EXT_IRQ215_Handler, /*     EXT IRQ215 Reserved */
    EXT_IRQ216_Handler, /*     EXT IRQ216 Reserved */
    EXT_IRQ217_Handler, /*     EXT IRQ217 Reserved */
    EXT_IRQ218_Handler, /*     EXT IRQ218 Reserved */
    EXT_IRQ219_Handler, /*     EXT IRQ219 Reserved */
    EXT_IRQ220_Handler, /*     EXT IRQ220 Reserved */
    EXT_IRQ221_Handler, /*     EXT IRQ221 Reserved */
    EXT_IRQ222_Handler, /*     EXT IRQ222 Reserved */
    EXT_IRQ223_Handler  /*     EXT IRQ223 Reserved */
};

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler(void)
{
    SystemInit();      /* CMSIS System Initialization */
    __PROGRAM_START(); /* Enter PreMain (C library entry point) */
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

/*----------------------------------------------------------------------------
  Hard Fault Handler
 *----------------------------------------------------------------------------*/

void HardFault_Handler(void)
{
    while (1)
        ;
}

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void Default_Handler(void)
{
    while (1)
        ;
}

void SysTick_Handler(void)
{
    HAL_SysTick_Handler();
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif
