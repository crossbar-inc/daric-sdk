/**
 ******************************************************************************
 * @file    daric_activecard_nto_vibrator.h
 * @author  PERIPHERIAL BSP Team
 * @brief   This file contains the common defines and functions prototypes for
 *          the daric_activecard_nto_vibrator.c driver.
 ******************************************************************************
 * @attention
 *
 * © Copyright CrossBar, Inc. 2024.
 *
 * All rights reserved.
 *
 * This software is the proprietary property of CrossBar, Inc. and is protected
 * by copyright laws. Any unauthorized reproduction, distribution, or
 * modification is strictly prohibited.
 *
 ******************************************************************************
 */
#ifndef DARIC_ACTIVECARD_NTO_VIB_H
#define DARIC_ACTIVECARD_NTO_VIB_H

#include <stdint.h>

/**
 * @brief Initializes the vibrator hardware.
 * @retval BSP status code.
 */
int BSP_Vibrator_Init();

/**
 * @brief Activates the vibrator for a short duration.
 * @retval BSP status code.
 */
int BSP_Vibrator_Short();

/**
 * @brief Activates the vibrator for a specified duration.
 * @param ms Duration in milliseconds for which the vibrator should remain active.
 * @retval BSP status code.
 */
int BSP_Vibrator_Long(uint32_t ms);

/**
 * @brief Plays a predefined RTP example.
 * @retval BSP status code.
 */
int BSP_Vibrator_Rtp_Play(void);

#endif /* DARIC_ACTIVECARD_NTO_VIB_H */
