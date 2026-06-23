/**
 ******************************************************************************
 * @file    daric_vibrator_rot.h
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
#ifndef DARIC_VIBRATOR_ROT_H
#define DARIC_VIBRATOR_ROT_H

#include <stdint.h>

/**
 * @brief Initializes the vibrator hardware.
 * @retval BSP status code.
 */
int BSP_Vibrator_Init_rotary();

/**
 * @brief Activates the vibrator for a short duration.
 * @retval BSP status code.
 */
int BSP_Vibrator_Short_rotary();

/**
 * @brief Activates the vibrator for a specified duration.
 * @param ms Duration in milliseconds for which the vibrator should remain active.
 * @retval BSP status code.
 */
int BSP_Vibrator_Long_rotary(uint32_t ms);

/**
 * @brief Plays a predefined RTP example.
 * @retval BSP status code.
 */
int BSP_Vibrator_Rtp_Play_rotary(void);

#endif /* DARIC_VIBRATOR_ROT_H */
