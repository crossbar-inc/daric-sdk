/**
 ******************************************************************************
 * @file    usb_core.c
 * @author  USB Team
 * @brief   USB Core module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the General Purpose Input/Output (USB)
 *          peripheral:
 *           + Initialization and de-initialization functions
 *           + IO operation functions
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "daric_hal_usb.h"
#include "usb_def.h"
#include "usb_core.h"

#define USB_EP_ATTR_MASK 0x03
#define USB_EP_DIR_MASK  0x80

#define USB_EP_IN  0x80
#define USB_EP_OUT 0x00

#define USB_EP_ATTR_CONTROL   0x00
#define USB_EP_ATTR_ISO       0x01
#define USB_EP_ATTR_BULK      0x02
#define USB_EP_ATTR_INTERRUPT 0x03

#define USB_TYPE_MASK      (0x03 << 5)
#define USB_TYPE_STANDARD  (0x00 << 5)
#define USB_TYPE_CLASS     (0x01 << 5)
#define USB_TYPE_VENDOR    (0x02 << 5)
#define USB_TYPE_RESERVED  (0x03 << 5)
#define USB_TYPE_INTERFACE (0X01)
#define USB_TYPE_ENDPOINT  (0X02)
#define USB_TYPE_DEVICE    (0x00)

#ifdef CONFIG_FIDO_USB_HID
#define USB_EP1_BUFSIZE     1024
#define USB_EP2_BUFSIZE     128
#define USB_EP3_BUFSIZE     64
#else
#define USB_EP1_BUFSIZE     1024
#define USB_EP2_BUFSIZE     128
#define USB_EP3_BUFSIZE     1024
#define USB_EP4_BUFSIZE     128

#endif

#define USB_EP1_IN_BUFADDR  (uint32_t) UDC_APP_BUFADDR
#ifdef CONFIG_FIDO_USB_HID
#define USB_EP1_OUT_BUFADDR USB_EP1_IN_BUFADDR + USB_EP1_BUFSIZE
#define USB_EP2_IN_BUFADDR  USB_EP1_OUT_BUFADDR + USB_EP1_BUFSIZE
#define USB_EP2_OUT_BUFADDR USB_EP2_IN_BUFADDR + USB_EP2_BUFSIZE
#define USB_EP3_IN_BUFADDR  USB_EP2_OUT_BUFADDR + USB_EP2_BUFSIZE
#define USB_EP3_OUT_BUFADDR USB_EP3_IN_BUFADDR + USB_EP3_BUFSIZE
#else
#define USB_EP1_OUT_BUFADDR USB_EP1_IN_BUFADDR + USB_EP1_BUFSIZE
#define USB_EP2_IN_BUFADDR USB_EP1_OUT_BUFADDR + USB_EP1_BUFSIZE
#define USB_EP2_OUT_BUFADDR USB_EP2_IN_BUFADDR + USB_EP2_BUFSIZE
#define USB_EP3_IN_BUFADDR USB_EP2_OUT_BUFADDR + USB_EP2_BUFSIZE
#define USB_EP3_OUT_BUFADDR USB_EP3_IN_BUFADDR + USB_EP3_BUFSIZE
#define USB_EP4_IN_BUFADDR USB_EP3_OUT_BUFADDR + USB_EP3_BUFSIZE
#define USB_EP4_OUT_BUFADDR USB_EP4_IN_BUFADDR + USB_EP4_BUFSIZE
#endif

#define CONFIG_DESC_TOTALLEN (USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE + USB_DT_ENDPOINT_SIZE * 2)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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
struct usb_ep_info_t
{
    uint32_t in_addr;
    uint32_t out_addr;
    uint32_t bufsize;
};

struct usb_class_t
{
    int                         ep;
    int                         interface_num;
    struct usb_class_callback_t callbacks;
    void                       *usr_data;
};

struct usb_context_t
{
    uint8_t  cur_interface_num;
    uint8_t  set_tm;
    uint8_t  device_state;
    uint8_t *ep0_buf;
    unsigned u2_RWE : 1;
    unsigned feature_u1_enable : 1;
    unsigned feature_u2_enable : 1;

    int                         alternate_settings[USB_MAX_INTERFACES];
    struct usb_interface_config interfaces[USB_MAX_INTERFACES];

    int                registered_class_count;
    struct usb_class_t registered_classes[USB_MAX_CLASS_TYPES];
};

static struct usb_ep_info_t usb_ep_info_table[] = {
    {USB_EP1_IN_BUFADDR, USB_EP1_OUT_BUFADDR, USB_EP1_BUFSIZE},
    {USB_EP2_IN_BUFADDR, USB_EP2_OUT_BUFADDR, USB_EP2_BUFSIZE},
    {USB_EP3_IN_BUFADDR, USB_EP3_OUT_BUFADDR, USB_EP3_BUFSIZE},
#ifndef CONFIG_FIDO_USB_HID
    {USB_EP4_IN_BUFADDR, USB_EP4_OUT_BUFADDR, USB_EP4_BUFSIZE}
#endif 
};

static const struct usb_device_descriptor device_descriptor = {
    .bLength         = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,

    .bcdUSB             = 0x0200,
#ifdef CONFIG_FIDO_USB_HID
    .bDeviceClass       = 0xEF,
    .bDeviceSubClass    = 0x02,
    .bDeviceProtocol    = 0x01,
#else
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
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

static struct usb_qualifier_descriptor qualifier_descriptor = {
    .bLength         = 0xA,
    .bDescriptorType = 0x6,

    .bcdUSB             = 0x200,
    .bDeviceClass       = 0x0,
    .bDeviceSubClass    = 0x0,
    .bDeviceProtocol    = 0x0,
    .bMaxPacketSize0    = 0x40,
    .bNumConfigurations = 0x1,
    .bRESERVED          = 0,
};

static struct usb_configuration_descriptor other_speed_config_descriptor = {
    /*standard USB configuration*/
    .bLength = USB_DT_CONFIG_SIZE, .bDescriptorType = USB_DT_OTHER_SPEED_CONFIG, .wTotalLength = CONFIG_DESC_TOTALLEN, .bNumInterfaces = 1, .bConfigurationValue = 1,
    .iConfiguration = 0x00,
    .bmAttributes   = 0xc0, // USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_interface_descriptor interface_descriptor[1] = {
    { .bLength            = USB_DT_INTERFACE_SIZE,
      .bDescriptorType    = USB_DT_INTERFACE,
      .bInterfaceNumber   = 0,
      .bAlternateSetting  = 0,
      .bNumEndpoints      = 2,
      .bInterfaceClass    = 0x02,
      .bInterfaceSubClass = 0x02,
      .bInterfaceProtocol = 0x01,
      .iInterface         = 0x00 },
};

static struct usb_endpoint_descriptor hs_other_endpoint_descriptor[2] = {
    { .bLength = USB_DT_ENDPOINT_SIZE, .bDescriptorType = USB_DT_ENDPOINT, .bEndpointAddress = 0x81, .bmAttributes = 0x02, .wMaxPacketSize = 0x40, .bInterval = 0x00 },

    { .bLength = USB_DT_ENDPOINT_SIZE, .bDescriptorType = USB_DT_ENDPOINT, .bEndpointAddress = 0x01, .bmAttributes = 0x02, .wMaxPacketSize = 0x40, .bInterval = 0x00 },
};

static const struct usb_descriptor_header *const hs_other_config_desc[] = {
    (struct usb_descriptor_header *)&other_speed_config_descriptor,

    (struct usb_descriptor_header *)&interface_descriptor[0],

    (struct usb_descriptor_header *)&hs_other_endpoint_descriptor[0],
    (struct usb_descriptor_header *)&hs_other_endpoint_descriptor[1],

};

static struct usb_desc_lang language_id_desc = {
    .header =
        {
            .bLength = sizeof(struct usb_desc_lang),
            .bDescriptorType = 0x3,
        },
    .wLANGID = 0x0409};

static struct usb_desc_str manufacturer_string = {
    .header =
        {
            .bLength = sizeof(struct usb_descriptor_header) + 10,
            .bDescriptorType = 0x3,
        },
    .unicode_string = {'D', 'a', 'r', 'i', 'c'}};

static struct usb_desc_str product_string = {
    .header =
        {
            .bLength = sizeof(struct usb_descriptor_header) + 6,
            .bDescriptorType = 0x3,
        },
    .unicode_string = {'C', 'D', 'C'}};

/* USBD serial string */
static struct usb_desc_str serial_string = {
    .header =
        {
            .bLength = sizeof(struct usb_descriptor_header) + 16,
            .bDescriptorType = 0x3,
        },
    .unicode_string = {'2', '0', '2', '4', '0', '9', '2', '7'}};

static void *const usb_cdc_strings[4]
    = { [0] = (uint8_t *)&language_id_desc, [1] = (uint8_t *)&manufacturer_string, [2] = (uint8_t *)&product_string, [3] = (uint8_t *)&serial_string };

/* Global variable to keep track of USB initialization state */
static int                  usb_initialized = 0;
static struct usb_context_t usb_ctx;

static __inline uint32_t get_usb_ep_bufaddr(uint8_t ep_num, int direction)
{
    struct usb_ep_info_t ep_info = usb_ep_info_table[ep_num - 1];

    if (direction == USB_SEND)
        return ep_info.in_addr;
    else if (direction == USB_RECV)
        return ep_info.out_addr;
    else
        return 0;
}

static __inline uint32_t get_usb_ep_bufsize(uint8_t ep_num)
{
    return usb_ep_info_table[ep_num - 1].bufsize;
}

static void get_status_request(uint8_t request_type, uint16_t index)
{
    uint8_t *ep0_buf    = usb_ctx.ep0_buf;
    uint32_t status_val = 0;
    uint8_t  recipient, ep_num, ep_dir;

    recipient = request_type & 0x1f;
    ep_num    = index & 0x7f;
    ep_dir    = (index & 0x80) ? USB_SEND : USB_RECV;
    switch (recipient)
    {
        case USB_RECIP_DEVICE:
            if (usb_ctx.u2_RWE)
            {
                status_val |= 0x1 << 1;
            }
            if (usb_ctx.feature_u1_enable)
                status_val |= 0x1 << 2;
            if (usb_ctx.feature_u2_enable)
                status_val |= 0x1 << 3;

            status_val |= 0x1;
            *ep0_buf = status_val;
            HAL_USB_Ep0Send(ep0_buf, 2, 0);
            break;
        case USB_RECIP_INTERFACE:
            memset(ep0_buf, 0, 2);
            HAL_USB_Ep0Send(ep0_buf, 2, 0);
            break;
        case USB_RECIP_ENDPOINT:
            if (HAL_USB_EpIsHalted(ep_num, ep_dir))
            {
                *ep0_buf = 1;
                HAL_USB_Ep0Send(ep0_buf, 2, 0);
            }
            else
            {
                *ep0_buf = 0;
                HAL_USB_Ep0Send(ep0_buf, 2, 0);
            }
            break;
        default:
            HAL_USB_EpxHalt(0, USB_RECV);
            break;
    }
}

static void set_feature_request(uint8_t request_type, uint16_t index, uint16_t value)
{
    uint8_t recipient, ep_num, ep_dir;

    recipient = request_type & 0x1f;
    ep_num    = index & 0x7f;
    ep_dir    = (index & 0x80) ? USB_SEND : USB_RECV;

    switch (recipient)
    {
        case USB_RECIP_DEVICE:
            switch (value)
            {
                case 1:
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case 48:
                    usb_ctx.feature_u1_enable = 1;
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case 49:
                    usb_ctx.feature_u2_enable = 1;
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case 2: /* Test mode */
                {
                    uint32_t u_pattern;

                    USB_LOG("USB_DEVICE_TEST_MODE called\n");
                    if (HAL_USB_GetSpeed() > USB_SPEED_HIGH)
                        HAL_USB_EpxHalt(0, USB_RECV);

                    if (usb_ctx.device_state < USB_STATE_DEFAULT)
                        HAL_USB_EpxHalt(0, USB_RECV);

                    HAL_USB_Ep0Send(NULL, 0, 0);

                    u_pattern = index >> 8;

                    /* TESTMODE is only defined for high speed device */
                    if (HAL_USB_GetSpeed() == USB_SPEED_HIGH)
                    {
                        USB_LOG("high speed test mode enter\n");
                        usb_ctx.set_tm = u_pattern;

                        if (usb_ctx.set_tm != 0)
                        {
                            USB_LOG("test pattern is %ld\n", u_pattern);
                            HAL_USB_SetTestMode(usb_ctx.set_tm);
                            usb_ctx.set_tm = 0;
                        }
                    }
                    break;
                }

                default:
                    HAL_USB_EpxHalt(0, USB_RECV);
                    break;
            }
            break;
        case USB_RECIP_INTERFACE:
            HAL_USB_Ep0Send(NULL, 0, 0);
            break;
        case USB_RECIP_ENDPOINT:
            if (!HAL_USB_EpIsHalted(ep_num, ep_dir))
            {
                HAL_USB_EpxHalt(ep_num, ep_dir);
            }
            HAL_USB_Ep0Send(NULL, 0, 0);
            break;
        default:
            HAL_USB_EpxHalt(0, USB_RECV);
            break;
    }
}

static void clear_feature_request(uint8_t request_type, uint16_t index, uint16_t value)
{
    uint8_t recipient, ep_num, ep_dir;

    recipient = request_type & 0x1f;
    ep_num    = index & 0x7f;
    ep_dir    = (index & 0x80) ? USB_SEND : USB_RECV;
    switch (recipient)
    {
        case USB_RECIP_DEVICE:
            switch (value)
            {
                case 1:
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case 48:
                    usb_ctx.feature_u1_enable = 0;
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case 49:
                    usb_ctx.feature_u2_enable = 0;
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                default:
                    HAL_USB_EpxHalt(0, USB_RECV);
                    break;
            }
            break;
        case USB_RECIP_INTERFACE:
            HAL_USB_Ep0Send(NULL, 0, 0);
            break;
        case USB_RECIP_ENDPOINT:
            if (HAL_USB_EpIsHalted(ep_num, ep_dir))
            {
                HAL_USB_ResetSeqNumber(ep_num, ep_dir);
                HAL_USB_EpxUnhalt(ep_num, ep_dir);
            }

            HAL_USB_Ep0Send(NULL, 0, 0);
            break;
        default:
            HAL_USB_EpxHalt(0, USB_RECV);
            break;
    }
}

static struct usb_configuration_descriptor *get_configuration_descriptor(uint16_t wValue)
{
    struct usb_configuration_descriptor *config_desc = 0;
    uint8_t                             *ep0_buf     = usb_ctx.ep0_buf;
    uint32_t                             offset      = 0;

    /* Validate configuration value, adjust if more configurations exist */
    if (wValue < 1 || wValue > USB_MAX_CONFIGURATIONS)
    {
        USB_LOG("Invalid configuration value: %d\n", wValue);
        return NULL;
    }

    memset(ep0_buf, 0, sizeof(ep0_buf));

    /* Based on wValue, select which configuration descriptor to apply */
    if (wValue == 1)
    {
        for (int i = 0; i < usb_ctx.registered_class_count; i++)
        {
            if (usb_ctx.registered_classes[i].callbacks.get_config_descriptor)
            {
                offset += usb_ctx.registered_classes[i].callbacks.get_config_descriptor(ep0_buf + offset, UDC_EP0_REQBUFSIZE - offset);
            }
        }
    }
    else
    {
        USB_LOG("%s: not supported\n", __func__);
    }

    /* Ensure the total length is correctly set */
    config_desc               = (struct usb_configuration_descriptor *)ep0_buf;
    config_desc->wTotalLength = offset;

    /* Return the generated configuration descriptor */
    return (struct usb_configuration_descriptor *)ep0_buf;
}

static int get_descriptor_request(uint16_t value, uint16_t index, uint16_t length)
{
    struct usb_context_t *ctx      = &usb_ctx;
    uint8_t              *ep0_buf  = usb_ctx.ep0_buf;
    uint32_t              offset   = 0;
    int                   buffsize = 0;
    int                   i        = 0;

    switch (value >> 8)
    {
        case USB_DT_DEVICE:
            if (ctx->registered_class_count == 1)
            {
                /* only 1 class registered */
                offset = ctx->registered_classes[0].callbacks.get_dev_descriptor(ep0_buf, length);
            }
            else
            {
                /* IAD used */
                offset = sizeof(device_descriptor);
                memcpy((uint8_t *)ep0_buf, &device_descriptor, offset);
            }
            buffsize = min_t(int, length, offset);
            HAL_USB_Ep0Send((uint8_t *)ep0_buf, buffsize, 0);
            break;

        case USB_DT_DEVICE_QUALIFIER:
            USB_LOG("USB_DT_DEVICE_QUALIFIER\n");

            buffsize = min_t(int, length, sizeof(qualifier_descriptor));
            memcpy((uint8_t *)ep0_buf, &qualifier_descriptor, buffsize);

            HAL_USB_Ep0Send((uint8_t *)ep0_buf, buffsize, 0);
            break;
        case USB_DT_OTHER_SPEED_CONFIG:
            USB_LOG("USB_DT_OTHER_SPEED_CONFIG\n");
            offset = 0;

            for (i = 0; i < ARRAY_SIZE(hs_other_config_desc); i++)
            {
                memcpy((uint8_t *)(ep0_buf + offset), hs_other_config_desc[i], hs_other_config_desc[i]->bLength);
                offset += hs_other_config_desc[i]->bLength;
            }

            ((struct usb_configuration_descriptor *)ep0_buf)->wTotalLength = offset;
            buffsize                                                       = min_t(int, length, offset);
            HAL_USB_Ep0Send((uint8_t *)ep0_buf, buffsize, 0);

            break;
        case USB_DT_CONFIG:
            USB_LOG("USB_DT_CONFIG\n");
            offset = 0;

            if (usb_ctx.registered_class_count == 1)
            {
                if (usb_ctx.registered_classes[0].callbacks.get_config_descriptor)
                {
                    offset = usb_ctx.registered_classes[0].callbacks.get_config_descriptor(ep0_buf, length);
                }

                ((struct usb_configuration_descriptor *)ep0_buf)->wTotalLength = offset;
            }
            else
            {
                struct usb_configuration_descriptor *config_desc = (struct usb_configuration_descriptor *)ep0_buf;
                config_desc->bLength                             = sizeof(struct usb_configuration_descriptor);
                config_desc->bDescriptorType                     = USB_DT_CONFIG;

                offset = config_desc->bLength;

                for (i = 0; i < usb_ctx.registered_class_count; i++)
                {
                    if (usb_ctx.registered_classes[i].callbacks.get_interface_ass_descriptor)
                    {
                        if (length > offset)
                            offset += usb_ctx.registered_classes[i].callbacks.get_interface_ass_descriptor(ep0_buf + offset, length - offset);
                        else
                            offset += usb_ctx.registered_classes[i].callbacks.get_interface_ass_descriptor(ep0_buf + offset, 0);
                    }
                    if (usb_ctx.registered_classes[i].callbacks.get_interface_descriptor)
                    {
                        if (length > offset)
                            offset += usb_ctx.registered_classes[i].callbacks.get_interface_descriptor(ep0_buf + offset, length - offset);
                        else
                            offset += usb_ctx.registered_classes[i].callbacks.get_interface_descriptor(ep0_buf + offset, 0);
                    }
                }

                config_desc->wTotalLength        = offset;

#ifndef CONFIG_FIDO_USB_HID
                config_desc->bNumInterfaces = usb_ctx.registered_class_count + 2;
#else
                config_desc->bNumInterfaces      = usb_ctx.registered_class_count + 1;
#endif
                config_desc->bConfigurationValue = 1;
                config_desc->iConfiguration      = 0;
                config_desc->bmAttributes        = 0xC0;
                config_desc->bMaxPower           = 0x32;
            }

            buffsize = min_t(int, length, offset);
            HAL_USB_Ep0Send((uint8_t *)ep0_buf, buffsize, 0);

            break;
        case USB_DT_STRING: {
            USB_LOG("USB_DT_STRING\n");
            uint8_t              id = (value & 0xFF);
            struct usb_desc_str *desc;

            if (id >= 0 && id < 4)
            {
                desc     = (struct usb_desc_str *)usb_cdc_strings[id];
                buffsize = min_t(int, length, desc->header.bLength);

                memcpy(ep0_buf, desc, buffsize);
            }
            else
            {
                desc     = (struct usb_desc_str *)usb_cdc_strings[0];
                buffsize = min_t(int, length, desc->header.bLength);
                memcpy(ep0_buf, desc, buffsize);
            }

            HAL_USB_Ep0Send((uint8_t *)ep0_buf, buffsize, 0);
            break;
        }
        default:
            USB_LOG("default\n");
            HAL_USB_Ep0Send(0, 0, 0);
            break;
    }

    return 0;
}

static void usb_ep_complete(int ep_num, uint8_t direction, uint8_t *buf, uint32_t len, uint8_t error)
{
    for (int i = 0; i < usb_ctx.registered_class_count; i++)
    {
        if (usb_ctx.registered_classes[i].ep == ep_num)
        {
            if (usb_ctx.registered_classes[i].callbacks.ep_complete)
            {
                usb_ctx.registered_classes[i].callbacks.ep_complete(ep_num, direction, buf, len & 0xffff, error);
            }
        }
    }
}

static void usb_apply_configuration(const struct usb_configuration_descriptor *config_desc)
{
    const struct usb_endpoint_descriptor *ep_desc;
    const struct usb_descriptor_header   *header       = (const struct usb_descriptor_header *)config_desc;
    uint16_t                              total_length = config_desc->wTotalLength;
    uint16_t                              offset       = 0;

    while (offset < total_length)
    {
        switch (header->bDescriptorType)
        {
            case USB_DT_INTERFACE:
                break;
            case USB_DT_ENDPOINT: {
                ep_desc                  = (const struct usb_endpoint_descriptor *)header;
                uint8_t  ep_addr         = ep_desc->bEndpointAddress;
                uint16_t max_packet_size = ep_desc->wMaxPacketSize;
                uint8_t  ep_type         = ep_desc->bmAttributes & USB_EP_ATTR_MASK;
                uint8_t  dir             = ((ep_addr & USB_EP_DIR_MASK) == USB_EP_IN ? USB_SEND : USB_RECV);

                USB_LOG("%s: ep_type:%d, dir:%d, max_size:%d, ep_addr:%d\n", __func__, ep_type, dir, max_packet_size, ep_addr & USB_EP_NUMBER_MASK);

                HAL_USB_Ep_CompRegister(ep_addr & USB_EP_NUMBER_MASK, dir, usb_ep_complete);
                HAL_USB_EpxEnable(ep_addr & USB_EP_NUMBER_MASK, ep_type, dir, max_packet_size);

                if (dir == USB_RECV)
                {
                    uint8_t *ep_buf     = (uint8_t *)get_usb_ep_bufaddr(ep_addr & USB_EP_NUMBER_MASK, USB_RECV);
                    uint32_t ep_buf_len = get_usb_ep_bufsize(ep_addr & USB_EP_NUMBER_MASK);
                    switch (ep_type)
                    {
                        case USB_BULK_ENDPOINT:
                            ep_type = USB_TRANSFER_BULK;
                            break;
                        case USB_INTERRUPT_ENDPOINT:
                            ep_type = USB_TRANSFER_INTERRUPT;
                            break;
                        case USB_ISOCHRONOUS_ENDPOINT:
                            ep_type = USB_TRANSFER_ISOCHRONOUS;
                            break;
                        default:
                            USB_LOG("%s: not support type: %d\n", __func__, ep_type);
                            ep_type = USB_TRANSFER_BULK;
                            break;
                    }
                    HAL_USB_EpxReceive(ep_addr & USB_EP_NUMBER_MASK, ep_buf, ep_buf_len, ep_type);
                }
            }
            break;
        }

        offset += header->bLength;
        header = (const struct usb_descriptor_header *)((uint8_t *)header + header->bLength);
    }
}

static void usb_handle_setup(uint8_t *setup)
{
    struct usb_control_request          *setup_pkt   = (struct usb_control_request *)setup;
    struct usb_configuration_descriptor *config_desc = 0;
    uint16_t                             wValue      = setup_pkt->wValue;
    uint16_t                             wIndex      = setup_pkt->wIndex;
    uint16_t                             wLength     = setup_pkt->wLength;
    uint8_t                             *ep0_buf     = usb_ctx.ep0_buf;

    USB_LOG("bmRequest: %x, bRequest:%x, wValue:%x, wIndex:%x, wLength:%x\n", setup_pkt->bmRequestType, setup_pkt->bRequest, setup_pkt->wValue, setup_pkt->wIndex,
            setup_pkt->wLength);

    if ((setup_pkt->bmRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
    {
        USB_LOG("USB_TYPE_STANDARD\n");
        if ((setup_pkt->bmRequestType & 0xF) == USB_TYPE_DEVICE)
        {
            USB_LOG("USB_TYPE_DEVICE\n");
            switch (setup_pkt->bRequest)
            {
                case USB_REQ_GET_STATUS:
                    USB_LOG("USB_REQ_GET_STATUS\n");
                    get_status_request(setup_pkt->bmRequestType, wIndex);
                    break;
                case USB_REQ_SET_ADDRESS:
                    USB_LOG("USB_REQ_SET_ADDRESS\n");
                    HAL_USB_SetAddress(wValue, 0);
                    usb_ctx.device_state = USB_STATE_ADDRESS;
                    break;
                case USB_REQ_SET_SEL:
                    USB_LOG("USB_REQ_SET_SEL\n");
                    /* FIXME: should be complete in ep0 complete function */
                    HAL_USB_Ep0Receive((uint8_t *)ep0_buf, wLength, 0);
                    /*do set sel*/
                    HAL_USB_Set_Sel(*ep0_buf, *(ep0_buf + 1), *(uint16_t *)(ep0_buf + 2), *(uint16_t *)(ep0_buf + 4));
                    break;
                case USB_REQ_SET_ISOCH_DELAY:
                    USB_LOG("USB_REQ_SET_ISOCH_DELAY\n");
                    /*do set isoch delay*/
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case USB_REQ_CLEAR_FEATURE:
                    USB_LOG("USB_REQ_CLEAR_FEATURE\n");
                    /* do clear feature */
                    clear_feature_request(setup_pkt->bmRequestType, wIndex, wValue);
                    break;
                case USB_REQ_SET_FEATURE:
                    USB_LOG("USB_REQ_SET_FEATURE\n");
                    /* do set feature */
                    if (usb_ctx.device_state == USB_STATE_CONFIGURED)
                        set_feature_request(setup_pkt->bmRequestType, wIndex, wValue);
                    else
                        HAL_USB_EpxHalt(0, USB_RECV);
                    break;
                case USB_REQ_SET_CONFIGURATION:
                    USB_LOG("USB_REQ_SET_CONFIGURATION\n");

                    if (wValue == 0)
                    {
                        usb_ctx.device_state = USB_STATE_ADDRESS;
                    }
                    else if (wValue == 1)
                    {
                        config_desc = get_configuration_descriptor(wValue);
                        usb_apply_configuration(config_desc);
                        usb_ctx.device_state = USB_STATE_CONFIGURED;
                    }
                    else
                    {
                        HAL_USB_EpxHalt(0, USB_RECV);
                        break;
                    }

                    HAL_USB_Ep0Send(NULL, 0, 0);

                    break;
                case USB_REQ_GET_DESCRIPTOR:
                    USB_LOG("USB_REQ_GET_DESCRIPTOR\n");
                    get_descriptor_request(wValue, wIndex, wLength);
                    break;
                case USB_REQ_GET_CONFIGURATION:
                    USB_LOG("USB_REQ_GET_CONFIGURATION\n");
                    if (usb_ctx.device_state != USB_STATE_CONFIGURED)
                    {
                        *ep0_buf = 0;
                        HAL_USB_Ep0Send((uint8_t *)ep0_buf, 1, 0);
                    }
                    else
                    {
                        *ep0_buf = 1;
                        HAL_USB_Ep0Send((uint8_t *)ep0_buf, 1, 0);
                    }
                    break;
                case USB_REQ_SET_INTERFACE:
                    USB_LOG("USB_REQ_SET_INTERFACE\n");
                    usb_ctx.cur_interface_num = wValue & 0xF;
                    USB_LOG("USB_REQ_SET_INTERFACE altsetting %x\n", usb_ctx.cur_interface_num);
                    HAL_USB_Ep0Send(NULL, 0, 0);
                    break;
                case USB_REQ_GET_INTERFACE:
                    USB_LOG("USB_REQ_GET_INTERFACE\n");
                    *ep0_buf = usb_ctx.cur_interface_num;
                    HAL_USB_Ep0Send((uint8_t *)ep0_buf, 1, 0);
                    break;
                default:
                    USB_LOG("USB_REQ default bRequest=%x, bmRequestType=%x\n", setup_pkt->bRequest, setup_pkt->bmRequestType);
                    HAL_USB_EpxHalt(0, USB_RECV);
            }
        }
        else if ((setup_pkt->bmRequestType & 0xF) == USB_TYPE_INTERFACE)
        {
            USB_LOG("USB_TYPE_INTERFACE\n");
            if (usb_ctx.registered_class_count == 1)
            {
                /* only 1 class registered */
                usb_ctx.registered_classes[0].callbacks.request_handler(setup_pkt);
            }
            else
            {
                for (int i = 0; i < usb_ctx.registered_class_count; i++)
                {
                    if (usb_ctx.registered_classes[i].interface_num == (setup_pkt->wIndex & 0xFF))
                    {
                        usb_ctx.registered_classes[i].callbacks.request_handler(setup_pkt);
                    }
                }
            }
        }
        else if ((setup_pkt->bmRequestType & 0xF) == USB_TYPE_ENDPOINT)
        {
            USB_LOG("USB_TYPE_ENDPOINT\n");
            if (usb_ctx.registered_class_count == 1)
            {
                /* only 1 class registered */
                usb_ctx.registered_classes[0].callbacks.request_handler(setup_pkt);
            }
            else
            {
                for (int i = 0; i < usb_ctx.registered_class_count; i++)
                {
                    if (usb_ctx.registered_classes[i].interface_num == (setup_pkt->wIndex & 0xFF))
                    {
                        usb_ctx.registered_classes[i].callbacks.request_handler(setup_pkt);
                    }
                }
            }
        }
        else
        {
            USB_LOG("%s: Vendor specified request:%x\n", __func__, setup_pkt->bmRequestType);
            HAL_USB_EpxHalt(0, USB_RECV);
        }
    }
    else if ((setup_pkt->bmRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS)
    {
        USB_LOG("USB_TYPE_CLASS\n");
        if (usb_ctx.registered_class_count == 1)
        {
            /* only 1 class registered */
            usb_ctx.registered_classes[0].callbacks.request_handler(setup_pkt);
        }
        else
        {
            for (int i = 0; i < usb_ctx.registered_class_count; i++)
            {
                if (usb_ctx.registered_classes[i].interface_num == (setup_pkt->wIndex & 0xFF))
                {
                    usb_ctx.registered_classes[i].callbacks.request_handler(setup_pkt);
                }
            }
        }
    }
    else
    {
        HAL_USB_EpxHalt(0, USB_RECV);
    }
}

static void usb_ep0_complete(uint8_t *buf, uint32_t len, uint8_t error)
{
    if (error)
    {
        USB_LOG("EP0 transfer error: %x\n", error);
        return;
    }

    for (int i = 0; i < usb_ctx.registered_class_count; i++)
    {
        if (usb_ctx.registered_classes[i].callbacks.ep0_complete)
        {
            usb_ctx.registered_classes[i].callbacks.ep0_complete(0, 0, buf, len & 0xffff, error);
        }
    }
}

static void usb_power_state(uint8_t state)
{
    switch (state)
    {
        case USB_POWER_STATE_U3:
            /* TODO: workaround for usb plugout notification */
            usb_ctx.device_state = USB_STATE_SUSPENDED;
            break;
        default:
            break;
    }
    USB_LOG("USB power state: %d, device_state:%d\n", state, usb_ctx.device_state);
}

/**
 * @brief Initializes the USB core, preparing it for operation.
 * @param None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function initializes the USB core, setting up necessary data structures,
 * Allocating resources, and preparing the device for enumeration by a USB host.
 */
int usb_initialize(void)
{
    HAL_USB_CtxTypeDef udc_ctx;

    if (usb_initialized)
    {
        USB_LOG("USB core already initialized.\n");
        return 0;
    }

    udc_ctx.handle_setup       = usb_handle_setup;
    udc_ctx.ep0_compl_func_ptr = usb_ep0_complete;
    udc_ctx.power_state_notify = usb_power_state;
    udc_ctx.max_speed          = USB_SPEED_HIGH;

    USB_LOG("Setting up USB core...\n");

    usb_ctx.ep0_buf = UDC_EP0_BUF_ADDR;

    HAL_USB_Reset();
    HAL_USB_Initialize(&udc_ctx);
    HAL_USB_EnableInt();

    usb_initialized = 1;

    USB_LOG("USB core initialized successfully.\n");

    return 0;
}

/**
 * @brief Uninitializes the USB core, releasing allocated resources.
 * @param None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function uninitializes the USB core, cleaning up resources that were
 * allocated during initialization. It should be called before the system shuts
 * down or when the USB functionality is no longer needed.
 */
int usb_uninitialize(void)
{
    if (!usb_initialized)
    {
        // USB core is not initialized
        USB_LOG("USB core is not initialized.\n");
        return 0;
    }

    USB_LOG("Cleaning up USB core...\n");

    HAL_USB_DisableInt();
    HAL_USB_Reset();
    HAL_USB_Uninitialize();

    usb_ctx.ep0_buf = 0;
    usb_initialized = 0;

    USB_LOG("USB core uninitialized successfully.\n");

    return 0;
}

/**
 * @brief Connect the USB device from the bus.
 * @param None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function simulates a connection from the USB bus by enabling the USB
 * transceiver. It is useful for scenarios where the device needs to be
 * logically removed from the bus.
 */
int usb_connect(void)
{
    if (usb_initialized)
        HAL_USB_Start();
    else
        USB_LOG("%s: usb is not initialized\n", __func__);

    return 0;
}

/**
 * @brief Disconnect the USB device from the bus.
 * @param None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function simulates a disconnection from the USB bus by disabling the USB
 * transceiver. It is useful for scenarios where the device needs to be
 * logically removed from the bus.
 */
int usb_disconnect(void)
{
    if (usb_initialized)
        HAL_USB_Stop();

    return 0;
}

/**
 * @brief  Registers a USB class driver with the USB core.
 * @param  ep ID of the ep.
 * @param  callback Pointer to the callback function for handling class-specific
 * events.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function allows the registration of a USB class driver with the USB
 * core. The class driver will receive notifications of class-specific events
 * through the callback.
 */
int usb_class_register(int ep, struct usb_class_callback_t cb)
{
    struct usb_context_t *ctx = &usb_ctx;

    if (ctx->registered_class_count >= USB_MAX_CLASS_TYPES)
    {
        return -1;
    }

    for (int i = 0; i < ctx->registered_class_count; i++)
    {
        if (ctx->registered_classes[i].ep == ep)
        {
            return -2;
        }
    }

    ctx->registered_classes[ctx->registered_class_count].ep        = ep;
    ctx->registered_classes[ctx->registered_class_count].callbacks = cb;

    switch (ep)
    {
        case USB_CDC_ACM_EP_NUM:
            ctx->registered_classes[ctx->registered_class_count].interface_num = USB_CDC_ACM_INTERFACE;
            break;

#ifndef CONFIG_FIDO_USB_HID
  case USB_CDC_ACM_EP_MPC_NUM:
    ctx->registered_classes[ctx->registered_class_count].interface_num = USB_CDC_ACM_MPC_CMD_INTERFACE;
    break;
#else
        case USB_HID_EP_NUM:
            ctx->registered_classes[ctx->registered_class_count].interface_num = USB_HID_INTERFACE;
            break;
#endif
        default:
            USB_LOG("%s: not support class type: %d\n", __func__, ep);
            return -1;
    }

    ctx->registered_class_count++;

    return 0;
}

/**
 * @brief  Unregisters a USB class driver from the USB core.
 * @param  ep ID of the ep to unregister.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function removes a previously registered USB class driver from the USB
 * core. Once unregistered, the class driver will no longer receive
 * class-specific events.
 */
int usb_class_unregister(int ep)
{
    struct usb_context_t *ctx = &usb_ctx;
    int                   i, j;

    for (i = 0; i < ctx->registered_class_count; i++)
    {
        if (ctx->registered_classes[i].ep == ep)
        {
            ctx->registered_classes[i].ep = 0;

            for (j = i; j < ctx->registered_class_count - 1; j++)
            {
                ctx->registered_classes[j] = ctx->registered_classes[j + 1];
            }

            ctx->registered_class_count--;
            memset(&ctx->registered_classes[ctx->registered_class_count], 0, sizeof(struct usb_class_t));
            ctx->registered_classes[ctx->registered_class_count].usr_data = 0;

            return 0;
        }
    }

    return -1;
}

/**
 * @brief  Retrieves the current status of the USB device.
 * @param  None.
 * @retval int The current status as a bitmask, or a negative value on error.
 *
 * This function returns the current status of the USB device, including whether
 * it is self-powered, whether remote wakeup is enabled, and other status flags.
 */
int usb_status_get(void)
{
    return usb_ctx.device_state;
}

/**
 * @brief  Sends data over a specified USB endpoint.
 * @param  ep_num: The USB endpoint number.
 * @param  buf: Pointer to the data buffer to be sent.
 * @param  len: Length of the data to be sent, in bytes.
 * @param  trans_type: Type of USB transfer (e.g., control, bulk, interrupt,
 * isochronous).
 * @retval int Number of bytes sent on success, or a 0 value on error.
 *
 * This function sends data over the specified USB endpoint. It takes the
 * endpoint number, data buffer, buffer length, and transfer type as parameters.
 */
int usb_ep_send(uint8_t ep_num, uint8_t *buf, uint32_t len, int trans_type)
{
    uint8_t *ep_buf      = 0;
    uint32_t ep_buf_len  = 0;
    uint32_t ep_send_len = 0;

    if (usb_ctx.device_state != USB_STATE_CONFIGURED)
    {
        USB_LOG("%s: usb is not ready, can't send data, state(%d)\n", __func__, usb_ctx.device_state);
        return -1;
    }

    if (ep_num == 0)
    {
        ep_buf     = usb_ctx.ep0_buf;
        ep_buf_len = UDC_EP0_REQBUFSIZE;

        ep_send_len = ep_buf_len < len ? ep_buf_len : len;
        memcpy(ep_buf, buf, ep_send_len);
        HAL_USB_Ep0Send(ep_buf, ep_send_len, 0);
    }
    else
    {
        USB_TransferTypeDef type;

        ep_buf     = (uint8_t *)get_usb_ep_bufaddr(ep_num, USB_SEND);
        ep_buf_len = get_usb_ep_bufsize(ep_num);

        ep_send_len = ep_buf_len < len ? ep_buf_len : len;
        memcpy(ep_buf, buf, ep_send_len);

        switch (trans_type)
        {
            case USB_TRANS_TYPE_BULK:
                type = USB_TRANSFER_BULK;
                break;
            case USB_TRANS_TYPE_INT:
                type = USB_TRANSFER_INTERRUPT;
                break;
            default:
                USB_LOG("%s: transfer type(%d) not support\n", __func__, trans_type);
                return -1;
        }
        HAL_USB_EpxSend(ep_num, ep_buf, ep_send_len, type);
    }

    return ep_send_len;
}

/**
 * @brief  Prepare receiving data from a specified USB endpoint.
 * @param  ep_num: The USB endpoint number.
 * @param  trans_type: Type of USB transfer (e.g., control, bulk, interrupt,
 * isochronous).
 * @retval int 0 on success, or a negative value on error.
 *
 * This function receives data from the specified USB endpoint and stores it in
 * the provided buffer. It takes the endpoint number, buffer, buffer length, and
 * transfer type as parameters.
 */
int usb_ep_recv_prepare(uint8_t ep_num, int trans_type)
{
    uint8_t *ep_buf     = 0;
    uint32_t ep_buf_len = 0;

    if (usb_ctx.device_state != USB_STATE_CONFIGURED)
    {
        USB_LOG("%s: usb is not ready, can't recv data, state(%d)\n", __func__, usb_ctx.device_state);
        return -1;
    }

    if (ep_num == 0)
    {
        ep_buf     = usb_ctx.ep0_buf;
        ep_buf_len = UDC_EP0_REQBUFSIZE;

        HAL_USB_Ep0Receive(ep_buf, ep_buf_len, 0);
    }
    else
    {
        USB_TransferTypeDef type;

        ep_buf     = (uint8_t *)get_usb_ep_bufaddr(ep_num, USB_RECV);
        ep_buf_len = get_usb_ep_bufsize(ep_num);

        switch (trans_type)
        {
            case USB_TRANS_TYPE_BULK:
                type = USB_TRANSFER_BULK;
                break;
            case USB_TRANS_TYPE_INT:
                type = USB_TRANSFER_INTERRUPT;
                break;
            default:
                USB_LOG("%s: transfer type(%d) not support\n", __func__, trans_type);
                return -1;
        }
        HAL_USB_EpxReceive(ep_num, ep_buf, ep_buf_len, type);
    }

    return 0;
}

/**
 * @brief  Wakes up the USB device if it is in a suspended state.
 * @param  None.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function wakes up the USB device if it has been suspended by the host.
 * It is typically used in conjunction with remote wakeup functionality,
 * allowing the device to signal the host to resume normal operation.
 */
int usb_wakeup(void)
{
    return 0;
}

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
int usb_suspend(void)
{
    return 0;
}