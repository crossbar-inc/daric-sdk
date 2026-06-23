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
#include <stdint.h>
#include <stdio.h>
#include "daric_hal.h"
#include "daric_pm.h"
#include "pinmap_config.h"
#include "tx_api.h"
#include "tg28.h"
#include "daric_pmu.h"
#include "daric_util.h"

/* Demo definitions */
#define DEMO_STACK_SIZE 1024
#define DEMO_QUEUE_SIZE (16 * sizeof(uint32_t))

static TX_THREAD thread_0;
static TX_THREAD thread_1;
static TX_QUEUE  queue_0_to_1;
static TX_QUEUE  queue_1_to_0;

static uint8_t  stack_0[DEMO_STACK_SIZE];
static uint8_t  stack_1[DEMO_STACK_SIZE];
static uint32_t queue_mem_0_to_1[16];
static uint32_t queue_mem_1_to_0[16];

void thread_0_entry(ULONG thread_input)
{
    uint32_t msg_send = 0x11112222;
    uint32_t msg_recv;
    UINT     status;

    printf("Thread 0 started.\n");

    while (1)
    {
        /* Send a message to thread 1 */
        status = tx_queue_send(&queue_0_to_1, &msg_send, TX_WAIT_FOREVER);
        if (status == TX_SUCCESS)
        {
            printf("Thread 0: Sent 0x%08x to Thread 1\n", (unsigned int)msg_send);
        }

        /* Wait for a message from thread 1 */
        status = tx_queue_receive(&queue_1_to_0, &msg_recv, TX_WAIT_FOREVER);
        if (status == TX_SUCCESS)
        {
            printf("Thread 0: Received 0x%08x from Thread 1\n", (unsigned int)msg_recv);
        }

        msg_send++;
        tx_thread_sleep((CONFIG_SYS_CLOCK_TICKS_PER_SEC) * 1);
    }
}

void thread_1_entry(ULONG thread_input)
{
    uint32_t msg_send = 0xAAAA5555;
    uint32_t msg_recv;
    UINT     status;

    printf("Thread 1 started.\n");

    while (1)
    {
        /* Wait for a message from thread 0 */
        status = tx_queue_receive(&queue_0_to_1, &msg_recv, TX_WAIT_FOREVER);
        if (status == TX_SUCCESS)
        {
            printf("Thread 1: Received 0x%08x from Thread 0\n", (unsigned int)msg_recv);
        }

        /* Send a response message to thread 0 */
        status = tx_queue_send(&queue_1_to_0, &msg_send, TX_WAIT_FOREVER);
        if (status == TX_SUCCESS)
        {
            printf("Thread 1: Sent 0x%08x to Thread 0\n", (unsigned int)msg_send);
        }

        msg_send--;
    }
}

int HAL_ClockPreProcess(HAL_CPU_FreqVolMap_TypeDef cpu_fv_map)
{
    //   AXP223_LDO_DCDC_Set_Voltage(AXP223_DCDC2, cpu_fv_map.Voltage);
    BSP_PM_PWR_set(CONFIG_CLOCK_POWER, cpu_fv_map.Voltage);
    return 0;
}

int main(void)
{
    int cnt = 0;
#ifdef HAL_PINMAP_MODULE_ENABLED
    HAL_PINMAP_init(g_pinMap, sizeof(g_pinMap) / sizeof(g_pinMap[0]));
#endif /* HAL_PINMAP_MODULE_ENABLED */

    /* CPU boost */
    /*
     * The configuration of the system frequency requires TG28,
     * so the I2C must be initialized first.
     */
    HAL_Init();
    HAL_TickInit();
    TG28_I2C_init();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    printf("cgufscr = %08lx\n", DARIC_CGU->cgufscr);
    printf("cgufsvld = %08lx\n", DARIC_CGU->cgufsvld);

    printf("Hello threadx demo. Build @ %s %s.\n", __DATE__, __TIME__);

    /* Create ThreadX resources */
    tx_queue_create(&queue_0_to_1, "Queue 0->1", TX_1_ULONG, queue_mem_0_to_1, DEMO_QUEUE_SIZE);
    tx_queue_create(&queue_1_to_0, "Queue 1->0", TX_1_ULONG, queue_mem_1_to_0, DEMO_QUEUE_SIZE);

    tx_thread_create(&thread_0, "Thread 0", thread_0_entry, 0, stack_0, DEMO_STACK_SIZE, 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_1, "Thread 1", thread_1_entry, 0, stack_1, DEMO_STACK_SIZE, 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);

    while (1)
    {
        printf("[%d] Main thread sleep 10s ...\n", cnt);

        cnt++;

        tx_thread_sleep((CONFIG_SYS_CLOCK_TICKS_PER_SEC) * 10);
    }

    return 0;
}
