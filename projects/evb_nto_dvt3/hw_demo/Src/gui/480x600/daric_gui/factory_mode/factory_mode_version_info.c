/**
 ******************************************************************************
 * @file    factory_mode_version_info.c
 * @author  AP Team
 * @brief   Source file of show device version information.
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

UINT factory_mode_version_info_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
        case GX_EVENT_SHOW:
            switchLCDRefreshMode(GC16);
            current_screen = (GX_WIDGET*)window;
            break;
        case GX_SIGNAL(ID_OOBE_single_NAV_BACK, GX_EVENT_CLICKED):
            switch_window((GX_WIDGET *)&factory_mode_second_win, current_screen);
            break;
    }
    return gx_window_event_process(window, event_ptr);
}