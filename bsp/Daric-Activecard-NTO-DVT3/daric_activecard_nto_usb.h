/**
******************************************************************************
* @file    daric_activecard_nto_usb.h
* @author  PERIPHERIAL BSP Team
* @brief   This file contains the common defines and functions prototypes for
*          the daric_activecard_nto_usb.h driver.
******************************************************************************
* @attention
*
* © Copyright CrossBar, Inc. 2024.
* All rights reserved.
*
* All rights reserved.
*
* This software is the proprietary property of CrossBar, Inc. and is protected
* by copyright laws. Any unauthorized reproduction, distribution, or
* modification is strictly prohibited.
*
******************************************************************************
*/

#ifndef DARIC_ACTIVECARD_NTO_USB_H
#define DARIC_ACTIVECARD_NTO_USB_H

#include <stdint.h>

/**
 * @brief BSP USB Status Type enumeration.
 */
typedef enum {
    BSP_USB_INSERT, /*!< USB is inserted */
    BSP_USB_REMOVE, /*!< USB is removed */
} BSP_USB_STATUS;

/**
 * @brief The callback type to notify USB status. The parameter wil be BSP_USB_STATUS.
 */
typedef void (*USB_DETECT_CB)(int16_t status);

int16_t BSP_USB_Detect_Register(USB_DETECT_CB cb);
int16_t BSP_USB_Detect_UnRegister();
int16_t BSP_USB_GetStatus();

#endif
