/**
 ******************************************************************************
 * @file    daric_hal_conf.h
 * @author  HAL Team
 * @brief   Header file of HAL conf module.
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
#ifndef __DARIC_HAL_CONF_H
#define __DARIC_HAL_CONF_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* Exported constants --------------------------------------------------------*/

    /* ########################## Module Selection ############################## */
    /**
     * @brief This is the list of modules to be used in the HAL driver
     */

    /* Includes ------------------------------------------------------------------*/
    /**
     * @brief Include module's header file
     */

#ifdef HAL_NVIC_MODULE_ENABLED
#include "daric_hal_nvic.h"

/* Drivers priority defines */
#define HAL_GPIO_IRQ_PRIO     2
#define HAL_GPIO_IRQ_SUB_PRIO 0

#define HAL_WDG_IRQ_PRIO     0
#define HAL_WDG_IRQ_SUB_PRIO 0

#define HAL_PWM0_IRQ_PRIO     0
#define HAL_PWM0_IRQ_SUB_PRIO 0
#define HAL_PWM1_IRQ_PRIO     0
#define HAL_PWM1_IRQ_SUB_PRIO 0
#define HAL_PWM2_IRQ_PRIO     0
#define HAL_PWM2_IRQ_SUB_PRIO 0
#define HAL_PWM3_IRQ_PRIO     0
#define HAL_PWM3_IRQ_SUB_PRIO 0

#define HAL_ATIMER_IRQ_PRIO     0
#define HAL_ATIMER_IRQ_SUB_PRIO 0

#define HAL_ADC_IRQ_PRIO     2
#define HAL_ADC_IRQ_SUB_PRIO 2

#define HAL_MDMA_IRQ_PRIO     0
#define HAL_MDMA_IRQ_SUB_PRIO 0

#define HAL_I2C_IRQ_PRIO     2
#define HAL_I2C_IRQ_SUB_PRIO 2

#define HAL_USB_IRQ_PRIO     1
#define HAL_USB_IRQ_SUB_PRIO 2

#define HAL_AOINT_IRQ_PRIO         0
#define HAL_AOINT_IRQ_SUB_PRIO     0
#define HAL_AOWKUPINT_IRQ_PRIO     0
#define HAL_AOWKUPINT_IRQ_SUB_PRIO 0

#define HAL_SD_IRQ_PRIO     2
#define HAL_SD_IRQ_SUB_PRIO 2

#define HAL_SPIM_IRQ_PRIO     1
#define HAL_SPIM_IRQ_SUB_PRIO 1

#define HAL_UART_IRQ_PRIO     2
#define HAL_UART_IRQ_SUB_PRIO 2
#endif /* HAL_NVIC_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
#include "daric_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_PINMAP_MODULE_ENABLED
#include "daric_hal_pinmap.h"
#endif /* HAL_PINMAP_MODULE_ENABLED */

#ifdef HAL_WDG_MODULE_ENABLED
#include "daric_hal_wdg.h"
#endif /* HAL_WDG_MODULE_ENABLED */

#ifdef HAL_PWM_MODULE_ENABLED
#include "daric_hal_pwm.h"
#endif

#ifdef HAL_ATIMER_MODULE_ENABLED
#include "daric_hal_atimer.h"
#endif /* HAL_ATIMER_MODULE_ENABLED */

/* HAL_TICK_ATIMER_ENABLED: use ATimer as the HAL tick source (HAL_TickInit /
 * HAL_GetTick / HAL_Delay). Independent from HAL_ATIMER_MODULE_ENABLED, which
 * controls whether the full ATimer driver is compiled. Requires ATimer HW. */
#ifdef HAL_TICK_ATIMER_ENABLED
#ifndef HAL_ATIMER_MODULE_ENABLED
#include "daric_hal_atimer.h"
#endif
#endif /* HAL_TICK_ATIMER_ENABLED */

#ifdef HAL_MDMA_MODULE_ENABLED
#include "daric_hal_mdma.h"
#endif /* HAL_MDMA_MODULE_ENABLED */

#ifdef HAL_USB_MODULE_ENABLED
#include "daric_hal_usb.h"
#endif

#ifdef HAL_IFRAMMGR_MODULE_ENABLED
#include "daric_ifram.h"
#endif

#ifdef HAL_AO_MODULE_ENABLED
#include "daric_hal_ao.h"
#endif

#ifdef HAL_PMU_MODULE_ENABLED
#include "daric_hal_pmu.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef USE_FULL_ASSERT
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

#ifdef __cplusplus
}
#endif

#endif /* __DARIC_HAL_CONF_H */
