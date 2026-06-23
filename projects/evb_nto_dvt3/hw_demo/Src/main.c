/**
 *******************************************************************************
 * @file    main.c
 * @author  Daric Team
 * @brief   Source file for main.c module.
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
#include <stdint.h>
#include <stdio.h>
#include "daric_hal.h"
#include "daric_pm.h"
#include "pinmap_config.h"
#include "tx_api.h"
#include "daric_filex_app.h"
#include "user_threads_attrdef.h"
#include "platform_threads_attrdef.h"
#include "tg28.h"
#include "daric_pmu.h"
#include "daric_util.h"

int HAL_ClockPreProcess(HAL_CPU_FreqVolMap_TypeDef cpu_fv_map)
{
    //   AXP223_LDO_DCDC_Set_Voltage(AXP223_DCDC2, cpu_fv_map.Voltage);
    BSP_PM_PWR_set(CONFIG_CLOCK_POWER, cpu_fv_map.Voltage);
    return 0;
}

int main(void)
{
    int cnt = 0;
#ifdef HAL_PINMAP_MODULE_ENABLED
    HAL_PINMAP_init(g_pinMap, sizeof(g_pinMap) / sizeof(g_pinMap[0]));
#endif /* HAL_PINMAP_MODULE_ENABLED */

    /* CPU boost */
    /*
     * The configuration of the system frequency requires TG28,
     * so the I2C must be initialized first.
     */
    HAL_Init();
    HAL_TickInit();
    TG28_I2C_init();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    BSP_PM_init();
    /*
     * The file system is initialized here,
     * and all functions that use the file system
     * need to be implemented later.
     */
    daricFilesystemInit();
    daricDataDiskLoad();
    /* The Thread Execution Management provides a solution for managing thread creation 
     * and initialization dependencies in ThreadX systems. 
     * It ensures that threads are created and initialized in the correct order, 
     * with proper handling of inter-thread dependencies. 
     * Please refer to the files threads_dependency.h and threads_dependency.c
     * All threads that need to start on boot will be initialized here.
    */
    startAllThreads();

    while (1)
    {
        printf("[%d] Main thread sleep 10s ...\n", cnt);

        cnt++;

        tx_thread_sleep((CONFIG_SYS_CLOCK_TICKS_PER_SEC) * 10);
    }

    return 0;
}
