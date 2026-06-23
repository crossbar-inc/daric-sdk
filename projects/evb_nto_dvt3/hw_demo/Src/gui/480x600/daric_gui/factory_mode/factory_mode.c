/**
 ******************************************************************************
 * @file    factory_mode.c
 * @author  AP Team
 * @brief   Source file of factory test mode.
 ******************************************************************************
 * @attention
 *
 * © Copyright CrossBar, Inc . 2024.
 *
 * All rights reserved.
 *
 * This software is the proprietary property of CrossBar, Inc. and is protected
 * by copyright laws. Any unauthorized reproduction, distribution, or
 * modification is strictly prohibited.
 *
 ******************************************************************************
 */
#include "factory_mode.h"
#include "rgb_led_service.h"
#include "message_dialog.h"
#include "daric_pm.h"
#include "common.h"
#include "battery_service.h"
#include "daric_activecard_nto_vibrator.h"
#include "bluetooth_uart.h"

#include "nfc_service.h"
#include "tx_low_power_user.h"
#include "daric_fingerprint.h"

#include "time_service.h"
#include "version.h"
#include "device_manager.h"
#ifndef CONFIG_BOARD_ACTIVECARD_NTO
#include "daric_board.h"
#endif
#include "tg28.h"
#include "daric_hal.h"
#include "daric_activecard_nto_usb.h"
#include "daric_errno.h"

static INT mm_lcd_refresh_mode = MM_LCD_A2_REFRESH_MODE;

const GX_CHAR *g_factory_mode_open_btn_str = "OPEN";
const GX_CHAR *g_factory_mode_close_btn_str = "CLOSE";
const GX_CHAR *g_factory_mode_enable_btn_str = "ENABLE";
const GX_CHAR *g_factory_mode_enabling_btn_str = "ENABLING";

const GX_CHAR *g_factory_mode_disable_btn_str = "DISABLE";
const GX_CHAR *g_factory_mode_disabling_btn_str = "DISABLING";

const GX_CHAR *g_factory_mode_start_btn_str = "START";
const GX_CHAR *g_factory_mode_stop_btn_str = "STOP";
const GX_CHAR *g_factory_mode_next_btn_str = "Next";
const GX_CHAR *g_factory_mode_finish_btn_str = "Finished";
const GX_CHAR *g_factory_mode_charging_str = "Charging";
const GX_CHAR *g_factory_mode_charged_str = "Full";
const GX_CHAR *g_factory_mode_uncharge_str = "Uncharge";
const GX_CHAR *g_factory_mode_enabled_state_str = "ENABLED";
const GX_CHAR *g_factory_mode_disabled_state_str = "DISABLED";
const GX_CHAR *g_factory_mode_not_test_str = "";
const GX_CHAR *g_factory_mode_test_pass_str = "✓";
const GX_CHAR *g_factory_mode_test_failed_str = "X";
const char *g_factory_mode_widow_name_prefix = "factory_";
const char *g_factory_mode_widow_name_prefix_fm = "fm_";

CHAR *buffer_hour;
CHAR *buffer_minute;
CHAR *buffer_year;
CHAR *buffer_month;
CHAR *buffer_day;
CHAR *buffer_weekday;

UINT buffer_size;
UINT content_size;

GX_CHAR g_factory_mode_title[20] = {""};
GX_CHAR g_factory_mode_des[100] = {""};
CHAR g_factory_mode_item_screen_info_string[20] = {'\0'};
CHAR g_factory_mode_item_charger_battery_level_string[20] = {'\0'};
CHAR g_factory_mode_bluetooth_name[20] = {'\0'};
CHAR g_factory_mode_bluetooth_address[30] = {'\0'};
GX_CHAR reram_free_str[15] = {'\0'};
GX_CHAR nand_flash_free_str[15] = {'\0'};
CHAR gSerialNumber[17] = {'\0'};

GX_WIDGET *g_factory_mode_previous_win = (GX_WIDGET *)&factory_mode_first_win;
GX_WIDGET *g_factory_mode_item_win[FACTORY_MODE_ITEM_COUNTS + 1] = {(GX_WIDGET *)&factory_mode_first_win,
                                                                    (GX_WIDGET *)&factory_mode_touchscreen_win, (GX_WIDGET *)&factory_mode_lcd_win,
                                                                    (GX_WIDGET *)&factory_mode_led_win, (GX_WIDGET *)&factory_mode_key_win,
                                                                    (GX_WIDGET *)&factory_mode_vibrator_win, (GX_WIDGET *)&factory_mode_fingerprint_win,
                                                                    (GX_WIDGET *)&factory_mode_charger_win, (GX_WIDGET *)&factory_mode_rtc_win,
                                                                    (GX_WIDGET *)&factory_mode_bluetooth_win, (GX_WIDGET *)&factory_mode_nfc_win,
                                                                    (GX_WIDGET *)&factory_mode_storage_win};
GX_PROMPT *g_factory_mode_test_result_view[FACTORY_MODE_ITEM_COUNTS + 1] = {GX_NULL,
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_touch_screen_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_lcd_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_led_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_key_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_vibrator_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_fingerprint_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_charger_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_rtc_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_bluetooth_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_nfc_test_status),
                                                                            (GX_PROMPT *)&(factory_mode_first_win.factory_mode_first_win_factory_mode_storage_test_status)};
FACTORY_MODE_ITME_E g_factory_mode_test_item = FACTORY_MODE_ITME_TOUCHSCREEN_TEST;

RGB_COLOR g_factory_mode_led_color = RGB_RED;

TX_TIMER g_factory_mode_test_timer;

bool g_factory_mode_is_testing = false;
bool g_factory_mode_key_pressed = false;
bool g_factory_mode_key_long_pressed = false;
bool g_factory_mode_test_result_key_enabled = false;
bool g_factory_mode_charger_battery_flip_cable = false;
bool g_factory_mode_charger_battery_wireless = false;

bool g_factory_mode_running = false;

bool is_in_invalid_click = false;

int32_t g_factory_mode_charger_battery_capacity = 0;
extern int32_t g_capacity;

GX_WIDGET *g_factory_mode_current_win = GX_NULL;

CHAR *buffer_second;
FX_MEDIA *g_factory_mode_test_result_disk = &gNandflashDisk;
extern bool is_factory_reset_request;
// extern GX_CONST GX_UBYTE main_display_STRING_487_English[];
extern GX_WIDGET * target_widget;
extern bool is_show_number_keyboard;

const char *g_factory_mode_test_result_folder = "factory";
const char *g_factory_mode_test_result_filename = "\\factory\\factory_test_result.bin";
const char *g_factory_mode_test_result_filename_mm2 = "\\factory\\factory_test_result_mm2.bin";

int32_t g_vibrate_duration_ms = FACTORY_MODE_LINEAR_VIBRATOR_ON_DURATION;
INT g_vibrate_test_cycle_all = FACTORY_MODE_LINEAR_VIBRATOR_TEST_CYCLE;
INT g_vibrate_test_cycle = 0;
bool g_is_linear_vibrator = true;
CHAR g_vibrate_test_left_str[30] = {'\0'};
CHAR gBootloaderVersion[BOOTLOADER_VERSION_LEN + 1] = {'\0'};
CHAR gBluetoothVersion[21] = {'\0'};
CHAR gHardwareVersion[5] = {'\0'};
// static int gGetBTInfoRetryCounts = 5;
extern CHAR gSerialNumber[17];
CHAR gFactorySerialNumber[15] = {'\0'};
const char *gBtNamePrefix = "SK"; //"SiliconKey";
char* g_nfc_test_wakelock_name = "nfc_test_wake_lock";
int g_acquire_wake_lock_counts = 0;

uint8_t factory_mode_charger_update_battery_level();
void factory_mode_test_timer_callback(ULONG arg);
UINT factory_mode_txt_input_event_process(GX_SINGLE_LINE_TEXT_INPUT *text_input, GX_EVENT *event_ptr);
int32_t factory_mode_rtc_update_date_and_time();
extern void refresh_error_toast_ui();
extern VOID settings_app_do_factory_reset();
extern bool factory_mode_reboot_enabled();
extern void factory_mode_reboot_update_window_info(bool enabled);
extern bool settigs_app_is_bluetooth_disabled();
VOID factory_mode_update_test_item_status_ui(FACTORY_MODE_ITME_E index, bool result);
VOID factory_mode_set_current_test_finished();
VOID factory_mode_set_numeric_prompt_str(GX_NUMERIC_PROMPT *numeric, INT left_counts);
VOID factory_mode_set_prompt_str(GX_PROMPT *prompt_view, INT left_counts);
VOID get_storage_status();
VOID check_usb_connection_status();
VOID set_default_factory_sn();

typedef struct
{
    uint32_t tested_item;
    uint32_t test_result;
} factory_mode_test_item_result_t;

factory_mode_test_item_result_t g_factory_mode_test_result;
const uint32_t g_factory_mode_test_result_size = sizeof(g_factory_mode_test_result);
factory_mode_test_item_result_t g_factory_mode_test_result_old;

factory_mode_test_item_result_t g_factory_mode_test_result_mm1;

extern FACTORY_MODE_TEST_STAGE current_test_stage;
uint8_t nfc_mode_index = 0;
// extern bool bt_enable_status;
// extern bool bt_connect_status;

/*! \brief Acquire a wake lock with the specified name
 *
 * @param lock_name Name of the wake lock to acquire
 */
void factory_mode_acquire_wake_lock(char *acquire_name)
{
    low_power_wake_lock(acquire_name);
    g_acquire_wake_lock_counts ++;
    LOGV("factory_mode_acquire_wake_lock, by: %s, locked: %d", acquire_name, g_acquire_wake_lock_counts);
}

/*! \brief Release a wake lock with the specified name
 *
 * @param lock_name Name of the wake lock to release
 */
void factory_mode_release_wake_lock(char *release_name)
{
    low_power_wake_unlock(release_name);
    g_acquire_wake_lock_counts --;
    LOGV("factory_mode_release_wake_lock, by: %s, left lock: %d", release_name, g_acquire_wake_lock_counts);
}

bool factory_mode_init_factory_result_file()
{
    bool file_exist = true;
    FX_FILE factory_mode_test_file;
    UINT factory_mode_test_file_status;
    ULONG bytes_read;
    g_factory_mode_test_result.test_result = 0;
    g_factory_mode_test_result.tested_item = 0;

    if (current_test_stage == FACTORY_MODE_TEST_STAGE_MM2)
    {
        factory_mode_test_file_status = fx_file_open(g_factory_mode_test_result_disk, &factory_mode_test_file,
                                                     (char *)g_factory_mode_test_result_filename_mm2, FX_OPEN_FOR_READ);
    }
    else
    {
        factory_mode_test_file_status = fx_file_open(g_factory_mode_test_result_disk, &factory_mode_test_file,
                                                     (char *)g_factory_mode_test_result_filename, FX_OPEN_FOR_READ);
    }

    LOGV("init factory result: %d", factory_mode_test_file_status);
    if (factory_mode_test_file_status != FX_SUCCESS)
    {
        if (factory_mode_test_file_status == FX_NOT_FOUND)
        {
            factory_mode_test_file_status = fx_directory_create(g_factory_mode_test_result_disk, (char *)g_factory_mode_test_result_folder);
            LOGV("init factory create folder result: %d", factory_mode_test_file_status);
            fx_media_flush(g_factory_mode_test_result_disk);
            if (factory_mode_test_file_status == FX_SUCCESS || factory_mode_test_file_status == FX_ALREADY_CREATED)
            {
                if (current_test_stage == FACTORY_MODE_TEST_STAGE_MM2)
                {
                    factory_mode_test_file_status = fx_file_create(g_factory_mode_test_result_disk, (char *)g_factory_mode_test_result_filename_mm2);
                }
                else
                {
                    factory_mode_test_file_status = fx_file_create(g_factory_mode_test_result_disk, (char *)g_factory_mode_test_result_filename);
                }
                fx_media_flush(g_factory_mode_test_result_disk);
                LOGV("init factory create result file result: %d", factory_mode_test_file_status);
            }
        }
        file_exist = false;
    }
    else
    {
        factory_mode_test_file_status = fx_file_seek(&factory_mode_test_file, 0);
        if (factory_mode_test_file_status == FX_SUCCESS)
        {
            factory_mode_test_file_status = fx_file_read(&factory_mode_test_file, (char *)&g_factory_mode_test_result, g_factory_mode_test_result_size, &bytes_read);
            LOGV("read factory mode test result: %d, tested item: %d", g_factory_mode_test_result.test_result, g_factory_mode_test_result.tested_item);
            g_factory_mode_test_result_old.test_result = g_factory_mode_test_result.test_result;
            g_factory_mode_test_result_old.tested_item = g_factory_mode_test_result.tested_item;
        }
        fx_file_close(&factory_mode_test_file);
        fx_media_flush(g_factory_mode_test_result_disk);
    }
    return file_exist;
}

UINT factory_mode_get_factory_result_mm1()
{
    FX_FILE factory_mode_test_file;
    ULONG bytes_read;
    UINT factory_mode_test_file_status = fx_file_open(g_factory_mode_test_result_disk, &factory_mode_test_file,
                                                      (char *)g_factory_mode_test_result_filename, FX_OPEN_FOR_READ);
    if (factory_mode_test_file_status == FX_SUCCESS)
    {
        // uint8_t buf[65] = {'\0'};
        factory_mode_test_file_status = fx_file_seek(&factory_mode_test_file, 0);
        if (factory_mode_test_file_status == FX_SUCCESS)
        {
            factory_mode_test_file_status = fx_file_read(&factory_mode_test_file, (char *)&g_factory_mode_test_result_mm1, g_factory_mode_test_result_size, &bytes_read);
            LOGV("read factory mode test result: %d, tested item: %d", g_factory_mode_test_result_mm1.test_result, g_factory_mode_test_result_mm1.tested_item);
        }
        fx_file_close(&factory_mode_test_file);
        fx_media_flush(g_factory_mode_test_result_disk);
    }
    return factory_mode_test_file_status;
}

UINT factory_mode_set_factory_result()
{
    FX_FILE factory_mode_test_file;
    UINT factory_mode_test_file_status;

    if (current_test_stage == FACTORY_MODE_TEST_STAGE_MM2)
    {
        factory_mode_test_file_status = fx_file_open(g_factory_mode_test_result_disk, &factory_mode_test_file,
                                                     (char *)g_factory_mode_test_result_filename_mm2, FX_OPEN_FOR_WRITE);
    }
    else
    {
        factory_mode_test_file_status = fx_file_open(g_factory_mode_test_result_disk, &factory_mode_test_file,
                                                     (char *)g_factory_mode_test_result_filename, FX_OPEN_FOR_WRITE);
    }

    if (factory_mode_test_file_status == FX_SUCCESS)
    {
        factory_mode_test_file_status = fx_file_seek(&factory_mode_test_file, 0);
        if (factory_mode_test_file_status == FX_SUCCESS)
        {
            factory_mode_test_file_status = fx_file_write(&factory_mode_test_file, (char *)&g_factory_mode_test_result, g_factory_mode_test_result_size);
        }
        if (current_test_stage != FACTORY_MODE_TEST_STAGE_MM2)
        {
            g_factory_mode_test_result_mm1.test_result = g_factory_mode_test_result.test_result;
            g_factory_mode_test_result_mm1.tested_item = g_factory_mode_test_result.tested_item;
        }
        fx_file_close(&factory_mode_test_file);
        fx_media_flush(g_factory_mode_test_result_disk);
    }
    return factory_mode_test_file_status;
}

UINT factory_mode_init_test_timer(int flag, int delay)
{
    UINT status = tx_timer_create(&g_factory_mode_test_timer, "factory_mode_test_timer",
                                  factory_mode_test_timer_callback,
                                  flag,
                                  delay,
                                  (ULONG)0,
                                  TX_NO_ACTIVATE);
    return status;
}

UINT factory_mode_activate_test_timer()
{
    UINT status = tx_timer_activate(&g_factory_mode_test_timer);
    LOGV("activate factory mode timer status: %d", status);
    return status;
}

UINT factory_mode_deactivate_test_timer()
{
    UINT status = tx_timer_delete(&g_factory_mode_test_timer);
    return status;
}

void factory_mode_test_timer_callback(ULONG arg)
{
    tx_timer_deactivate(&g_factory_mode_test_timer);
    switch (arg)
    {
    case FACTORY_MODE_TEST_LED:
        if (g_factory_mode_led_color == RGB_BLUE)
        {
            g_factory_mode_led_color = RGB_RED;
        }
        else
        {
            g_factory_mode_led_color++;
        }
        set_rgb_led_on(FRONT_RGB_LED_INDEX, g_factory_mode_led_color, FACTORY_MODE_LED_ON_DIM);
        tx_timer_change(&g_factory_mode_test_timer,
                        FACTORY_MODE_TEST_DELAY,
                        (ULONG)0);
        factory_mode_activate_test_timer();
        break;
    case FACTORY_MODE_TEST_VIBRATOR:
        // BSP_Vibrator_Short();
        LOGV("vibrate test cycle: %d", g_vibrate_test_cycle);
        if (g_vibrate_test_cycle < g_vibrate_test_cycle_all)
        {
            BSP_Vibrator_Long(g_vibrate_duration_ms);
            tx_timer_change(&g_factory_mode_test_timer,
                            FACTORY_MODE_VIBRATOR_TEST_DELAY,
                            (ULONG)0);
            factory_mode_activate_test_timer();
            g_vibrate_test_cycle++;
        }
        factory_mode_refresh_toast_ui(FACTORY_MODE_UPDATE_VIBRATOR_LEFT_COUNTS_SENDER);
        break;
    case FACTORY_MODE_TEST_CHARGER:
        if (g_factory_mode_charger_battery_capacity != g_capacity)
        {
            g_factory_mode_charger_battery_capacity = g_capacity;
            factory_mode_charger_update_battery_level();
        }
        tx_timer_change(&g_factory_mode_test_timer,
                        FACTORY_MODE_TEST_DELAY,
                        (ULONG)0);
        factory_mode_activate_test_timer();
        break;
    case FACTORY_MODE_TEST_BLUETOOTH:
        break;
    case FACTORY_MODE_TEST_RTC:
        break;
    }
}

VOID factory_mode_set_button_str(GX_TEXT_BUTTON *btn, const GX_CHAR *str)
{
    gx_text_button_text_set(btn, str);
}

VOID factory_mode_window_info(FACTORY_MODE_ITME_E item_index)
{
    switch (item_index)
    {
    case FACTORY_MODE_ITME_TOUCHSCREEN_TEST:
    case FACTORY_MODE_MAIN:
        strcpy(g_factory_mode_title, "Touch Test");
        strcpy(g_factory_mode_des, "Tap the point of left top, right top, center, left bottom and right bottom for testing.");
        break;
    case FACTORY_MODE_ITME_LCD_TEST:
        strcpy(g_factory_mode_title, "LCD Test");
        strcpy(g_factory_mode_des, "The test will show black, white and gray window, tap screen to switch window.");
        break;
    case FACTORY_MODE_ITME_LED_TEST:
        strcpy(g_factory_mode_title, "LED Test");
        strcpy(g_factory_mode_des, "Clicked the OPEN button to turn on LED or clicked the CLOSE button to turn off LED.");
        break;
    case FACTORY_MODE_ITME_KEY_TEST:
        strcpy(g_factory_mode_title, "Key Test");
        strcpy(g_factory_mode_des, "Clicked and Release the power key for testing.");
        break;
    case FACTORY_MODE_ITME_VIBRATOR_TEST:
        strcpy(g_factory_mode_title, "Vibrator Test");
        strcpy(g_factory_mode_des, "Clicked the ENABLE button to enable vibrator or clicked the DISABLE button to disable vibrator.");
        break;
    case FACTORY_MODE_ITME_FINGERPRINT_TEST:
        strcpy(g_factory_mode_title, "Fingerprint Test");
        strcpy(g_factory_mode_des, "Clicked the Calibrate button to do fingerprint calibration, then touch the fingerprint sensor.");
        break;
    case FACTORY_MODE_ITME_CHARGER_TEST:
        strcpy(g_factory_mode_title, "Charger Test");
        strcpy(g_factory_mode_des, "Please plug in the charger cable.");
        break;
    case FACTORY_MODE_ITME_RTC_TEST:
        strcpy(g_factory_mode_title, "RTC Test");
        strcpy(g_factory_mode_des, "");
        break;
    case FACTORY_MODE_ITME_BLUETOOTH_TEST:
        strcpy(g_factory_mode_title, "Bluetooth Test");
        strcpy(g_factory_mode_des, "Clicked the ENABLE button to enable bluetooth or clicked the DISABLE button to disable bluetooth.");
        break;
    case FACTORY_MODE_ITME_NFC_TEST:
        strcpy(g_factory_mode_title, "NFC Test");
        // strcpy(g_factory_mode_des, "Clicked the ENABLE button to enable NFC or clicked the DISABLE button to disable NFC.");
        strcpy(g_factory_mode_des, "Please click the 'ENABLE RW MODE' button to start the RW mode test");
        factory_mode_set_button_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), "ENABLE RW MODE");
        break;
    case FACTORY_MODE_ITME_STORAGE_TEST:
        strcpy(g_factory_mode_title, "Storage Test");
        strcpy(g_factory_mode_des, "");
        break;
    case FACTORY_MODE_ITME_END:
        strcpy(g_factory_mode_title, "");
        strcpy(g_factory_mode_des, "");
        break;
    default:
        LOGE("Factory mode item not support");
        break;
    }
}

VOID factory_mode_update_test_result(FACTORY_MODE_ITME_E index, bool test_pass)
{
    if (test_pass)
    {
        g_factory_mode_test_result.test_result |= (1 << (index - 1));
    }
    else
    {
        g_factory_mode_test_result.test_result &= ~(1 << (index - 1));
    }
    g_factory_mode_test_result.tested_item |= (1 << (index - 1));
    LOGV("Factroy mode test result: %d, tested item: %d",
         g_factory_mode_test_result.test_result, g_factory_mode_test_result.tested_item);
    // factory_mode_update_test_item_status_ui(index, test_pass);
}

VOID factory_mode_update_test_status_ui()
{
    if (g_factory_mode_test_result.tested_item != 0)
    {
        for (int i = 1; i < FACTORY_MODE_ITEM_COUNTS + 1; i++)
        {
            if ((g_factory_mode_test_result.tested_item & (1 << (i - 1))) != 0)
            {
                if ((g_factory_mode_test_result.test_result & (1 << (i - 1))) != 0)
                {
                    gx_prompt_text_set(g_factory_mode_test_result_view[i], g_factory_mode_test_pass_str);
                }
                else
                {
                    gx_prompt_text_set(g_factory_mode_test_result_view[i], g_factory_mode_test_failed_str);
                }
            }
            else
            {
                gx_prompt_text_set(g_factory_mode_test_result_view[i], g_factory_mode_not_test_str);
            }
        }
    }
    else
    {
        for (int i = 1; i < FACTORY_MODE_ITEM_COUNTS + 1; i++)
        {
            gx_prompt_text_set(g_factory_mode_test_result_view[i], g_factory_mode_not_test_str);
        }
    }
}

VOID factory_mode_update_test_item_status_ui(FACTORY_MODE_ITME_E index, bool result)
{
    if (result)
    {
        gx_prompt_text_set(g_factory_mode_test_result_view[index], g_factory_mode_test_pass_str);
    }
    else
    {
        gx_prompt_text_set(g_factory_mode_test_result_view[index], g_factory_mode_test_failed_str);
    }
}

VOID factory_mode_update_status_button_state(bool enabled)
{
    GX_TEXT_BUTTON *pass_btn = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_COMMON_PASS_BTN, GX_SEARCH_DEPTH_INFINITE, &pass_btn);
    if (pass_btn != GX_NULL)
    {
        if (enabled)
        {
            gx_widget_style_add(pass_btn, GX_STYLE_ENABLED);
        }
        else
        {
            gx_widget_style_remove(pass_btn, GX_STYLE_ENABLED);
        }
    }
}

VOID factory_mode_update_bt_status_button_state(bool enabled)
{
    /* GX_TEXT_BUTTON *enable_btn = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_BLUETOOTH_BTN, GX_SEARCH_DEPTH_INFINITE, &enable_btn);
    if (enable_btn != GX_NULL)
    {
        if (enabled)
        {
             gx_widget_show(enable_btn);
            //gx_widget_style_add(enable_btn, GX_STYLE_ENABLED);
        }
        else
        {
            gx_widget_hide(enable_btn);
           //gx_widget_style_remove(enable_btn, GX_STYLE_ENABLED);
        }
    } */
    GX_TEXT_BUTTON *pass_btn = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_COMMON_PASS_BTN, GX_SEARCH_DEPTH_INFINITE, &pass_btn);
    if (pass_btn != GX_NULL)
    {
        if (enabled)
        {
            gx_widget_show(pass_btn);
            // gx_widget_style_add(pass_btn, GX_STYLE_ENABLED);
        }
        else
        {
               gx_widget_hide(pass_btn);
            //gx_widget_style_remove(pass_btn, GX_STYLE_ENABLED);
        }
    }
    GX_TEXT_BUTTON *failed_btn = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_COMMON_FAILED_BTN, GX_SEARCH_DEPTH_INFINITE, &failed_btn);
    if (failed_btn != GX_NULL)
    {
        if (enabled)
        {
              gx_widget_show(failed_btn);
           // gx_widget_style_add(failed_btn, GX_STYLE_ENABLED);
        }
        else
        {
              gx_widget_hide(failed_btn);
            //gx_widget_style_remove(failed_btn, GX_STYLE_ENABLED);
        }
    }

    GX_TEXT_BUTTON *next_btn = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_COMMON_NEXT_BTN, GX_SEARCH_DEPTH_INFINITE, &next_btn);
    if (next_btn != GX_NULL)
    {
        if (enabled)
        {
            gx_widget_show(next_btn);
            // gx_widget_style_add(next_btn, GX_STYLE_ENABLED);
        }
        else
        {
            gx_widget_hide(next_btn);
            // gx_widget_style_remove(next_btn, GX_STYLE_ENABLED);
        }
    }

    GX_TEXT_BUTTON *back_btn = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_COMMON_BACK_BTN, GX_SEARCH_DEPTH_INFINITE, &back_btn);
    if (back_btn != GX_NULL)
    {
        if (enabled)
        {
            gx_widget_show(back_btn);
            //gx_widget_style_add(back_btn, GX_STYLE_ENABLED);
        }
        else
        {
            gx_widget_hide(back_btn);
            // gx_widget_style_remove(back_btn, GX_STYLE_ENABLED);
        }
    }

    is_in_invalid_click = true;
    if (enabled)
    {
        gx_system_timer_start((GX_WIDGET *)&factory_mode_bluetooth_win, FACTORY_MODE_INVALID_ACTION_TIMER, 25, 0);
    }
}

VOID factory_mode_update_button_str(uint32_t id, const GX_CHAR *str)
{
    GX_PROMPT *btn = GX_NULL;
    gx_widget_find(current_screen, id, GX_SEARCH_DEPTH_INFINITE, &btn);
    if (btn != GX_NULL)
    {
        gx_prompt_text_set(btn, str);
    }
}

const GX_CHAR *factory_mode_get_button_str_from_id(uint32_t id)
{
    GX_PROMPT *btn = GX_NULL;
    const GX_CHAR *str = GX_NULL;
    gx_widget_find(current_screen, id, GX_SEARCH_DEPTH_INFINITE, &btn);
    if (btn != GX_NULL)
    {
        gx_prompt_text_get(btn, &str);
    }
    return str;
}

VOID factory_mode_set_numeric_prompt_str(GX_NUMERIC_PROMPT *numeric, INT left_counts)
{
    if (left_counts <= 0)
    {
        left_counts = 0;
    }
    LOGV("factory_mode_set_numeric_prompt_str left counts: %d", left_counts);
    gx_numeric_prompt_value_set(numeric, left_counts);
    gx_system_dirty_mark(numeric);
    // GX_RECTANGLE partial_area;
    // gx_utility_rectangle_define(&partial_area, 209, 350, 330, 390);
    // gx_system_dirty_partial_add((GX_WIDGET *)&factory_mode_vibrator_win, &partial_area);
    // activate_refresh_screen_timer();
}

VOID factory_mode_set_prompt_str(GX_PROMPT *prompt_view, INT left_counts)
{
    if (left_counts <= 0)
    {
        left_counts = 0;
    }
    memset(g_vibrate_test_left_str, '\0', sizeof(g_vibrate_test_left_str));
    sprintf(g_vibrate_test_left_str, "%d", left_counts);
    LOGV("factory_mode_set_numeric_prompt_str left counts str: %s", g_vibrate_test_left_str);
    gx_prompt_text_set(prompt_view, g_vibrate_test_left_str);
}

const GX_CHAR *factory_mode_get_button_str(GX_TEXT_BUTTON *btn)
{
    const GX_CHAR *str;
    gx_text_button_text_get(btn, &str);
    return str;
}

VOID factory_mode_update_item_status_str(GX_PROMPT *view, const GX_CHAR *str)
{
    memset(g_factory_mode_item_screen_info_string, '\0', sizeof(g_factory_mode_item_screen_info_string));
    sprintf(g_factory_mode_item_screen_info_string, "Status: %s", str);
    gx_prompt_text_set(view, g_factory_mode_item_screen_info_string);
}

bool factory_mode_is_testing(GX_TEXT_BUTTON *btn, const GX_CHAR *btn_str)
{
    const GX_CHAR *str = factory_mode_get_button_str(btn);
    return (strcmp(btn_str, str) == 0) ? true : false;
}

VOID factory_mode_update_ui_info()
{
    GX_PROMPT *title = GX_NULL;
    GX_MULTI_LINE_TEXT_VIEW *description = GX_NULL;
    gx_widget_find(current_screen, ID_FACTORY_MODE_WIN_TITLE, GX_SEARCH_DEPTH_INFINITE, &title);
    if (title != GX_NULL)
    {
        gx_prompt_text_set(title, "");
        gx_prompt_text_set(title, g_factory_mode_title);
    }
    gx_widget_find(current_screen, ID_FACTORY_MODE_WIN_DESCRIPTION, GX_SEARCH_DEPTH_INFINITE, &description);
    if (description != GX_NULL)
    {
        gx_multi_line_text_view_text_set(description, "");
        gx_multi_line_text_view_text_set(description, g_factory_mode_des);
    }
}

VOID factory_mode_set_current_test_finished()
{
    LOGV("factory_mode_set_current_test_finished test item: %d, testing: %d", g_factory_mode_test_item, g_factory_mode_is_testing);
    if (g_factory_mode_test_item == FACTORY_MODE_ITME_LED_TEST && g_factory_mode_is_testing)
    {
        if (factory_mode_is_testing(&(factory_mode_led_win.factory_mode_led_win_factory_mode_led_btn), g_factory_mode_close_btn_str))
        {
            set_rgb_led_off(FRONT_RGB_LED_INDEX);
        }
        factory_mode_set_button_str(&(factory_mode_led_win.factory_mode_led_win_factory_mode_led_btn), g_factory_mode_open_btn_str);
        factory_mode_deactivate_test_timer();
    }
    else if (g_factory_mode_test_item == FACTORY_MODE_ITME_VIBRATOR_TEST && g_factory_mode_is_testing)
    {
        factory_mode_set_button_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_btn), g_factory_mode_enable_btn_str);
        factory_mode_deactivate_test_timer();
        g_vibrate_test_cycle = 0;
        activate_refresh_screen_timer();
    }
    else if (g_factory_mode_test_item == FACTORY_MODE_ITME_CHARGER_TEST && g_factory_mode_is_testing)
    {
        factory_mode_deactivate_test_timer();
    }
    else if (g_factory_mode_test_item == FACTORY_MODE_ITME_BLUETOOTH_TEST && g_factory_mode_is_testing)
    {
        if (factory_mode_is_testing(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_disable_btn_str))
        {
            disable_bluetooth();
            // bt_enable_status = false;
            // bt_connect_status = false;
        }

        factory_mode_set_button_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_enable_btn_str);
    }

    else if (g_factory_mode_test_item == FACTORY_MODE_ITME_NFC_TEST && g_factory_mode_is_testing)
    {
        if (get_current_nfc_mode() != 0 && factory_mode_is_testing(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), g_factory_mode_disable_btn_str))
        {
            disable_all_nfc_modes();
        }
        else
        {
            switch_to_nfc_mode();
            tx_thread_sleep(100);
            disable_all_nfc_modes();
        }
        factory_mode_set_button_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), g_factory_mode_enable_btn_str);
        factory_mode_release_wake_lock(g_nfc_test_wakelock_name);
    }

    else if (g_factory_mode_test_item == FACTORY_MODE_ITME_FINGERPRINT_TEST && g_factory_mode_is_testing)
    {
        if (factory_mode_is_testing(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_start_btn), g_factory_mode_stop_btn_str))
        {
            BSP_FP_DetectStop();
        }
        factory_mode_set_button_str(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_start_btn), g_factory_mode_start_btn_str);
    }
    else if (g_factory_mode_test_item == FACTORY_MODE_ITME_RTC_TEST && g_factory_mode_is_testing)
    {
        factory_mode_deactivate_test_timer();
    }
    g_factory_mode_is_testing = false;
    g_factory_mode_test_result_key_enabled = false;
    g_factory_mode_key_long_pressed = false;
    g_factory_mode_key_pressed = false;
    g_factory_mode_charger_battery_flip_cable = false;
    g_factory_mode_charger_battery_wireless = false;
}

uint8_t factory_mode_charger_update_battery_level()
{
    LOGV("factory_mode_charger_update_battery_level, g_capacity: %d", g_factory_mode_charger_battery_capacity);
    memset(g_factory_mode_item_charger_battery_level_string, '\0', sizeof(g_factory_mode_item_charger_battery_level_string));
    sprintf(g_factory_mode_item_charger_battery_level_string, "Level: %ld%%", g_factory_mode_charger_battery_capacity);
    gx_prompt_text_set(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_level), g_factory_mode_item_charger_battery_level_string);
    activate_refresh_screen_timer();
    return 0;
}

VOID factory_mode_update_charger_status()
{
    int32_t factory_mode_charge_status = get_battery_charge_status();
    if (factory_mode_charge_status == BSP_PM_CHARG_ST_CHGING)
    {
        TG28_dumpBatteryParam();
        TG28_dumpRegister();
        factory_mode_update_item_status_str(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_status), g_factory_mode_charging_str);
        check_usb_connection_status();
    }
    else if (factory_mode_charge_status == BSP_PM_CHARG_ST_DONE)
    {
        TG28_dumpBatteryParam();
        TG28_dumpRegister();
        factory_mode_update_item_status_str(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_status), g_factory_mode_charged_str);
        check_usb_connection_status();
    }
    else
    {
        TG28_dumpBatteryParam();
        TG28_dumpRegister();
        factory_mode_update_item_status_str(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_status), g_factory_mode_uncharge_str);
        strcpy(g_factory_mode_des, "Please plug in the charger cable.");
    }
    factory_mode_update_ui_info();
    g_factory_mode_charger_battery_capacity = g_capacity;
    factory_mode_charger_update_battery_level();
}

int32_t factory_mode_rtc_update_date_and_time()
{
    int32_t result = 0;
    gx_single_line_text_input_buffer_clear(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_year));
    gx_single_line_text_input_buffer_clear(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_month));
    gx_single_line_text_input_buffer_clear(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_day));
    gx_single_line_text_input_buffer_clear(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_hour));
    gx_single_line_text_input_buffer_clear(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_minute));
    gx_single_line_text_input_buffer_clear(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_second));
    refresh_new_win_ui_full(current_screen);

    int32_t get_time_result = -1;
    int32_t get_date_result = -1;
    char date_time_str[30] = {'\0'};
    char buffer[5];
    get_date_result = get_current_date(date_time_str);
    // handle year
    memset(buffer, '\0', 5);
    strncpy(buffer, date_time_str, 4);
    gx_single_line_text_input_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_year), buffer);

    // handle month
    memset(buffer, '\0', 5);
    strncpy(buffer, date_time_str + 5, 2);
    gx_single_line_text_input_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_month), buffer);

    // handle day
    memset(buffer, '\0', 5);
    strncpy(buffer, date_time_str + 8, 2);
    gx_single_line_text_input_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_day), buffer);

    memset(date_time_str, '\0', sizeof(date_time_str));
    get_time_result = get_current_time(date_time_str);
    LOGV("current time: %s", date_time_str);
    // handle hours
    memset(buffer, '\0', 5);
    strncpy(buffer, date_time_str, 2);
    gx_single_line_text_input_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_hour), buffer);

    // handle minutes
    memset(buffer, '\0', 5);
    strncpy(buffer, date_time_str + 3, 2);
    gx_single_line_text_input_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_minute), buffer);

    // handle seconds
    memset(buffer, '\0', 5);
    strncpy(buffer, date_time_str + 6, 2);
    gx_single_line_text_input_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_second), buffer);

    LOGV("Fatory mode set date result: %d, set time result: %d", get_date_result, get_time_result);
    if (get_time_result != BSP_ERROR_NONE || get_date_result != BSP_ERROR_NONE)
    {
        result = -1;
    }
    return result;
}

VOID factory_mode_confirm_save_test_result()
{
    GX_WIDGET *dialog_win = (GX_WIDGET *)&message_dialog_win;
    strcpy(dialog_title, "Save result?");
    strcpy(dialog_msg, "Saved the test result to file.");
    target_event_type = MESSAGE_DIALOG_FACTORY_MODE_SAVE_RESULT_CONFIRMED_EVENT;
    target_cancel_event_type = MESSAGE_DIALOG_FACTORY_MODE_SAVE_RESULT_CANCELED_EVENT;
    popup_dialog_window(current_screen, dialog_win);
}

void factory_mode_refresh_toast_ui(USHORT sender)
{
    GX_EVENT resource_event;

    resource_event.gx_event_target = current_screen;
    resource_event.gx_event_type = GX_EVENT_RESOURCE_CHANGE;
    resource_event.gx_event_sender = sender;
    gx_system_event_send(&resource_event);
}

UINT factory_mode_get_bluetooth_fw_version()
{
    UINT ret = RESULT_SUCCESS;
    INT length = strlen("Not Avialable");
    INT bleFwVerLen = strlen(gBluetoothVersion);
    if (bleFwVerLen == 0 || (strcmp(gBluetoothVersion, "Not Avialable") == 0))
    {

        if(is_ble_enabled())
        {
            ret = get_ble_firmware_name(gBluetoothVersion);
            if (ret != RESULT_SUCCESS)
            {
                memcpy(gBluetoothVersion, "Not Avialable", length);
            }
        }
        else
        {
            memcpy(gBluetoothVersion, "Not Avialable", length);
        }

    }

    return ret;
}

VOID factory_mode_init_hardware_version()
{
    // if (strlen(gHardwareVersion) == 0)
    // {
    //     uint8_t hardwareId = HAL_GetHwId();
    //     switch (hardwareId)
    //     {
    //     case HAL_BOARD_PRE_DVT2: // DVT
    //         sprintf(gHardwareVersion, "%s", "PRE");
    //         break;
    //     case HAL_BOARD_DVT2: // DVT2
    //         sprintf(gHardwareVersion, "%s", "DVT2");
    //         break;
    //     case HAL_BOARD_DVT3: // DVT3
    //         sprintf(gHardwareVersion, "%s", "DVT3");
    //         break;
    //     case HAL_BOARD_PVT: // PVT
    //         sprintf(gHardwareVersion, "%s", "PVT");
    //         break;
    //     default:
    //         LOGE("Invalid hardware ID");
    //     }
    // }
    sprintf(gHardwareVersion, "%s", "DVT3");
}

UINT factory_mode_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        switchLCDRefreshMode(GC16);

        gx_widget_delete(&factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left);
        gx_widget_delete(&factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left_counts);
        gx_widget_delete(&factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_linear_vibrator_test);
        gx_widget_delete(&factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left_title);

        memset(g_factory_mode_title, '\0', sizeof(g_factory_mode_title));
        memset(g_factory_mode_des, '\0', sizeof(g_factory_mode_des));
        current_screen = (GX_WIDGET *)window;
        g_factory_mode_previous_win = (GX_WIDGET *)&factory_mode_first_win;
        g_factory_mode_test_result_key_enabled = false;

        factory_mode_update_test_status_ui();

        g_factory_mode_running = true;
        break;
    case GX_EVENT_HIDE:
        g_factory_mode_running = false;
        break;
    case GX_EVENT_PEN_DOWN:
        switchLCDRefreshMode(DU2);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCH_SCREEN, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_TOUCHSCREEN_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_ITME_TOUCHSCREEN_TEST];
        switch_window(g_factory_mode_item_win[g_factory_mode_test_item], current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_LCD, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_LCD_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window(g_factory_mode_item_win[g_factory_mode_test_item], current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_LED, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_LED_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_led_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_KEY, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_KEY_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_key_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_VIBRATOR, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_VIBRATOR_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_vibrator_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_FINGERPRINT, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_FINGERPRINT_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_fingerprint_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_CHARGER, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_CHARGER_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_charger_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_RTC, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_RTC_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_rtc_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_BLUETOOTH, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_BLUETOOTH_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_bluetooth_win, current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_NFC, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_NFC_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_nfc_win, current_screen);
        factory_mode_acquire_wake_lock(g_nfc_test_wakelock_name);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_STORAGE, GX_EVENT_CLICKED):
        g_factory_mode_test_item = FACTORY_MODE_ITME_STORAGE_TEST;
        factory_mode_window_info(g_factory_mode_test_item);
        g_factory_mode_previous_win = g_factory_mode_item_win[FACTORY_MODE_MAIN];
        switch_window((GX_WIDGET *)&factory_mode_storage_win, current_screen);
        break;
    // case GX_SIGNAL(ID_FACTORY_MODE_NFC_RW, GX_EVENT_CLICKED):
    //     switch_window((GX_WIDGET *)&factory_mode_nfc_rw_win, current_screen);
    //     break;
    // case GX_SIGNAL(ID_FACTORY_MODE_NFC_APPLET, GX_EVENT_CLICKED):
    //     switch_window((GX_WIDGET *)&factory_mode_nfc_applet_win, current_screen);
    //     break;
    // case GX_SIGNAL(ID_FACTORY_MODE_FACTORY_RESET, GX_EVENT_CLICKED):
    //     factory_mode_factory_reset();
    //     break;
    case GX_SIGNAL(ID_OOBE_single_NAV_BACK, GX_EVENT_CLICKED):
        switch_window((GX_WIDGET *)&factory_mode_first_win, current_screen);
        break;
    // case GX_SIGNAL(ID_OOBE_NAV_BACK, GX_EVENT_CLICKED):
    //     if (g_factory_mode_test_result_old.test_result != g_factory_mode_test_result.test_result || g_factory_mode_test_result_old.tested_item != g_factory_mode_test_result.tested_item)
    //     {
    //         factory_mode_confirm_save_test_result();
    //     }
    //     else
    //     {
    //         // switch_window((GX_WIDGET *)&about_settings_first_win, current_screen);
    //         switch_window((GX_WIDGET *)&factory_mode_home_win, current_screen);
    //     }
    //     break;
    case GX_SIGNAL(ID_OOBE_NAV_NEXT, GX_EVENT_CLICKED):
        switch_window((GX_WIDGET *)&factory_mode_second_win, current_screen);
        break;
//     case MESSAGE_DIALOG_FACTORY_MODE_SAVE_RESULT_CONFIRMED_EVENT:
//         factory_mode_set_factory_result();
//         hide_dialog_window((GX_WIDGET *)&message_dialog_win);
// #ifndef CONFIG_PCBA_SW
//         // switch_window((GX_WIDGET *)&about_settings_first_win, current_screen);
//         switch_window((GX_WIDGET *)&factory_mode_home_win, current_screen);
// #endif
//         break;
//     case MESSAGE_DIALOG_FACTORY_MODE_SAVE_RESULT_CANCELED_EVENT:
//         // switch_window((GX_WIDGET *)&about_settings_first_win, current_screen);
//         switch_window((GX_WIDGET *)&factory_mode_home_win, current_screen);
//         break;
//     case MESSAGE_DIALOG_FACTORY_RESET_CONFIRMED_EVENT:
// #ifndef CONFIG_PCBA_SW
//         is_factory_reset_request = true;
//         switch_window((GX_WIDGET *)&unlock_pin_window, current_screen);
// #else
//         settings_app_do_factory_reset();
// #endif
//         break;
    // case GX_SIGNAL(ID_FACTORY_MODE_REBOOT, GX_EVENT_CLICKED):
    //     reset();
        // factory_mode_reboot_update_window_info(factory_mode_reboot_enabled());
        // switch_window((GX_WIDGET *)&factory_mode_reboot_win, current_screen);
    case GX_SIGNAL(ID_FACTORY_MODE_VERSION_INFO, GX_EVENT_CLICKED):
        memset(gSerialNumber, '\0', 17);
        UINT ret = get_serial_number(gSerialNumber, 17);
        if (ret != 0)
        {
            LOGE("Failed to get serial number");
        }

        memset(gFactorySerialNumber, '\0', 15);
        UINT result = get_factory_sn_from_reram(gFactorySerialNumber, 15);
        if (result != 0)
        {
            set_default_factory_sn();
            LOGE("Failed to get factory serial number");
        }

        memset(gBootloaderVersion, '\0', BOOTLOADER_VERSION_LEN + 1);

        get_bootloader_version(gBootloaderVersion, BOOTLOADER_VERSION_LEN + 1);

        gx_prompt_text_set(&(factory_mode_version_info_win.factory_mode_version_info_win_version_info_build_number_summary), DARIC_ACTIVECARD_BUILD_NUMBER);
        gx_prompt_text_set(&(factory_mode_version_info_win.factory_mode_version_info_win_version_info_serial_number_summary), gSerialNumber);
        gx_prompt_text_set(&(factory_mode_version_info_win.factory_mode_version_info_win_version_info_factory_sn_summary), gFactorySerialNumber);
        gx_prompt_text_set(&(factory_mode_version_info_win.factory_mode_version_info_win_version_info_bootloader_version_summary), gBootloaderVersion);
        factory_mode_get_bluetooth_fw_version();
        LOGV("get ble fw version length: %d", strlen(gBluetoothVersion));
        gx_prompt_text_set(&(factory_mode_version_info_win.factory_mode_version_info_win_version_info_bt_firmware_version_summary), gBluetoothVersion);
        factory_mode_init_hardware_version();
        if (strlen(gHardwareVersion) > 0)
        {
            gx_prompt_text_set(&(factory_mode_version_info_win.factory_mode_version_info_win_version_info_hardware_version_summary), gHardwareVersion);
        }
        switch_window((GX_WIDGET *)&factory_mode_version_info_win, current_screen);
        break;
    }
    return gx_window_event_process(window, event_ptr);
}

void factory_mode_bluetooth_update_info()
{
    memset(g_factory_mode_bluetooth_name, '\0', sizeof(g_factory_mode_bluetooth_name));
    memset(g_factory_mode_bluetooth_address, '\0', sizeof(g_factory_mode_bluetooth_address));
    UCHAR bt_info[20] = {'\0'};
    int get_name_result = get_ble_name((CHAR *)bt_info);
    if (get_name_result == 0)
    {
        sprintf(g_factory_mode_bluetooth_name, "Name: %s", bt_info);
        gx_prompt_text_set(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_name), g_factory_mode_bluetooth_name);
    }
    memset(bt_info, '\0', sizeof(bt_info));
    int get_addr_result = get_ble_addr((CHAR *)bt_info);
    if (get_addr_result == 0)
    {
        sprintf(g_factory_mode_bluetooth_address, "Address: %s", bt_info);
        gx_prompt_text_set(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_address), g_factory_mode_bluetooth_address);
    }
}

void remove_colons_std(char *str)
{
    char *src = str;
    char *dst = str;
    while (*src)
    {
        if (*src != ':')
        {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
    LOGV("remove_colons_std, after str: %s", str);
}

UINT factory_mode_test_item_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        current_screen = (GX_WIDGET *)window;
        // g_factory_mode_is_testing = false;
        factory_mode_update_ui_info();
        if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_TOUCHSCREEN_TEST])
        {
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_left_top_btn);
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_right_top_btn);
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_right_center_btn);
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_left_center_btn);
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_center_btn);
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_left_bottom_btn);
            gx_widget_attach(current_screen, &factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_right_bottom_btn);
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_KEY_TEST])
        {
            gx_prompt_text_set(&(factory_mode_key_win.factory_mode_key_win_factory_mode_key_status), "Power Key Released");
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_CHARGER_TEST])
        {
            g_factory_mode_is_testing = true;
            factory_mode_update_charger_status();
            TG28_dumpBatteryParam();
            TG28_dumpRegister();
            // factory_mode_init_test_timer(FACTORY_MODE_TEST_CHARGER);
            // factory_mode_activate_test_timer();
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_BLUETOOTH_TEST] && !g_factory_mode_is_testing)
        {
            gx_prompt_text_set(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_name), "Name: NULL");
            gx_prompt_text_set(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_address), "Address: NULL");
            factory_mode_update_item_status_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_status), g_factory_mode_disabled_state_str);
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_NFC_TEST])
        {
            factory_mode_update_item_status_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_status), g_factory_mode_disabled_state_str);
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_FINGERPRINT_TEST])
        {
            gx_icon_pixelmap_set(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_detected_icon), GX_NULL, GX_NULL);
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_RTC_TEST])
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "Set RTC then Get RTC for testing.");
            factory_mode_rtc_update_date_and_time();
        }
        else if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_VIBRATOR_TEST])
        {
            gx_button_select((GX_BUTTON *)&factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_linear_vibrator_test);
            g_vibrate_duration_ms = FACTORY_MODE_LINEAR_VIBRATOR_ON_DURATION;
            g_vibrate_test_cycle_all = FACTORY_MODE_LINEAR_VIBRATOR_TEST_CYCLE;
            g_vibrate_test_cycle = 0;
            factory_mode_set_numeric_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left_counts), g_vibrate_test_cycle_all);
            factory_mode_set_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left), g_vibrate_test_cycle_all);
        }

        if (g_factory_mode_previous_win == (GX_WIDGET *)&factory_mode_first_win || current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_STORAGE_TEST])
        {
            factory_mode_update_button_str(ID_FACTORY_MODE_COMMON_NEXT_BTN, g_factory_mode_finish_btn_str);
        }
        else
        {
            factory_mode_update_button_str(ID_FACTORY_MODE_COMMON_NEXT_BTN, g_factory_mode_next_btn_str);
        }
        factory_mode_update_status_button_state(g_factory_mode_test_result_key_enabled);
        g_factory_mode_test_result_key_enabled = false;

        if (current_screen == g_factory_mode_item_win[FACTORY_MODE_ITME_STORAGE_TEST])
        {
            get_storage_status();
        }
        switchLCDRefreshMode(GC16);
        break;
    case GX_EVENT_PEN_DOWN:
        switchLCDRefreshMode(DU2);
        break;
    case GX_EVENT_PEN_UP:
        if (current_screen == (GX_WIDGET *)&factory_mode_lcd_win)
        {
            switchLCDRefreshMode(GC16);
            switch_window((GX_WIDGET *)&fm_lcd_gray_mm_test_win, current_screen);
            //activate_refresh_screen_timer();
        }
        /* else if (current_screen == (GX_WIDGET *)&factory_mode_lcd_white_win)
        {
            switchLCDRefreshMode(GC16);
            switch_window((GX_WIDGET *)&factory_mode_lcd_black_win, current_screen);
        }
        else if (current_screen == (GX_WIDGET *)&factory_mode_lcd_black_win)
        {
            switchLCDRefreshMode(GC16);
            switch_window((GX_WIDGET *)&factory_mode_lcd_gray_win, current_screen);
        }
        else if (current_screen == (GX_WIDGET *)&factory_mode_lcd_gray_win)
        {
            switchLCDRefreshMode(GC16);
            g_factory_mode_test_result_key_enabled = true;
            switch_window((GX_WIDGET *)&factory_mode_lcd_win, current_screen);
        } */
        break;
    case GX_EVENT_CLIENT_UPDATED:
        refresh_new_win_ui_full(current_screen);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_COMMON_BACK_BTN, GX_EVENT_CLICKED):
        if (is_in_invalid_click)
        {
            break;
        }
        factory_mode_set_current_test_finished();
        if (g_factory_mode_previous_win == (GX_WIDGET *)&factory_mode_first_win || g_factory_mode_test_item == FACTORY_MODE_ITME_STORAGE_TEST)
        {
            g_factory_mode_test_item = FACTORY_MODE_MAIN;
        }
        else if (g_factory_mode_previous_win != (GX_WIDGET *)&factory_mode_first_win && g_factory_mode_test_item > FACTORY_MODE_MAIN)
        {
            --g_factory_mode_test_item;
        }
        LOGV("Back to test item: %d", g_factory_mode_test_item);
        if (g_factory_mode_test_item >= FACTORY_MODE_MAIN)
        {
            factory_mode_window_info(g_factory_mode_test_item);
            switch_window(g_factory_mode_item_win[g_factory_mode_test_item], current_screen);
        }
        update_led();
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_COMMON_NEXT_BTN, GX_EVENT_CLICKED):
        if (is_in_invalid_click)
        {
            break;
        }
        factory_mode_set_current_test_finished();
        if (g_factory_mode_previous_win == (GX_WIDGET *)&factory_mode_first_win || g_factory_mode_test_item == FACTORY_MODE_ITME_STORAGE_TEST)
        {
            g_factory_mode_test_item = 0;
        }
        else if (g_factory_mode_previous_win != (GX_WIDGET *)&factory_mode_first_win && g_factory_mode_test_item < FACTORY_MODE_ITME_STORAGE_TEST)
        {
            ++g_factory_mode_test_item;
        }
        LOGV("Go to next test item: %d", g_factory_mode_test_item);
        if (g_factory_mode_test_item >= FACTORY_MODE_ITME_END)
        {
            g_factory_mode_test_item = 0;
            g_factory_mode_previous_win = g_factory_mode_item_win[0];
        }
        else
        {
            g_factory_mode_previous_win = g_factory_mode_item_win[g_factory_mode_test_item];
        }
        factory_mode_window_info(g_factory_mode_test_item);
        switch_window(g_factory_mode_item_win[g_factory_mode_test_item], current_screen);
        update_led();
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_COMMON_PASS_BTN, GX_EVENT_CLICKED):
        if (is_in_invalid_click)
        {
            break;
        }
        factory_mode_set_current_test_finished();
        factory_mode_update_test_result(g_factory_mode_test_item, true);
        if (g_factory_mode_previous_win == (GX_WIDGET *)&factory_mode_first_win || g_factory_mode_test_item == FACTORY_MODE_ITME_STORAGE_TEST)
        {
            g_factory_mode_test_item = 0;
        }
        else if (g_factory_mode_previous_win != (GX_WIDGET *)&factory_mode_first_win && g_factory_mode_test_item < FACTORY_MODE_ITME_STORAGE_TEST)
        {
            ++g_factory_mode_test_item;
        }
        LOGV("pass to next test item: %d", g_factory_mode_test_item);
        if (g_factory_mode_test_item >= FACTORY_MODE_ITME_END)
        {
            g_factory_mode_test_item = 0;
            g_factory_mode_previous_win = g_factory_mode_item_win[0];
        }
        else
        {
            g_factory_mode_previous_win = g_factory_mode_item_win[g_factory_mode_test_item];
        }
        factory_mode_window_info(g_factory_mode_test_item);
        switch_window(g_factory_mode_item_win[g_factory_mode_test_item], current_screen);
        update_led();
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_COMMON_FAILED_BTN, GX_EVENT_CLICKED):
        if (is_in_invalid_click)
        {
            break;
        }
        factory_mode_set_current_test_finished();
        factory_mode_update_test_result(g_factory_mode_test_item, false);
        g_factory_mode_test_item = FACTORY_MODE_MAIN;
        switch_window(g_factory_mode_item_win[g_factory_mode_test_item], current_screen);
        update_led();
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_LEFT_TOP_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_left_top_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_RIGHT_TOP_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_right_top_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_LEFT_CENTER_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_left_center_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_RIGHT_CENTER_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_right_center_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_CENTER_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_center_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_LEFT_BOTTOM_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_left_bottom_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_TOUCHSCREEN_RIGHT_BOTTOM_BTN, GX_EVENT_CLICKED):
        gx_widget_detach(&factory_mode_touchscreen_win.factory_mode_touchscreen_win_factory_mode_touchscreen_right_bottom_btn);
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_LED_BTN, GX_EVENT_CLICKED):
        g_factory_mode_is_testing = factory_mode_is_testing(&(factory_mode_led_win.factory_mode_led_win_factory_mode_led_btn), g_factory_mode_close_btn_str);
        if (g_factory_mode_is_testing)
        {
            factory_mode_deactivate_test_timer();
            factory_mode_set_button_str(&(factory_mode_led_win.factory_mode_led_win_factory_mode_led_btn), g_factory_mode_open_btn_str);
            set_rgb_led_off(FRONT_RGB_LED_INDEX);
            g_factory_mode_is_testing = false;
        }
        else
        {
            factory_mode_set_button_str(&(factory_mode_led_win.factory_mode_led_win_factory_mode_led_btn), g_factory_mode_close_btn_str);
            if (g_factory_mode_led_color > RGB_BLUE)
            {
                g_factory_mode_led_color = RGB_RED;
            }
            set_rgb_led_on(FRONT_RGB_LED_INDEX, g_factory_mode_led_color, FACTORY_MODE_LED_ON_DIM);
            factory_mode_init_test_timer(FACTORY_MODE_TEST_LED, FACTORY_MODE_TEST_DELAY);
            factory_mode_activate_test_timer();
            g_factory_mode_is_testing = true;
        }
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_VIBRATOR_BTN, GX_EVENT_CLICKED):
        g_factory_mode_is_testing = factory_mode_is_testing(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_btn), g_factory_mode_disable_btn_str);
        if (g_factory_mode_is_testing)
        {
            factory_mode_deactivate_test_timer();
            factory_mode_set_button_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_btn), g_factory_mode_enable_btn_str);
            g_factory_mode_is_testing = false;
        }
        else
        {
            factory_mode_set_button_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_btn), g_factory_mode_disable_btn_str);
            BSP_Vibrator_Long(g_vibrate_duration_ms);
            ++g_vibrate_test_cycle;
            LOGV("vibrate test cycle count: %d", g_vibrate_test_cycle);
            factory_mode_set_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left), g_vibrate_test_cycle_all - g_vibrate_test_cycle);
            // BSP_Vibrator_Short();
            factory_mode_init_test_timer(FACTORY_MODE_TEST_VIBRATOR, FACTORY_MODE_VIBRATOR_TEST_DELAY);
            factory_mode_activate_test_timer();
            g_factory_mode_is_testing = true;
        }
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_LINEAR_VIBRATOR_TEST, GX_EVENT_TOGGLE_ON):
        g_vibrate_duration_ms = FACTORY_MODE_LINEAR_VIBRATOR_ON_DURATION;
        g_vibrate_test_cycle_all = FACTORY_MODE_LINEAR_VIBRATOR_TEST_CYCLE;
        g_vibrate_test_cycle = 0;
        factory_mode_set_numeric_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left_counts), g_vibrate_test_cycle_all);
        factory_mode_set_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left), g_vibrate_test_cycle_all);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_LINEAR_VIBRATOR_TEST, GX_EVENT_TOGGLE_OFF):
        g_vibrate_duration_ms = FACTORY_MODE_NONLINEAR_VIBRATOR_ON_DURATION;
        g_vibrate_test_cycle_all = FACTORY_MODE_NONLINEAR_VIBRATOR_TEST_CYCLE;
        g_vibrate_test_cycle = 0;
        factory_mode_set_numeric_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left_counts), g_vibrate_test_cycle_all);
        factory_mode_set_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left), g_vibrate_test_cycle_all);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_BLUETOOTH_BTN, GX_EVENT_CLICKED):
        if (is_in_invalid_click)
        {
            break;
        }
        g_factory_mode_is_testing = factory_mode_is_testing(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_disable_btn_str);
        memset(g_factory_mode_item_screen_info_string, '\0', sizeof(g_factory_mode_item_screen_info_string));
        if (g_factory_mode_is_testing)
        {
            factory_mode_set_button_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_disabling_btn_str);
            factory_mode_update_bt_status_button_state(false);
            gx_system_timer_start((GX_WIDGET *)&factory_mode_bluetooth_win, FACTORY_MODE_MM2_TIMER, 25, 0);
        }
        else
        {
            factory_mode_set_button_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_enabling_btn_str);
            factory_mode_update_bt_status_button_state(false);
            gx_system_timer_start((GX_WIDGET *)&factory_mode_bluetooth_win, FACTORY_MODE_MM2_TIMER, 25, 0);
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_NFC_BTN, GX_EVENT_CLICKED):
        // g_factory_mode_is_testing = factory_mode_is_testing(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), g_factory_mode_disable_btn_str);
        LOGV("ID_FACTORY_MODE_NFC_BTN, g_factory_mode_is_testing:%d, nfc_mode_index: %d", g_factory_mode_is_testing, nfc_mode_index);
        if (g_factory_mode_is_testing && nfc_mode_index == 1)
        {
            if (switch_to_nfc_mode())
            {
                strcpy(g_factory_mode_title, "NFC Test");
                strcpy(g_factory_mode_des, "Please use an NFC reader to read AC. If successful, disable NFC and click Pass to the next stage.");
                factory_mode_set_button_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), g_factory_mode_disable_btn_str);
                factory_mode_update_item_status_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_status), "CARD MODE");
                nfc_mode_index = 2;
                factory_mode_update_ui_info();
            }
        }
        else if (g_factory_mode_is_testing && nfc_mode_index == 2)
        {
            if (disable_all_nfc_modes())
            {
                strcpy(g_factory_mode_title, "NFC Test");
                strcpy(g_factory_mode_des, "Please click the 'ENABLE RW MODE' button to start the RW mode test");
                factory_mode_set_button_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), "ENABLE RW MODE");
                factory_mode_update_item_status_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_status), g_factory_mode_disable_btn_str);
                // g_factory_mode_is_testing = false;
                nfc_mode_index = 0;
                factory_mode_update_ui_info();
            }
        }
        else
        {
            if (switch_to_nfc_rw_mode())
            {
                gx_system_timer_start((GX_WIDGET *)&factory_mode_nfc_win, FACTORY_MODE_MM2_TIMER, 10, 0);
                strcpy(g_factory_mode_title, "NFC Test");
                strcpy(g_factory_mode_des, "Please scan an NFC card for testing...");
                factory_mode_update_ui_info();
                factory_mode_update_item_status_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_status), "RW MODE");
                // tx_thread_sleep(50);
                nfc_mode_index = 1;
                g_factory_mode_is_testing = true;
            }
        }
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_FINGERPRINT_START_BTN, GX_EVENT_CLICKED):
        g_factory_mode_is_testing = factory_mode_is_testing(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_start_btn), g_factory_mode_stop_btn_str);
        if (g_factory_mode_is_testing)
        {
            BSP_FP_DetectStop();
            factory_mode_set_button_str(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_start_btn), g_factory_mode_start_btn_str);
            g_factory_mode_is_testing = false;
        }
        else
        {
            gx_icon_pixelmap_set(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_detected_icon), GX_NULL, GX_NULL);
            BSP_FP_DetectStart(false);
            factory_mode_set_button_str(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_start_btn), g_factory_mode_stop_btn_str);
            g_factory_mode_is_testing = true;
        }
        if (!g_factory_mode_test_result_key_enabled)
        {
            factory_mode_update_status_button_state(true);
            g_factory_mode_test_result_key_enabled = true;
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_FINGERPRINT_CALIBRATE_BTN, GX_EVENT_CLICKED):
        LOGV("ID_FACTORY_MODE_FINGERPRINT_CALIBRATE_BTN click---");
        strcpy(dialog_title, "FP Calibration");
        strcpy(dialog_msg, "During calibration, please keep the fingerprint sensor clean and do not block the sensor.");
        target_event_type = MESSAGE_DIALOG_FP_CALIBRATE_CONFIRMED_EVENT;
        popup_dialog_window(current_screen, (GX_WIDGET *)&message_dialog_win);
        break;
    case MESSAGE_DIALOG_FP_CALIBRATE_CONFIRMED_EVENT:
        BSP_FP_Calibrate();
        hide_dialog_window((GX_WIDGET *)&message_dialog_win);
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_RTC_SET_BTN, GX_EVENT_CLICKED):
        gx_system_timer_stop((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER);
        int32_t set_time_result = -1;
        int32_t set_date_result = -1;
        gx_single_line_text_input_buffer_get(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_hour), &buffer_hour,
                                             &content_size, &buffer_size);
        gx_single_line_text_input_buffer_get(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_minute), &buffer_minute,
                                             &content_size, &buffer_size);
        gx_single_line_text_input_buffer_get(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_second), &buffer_second,
                                             &content_size, &buffer_size);
        int hour = atoi(buffer_hour);
        int minute = atoi(buffer_minute);
        int second = atoi(buffer_second);
        LOGV("set_time_ok hour: %d, minute: %d ", hour, minute);
        if (is_time_valid(hour, minute, second))
        {
            set_time_result = set_time(hour, minute, second);
        }
        else
        {
            factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_SET_RTC_INVALID_TOAST_SENDER);
            return gx_window_event_process(window, event_ptr);
            ;
        }

        gx_single_line_text_input_buffer_get(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_year), &buffer_year,
                                             &content_size, &buffer_size);
        gx_single_line_text_input_buffer_get(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_month), &buffer_month,
                                             &content_size, &buffer_size);
        gx_single_line_text_input_buffer_get(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_input_day), &buffer_day,
                                             &content_size, &buffer_size);
        int year = atoi(buffer_year);
        int month = atoi(buffer_month);
        int day = atoi(buffer_day);
        if (is_date_valid(year, month, day))
        {
            set_date_result = set_date(year - 2000, month, day, 0);
        }
        else
        {
            factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_SET_RTC_INVALID_TOAST_SENDER);
            return gx_window_event_process(window, event_ptr);
            ;
        }

        LOGV("set_time result: %d, set date result: %d ", set_time_result, set_date_result);
        if (set_time_result == BSP_ERROR_NONE && set_date_result == BSP_ERROR_NONE)
        {
            factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_SET_RTC_PASS_TOAST_SENDER);
            g_factory_mode_is_testing = true;
        }
        else
        {
            factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_SET_RTC_FAILED_TOAST_SENDER);
        }
        break;
    case GX_SIGNAL(ID_FACTORY_MODE_RTC_GET_BTN, GX_EVENT_CLICKED):
    {
        gx_system_timer_stop((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER);
        if (factory_mode_rtc_update_date_and_time() == BSP_ERROR_NONE)
        {
            if (g_factory_mode_is_testing)
            {
                factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_RTC_PASS_TOAST_SENDER);
            }
            else
            {
                factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_GET_RTC_PASS_TOAST_SENDER);
            }
        }
        else
        {
            factory_mode_refresh_toast_ui(FACTORY_MODE_REFRESH_GET_RTC_FAILED_TOAST_SENDER);
        }
        break;
    }
    case GX_EVENT_RESOURCE_CHANGE:
        if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_SET_RTC_PASS_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "Set RTC Pass!");
            // 0: auto close timer and perform once only, 200 : 150 * 20(default frequency) = 3s init perform
            gx_system_timer_start((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER, 150, 0);
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_SET_RTC_FAILED_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "Set RTC Failed!");
            // 0: auto close timer and perform once only, 200 : 150 * 20(default frequency) = 3s init perform
            gx_system_timer_start((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER, 150, 0);
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_GET_RTC_PASS_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "Get RTC Pass!");
            // 0: auto close timer and perform once only, 200 : 150 * 20(default frequency) = 3s init perform
            gx_system_timer_start((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER, 150, 0);
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_GET_RTC_FAILED_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "Get RTC Failed!");
            // 0: auto close timer and perform once only, 200 : 150 * 20(default frequency) = 3s init perform
            gx_system_timer_start((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER, 150, 0);
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_SET_RTC_INVALID_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "Invalid input,try again!");
            // 0: auto close timer and perform once only, 200 : 150 * 20(default frequency) = 3s init perform
            gx_system_timer_start((GX_WIDGET *)&factory_mode_rtc_win, FACTORY_MODE_TIMER, 150, 0);
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_RTC_PASS_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "RTC Test Pass!");
            if (!g_factory_mode_test_result_key_enabled)
            {
                factory_mode_update_status_button_state(true);
                g_factory_mode_test_result_key_enabled = true;
            }
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_UPDATE_VIBRATOR_LEFT_COUNTS_SENDER)
        {
            switchLCDRefreshMode(GC16);
            factory_mode_set_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left),
                                        g_vibrate_test_cycle_all - g_vibrate_test_cycle);
            if (g_vibrate_test_cycle >= g_vibrate_test_cycle_all)
            {
                factory_mode_set_current_test_finished();
            }
        }
        if (event_ptr->gx_event_sender == FACTORY_MODE_REFRESH_RTC_PASS_TOAST_SENDER)
        {
            gx_prompt_text_set(&(factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "RTC Test Pass!");
            if (!g_factory_mode_test_result_key_enabled)
            {
                factory_mode_update_status_button_state(true);
                g_factory_mode_test_result_key_enabled = true;
            }
        }
        else if (event_ptr->gx_event_sender == FACTORY_MODE_UPDATE_VIBRATOR_LEFT_COUNTS_SENDER)
        {
            switchLCDRefreshMode(GC16);
            factory_mode_set_prompt_str(&(factory_mode_vibrator_win.factory_mode_vibrator_win_factory_mode_vibrator_test_left),
                                        g_vibrate_test_cycle_all - g_vibrate_test_cycle);
            if (g_vibrate_test_cycle >= g_vibrate_test_cycle_all)
            {
                factory_mode_set_current_test_finished();
            }
        }
        break;
    case GX_EVENT_TIMER:
        if (current_screen == (GX_WIDGET *)&factory_mode_nfc_win)
        {
            bool is_pass = wait_iso_dep_card();
            LOGV("GX_EVENT_TIMER, is_pass: %d", is_pass);
            if (is_pass)
            {
                strcpy(g_factory_mode_title, "NFC Test");
                strcpy(g_factory_mode_des, "RW mode test passed, cilck 'ENABLE CARD MODE' to start the card mode test");
                factory_mode_update_ui_info();
            }
            factory_mode_set_button_str(&(factory_mode_nfc_win.factory_mode_nfc_win_factory_mode_nfc_btn), "ENABLE CARD MODE");
            disable_all_nfc_modes();
        }
        if (current_screen == (GX_WIDGET *)&factory_mode_rtc_win)
        {
            gx_prompt_text_set(
                ((GX_PROMPT *)&factory_mode_rtc_win.factory_mode_rtc_win_factory_mode_rtc_set_error_prompt), "");
        }

        if (current_screen == (GX_WIDGET *)&factory_mode_bluetooth_win)
        {
            if (event_ptr->gx_event_payload.gx_event_timer_id == FACTORY_MODE_INVALID_ACTION_TIMER)
            {
                is_in_invalid_click = false;
            }
            else
            {
                if (g_factory_mode_is_testing)
                {
                    if (disable_bluetooth())
                    {
                        factory_mode_set_button_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_enable_btn_str);
                        factory_mode_update_item_status_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_status), g_factory_mode_disable_btn_str);
                        gx_prompt_text_set(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_name), "Name: NULL");
                        gx_prompt_text_set(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_address), "Address: NULL");

                        g_factory_mode_is_testing = false;
                        // bt_enable_status = false;
                        // bt_connect_status = false;
                    }
                }
                else
                {

                    if (enable_bluetooth())
                    {
                        factory_mode_set_button_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_btn), g_factory_mode_disable_btn_str);
                        factory_mode_update_item_status_str(&(factory_mode_bluetooth_win.factory_mode_bluetooth_win_factory_mode_bluetooth_status), g_factory_mode_enable_btn_str);
                        factory_mode_bluetooth_update_info();
                        g_factory_mode_is_testing = true;
                    }
                }
                if (!g_factory_mode_test_result_key_enabled)
                {
                    factory_mode_update_status_button_state(true);
                    g_factory_mode_test_result_key_enabled = true;
                }
                factory_mode_update_bt_status_button_state(true);
            }
        }
        break;
    }
    return gx_window_event_process(window, event_ptr);
}

UINT factory_mode_txt_input_event_process(GX_SINGLE_LINE_TEXT_INPUT *text_input, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        gx_single_line_text_input_style_remove(text_input, GX_STYLE_CURSOR_ALWAYS);
        break;
    case GX_EVENT_FOCUS_GAINED:
        if (target_widget == NULL || (strcmp(target_widget->gx_widget_name, text_input->gx_widget_name) != 0 || !is_show_number_keyboard))
        {
            if (target_widget != NULL)
            {
                gx_single_line_text_input_style_remove(target_widget, GX_STYLE_CURSOR_ALWAYS);
            }
            target_widget = (GX_WIDGET *)text_input;
            gx_single_line_text_input_style_add(text_input, GX_STYLE_CURSOR_ALWAYS);
            gx_single_line_text_input_style_remove(text_input, GX_STYLE_CURSOR_BLINK);
            popup_keyboard_window(current_screen, (GX_WIDGET *)&number_keyboard_win);
            is_show_number_keyboard = true;
        }
        break;
    case GX_EVENT_PEN_DOWN:
        gx_single_line_text_input_event_process(text_input, event_ptr);
        switchLCDRefreshMode(GC16);
        return GX_SUCCESS;
    }
    return gx_single_line_text_input_event_process(text_input, event_ptr);
}

void factory_mode_update_test_status(FACTORY_MODE_ITME_E factory_mode, uint32_t status, uint32_t battery_capacity)
{
    LOGV("update factory mode, factory mode: %d", factory_mode);
    switch (factory_mode)
    {
    case FACTORY_MODE_ITME_KEY_TEST:
        switchLCDRefreshMode(DU2);
        if (status == BSP_PM_EVT_PEK_SHORT_PRESS)
        {
            gx_prompt_text_set(&(factory_mode_key_win.factory_mode_key_win_factory_mode_key_status), "Power Key Is Pressed");
            g_factory_mode_key_pressed = true;
            if (g_factory_mode_is_testing && g_factory_mode_key_long_pressed)
            {
                strcpy(g_factory_mode_title, "Key Test");
                strcpy(g_factory_mode_des, "Key Test Pass.");
                factory_mode_update_status_button_state(true);
            }
            else
            {
                strcpy(g_factory_mode_title, "Key Test");
                strcpy(g_factory_mode_des, "Holding and Release the power key for testing.");
                g_factory_mode_is_testing = true;
            }
            factory_mode_update_ui_info();
        }
        else if (status == BSP_PM_EVT_PEK_LONG_PRESS)
        {
            gx_prompt_text_set(&(factory_mode_key_win.factory_mode_key_win_factory_mode_key_status), "Power Key Is Holding");
            g_factory_mode_key_long_pressed = true;
            if (g_factory_mode_is_testing && g_factory_mode_key_pressed)
            {
                strcpy(g_factory_mode_title, "Key Test");
                strcpy(g_factory_mode_des, "Key Test Pass.");
                factory_mode_update_status_button_state(true);
            }
            else
            {
                strcpy(g_factory_mode_title, "Key Test");
                strcpy(g_factory_mode_des, "Pressed and Release the power key for testing.");
                g_factory_mode_is_testing = true;
            }
            factory_mode_update_ui_info();
        }
        activate_refresh_screen_timer();
        break;
    case FACTORY_MODE_ITME_CHARGER_TEST:
        switchLCDRefreshMode(DU2);
        INT usbState = BSP_USB_GetStatus();
        // g_factory_mode_is_testing = true;
        if (status == BSP_PM_CHARG_ST_DONE)
        {
            factory_mode_update_item_status_str(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_status), g_factory_mode_charged_str);
            strcpy(g_factory_mode_title, "Charger Test");
            strcpy(g_factory_mode_des, "Please plug out the charger cable.");
            factory_mode_update_ui_info();
        }
        else if (status == BSP_PM_CHARG_ST_CHGING)
        {
            factory_mode_update_item_status_str(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_status), g_factory_mode_charging_str);
            strcpy(g_factory_mode_title, "Charger Test");
            if (g_factory_mode_is_testing)
            {
                if (g_factory_mode_charger_battery_wireless)
                {
                    strcpy(g_factory_mode_des, "Please remove it from the wireless charger.");
                }
                else
                {
                    check_usb_connection_status();
                }
            }
            factory_mode_update_ui_info();
        }
        else if (status == BSP_PM_CHARG_ST_NONE)
        {
            factory_mode_update_item_status_str(&(factory_mode_charger_win.factory_mode_charger_win_factory_mode_charger_status), g_factory_mode_uncharge_str);
            if (g_factory_mode_is_testing)
            {
                if (g_factory_mode_charger_battery_flip_cable && g_factory_mode_charger_battery_wireless)
                {
                    strcpy(g_factory_mode_title, "Charger Test");
                    strcpy(g_factory_mode_des, "Charger Test Pass");
                    factory_mode_update_status_button_state(true);
                }
                else if (g_factory_mode_charger_battery_flip_cable && !g_factory_mode_charger_battery_wireless)
                {
                    strcpy(g_factory_mode_des, "Please place it on the wireless charger for charging.");
                    g_factory_mode_charger_battery_wireless = true;
                }
                else
                {
                    if (usbState == BSP_USB_INSERT)
                    {
                        strcpy(g_factory_mode_des, "USB cable connection status error. Charger test fail.");
                        g_factory_mode_is_testing = false;
                        g_factory_mode_charger_battery_wireless = false;
                    }
                    else
                    {
                        strcpy(g_factory_mode_title, "Charger Test");
                        strcpy(g_factory_mode_des, "Please flip the charger cable and reinsert it.");
                        g_factory_mode_charger_battery_flip_cable = true;
                    }
                }
            }
            factory_mode_update_ui_info();
        }
        else if (status == BSP_PM_EVT_BAT_CAPACITY)
        {
            if (g_factory_mode_charger_battery_capacity != battery_capacity)
            {
                g_factory_mode_charger_battery_capacity = battery_capacity;
                factory_mode_charger_update_battery_level();
            }
            break;
        }
        activate_refresh_screen_timer();
        break;
    case FACTORY_MODE_ITME_FINGERPRINT_TEST:
        BSP_Vibrator_Short();
        gx_icon_pixelmap_set(&(factory_mode_fingerprint_win.factory_mode_fingerprint_win_factory_mode_fingerprint_detected_icon), GX_PIXELMAP_ID_FINGER_PRINT, GX_PIXELMAP_ID_FINGER_PRINT);
        activate_refresh_screen_timer();
        break;
    case FACTORY_MODE_MAIN:
    case FACTORY_MODE_ITME_TOUCHSCREEN_TEST:
    case FACTORY_MODE_ITME_LCD_TEST:
    case FACTORY_MODE_ITME_LED_TEST:
    case FACTORY_MODE_ITME_VIBRATOR_TEST:
    case FACTORY_MODE_ITME_RTC_TEST:
    case FACTORY_MODE_ITME_BLUETOOTH_TEST:
    case FACTORY_MODE_ITME_NFC_TEST:
    case FACTORY_MODE_ITME_STORAGE_TEST:
    case FACTORY_MODE_ITME_END:
        break;
    }
}

bool factory_mode_is_running()
{
    if (g_factory_mode_running == true)
    {
        return g_factory_mode_running;
    }
    g_factory_mode_current_win = get_current_window();
    if (g_factory_mode_current_win != GX_NULL)
    {
        GX_CONST GX_CHAR *widgetName = g_factory_mode_current_win->gx_widget_name;
        if (widgetName != GX_NULL)
        {
            char name_temp[FACTORY_MODE_WIN_NAME_PREFIX_LEN + 1] = {'\0'};
            char name_temp_fm[FACTORY_MODE_WIN_NAME_PREFIX_FM_LEN + 1] = {'\0'};
            memcpy(name_temp, widgetName, FACTORY_MODE_WIN_NAME_PREFIX_LEN);
            memcpy(name_temp_fm, widgetName, FACTORY_MODE_WIN_NAME_PREFIX_FM_LEN);
            if (strcmp(name_temp, g_factory_mode_widow_name_prefix) == 0 || strcmp(name_temp_fm, g_factory_mode_widow_name_prefix_fm) == 0)
            {
                g_factory_mode_running = true;
            }
        }
    }
    return g_factory_mode_running;
}

VOID get_storage_status()
{
    // Left space
    uint32_t reramLeftSpace;
    uint32_t nandFlashLeftSpace;
    UINT reram_status = fx_media_space_available(&gReramDisk, &reramLeftSpace);
    if (reram_status == FX_SUCCESS)
    {
        reramLeftSpace /= 1024;
        memset(reram_free_str, 0, sizeof(reram_free_str));
        sprintf(reram_free_str, "%ld KB Free", reramLeftSpace);
        LOGV("get_storage_status reram_free_str: %s", reram_free_str);
        gx_prompt_text_set((GX_PROMPT *)&factory_mode_storage_win.factory_mode_storage_win_reram_status_summary, reram_free_str);
    }
    else
    {
        gx_prompt_text_set((GX_PROMPT *)&factory_mode_storage_win.factory_mode_storage_win_reram_status_summary, "Not available");
    }

    UINT nand_status = fx_media_space_available(&gNandflashDisk, &nandFlashLeftSpace);
    if (nand_status == FX_SUCCESS)
    {
        nandFlashLeftSpace /= 1024;
        memset(nand_flash_free_str, 0, sizeof(nand_flash_free_str));
        sprintf(nand_flash_free_str, "%ld KB Free", nandFlashLeftSpace);
        LOGV("get_storage_status nand_flash_free_str: %s", nand_flash_free_str);
        gx_prompt_text_set((GX_PROMPT *)&factory_mode_storage_win.factory_mode_storage_win_nand_flash_status_summary, nand_flash_free_str);
    }
    else
    {
        gx_prompt_text_set((GX_PROMPT *)&factory_mode_storage_win.factory_mode_storage_win_nand_flash_status_summary, "Not available");
    }

    LOGV("get_storage_status reram_status: %d, nand_status: %d", reram_status, nand_status);

    if (reram_status == FX_SUCCESS && nand_status == FX_SUCCESS)
    {
        strcpy(g_factory_mode_title, "Storage Test");
        strcpy(g_factory_mode_des, "Storage Test Pass");
        factory_mode_update_status_button_state(true);
    }
    else
    {
        strcpy(g_factory_mode_title, "Storage Test");
        strcpy(g_factory_mode_des, "Storage Test Fail");
        factory_mode_update_status_button_state(false);
    }
    factory_mode_update_ui_info();
}

bool is_all_mm1_test_pass()
{
    // There are 11 test items mapped to bit0 ~ bit10.
    // All tests pass when these 11 bits are all set to 1 (all pass: 0x7FF).
    uint32_t all_pass_bits = (1U << 11) - 1;

    // Mask the lower 11 bits and check if all are 1
    if ((g_factory_mode_test_result_mm1.test_result & all_pass_bits) == all_pass_bits)
    {
        return true;
    }
    else
    {
        return false;
    }
}

UINT fm_lcd_mm_test_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    LOGD("fm_lcd_mm_test_event_handler,event_ptr->gx_event_type %d, window->gx_widget_id : %d,window->gx_widget_name : %s", event_ptr->gx_event_type, window->gx_widget_id, window->gx_widget_name);

    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        current_screen = (GX_WIDGET *)window;
        switchLCDRefreshMode(GC16);
        if (window->gx_widget_id == ID_FM_LCD_REFRESH_MM_WIN)
        {
            gx_system_timer_start(window, LCD_MM_TEST_TIMER_ID, 100, 0);
        }
        g_factory_mode_running = true;
        break;
    case GX_EVENT_HIDE:
        g_factory_mode_running = false;
        break;
    case GX_EVENT_TIMER:
        if (window->gx_widget_id == ID_FM_LCD_REFRESH_MM_WIN)
        {
            if (mm_lcd_refresh_mode == MM_LCD_A2_REFRESH_MODE)
            {
                mm_lcd_refresh_mode = MM_LCD_DU2_REFRESH_MODE;
                switchLCDRefreshMode(A2);
                gx_prompt_text_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_a2_refresh, GX_COLOR_ID_GRAY_0, GX_COLOR_ID_GRAY_0, GX_COLOR_ID_GRAY_0);
                gx_widget_fill_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_a2_refresh, GX_COLOR_ID_GRAY_F, GX_COLOR_ID_GRAY_F, GX_COLOR_ID_GRAY_F);
                gx_system_timer_start(window, LCD_MM_TEST_TIMER_ID, 100, 0);
            }
            else if (mm_lcd_refresh_mode == MM_LCD_DU2_REFRESH_MODE)
            {
                mm_lcd_refresh_mode = MM_LCD_GC16_REFRESH_MODE;
                switchLCDRefreshMode(DU2);
                gx_prompt_text_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_du2_refresh, GX_COLOR_ID_GRAY_0, GX_COLOR_ID_GRAY_0, GX_COLOR_ID_GRAY_0);
                gx_widget_fill_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_du2_refresh, GX_COLOR_ID_GRAY_F, GX_COLOR_ID_GRAY_F, GX_COLOR_ID_GRAY_F);
                gx_system_timer_start(window, LCD_MM_TEST_TIMER_ID, 100, 0);
            }
            else if (mm_lcd_refresh_mode == MM_LCD_GC16_REFRESH_MODE)
            {
                mm_lcd_refresh_mode = MM_LCD_REFRESH_MODE_COMPLETE;
                switchLCDRefreshMode(GC16);
                gx_prompt_text_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_gc16_refresh, GX_COLOR_ID_GRAY_D, GX_COLOR_ID_GRAY_D, GX_COLOR_ID_GRAY_D);
                gx_widget_fill_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_gc16_refresh, GX_COLOR_ID_GRAY_3, GX_COLOR_ID_GRAY_3, GX_COLOR_ID_GRAY_3);
                gx_system_timer_start(window, LCD_MM_TEST_TIMER_ID, 100, 0);
            }
            else if (mm_lcd_refresh_mode == MM_LCD_REFRESH_MODE_COMPLETE)
            {
                g_factory_mode_test_result_key_enabled = true;
                switch_window((GX_WIDGET *)&factory_mode_lcd_win, current_screen);
                mm_lcd_refresh_mode = MM_LCD_A2_REFRESH_MODE;
                gx_prompt_text_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_a2_refresh, GX_COLOR_ID_GRAY_F, GX_COLOR_ID_GRAY_F, GX_COLOR_ID_GRAY_F);
                gx_widget_fill_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_a2_refresh, GX_COLOR_ID_GRAY_0, GX_COLOR_ID_GRAY_0, GX_COLOR_ID_GRAY_0);
                gx_prompt_text_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_du2_refresh, GX_COLOR_ID_GRAY_C, GX_COLOR_ID_GRAY_C, GX_COLOR_ID_GRAY_C);
                gx_widget_fill_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_du2_refresh, GX_COLOR_ID_GRAY_3, GX_COLOR_ID_GRAY_3, GX_COLOR_ID_GRAY_3);
                gx_prompt_text_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_gc16_refresh, GX_COLOR_ID_GRAY_6, GX_COLOR_ID_GRAY_6, GX_COLOR_ID_GRAY_6);
                gx_widget_fill_color_set(&fm_lcd_refresh_mm_win.fm_lcd_refresh_mm_win_gc16_refresh, GX_COLOR_ID_GRAY_D, GX_COLOR_ID_GRAY_D, GX_COLOR_ID_GRAY_D);
            }
        }
        break;
    case GX_EVENT_PEN_UP:
        if (window->gx_widget_id == ID_FM_LCD_GRAY_MM_TEST_WIN)
        {
            switch_window((GX_WIDGET *)&fm_lcd_refresh_mm_win, current_screen);
        }
        break;
    }
    return gx_window_event_process(window, event_ptr);
}

// UINT factory_mode_txt_input_event_process(GX_SINGLE_LINE_TEXT_INPUT *text_input, GX_EVENT *event_ptr)
// {
//     return gx_single_line_text_input_event_process(text_input, event_ptr);
// }

UINT factory_mode_reboot_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    return gx_window_event_process(window, event_ptr);
}

VOID check_usb_connection_status()
{
    INT usbState = BSP_USB_GetStatus();
    if (usbState == BSP_USB_INSERT)
    {
        strcpy(g_factory_mode_des, "Please plug out the charger cable.");
    }
    else
    {
        strcpy(g_factory_mode_des, "USB cable connection status error. Charger test fail.");
        g_factory_mode_is_testing = false;
    }
}

VOID set_default_factory_sn()
{
    strncpy(gFactorySerialNumber, "DA000000000000", sizeof(gFactorySerialNumber));
    gFactorySerialNumber[14] = '\0';
}

void fingerprint_event_listener(FP_EVENT_T event, uint16_t fid)
{
    LOGV("fingerprint_event_listener , event: %d, fid: %d", event, fid);
    switch (event)
    {
    case FP_EVENT_DETECT_SUCCEED:
        if (current_screen == (GX_WIDGET*)&factory_mode_fingerprint_win)
        {
            factory_mode_update_test_status(FACTORY_MODE_ITME_FINGERPRINT_TEST, 0, 0);
            return;
        }
        break;
    case FP_EVENT_DETECT_FAILED:
        if (current_screen == (GX_WIDGET*)&factory_mode_fingerprint_win)
        {
            factory_mode_update_test_status(FACTORY_MODE_ITME_FINGERPRINT_TEST, 1, 0);
            return;
        }
        break;
    default:
        return;
    }
}
