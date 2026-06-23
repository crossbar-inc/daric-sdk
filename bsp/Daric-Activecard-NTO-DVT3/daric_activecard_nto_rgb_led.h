/**
******************************************************************************
* @file    daric_activecard_nto_rgb_led.h
* @author  PERIPHERIAL BSP Team
* @brief   This file contains the common defines and functions prototypes for
*          the daric_activecard_nto_rgb_led.h driver.
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
#ifndef DARIC_ACTIVECARD_NTO_RGB_LED_H
#define DARIC_ACTIVECARD_NTO_RGB_LED_H
#include "rgb_led.h"
#include <stdint.h>

void BSP_RGB_Enter_LowPower(void);
void BSP_RGB_Exit_LowPower(void);

void BSP_RGB_LED_On(uint8_t led_id, RGB_COLOR color, uint8_t dim);
void BSP_RGB_LED_Off(uint8_t led_id);
void BSP_RGB_LED_Blink(uint8_t led_id, RGB_COLOR color, RGB_TIME full_on, RGB_TIME full_off);
void BSP_RGB_LED_Breath(uint8_t led_id, RGB_COLOR color, RGB_TIME fade_on, RGB_TIME full_on,
                        RGB_TIME fade_off, RGB_TIME full_off);

#endif /* DARIC_ACTIVECARD_NTO_RGB_LED_H */
