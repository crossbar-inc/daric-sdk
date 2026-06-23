/**
 ******************************************************************************
 * @file    number_keyboard.c
 * @author  AP Team
 * @brief   This file defines the drawing of number keyboard input and the handling of key events.
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
/* This is a small program of the high-performance GUIX graphics framework. */

// #include "keyboard.h"
#include "tx_log.h"
#include "daric_gui.h"

GX_WIDGET *target_widget = NULL;
bool is_show_number_keyboard = false;
/* declare a structure to define one number keyboard key - number_keyboard.c*/
#define NUMBER_KEYBOARD_KEYS_COUNT 12

typedef struct NUMBER_KEYBOARD_LAYOUT_STRUCT
{
    GX_VALUE xoffset;
    GX_VALUE yoffset;
    GX_VALUE width;
    GX_VALUE height;
    char *name;
    char *text;
    USHORT key_val;
} NUMBER_KEYBOARD_LAYOUT;

typedef struct NUMBER_KEY_WIDGET_STRUCT
{
    GX_TEXT_BUTTON_MEMBERS_DECLARE
    USHORT key_value;
} NUMBER_KEY_BUTTON;

static NUMBER_KEY_BUTTON number_keys_list[NUMBER_KEYBOARD_KEYS_COUNT];
static const NUMBER_KEYBOARD_LAYOUT number_keyboard_layout_list[NUMBER_KEYBOARD_KEYS_COUNT + 1] = {
    {15, 0, 105, 90, "number_key_1", "1", 0x31},
    {130, 0, 105, 90, "number_key_2", "2", 0x32},
    {245, 0, 105, 90, "number_key_3", "3", 0x33},
    {360, 0, 105, 90, "number_key_4", "4", 0x34},
    {15, 96, 105, 90, "number_key_5", "5", 0x35},
    {130, 96, 105, 90, "number_key_6", "6", 0x36},
    {245, 96, 105, 90, "number_key_7", "7", 0x37},
    {360, 96, 105, 90, "number_key_8", "8", 0x38},
    {15, 192, 105, 90, "number_key_9", "9", 0x39},
    {130, 192, 105, 90, "number_key_0", "0", 0x30},
    {245, 192, 105, 90, "number_key_del", "del", GX_KEY_BACKSPACE},
    {360, 192, 105, 90, "number_key_ok", "ok", GX_KEY_SELECT},
    {0, 0, 0, 0, 0, 0, 0},
};
extern const uint16_t screen_width;
extern const uint16_t screen_height;

static VOID number_key_select(GX_WIDGET *widget)
{
    NUMBER_KEY_BUTTON *key_widget = (NUMBER_KEY_BUTTON *)widget;
    if (key_widget->key_value == GX_KEY_SELECT)
    {
        hide_keyboard_window((GX_WIDGET *)&number_keyboard_win);
        is_show_number_keyboard = false;
        refresh_gui_partial_area(0, 0, screen_width, screen_height);
    }
    else
    {
        GX_EVENT key_event;
        key_event.gx_event_payload.gx_event_ushortdata[0] = key_widget->key_value;

        key_event.gx_event_sender = widget->gx_widget_id;
        key_event.gx_event_target = target_widget;
        key_event.gx_event_type = GX_EVENT_KEY_DOWN;
        gx_system_event_send(&key_event);
        key_event.gx_event_type = GX_EVENT_KEY_UP;
        gx_system_event_send(&key_event);
    }
}

static VOID number_widget_create(const NUMBER_KEYBOARD_LAYOUT *layout, GX_WIDGET *number_win, NUMBER_KEY_BUTTON *key_widget)
{
    GX_RECTANGLE size;
    INT left;
    INT top;
    GX_TEXT_BUTTON *txt_btn = (GX_TEXT_BUTTON *)key_widget;
    left = number_win->gx_widget_size.gx_rectangle_left + layout->xoffset;
    top = number_win->gx_widget_size.gx_rectangle_top + layout->yoffset;
    gx_utility_rectangle_define(&size, left, top, left + layout->width, top + layout->height);
    gx_text_button_create(txt_btn, layout->name, number_win, GX_ID_NONE,
                          GX_STYLE_BORDER_THIN | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, GX_ID_NONE, &size);

    gx_text_button_text_color_set(txt_btn, GX_COLOR_ID_TEXT, GX_COLOR_ID_TEXT, GX_COLOR_ID_TEXT);
    gx_text_button_text_set(txt_btn, layout->text);
    gx_text_button_font_set(txt_btn, GX_FONT_ID_TEXT_INPUT);
    key_widget->key_value = layout->key_val;
    key_widget->gx_button_select_handler = number_key_select;
}

static VOID number_keys_create(GX_WIDGET *number_win, const NUMBER_KEYBOARD_LAYOUT *layout_list)
{
    INT index = 0;
    while (layout_list[index].key_val)
    {
        number_widget_create(&layout_list[index], number_win,&number_keys_list[index]);
        index++;
    }
}

/*!   \brief  Initializes the numeric keypad for Settings such as date and time.
 *
 */
VOID number_keyboard_layout_create()
{
    number_keys_create((GX_WIDGET *)&number_keyboard_win, number_keyboard_layout_list);
}