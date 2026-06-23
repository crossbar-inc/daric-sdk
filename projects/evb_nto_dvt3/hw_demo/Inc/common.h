/**
 *****************************************************************************
 * @file    common.h
 * @author  AP Team
 * @brief   Define some common data structure.
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
#include "tx_api.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "lcd_common.h"
#include "daric_gui_resources.h"
#ifndef __COMMON_H
#define __COMMON_H

#ifndef __ECDH_PUBKEY
//#define __ECDH_PUBKEY
#endif
#ifndef __SESSION_SUPPORT
//#define __SESSION_SUPPORT
#endif

#define LOGIN_WIN_ID_TIMER 1
#define SET_PIN_CODE_WIN_ID_TIMER 2
/*
 * Wrapper for all messages, construct from the transport layer
 */

typedef struct SESSION_INFO
{
    UCHAR session_id[8];
    ULONG session_starttime;
    bool isDataService;
} SESSION_INFO;

typedef enum trans_method
{
    NONE_TRAN,
    USB_TRAN,
    BLE_TRAN,
    NFC_TRAN
} trans_method;

typedef struct
{
    char *str;
    size_t length;
} StringData;

typedef struct
{
    uint8_t *bytes;
    size_t length;
} BytesData;

// 1.2 READ_DARIC_DEVICE_INFO_RSP
typedef struct {
    char *deviceID;
    char *modName;
    char *manufacturerName;
    char *sysLang;
    char *osVersion;
    char *buildTime;
    char *fwVersion;
    char *accessToken;
    uint32_t batteryLevel;
    uint32_t leftSpace;
} GetDeviceInfo;

// 1.3 SEND_DARIC_UPDATE_REQ
typedef struct {
    char *deviceID;
    char *modName;
    char *manufacturerName;    
    char *sysLang;
    char *osVersion;
    char *buildTime;
    char *fwVersion;
} UpdateVerReq;

typedef enum {
    JANUARY = 1,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER
} MONTH_E;

/* Define the display dimentions specific to this implemenation. */
#define GUIX_DISPLAY_WIDTH      480
#define GUIX_DISPLAY_HEIGHT     600
#define OUTPUT_LCD_WIDTH        GUIX_DISPLAY_HEIGHT
#define OUTPUT_LCD_HEIGHT       GUIX_DISPLAY_WIDTH
/* Define the buffer size on the canvas, 1bit per pixel */
#define GUIX_BUFFER_SIZE (GUIX_DISPLAY_WIDTH * GUIX_DISPLAY_HEIGHT / 2)

extern void switchLCDRefreshMode(LCD_Mode mode);
extern void init_refresh_screen_timer();
extern void activate_refresh_screen_timer();
extern void deactivate_refresh_screen_timer();
extern GX_WIDGET* get_current_window();
extern bool is_date_valid(int32_t year, MONTH_E month, int32_t day);
extern bool is_time_valid(int32_t hour, int32_t minute, int32_t second);
#endif