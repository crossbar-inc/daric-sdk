/**
 ******************************************************************************
 * @file    daric_vibrator_linear.h
 * @author  PERIPHERIAL BSP Team
 * @brief   This file contains the common defines and functions prototypes for
 *          the daric_vibrator_linear.c driver.
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
#ifndef DARIC_VIBRATOR_LINEAR_H
#define DARIC_VIBRATOR_LINEAR_H

#include <stdint.h>

/**
 * @brief Initializes the vibrator hardware.
 * @retval BSP status code.
 */
int BSP_Vibrator_Init_linear();

/**
 * @brief Activates the vibrator for a short duration.
 * @retval BSP status code.
 */
int BSP_Vibrator_Short_linear();

/**
 * @brief Activates the vibrator for a specified duration.
 * @param ms Duration in milliseconds for which the vibrator should remain active.
 * @retval BSP status code.
 */
int BSP_Vibrator_Long_linear(uint32_t ms);

/**
 * @brief Plays a predefined RTP example.
 * @retval BSP status code.
 */
int BSP_Vibrator_Rtp_Play_linear(void);

#endif /* DARIC_VIBRATOR_LINEAR_H */
