/**
******************************************************************************
* @file    daric_activecard_nto_usb.c
* @author  PERIPHERIAL BSP Team
* @brief   This file contains the common defines and functions prototypes for
*          the daric_activecard_nto_usb.c driver.
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
#include <stdbool.h>
#include <stdint.h>

#include "daric_errno.h"
#include "daric_activecard_nto_usb.h"
#include "daric_hal_gpio.h"
#include "daric_hal_pinmap.h"
#include "daric_log.h"
#include "tx_work_item_queue.h"

#define delay(ms) tx_thread_sleep((ms)*CONFIG_SYS_CLOCK_TICKS_PER_SEC / 1000)

static void USB_DETECT_IRQ_HANDLE(void *data);

static GPIO_TypeDef    *PORT = GPIOC;
static GPIO_InitTypeDef PIN  = {.Pin        = GPIO_PIN_4,
                                .Mode       = GPIO_MODE_INPUT,
                                .Pull       = GPIO_NOPULL,
                                .IsrHandler = USB_DETECT_IRQ_HANDLE};

static USB_DETECT_CB call_back  = NULL;
static bool          pin_inited = false;

static GPIO_PinState read_pin() {
    if (!pin_inited) {
        HAL_GPIO_Init(PORT, &PIN);
        pin_inited = true;
    }
    return HAL_GPIO_ReadPin(PORT, PIN.Pin);
}

static void set_pin_mode(uint32_t mode) {
    PIN.Mode = mode;
    HAL_GPIO_Init(PORT, &PIN);
}

static GPIO_PinState pre_status = GPIO_PIN_RESET;
// static int16_t irq_cnt = 0;

static void IRQ_WORK(void *obj) {
    USB_DETECT_CB cb     = (USB_DETECT_CB)obj;
    GPIO_PinState status = GPIO_PIN_RESET;
    delay(50);
    if (cb != NULL) {
        status = read_pin();
        // LOGD("irq_cont=%d, pre_status=%d status=%d", irq_cnt, pre_status, status);
        if (status != pre_status) {
            if (status == GPIO_PIN_RESET) {
                set_pin_mode(GPIO_MODE_IT_RISING);
                cb(BSP_USB_INSERT);
            } else {
                set_pin_mode(GPIO_MODE_IT_FALLING);
                cb(BSP_USB_REMOVE);
            }
            pre_status = status;
        }
    } else {
        set_pin_mode(GPIO_MODE_INPUT);
    }
    return;
}

static void USB_DETECT_IRQ_HANDLE(void *data) {
    // irq_cnt++;
    submitWorkItem(IRQ_WORK, call_back, DEV_ID_OTHER);
}

/* Exported functions --------------------------------------------------------*/
/**
 * @brief  The application layer could call this function to register the
 *         notification of USB status changed.
 *
 * @param  cb The callback to norify the USB status
 *
 * @retval BSP result. Return BSP_ERROR_NONE if success others if failed.
 */
int16_t BSP_USB_Detect_Register(USB_DETECT_CB cb) {
    if (cb == NULL) {
        return BSP_ERROR_WRONG_PARAM;
    }
    call_back            = cb;
    GPIO_PinState status = read_pin();
    pre_status           = status;
    if (status == GPIO_PIN_RESET && PIN.Mode != GPIO_MODE_IT_RISING) {
        set_pin_mode(GPIO_MODE_IT_RISING);
    } else if (status == GPIO_PIN_SET && PIN.Mode != GPIO_MODE_IT_FALLING) {
        set_pin_mode(GPIO_MODE_IT_FALLING);
    }
    return BSP_ERROR_NONE;
}

/**
 * @brief  The application layer could call this function to un-register the
 *         notification of USB status changed.
 *
 * @retval BSP result. Return BSP_ERROR_NONE if success others if failed.
 */
int16_t BSP_USB_Detect_UnRegister() {
    call_back = NULL;
    set_pin_mode(GPIO_MODE_INPUT);
    return BSP_ERROR_NONE;
}

/**
 * @brief  The application layer could call this function to the USB status.
 *
 * @retval Return BSP_USB_STATUS
 */
int16_t BSP_USB_GetStatus() {
    if (GPIO_PIN_RESET == read_pin()) {
        return BSP_USB_INSERT;
    } else {
        return BSP_USB_REMOVE;
    }
}
