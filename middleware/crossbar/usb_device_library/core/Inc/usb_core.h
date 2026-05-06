/**
 ******************************************************************************
 * @file    usb_core.h
 * @author  USB Team
 * @brief   Header file of USB Core module.
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
#ifndef __USB_CORE_H
#define __USB_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "usb_def.h"

#define USB_VENDOR_ID  0x0ABC
#define USB_PRODUCT_ID 0x3880

#define USB_MAX_CONFIGURATIONS 4
#define USB_MAX_INTERFACES     4
#define USB_MAX_ENDPOINTS      8

#define USB_CDC_ACM_INTERFACE 0x00
#define USB_HID_INTERFACE     0x02
// add new cdc acm
#define USB_CDC_ACM_MPC_CMD_INTERFACE 0x02
// add new cdc acm
#define USB_CDC_ACM_MPC_DATA_INTERFACE 0x03

#define USB_CDC_ACM_EP_NUM 0x01
// add new cdc acm
#define USB_CDC_ACM_EP_MPC_NUM 0x03
#define USB_HID_EP_NUM     0x03

#define USB_EP_NUMBER_MASK 0x0F

#define USB_MAX_CLASS_TYPES 0x5

#define min_t(type, x, y)                  \
    ({                                     \
        type __min1 = (x);                 \
        type __min2 = (y);                 \
        __min1 < __min2 ? __min1 : __min2; \
    })

#define max_t(type, x, y)                  \
    ({                                     \
        type __max1 = (x);                 \
        type __max2 = (y);                 \
        __max1 > __max2 ? __max1 : __max2; \
    })

    /* definitions of device state */
    enum usb_state_e
    {
        USB_STATE_NOTATTACHED = 0,
        USB_STATE_ATTACHED,
        USB_STATE_POWERED,
        USB_STATE_RECONNECTING,
        USB_STATE_UNAUTHENTICATED,
        USB_STATE_DEFAULT,
        USB_STATE_ADDRESS,
        USB_STATE_CONFIGURED,
        USB_STATE_SUSPENDED
    };

    enum usb_trans_type
    {
        USB_TRANS_TYPE_BULK,
        USB_TRANS_TYPE_INT,
        USB_TRANS_TYPE_ISO
    };

    /* callback for usb class */
    typedef void (*usb_ep_complete_callback_t)(uint8_t ep_num, uint8_t direction, uint8_t *buf, uint32_t len, uint8_t error);
    typedef int (*usb_class_request_handler_t)(struct usb_control_request *setup);
    typedef int (*usb_class_get_descriptor_t)(uint8_t *buf, uint16_t len);

    struct usb_class_callback_t
    {
        usb_ep_complete_callback_t  ep_complete;
        usb_ep_complete_callback_t  ep0_complete;
        usb_class_get_descriptor_t  get_dev_descriptor;
        usb_class_get_descriptor_t  get_config_descriptor;
        usb_class_get_descriptor_t  get_interface_descriptor;
        usb_class_get_descriptor_t  get_interface_ass_descriptor;
        usb_class_request_handler_t request_handler;
    };

    /**
     * @brief Initializes the USB core, preparing it for operation.
     * @param None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function initializes the USB core, setting up necessary data structures,
     * Allocating resources, and preparing the device for enumeration by a USB host.
     */
    int usb_initialize(void);

    /**
     * @brief Uninitializes the USB core, releasing allocated resources.
     * @param None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function uninitializes the USB core, cleaning up resources that were
     * allocated during initialization. It should be called before the system shuts
     * down or when the USB functionality is no longer needed.
     */
    int usb_uninitialize(void);

    /**
     * @brief Connect the USB device from the bus.
     * @param None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function simulates a connection from the USB bus by enabling the USB
     * transceiver. It is useful for scenarios where the device needs to be
     * logically removed from the bus.
     */
    int usb_connect(void);

    /**
     * @brief Disconnect the USB device from the bus.
     * @param None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function simulates a disconnection from the USB bus by disabling the USB
     * transceiver. It is useful for scenarios where the device needs to be
     * logically removed from the bus.
     */
    int usb_disconnect(void);

    /**
     * @brief  Registers a USB class driver with the USB core.
     * @param  ep ID of the class to register.
     * @param  callback Pointer to the callback function for handling class-specific
     * events.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function allows the registration of a USB class driver with the USB
     * core. The class driver will receive notifications of class-specific events
     * through the callback.
     */
    int usb_class_register(int ep, struct usb_class_callback_t cb);

    /**
     * @brief  Unregisters a USB class driver from the USB core.
     * @param  ep ID of the class to unregister.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function removes a previously registered USB class driver from the USB
     * core. Once unregistered, the class driver will no longer receive
     * class-specific events.
     */
    int usb_class_unregister(int ep);

    /**
     * @brief  Retrieves the current status of the USB device.
     * @param  None.
     * @retval int The current status as a bitmask, or a negative value on error.
     *
     * This function returns the current status of the USB device, including whether
     * it is self-powered, whether remote wakeup is enabled, and other status flags.
     */
    int usb_status_get(void);

    /**
     * @brief  Sends data over a specified USB endpoint.
     * @param  ep_num: The USB endpoint number.
     * @param  buf: Pointer to the data buffer to be sent.
     * @param  len: Length of the data to be sent, in bytes.
     * @param  trans_type: Type of USB transfer (e.g., control, bulk, interrupt,
     * isochronous).
     * @retval int Number of bytes sent on success, or a negative value on error.
     *
     * This function sends data over the specified USB endpoint. It takes the
     * endpoint number, data buffer, buffer length, and transfer type as parameters.
     */
    int usb_ep_send(uint8_t ep_num, uint8_t *buf, uint32_t len, int trans_type);

    /**
     * @brief  Prepare receiving data from a specified USB endpoint.
     * @param  ep_num: The USB endpoint number.
     * @param  trans_type: Type of USB transfer (e.g., control, bulk, interrupt,
     * isochronous).
     * @retval int Number of bytes received on success, or a negative value on
     * error.
     *
     * This function receives data from the specified USB endpoint and stores it in
     * the provided buffer. It takes the endpoint number, buffer, buffer length, and
     * transfer type as parameters.
     */
    int usb_ep_recv_prepare(uint8_t ep_num, int trans_type);

    /**
     * @brief  Wakes up the USB device if it is in a suspended state.
     * @param  None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function wakes up the USB device if it has been suspended by the host.
     * It is typically used in conjunction with remote wakeup functionality,
     * allowing the device to signal the host to resume normal operation.
     */
    int usb_wakeup(void);

    /**
     * @brief  Suspends the USB device, reducing its power consumption.
     * @param  None.
     * @retval int 0 if successful, negative value if an error occurs.
     *
     * This function puts the USB device into a suspended state to save power.
     * During suspension, the device stops responding to bus traffic, and its power
     * consumption is minimized. The device can later be woken up using the
     * `usb_wakeup` function.
     */
    int usb_suspend(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CORE_H */
