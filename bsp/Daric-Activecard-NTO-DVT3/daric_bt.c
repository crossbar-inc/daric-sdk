/**
******************************************************************************
* @file    daric_bt.c
* @author  PERIPHERIAL BSP Team
* @brief   This file includes the driver for bluetooth module.
******************************************************************************
* @attention
*
* © Copyright CrossBar, Inc. 2026.
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
#include "daric_bt.h"
#include "daric_hal_uart.h"
#include "daric_log.h"
#include "stdbool.h"
#include "stdint.h"
#include "tx_uart_continuous.h"

#undef LOG_TAG
#define LOG_TAG "BSP_BT_TAG"

#define RX_BUFF_LEN 32

static UART_HandleTypeDef s_bt_uart = {0};
static uint8_t s_rx_buffer[RX_BUFF_LEN] = {0};
static bt_uart_rx_cb_t s_rx_callback = NULL;

static bool s_continuous_mode = false;

static void bt_UART_Txfinish_cb(UART_HandleTypeDef *huart)
{
    LOGV("bt_UART_Txfinish_cb");
}

static void bt_UART_Rxfull_cb(UART_HandleTypeDef *huart)
{
    if (huart->id == CONFIG_BLUETOOTH_HAL_UART_ID) {
        if (s_rx_callback) {
            s_rx_callback(s_rx_buffer, RX_BUFF_LEN);
        }
		LOGV("shoubing bt_UART_Rxfull_cb");
        HAL_UART_Receive_IT(&s_bt_uart, s_rx_buffer, RX_BUFF_LEN);
    }
}

HAL_StatusTypeDef bt_uart_init(void)
{
    memset(&s_bt_uart, 0, sizeof(UART_HandleTypeDef));

    s_bt_uart.id = CONFIG_BLUETOOTH_HAL_UART_ID;
    s_bt_uart.init.BaudRate = CONFIG_BLUETOOTH_HAL_UART_BAUDRATE;
    s_bt_uart.init.Rx_En = 1;
    s_bt_uart.init.Tx_En = 1;
    s_bt_uart.init.Clean_Rx_Fifo = 0;
    s_bt_uart.init.Poll_En = 0;
    s_bt_uart.init.StopBits = 0;   // 1BIT
    s_bt_uart.init.WordLength = 3; // 8BIT
    s_bt_uart.init.Parity = 0;

    HAL_StatusTypeDef result = HAL_UART_Init(&s_bt_uart);
    if (result != HAL_OK)
    {
		LOGE("HAL_UART_Init fail reason = %d", result);
        goto exit;
    }

    result = HAL_UART_RegisterCallback(&s_bt_uart, HAL_UART_RX_FULL_CB_ID,
                                       bt_UART_Rxfull_cb);
    if (result != HAL_OK)
    {
		LOGE("HAL_UART_RX_FULL_CB_ID fail reason = %d", result);
        goto exit;
    }

    result = HAL_UART_RegisterCallback(&s_bt_uart, HAL_UART_TX_FINISH_CB_ID,
                                       bt_UART_Txfinish_cb);
    if (result != HAL_OK)
    {
		LOGE("HAL_UART_TX_FINISH_CB_ID fail reason = %d", result);
        goto exit;
    }

exit:
    LOGV("bluetooth_init: %s", result == HAL_OK ? "PASSED" : "FAILED");
    return result;
}

void bt_uart_register_rx_callback(bt_uart_rx_cb_t cb)
{
	LOGV("shoubing bt_uart_register_rx_callback");
    s_rx_callback = cb;
}

bool bt_power_on()
{
    GPIO_TypeDef *s_OutputPx = CONFIG_BT_POWER_PORT;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CONFIG_BT_POWER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(s_OutputPx, &GPIO_InitStruct);
    GPIO_PinState state = HAL_GPIO_ReadPin(s_OutputPx, CONFIG_BT_POWER_PIN);
    if (state == GPIO_PIN_RESET)
    {
        LOGV("enable_bluetooth state is GPIO_PIN_RESET!");
        HAL_GPIO_WritePin(s_OutputPx, CONFIG_BT_POWER_PIN, GPIO_PIN_SET);
    }

    HAL_GPIO_WritePin(s_OutputPx, CONFIG_BT_POWER_PIN, GPIO_PIN_RESET);
    state = HAL_GPIO_ReadPin(s_OutputPx, CONFIG_BT_POWER_PIN);
    if (state == GPIO_PIN_RESET)
    {
        LOGV("enable_bluetooth write GPIO_PIN_SET success!");
    }
    tx_thread_sleep((CONFIG_SYS_CLOCK_TICKS_PER_SEC)*3);

    HAL_GPIO_WritePin(s_OutputPx, CONFIG_BT_POWER_PIN, GPIO_PIN_SET);
	return true;
	
}

bool bt_power_off()
{
    GPIO_TypeDef *s_OutputPx = CONFIG_BT_POWER_PORT;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CONFIG_BT_POWER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(s_OutputPx, &GPIO_InitStruct);
    GPIO_PinState state = HAL_GPIO_ReadPin(s_OutputPx, CONFIG_BT_POWER_PIN);
    if (state == GPIO_PIN_RESET)
    {
        LOGV("disable_bluetooth state is GPIO_PIN_RESET!");
        HAL_GPIO_WritePin(s_OutputPx, CONFIG_BT_POWER_PIN, GPIO_PIN_SET);
    }

    HAL_GPIO_WritePin(s_OutputPx, CONFIG_BT_POWER_PIN, GPIO_PIN_RESET);
    state = HAL_GPIO_ReadPin(s_OutputPx, CONFIG_BT_POWER_PIN);
    if (state == GPIO_PIN_RESET)
    {
        LOGV("disable_bluetooth write GPIO_PIN_SET success!");
    }
    tx_thread_sleep((CONFIG_SYS_CLOCK_TICKS_PER_SEC)*3.5);
    HAL_GPIO_WritePin(s_OutputPx, CONFIG_BT_POWER_PIN, GPIO_PIN_SET);
	return true;
}

HAL_StatusTypeDef bt_uart_transmit(const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    return HAL_UART_Transmit(&s_bt_uart, (uint8_t *)data, len, timeout_ms);
}

HAL_StatusTypeDef bt_uart_transmit_stop()
{
	return HAL_UART_TransmitStop(&s_bt_uart);
}

HAL_StatusTypeDef bt_uart_receive_it(void)
{
    if (s_continuous_mode)
	{
        bt_uart_stop_continuous_poll();
    }
    return HAL_UART_Receive_IT(&s_bt_uart, s_rx_buffer, RX_BUFF_LEN);
}

HAL_StatusTypeDef bt_uart_start_continuous_poll(void)
{
	return HAL_UART_ReceiveContinousForPoll(&s_bt_uart);
}

HAL_StatusTypeDef bt_uart_stop_continuous_poll(void)
{
	return HAL_UART_DeactivateContinousForPoll(&s_bt_uart);
}

int bt_uart_continuous_read(uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
    return daricUartContinuReadSync(&s_bt_uart, buf, len, timeout_ms);
}

HAL_StatusTypeDef bt_uart_reset(void)
{
    HAL_StatusTypeDef result = HAL_OK;
    result = HAL_UART_DeactivateContinousForPoll(&s_bt_uart);
    result = HAL_UART_ReceiveStop(&s_bt_uart);
    result = HAL_UART_TransmitStop(&s_bt_uart);

    result = HAL_UART_DeInit(&s_bt_uart);
    if (result != HAL_OK) {
        return result;
    }

    result = bt_uart_init();
    if (result != HAL_OK) {
        return result;
    }

    if (s_rx_callback) {
        result = bt_uart_receive_it();
    }

    return result;
}