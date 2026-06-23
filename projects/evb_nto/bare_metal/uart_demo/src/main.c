/**
 *******************************************************************************
 * @file    main.c
 * @author  Daric Team
 * @brief   Source file for main.c module.
 *******************************************************************************
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
#include "daric_util.h"
#include "daric.h"
#include "daric_hal.h"
#include "system_daric.h"
#include "daric_hal_uart.h"
#include "daric_hal_pinmap.h"
#include <string.h>

#define SHELL_PROMPT "\r\ndaric> "

static UART_HandleTypeDef huart0;
static PINMAP_InitTypeDef pinMap[] = {
    { PORT_A, PIN_0, AF2_PA0_I2C1_SCL }, // Configure PA0
    { PORT_A, PIN_1, AF2_PA1_I2C1_SDA }, // Configure PA1
    { PORT_A, PIN_3, AF1_PA3_UART0_RX }, // Configure PA3
    { PORT_A, PIN_4, AF1_PA4_UART0_TX }, // Configure PA4
};

void shell_init(void)
{
    huart0.id              = UART0_ID;
    huart0.init.BaudRate   = 115200;
    huart0.init.WordLength = 3; // 8 bits
    huart0.init.StopBits   = 0; // 1 stop bit
    huart0.init.Parity     = 0; // No parity
    huart0.init.Tx_En      = 1;
    huart0.init.Rx_En      = 1;
    huart0.init.Poll_En    = 0; // Should not enable this bit when transfering data via uDMA

    if (HAL_UART_Init(&huart0) != HAL_OK)
    {
        printf("UART0 Init Failed!\n");
    }
    else
    {
        printf("UART0 Init Success for Shell.\n");
    }
}

void shell_loop(void)
{
    uint8_t  ch;
    char     buffer[10] = { 0 };
    uint32_t index      = 0;

    HAL_UART_Transmit(&huart0, (uint8_t *)SHELL_PROMPT, strlen(SHELL_PROMPT), 100);

    while (1)
    {
        if (HAL_UART_Receive(&huart0, &ch, 1, 0xFFFFFFFF) == HAL_OK)
        {
            // Echo back
            if (ch == '\r' || ch == '\n')
            {
                HAL_UART_Transmit(&huart0, (uint8_t *)"\r\n", 2, 100000);
                if (index > 0)
                {
                    buffer[index] = '\0';
                    // Process command here if needed
                    printf("Received command: %s\n", buffer);

                    if (strcmp(buffer, "help") == 0)
                    {
                        const char *msg = "Available commands: help, hello, version\r\n";
                        HAL_UART_Transmit(&huart0, (uint8_t *)msg, strlen(msg), 100);
                    }
                    else if (strcmp(buffer, "hello") == 0)
                    {
                        const char *msg = "Hello from Daric UART0 Shell!\r\n";
                        HAL_UART_Transmit(&huart0, (uint8_t *)msg, strlen(msg), 100);
                    }
                    else if (strcmp(buffer, "version") == 0)
                    {
                        const char *msg = "Daric Baremetal UART Demo v1.0\r\n";
                        HAL_UART_Transmit(&huart0, (uint8_t *)msg, strlen(msg), 100);
                    }
                    else
                    {
                        const char *msg = "Unknown command. Type 'help' for info.\r\n";
                        HAL_UART_Transmit(&huart0, (uint8_t *)msg, strlen(msg), 100);
                    }
                    index = 0;
                    memset(buffer, 0, sizeof(buffer));
                }
                HAL_UART_Transmit(&huart0, (uint8_t *)SHELL_PROMPT, strlen(SHELL_PROMPT), 100);
            }
            else if (ch == 127 || ch == 8) // Backspace
            {
                if (index > 0)
                {
                    index--;
                    HAL_UART_Transmit(&huart0, (uint8_t *)"\b \b", 3, 100);
                }
            }
            else
            {
                if (index < (sizeof(buffer) - 1))
                {
                    buffer[index++] = ch;
                    HAL_UART_Transmit(&huart0, &ch, 1, 100);
                }
            }
        }
    }
}

int main(void)
{
    printf("Hello baremetal uart demo. Build @ %s %s.\n", __DATE__, __TIME__);

    HAL_Init();
    HAL_PINMAP_init(pinMap, sizeof(pinMap) / sizeof(pinMap[0]));
    HAL_TickInit();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    printf("\nDelay 2 seconds.\n");
    HAL_Delay(2000);
    printf("Starting Baremetal UART Demo...\n");
    printf("Debug trace is on DUART, Shell is on UART0.\n");
    shell_init();
    shell_loop();

    return 0;
}
