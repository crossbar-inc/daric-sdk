/**
 *****************************************************************************
 * @file    rgb_led_service.h
 * @author  AP Team
 * @brief   Header file of rgb_led_service.
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
#include <stdbool.h>

#if defined(CONFIG_BOARD_EVB2)
#include "daric_evb2_rgb_led.h"
#define FULL_ON_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FULL_ON_TIME_DURATION_1 RGB_FADE_TIME_0260MS
#define FULL_ON_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#define FULL_OFF_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FULL_OFF_TIME_DURATION_1 RGB_FADE_TIME_0260MS
#define FULL_OFF_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#define FADE_ON_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FADE_ON_TIME_DURATION_1 RGB_FADE_TIME_0260MS
#define FADE_ON_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#define FADE_OFF_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FADE_OFF_TIME_DURATION_1 RGB_FADE_TIME_0630MS
#define FADE_OFF_TIME_DURATION_2 RGB_FADE_TIME_0380MS
#elif defined(CONFIG_BOARD_ACTIVECARD_NTO) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT2) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT3)
#include "daric_activecard_nto_rgb_led.h"
#define FULL_ON_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FULL_ON_TIME_DURATION_1 RGB_FADE_TIME_0260MS
#define FULL_ON_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#define FULL_OFF_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FULL_OFF_TIME_DURATION_1 RGB_FADE_TIME_0260MS
#define FULL_OFF_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#define FADE_ON_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FADE_ON_TIME_DURATION_1 RGB_FADE_TIME_0260MS
#define FADE_ON_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#define FADE_OFF_TIME_DURATION_0 RGB_FADE_TIME_0130MS
#define FADE_OFF_TIME_DURATION_1 RGB_FADE_TIME_0630MS
#define FADE_OFF_TIME_DURATION_2 RGB_FADE_TIME_0380MS

#endif

#define FRONT_RGB_LED_INDEX 1
#define BACK_RGB_LED_INDEX 0

#define RGB_LED_FULL_BRIGHTNESS 255
#define RGB_LED_HALF_BRIGHTNESS 127

typedef enum {
    LED_STATE_NONE = 0,       // free
    LED_STATE_BLE_CONNECTED,  // bt
    LED_STATE_FIDO_ACTIVE,    // FIDO
    LED_STATE_CHARGING,       // charge
} LedState;

extern bool rgb_ble_on;
extern bool rgb_fido_active;
extern bool rgb_charging;

void set_rgb_led_on(uint8_t led_id, RGB_COLOR color, uint8_t dim);
void set_rgb_led_off(uint8_t led_id);
void set_rgb_led_blink(uint8_t led_id, RGB_COLOR color, RGB_TIME full_on_time,
                          RGB_TIME full_off_time);
void set_rgb_led_breath(uint8_t led_id, RGB_COLOR color, RGB_TIME fade_on_time,
                           RGB_TIME full_on_time, RGB_TIME fade_off_time,
                           RGB_TIME full_off_time);      
void update_led(void);
