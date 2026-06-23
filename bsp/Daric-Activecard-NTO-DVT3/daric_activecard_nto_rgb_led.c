/**
 ******************************************************************************
 * @file    daric_activecard_nto_rgb2_led.c
 * @author  PERIPHERIAL BSP Team
* @brief    This file contains the common defines and functions prototypes for
*           the  daric_activecard_nto_rgb_led.c driver.
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
#include "rgb_led.h"
#include <stdint.h>

void aw2023_enter_lowpower();
void aw2023_exit_lowpower();
void aw2023_set_rgb_led_always_on(uint32_t color);
void aw2023_led_off(void);
void aw2023_set_rgb_led_flash(uint32_t color, RGB_TIME on_time, RGB_TIME off_time);
void aw2023_set_rgb_led_breath(uint32_t color, RGB_TIME fade_on, RGB_TIME full_on,
                               RGB_TIME fade_off, RGB_TIME full_off);

#define BRIGHTNESS 1

uint32_t convert_color(RGB_COLOR color, uint8_t dim) {
    uint32_t c = 0;
    switch (color) {
    case RGB_RED:
        c = dim << 16;
        break;
    case RGB_GREEN:
        c = dim << 8;
        break;
    case RGB_BLUE:
        c = dim;
        break;
        ;
    }
    return c;
}

/**
 * @brief Set RGB chip enter low power mode
 * @return null
 */
void BSP_RGB_Enter_LowPower(void){
    aw2023_enter_lowpower();
}

/**
 * @brief Set RGB chip exit low power mode
 * @return null
 */
void BSP_RGB_Exit_LowPower(void){
    aw2023_exit_lowpower();
}

/**
 * @brief Set RGB LED to always on
 * @param led_id  led index, should be 0 if there is only one LED
 * @param color  color enum
 * @param dim  the led dim value @deprecated
 * @return null
 */
void BSP_RGB_LED_On(uint8_t led_id, RGB_COLOR color, uint8_t dim) {
    aw2023_set_rgb_led_always_on(convert_color(color, BRIGHTNESS));
}

/**
 * @brief close RGB LED
 * @param led_id  led index, should be 0 if there is only one LED
 * @return null
 */
void BSP_RGB_LED_Off(uint8_t led_id) { aw2023_led_off(); }

/**
 * @brief Sets RGB LED to blink
 * @param led_id  led index, should be 0 if there is only one LED
 * @param color color enum
 * @param full_on the on time in a blink period
 * @param full_off the off time in a blink period
 * @return null
 */
void BSP_RGB_LED_Blink(uint8_t led_id, RGB_COLOR color, RGB_TIME full_on, RGB_TIME full_off) {
    aw2023_set_rgb_led_flash(convert_color(color, BRIGHTNESS), full_on, full_off);
}

/**
 * @brief Sets RGB LED to breath
 * @param led_id  led index, should be 0 if there is only one LED
 * @param color  color enum
 * @param fade_on the fade on time in a breath period
 * @param full_on the on time in a breath period
 * @param fade_on the fade off time in a breath period
 * @param full_off the off time in a breath period
 * @return null
 */
void BSP_RGB_LED_Breath(uint8_t led_id, RGB_COLOR color, RGB_TIME fade_on, RGB_TIME full_on,
                        RGB_TIME fade_off, RGB_TIME full_off) {
    aw2023_set_rgb_led_breath(convert_color(color, BRIGHTNESS), fade_on, full_on, fade_off, full_off);
}
