/**
 ******************************************************************************
 * @file    user_threads_table.h
 * @author  OS Team
 * @brief   All properties of the usr threads are defined here, 
 *          and the application uses the corresponding ID to access 
 *          the relevant properties of the threads.
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
/* Genesis,Initial thread, the first thread started by the system
 * where the main function is executed.
 */
// THREAD_ITEM(USER_THREAD_MAIN,          //id
//             "Main thread",              //name
//             1024,                       //stackSize
//             16,                         //Priority
//             16,                         //PreemptThod
//             10)                         //Slice

/* This thread is the adaptation of Guix for the cst9220 touchpanel.
 * interacts with the touch panel and then sends converted touch messages to guix
 */

/* Gui thread, Here is where we initialize the LCD driver and guix
* This is not Guix Thread. Guix has its own independent thread.
*/
THREAD_ITEM(USER_THREAD_GUI,           
    "Gui thread", 
    STACK_SIZE_6KB,
    TX_PRIORITY_UI,         
    TX_PRIORITY_UI,         
    TX_TIME_SLICE_10)

THREAD_ITEM(USER_THREAD_BLUETOOTH,     
            "bluetooth_thread", 
            STACK_SIZE_15KB,               
            TX_PRIORITY_SYS_NORMAL,         
            TX_PRIORITY_SYS_NORMAL,         
            TX_TIME_SLICE_10)

 THREAD_ITEM(USER_THREAD_USBSERVICE,
             "USB service thread",
             STACK_SIZE_4KB,
             TX_PRIORITY_SYS_NORMAL,
             TX_PRIORITY_SYS_NORMAL,
             TX_TIME_SLICE_10)
