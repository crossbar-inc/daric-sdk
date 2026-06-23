/**
 ******************************************************************************
 * @file    daric_hal.h
 * @author  HAL Team
 * @brief   DARIC Hardware Abstraction Layer (HAL) header file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DARIC_HAL_H
#define __DARIC_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "daric_hal_conf.h"
#include "daric_hal_def.h"

    /** @addtogroup DARIC_HAL_Driver
     * @{
     */

    /** @addtogroup HAL
     * @{
     */

    /* Exported types ------------------------------------------------------------*/
    typedef enum
    {
        HAL_CPU_FREQSEL_48MHZ = 0,
        HAL_CPU_FREQSEL_100MHZ,
        HAL_CPU_FREQSEL_200MHZ,
        HAL_CPU_FREQSEL_300MHZ,
        HAL_CPU_FREQSEL_400MHZ,
        HAL_CPU_FREQSEL_500MHZ,
        HAL_CPU_FREQSEL_600MHZ,
        HAL_CPU_FREQSEL_700MHZ,
        HAL_CPU_FREQSEL_800MHZ,

        HAL_CPU_FREQSEL_NUM,
    } HAL_CPU_FreqSel_TypeDef;

    typedef struct
    {
        HAL_CPU_FreqSel_TypeDef FreqSel;
        uint32_t                Frequency; /*< CPU frequency, unit: Hz. */
        uint32_t                Voltage;   /*< CPU voltage, unit: mV. */
    } HAL_CPU_FreqVolMap_TypeDef;

/* Exported constants --------------------------------------------------------*/
/** @defgroup HAL_Exported_Constants HAL Exported Constants
 * @{
 */
#define HAL_UNIT_MHZ (1000000UL)
    /**
     * @}
     */

    /* Exported macro ------------------------------------------------------------*/
    /** @defgroup HAL_Exported_Macros HAL Exported Macros
     * @{
     */
    __attribute__((always_inline)) static inline unsigned int __hal_disable(void)
    {
        unsigned int int_posture;

        __asm__ volatile("MRS  %0, PRIMASK " : "=r"(int_posture));
        __asm__ volatile("CPSID i" : : : "memory");
        return (int_posture);
    }

    __attribute__((always_inline)) static inline void __hal_restore(unsigned int int_posture)
    {
        __asm__ volatile("MSR  PRIMASK,%0" : : "r"(int_posture) : "memory");
    }

#define HAL_INTERRUPT_SAVE_AREA unsigned int interrupt_save;
#define HAL_LOCK                interrupt_save = __hal_disable();
#define HAL_UNLOCK              __hal_restore(interrupt_save);
    /**
     * @}
     */

    /** @defgroup HAL_Private_Macros HAL Private Macros
     * @{
     */
    /**
     * @}
     */

    /* Exported functions --------------------------------------------------------*/
    /** @addtogroup HAL_Exported_Functions
     * @{
     */
    /** @addtogroup HAL_Exported_Functions_Group1
     * @{
     */
    /* Initialization and Configuration functions  ******************************/
    HAL_StatusTypeDef HAL_Init(void);
    HAL_StatusTypeDef HAL_DeInit(void);
    HAL_StatusTypeDef HAL_ClockConfig(HAL_CPU_FreqSel_TypeDef freq_sel);

    /**
     * @brief Get CPU core frequency configuration.
     * @param none.
     * @retval CPU core frequency(unit:MHz).
     */
    uint32_t HAL_GetCoreClkMHz(void);

    /**
     * @brief Get peripheral frequency configuration.
     * @param none.
     * @retval peripheral frequency(unit:Hz).
     */
    double HAL_GetPerClkHz(void);
    /**
     * @}
     */

    /* Exported variables
     * ---------------------------------------------------------*/
    /** @addtogroup HAL_Exported_Variables
     * @{
     */
    /**
     * @}
     */

    /** @addtogroup HAL_Exported_Functions_Group2
     * @{
     */
    /* Peripheral Control functions
     * ************************************************/
    HAL_StatusTypeDef HAL_TickInit(void);
    uint32_t HAL_GetTick(void);
    uint32_t HAL_GetUs(void);
    uint32_t HAL_GetMs(void);
    void     HAL_Delay(uint32_t Delay);
    void     HAL_DelayUs(uint32_t Delay);
    void     HAL_SysTick_Handler(void);
    /**
     * @}
     */

    /**
     * @}
     */
    /* Private types -------------------------------------------------------------*/
    /* Private variables ---------------------------------------------------------*/
    /** @defgroup HAL_Private_Variables HAL Private Variables
     * @{
     */
    /**
     * @}
     */
    /* Private constants ---------------------------------------------------------*/
    /** @defgroup HAL_Private_Constants HAL Private Constants
     * @{
     */
    /**
     * @}
     */
    /* Private macros ------------------------------------------------------------*/
    /* Private functions ---------------------------------------------------------*/
    /**
     * @}
     */

    /**
     * @}
     */

#ifdef __cplusplus
}
#endif

#endif /* __DARIC_HAL_H */
