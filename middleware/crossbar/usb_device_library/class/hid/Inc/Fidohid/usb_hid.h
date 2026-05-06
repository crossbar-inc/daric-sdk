/**
 ******************************************************************************
 * @file    usb_hid.h
 * @author  USB Team
 * @brief   Header file of USB HID module.
 ******************************************************************************
 * @attention
 *
 * Copyright 2024-2026 CrossBar, Inc.
 * This file has been modified by CrossBar, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_HID_H
#define __USB_HID_H

#ifdef __cplusplus
extern "C"
{
#endif
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "usb_def.h"
#define USB_HID_MAX_REPORT_SIZE 64
#define HID_PACKET_SIZE         USB_HID_MAX_REPORT_SIZE

// #define HID_EPIN_ADDR                   0x81U
#define HID_EPIN_SIZE HID_PACKET_SIZE
// #define HID_EPOUT_ADDR                  0x01U
#define HID_EPOUT_SIZE HID_PACKET_SIZE

#if 0
#define USB_HID_EP_IN  HID_EPIN_ADDR
#define USB_HID_EP_OUT HID_EPOUT_ADDR
#else
#define USB_HID_EP_IN  0x83
#define USB_HID_EP_OUT 0x03
#endif

#define USB_HID_EP_ID 0x3

    typedef void (*hid_report_callback_t)(uint8_t report_id, const void *buf, size_t length);

    /**
     * @brief  Initialize the HID interface for FIDO U2F.
     * @param  None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function initializes the HID interface for the FIDO U2F application.
     * It sets up necessary endpoints, allocates required resources, and prepares
     * the device for communication with the host over USB HID.
     */
    int usb_hid_init(void);

    /**
     * @brief  Uninitialize the HID interface for FIDO U2F.
     * @param  None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function uninitializes the HID interface, releasing any resources that
     * were allocated during initialization. It should be called when the HID
     * interface is no longer needed or when the device is shutting down.
     */
    int usb_hid_uninit(void);

    /**
     * @brief  Send a HID report to the host.
     * @param  report_id The ID of the report to be sent.
     * @param  buf Pointer to the buffer containing the report data.
     * @param  length Length of the report data.
     * @retval int The number of bytes actually sent, or a negative value on error.
     *
     * This function sends a HID report to the host. The report is identified by its
     * ID, and the data to be sent is provided in the buffer. This is typically used
     * to send authentication data, status information, or responses to requests
     * from the host.
     */
    int hid_send_report(uint8_t report_id, const void *buf, size_t length);
    int hid_send_report_raw(const void *buf, size_t length);
    /**
     * @brief  Receive a HID report from the host.
     * @param  buf Pointer to the buffer where the received report will be stored.
     * @param  length Maximum length of the buffer.
     * @retval int The number of bytes actually received, or a negative value on
     * error.
     *
     * This function receives a HID report from the host. The received data is
     * stored in the provided buffer. This is typically used to handle incoming
     * commands, requests, or authentication challenges from the host.
     */
    int hid_receive_report(void *buf, size_t length);

    /**
     * @brief  Set a callback function for handling incoming HID reports.
     * @param  callback Pointer to the callback function.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function sets a callback that will be invoked whenever a new HID report
     * is received from the host. The callback function should handle the processing
     * of the received data and respond appropriately.
     */
    int hid_set_report_callback(void (*callback)(uint8_t report_id, const void *buf, size_t length));

    /**
     * @brief  Handle a control request from the host.
     * @param  request Pointer to the control request structure.
     * @retval int 0 if the request was handled successfully, negative value if an
     * error occurs.
     *
     * This function handles control requests sent by the host to the device. It
     * processes standard and class-specific requests, such as GET_REPORT,
     * SET_REPORT, and GET_IDLE, which are essential for FIDO U2F communication over
     * HID.
     */
    int hid_handle_control_request(struct usb_control_request *request);

#ifdef __cplusplus
}
#endif

#endif /* __HID_H */