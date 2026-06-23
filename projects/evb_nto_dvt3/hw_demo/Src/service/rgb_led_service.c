/**
 ******************************************************************************
 * @file    rgb_led_service.c
 * @author  AP Team
 * @brief   This file mainly defines some rgb_led display interfaces.
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
#include "rgb_led_service.h"
// #include "status_bar.h"
#include "tx_log.h"

#undef LOG_TAG
#define LOG_TAG "RGB_SERV"

bool rgb_ble_on = false;
bool rgb_fido_active = false;
bool rgb_charging = false;
extern int32_t g_capacity;
/*! \brief Set RGB LED to always on
 *
 * @param led_id led index (FRONT_RGB_LED_INDEX:0 or BACK_RGB_LED_INDEX:1)
 * @param color colors enumation: RGB_COLOR
 * @param dim brightness of the color (range:0-255)
 * @return 0 failed, 1 success
 */
void set_rgb_led_on(uint8_t led_id, RGB_COLOR color, uint8_t dim)
{
    BSP_RGB_LED_On(led_id, color, dim);
}

/*! \brief Set RGB LED to always off
 *
 * @param led_id led index (FRONT_RGB_LED_INDEX:0 or BACK_RGB_LED_INDEX:1)
 * @return 0 failed, 1 success
 */
void set_rgb_led_off(uint8_t led_id)
{
    BSP_RGB_LED_Off(led_id);
}

/*! \brief Set RGB LED to blink
 *
 * @param led_id led index (FRONT_RGB_LED_INDEX:0 or BACK_RGB_LED_INDEX:1)
 * @param color colors enumation RGB_COLOR
 * @param full_on_time LED full on duration enumation: RGB_TIME
 * @param full_off_time LED full off duration enumation: RGB_TIME
 * @return 0 failed, 1 success
 */
void set_rgb_led_blink(uint8_t led_id, RGB_COLOR color, RGB_TIME full_on_time,
                          RGB_TIME full_off_time)
{
    BSP_RGB_LED_Blink(led_id, color, full_on_time, full_off_time);
}

/*! \brief Set RGB LED to breath
 *
 * @param led_id led index (FRONT_RGB_LED_INDEX:0 or BACK_RGB_LED_INDEX:1)
 * @param color colors enumation RGB_COLOR
 * @param fade_on_time LED fade on duration enumation: RGB_TIME
 * @param full_on_time LED full on duration enumation: RGB_TIME
 * @param fade_off_time LED fade off duration enumation: RGB_TIME
 * @param full_off_time LED full off duration enumation: RGB_TIME
 * @return 0 failed, 1 success
 */
void set_rgb_led_breath(uint8_t led_id, RGB_COLOR color, RGB_TIME fade_on_time,
                           RGB_TIME full_on_time, RGB_TIME fade_off_time,
                           RGB_TIME full_off_time)
{
    BSP_RGB_LED_Breath(led_id, color, fade_on_time, full_on_time, fade_off_time, full_off_time);
}

LedState get_current_led_state(void)
{
    LOGV("get_current_led_state, rgb_ble_on: %d, rgb_fido_active: %d, rgb_charging: %d", rgb_ble_on, rgb_fido_active, rgb_charging);
    if (rgb_ble_on) {
        return LED_STATE_BLE_CONNECTED;
    }
    if (rgb_fido_active) {
        return LED_STATE_FIDO_ACTIVE;
    }
    if (rgb_charging) {
        return LED_STATE_CHARGING;
    }
    return LED_STATE_NONE;
}

void update_led(void)
{
    LedState state = get_current_led_state();
    LOGV("update_led, current led state: %d", state);
    switch (state) {
        case LED_STATE_BLE_CONNECTED:
            set_rgb_led_blink(FRONT_RGB_LED_INDEX, RGB_BLUE, FULL_ON_TIME_DURATION_0, FULL_OFF_TIME_DURATION_0);
            break;

        case LED_STATE_FIDO_ACTIVE:
            set_rgb_led_on(FRONT_RGB_LED_INDEX, RGB_GREEN, RGB_LED_FULL_BRIGHTNESS);
            // set_rgb_led_off(FRONT_RGB_LED_INDEX);
            break;

        case LED_STATE_CHARGING:
            if (g_capacity == 100)
            {
                set_rgb_led_on(FRONT_RGB_LED_INDEX, RGB_GREEN, RGB_LED_FULL_BRIGHTNESS);
            }
            else
            {
                set_rgb_led_on(FRONT_RGB_LED_INDEX, RGB_RED, RGB_LED_FULL_BRIGHTNESS);
            }
            // set_rgb_led_off(FRONT_RGB_LED_INDEX);
            break;

        case LED_STATE_NONE:
        default:
            set_rgb_led_off(FRONT_RGB_LED_INDEX);
            break;
    }
}