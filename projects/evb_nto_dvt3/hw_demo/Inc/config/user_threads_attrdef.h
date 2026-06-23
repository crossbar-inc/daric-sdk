/**
 ******************************************************************************
 * @file    user_threads_attrdef.h
 * @author  OS Team
 * @brief   This file defines the basic structure needed to create a thread. 
 *          Threads in the project will use a unified structure for management, 
 *          The purpose is to facilitate adjust various thread attributes 
 *          and monitor thread stack usage.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __THREADS_ATTRDEF_H__
#define __THREADS_ATTRDEF_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "stdint.h"
#include "tx_api.h"
#include "tx_initialize.h"
#include "tx_thread_attrdef.h"
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  The thread enum definition
 */

/**
 * @enum ThreadStatus
 * @brief Thread lifecycle status enumeration
 * 
 * Tracks the current state of each thread in the system.
 */
typedef enum {
    THREAD_STATUS_NOT_CREATED = 0,  /* thread not yet created */
    THREAD_STATUS_CREATED,          /* thread created but not initialized */
    THREAD_STATUS_INITIALIZED,      /* thread fully initialized and running */
} T_ThreadStatus;

/* It must start from 0; this is an index of an array */
#define THREAD_ITEM(id, trdName, trdStkSize, trdPriority, trdPreemptThod, trdSlice) id,
typedef enum {
    #include "user_threads_table.h"
  USER_THREAD_MAXNUM,
} T_UserThreadEnum;
#undef THREAD_ITEM

extern T_ThreadAttrType gUsrThreads_cfg_table[USER_THREAD_MAXNUM];

typedef VOID (*T_EntryFunc)(ULONG id);

typedef struct threads_dependency
{
    T_UserThreadEnum    threadId;       /* thread id */
    T_UserThreadEnum    *dependencies;  /* dependency threads */
    UINT                depCount;       /* dependency count */
    T_EntryFunc         entryFunc;      /* thread entry point */
    ULONG               param;          /* entry point param */
    T_ThreadStatus      status;         /* thread lifecycle status */
    TX_THREAD           thread;         /* thread control block */
} T_ThreadsDepType;

extern T_ThreadsDepType gStartThreadList[];
extern TX_EVENT_FLAGS_GROUP threadInitEvent;
/* Exported functions --------------------------------------------------------*/
/**
 * @brief Starts all configured threads in the correct order
 * @param None
 * @return uint32_t TX_SUCCESS if successful, error code otherwise
 * 
 * Iteratively creates threads while respecting their dependencies.
 * Continues until all threads are created and started.
 */
extern uint32_t startAllThreads(void);

#ifdef __cplusplus
}
#endif

#endif