/**
******************************************************************************
* @file    daric_pmu.c
* @author  PERIPHERIAL BSP Team
* @brief   This file contains the apis to enable/disable pmu power output
******************************************************************************
* @attention
*
* © Copyright CrossBar, Inc. 2024.
* All rights reserved.
*
* All rights reserved.
*
* This software is the proprietary property of CrossBar, Inc. and is protected
* by copyright laws. Any unauthorized reproduction, distribution, or
* modification is strictly prohibited.
*
******************************************************************************
*/

#include "daric_pmu.h"
#include "tg28.h"
#include <stdint.h>

/**
 * @brief  Enable or disable the power output.
 * @param  ch the selected channel. Should be one of BSP_PMU_PWR_E.
 * @param  en enable power output when true, disable otherwise.
 */
void BSP_PMU_Power_en(uint8_t ch, bool en) {
    if (en) {
        TG28_Ch_Power_On(ch);
    } else {
        TG28_Ch_Power_Off(ch);
    }
}

/**
 * @brief  Set the channel voltage and enable the channel.
 * @param  ch the selected channel. Should be one of BSP_PMU_PWR_E.
 * @param  mv the voltagbe(micro v) setted.
 */
void BSP_PMU_Power_set(uint8_t ch, uint16_t mv) {
    TG28_Ch_Power_Set(ch, mv);
}