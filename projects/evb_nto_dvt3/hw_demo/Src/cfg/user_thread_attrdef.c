/**
 ******************************************************************************
 * @file    user_thread_attrdef.c
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

/* Includes --------------------------------------------------------------------------*/
#include "user_threads_attrdef.h"


#define THREAD_ITEM(id, trdName, trdStkSize, trdPriority, trdPreemptThod, trdSlice) \
    static char gStack##id[trdStkSize]  __attribute__((aligned(4)));
#include "user_threads_table.h"
#undef THREAD_ITEM

#define THREAD_ITEM(id, trdName, trdStkSize, trdPriority, trdPreemptThod, trdSlice) \
    [id] = { \
        .stackSize      = trdStkSize, \
        .priority       = trdPriority, \
        .preemptThod    = trdPreemptThod, \
        .timeSlice      = trdSlice, \
        .pStack         = gStack##id, \
        .name           = trdName \
    },

T_ThreadAttrType gUsrThreads_cfg_table[USER_THREAD_MAXNUM] = {
    #include "user_threads_table.h"
};
#undef THREAD_ITEM

#if 0
T_ThreadAttrType gUsrThreads_cfg_table[USER_THREAD_MAXNUM] = {
    /* Genesis,Initial thread, the first thread started by the system
     * where the main function is executed.
     */
    [USER_THREAD_MAIN] = {
        .stackSize = 2048,
        .priority = 16,
        .preemptThod = 16,
        .timeSlice = 10,
        .pStack = NULL,
        .name = "main thread"
    },
    /* adaptation of Guix for the cst9220 touchpanel */
    [USER_THREAD_TOUCH] = {
        .stackSize = 2048,
        .priority = 16,
        .preemptThod = 16,
        .timeSlice = 10,
        .pStack = NULL,
        .name = "touchpanel thread"
    },    
};
#endif



/************************************* END OF FILE ************************************/
