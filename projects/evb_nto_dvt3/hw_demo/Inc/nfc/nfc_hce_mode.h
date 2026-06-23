/**
 ******************************************************************************
 * @file    nfc_hce_mode.h
 * @author  AP Team
 * @brief   This file is to start nfc hce mode.
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

#ifndef _NFC_HCE_MODE_H_
#define _NFC_HCE_MODE_H_

#include "port.h"
#include "nfc_stage.h"

#define READ_BINARY                   0xB0  //not use
#define RECEIVE_ANDROID_DATA          0xD1
#define RECEIVE_ANDROID_HEART_BEAT    0xD2
#define SEND_ANDROID_DATA             0xD3
#define GET_ANDROID_BT_ADDRESS        0xD4
#define GET_IOS_BT_ADDRESS            0xD5
#define UPDATE_BINARY                 0xD6  //not use
#define RECEIVE_IOS_HEART_BEAT        0xD7
#define SEND_IOS_DATA                 0xD8
#define RECEIVE_IOS_DATA              0xD9

uint8_t hce_callback(uint8_t *p_data, uint16_t data_length);

void start_hce_mode(void);

#endif
