/**
 ******************************************************************************
 * @file    daric_gui.c
 * @author  AP Team
 * @brief   This file is the enter of guix, handle home window event,
 *          init status bar,start gx_system and handle window change.
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
/* This is a program of the high-performance GUIX graphics framework. */

#include <stdio.h>
#include "gx_api.h"

#include "daric_gui_resources.h"
#include "daric_gui_specifications.h"
#include "daric_gui.h"

#include "daric_fingerprint.h"
#include <stdint.h>
#include "common.h"
#include "tx_log.h"
#include "daric_bt.h"
#undef LOG_TAG
#define LOG_TAG "DARIC_GUI"

GX_WIDGET *pMainScreen;
GX_WINDOW_ROOT *root;
uint8_t OOBE_SETUP_WIZARD_COM_STAT = 0;
GX_WIDGET *current_screen = (GX_WIDGET *)&factory_mode_first_win;

extern int32_t init_battery_service();
extern int32_t init_time_service(void);
extern void fingerprint_event_listener(FP_EVENT_T event, uint16_t fid);
extern VOID number_keyboard_layout_create();
/* Application entry.*/
VOID start_guix(VOID)
{
    gx_studio_named_widget_create("factory_mode_first_win", (GX_WIDGET *)root, (GX_WIDGET **)&pMainScreen);

    /* Factory mode screen */
    // gx_studio_named_widget_create("factory_mode_first_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_second_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_touchscreen_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_lcd_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_lcd_black_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_lcd_gray_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_lcd_white_win", GX_NULL, GX_NULL);

    gx_studio_named_widget_create("fm_lcd_gray_mm_test_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("fm_lcd_refresh_mm_win", GX_NULL, GX_NULL);

    gx_studio_named_widget_create("factory_mode_led_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_key_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_vibrator_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_fingerprint_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_charger_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_rtc_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_bluetooth_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_nfc_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_version_info_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_reboot_win", GX_NULL, GX_NULL);

    gx_studio_named_widget_create("message_dialog_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("factory_mode_storage_win", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("number_keyboard_win", GX_NULL, GX_NULL);

    number_keyboard_layout_create();
    /* Show the root window to make it and patients screen visible.  */
    gx_widget_show(root);
    /* start GUIX thread */
    gx_system_start();

    init_time_service();
    init_battery_service();

    uint16_t ret = BSP_FP_Init(0);
    BSP_FP_Reg(fingerprint_event_listener);
    LOGV("----BSP_FP_Init ,ret : %d", ret);

    init_refresh_screen_timer();
}

VOID popup_dialog_window(GX_WIDGET *cur_win, GX_WIDGET *dialog_win)
{
    LOGV("popup_dialog_window , cur_win : %s ",cur_win->gx_widget_name);
    GX_WIDGET *current_win = get_current_window();
    if (current_win != GX_NULL)
    {
        gx_widget_attach(current_win, dialog_win);
    }
    else
    {
        gx_widget_attach(cur_win, dialog_win);
    }
}

VOID hide_dialog_window(GX_WIDGET *cur_win)
{
    gx_widget_detach(cur_win);
    gx_widget_hide(cur_win);
}

VOID popup_keyboard_window(GX_WIDGET *cur_win, GX_WIDGET *keyboard_win)
{
    LOGV("popup_keyboard_window , cur_win : %s ",cur_win->gx_widget_name);
    gx_widget_attach(cur_win, keyboard_win);
}

VOID hide_keyboard_window(GX_WIDGET *cur_win)
{
    gx_widget_hide(cur_win);
}

static bool is_window_switching = false;
VOID switch_window(GX_WIDGET *new_win, GX_WIDGET *cur_win)
{
    if (is_window_switching)
    {
        return;
    }
    UINT status = 1;
    LOGV("switch_window new_win : %s, cur_win : %s",
         new_win->gx_widget_name, cur_win->gx_widget_name);
    if (new_win != cur_win)
    {
        is_window_switching = true;

        if (!new_win->gx_widget_parent)
        {
            status = gx_widget_attach(root, (GX_WIDGET *)new_win);
            LOGV("switch_window gx_widget_attach ,status : %d", status);
        }
        else
        {
            status = gx_widget_show((GX_WIDGET *)new_win);
            LOGV("switch_window gx_widget_show ,status : %d", status);
        }
        status = gx_widget_detach((GX_WIDGET *)cur_win);
        LOGV("switch_window gx_widget_detach ,status : %d, cur_win : %s", status, cur_win->gx_widget_name);

        current_screen = new_win;
        is_window_switching = false;
    }
}

UINT refresh_new_win_ui_full(GX_WIDGET *new_win)
{
    UINT status;
    status = gx_system_dirty_mark(new_win);
    status = gx_system_canvas_refresh();
    LOGV("refresh_new_win_ui_full status : %d", status);
    return status;
}

VOID send_gx_event_to_target(ULONG event_type, GX_WIDGET *target, int sender_id)
{
    GX_EVENT resource_event;
    resource_event.gx_event_target = target;
    resource_event.gx_event_type = event_type;
    resource_event.gx_event_sender = sender_id;
    gx_system_event_send(&resource_event);
}

UINT refresh_gui_partial_area(GX_VALUE left, GX_VALUE top,
                              GX_VALUE right, GX_VALUE bottom)
{
    UINT status;
    GX_RECTANGLE partial_area;
    gx_utility_rectangle_define(&partial_area, left, top, right, bottom);
    status = gx_system_dirty_partial_add(current_screen, &partial_area);
    LOGV("refresh_gui_partial_area status : %d", status);
    return status;
}
