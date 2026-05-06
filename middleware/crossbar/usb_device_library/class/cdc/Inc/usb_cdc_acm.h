/**
 ******************************************************************************
 * @file    usb_cdc_acm.h
 * @author  USB Team
 * @brief   Header file of USB CDC module.
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
#ifndef __USB_CDC_ACM_H
#define __USB_CDC_ACM_H

#ifdef __cplusplus
extern "C"
{
#endif
/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

#define USB_CDC_ACM_BUF_SIZE 1024

#define USB_CDC_ACM_EP_CMD 0x82
#define USB_CDC_ACM_EP_MPC_CMD 0x84
#define USB_CDC_ACM_EP_IN 0x81
#define USB_CDC_ACM_EP_MPC_IN 0x83
#define USB_CDC_ACM_EP_OUT 0x01
#define USB_CDC_ACM_EP_MPC_OUT 0x03
#define USB_CDC_ACM_EP_ID 0x01
#define USB_CDC_ACM_EP_MPC_ID 0x03

    typedef struct
    {
        uint32_t baud_rate; // Baud rate for serial communication (e.g., 9600, 115200)
        uint8_t  stop_bits; // Number of stop bits (1 or 2)
        uint8_t  parity;    // Parity setting (0: None, 1: Odd, 2: Even)
        uint8_t  data_bits; // Number of data bits (typically 8)
    } __attribute__((packed)) cdc_acm_config_t;

    typedef enum
    {
        CDC_ACM_EVENT_DATA_RECEIVED,      // Data received event
        CDC_ACM_EVENT_DATA_SENT,          // Data has been sent
        CDC_ACM_EVENT_CONNECTION_LOST,    // Connection lost event
        CDC_ACM_EVENT_LINE_STATE_CHANGED, // Line state changed (e.g., DTR/RTS)
        CDC_ACM_EVENT_ERROR               // General error event
    } cdc_acm_event_e;

    typedef enum
    {
        CDC_ACM_PORT_OPEN,
        CDC_ACM_PORT_CLOSE,
        CDC_ACM_PORT_ERROR
    } cdc_acm_state_e;

    /**
     * @brief  Callback function prototype for CDC-ACM events.
     * @param  handle Pointer to the specific CDC-ACM port handle where the event
     * occurred.
     * @param  event The event type that occurred (e.g., data received, connection
     * lost).
     * @param  context Pointer to user-defined context information associated with
     * the event.
     * @retval None
     *
     * This callback function is called by the CDC-ACM driver when an event occurs
     * on the CDC-ACM port. The event type and port handle are prodsuvided, allowing
     * the user to handle different types of events accordingly. The `context`
     * parameter allows for the passing of additional user-defined data to the
     * callback.
     */
    typedef void (*cdc_acm_event_callback_t)(void *handle, cdc_acm_event_e event, void *context);

    /**
     * @brief  CDC-ACM class initialization (e.g., class registration, resource
     * allocation).
     * @retval 0 on success, non-zero on failure.
     *
     * This function initializes the CDC-ACM class. It should be called before any
     * other CDC-ACM functions to set up the necessary resources and register the
     * class.
     */
    int usb_cdc_acm_init(void);

    /**
     * @brief  CDC-ACM class uninitialization (e.g., class unregistration, resource
     * deallocation).
     * @retval 0 on success, non-zero on failure.
     *
     * This function uninitializes the CDC-ACM class. It should be called to clean
     * up resources when the CDC-ACM functionality is no longer needed.
     */
    int usb_cdc_acm_uninit(void);

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
    void *usb_cdc_acm_open(int port);

    /**
     * @brief  Closes the specific CDC-ACM port.
     * @param  handle Handle pointing to the specific CDC-ACM port.
     * @retval None.
     *
     * This function closes the specified CDC-ACM port. The handle is no longer
     * valid after this function is called.
     */
    void usb_cdc_acm_close(void *handle);

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
    int usb_cdc_acm_read(void *handle, void *buf, size_t length);

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
    int usb_cdc_acm_write(void *handle, void *buf, size_t length);

    /**
     * @brief  Retrieves the status of the specific CDC-ACM port.
     * @param  handle Handle pointing to the specific CDC-ACM port.
     * @retval status of cdc-acm port
     *
     * This function retrieves the current status of the CDC-ACM port, including
     * information such as whether the port is open, if data is available for
     * reading, etc.
     */
    int usb_cdc_acm_get_status(void *handle);

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
    int usb_cdc_acm_set_config(void *handle, cdc_acm_config_t *config);

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
    int usb_cdc_acm_register_callback(void *handle, cdc_acm_event_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_ACM_H */