/*
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

#include <stdint.h>
#include <stddef.h>

#ifndef __DARIC_UTIL_H__
#define __DARIC_UTIL_H__

#define DARIC_SYSCTRL_CGU_BASE         0x40040000UL

typedef struct {
    volatile uint32_t ar    ;  // 0x0090  0x00000000  CR  yes ipc_ar
    volatile uint32_t en    ;  // 0x0094  0x00000000  CR  yes ipc_en
    volatile uint32_t lpen  ;  // 0x0098  0x00000000  CR  yes ipc_lpen
    volatile uint32_t osc   ;  // 0x009c  0x00000000  CR  yes ipc_osc
    volatile uint32_t pll_mn;  // 0x00a0  0x00000000  CR  yes ipc_pll_mn for FPGA, [15:8] m, [7:0] n
                      //                                        for ASIC, 11:0
    volatile uint32_t pll_f ;  // 0x00a4  0x00000000  CR  yes ipc_pll_f, 24-bit
    volatile uint32_t pll_q ;  // 0x00a8  0x00000000  CR  yes ipc_pll_q, 16-bit
    volatile uint32_t ipc;  // 0x00ac  0x00000000  CR  yes IP control, CR(TBD)
} DARIC_SYSCTRL_IPC_T;

#define DARIC_SYSCTRL_IPC_BASE         0x40040090UL

typedef struct {
    volatile uint32_t cache;        // 00
    volatile uint32_t itcm;         // 04
    volatile uint32_t dtcm;         // 08
    volatile uint32_t sram0;        // 0c
    volatile uint32_t sram1;        // 10
    volatile uint32_t vexram;       // 14
    volatile uint32_t DUMMY08[2];
    volatile uint32_t srambankerr;  // 20
} DARIC_CORE_SRAMCFG_T;

#define DARIC_CORE_SRAMCFG_BASE         0x40014000UL


#define DARIC_RCU_RST_CHIP 0x40040080
#define DARIC_RCU_RST_CORE 0x40040084
#define DARIC_RCU_SRC_FLAG 0x40040088



void HardFault_Handler(void);
uint16_t peekU16(size_t addr);
void snapshotTime(void);
void simDone(void);
uint16_t writeU16(size_t addr, uint16_t value);

typedef enum
{
  HAL_TICK_FREQ_10HZ         = 100U,
  HAL_TICK_FREQ_100HZ        = 10U,
  HAL_TICK_FREQ_1KHZ         = 1U,
  HAL_TICK_FREQ_DEFAULT      = HAL_TICK_FREQ_1KHZ
} HAL_TickFreqTypeDef;

void HAL_IncTick(void);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t delay);
void delay_ms(uint32_t nms);

#if 0
static inline void printString(const char *s) {
    char c;
    size_t i = 0;
    while ((c = s[i++]) != 0) {
        CHAR_DEVICE = c;
    }
    CHAR_DEVICE = '\n';
}
#endif

typedef struct {
    volatile uint8_t TX;
    volatile uint8_t DUMMY1[3];
    volatile uint8_t EN;
    volatile uint8_t DUMMY5[3];
    volatile uint8_t BUSY;
    volatile uint8_t DUMMY9[3];
    volatile uint8_t ETU;
    volatile uint8_t DUMMYD[3];
} DUART_T;

typedef struct{
    volatile uint32_t CFG_CG;
    volatile uint32_t CFG_EVENT;
    volatile uint32_t CFG_RST;
} UDMACORE_T;

extern volatile UDMACORE_T * const UDMACORE;
extern volatile DUART_T * const DUART;

#endif // __DARIC_UTIL_H__
