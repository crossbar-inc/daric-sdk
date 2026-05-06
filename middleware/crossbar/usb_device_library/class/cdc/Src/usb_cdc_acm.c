/**
 ******************************************************************************
 * @file    usb_cdc_acm.c
 * @author  USB Team
 * @brief   USB CDC module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the General Purpose Input/Output (USB CDC)
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
#include "usb_cdc_acm.h"
#include "daric_hal_usb.h"
#include "usb_cdc.h"
#include "usb_core.h"
#include "usb_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CDC_SET_LINE_CODING        0x20
#define CDC_GET_LINE_CODING        0x21
#define CDC_SET_CONTROL_LINE_STATE 0x22
#define CDC_SEND_BREAK             0x23

#define MAX_BAUD_RATE    9216000
#define MAX_DATA_BITS    8
#define MAX_STOP_BITS    2
#define MAX_PARITY       2
#define MAX_FLOW_CONTROL 2

#ifdef CONFIG_FIDO_USB_HID
#define USB_CDC_ACM_NUM 1
#else
#define USB_CDC_ACM_NUM 2
#endif
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
    uint8_t  buf[USB_CDC_ACM_BUF_SIZE];
    uint32_t act_len;
} buffer_info_t;

typedef struct
{
    cdc_acm_event_callback_t event_callback;
    void                    *callback_context;
    cdc_acm_config_t         config;
    buffer_info_t            receive_buffer;
    buffer_info_t            transmit_buffer;
} cdc_acm_context_t;

typedef struct
{
    uint32_t           ep_num;
    uint32_t           if_num;
    cdc_acm_context_t *ctx;
    cdc_acm_state_e    state;
    uint32_t           line_coding;
} cdc_acm_t;

static const struct usb_device_descriptor cdc_device_desc = {
    .bLength         = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,

    .bcdUSB             = 0x0200,
#ifdef CONFIG_FIDO_USB_HID
    .bDeviceClass       = 0x02,
    .bDeviceSubClass    = 0x02,
    .bDeviceProtocol    = 0x00,
#else
    .bDeviceClass       = 0xEF,
    .bDeviceSubClass    = 0x02,
    .bDeviceProtocol    = 0x01,
#endif

    .bMaxPacketSize0    = 0x40,
    .idVendor           = USB_VENDOR_ID,
    .idProduct          = USB_PRODUCT_ID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 1,
};

static const struct usb_cdc_config_descriptor cdc_config_desc = {
    .config = {.bLength = sizeof(struct usb_configuration_descriptor),
               .bDescriptorType = USB_DT_CONFIG, /* Configuration */
               .wTotalLength = 0x4B,
               .bNumInterfaces = 0x02U,
               .bConfigurationValue = 0x01U,
               .iConfiguration = 0x00U,
               .bmAttributes = 0xC0U,
               .bMaxPower = 0x32U},
#ifdef CONFIG_FIDO_USB_HID
    .itf_ass =
        {
            .bLength = sizeof(struct usb_interface_association_descriptor),
            .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
            .bFirstInterface = USB_CDC_ACM_INTERFACE,
            .bInterfaceCount = 0x02,
            .bFunctionClass = 0x02,
            .bFunctionSubClass = 0x02,
            .bFunctionProtocol = 0x01,
            .iFunction = 0x00,
        },

    .cmd_itf = {.bLength = sizeof(struct usb_interface_descriptor),
                .bDescriptorType = USB_DT_INTERFACE,
                .bInterfaceNumber = USB_CDC_ACM_INTERFACE,
                .bAlternateSetting = 0x00U,
                .bNumEndpoints = 0x01U,
                .bInterfaceClass = USB_CLASS_CDC,
                .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
                .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
                .iInterface = 0x00U},

    .cdc_header = {.header =
                       {
                           .bLength = sizeof(struct usb_header_func_descriptor),
                           .bDescriptorType = USB_DT_PIPE_USAGE,
                       },
                   .bDescriptorSubtype = 0x00U,
                   .bcdCDC = 0x0110U},

    .cdc_call_managment =
        {.header =
             {
                 .bLength = sizeof(struct usb_call_management_func_descriptor),
                 .bDescriptorType = USB_DT_PIPE_USAGE,
             },
         .bDescriptorSubtype = 0x01U,
         .bmCapabilities = 0x00U,
         .bDataInterface = 0x01U},

    .cdc_acm =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_acm_func_descriptor),
                    .bDescriptorType = USB_DT_PIPE_USAGE,
                },
            .bDescriptorSubtype = 0x02U,
            .bmCapabilities = 0x02U,
        },

    .cdc_union =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_union_func_descriptor),
                    .bDescriptorType = USB_DT_PIPE_USAGE,
                },
            .bDescriptorSubtype = 0x06U,
            .bMasterInterface = 0x00U,
            .bSlaveInterface0 = 0x01U,
        },

    .cdc_cmd_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                         .bDescriptorType = USB_DT_ENDPOINT,
                         .bEndpointAddress = USB_CDC_ACM_EP_CMD,
                         .bmAttributes = USB_EP_XFER_INTERRUPT,
                         .wMaxPacketSize = 0x0A,
                         .bInterval = 0x10U},

    .cdc_data_interface = {.bLength = sizeof(struct usb_interface_descriptor),
                           .bDescriptorType = USB_DT_INTERFACE,
                           .bInterfaceNumber = 0x01U,
                           .bAlternateSetting = 0x00U,
                           .bNumEndpoints = 0x02U,
                           .bInterfaceClass = USB_CLASS_DATA,
                           .bInterfaceSubClass = 0x00U,
                           .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
                           .iInterface = 0x00U},

    .cdc_out_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                         .bDescriptorType = USB_DT_ENDPOINT,
                         .bEndpointAddress = USB_CDC_ACM_EP_OUT,
                         .bmAttributes = USB_EP_XFER_BULK,
                         .wMaxPacketSize = 0x200,
                         .bInterval = 0x00U},

    .cdc_in_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                        .bDescriptorType = USB_DT_ENDPOINT,
                        .bEndpointAddress = USB_CDC_ACM_EP_IN,
                        .bmAttributes = USB_EP_XFER_BULK,
                        .wMaxPacketSize = 0x200,
                        .bInterval = 0x00U}
#else
    .itf_ass =
        {
            .bLength = sizeof(struct usb_interface_association_descriptor),
            .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
            .bFirstInterface = USB_CDC_ACM_INTERFACE,
            .bInterfaceCount = 0x02,
            .bFunctionClass = 0x02,
            .bFunctionSubClass = 0x02,
            .bFunctionProtocol = 0x01,
            .iFunction = 0x00,
        },

    .cmd_itf = {.bLength = sizeof(struct usb_interface_descriptor),
                .bDescriptorType = USB_DT_INTERFACE,
                .bInterfaceNumber = USB_CDC_ACM_INTERFACE,
                .bAlternateSetting = 0x00U,
                .bNumEndpoints = 0x01U,
                .bInterfaceClass = USB_CLASS_CDC,
                .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
                .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
                .iInterface = 0x00U},

    .cdc_header = {.header =
                       {
                           .bLength = sizeof(struct usb_header_func_descriptor),
                           .bDescriptorType = USB_DT_PIPE_USAGE,
                       },
                   .bDescriptorSubtype = 0x00U,
                   .bcdCDC = 0x0110U},

    .cdc_call_managment =
        {.header =
             {
                 .bLength = sizeof(struct usb_call_management_func_descriptor),
                 .bDescriptorType = USB_DT_PIPE_USAGE,
             },
         .bDescriptorSubtype = 0x01U,
         .bmCapabilities = 0x00U,
         .bDataInterface = 0x01U},

    .cdc_acm =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_acm_func_descriptor),
                    .bDescriptorType = USB_DT_PIPE_USAGE,
                },
            .bDescriptorSubtype = 0x02U,
            .bmCapabilities = 0x02U,
        },

    .cdc_union =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_union_func_descriptor),
                    .bDescriptorType = USB_DT_PIPE_USAGE,
                },
            .bDescriptorSubtype = 0x06U,
            .bMasterInterface = 0x00U,
            .bSlaveInterface0 = 0x01U,
        },

    .cdc_cmd_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                         .bDescriptorType = USB_DT_ENDPOINT,
                         .bEndpointAddress = USB_CDC_ACM_EP_CMD,
                         .bmAttributes = USB_EP_XFER_INTERRUPT,
                         .wMaxPacketSize = 0x0A,
                         .bInterval = 0x10U},

    .cdc_data_interface = {.bLength = sizeof(struct usb_interface_descriptor),
                           .bDescriptorType = USB_DT_INTERFACE,
                           .bInterfaceNumber = 0x01U,
                           .bAlternateSetting = 0x00U,
                           .bNumEndpoints = 0x02U,
                           .bInterfaceClass = USB_CLASS_DATA,
                           .bInterfaceSubClass = 0x00U,
                           .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
                           .iInterface = 0x00U},

    .cdc_out_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                         .bDescriptorType = USB_DT_ENDPOINT,
                         .bEndpointAddress = USB_CDC_ACM_EP_OUT,
                         .bmAttributes = USB_EP_XFER_BULK,
                         .wMaxPacketSize = 0x200,
                         .bInterval = 0x00U},

    .cdc_in_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                        .bDescriptorType = USB_DT_ENDPOINT,
                        .bEndpointAddress = USB_CDC_ACM_EP_IN,
                        .bmAttributes = USB_EP_XFER_BULK,
                        .wMaxPacketSize = 0x200,
                        .bInterval = 0x00U},

#endif
};

#ifndef CONFIG_FIDO_USB_HID
static const struct usb_cdc_config_descriptor cdc_config_desc_mpc = {
    .config = {.bLength = sizeof(struct usb_configuration_descriptor),
               .bDescriptorType = USB_DT_CONFIG, /* Configuration */
               .wTotalLength = 0x4B,

               .bNumInterfaces = 0x02U,

               .bConfigurationValue = 0x01U,
               .iConfiguration = 0x00U,
               .bmAttributes = 0xC0U,
               .bMaxPower = 0x32U},
    .itf_ass =
        {
            .bLength = sizeof(struct usb_interface_association_descriptor),
            .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
            .bFirstInterface = USB_CDC_ACM_MPC_CMD_INTERFACE,
            .bInterfaceCount = 0x02,
            .bFunctionClass = 0x02,
            .bFunctionSubClass = 0x02,
            .bFunctionProtocol = 0x01,
            .iFunction = 0x00,
        },

    .cmd_itf = {.bLength = sizeof(struct usb_interface_descriptor),
                .bDescriptorType = USB_DT_INTERFACE,
                .bInterfaceNumber = USB_CDC_ACM_MPC_CMD_INTERFACE,
                .bAlternateSetting = 0x00U,
                .bNumEndpoints = 0x01U,
                .bInterfaceClass = USB_CLASS_CDC,
                .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
                .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
                .iInterface = 0x00U},

    .cdc_header = {.header =
                       {
                           .bLength = sizeof(struct usb_header_func_descriptor),
                           .bDescriptorType = USB_DT_PIPE_USAGE,
                       },
                   .bDescriptorSubtype = 0x00U,
                   .bcdCDC = 0x0110U},

    .cdc_call_managment =
        {.header =
             {
                 .bLength = sizeof(struct usb_call_management_func_descriptor),
                 .bDescriptorType = USB_DT_PIPE_USAGE,
             },
         .bDescriptorSubtype = 0x01U,
         .bmCapabilities = 0x00U,
         .bDataInterface = USB_CDC_ACM_MPC_DATA_INTERFACE},

    .cdc_acm =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_acm_func_descriptor),
                    .bDescriptorType = USB_DT_PIPE_USAGE,
                },
            .bDescriptorSubtype = 0x02U,
            .bmCapabilities = 0x02U,
        },

    .cdc_union =
        {
            .header =
                {
                    .bLength = sizeof(struct usb_union_func_descriptor),
                    .bDescriptorType = USB_DT_PIPE_USAGE,
                },
            .bDescriptorSubtype = 0x06U,
            .bMasterInterface = USB_CDC_ACM_MPC_CMD_INTERFACE,
            .bSlaveInterface0 = USB_CDC_ACM_MPC_DATA_INTERFACE,
        },

    .cdc_cmd_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                         .bDescriptorType = USB_DT_ENDPOINT,
                         .bEndpointAddress = USB_CDC_ACM_EP_MPC_CMD,
                         .bmAttributes = USB_EP_XFER_INTERRUPT,
                         .wMaxPacketSize = 0x0A,
                         .bInterval = 0x10U},

    .cdc_data_interface = {.bLength = sizeof(struct usb_interface_descriptor),
                           .bDescriptorType = USB_DT_INTERFACE,
                           .bInterfaceNumber = USB_CDC_ACM_MPC_DATA_INTERFACE,
                           .bAlternateSetting = 0x00U,
                           .bNumEndpoints = 0x02U,
                           .bInterfaceClass = USB_CLASS_DATA,
                           .bInterfaceSubClass = 0x00U,
                           .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
                           .iInterface = 0x00U},

    .cdc_out_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                         .bDescriptorType = USB_DT_ENDPOINT,
                         .bEndpointAddress = USB_CDC_ACM_EP_MPC_OUT,
                         .bmAttributes = USB_EP_XFER_BULK,
                         .wMaxPacketSize = 0x200,
                         .bInterval = 0x00U},

    .cdc_in_endpoint = {.bLength = sizeof(struct usb_endpoint_descriptor),
                        .bDescriptorType = USB_DT_ENDPOINT,
                        .bEndpointAddress = USB_CDC_ACM_EP_MPC_IN,
                        .bmAttributes = USB_EP_XFER_BULK,
                        .wMaxPacketSize = 0x200,
                        .bInterval = 0x00U}};
#endif

static cdc_acm_t s_cdc_acm[USB_CDC_ACM_NUM] = {
#ifdef CONFIG_FIDO_USB_HID
    {
        .ep_num = 1,
        .if_num = USB_CDC_ACM_INTERFACE,
    }
#else
    {
        .ep_num = 1,
        .if_num = USB_CDC_ACM_INTERFACE,
    },
    {
        .ep_num = 3,
        .if_num = USB_CDC_ACM_MPC_CMD_INTERFACE,
    }
#endif
};

static int cdc_acm_request_handler(struct usb_control_request *setup)
{
    uint8_t    buffer[sizeof(cdc_acm_config_t)] = { 0 };
    cdc_acm_t *cdc_acm                          = 0;
    int        retval                           = 0;

    for (int i = 0; i < USB_CDC_ACM_NUM; i++)
    {
        if (s_cdc_acm[i].if_num == (setup->wIndex & 0xff))
        {
            cdc_acm = &s_cdc_acm[i];
        }
    }

    if (!setup || !cdc_acm || !cdc_acm->ctx)
    {
        USB_LOG("Invalid setup request\n");
        return -1;
    }

    switch (setup->bRequest)
    {
        case CDC_SET_LINE_CODING:
            if (setup->wLength >= sizeof(cdc_acm_config_t))
            {
                usb_ep_recv_prepare(0, USB_TRANS_TYPE_INT);
                cdc_acm->line_coding = 1;
            }
            break;

        case CDC_GET_LINE_CODING:
            buffer[0] = (uint8_t)(cdc_acm->ctx->config.baud_rate);
            buffer[1] = (uint8_t)(cdc_acm->ctx->config.baud_rate >> 8);
            buffer[2] = (uint8_t)(cdc_acm->ctx->config.baud_rate >> 16);
            buffer[3] = (uint8_t)(cdc_acm->ctx->config.baud_rate >> 24);
            buffer[4] = cdc_acm->ctx->config.stop_bits;
            buffer[5] = cdc_acm->ctx->config.parity;
            buffer[6] = cdc_acm->ctx->config.data_bits;

            retval = usb_ep_send(0, buffer, sizeof(cdc_acm_config_t), USB_TRANS_TYPE_INT);
            if (retval < 0)
            {
                return -1;
            }
            return sizeof(cdc_acm_config_t);
        default:
            USB_LOG("Unhandled request: %d\n", setup->bRequest);
            retval = 0;
            usb_ep_send(0, 0, 0, USB_TRANS_TYPE_INT);
            break;
    }

    return retval;
}

static void cdc_acm_ep0_complete(uint8_t ep_num, uint8_t direction, uint8_t *buf, uint32_t len, uint8_t error)
{
    cdc_acm_t *cdc_acm = 0;

    for (int i = 0; i < USB_CDC_ACM_NUM; i++)
    {
        if (s_cdc_acm[i].line_coding == 1)
        {
            cdc_acm = &s_cdc_acm[i];
        }
    }

    if (cdc_acm)
    {
        cdc_acm->ctx->config.baud_rate = (uint32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | buf[3] << 24);
        cdc_acm->ctx->config.stop_bits = buf[4];
        cdc_acm->ctx->config.parity    = buf[5];
        cdc_acm->ctx->config.data_bits = buf[6];

        USB_LOG(
            "Set line coding: baud_rate=%ld, data_bits=%d, stop_bits=%d, "
            "parity=%d\n",
            cdc_acm->ctx->config.baud_rate, cdc_acm->ctx->config.data_bits, cdc_acm->ctx->config.stop_bits, cdc_acm->ctx->config.parity);
        cdc_acm->line_coding = 0;
    }

    return;
}

static void cdc_acm_ep_complete(uint8_t ep_num, uint8_t direction, uint8_t *buf, uint32_t len, uint8_t error)
{
    cdc_acm_t *cdc_acm = 0;

    for (int i = 0; i < USB_CDC_ACM_NUM; i++)
    {
        if (s_cdc_acm[i].ep_num == ep_num)
        {
            cdc_acm = &s_cdc_acm[i];
        }
    }

    if (!cdc_acm || !cdc_acm->ctx)
        return;

    if (error)
    {
        if (cdc_acm->ctx->event_callback)
        {
            cdc_acm->ctx->event_callback(cdc_acm->ctx->callback_context, CDC_ACM_EVENT_ERROR, NULL);
        }
        return;
    }

    if (len > 0 && direction == USB_SEND)
    {
        memset(cdc_acm->ctx->transmit_buffer.buf, 0, cdc_acm->ctx->transmit_buffer.act_len);
        cdc_acm->ctx->transmit_buffer.act_len = 0;

        if (cdc_acm->ctx->event_callback)
        {
            cdc_acm->ctx->event_callback(cdc_acm->ctx->callback_context, CDC_ACM_EVENT_DATA_SENT, 0);
        }
    }
    else if (len > 0 && direction == USB_RECV)
    {
        memcpy(cdc_acm->ctx->receive_buffer.buf, buf, len);
        cdc_acm->ctx->receive_buffer.act_len = len;
        if (cdc_acm->ctx->event_callback)
        {
            cdc_acm->ctx->event_callback(cdc_acm->ctx->callback_context, CDC_ACM_EVENT_DATA_RECEIVED, (void *)&(cdc_acm->ctx->receive_buffer));
        }

        usb_ep_recv_prepare(ep_num, USB_TRANS_TYPE_BULK);
    }
}

static int cdc_acm_get_dev_descriptor(uint8_t *buf, uint16_t length)
{
    int offset = 0;

    if (!buf || !length)
    {
        return -1;
    }

    memcpy(buf + offset, &cdc_device_desc, sizeof(cdc_device_desc));
    offset += sizeof(cdc_device_desc);

    return offset;
}

static int cdc_acm_get_config_descriptor(uint8_t *buf, uint16_t length)
{
    int offset = 0;

    if (!buf || !length)
    {
        return -1;
    }

    if (length < sizeof(cdc_config_desc))
        memcpy(buf + offset, &cdc_config_desc, length);
    else
        memcpy(buf + offset, &cdc_config_desc, sizeof(cdc_config_desc));

    offset += sizeof(cdc_config_desc);

    return offset;
}

#ifndef CONFIG_FIDO_USB_HID
static int cdc_acm_mpc_get_config_descriptor(uint8_t *buf, uint16_t length)
{
    int offset = 0;

    if (!buf || !length)
    {
      return -1;
    }

    if (length < sizeof(cdc_config_desc_mpc))
      memcpy(buf + offset, &cdc_config_desc_mpc, length);
    else
      memcpy(buf + offset, &cdc_config_desc_mpc, sizeof(cdc_config_desc_mpc));

    offset += sizeof(cdc_config_desc_mpc);

    return offset;
}
#endif

static int cdc_acm_get_interface_descriptor(uint8_t *buf, uint16_t length)
{
    uint16_t copy_len          = 0;
    size_t   descriptor_length = sizeof(cdc_config_desc) - sizeof(cdc_config_desc.config) - sizeof(cdc_config_desc.itf_ass);

    if (!buf)
    {
        return -1;
    }

    if (length != 0)
    {
        copy_len = length > descriptor_length ? descriptor_length : length;
        memcpy(buf, &cdc_config_desc.cmd_itf, copy_len);
    }

    return descriptor_length;
}

static int cdc_acm_get_interface_ass_descriptor(uint8_t *buf, uint16_t length)
{
    uint16_t copy_len          = 0;
    size_t   descriptor_length = sizeof(cdc_config_desc.itf_ass);

    if (!buf)
    {
        return -1;
    }

    if (length != 0)
    {
        copy_len = length > descriptor_length ? descriptor_length : length;
        memcpy(buf, &cdc_config_desc.itf_ass, copy_len);
    }

    return descriptor_length;
}
#ifndef CONFIG_FIDO_USB_HID
static int cdc_acm_mpc_get_interface_descriptor(uint8_t *buf, uint16_t length)
{
    uint16_t copy_len = 0;
    size_t descriptor_length = sizeof(cdc_config_desc_mpc) -
                               sizeof(cdc_config_desc_mpc.config) -
                               sizeof(cdc_config_desc_mpc.itf_ass);

    if (!buf)
    {
      return -1;
    }

    if (length != 0)
    {
      copy_len = length > descriptor_length ? descriptor_length : length;
      memcpy(buf, &cdc_config_desc_mpc.cmd_itf, copy_len);
    }

    return descriptor_length;
}

static int cdc_acm_mpc_get_interface_ass_descriptor(uint8_t *buf, uint16_t length)
{
    uint16_t copy_len = 0;
    size_t descriptor_length = sizeof(cdc_config_desc_mpc.itf_ass);

    if (!buf)
    {
      return -1;
    }

    if (length != 0)
    {
      copy_len = length > descriptor_length ? descriptor_length : length;
      memcpy(buf, &cdc_config_desc_mpc.itf_ass, copy_len);
    }

    return descriptor_length;
}

#endif

/**
 * @brief  CDC-ACM class initialization (e.g., class registration, resource
 * allocation).
 * @retval 0 on success, non-zero on failure.
 *
 * This function initializes the CDC-ACM class. It should be called before any
 * other CDC-ACM functions to set up the necessary resources and register the
 * class.
 */
int usb_cdc_acm_init(void)
{
    cdc_acm_context_t *ctx = 0;

    struct usb_class_callback_t cdc_acm_callbacks = {
        .get_dev_descriptor           = cdc_acm_get_dev_descriptor,
        .get_config_descriptor        = cdc_acm_get_config_descriptor,
        .get_interface_descriptor     = cdc_acm_get_interface_descriptor,
        .get_interface_ass_descriptor = cdc_acm_get_interface_ass_descriptor,
        .ep_complete                  = cdc_acm_ep_complete,
        .ep0_complete                 = cdc_acm_ep0_complete,
        .request_handler              = cdc_acm_request_handler,
    };

#ifndef CONFIG_FIDO_USB_HID
    struct usb_class_callback_t cdc_acm_mpc_callbacks = {
        .get_dev_descriptor = cdc_acm_get_dev_descriptor,
        .get_config_descriptor = cdc_acm_mpc_get_config_descriptor,

        .get_interface_descriptor = cdc_acm_mpc_get_interface_descriptor,
        .get_interface_ass_descriptor = cdc_acm_mpc_get_interface_ass_descriptor,

        .ep_complete = cdc_acm_ep_complete,
        .ep0_complete = cdc_acm_ep0_complete,
        .request_handler = cdc_acm_request_handler,
    };
#endif

    usb_class_register(USB_CDC_ACM_EP_NUM, cdc_acm_callbacks);
#ifndef CONFIG_FIDO_USB_HID
    usb_class_register(USB_CDC_ACM_EP_MPC_NUM, cdc_acm_mpc_callbacks);
#endif

    for (int i = 0; i < USB_CDC_ACM_NUM; i++)
    {
        ctx = (cdc_acm_context_t *)malloc(sizeof(cdc_acm_context_t));
        if (!ctx)
        {
            USB_LOG("%s: malloc failed\n", __func__);
            return -1;
        }

        memset(ctx, 0, sizeof(cdc_acm_context_t));

        ctx->config.baud_rate = 115200;
        ctx->config.data_bits = 8;
        ctx->config.parity    = 0;
        ctx->config.stop_bits = 0;
        s_cdc_acm[i].ctx      = ctx;
    }

    return 0;
}

/**
 * @brief  CDC-ACM class uninitialization (e.g., class unregistration, resource
 * deallocation).
 * @param  None.
 * @retval 0 on success, non-zero on failure.
 *
 * This function uninitializes the CDC-ACM class. It should be called to clean
 * up resources when the CDC-ACM functionality is no longer needed.
 */
int usb_cdc_acm_uninit(void)
{
    cdc_acm_context_t *ctx = 0;

    usb_disconnect();
    usb_uninitialize();

    usb_class_unregister(USB_CDC_ACM_EP_NUM);
#ifndef CONFIG_FIDO_USB_HID
    usb_class_unregister(USB_CDC_ACM_EP_MPC_NUM);
#endif

    for (int i = 0; i < USB_CDC_ACM_NUM; i++)
    {
        ctx = s_cdc_acm[i].ctx;
        if (ctx)
            free(ctx);

        s_cdc_acm[i].ctx = 0;
    }

    return 0;
}

/**
 * @brief  Opens the specific CDC-ACM port.
 * @param  port The specific port number to open.
 * @retval void* Handle pointing to the specific CDC-ACM port that has been
 * opened.
 *
 * This function opens the specified CDC-ACM port and returns a handle to it.
 * The handle is used for subsequent operations like read, write, and close.
 *
 * @note The handle must be checked for NULL to ensure that the port was opened
 * successfully.
 */
void *usb_cdc_acm_open(int port)
{
    if (port >= USB_CDC_ACM_NUM)
    {
        USB_LOG("%s: port error:%d\n", __func__, port);
        return NULL;
    }

    s_cdc_acm[port].state = CDC_ACM_PORT_OPEN;

    return (void *)&s_cdc_acm[port];
}

/**
 * @brief  Closes the specific CDC-ACM port.
 * @param  handle Handle pointing to the specific CDC-ACM port.
 * @retval None.
 *
 * This function closes the specified CDC-ACM port. The handle is no longer
 * valid after this function is called.
 */
void usb_cdc_acm_close(void *handle)
{
    cdc_acm_t *cdc_acm = (cdc_acm_t *)handle;

    if (!cdc_acm)
        return;

    cdc_acm->state = CDC_ACM_PORT_CLOSE;
}

/**
 * @brief  Reads data from the host through the specified CDC-ACM port.
 * @param  handle Handle pointing to the specific CDC-ACM port.
 * @param  buf Pointer to the buffer for receiving and storing data.
 * @param  length The size of the buffer.
 * @retval int The actual length of data read from the host, or a negative value
 * on error.
 *
 * This function reads data from the host via the CDC-ACM port. The actual
 * amount of data read is returned, which may be less than the requested length.
 */
int usb_cdc_acm_read(void *handle, void *buf, size_t length)
{
    size_t     actual_length = 0;
    cdc_acm_t *cdc_acm       = (cdc_acm_t *)handle;

    if (!buf || length == 0 || length > USB_CDC_ACM_BUF_SIZE)
    {
        return -1;
    }

    if (!cdc_acm || !cdc_acm->ctx)
    {
        return -1;
    }

    if (!cdc_acm->ctx->receive_buffer.act_len)
        return 0;

    actual_length = cdc_acm->ctx->receive_buffer.act_len > length ? length : cdc_acm->ctx->receive_buffer.act_len;
    memcpy(buf, cdc_acm->ctx->receive_buffer.buf, actual_length);
    cdc_acm->ctx->receive_buffer.act_len = 0;

    return actual_length;
}

/**
 * @brief  Writes data to the host through the specified CDC-ACM port.
 * @param  handle Handle pointing to the specific CDC-ACM port.
 * @param  buf Pointer to the buffer containing the data to write to the host.
 * @param  length The size of the data to write.
 * @retval int The actual length of data written to the host, or a negative
 * value on error.
 *
 * This function writes data to the host via the CDC-ACM port. The actual amount
 * of data written is returned, which may be less than the requested length.
 */
int usb_cdc_acm_write(void *handle, void *buf, size_t length)
{
    int        retval  = 0;
    cdc_acm_t *cdc_acm = (cdc_acm_t *)handle;

    if (!buf || length == 0 || length > USB_CDC_ACM_BUF_SIZE)
    {
        return -1;
    }

    if (!cdc_acm || !cdc_acm->ctx)
    {
      return -2;
    }

    if (cdc_acm->ctx->transmit_buffer.act_len != 0)
    {
        USB_LOG("%s: data is sending, left %ld\n", __func__, cdc_acm->ctx->transmit_buffer.act_len);
        return -3;
    }

    memcpy(cdc_acm->ctx->transmit_buffer.buf, buf, length);
    cdc_acm->ctx->transmit_buffer.act_len = length;

    // retval = usb_ep_send(USB_CDC_ACM_EP_ID, cdc_acm->ctx->transmit_buffer.buf,
    //                     length, USB_TRANSFER_BULK);
    retval = usb_ep_send(cdc_acm->ep_num, cdc_acm->ctx->transmit_buffer.buf,
                       length, USB_TRANSFER_BULK);
    if (retval < 0)
    {
        USB_LOG("%s: write failed: retval:%d\n", __func__, retval);
        return -4;
    }

    return retval;
}

/**
 * @brief  Retrieves the status of the specific CDC-ACM port.
 * @param  handle Handle pointing to the specific CDC-ACM port.
 * @retval int 0 on success, non-zero on failure.
 *
 * This function retrieves the current status of the CDC-ACM port, including
 * information such as whether the port is open, if data is available for
 * reading, etc.
 */
int usb_cdc_acm_get_status(void *handle)
{
    cdc_acm_t *cdc_acm = (cdc_acm_t *)handle;

    if (!cdc_acm || !cdc_acm->ctx)
    {
        return -1;
    }

    return cdc_acm->state;
}

/**
 * @brief  Configures the specific CDC-ACM port.
 * @param  handle Handle pointing to the specific CDC-ACM port.
 * @param  config Pointer to a configuration structure with the desired
 * settings.
 * @retval int 0 on success, non-zero on failure.
 *
 * This function configures the CDC-ACM port with specific settings, such as
 * baud rate, data bits, stop bits, and parity.
 */
int usb_cdc_acm_set_config(void *handle, cdc_acm_config_t *config)
{
    cdc_acm_t *cdc_acm = (cdc_acm_t *)handle;

    if (!cdc_acm || !cdc_acm->ctx || !config)
    {
        return -1;
    }

    memcpy(&cdc_acm->ctx->config, config, sizeof(cdc_acm_config_t));

    return 0;
}

/**
 * @brief  Registers a callback function for CDC-ACM events.
 * @param  handle Handle pointing to the specific CDC-ACM port.
 * @param  callback Pointer to the callback function that will handle events.
 * @retval int 0 on success, non-zero on failure.
 *
 * This function registers a callback function that will be invoked when certain
 * events occur on the CDC-ACM port, such as data received, connection lost,
 * etc.
 */
int usb_cdc_acm_register_callback(void *handle, cdc_acm_event_callback_t callback)
{
    cdc_acm_t *cdc_acm = (cdc_acm_t *)handle;

    if (!cdc_acm || !cdc_acm->ctx || !callback)
    {
        return -1;
    }

    cdc_acm->ctx->event_callback   = callback;
    cdc_acm->ctx->callback_context = handle;

    return 0;
}