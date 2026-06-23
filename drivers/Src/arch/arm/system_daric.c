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

#include "daric.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define XTAL (48000000UL) /* External XTAL frequency */
#define OSC  (32000000UL) /* Internal Oscillator frequency */

#define SYSTEM_CLOCK (OSC)

#define DARIC_FCLK_FREQ_MAX  (800)
#define DARIC_ACLK_FREQ_MAX  (400)
#define DARIC_HCLK_FREQ_MAX  (200)
#define DARIC_ICLK_FREQ_MAX  (100)
#define DARIC_PCLK_FREQ_MAX  (50)
#define DARIC_AOCLK_FREQ_MAX (2)

/*---------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *---------------------------------------------------------------------------*/
extern const VECTOR_TABLE_Type __VECTOR_TABLE[16 + TOTAL_IRQ_NUMS];

/*---------------------------------------------------------------------------
  System Core Clock Variable
 *---------------------------------------------------------------------------*/

/* System Clock Frequency (Core Clock)*/
uint32_t SystemCoreClock = SYSTEM_CLOCK;

/*---------------------------------------------------------------------------
  System Core Clock function
 *---------------------------------------------------------------------------*/
static inline uint32_t log_2(const uint32_t x)
{
    return (x == 0) ? 0 : (31 - __builtin_clz(x));
}

uint16_t calcPllDiv(uint32_t fVCOkHz, uint32_t fTargetkHz)
{
    uint16_t div = 0x0077;
    // rounded
    int32_t targetDiv = (fVCOkHz + fTargetkHz / 2) / fTargetkHz;
    int32_t tries[8]  = { 8, 4, 2, 6, 7, 5, 3, 1 }; // prefer 2^n div for final stage
    int32_t bestErr   = 64;
    int32_t bestA     = 8;
    int32_t bestB     = 8;

    for (uint16_t i = 0; i < 8; ++i)
    {
        int32_t b   = tries[i];
        int32_t a   = (targetDiv + b / 2) / b;
        int32_t err = a * b - targetDiv;
        if (abs(err) < abs(bestErr) && (a <= 8))
        {
            bestA   = a;
            bestB   = b;
            bestErr = err;
            if (0 == err)
            {
                break;
            }
        }
    }
    div = ((bestA - 1) << 4) | (bestB - 1);
    return div;
}

uint32_t SystemClock1 = 0;
#define KHZ 1000UL
#define MHZ 1000000UL
#define GHZ 1000000000UL

#define CGU_SET_COMMIT 0x32
#define IPC_AR_COMMIT  0x32
#define IPC_AR_IPFLOW  0x57

#define F_SENSOR_FACTOR 10

#define SEL_CLKTOP_PLL 1
#define SEL_CLKTOP_SYS 0

#define SEL_CLKSYS_XTAL 1
#define SEL_CLKSYS_IOSC 0

#define SEL_CLKTOP_PLL 1
#define SEL_CLKTOP_SYS 0
#define FREQ_XTAL_MHZ  48
#define FREQ_IOSC_MHZ  32
typedef struct
{
    uint32_t M;
    uint32_t N;
    uint32_t NF;
    uint32_t Q0;
    uint32_t Q1;
    uint32_t MHz; // nominal MHz, for easy calculation
} PLLConfig_type;

static const uint32_t PLL_FRAC_BITS = 24;

static const PLLConfig_type SIMPLE_PLL_SETTINGS[] = {
    { 1, 32, 0x00000000, 0x37, 0x37, 48 },   //  0: VCO 1536, output   48 /  48 MHz
    { 1, 25, 0x00000000, 0x23, 0x23, 100 },  //  1: VCO 1200, output  100 / 100 MHz
    { 1, 25, 0x00000000, 0x21, 0x21, 200 },  //  2: VCO 1200, output  200 / 200 MHz
    { 1, 25, 0x00000000, 0x11, 0x11, 300 },  //  3: VCO 1200, output  300 / 300 MHz
    { 1, 50, 0x00000000, 0x21, 0x13, 400 },  //  4: VCO 2400, output  400 / 300 MHz
    { 4, 125, 0x00000000, 0x02, 0x04, 500 }, //  5: VCO 1500, output  500 / 300 MHz
    { 1, 25, 0x00000000, 0x01, 0x11, 600 },  //  6: VCO 2400, output  600 / 300 MHz
    { 4, 175, 0x00000000, 0x02, 0x06, 700 }, //  7: VCO 2100, output  700 / 300 MHz
    { 1, 50, 0x00000000, 0x02, 0x13, 800 },  //  8: VCO 2400, output  800 / 300 MHz
    { 2, 75, 0x00000000, 0x01, 0x04, 900 },  //  9: VCO 1800, output  900 / 300 MHz
    { 2, 75, 0x00000000, 0x01, 0x04, 1000 }, // 10: VCO 1800, output 1000 / 300 MHz
    { 1, 50, 0x00000000, 0x02, 0x31, 800 },  // 11: VCO 2400, output  800 / 400 MHz
    { 1, 50, 0x00000000, 0x02, 0x04, 800 },  // 11: VCO 2400, output  800 / 480 MHz
};

/**
    @retval frequency divider factor, fixed-point with 8 fraction bits,
            e.g. 0x100 means 1.000, 0x051e means 5.117188
*/
uint32_t fdreg2divfp8(uint32_t fdreg)
{
    uint32_t ndiv = 1; // don't init to 0
    // normalize to 1<<16,
    uint32_t density = ((fdreg & 0xff) + 1);
    uint32_t ddiv    = (((fdreg >> 16) & 0xff) + 1) << 8;
    if (ddiv > (1 << 16) / density)
    {
        ndiv = ddiv;
    }
    else
    {
        ndiv = (1 << 16) / density;
    }
    // printf("density %08x, ddiv %08x, ndiv %04" PRIx32 "\n", density, ddiv, ndiv);
    return ndiv;
}

void setFDsAuto(uint32_t clkTopHz)
{
    uint32_t              idxFdA         = 2;
    static const uint32_t PREFERED_FDS[] = {
        0x07001fff, 0x07001fff, 0x0f010f7f, 0x1f03073f, 0x3f07031f, 0x7f0f010f, 0x7f0f0107,
    };
    static const uint32_t PREFERED_FD_PERS[] = {
        0x07001fff, // up to    0 MHz
        0x07001fff, // up to  100 MHz
        0x07001fff, // up to  200 MHz
        0x07011f7f, //        300
        0x07011f7f, //        400
        0x07021f7f, //        500
        0x07021f7f, //        600
        0x07031f7f, //        700
        0x07031f7f, //        800
        0x09041f3f, //        900
        0x09041f3f, // up to 1000 MHz
        0x0b051f3f, // up to 1100 MHz
        0x0b051f3f, // up to 1200 MHz
    };

    // ceilling
    uint32_t clkTopCeil100MHz = (clkTopHz / MHZ + 99) / 100;
    if (clkTopCeil100MHz > 13)
    {
        clkTopCeil100MHz = 13;
    }
    switch (clkTopCeil100MHz)
    {
        case 0:
        case 1:
            idxFdA = 0;
            break;
        case 2:
        case 3:
            // case 4:
            idxFdA = 1;
            break;
        default:
            idxFdA = 2;
            break;
    }
    // uint32_t divao = (clkTopHz + MHZ - 1) / MHZ / 2 - 1;
    uint32_t divao = clkTopHz / MHZ / 2;
    if (0 != divao)
    { // possible?
        divao--;
    }
    divao = divao > 0xff ? 0xff : divao;   // saturate
    divao = (divao * 0x01010000) | 0xffff; //

    DARIC_CGU->fdfclk  = PREFERED_FDS[0]; // for CPU core, always use full throttle, 8x for LP
    DARIC_CGU->fdaclk  = PREFERED_FDS[idxFdA];
    DARIC_CGU->fdhclk  = PREFERED_FDS[idxFdA + 1];
    DARIC_CGU->fdiclk  = PREFERED_FDS[idxFdA + 2];
    DARIC_CGU->fdpclk  = PREFERED_FDS[idxFdA + 3];
    DARIC_CGU->fdaoclk = divao;
    DARIC_CGU->fdaoram = (divao >> 8) & 0xffff;

    DARIC_CGU->fdper = PREFERED_FD_PERS[clkTopCeil100MHz];
    __DMB();
    DARIC_CGU->cguset = CGU_SET_COMMIT; // commit
    __DSB();
}

// TODO: clktop protection
void setPLL(const PLLConfig_type *pllcfg)
{
    // PD PLL
    DARIC_IPC->lpen |= 0x02; // PLL shutdown
    __DMB();
    DARIC_IPC->ar = 0x0032; // commit
    __DSB();

#if 0
    double nnf = (double)pllcfg->N + (double)(pllcfg->NF & 0x00ffffff) / (1UL << 24);
    printf("%s(): setup PLL using\n    M = %2" PRIu32" \n", __func__, pllcfg->M);
    printf("    N = %2" PRIu32" \n", pllcfg->N);
    printf("    NF = %06" PRIx32" \n", (pllcfg->NF & 0x00ffffff) );
    printf("    N.NF = %13.7f\n", nnf);
    printf("    Q0 = %02" PRIx32" \n", pllcfg->Q0);
    printf("    Q1 = %02" PRIx32" \n", pllcfg->Q1);
    printf("    f_VCO %8.3f MHz\n", 48.0 / pllcfg->M * nnf);
#endif

#if 0
    // PLL PD delay 
    for (uint32_t i = 0; i < 1024; i++){
        __NOP();
    }
#endif

#if 1
    DARIC_IPC->pll_mn = ((pllcfg->M << 12) & 0x0001F000) | (pllcfg->N & 0x00000fff);
    DARIC_IPC->pll_f  = pllcfg->NF;                              // if use fraction n
    DARIC_IPC->pll_q  = (pllcfg->Q0 & 0x77) | (pllcfg->Q1 << 8); // calculated DIV for VCO freq and target clk1
#else
    // 600-300
    DARIC_IPC->pll_mn = 0x04096;
    DARIC_IPC->pll_f  = 0x1000000;
    DARIC_IPC->pll_q  = 0x0502;
#endif
    __DMB();
    DARIC_IPC->ar = 0x0032; // commit
    __DSB();

#ifdef IPC_BIAS_DEBUG
    //               VCO bias   CPP bias   CPI bias
    //                1          2          3
    DARIC_IPC->ipc = (1 << 6) | (2 << 3) | (3);
    // DARIC_IPC->ipc = (3 << 6) | (5 << 3) | (5);
#endif

    DARIC_IPC->lpen &= ~0x02; // enable PLL
    __DMB();
    DARIC_IPC->ar = 0x0032; // commit
    __DSB();

    // wait/poll lock status?
    for (uint32_t i = 0; i < 1024; i++)
    {
        __NOP();
    }
    // printf("%s(): Finished \n", __func__);
}

uint32_t clkGetClkSysHz()
{
    // static const uint32_t CLKSYS_F_MHZ[2] = {32, 48};

    uint32_t fClkSys = FREQ_XTAL_MHZ * MHZ; // default , xtal
    if (SEL_CLKSYS_IOSC == DARIC_CGU->cgusel1)
    {
        // APPEARED xtal freq
        uint32_t fxtal = (DARIC_CGU->cgufsfreq2 >> 0) & 0xffff;
        //        already * F_SENSOR_FACTOR
        fClkSys = DARIC_CGU->cgufscr * FREQ_XTAL_MHZ / fxtal * MHZ;
    }
    return fClkSys;
}

void clkGetClkPLLHz(uint32_t *f0, uint32_t *f1)
{
    uint32_t t0 = 0, t1 = 0;
    double   f0Hz = 0, f1Hz = 0, fvco = 0;

    uint32_t fClkSysHz = clkGetClkSysHz();

    t0               = DARIC_IPC->pll_mn;
    t1               = DARIC_IPC->pll_f;
    uint32_t M       = (t0 >> 12) & 0x1F;
    uint64_t N       = t0 & 0xFFF;
    double   N_Float = (N << 24) | ((0 != (t1 & 0x01000000)) ? (t1 & 0x00ffffff) : 0);
    N_Float /= (double)0x01000000; // fixed magic number, 24-bit frac
    t1 = DARIC_IPC->pll_q;
    if (SEL_CLKTOP_PLL == DARIC_CGU->cgusel0)
    {
        fvco = fClkSysHz / M * N_Float;
    }

    // printf("    Hz %8" PRIu32 ",\n", fClkSysHz);
    // printf("    M = %2" PRIu32 ", F_PFD = %8" PRIu32 "Hz.\n", M, fClkSysHz / M);
    // printf("    N = %13.8f, F_VCO = %12.6f Hz.\n", N_Float, fvco);

    if (NULL != f0)
    {
        if (SEL_CLKTOP_SYS == DARIC_CGU->cgusel0)
        {
            f0Hz = (double)fClkSysHz;
        }
        else
        {
            uint32_t q1 = (t1 >> 4) & 0x07;
            uint32_t q0 = t1 & 0x07;
            f0Hz        = fvco / ((q1 + 1) * (q0 + 1));
        }
        *f0 = f0Hz;
    }

    if (NULL != f1)
    {
        if (SEL_CLKTOP_SYS == DARIC_CGU->cgusel0)
        {
            f1Hz = (double)fClkSysHz;
        }
        else
        {
            uint32_t q1 = (t1 >> 12) & 0x07;
            uint32_t q0 = (t1 >> 8) & 0x07;
            f1Hz        = fvco / ((q1 + 1) * (q0 + 1));
        }
        *f1 = f1Hz;
    }
}

void recalculateSystemClocks(void)
{
    clkGetClkPLLHz((uint32_t *)&SystemCoreClock, (uint32_t *)&SystemClock1);
}

/*!
 * Select ClkSys between Internal OSC 32MHz  and external XTAL 48MHz
 *
 * @param 0: select Internal OSC 32MHZ
 *        1: select external XTAL 48MHz
 **/
void selClkSys(uint32_t clkSrc)
{
    // only when PLL not in use
    if (SEL_CLKTOP_SYS == DARIC_CGU->cgusel0)
    {
        uint32_t save = DARIC_CGU->cgusel1;
        switch (clkSrc)
        {
            case SEL_CLKSYS_XTAL:
                sys_nopDelayMs(10);
                DARIC_CGU->cgufscr = FREQ_XTAL_MHZ * F_SENSOR_FACTOR; // external crystal is 48MHz
                SystemCoreClock    = FREQ_XTAL_MHZ * MHZ;
                break;
            case SEL_CLKSYS_IOSC: {
                uint32_t fosc  = (DARIC_CGU->cgufsfreq2 >> 16) & 0xffff;
                uint32_t fxtal = (DARIC_CGU->cgufsfreq2 >> 0) & 0xffff;
                // will not work well
                if (SEL_CLKSYS_XTAL == save)
                {
                    // use measured fosc
                    DARIC_CGU->cgufscr = fosc;
                    SystemCoreClock    = FREQ_IOSC_MHZ * MHZ / F_SENSOR_FACTOR;
                }
                else
                {
                    // calculate using better clk source
                    SystemCoreClock    = DARIC_CGU->cgufscr * FREQ_XTAL_MHZ * F_SENSOR_FACTOR / fxtal;
                    DARIC_CGU->cgufscr = SystemCoreClock * F_SENSOR_FACTOR / MHZ;
                }
                break;
            }
            default:
                break;
        }
        DARIC_CGU->cgusel1 = clkSrc;          // 0: RC, 1: XTAL
        SystemClock1       = SystemCoreClock; // when switching, PLL should be bypassed
        __DMB();
        DARIC_CGU->cguset = CGU_SET_COMMIT; // commit
        __DSB();
        // printf("Set ClkSys to %" PRIu32 " MHz\n", SystemCoreClock / 1000 / 1000);
    }
}

/*!
 * Select ClkTop between ClkSys and PLL0/1
 *
 * @param 0: select ClkSys for both ClkTop and ClkPke
 *        1: select PLL0 for ClkTop, PLL1 for ClkPke
 **/
uint32_t selClkTop(uint32_t clkSrc)
{
    uint32_t old = DARIC_CGU->cgusel0;
    // printf("%s(): Select ClkTop %" PRIu32 "\n", __func__, clkSrc);

    // printf("Check PLL settings: %08" PRIx32 ",%08" PRIx32 ",%08" PRIx32 "\n", DARIC_IPC->pll_mn, DARIC_IPC->pll_f, DARIC_IPC->pll_q);
    DARIC_CGU->cgusel0 = clkSrc; // clktop sel, 0:clksys, 1:clkpll0
    __DMB();
    DARIC_CGU->cguset = CGU_SET_COMMIT; // commit
    __DSB();
    // Need this?
    __ISB();
    // printf("%s() finished\n", __func__);

    recalculateSystemClocks();
    return old;
}

/*!
 * Simple PLL setup
 *
 * @param freq100MHz
 *
 * 0是 48MHz,
 *
 **/
int32_t initClockSimple(uint32_t freq100MHz)
{
    int32_t r = 1;

    if (freq100MHz > sizeof(SIMPLE_PLL_SETTINGS) / sizeof(SIMPLE_PLL_SETTINGS[0]))
    {
        r = 1; // error
    }
    else
    {
        bool setRAMdivFirst = (SIMPLE_PLL_SETTINGS[freq100MHz].MHz > SystemCoreClock);

        selClkTop(SEL_CLKTOP_SYS);

        if (setRAMdivFirst)
        {
            setFDsAuto(SIMPLE_PLL_SETTINGS[freq100MHz].MHz * MHZ);
        }

        setPLL(&SIMPLE_PLL_SETTINGS[freq100MHz]);

        if (!setRAMdivFirst)
        {
            setFDsAuto(SIMPLE_PLL_SETTINGS[freq100MHz].MHz * MHZ);
        }
        selClkTop(SEL_CLKTOP_PLL);
        r = 0;
    }

    return r;
}

/**
 *
 * @param freqHz: 0: use internal OSC
 *                1: use external Xtal but not PLL
 *                > 16_000_000 (16MHz): target PLL clk0 frequency in Hz,  use external Xtal and PLL
 * @param targetClk1Hz: target PLL clk1 frequency in Hz, result might not be exactly value PKE CLK
 *
 */
void initClockASIC(uint32_t freqHz, uint32_t targetClk1Hz, bool setVoltage)
{
    static const uint32_t PFD_F_MHZ = 16; // TODO
    static const uint32_t FREQ_0    = 16UL * MHZ;
    static const uint16_t TBL_Q[]   = {
        // keep later DIV even number as possible
        // change 20250807, only for clk0 Q
        0x0077, // 16-32 MHz
        0x0037, // 32-64
        0x0033, // 64-128
        0x0013, // 128-256
        0x0011, // 256-512 // keep ~ 100MHz
        0x0001, // 512-1024
        0x0001, // 1024-1600
    };
    static const uint32_t TBL_MUL[] = {
        // change 20250807, only for clk0 Q
        64, // 16-32 MHz
        32, // 32-64
        16, // 64-128
        8,  // 128-256
        4,  // 256-512
        2,  // 512-1024
        2,  // 1024-1600
    };
    uint32_t actualHz = 0;

    if (targetClk1Hz == 0)
    {
        targetClk1Hz = 200 * MHZ; // default
    }
    else if (targetClk1Hz > 300 * MHZ)
    {
        targetClk1Hz = 300 * MHZ; // limit
    }

    if (freqHz < targetClk1Hz)
    {
        targetClk1Hz = freqHz; // Do not go higher than clk0
    }

    selClkTop(SEL_CLKTOP_SYS); // switch to ClkSys

    if (0 != freqHz)
    {
        // TODO check XTAL status ?
        selClkSys(SEL_CLKSYS_XTAL); // Switch to XTAL
        if (1 == freqHz)
        { // special value, 1 set ClkTop to ClkSys, skip PLL
            actualHz = FREQ_XTAL_MHZ * MHZ;
        }
        else
        {
            if (freqHz < 15625000)
            {
                actualHz = 15625000;
            }
            else
            {
                actualHz = freqHz;
            }
        }
    }
    else
    {
        selClkSys(SEL_CLKSYS_IOSC);     // Switch to XTAL
        actualHz = FREQ_IOSC_MHZ * MHZ; // TODO measured valur
    }
    bool setRAMdivFirst = actualHz > SystemCoreClock;
    if (setRAMdivFirst)
    {
        setFDsAuto(actualHz);
    }
    if (freqHz < 15625000)
    { // absolute limit
        // lower than limit 1000MHz / 64, impossible to set, use ClkSys
        // do not configure or switch to PLL
        if (freqHz > 100)
        {
            // printf("%" PRIu32 " MHz is too low, skip PLL, Set clk to %" PRIu32 " MHz\n", (uint32_t)(freqHz / MHZ), (uint32_t)(actualHz / MHZ));
        }
    }
    else
    {
        PLLConfig_type pllcfg     = { 0 };
        uint64_t       n_fxp24    = 0; // fixed point
        uint32_t       f16MHzLog2 = log_2(actualHz / FREQ_0);

        n_fxp24 = (((uint64_t)actualHz << PLL_FRAC_BITS) * TBL_MUL[f16MHzLog2] + PFD_F_MHZ * MHZ / 2) / (PFD_F_MHZ * MHZ); // rounded
        // static const uint32_t M = (FREQ_XTAL_MHZ / PFD_F_MHZ);
        pllcfg.M = FREQ_XTAL_MHZ / PFD_F_MHZ;
        pllcfg.N = (n_fxp24 >> PLL_FRAC_BITS);
        if (pllcfg.N > 0x00000fff)
        {
            pllcfg.N = 0x00000fff;
        }
        pllcfg.NF = (uint32_t)(n_fxp24 & ((1UL << PLL_FRAC_BITS) - 1));
        if (0 != pllcfg.NF)
        {
            pllcfg.NF |= 1UL << PLL_FRAC_BITS;
        }
        pllcfg.Q1 = calcPllDiv(FREQ_XTAL_MHZ * 1000 / pllcfg.M * pllcfg.N, targetClk1Hz / 1000);
        pllcfg.Q0 = TBL_Q[f16MHzLog2] & 0x00ff;

        setPLL(&pllcfg);

        selClkTop(SEL_CLKTOP_PLL); // select PLL0/1
        __DSB();
    }
    if (!setRAMdivFirst)
    {
        setFDsAuto(actualHz);
    }
    recalculateSystemClocks();

    DARIC_UDMACORE->CFG_CG = 0xffffffff; // every interface in udma on
    __DSB();

    if (setVoltage)
        dvfs(SystemCoreClock);
}

/**
    @Brief: Frenquency unit (Hz. kHz, MHz...) is universal, output frequency shares same unit as input.
    @param *foLp output low-power mode frequency if not NULL.
    @param fInput Input frquency
    @param fdCFG DARIC_CGU->fd* value, uint32_t
    @retval Calculated output frquency, same unit as fInput
*/
uint32_t daricCalcFdOutput(uint32_t *foLp, uint32_t fInput, uint32_t fdCFG)
{
    uint32_t fo = 0;

    fo = (uint64_t)fInput * 256 / fdreg2divfp8(fdCFG);
    if (NULL != foLp)
    {
        *foLp = (uint64_t)fInput * 256 / fdreg2divfp8(fdCFG >> 8);
    }
    // printf("%s(): %9" PRIu32 ", %08" PRIx32 ", %09" PRIu32 "\n", __func__, fInput, fdCFG, fo);
    return fo;
}

/**
    @Brief  Calculate actual fclk frequency and potencial low-power
            mode frequency
    @param  clkTopHz clkTop Frequency in Hz
    @retval Result frenquency unit is Hz, high 32-bit is low-power mode frequency.
*/
uint64_t clkGetFClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdfclk);
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetAClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdaclk);
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetHClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdhclk);
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetIClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdiclk);
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetPClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdpclk);
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetAoClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdaoclk) / 2;
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetAorClk_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    uint32_t cfg  = DARIC_CGU->fdaoram;
    cfg           = (((cfg & 0xff00) << 8) | (cfg & 0xff)) * 0x00000101;
    fo            = daricCalcFdOutput(&folp, clkTopHz, cfg) / 2;
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetClkPer_Hz(uint32_t clkTopHz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkTopHz, DARIC_CGU->fdper) / 2;
    folp /= 2;
    return ((uint64_t)folp << 32) | fo;
}

uint64_t clkGetClkPke_Hz(uint32_t clkF1Hz)
{
    uint32_t fo   = 0;
    uint32_t folp = 0;
    fo            = daricCalcFdOutput(&folp, clkF1Hz, DARIC_CGU->fdpke);
    return ((uint64_t)folp << 32) | fo;
}

uint32_t clkGetClkTop_MHz(void)
{
    __DSB();
    __ISB();
    uint32_t t0 = 0, t1 = 0;
    uint32_t f0MHz      = 0;
    uint32_t fclksysMHz = clkGetClkSysHz() / MHZ;

    t0               = DARIC_IPC->pll_mn;
    t1               = DARIC_IPC->pll_f;
    uint32_t M       = (t0 >> 12) & 0x1F;
    uint64_t N       = t0 & 0xFFF;
    double   N_Float = (N << 24) | ((0 != (t1 & 0x01000000)) ? (t1 & 0x00ffffff) : 0);
    N_Float /= (double)0x01000000;
    t1            = DARIC_IPC->pll_q;
    double   fvco = fclksysMHz / M * N_Float;
    uint32_t q1   = (t1 >> 4) & 0x07;
    uint32_t q0   = t1 & 0x07;
    f0MHz         = fvco / ((q1 + 1) * (q0 + 1));

    if (SEL_CLKTOP_SYS == DARIC_CGU->cgusel0)
    {
        f0MHz = fclksysMHz;
    }
    return f0MHz;
}
/**
    @Brief: a fiunction for debugging
*/
void clkAnalysis()
{
    static const char *const CLKSYS_SRCS[2] = { "OSC32M", "XTAK48M" };
    static const char *const CLKTOP_SRCS[2] = { "ClkSys", "PLLClk0" };
    __DSB();
    __ISB();
    uint32_t t0 = 0, t1 = 0;
    double   f0MHz = 0, f1MHz = 0;
    uint32_t fclksysMHz = clkGetClkSysHz() / MHZ;

    printf("ClkSys source: %s.\n", CLKSYS_SRCS[DARIC_CGU->cgusel1]);
    printf("ClkTop source: %s.\n", CLKTOP_SRCS[DARIC_CGU->cgusel0]);

    printf("PLL:\n");
    t0               = DARIC_IPC->pll_mn;
    t1               = DARIC_IPC->pll_f;
    uint32_t M       = (t0 >> 12) & 0x1F;
    uint32_t N       = t0 & 0xFFF;
    double   N_Float = (N << 24) | ((0 != (t1 & 0x01000000)) ? (t1 & 0x00ffffff) : 0);
    printf("    M = %2" PRIu32 ", N %3" PRIu32 ", F  %07" PRIx32 ".\n", M, N, t1);
    N_Float /= (double)0x01000000;
    t1          = DARIC_IPC->pll_q;
    double fvco = fclksysMHz / M * N_Float;
    printf("    M = %2" PRIu32 ", F_PFD = %8" PRIu32 "Hz.\n", M, fclksysMHz * 1000000 / M);
    printf("    N = %13.8f, F_VCO = %12.6f MHz.\n", N_Float, fvco);
    uint32_t q1 = (t1 >> 4) & 0x07;
    uint32_t q0 = t1 & 0x07;
    f0MHz       = fvco / ((q1 + 1) * (q0 + 1));
    printf("    clk0 q = %1" PRIu32 "x%1" PRIu32 ", F_CLK0 = %12.6f MHz.\n", q1 + 1, q0 + 1, f0MHz);
    q1    = (t1 >> 12) & 0x07;
    q0    = (t1 >> 8) & 0x07;
    f1MHz = fvco / ((q1 + 1) * (q0 + 1));
    printf("    clk1 q = %1" PRIu32 "x%1" PRIu32 ", F_CLK0 = %12.6f MHz.\n", q1 + 1, q0 + 1, f1MHz);

    if (SEL_CLKTOP_SYS == DARIC_CGU->cgusel0)
    {
        f0MHz = fclksysMHz;
        f1MHz = fclksysMHz;
    }
    printf("f0, f1  : %8.3f MHz, %8.3f MHz\n", f0MHz, f1MHz);
    printf("fdclk    : %8.3f MHz,  %08" PRIx32 ", div  %5.2f\n", f0MHz, DARIC_CGU->fdfclk, fdreg2divfp8(DARIC_CGU->fdfclk) / 256.0);

    printf("fds, f: %08" PRIx32 " a: %08" PRIx32 " h: %08" PRIx32 " i: %08" PRIx32 "\n", DARIC_CGU->fdfclk, DARIC_CGU->fdaclk, DARIC_CGU->fdhclk, DARIC_CGU->fdiclk);
    printf("     p: %08" PRIx32 " per: %08" PRIx32 " ao: %08" PRIx32 " aoram: %08" PRIx32 "\n", DARIC_CGU->fdpclk, DARIC_CGU->fdper, DARIC_CGU->fdaoclk, DARIC_CGU->fdaoram);

    printf("Calculated:\n");
#if 0
    printf("  fclk    : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdfclk )));
    printf("  aclk    : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdaclk )));
    printf("  hclk    : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdhclk )));
    printf("  iclk    : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdiclk )));
    printf("  pclk    : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdpclk )));
    printf("  clkpke  : %8.3f MHz\n", (f1MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdpke  )));
    printf("  clkao   : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdao   )) / 2);
    printf("  clkaoram: %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdaoram)) / 2);
    printf("  per     : %8.3f MHz\n", (f0MHz * 0x100 / fdreg2divfp8(DARIC_CGU->fdper  )) / 2);
#else
    printf("  fclk    : %8" PRIu32 " kHz\n", (uint32_t)(clkGetFClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  aclk    : %8" PRIu32 " kHz\n", (uint32_t)(clkGetAClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  hclk    : %8" PRIu32 " kHz\n", (uint32_t)(clkGetHClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  iclk    : %8" PRIu32 " kHz\n", (uint32_t)(clkGetIClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  pclk    : %8" PRIu32 " kHz\n", (uint32_t)(clkGetPClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  aoclk   : %8" PRIu32 " kHz\n", (uint32_t)(clkGetAoClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  aoramclk: %8" PRIu32 " kHz\n", (uint32_t)(clkGetAorClk_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
    printf("  clkpke  : %8" PRIu32 " kHz\n", (uint32_t)(clkGetClkPke_Hz((uint32_t)(f1MHz * MHZ))) / 1000);
    printf("  per     : %8" PRIu32 " kHz\n", (uint32_t)(clkGetClkPer_Hz((uint32_t)(f0MHz * MHZ))) / 1000);
#endif

#if 0
    printf("Frequency Counters:\n");
    printf("  pll0    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq3 >> 16) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  pll1    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq3 >> 0) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  fclk    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq0 >> 16) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  clkpke  : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq0 >> 0) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  clkao   : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq1 >> 16) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  clkaoram: %8.1f MHz\n", (((DARIC_CGU->cgufsfreq1 >> 0) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  intosc  : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq2 >> 16) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
    printf("  xtal    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq2 >> 0) & 0xffff) + 1) / (double)F_SENSOR_FACTOR);
#else
    printf("Frequency Counters:\n");
    printf("  pll0    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq3 >> 16) & 0xffff) + 1) / 1.0);
    printf("  pll1    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq3 >> 0) & 0xffff) + 1) / 1.0);
    printf("  fclk    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq0 >> 16) & 0xffff) + 1) / 1.0);
    printf("  clkpke  : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq0 >> 0) & 0xffff) + 1) / 1.0);
    printf("  clkao   : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq1 >> 16) & 0xffff) + 1) / 1.0);
    printf("  clkaoram: %8.1f MHz\n", (((DARIC_CGU->cgufsfreq1 >> 0) & 0xffff) + 1) / 1.0);
    printf("  intosc  : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq2 >> 16) & 0xffff) + 1) / 1.0);
    printf("  xtal    : %8.1f MHz\n", (((DARIC_CGU->cgufsfreq2 >> 0) & 0xffff) + 1) / 1.0);
#endif
}

void clkAnalysisDeprecated()
{
    static const char *const CLKSYS_SRCS[2]  = { "OSC32M", "XTAK48M" };
    static const char *const CLKTOP_SRCS[2]  = { "ClkSys", "PLLClk0" };
    static const uint32_t    CLKSYS_F_MHZ[2] = { 32, 48 };

    uint32_t t0 = 0, t1 = 0;
    double   f0MHz = 0, f1MHz = 0;
    uint32_t fclksys = CLKSYS_F_MHZ[DARIC_CGU->cgusel1];

    printf("ClkSys source: %s.\n", CLKSYS_SRCS[DARIC_CGU->cgusel1]);
    printf("ClkTop source: %s.\n", CLKTOP_SRCS[DARIC_CGU->cgusel0]);
    printf("PLL:\n");
    t0               = DARIC_IPC->pll_mn;
    t1               = DARIC_IPC->pll_f;
    uint32_t M       = (t0 >> 12) & 0x1F;
    uint64_t N       = t0 & 0xFFF;
    double   N_Float = (N << 24) | ((0 != (t1 & 0x01000000)) ? (t1 & 0x00ffffff) : 0);
    N_Float /= (double)0x01000000;
    t1          = DARIC_IPC->pll_q;
    double fvco = fclksys / M * N_Float;
    printf("    M = %2" PRIu32 ", F_PFD = %8" PRIu32 "Hz.\n", M, fclksys * 1000000 / M);
    printf("    N = %13.8f, F_VCO = %12.6f MHz.\n", N_Float, fvco);
    uint32_t q1 = (t1 >> 4) & 0x07;
    uint32_t q0 = t1 & 0x07;
    f0MHz       = fvco / ((q1 + 1) * (q0 + 1));
    printf("    clk0 q = %1" PRIu32 "x%1" PRIu32 ", F_CLK0 = %12.6f MHz.\n", q1 + 1, q0 + 1, f0MHz);
    q1    = (t1 >> 12) & 0x07;
    q0    = (t1 >> 8) & 0x07;
    f1MHz = fvco / ((q1 + 1) * (q0 + 1));
    printf("    clk1 q = %1" PRIu32 "x%1" PRIu32 ", F_CLK0 = %12.6f MHz.\n", q1 + 1, q0 + 1, f1MHz);

    if (0 == DARIC_CGU->cgusel0)
    {
        f0MHz = (double)CLKSYS_F_MHZ[DARIC_CGU->cgusel1];
    }
    printf("fds, f: %08" PRIx32 " a: %08" PRIx32 " h: %08" PRIx32 " i: %08" PRIx32 "\n", DARIC_CGU->fdfclk, DARIC_CGU->fdaclk, DARIC_CGU->fdhclk, DARIC_CGU->fdiclk);
    printf("     p: %08" PRIx32 " per: %08" PRIx32 " ao: %08" PRIx32 " aoram: %08" PRIx32 "\n", DARIC_CGU->fdpclk, DARIC_CGU->fdper, DARIC_CGU->fdaoclk, DARIC_CGU->fdaoram);

    printf("Calculated:\n");
    printf("  fclk    : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdfclk & 0xff) + 1) / 0x100));
    printf("  aclk    : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdaclk & 0xff) + 1) / 0x100));
    printf("  hclk    : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdhclk & 0xff) + 1) / 0x100));
    printf("  iclk    : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdiclk & 0xff) + 1) / 0x100));
    printf("  pclk    : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdpclk & 0xff) + 1) / 0x100));
    printf("  clkpke  : %8.3f MHz\n", (f1MHz * ((DARIC_CGU->fdpke & 0xff) + 1) / 0x100));
    printf("  clkao   : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdaoclk & 0xff) + 1) / 0x100 / 2));
    printf("  clkaoram: %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdaoram & 0xff) + 1) / 0x100 / 2));
    printf("  per     : %8.3f MHz\n", (f0MHz * ((DARIC_CGU->fdper & 0xff) + 1) / 0x100 / 2));

    printf("Frequency Counters:\n");
    printf("  pll0    : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq3 >> 16) & 0xffff) * 0.1);
    printf("  pll1    : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq3 >> 0) & 0xffff) * 0.1);
    printf("  fclk    : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq0 >> 16) & 0xffff) * 0.1);
    printf("  clkpke  : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq0 >> 0) & 0xffff) * 0.1);
    printf("  clkao   : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq1 >> 16) & 0xffff) * 0.1);
    printf("  clkaoram: %8.3f MHz\n", ((DARIC_CGU->cgufsfreq1 >> 0) & 0xffff) * 0.1);
    printf("  intosc  : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq2 >> 16) & 0xffff) * 0.1);
    printf("  xtal    : %8.3f MHz\n", ((DARIC_CGU->cgufsfreq2 >> 0) & 0xffff) * 0.1);
}

/******************************sram triming*****************************************/
// 0.8v   0.9v
const uint16_t TABLE_SRAM_TRM[][2] = {
    { 0x8045, 0x6140 }, // 0
    { 0x8140, 0x6000 }, // 1
    { 0x8148, 0x6100 }, // 2
    { 0x8148, 0x6100 }, // 3
    { 0x8148, 0x6100 }, // 4
    { 0x8148, 0x6100 }, // 5
    { 0x8140, 0x6000 }, // 6
    { 0x8140, 0x6000 }, // 7
    { 0x804f, 0x4045 }, // 8
    { 0x8140, 0x6000 }, // 9
    { 0x8048, 0x6140 }, // a
    { 0x8048, 0x6140 }, // b
    { 0x8048, 0x6140 }, // c
    { 0x8048, 0x6140 }, // d
    { 0x7000, 0x2800 }, // e
    { 0x7000, 0x2800 }, // f
    { 0x7000, 0x2800 }, // 10
    { 0x7000, 0x2800 }, // 11
    { 0x7000, 0x2800 }, // 12
    { 0x8148, 0x6100 }, // 13
    { 0x7000, 0x2800 }, // 14
    { 0x7000, 0x2800 }, // 15
    { 0x7000, 0x2800 }, // 16
    { 0x7000, 0x2800 }, // 17
    { 0x7000, 0x2800 }, // 18
    { 0x7000, 0x2800 }, // 19
    { 0x8045, 0x6140 }, // 1a
    { 0x8048, 0x8048 }, // 1b
};

void configSramTrim(bool isAbove900mV)
{
    for (size_t i = 0; i < sizeof(TABLE_SRAM_TRM) / sizeof(uint16_t) / 2; i++)
    {
        DARIC_SRAMTRIM->cr = ((i << 16) & 0xffff0000) | TABLE_SRAM_TRM[i][isAbove900mV ? 1 : 0];
        __DMB();
    }
    DARIC_SRAMTRIM->cr = 0x5a;
    __DSB();
}

const DVFS_VOLTAGE_TABLE_t DVFS_TAB[] = {
    { 48, 800, 0x1E },    //  0:   48 or 32 MHz  0.8V
    { 100, 800, 0x1E },   //  1:  100 MHz 0.8V
    { 200, 800, 0x1E },   //  2:  200 MHz 0.8V
    { 300, 800, 0x1E },   //  3:  300 MHz 0.8V
    { 400, 800, 0x1E },   //  4:  400 MHz 0.8V
    { 500, 850, 0x23 },   //  5:  500 MHz 0.85V
    { 600, 850, 0x23 },   //  6:  600 MHz 0.85V
    { 700, 900, 0x28 },   //  7:  700 MHz 0.9V
    { 800, 950, 0x2D },   //  8:  800 MHz 0.95V
    { 900, 950, 0x2D },   //  9:  900 MHz 0.95V
    { 1000, 950, 0x2D },  // 10: 1000 MHz 0.95V
    { 1100, 1000, 0x32 }, // 11: 1100 MHz 1V
    { 1200, 1050, 0x37 }, // 12: 1200 MHz 1.05V
};

// static
size_t N_DVFS_TAB = sizeof(DVFS_TAB) / sizeof(DVFS_VOLTAGE_TABLE_t);

/**
    @brief  Dynamic Voltage Frequency Scaling
    @param step performance step, normal range 0-8. 9-12 is for debugging/stress only
*/
void evbAxp223SetRegister(uint8_t addr, uint8_t dat);
void dvfs(uint32_t freq)
{
    uint8_t step = 0;
    if (freq <= 400 * MHZ)
    {
        step = 0;
    }
    else if (freq <= 600 * MHZ)
    {
        step = 5;
    }
    else if (freq <= 700 * MHZ)
    {
        step = 7;
    }
    else if (freq <= 1000 * MHZ)
    {
        step = 8;
    }
    else if (freq <= 1100 * MHZ)
    {
        step = 11;
    }
    else
    {
        step = 12;
    }
    // printf("DVFS to %" PRIu32 " MHz, step %u, reg 0x%02x.\n", freq / MHZ, step, DVFS_TAB[step]);
    // evbAxp223SetRegister(0x83, DVFS_TAB[step]);
    configSramTrim(DVFS_TAB[step].voltmV >= 900); // sram trim 0.8 和 0.9 两套配置
}

uint16_t voltV85dmV = 950;
void     dvfs_sample(uint16_t step)
{
    uint16_t targetVmV    = DVFS_TAB[step].voltmV;
    bool     voltageFirst = targetVmV > voltV85dmV;

    if (voltageFirst)
    {
        // evbAxp223SetRegister(0x83, DVFS_TAB[step].voltRegData);
    }

    initClockSimple(step);

    if (!voltageFirst)
    {
        // evbAxp223SetRegister(0x83, DVFS_TAB[step].voltRegData);
    }

    configSramTrim(targetVmV >= 900);
}

/*
 *  Using NOP instructions to achieve a delay effect
 *  is only suitable for situations where precision is not required,
 *  and it continuously occupies processor time.
 */
void sys_nopDelayMs(uint32_t ms)
{
    int nopCnt;
    /* wait/poll lock status */
    nopCnt = (SystemCoreClock / 1000 / 2) * ms;
    __asm volatile(
        "1:             \n"
        "subs %0, #1    \n"
        "bne 1b         \n"
        : "+r"(nopCnt)
        :
        : "cc");
}

void DaricClockInit(uint32_t freq)
{
    uint32_t clkMhz   = freq / MHZ;
    int16_t  clkIndex = -1;
    for (size_t i = 0; i < N_DVFS_TAB; i++)
    {
        if (DVFS_TAB[i].freq == clkMhz)
        {
            clkIndex = i;
            break;
        }
    }
    if (clkIndex < 0)
    {
        clkIndex = 3;
    }
    dvfs_sample(clkIndex);

    /* wait/poll lock status */
    sys_nopDelayMs(5);
}

void sys_nopDelayUs(uint32_t us)
{
    int nopCnt;
    nopCnt = (SystemCoreClock / 1000000 / 2) * us;
    __asm volatile(
        "1:             \n"
        "subs %0, #1    \n"
        "bne 1b         \n"
        : "+r"(nopCnt)
        :
        : "cc");
}

/*---------------------------------------------------------------------------
  Debug UART function
 *---------------------------------------------------------------------------*/
void DUART_Init(void)
{
    DARIC_DUART->EN = 0;
    __DMB();
    __ISB();
    /* 921600 baud rate, check clksys is 32M or 48M */
    if (DARIC_CGU->cgusel1 == 1)
    {
        DARIC_DUART->ETU = 52; // 48,000,000 / 921600 = 52.08
    }
    else
    {
        DARIC_DUART->ETU = 35; // 32,000,000 / 921600 = 34.72
    }
    __DMB();
    __ISB();
    DARIC_DUART->EN = 1;
    __DMB();
    __ISB();
}

void DUART_PutChar(char c)
{
    while (DARIC_DUART->BUSY)
    {
        /* wait */
    }
    DARIC_DUART->TX = (uint8_t)c;
}

/*---------------------------------------------------------------------------
  System initialization function
 *---------------------------------------------------------------------------*/
void SystemInit(void)
{
    /* Do not use global variables because this function is called before
       reaching pre-main. RW section maybe overwritten afterwards. */

#if defined(__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
    SCB->VTOR = (uint32_t)(&__VECTOR_TABLE[0]);
#endif

#if (defined(__FPU_USED) && (__FPU_USED == 1U)) || (defined(__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE > 0U))
    SCB->CPACR |= ((3U << 10U * 2U) | /* enable CP10 Full Access */
                   (3U << 11U * 2U)); /* enable CP11 Full Access */
#endif

    /* Switch to accurate XTAL early to ensure stable UART and memory access */
    initClockASIC(1, 0, false);
    SystemCoreClock = XTAL;

    /* Basic trace to confirm entry into main */
    DUART_Init();
}
