/**
 ******************************************************************************
 * Copyright 2024-2026 CrossBar, Inc.
 * This file has been modified by CrossBar, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************
 */

/******************************************************************************
  *
  *  Copyright (c) 2023, CEC Huada Electronic Design Co.,Ltd.
  *
  *  Licensed under the Apache License, Version 2.0 (the "License");
  *  you may not use this file except in compliance with the License.
  *  You may obtain a copy of the License at:
  *
  *  http://www.apache.org/licenses/LICENSE-2.0
  *
  *  Unless required by applicable law or agreed to in writing, software
  *  distributed under the License is distributed on an "AS IS" BASIS,
  *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  *  See the License for the specific language governing permissions and
  *  limitations under the License.
  *
  ******************************************************************************/
  

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PORT_OS_H__
#define __PORT_OS_H__

#include <stdint.h>
#include "tx_api.h"


#define FREERTOS_TOTAL_HEAP_SIZE        (15*1024)
#define FREERTOS_MAX_PRIORITIES         (7)
#define FREERTOS_MINIMAL_STACK_SIZE     ((uint16_t)128)
#define FREERTOS_MAX_TASK_NAME_LEN      (10)
#define FREERTOS_TIMER_TASK_PRIORITY    (FREERTOS_MAX_PRIORITIES-1)//set to Highest Priority
#define FREERTOS_TIMER_QUEUE_LENGTH     (5)
#define FREERTOS_TIMER_TASK_STACK_DEPTH FREERTOS_MINIMAL_STACK_SIZE /* 1 seems sufficient so far */


#define HED_REAL_TIME_PRIORITY         6
#define HED_HIGH_PRIORITY            	 5
#define HED_ABOVE_NORMAL_PRIORITY      4
#define HED_NORMAL_PRIORITY            3
#define HED_BELOW_PRIORITY             2



typedef TX_THREAD PORT_ThrdHandle_t;
typedef TX_MUTEX portMutexId;
typedef TX_SEMAPHORE portSemaphoreId;
typedef TX_SEMAPHORE portSemaphoreDef_t;
#define portWaitForever   TX_WAIT_FOREVER
#define osWaitForever   TX_WAIT_FOREVER

typedef enum  {
  osPriorityIdle          = -3,          ///< priority: idle (lowest)
  osPriorityLow           = -2,          ///< priority: low
  osPriorityBelowNormal   = -1,          ///< priority: below normal
  osPriorityNormal        =  0,          ///< priority: normal (default)
  osPriorityAboveNormal   = +4,          ///< priority: above normal
  osPriorityHigh          = +3,          ///< priority: high
  osPriorityRealtime      = +4,          ///< priority: realtime (highest)
  osPriorityError         =  0x84        ///< system cannot determine priority or thread has illegal priority
} osPriority;

typedef enum
{
  PORT_STATUS_SUCCESS,          /*!< PORT Status SUCCESS    */
  PORT_STATUS_FAILED,           /*!< PORT Status FAILED     */
  PORT_STATUS_SUCCESS_EXT,      /*!< PORT Status used in fwdl: SUCCESS WITH NO MORE CMD*/
  PORT_STATUS_INSUFFICIENT_SPACE,
} PORT_Status_t;                /*!< PORT Status type       */

typedef void (*port_pthread) (ULONG argument);

typedef struct os_thread_def  {
  char                   *name;        ///< Thread name
  port_pthread           pthread;     
  osPriority             tpriority;    ///< initial thread priority
  uint32_t               instances;    ///< maximum number of instances of that thread function
  uint32_t               stacksize;    ///< stack size requirements in bytes; 0 is default stack size
  TX_THREAD              *threadptr;
} portThreadDef_t;

typedef struct os_semaphore_def  {
  uint32_t                   dummy;    ///< dummy value.
} osSemaphoreDef_t;


void *port_mem_getMem(const uint32_t size);
void port_mem_freeMem(void *pMem);
UINT port_thread_create(const portThreadDef_t *thread_def, void *argument);
void port_thread_delete(PORT_ThrdHandle_t *threadptr);
PORT_Status_t port_semaphore_create(portSemaphoreDef_t *semaphore_id, int32_t count, char *name);
PORT_Status_t port_semaphore_wait(portSemaphoreId *semaphore_id, uint32_t millisec);
PORT_Status_t port_semaphore_release(portSemaphoreId *semaphore_id);
PORT_Status_t port_semaphore_delete(portSemaphoreId *semaphore_id);
void port_timer_delay_ms(ULONG ms);

extern PORT_Status_t port_i2c_mutex_init(portMutexId *context, char *name);
extern PORT_Status_t port_i2c_mutex_wait(portMutexId *context);
extern PORT_Status_t port_i2c_mutex_release(portMutexId *context);




#endif /* __PORT_MEM_H__ */

