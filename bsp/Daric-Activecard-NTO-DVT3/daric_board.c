/**
******************************************************************************
* @file    daric_board.c
* @author  PERIPHERIAL BSP Team
* @brief   This file includes the driver for board module.
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

#include "daric_hal_gpio.h"
#include <stdint.h>
#include <string.h>

#include "daric_log.h"

#define ID_1_PORT GPIOC
#define ID_2_PORT GPIOC
#define ID_3_PORT GPIOE
#define ID_1_PIN  GPIO_PIN_9
#define ID_2_PIN  GPIO_PIN_11
#define ID_3_PIN  GPIO_PIN_3

static bool    inited = false;
static uint8_t s_id   = 0;

static void init_gpio() {
    if (inited) {
        return;
    }
    GPIO_InitTypeDef init1 = {0};
    init1.Pin              = ID_1_PIN;
    init1.Mode             = GPIO_MODE_INPUT;
    init1.Pull             = GPIO_NOPULL;
    init1.UserData         = NULL;
    HAL_GPIO_Init(ID_1_PORT, &init1);

    GPIO_InitTypeDef init2 = {0};
    init2.Pin              = ID_2_PIN;
    init2.Mode             = GPIO_MODE_INPUT;
    init2.Pull             = GPIO_NOPULL;
    init2.UserData         = NULL;
    HAL_GPIO_Init(ID_2_PORT, &init2);

    GPIO_InitTypeDef init3 = {0};
    init3.Pin              = ID_3_PIN;
    init3.Mode             = GPIO_MODE_INPUT;
    init3.Pull             = GPIO_NOPULL;
    init3.UserData         = NULL;
    HAL_GPIO_Init(ID_3_PORT, &init3);

    GPIO_PinState id1 = HAL_GPIO_ReadPin(ID_1_PORT, ID_1_PIN);
    GPIO_PinState id2 = HAL_GPIO_ReadPin(ID_2_PORT, ID_2_PIN);
    GPIO_PinState id3 = HAL_GPIO_ReadPin(ID_3_PORT, ID_3_PIN);

    s_id              = id3 << 2 | id2 << 1 | id1;
    LOGI("Hardware id = %d", s_id);

    init1.Mode = GPIO_MODE_OUTPUT;
    init2.Mode = GPIO_MODE_OUTPUT;
    init3.Mode = GPIO_MODE_OUTPUT;

    HAL_GPIO_WritePin(ID_1_PORT, ID_1_PIN, id1);
    HAL_GPIO_WritePin(ID_2_PORT, ID_2_PIN, id2);
    HAL_GPIO_WritePin(ID_3_PORT, ID_3_PIN, id3);

    inited = true;
}

uint8_t BSP_BOARD_getHwId() {
    if (!inited) {
        init_gpio();
    }
    LOGI("Hardware id = %d", s_id);

    return s_id;
}