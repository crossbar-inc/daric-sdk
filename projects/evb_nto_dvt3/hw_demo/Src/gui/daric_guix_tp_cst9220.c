/**
******************************************************************************
* @file    daric_guix_tp_cst9220.c
* @author  OS Team
* @brief   This file's contents are the adaptation of Guix for the cst9220 touchpanel.
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  
#include <string.h>
#include "tx_api.h"
#include "gx_api.h"
#include "gx_system.h"
#include "gx_display.h"
#include "battery_service.h"

#if defined(CONFIG_BOARD_ACTIVECARD_NTO) || defined(CONFIG_BOARD_ACTIVECARD_NTO_DVT2) || defined(CONFIG_BOARD_ACTIVECARD_NTO_DVT3)
#include "daric_activecard_nto_ts.h"
#elif defined(CONFIG_BOARD_EVB_NTO)
#include "daric_evb_nto_ts.h"
#endif
#include "time_service.h"
#include "user_threads_attrdef.h"
#include "tx_log.h"
#undef LOG_TAG
#define LOG_TAG "DARIC_GUIX_TP"

#define TOUCH_STATE_TOUCHED  1
#define TOUCH_STATE_RELEASED 2
#define MIN_DRAG_DELTA       10

static int last_pos_x;
static int last_pos_y;
static int curpos_x;
static int curpos_y;
static int touch_state;
static uint32_t gTpInstance;
// static TX_THREAD gTouchpThread;
// uint32_t last_touch_time_in_seconds = 0;

extern void bsp_pm_send_short_powerkey_event();
extern uint32_t screen_timeout_timer_repeat_count;

const uint16_t screen_width = 480;
const uint16_t screen_height = 600;
const uint16_t statusbar_height = 60;

/* Send a message to GUIX indicating that a finger pressed event. */
void SendPenDownEvent(void)
{
    GX_EVENT event;
    event.gx_event_type = GX_EVENT_PEN_DOWN;
    event.gx_event_payload.gx_event_pointdata.gx_point_x = curpos_x;
    event.gx_event_payload.gx_event_pointdata.gx_point_y = curpos_y;
    event.gx_event_sender = 0;
    event.gx_event_target = 0;
    event.gx_event_display_handle = 0;
    last_pos_x = curpos_x;
    last_pos_y = curpos_y;
    gx_system_event_send(&event);
}

/* Send a message to GUIX indicating that a finger drag event. */
void SendPenDragEvent(void)
{
    GX_EVENT event;
    int x_delta = abs(curpos_x - last_pos_x);
    int y_delta = abs(curpos_y - last_pos_y);

    if (x_delta > MIN_DRAG_DELTA || y_delta > MIN_DRAG_DELTA)
    {
        event.gx_event_type = GX_EVENT_PEN_DRAG;
        event.gx_event_payload.gx_event_pointdata.gx_point_x = curpos_x;
        event.gx_event_payload.gx_event_pointdata.gx_point_y = curpos_y;
        event.gx_event_sender = 0;
        event.gx_event_target = 0;
        event.gx_event_display_handle = 0;
        last_pos_x = curpos_x;
        last_pos_y = curpos_y;
        gx_system_event_fold(&event);
    }
}

/* Send a message to GUIX indicating that a finger release event. */
void SendPenUpEvent(void)
{
    GX_EVENT event;

    event.gx_event_type = GX_EVENT_PEN_UP;
    event.gx_event_payload.gx_event_pointdata.gx_point_x = curpos_x;
    event.gx_event_payload.gx_event_pointdata.gx_point_y = curpos_y;
    event.gx_event_sender = 0;
    event.gx_event_target = 0;
    event.gx_event_display_handle = 0;
    last_pos_x = curpos_x;
    last_pos_y = curpos_y;
    gx_system_event_send(&event);
}

/* Touch panel event handler */
void  touch_thread_entry(ULONG thread_input)
{
    UINT status;
    ULONG actual_flags;
    pos_info testinfo;
    touch_state = TOUCH_STATE_RELEASED;
    // bool isSended = false;

    /* Initial touch panel driver */
    BSP_TS_Init(gTpInstance);
    /* Enable touch panel interrupt */
    BSP_TS_Get_MultiTouchState(gTpInstance, &testinfo);

    while(1)
    {
        //tx_thread_sleep(300);
        status = tx_event_flags_get(&gTouchEventGroup, TOUCH_PANEL_EVENT, 
                                    TX_OR_CLEAR,
                                    &actual_flags, 
                                    TX_WAIT_FOREVER);

        if (status == TX_SUCCESS)
        {
            /*
             * When the screen is off, touch panel messages should no longer be sent.
             * In the screen-off state, only the first touch message should trigger
             * a short powerkey message to wake up the screen,
             * and all subsequent touch messages should be ignored.
             */

            if (!(testinfo.event))
            {
                // no touch, check so see if last was touched
                if (touch_state == TOUCH_STATE_TOUCHED)
                {
                    touch_state = TOUCH_STATE_RELEASED;
                    SendPenUpEvent();
                }
            }
            else
            {
                if (testinfo.pos_x > screen_width ||
                    testinfo.pos_y > screen_height)
                    continue;
                // screen is touched, update coords:
                curpos_x = testinfo.pos_x;

                curpos_y = testinfo.pos_y;

                if (touch_state == TOUCH_STATE_RELEASED)
                {
                    touch_state = TOUCH_STATE_TOUCHED;
                    SendPenDownEvent();
                }
                else
                {
                    // test and send pen drag
                    SendPenDragEvent();
                }
            }
        }
    }
}
