/**
 ******************************************************************************
 * @file    usb_service.c
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
#include "daric_pm.h"

#include "tx_api.h"
#include <stdint.h>
#include <stdio.h>
#include "user_threads_attrdef.h"
#include "usb_cdc_acm.h"
#include "usb_core.h"


#ifdef CONFIG_FIDO_USB_HID
#include "usb_hid.h"
#endif

#include "system_daric.h"
#include "cmsis_gcc.h"
#include <stdarg.h>
#include "tg28.h"
#include "usb_service.h"

#include "user_threads_attrdef.h"
#include "tx_log.h"

#include "daric_activecard_nto_usb.h"
#ifdef CONFIG_DARIC_GUIX
#include "connection.h"
#include "daric_gui.h"
#endif

#include "daric_pmu.h"
#include "sys_config.h"

#undef LOG_TAG
#define LOG_TAG "USB_SERV"

void *cdc_acm_handle;
void *cdc_acm_mpc_handle;
static TX_EVENT_FLAGS_GROUP usb_event_flags;
TX_EVENT_FLAGS_GROUP mpc_event_flags;
static TX_EVENT_FLAGS_GROUP usb_plug_event_flags;
bool is_usb_inserted;
bool is_usb_config;

#define USB_EVENT_RECV 0x1
#define USB_EVENT_SENT 0x2
#define USB_EVENT_MPC_RECV 0x04
#define USB_EVENT_MPC_SENT 0x08
#define USB_EVENT_RECV_HID 0x10
#define USB_EVENT_PLUG_IN 0x1
#define USB_EVENT_PLUG_OUT 0x2

/* Wait event definition */
typedef enum {
    USB_CDC_DATA_EVENT  = 1 << 0,  // wait cdc event
    USB_FIDO_DATA_EVENT = 1 << 1,  // wait fido event
} WaitEventDataService;

void thread_usb_service_entry(ULONG thread_input);

/**
 * @brief  Callback function for usb plug
 * @param status usb plug in or out
 * @retval void.
 */
void usb_detect_callback(int16_t status)
{
    LOGV("%s, status: %d", __func__, status);
    if (status == BSP_USB_INSERT)
    {
        BSP_PM_PWR_en(PM_PWR_ALDO3, true);
        tx_event_flags_set(&usb_plug_event_flags, USB_EVENT_PLUG_IN, TX_OR);
    }
    else
    {
        BSP_PM_PWR_en(PM_PWR_ALDO3, false);
        tx_event_flags_set(&usb_plug_event_flags, USB_EVENT_PLUG_OUT, TX_OR);
    }
}

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
void cdc_acm_event_callback(void *handle, cdc_acm_event_e event,
                            void *context)
{
    if (event == CDC_ACM_EVENT_DATA_SENT)
    {
        
        tx_event_flags_set(&usb_event_flags, USB_EVENT_SENT, TX_OR);
        
    }
    else if (event == CDC_ACM_EVENT_DATA_RECEIVED)
    {
       
        tx_event_flags_set(&usb_event_flags, USB_EVENT_RECV, TX_OR);
        
    }
    else
    {
        LOGV("%s: event:%d, not process", __func__, event);
    }
}

/**
 * @brief  Callback function prototype for mpc CDC-ACM events.
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
#ifndef CONFIG_FIDO_USB_HID
void cdc_acm_mpc_event_callback(void *handle, cdc_acm_event_e event,
                                void *context)
{
    if (event == CDC_ACM_EVENT_DATA_SENT)
    {

        tx_event_flags_set(&mpc_event_flags, USB_EVENT_MPC_SENT, TX_OR);

    }
    else if (event == CDC_ACM_EVENT_DATA_RECEIVED)
    {
        tx_event_flags_set(&mpc_event_flags, USB_EVENT_MPC_RECV, TX_OR);
     
    }
    else
    {
        LOGV("%s: event:%d, not process", __func__, event);
    }
}
#endif

/**
 * @brief  Set a callback function for handling incoming HID reports.
 * @param  callback Pointer to the callback function.
 * @retval int 0 if successful, negative value if an error occurs.
 *
 * This function sets a callback that will be invoked whenever a new HID report
 * is received from the host. The callback function should handle the processing
 * of the received data and respond appropriately.
 */
#ifdef CONFIG_FIDO_USB_HID
static void hid_report_recv_callback(uint8_t report_id, const void *buf,
                                     size_t length)
{
    LOGV("%s: report received!", __func__);

    tx_event_flags_set(&usb_event_flags, USB_EVENT_RECV_HID, TX_OR);
   
}

#endif

/**
 * @brief  init usb
 * @param  null
 * @retval void
 */
void init_usb(void)
{
    // usb core initialize
    usb_initialize();

    // initial usb_cdc_acm
    usb_cdc_acm_init();
#ifdef CONFIG_FIDO_USB_HID
    // initial usb hid
    usb_hid_init();
#endif
    // start usb enumeration
    usb_connect();
}

/**
 * @brief  uninit usb
 * @param  null
 * @retval void
 */
void uninit_usb(void)
{
    usb_disconnect();
#ifdef CONFIG_FIDO_USB_HID
    usb_hid_uninit();
#endif
    usb_cdc_acm_uninit();
    usb_uninitialize();
}


/**
 * @brief  usb service thread entry
 * @param  thread_input   thread input
 * @retval void
 */
void thread_usb_service_entry(ULONG thread_input)
{
    init_usb();
    if (BSP_USB_GetStatus() == BSP_USB_INSERT)
    {
        LOGV("USB cable insert.");
        BSP_PM_PWR_en(PM_PWR_ALDO3, true);
    }
    BSP_USB_Detect_Register(usb_detect_callback);

    LOGV("%s: waiting for usb ready", __func__);
    while (usb_status_get() != USB_STATE_CONFIGURED)
    {
        tx_thread_sleep(1000);
    }

    LOGV("____________USB_STATE_CONFIGURED___________________");
#ifdef CONFIG_FIDO_USB_HID
    hid_set_report_callback(hid_report_recv_callback);
#endif
    LOGV("FIDO USB init finished.");

    cdc_acm_handle = usb_cdc_acm_open(0);

    if (!cdc_acm_handle)
    {
        LOGV("%s: handle invalid!", __func__);
        
        return;
    }

    usb_cdc_acm_register_callback(cdc_acm_handle, cdc_acm_event_callback);
    tx_event_flags_create(&usb_event_flags, "usb_event");
    tx_event_flags_create(&mpc_event_flags, "usb_mpc_event");
#ifndef CONFIG_FIDO_USB_HID
    cdc_acm_mpc_handle = usb_cdc_acm_open(1);
    if (!cdc_acm_mpc_handle)
    {
        LOGV("%s: handle invalid!\n", __func__);
        
        return;
    }
    usb_cdc_acm_register_callback(cdc_acm_mpc_handle, cdc_acm_mpc_event_callback);
#endif
    tx_event_flags_create(&usb_plug_event_flags, "usb_plug_event");
    is_usb_inserted = true;
    is_usb_config = true;

#ifdef CONFIG_SUPPORT_FIDO
    set_device_type(DEVICE_TYPE_USB);
#endif
    
    uint32_t actual_usb_plug_flags = 0;
    
   
    while (1)
    {
        if (is_usb_inserted && usb_status_get() == USB_STATE_CONFIGURED && is_usb_config == false)
        {
            is_usb_config = true;

            LOGV("%s: waiting for usb ready", __func__);
            LOGV("USB_STATE_CONFIGURED");
#ifdef CONFIG_FIDO_USB_HID
#ifdef CONFIG_SUPPORT_FIDO
            set_device_type(DEVICE_TYPE_USB);
#endif
            hid_set_report_callback(hid_report_recv_callback);
#endif
            LOGV("FIDO USB init finished.");

            cdc_acm_handle = usb_cdc_acm_open(0);

            if (!cdc_acm_handle)
            {
                LOGV("%s: handle invalid!", __func__);
                return;
            }

            usb_cdc_acm_register_callback(cdc_acm_handle, cdc_acm_event_callback);

#ifndef CONFIG_FIDO_USB_HID
            cdc_acm_mpc_handle = usb_cdc_acm_open(1);
            if (!cdc_acm_mpc_handle)
            {
                LOGV("%s: handle invalid!\n", __func__);
                
                return;
            }
            usb_cdc_acm_register_callback(cdc_acm_mpc_handle, cdc_acm_mpc_event_callback);
#endif
        }
#if defined(CONFIG_FIDO_USB_HID) && defined(CONFIG_DARIC_GUIX) && defined(CONFIG_SUPPORT_FIDO)
        else if (is_usb_inserted && is_usb_config == false)
        {
            
            if (connection_type_config == FIDO_SERVICE_CONN_MSG_VALUE_AUTO)
            {
                set_device_type(DEVICE_TYPE_BLE);
            }
            else
            {
                set_device_type(DEVICE_TYPE_USB);
            }
        }
#endif
        tx_event_flags_get(&usb_plug_event_flags,
                           USB_EVENT_PLUG_IN | USB_EVENT_PLUG_OUT, TX_OR_CLEAR,
                           &actual_usb_plug_flags, TX_NO_WAIT);

        if (actual_usb_plug_flags & USB_EVENT_PLUG_IN)
        {
            if (!is_usb_inserted)
            {
                is_usb_inserted = true;

                LOGV("*************************************is_usb_inserted = true!");
                LOGV("***************************************BSP_USB_GetStatus = %d", BSP_USB_GetStatus());

                BSP_PM_PWR_en(CONFIG_USB_POWER_3V1, true);
                BSP_PM_PWR_en(CONFIG_USB_POWER_0V9, true);
                init_usb();
            }
        }

        if (actual_usb_plug_flags & USB_EVENT_PLUG_OUT) // usb removed
        {
            LOGV("***************************************is_usb_inserted = false;");
            LOGV("***************************************BSP_USB_GetStatus = %d", BSP_USB_GetStatus());
           
            if (is_usb_inserted)
            {
                uninit_usb();

                is_usb_inserted = false;
                is_usb_config = false;
                cdc_acm_handle = NULL;
            }
#if defined(CONFIG_FIDO_USB_HID) && defined(CONFIG_DARIC_GUIX) && defined(CONFIG_SUPPORT_FIDO)
            if (current_screen == (GX_WIDGET *)&fido_ui_confirm_window)
            {
                LOGV("**************** is_usb_inserted = false, current_screen is fido_ui_timeout_window");
                uv_auto_cancel_handle();
            }

            if (connection_type_config == FIDO_SERVICE_CONN_MSG_VALUE_AUTO)
            {
                set_device_type(DEVICE_TYPE_BLE);
            }
            else
            {
                set_device_type(DEVICE_TYPE_USB);
            }
#endif

            BSP_PM_PWR_en(CONFIG_USB_POWER_3V1, false);
            BSP_PM_PWR_en(CONFIG_USB_POWER_0V9, false);
        }

        tx_thread_sleep(5);
    }
    return;
}

/**
 * @brief  get usb connect state
 * @param  void
 * @retval ture:usb is connect false:usb is removed
 */
bool is_usb_connected()
{
    return is_usb_inserted&&is_usb_config&&(usb_status_get() == USB_STATE_CONFIGURED);
}
