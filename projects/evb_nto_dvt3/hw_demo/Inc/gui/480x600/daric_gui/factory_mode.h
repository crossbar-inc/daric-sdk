/**
 *****************************************************************************
 * @file    factory_mode.h
 * @author  AP Team
 * @brief   Header file of factory test.
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
#ifndef __HEADER_FACTORY_MODE__
#define __HEADER_FACTORY_MODE__
#include "gx_api.h"
#include "daric_gui.h"
#include "common.h"
#include "daric_gui_resources.h"
#include "daric_gui_specifications.h"
#include "fx_api.h"
#include "daric_filex_app.h"
#include "tx_log.h"

#undef LOG_TAG
#define LOG_TAG "FACTORY_MODE_TAG"

#define FACTORY_MODE_ITEM_COUNTS 11

#define SYSTEM_ENABLE_DEBUG_MODE_THRESHOLD 7
#define FACTORY_MODE_TIMER 3
#define FACTORY_MODE_MM2_TIMER 4
#define FACTORY_MODE_INVALID_ACTION_TIMER 5


#define MM_LCD_A2_REFRESH_MODE 1
#define MM_LCD_DU2_REFRESH_MODE 2
#define MM_LCD_GC16_REFRESH_MODE 3
#define MM_LCD_REFRESH_MODE_COMPLETE 4
#define LCD_MM_TEST_TIMER_ID 15

#define FACTORY_MODE_REFRESH_GET_RTC_PASS_TOAST_SENDER 0
#define FACTORY_MODE_REFRESH_GET_RTC_FAILED_TOAST_SENDER 1
#define FACTORY_MODE_REFRESH_SET_RTC_PASS_TOAST_SENDER 2
#define FACTORY_MODE_REFRESH_SET_RTC_FAILED_TOAST_SENDER 3
#define FACTORY_MODE_REFRESH_SET_RTC_INVALID_TOAST_SENDER 4
#define FACTORY_MODE_REFRESH_RTC_PASS_TOAST_SENDER 5
#define FACTORY_MODE_ENABLE_FACTORY_MODE_TOAST_SENDER 6
#define FACTORY_MODE_UPDATE_VIBRATOR_LEFT_COUNTS_SENDER 7
#define FACTORY_MODE_REBOOT_TEST_SENDER 8

#define FACTORY_MODE_LED_ON_DIM 1 //1 s
#define FACTORY_MODE_TEST_DELAY 1000 //1 s
#define FACTORY_MODE_VIBRATOR_TEST_DELAY 3000 //3 s
#define FACTORY_MODE_LINEAR_VIBRATOR_ON_DURATION 2000 //2 s
#define FACTORY_MODE_LINEAR_VIBRATOR_TEST_CYCLE 300000
#define FACTORY_MODE_NONLINEAR_VIBRATOR_ON_DURATION 1000 //1 s
#define FACTORY_MODE_NONLINEAR_VIBRATOR_TEST_CYCLE 70000 //1 s
#define FACTORY_MODE_TEST_LED 0
#define FACTORY_MODE_TEST_VIBRATOR 1
#define FACTORY_MODE_TEST_CHARGER 2
#define FACTORY_MODE_TEST_BLUETOOTH 4
#define FACTORY_MODE_TEST_RTC 8
#define FACTORY_MODE_WIN_NAME_PREFIX_LEN 8
#define FACTORY_MODE_WIN_NAME_PREFIX_FM_LEN 3
#define BLUETOOTH_NAME_MAX_LENGTH 29

typedef enum {
    FACTORY_MODE_TEST_STAGE_NONE = 0,
    FACTORY_MODE_TEST_STAGE_MM1,
    FACTORY_MODE_TEST_STAGE_MM2
} FACTORY_MODE_TEST_STAGE;

typedef enum {
    FACTORY_MODE_MAIN = 0,
    FACTORY_MODE_ITME_TOUCHSCREEN_TEST,
    FACTORY_MODE_ITME_LCD_TEST,
    FACTORY_MODE_ITME_LED_TEST,
    FACTORY_MODE_ITME_KEY_TEST,
    FACTORY_MODE_ITME_VIBRATOR_TEST,
    FACTORY_MODE_ITME_FINGERPRINT_TEST,
    FACTORY_MODE_ITME_CHARGER_TEST,
    FACTORY_MODE_ITME_RTC_TEST,
    FACTORY_MODE_ITME_BLUETOOTH_TEST,
    FACTORY_MODE_ITME_NFC_TEST,
    FACTORY_MODE_ITME_STORAGE_TEST,
    FACTORY_MODE_ITME_END
} FACTORY_MODE_ITME_E;

extern GX_WIDGET *g_factory_mode_current_win;
extern FX_MEDIA *g_factory_mode_test_result_disk;

void factory_mode_update_test_status(FACTORY_MODE_ITME_E factory_mode, uint32_t status, uint32_t battery_capacity);
void factory_mode_refresh_toast_ui(USHORT sender);
bool factory_mode_init_factory_result_file(void);
void factory_mode_refresh_toast_ui(USHORT sender);
bool factory_mode_is_running();
UINT factory_mode_get_factory_result_mm1();
UINT factory_mode_get_stress_test_status();
#endif