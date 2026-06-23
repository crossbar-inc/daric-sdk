/**
 *****************************************************************************
 * @file    daric_gui.h
 * @author  AP Team
 * @brief   Header file of daric_gui.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gx_api.h"
#include "stdint.h"
// #include "fx_api.h"

#include "daric_gui_resources.h"
#include "daric_gui_specifications.h"
#include <stdbool.h>

extern GX_WIDGET *current_screen;

extern uint8_t OOBE_SETUP_WIZARD_COM_STAT;
VOID switch_window(GX_WIDGET *new_win, GX_WIDGET *cur_win);

VOID popup_dialog_window(GX_WIDGET *cur_win, GX_WIDGET *dialog_win);
VOID hide_dialog_window(GX_WIDGET *cur_win);
VOID popup_keyboard_window(GX_WIDGET *cur_win, GX_WIDGET *keyboard_win);
VOID hide_keyboard_window(GX_WIDGET *cur_win);
UINT refresh_new_win_ui_full(GX_WIDGET *new_win);
VOID send_gx_event_to_target(ULONG event_type, GX_WIDGET *target, int sender_id);
UINT refresh_gui_partial_area(GX_VALUE left, GX_VALUE top, GX_VALUE right, GX_VALUE bottom);
