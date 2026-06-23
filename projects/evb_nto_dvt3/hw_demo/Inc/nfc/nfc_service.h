/**
 ******************************************************************************
 * @file    nfc_service.h
 * @author  AP Team
 * @brief   This file provides an external interface for NFC modules
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
#ifndef __NFC_SERVICE_H
#define __NFC_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "tx_api.h"

#define NFC_LOCAL_M1_FEATURE            0
#define NFC_COPY_M1_TO_LOCAL_M1_FEATURE 0
#define NFC_RW_FEATURE                  0
#define NFC_HCE_FEATURE                 1

/* Common Error codes */
#define BSP_ERROR_NONE                  0
#define BSP_ERROR_NO_INIT               -1
#define BSP_ERROR_WRONG_PARAM           -2
#define BSP_ERROR_BUSY                  -3
#define BSP_ERROR_PERIPH_FAILURE        -4
#define BSP_ERROR_COMPONENT_FAILURE     -5
#define BSP_ERROR_UNKNOWN_FAILURE       -6
#define BSP_ERROR_UNKNOWN_COMPONENT     -7
#define BSP_ERROR_BUS_FAILURE           -8
#define BSP_ERROR_CLOCK_FAILURE         -9
#define BSP_ERROR_MSP_FAILURE           -10
#define BSP_ERROR_NOT_SUPPORTED_FEATURE -11

#define NFC_SERVICE_MSG_TYPE_ENABLE_NFC 0
#define NFC_SERVICE_MSG_TYPE_DISABLE_NFC 1
#define NFC_SERVICE_MSG_TYPE_GET_VERSION 2
#define NFC_SERVICE_MSG_TYPE_ENABLE_RW 3
#define NFC_SERVICE_MSG_TYPE_DISABLE_RW 4
#define NFC_SERVICE_MSG_TYPE_SELECT 5

typedef enum {
    NFC_MODE_NONE = 0,      // NONE
    NFC_MODE_NFC,           // sta
    NFC_MODE_NFC_RW         // RW
} nfc_mode_t;

typedef struct {
    uint32_t msg_type;
    uint8_t *data;
} NFC_SERVICE_MESSAGE;

typedef struct 
{
    /* 0:usb state, 1:usb report  */
    //uint32_t messageSize;
    /* when type is 0, ptr_val means 0:usb removed, 1:usb connect,*/
    //void *ptr_val;
    int32_t msg_type;    
    void *ptr_val;
    int32_t msg_size;
    ULONG sent_time;
} NFC_OUTPUT_QUEUE_MESSAGE;

bool enable_nfc(void);
bool disable_nfc(void);

void get_nfc_sw_version(uint8_t max_buff, uint8_t* version);
nfc_mode_t get_current_nfc_mode(void);
int get_nfc_mode();

bool switch_to_nfc_mode();
bool disable_all_nfc_modes();
bool switch_to_nfc_rw_mode();
bool enable_nfc_rw();
bool disable_nfc_rw();

bool wait_iso_dep_card();

#endif
