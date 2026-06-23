/**
 ******************************************************************************
 * @file    daric_vibrator_linear.c
 * @author  PERIPHERIAL BSP Team
 * @brief   file of vibrator logic layer.
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

#include "daric_activecard_nto_vibrator.h"
#include "../Components/AWHapticNv/haptic_nv.h"
#include "daric_errno.h"
#include "daric_hal.h"
#include "daric_hal_def.h"
#include "daric_hal_gpio.h"
#include "daric_hal_i2c.h"
#include "daric_log.h"
#include "system_daric.h"
#include "tx_api.h"
#include "tx_work_item_queue.h"

I2C_HandleTypeDef vibrator_i2c_handle;
TX_TIMER vibrator_timer_handle;
extern void haptic_nv_tim_periodelapsedcallback(ULONG timer_id);
static int isInitDone = 0;

static void Vibrator_I2C_init(void) {
    memset(&vibrator_i2c_handle, 0, sizeof(I2C_HandleTypeDef));
    vibrator_i2c_handle.instance_id = CONFIG_VIBRATOR_I2C_ID;
    vibrator_i2c_handle.init.wait = 1;
    vibrator_i2c_handle.init.repeat = 1;
    vibrator_i2c_handle.init.baudrate = CONFIG_VIBRATOR_I2C_SPEED;
    vibrator_i2c_handle.init.rx_buf = 0;
    vibrator_i2c_handle.init.rx_size = 0;
    vibrator_i2c_handle.init.tx_buf = 0;
    vibrator_i2c_handle.init.tx_size = 0;
    vibrator_i2c_handle.init.cmd_buf = 0;
    vibrator_i2c_handle.init.cmd_size = 0;

    HAL_StatusTypeDef result = HAL_I2C_Init(&vibrator_i2c_handle);
    if (result != HAL_OK) {
        LOGE("FAILED!\n");
        return;
    }
    LOGI("SUCCEED!\n");
}

static void Vibrator_Callback_wrap(void *obj) { g_func_haptic_nv->irq_handle(); }

static void Vibrator_Callback(void *UserData) { submitWorkItem(Vibrator_Callback_wrap, NULL, DEV_ID_OTHER); }

static void Vibrator_GPIO_init1(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = CONFIG_AW_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(CONFIG_AW_RST_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(CONFIG_AW_RST_GPIO_PORT, CONFIG_AW_RST_Pin, GPIO_PIN_SET);
}

static void Vibrator_GPIO_init2(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CONFIG_AW_IRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // extern pull-up resistor
    GPIO_InitStruct.UserData = NULL;
    GPIO_InitStruct.IsrHandler = Vibrator_Callback;
    HAL_GPIO_Init(CONFIG_AW_IRQ_GPIO_PORT, &GPIO_InitStruct);
}

static void Vibrator_Timer_init(void) {
    tx_timer_create(&vibrator_timer_handle, "Vibrator Timer", haptic_nv_tim_periodelapsedcallback, (ULONG)0,
                    CONFIG_SYS_CLOCK_TICKS_PER_SEC / 1000, CONFIG_SYS_CLOCK_TICKS_PER_SEC / 1000, TX_NO_ACTIVATE);
}

#define CHECK_INIT_DONE()                                                                                              \
    if (isInitDone == 0) {                                                                                             \
        LOGE("Vibrator not initialized!\n");                                                                           \
        return BSP_ERROR_NO_INIT;                                                                                      \
    }

int BSP_Vibrator_Short_linear(void) {
    CHECK_INIT_DONE();
    g_func_haptic_nv->short_vib_work(1, 0x80, 1);
    return BSP_ERROR_NONE;
}

int BSP_Vibrator_Long_linear(uint32_t ms) {
    CHECK_INIT_DONE();
    g_func_haptic_nv->long_vib_work(4, 0x80, ms);
    return BSP_ERROR_NONE;
}

int BSP_Vibrator_Rtp_Play_linear(void) {
    CHECK_INIT_DONE();
    g_func_haptic_nv->rtp_vib_work(0x80);
    return BSP_ERROR_NONE;
}

int BSP_Vibrator_Init_linear(void) {
    int ret = AW_SUCCESS;
    LOGI("Initializing Vibrator...\n");

    Vibrator_I2C_init();
    Vibrator_GPIO_init1();
    Vibrator_Timer_init();

    // init haptic ic
    ret = haptic_nv_boot_init();
    if (ret != AW_SUCCESS) {
        LOGE("Vibrator initialization failed with error code: %d\n", ret);
        return BSP_ERROR_PERIPH_FAILURE;
    }
    // init irq gpio after haptic_nv_boot_init
    Vibrator_GPIO_init2();

    LOGI("Vibrator initialization complete.\n");
    isInitDone = 1;
    return BSP_ERROR_NONE;
}
