/**
 ******************************************************************************
 * @file    message_dialog.h
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
#include "daric_gui.h"

#define MESSAGE_DIALOG_FACTORY_RESET_CONFIRMED_EVENT                   205
#define MESSAGE_DIALOG_FP_CALIBRATE_CONFIRMED_EVENT                    206
#define MESSAGE_DIALOG_FACTORY_MODE_SAVE_RESULT_CONFIRMED_EVENT        207
#define MESSAGE_DIALOG_FACTORY_MODE_SAVE_RESULT_CANCELED_EVENT         208
extern int target_event_type;
extern int target_cancel_event_type;
extern GX_CHAR dialog_title[20];
extern GX_CHAR dialog_msg[100];