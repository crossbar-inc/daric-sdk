/**
 ******************************************************************************
 * @file    usb_service.h
 * @author  AP Team
 * @brief   This file mainly defines the management of USB service, 
 *          including USB initialization, USB plugging and unplugging, 
 *          CDC and HID initialization, etc.
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
#ifndef __USBSERVICE_H__
#define __USBSERVICE_H__

#include "tx_api.h"
#include <stdbool.h>

/**
 * @brief  get usb connect state
 * @param  void
 * @retval ture:usb is connect false:usb is removed
 */
bool is_usb_connected();

/**
 * @brief  Callback function for usb plug
 * @param status usb plug in or out
 * @retval void.
 */
void usb_detect_callback(int16_t status);

//void notify_usb_disconnected();

int usb_cdc_acm_write(void *handle, void *buf, size_t length);
#endif