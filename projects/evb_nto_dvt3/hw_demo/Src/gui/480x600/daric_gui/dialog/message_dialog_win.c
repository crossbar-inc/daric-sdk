/**
 ******************************************************************************
 * @file    message_dialog_win.c
 * @author  AP Team
 * @brief   This file provides a generic message prompt dialog.
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

#include "tx_log.h"
#include "message_dialog.h"
#include "common.h"

int target_event_type = -1;
int target_cancel_event_type = -1;
GX_CHAR dialog_title[20] = {""};
GX_CHAR dialog_msg[100] = {""};

VOID update_msg_ui()
{
    gx_prompt_text_set((GX_PROMPT *)&message_dialog_win.message_dialog_win_prompt_dialog_title, dialog_title);
    gx_multi_line_text_view_text_set((GX_MULTI_LINE_TEXT_VIEW *)&message_dialog_win.message_dialog_win_prompt_dialog_message, dialog_msg);
}

UINT message_dialog_event_process(GX_WINDOW *widget, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        update_msg_ui();
        switchLCDRefreshMode(GC16);
        break;
    case GX_EVENT_HIDE:
        switchLCDRefreshMode(GC16);
        break;
    case GX_EVENT_PEN_DOWN:
        switchLCDRefreshMode(A2);
        break;
    case GX_SIGNAL(ID_OK_DIALOG_BTN, GX_EVENT_CLICKED):
        if (target_event_type >= 0)
        {
            send_gx_event_to_target(target_event_type, widget->gx_widget_parent, 0);
            LOGV("message_dialog_event_process, OK send event %d", target_event_type);
        }
        else
        {
            LOGV("message_dialog_event_process, Invalid event type");
        }
        target_cancel_event_type = -1;
        target_event_type = -1;
       // hide_dialog_window((GX_WIDGET *)widget);
        break;
    case GX_SIGNAL(ID_CANCEL_DIALOG_BTN, GX_EVENT_CLICKED):
        if (target_cancel_event_type >= 0)
        {
            send_gx_event_to_target(target_cancel_event_type, widget->gx_widget_parent, 0);
            LOGV("message_dialog_event_process, cancel send event %d", target_cancel_event_type);
        }
        target_cancel_event_type = -1;
        target_event_type = -1;
        hide_dialog_window((GX_WIDGET *)widget);
        break;
    }
    return gx_window_event_process(widget, event_ptr);
}