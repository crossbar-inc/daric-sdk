/**
 *****************************************************************************
 * @file    sys_config.h
 * @author  AP Team
 * @brief   Define common configuration of device.
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
#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_
#include "daric_hal_wdg.h"

#ifdef CONFIG_DUMP_HEAPINFO
    /**
     * @brief Get the amount of free memory in the default heap pool.
     * @return uint32_t The number of free bytes currently available in the default heap.
     * 
     * @note The function only retrieves the free size information.
     *       other pool attributes are ignored by passing NULL pointers for those parameters.
     *       Total free memory may be fragmented.
     *
     * This function queries the memory pool information to retrieve the current
     * amount of free memory available for allocation in the default heap pool.
     */
    extern uint32_t getHeapFreeSize();
    /**
     * @brief Traverse and dump detailed information about allocated blocks in the default heap.
     * @return none
     *
     * @note This function disables interrupts during traversal to ensure heap consistency
     *       while inspecting internal structures. Interrupts are restored before returning.
     *
     * This function performs a low-level traversal of the internal block structure of the
     * default heap memory pool. It walks through the heap's block list, 
     * identifies allocated blocks, and prints detailed information for each allocated block.
     * The function also calculates and prints summary statistics 
     * including total allocation count, 
     * total allocated bytes, and remaining free heap space.
     */
    extern void sysDumpHeapInfo();
#endif

#ifdef CONFIG_SOC_DARIC_NTO_A
#define ITCM_PUT_BUFF_SECTION __attribute__((section("itcmdata"), aligned(4)))
#define DTCM_PUT_BUFF_SECTION __attribute__((section("dtcmdata"), aligned(4)))
#else
#define ITCM_PUT_BUFF_SECTION 
#define DTCM_PUT_BUFF_SECTION 
#endif

#define WATCHDOG_TIMEOUT_MS (30000 * WDG_TICKS_PER_MS)
#endif // _SYS_CONFIG_H_
