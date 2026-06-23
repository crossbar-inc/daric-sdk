/**
 ******************************************************************************
 * @file    threads_dependency.h
 * @author  OS Team
 * @brief   The Thread Execution Management Framework provides a robust solution 
 *          for managing thread creation and initialization dependencies 
 *          in ThreadX RTOS-based systems. 
 *          It ensures that threads are created and initialized in the correct order, 
 *          with proper handling of inter-thread dependencies.
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
/* START_THREAD_ITEM(threadID,       entry function,     param ,
 *                   DEPENDENCY_ITEMS(dependency threads))     
 */

 START_THREAD_ITEM(USER_THREAD_USBSERVICE,   thread_usb_service_entry,   0,
                   DEPENDENCY_ITEMS())

START_THREAD_ITEM(USER_THREAD_GUI,          guiThreadEntry,             0,
                  DEPENDENCY_ITEMS())

START_THREAD_ITEM(USER_THREAD_BLUETOOTH,    bluetooth_entry,            0,
                  DEPENDENCY_ITEMS(USER_THREAD_GUI))
