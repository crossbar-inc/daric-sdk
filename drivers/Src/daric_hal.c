/**
 ******************************************************************************
 * @file    daric_hal.c
 * @author  HAL Team
 * @brief   DARIC Hardware Abstraction Layer (HAL) source file.
 *          This file provides functions for initialization, configuration,
 *          and delay utilities.
 *
 ******************************************************************************
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
#include <daric_hal.h>
#include "system_daric.h"

#ifdef HAL_IFRAMMGR_MODULE_ENABLED
#ifdef HAL_USB_MODULE_ENABLED
#ifdef CONFIG_IFRAM_BASE
#define IFRAM_BASE (CONFIG_IFRAM_BASE + UDC_IFRAM_SIZE)
#define IFRAM_SIZE (CONFIG_IFRAM_SIZE - UDC_IFRAM_SIZE)
#else
#define IFRAM_BASE (0x50000000 + UDC_IFRAM_SIZE)
#define IFRAM_SIZE (0x00040000 - UDC_IFRAM_SIZE)
#endif
#else
#ifdef CONFIG_IFRAM_BASE
#define IFRAM_BASE CONFIG_IFRAM_BASE
#define IFRAM_SIZE CONFIG_IFRAM_SIZE
#else
#define IFRAM_BASE 0x50000000
#define IFRAM_SIZE 0x00040000
#endif
#endif
#endif

#define HAL_MICROSECOND_PER_SECOND (1000000UL)
#define HAL_MILLISECOND_PER_SECOND (1000UL)

#define HAL_CPU_FREQ_ERROR (10 * HAL_UNIT_MHZ)

const HAL_CPU_FreqVolMap_TypeDef g_cpu_fv_map[HAL_CPU_FREQSEL_NUM] = {
    /* {FreqSel, Frequency, Voltage} */
    { HAL_CPU_FREQSEL_48MHZ, 48 * HAL_UNIT_MHZ, 900 },   { HAL_CPU_FREQSEL_100MHZ, 100 * HAL_UNIT_MHZ, 900 }, { HAL_CPU_FREQSEL_200MHZ, 200 * HAL_UNIT_MHZ, 900 },
    { HAL_CPU_FREQSEL_300MHZ, 300 * HAL_UNIT_MHZ, 900 }, { HAL_CPU_FREQSEL_400MHZ, 400 * HAL_UNIT_MHZ, 900 }, { HAL_CPU_FREQSEL_500MHZ, 500 * HAL_UNIT_MHZ, 900 },
    { HAL_CPU_FREQSEL_600MHZ, 600 * HAL_UNIT_MHZ, 900 }, { HAL_CPU_FREQSEL_700MHZ, 700 * HAL_UNIT_MHZ, 950 }, { HAL_CPU_FREQSEL_800MHZ, 800 * HAL_UNIT_MHZ, 1000 }
};

#ifdef HAL_TICK_ATIMER_ENABLED
#define HAL_TICKS_PER_SECOND HAL_MICROSECOND_PER_SECOND
#define HAL_SYSTICK_CYCLES   (((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / (CONFIG_SYS_CLOCK_TICKS_PER_SEC)) - 1)
#define HAL_SYSTICK_FACTOR   (1.04167f)

static ATimer_HandleTypeDef s_sys_tim;
static float                tim_factor;
static bool                 s_tick_initialized = false;
static uint32_t             s_systick_ctrl_saved = 0;

static void HAL_TickStop(void)
{
    s_systick_ctrl_saved = SysTick->CTRL;
    SysTick->CTRL = 0;
    HAL_ATimer_Stop(&s_sys_tim);
}

static void HAL_TickRestart(void)
{
    /* Reconfigure ATimer prescaler for the new clock */
    tim_factor = (float)HAL_GetCoreClkMHz() / 100 * (((DARIC_CGU->fdpclk & 0xff) >= 0x1f) ? (0x1f) : (DARIC_CGU->fdpclk & 0xff)) / 0x0f;

    s_sys_tim.Instance               = ATIMER0;
    s_sys_tim.Init.ClockSelection    = ATIMER_CLOCK_SELECTION_PCLK;
    s_sys_tim.Init.ConterMode        = ATIMER_COUNTER_MODE_RESET;
    s_sys_tim.Init.AutoReloadPreload = -1;
    s_sys_tim.Init.Prescaler         = (uint32_t)(6 * tim_factor - 1);
    s_sys_tim.Init.CascadeMode       = ATIMER_CASCADE_MODE_INDEPENDENT;
    HAL_ATimer_Init(&s_sys_tim);
    HAL_ATimer_Start(&s_sys_tim);

    /* Resync SysTick to the new SystemCoreClock and restore its previous
     * CTRL state (e.g. TICKINT enabled by ThreadX). */
    if (s_systick_ctrl_saved & SysTick_CTRL_ENABLE_Msk)
    {
        SysTick->LOAD = (SystemCoreClock / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1;
        SysTick->VAL  = 0;
        SysTick->CTRL = s_systick_ctrl_saved;
    }
}

HAL_StatusTypeDef HAL_TickInit(void)
{
    tim_factor = (float)HAL_GetCoreClkMHz() / 100 * (((DARIC_CGU->fdpclk & 0xff) >= 0x1f) ? (0x1f) : (DARIC_CGU->fdpclk & 0xff)) / 0x0f;

    s_sys_tim.Instance               = ATIMER0;
    s_sys_tim.Init.ClockSelection    = ATIMER_CLOCK_SELECTION_PCLK;
    s_sys_tim.Init.ConterMode        = ATIMER_COUNTER_MODE_RESET;
    s_sys_tim.Init.AutoReloadPreload = -1;
    s_sys_tim.Init.Prescaler         = (uint32_t)(6 * tim_factor - 1);
    s_sys_tim.Init.CascadeMode       = ATIMER_CASCADE_MODE_INDEPENDENT;
    HAL_ATimer_Init(&s_sys_tim);
    HAL_ATimer_Start(&s_sys_tim);

    s_tick_initialized = true;
    return HAL_OK;
}

__weak void HAL_SysTick_Handler(void) {}

#else
#define HAL_TICKS_PER_SECOND CONFIG_SYS_CLOCK_TICKS_PER_SEC

static volatile uint32_t s_uwTick = 0;

HAL_StatusTypeDef HAL_TickInit(void)
{
    /* Recalibrate LOAD for the current SystemCoreClock. Preserve CTRL as-is:
     * an RTOS (e.g. ThreadX) may have already configured TICKINT before this
     * call, and we must not clear it. If SysTick is not yet running, start it
     * with TICKINT enabled for bare-metal interrupt-driven tick counting. */
    SysTick->LOAD = (SystemCoreClock / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1;
    SysTick->VAL  = 0;
    if (!(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk))
    {
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    }
    return HAL_OK;
}

__weak void HAL_SysTick_Handler(void)
{
    s_uwTick++;
}
#endif

extern void DaricClockInit(uint32_t Freq);
extern void DUART_Init(void);
/**
 * @brief CPU frequency configuration preprocessing.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @param Freq CPU frequency, unit: Hz.
 * @param Vol CPU voltage, unit: mV.
 * @retval 0: SUCCESS, other: FAILURE.
 */
__weak int HAL_ClockPreProcess(HAL_CPU_FreqVolMap_TypeDef cpu_fv_map)
{
    return 0;
}

/**
 * @brief Get CPU core(fclk) frequency configuration.
 * @param none.
 * @retval CPU core frequency(unit:MHz).
 */
uint32_t HAL_GetCoreClkMHz(void)
{
    uint32_t coreClk;

#if defined(CONFIG_SOC_DARIC_NTO_A)
/* return fclk*/
#if defined(CONFIG_SUPPORT_800MFREQ)
    coreClk = clkGetClkTop_MHz();
#else
    /* The frequency sensor was increased by a factor of 10 */
    coreClk = (((DARIC_CGU->cgufsfreq0 >> 16) & 0x0000FFFF) + 5) / 10;
#endif
#else
    coreClk = DARIC_CGU->cgufsfreq2;
#endif

    return coreClk;
}

/**
 * @brief Get peripheral frequency configuration.
 * @param none.
 * @retval peripheral frequency(unit:Hz).
 */
double HAL_GetPerClkHz(void)
{
    volatile double perClk;

#if defined(CONFIG_SOC_DARIC_NTO_A)
#if defined(CONFIG_SUPPORT_800MFREQ)
    uint32_t coreClkHz;
    uint32_t perClkHz;
    coreClkHz = clkGetClkTop_MHz() * 1000000;
    perClkHz  = clkGetClkPer_Hz(coreClkHz);
    perClk    = perClkHz;
#else
    volatile double clktop;
    clktop = (DARIC_CGU->cgufsfreq3 >> 16) & 0x0000FFFF;
    perClk = (clktop * ((double)((DARIC_CGU->fdper & 0xff) + 1) / 0x100)) * 1000000UL / 2;
    /* The frequency sensor was increased by a factor of 10 */
    perClk /= 10;
#endif
#else
    perClk = *((uint32_t *)(DARIC_CGU->cgufsfreq3)) * 1000000;
#endif

    return perClk;
}

/**
 * @brief CPU frequency configuration.
 * @param Freq CPU frequency, unit: Hz.
 * @retval HAL_OK: SUCCESS, other: FAILURE.
 */
HAL_StatusTypeDef HAL_ClockConfig(HAL_CPU_FreqSel_TypeDef freq_sel)
{
    uint32_t coreClkMHz    = HAL_GetCoreClkMHz();
    uint32_t original_freq = coreClkMHz * HAL_UNIT_MHZ;

    /* Check the CPU frequency selection */
    if (freq_sel >= HAL_CPU_FREQSEL_NUM)
    {
        return HAL_ERROR;
    }

    /* Process before cpu clock update */
    if (HAL_ClockPreProcess(g_cpu_fv_map[freq_sel]) != 0)
    {
        return HAL_ERROR;
    }

    /* Source lock */
    HAL_INTERRUPT_SAVE_AREA
    HAL_LOCK

#ifdef HAL_TICK_ATIMER_ENABLED
    uint32_t atimer_counter = 0;
    if (s_tick_initialized)
    {
        HAL_TickStop();
        atimer_counter = HAL_ATimer_GetCounter(&s_sys_tim);
    }
#else
    uint32_t systick_ctrl_saved = SysTick->CTRL;
    SysTick->CTRL = 0;
#endif

    /* CPU frequency configure */
    DaricClockInit(g_cpu_fv_map[freq_sel].Frequency);

    coreClkMHz = DARIC_CGU->cgufsfreq0; // HAL_GetCoreClkMHz();
    if ((HAL_GetCoreClkMHz() * HAL_UNIT_MHZ < g_cpu_fv_map[freq_sel].Frequency - HAL_CPU_FREQ_ERROR)
        || (HAL_GetCoreClkMHz() * HAL_UNIT_MHZ > g_cpu_fv_map[freq_sel].Frequency + HAL_CPU_FREQ_ERROR))
    {
        /* Revert the clock */
        DaricClockInit(original_freq);

        /* Configure DUART */
        DUART_Init();
        /* Update the SystemCoreClock */
        SystemCoreClock = HAL_GetCoreClkMHz() * HAL_UNIT_MHZ;

#ifdef HAL_TICK_ATIMER_ENABLED
        if (s_tick_initialized)
        {
            HAL_TickRestart();
            ATIMER0->CNT = atimer_counter;
        }
#else
        if (systick_ctrl_saved & SysTick_CTRL_ENABLE_Msk)
        {
            SysTick->LOAD = (SystemCoreClock / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1;
            SysTick->VAL  = 0;
            SysTick->CTRL = systick_ctrl_saved;
        }
#endif

        /* Source unlock */
        HAL_UNLOCK

        return HAL_ERROR;
    }
    else
    {
        /* Update the SystemCoreClock */
        SystemCoreClock = HAL_GetCoreClkMHz() * HAL_UNIT_MHZ;

        /* Configure DUART */
        DUART_Init();

#ifdef HAL_TICK_ATIMER_ENABLED
        if (s_tick_initialized)
        {
            HAL_TickRestart();
            ATIMER0->CNT = atimer_counter;
        }
#else
        if (systick_ctrl_saved & SysTick_CTRL_ENABLE_Msk)
        {
            SysTick->LOAD = (SystemCoreClock / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1;
            SysTick->VAL  = 0;
            SysTick->CTRL = systick_ctrl_saved;
        }
#endif
    }
    /* Source unlock */
    HAL_UNLOCK

    return HAL_OK;
}

HAL_StatusTypeDef HAL_Init(void)
{
    /* TODO */
    for (uint8_t freq_sel = 0; freq_sel < HAL_CPU_FREQSEL_NUM; freq_sel++)
    {
        if (g_cpu_fv_map[freq_sel].FreqSel != freq_sel)
        {
            while (1)
            {
                ;
            }
        }
    }

#ifdef HAL_IFRAMMGR_MODULE_ENABLED
    IframMgr_Init((void *)IFRAM_BASE, IFRAM_SIZE);
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
    HAL_GPIO_InitComponent();
#endif
    return HAL_OK;
}

HAL_StatusTypeDef HAL_DeInit(void)
{
    /* TODO */
    return HAL_OK;
}

/**
 * @brief Provides a tick value in systick count.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @retval tick value
 */
__weak uint32_t HAL_GetTick(void)
{
#ifdef HAL_TICK_ATIMER_ENABLED
    return (uint32_t)(HAL_ATimer_GetCounter(&s_sys_tim) / HAL_SYSTICK_FACTOR);
#else
    /* Accumulate ticks via COUNTFLAG so this works without SysTick_Handler
     * (e.g. under an RTOS that owns SysTick_Handler). When the bare-metal
     * HAL_SysTick_Handler is active it also increments s_uwTick, so counts
     * are never lost either way. */
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
        s_uwTick++;
    }
    return s_uwTick;
#endif
}

/**
 * @brief Provides a tick value in microseconds.
 * @note This function is declared as __weak to allow overriding with a
 * different implementation in the user's file.
 * @retval Current tick value in microseconds.
 */
uint32_t HAL_GetUs(void)
{
    return (uint32_t)((uint64_t)HAL_GetTick() * HAL_MICROSECOND_PER_SECOND / HAL_TICKS_PER_SECOND);
}

/**
 * @brief Provides a tick value in millisecond.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @retval tick value
 */
uint32_t HAL_GetMs(void)
{
    return (uint32_t)((uint64_t)HAL_GetTick() * HAL_MILLISECOND_PER_SECOND / HAL_TICKS_PER_SECOND);
}

uint32_t HAL_GetTickPerUs(void)
{
    return (HAL_TICKS_PER_SECOND / HAL_MICROSECOND_PER_SECOND);
}
uint32_t HAL_GetTickPerMs(void)
{
    return (HAL_TICKS_PER_SECOND / HAL_MILLISECOND_PER_SECOND);
}

/**
 * @brief This function provides a minimum delay in microseconds based
 *        on a variable incremented by the microsecond tick function.
 * @note In the default implementation, the SysTick timer is the source of the
 *       time base and is used to generate interrupts at regular intervals where
 *       'uwTick' is incremented. This function creates a busy-wait loop based
 * on 'HAL_GetUs()' for finer time delays in microseconds.
 * @note This function is declared as __weak to allow overriding with a
 * different implementation in the user's file.
 * @param Delay  Specifies the delay duration in microseconds.
 * @retval None
 */
void HAL_DelayUs(uint32_t Delay)
{
    uint32_t tickstart = HAL_GetUs();
    uint32_t wait      = Delay;

    while ((HAL_GetUs() - tickstart) < wait)
    {
    }
}

/**
 * @brief This function provides minimum delay (in millisecond) based
 *        on variable incremented.
 * @note In the default implementation , SysTick timer is the source of time
 * base. It is used to generate interrupts at regular time intervals where
 * uwTick is incremented.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @param Delay  specifies the delay time length, in milliseconds.
 * @retval None
 */
void HAL_Delay(uint32_t Delay)
{
    uint32_t tickstart = HAL_GetMs();
    uint32_t wait      = Delay;

    while ((HAL_GetMs() - tickstart) < wait)
    {
    }
}
