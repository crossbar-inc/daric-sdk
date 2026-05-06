/**
 ******************************************************************************
 * @file    usb_hid.c
 * @author  USB Team
 * @brief   USB HID module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the General Purpose Input/Output (USB HID)
 *          peripheral:
 *           + Initialization and de-initialization functions
 *
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "daric_hal_usb.h"
#include "usb_core.h"
#include "usb_def.h"
#include "usb_hid_def.h"
#include "usb_hid.h"

#define USB_HID_BUFFER_SIZE USB_HID_MAX_REPORT_SIZE

#define USB_HID_GET_REPORT 0x01
#define USB_HID_SET_REPORT 0x09
#define USB_HID_SET_IDLE   0x0A
#define USB_HID_GET_IDLE   0x02

// #define DEVDBG
#ifdef DEVDBG
#define USB_LOG(format, ...) printf(format, ##__VA_ARGS__)
#define USB_LOG(format, ...) printf(format, ##__VA_ARGS__)
#define USB_LOG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define USB_LOG(format, ...)
#define USB_LOG(format, ...)
#define USB_LOG(format, ...)
#endif

typedef struct
{
    uint8_t  buf[USB_HID_BUFFER_SIZE];
    uint32_t act_len;
} buffer_info_t;

typedef struct
{
    uint32_t              idle_state;
    hid_report_callback_t callback;
    buffer_info_t         receive_buffer;
    buffer_info_t         transmit_buffer;
} hid_context_t;

static const struct usb_device_descriptor hid_device_desc = {
    .bLength         = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,

    .bcdUSB             = 0x0,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 0x00,
    .idVendor           = USB_VENDOR_ID,
    .idProduct          = USB_PRODUCT_ID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
    0x06,
    0xd0,
    0xf1, // USAGE_PAGE (FIDO Alliance)
    0x09,
    0x01, /* USAGE (Keyboard) */
    0xa1,
    0x01, /* COLLECTION (Application) */

    0x09,
    0x20, //   USAGE (Input Report Data)
    0x15,
    0x00, //   LOGICAL_MINIMUM (0)
    0x26,
    0xff,
    0x00, //   LOGICAL_MAXIMUM (255)
    0x75,
    0x08, //   REPORT_SIZE (8)
    0x95,
    HID_PACKET_SIZE, //   REPORT_COUNT (64)
    0x81,
    0x02, //   INPUT (Data,Var,Abs)
    0x09,
    0x21, //   USAGE(Output Report Data)
    0x15,
    0x00, //   LOGICAL_MINIMUM (0)
    0x26,
    0xff,
    0x00, //   LOGICAL_MAXIMUM (255)
    0x75,
    0x08, //   REPORT_SIZE (8)
    0x95,
    HID_PACKET_SIZE, //   REPORT_COUNT (64)
    0x91,
    0x02, //   OUTPUT (Data,Var,Abs)

    0xc0, // END_COLLECTION
};

static const struct usb_interface_association_descriptor hid_ass_desc = { .bLength           = sizeof(struct usb_interface_association_descriptor),
                                                                          .bDescriptorType   = USB_DT_INTERFACE_ASSOCIATION,
                                                                          .bFirstInterface   = USB_HID_INTERFACE,
                                                                          .bInterfaceCount   = 0x01,
                                                                          .bFunctionClass    = 0x03,
                                                                          .bFunctionSubClass = 0x00,
                                                                          .bFunctionProtocol = 0x00,
                                                                          .iFunction         = 0x00 };

static const struct usb_hid_config_set_descriptor hid_config_desc = {
    .config = {.bLength = sizeof(struct usb_hid_descriptor),
               .bDescriptorType = USB_DT_CONFIG,
               .wTotalLength = 0x29, //0x22,
               .bNumInterfaces = 0x01U,
               .bConfigurationValue = 0x01U,
               .iConfiguration = 0x00U,
               .bmAttributes = 0xC0U,
               .bMaxPower = 0x32U},

    .hid_itf = {.bLength = sizeof(struct usb_interface_descriptor),
                .bDescriptorType = USB_DT_INTERFACE,
                .bInterfaceNumber = USB_HID_INTERFACE,
                .bAlternateSetting = 0x00U,
                .bNumEndpoints = 0x02U,
                .bInterfaceClass = USB_HID_CLASS,
                .bInterfaceSubClass = 0x0U,
                .bInterfaceProtocol = 0x0,
                .iInterface = 0x00U},

    .hid_vendor =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_hid_descriptor),
                    .bDescriptorType = USB_DT_HID,
                },
            .bcdHID = 0x0110U,
            .bCountryCode = 0x00U,
            .bNumDescriptors = 0x01U,
            .bDescriptorType = USB_DT_REPORT,
            .wDescriptorLength = sizeof(hid_report_descriptor),
        },

    .hid_epin = {.bLength = sizeof(struct usb_endpoint_descriptor),
                 .bDescriptorType = USB_DT_ENDPOINT,
                 .bEndpointAddress = USB_HID_EP_IN,
                 .bmAttributes = USB_EP_XFER_INTERRUPT,
                 .wMaxPacketSize = 0x40U,
                 //.bInterval = 0x40U},
                 .bInterval = 0x5U},

    .hid_epout = {.bLength = sizeof(struct usb_endpoint_descriptor),
                  .bDescriptorType = USB_DT_ENDPOINT,
                  .bEndpointAddress = USB_HID_EP_OUT,
                  .bmAttributes = USB_EP_XFER_INTERRUPT,
                  .wMaxPacketSize = 0x40U,
                  //.bInterval = 0x40U}};
                  .bInterval = 0x5U}};

static hid_context_t hid_ctx;

static int hid_get_dev_descriptor(uint8_t *buf, uint16_t length)
{
    size_t descriptor_length = sizeof(hid_device_desc);

    if (!buf || length < descriptor_length)
    {
        return -1;
    }

    memcpy(buf, &hid_device_desc, descriptor_length);
    return descriptor_length;
}

static int hid_get_config_descriptor(uint8_t *buf, uint16_t length)
{
    int offset = 0;

    if (!buf || !length)
    {
        return -1;
    }

    if (length < sizeof(hid_config_desc))
        memcpy(buf + offset, &hid_config_desc, length);
    else
        memcpy(buf + offset, &hid_config_desc, sizeof(hid_config_desc));

    offset += sizeof(hid_config_desc);

    return offset;
}

static int hid_get_interface_descriptor(uint8_t *buf, uint16_t length)
{
    uint16_t copy_len          = 0;
    size_t   descriptor_length = sizeof(hid_config_desc) - sizeof(hid_config_desc.config);

    if (!buf)
    {
        return -1;
    }

    if (length != 0)
    {
        copy_len = length > descriptor_length ? descriptor_length : length;
        memcpy(buf, &hid_config_desc.hid_itf, copy_len);
    }

    return descriptor_length;
}

static int hid_get_interface_ass_descriptor(uint8_t *buf, uint16_t length)
{
    uint16_t copy_len          = 0;
    size_t   descriptor_length = sizeof(hid_ass_desc);

    if (!buf)
    {
        return -1;
    }

    if (length != 0)
    {
        copy_len = length > descriptor_length ? descriptor_length : length;
        memcpy(buf, &hid_ass_desc, copy_len);
    }

    return descriptor_length;
}

static int hid_get_report_descriptor(uint8_t *buf, uint16_t length)
{
    size_t descriptor_length = sizeof(hid_report_descriptor);

    if (!buf || length < descriptor_length)
    {
        return -1;
    }

    memcpy(buf, hid_report_descriptor, descriptor_length);
    return descriptor_length;
}

static int hid_get_vendor_descriptor(uint8_t *buf, uint16_t length)
{
    size_t descriptor_length = sizeof(struct usb_hid_descriptor);

    if (!buf || length < descriptor_length)
    {
        return -1;
    }

    memcpy(buf, &(hid_config_desc.hid_vendor), descriptor_length);

    return descriptor_length;
}

static void usb_hid_ep_complete(uint8_t ep_num, uint8_t direction, uint8_t *buf, uint32_t len, uint8_t error)
{
    // printf("%s: ep_num:%d, dir:%d\n", __func__, ep_num, direction);

    if (error)
    {
        USB_LOG("%s: error occur, error code:%d\n", __func__, error);
        return;
    }

    if (len > 0 && direction == USB_SEND)
    {
        // printf("report has been sent: %ld bytes\n", len);
        memset(hid_ctx.transmit_buffer.buf, 0, hid_ctx.transmit_buffer.act_len);
        hid_ctx.transmit_buffer.act_len = 0;
    }
    else if (len > 0 && direction == USB_RECV)
    {
        // printf("report received: %ld bytes\n", len);

        if (hid_ctx.callback)
        {
            hid_ctx.callback(buf[0], buf, len);
        }
        else
        {
            USB_LOG("No callback function registered!\n");
            memcpy(hid_ctx.receive_buffer.buf, buf, len);
            hid_ctx.receive_buffer.act_len = len;
        }
        usb_ep_recv_prepare(ep_num, USB_TRANS_TYPE_INT);
    }
}

/**
 * @brief  Initialize the HID interface for FIDO U2F.
 * @param  None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function initializes the HID interface for the FIDO U2F application.
 * It sets up necessary endpoints, allocates required resources, and prepares
 * the device for communication with the host over USB HID.
 */
int usb_hid_init(void)
{
    memset(&hid_ctx, 0, sizeof(hid_context_t));

    struct usb_class_callback_t hid_callbacks = {
        .ep_complete                  = usb_hid_ep_complete,
        .ep0_complete                 = 0,
        .get_dev_descriptor           = hid_get_dev_descriptor,
        .get_config_descriptor        = hid_get_config_descriptor,
        .get_interface_descriptor     = hid_get_interface_descriptor,
        .get_interface_ass_descriptor = hid_get_interface_ass_descriptor,
        .request_handler              = hid_handle_control_request,
    };

    usb_class_register(USB_HID_EP_NUM, hid_callbacks);

    return 0;
}

/**
 * @brief  Uninitialize the HID interface for FIDO U2F.
 * @param  None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function uninitializes the HID interface, releasing any resources that
 * were allocated during initialization. It should be called when the HID
 * interface is no longer needed or when the device is shutting down.
 */
int usb_hid_uninit(void)
{
    return usb_class_unregister(USB_HID_EP_NUM);
}

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
int hid_send_report(uint8_t report_id, const void *buf, size_t length)
{
    if (!buf || length == 0 || length > USB_HID_MAX_REPORT_SIZE)
    {
        return -1;
    }

    if (hid_ctx.transmit_buffer.act_len != 0)
    {
        // printf("%s: report is sending, left %ld\n", __func__,
        //        hid_ctx.transmit_buffer.act_len);
        return -1;
    }

    hid_ctx.transmit_buffer.buf[0] = report_id;
    memcpy(hid_ctx.transmit_buffer.buf + 1, buf, length);
    hid_ctx.transmit_buffer.act_len = length + 1;

    return usb_ep_send(USB_HID_EP_ID, hid_ctx.transmit_buffer.buf, length + 1, USB_TRANS_TYPE_INT);
}

int hid_send_report_raw(const void *buf, size_t length)
{
    if (!buf || length == 0 || length > USB_HID_MAX_REPORT_SIZE)
    {
        return -1;
    }

    if (hid_ctx.transmit_buffer.act_len != 0)
    {
        // printf("%s: report is sending, left %ld\n", __func__,
        //        hid_ctx.transmit_buffer.act_len);
        return -2;
    }

    memcpy(hid_ctx.transmit_buffer.buf, buf, length);
    hid_ctx.transmit_buffer.act_len = length;

    return usb_ep_send(USB_HID_EP_ID, hid_ctx.transmit_buffer.buf, length, USB_TRANS_TYPE_INT);
}

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
int hid_receive_report(void *buf, size_t length)
{
    size_t actual_length = 0;

    if (!buf || length == 0 || length > USB_HID_MAX_REPORT_SIZE)
    {
        return -1;
    }

    if (!hid_ctx.receive_buffer.act_len)
        return 0;

    actual_length = hid_ctx.receive_buffer.act_len > length ? length : hid_ctx.receive_buffer.act_len;
    memcpy(buf, hid_ctx.receive_buffer.buf, actual_length);
    hid_ctx.receive_buffer.act_len = 0;

    return actual_length;
}

/**
 * @brief  Set a callback function for handling incoming HID reports.
 * @param  callback Pointer to the callback function.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function sets a callback that will be invoked whenever a new HID report
 * is received from the host. The callback function should handle the processing
 * of the received data and respond appropriately.
 */
int hid_set_report_callback(void (*callback)(uint8_t report_id, const void *buf, size_t length))
{
    if (!callback)
    {
        return -1;
    }

    hid_ctx.callback = callback;
    return 0;
}

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
int hid_handle_control_request(struct usb_control_request *request)
{
    int      offset   = 0;
    int      buffsize = 0;
    uint8_t *buf      = hid_ctx.transmit_buffer.buf;

    USB_LOG("%s: bRequest:%x, bRequestType:%x, wLength:%x\n", __func__, request->bRequest, request->bmRequestType, request->wLength);

    switch (request->bRequest)
    {
        case USB_HID_GET_REPORT:
            USB_LOG("get report?.......................\n");
            break;
        case USB_HID_SET_REPORT:
            USB_LOG("set report?.......................\n");
            usb_ep_send(0, (uint8_t *)&(hid_ctx.idle_state), 1, USB_TRANS_TYPE_INT);
            break;
        case USB_HID_SET_IDLE:
            hid_ctx.idle_state = (uint8_t)(request->wValue >> 8);
            break;
        case USB_HID_GET_IDLE:
            break;
        case USB_REQ_GET_DESCRIPTOR:
            if (USB_DT_REPORT == (request->wValue >> 8))
                offset = hid_get_report_descriptor(buf, USB_HID_BUFFER_SIZE);
            else if (USB_DT_HID == (request->wValue >> 8U))
                offset = hid_get_vendor_descriptor(buf, USB_HID_BUFFER_SIZE);

            buffsize = min_t(int, request->wLength, offset);
            usb_ep_send(0, (uint8_t *)buf, buffsize, USB_TRANS_TYPE_INT);
            break;
        default:
            USB_LOG("%s: unhandled request:%d\n", __func__, request->bRequest);
            return -1;
    }

    return 0;
}
