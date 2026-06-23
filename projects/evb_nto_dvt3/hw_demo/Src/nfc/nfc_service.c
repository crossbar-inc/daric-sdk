/**
 ******************************************************************************
 * @file    nfc_service.c
 * @author  AP Team
 * @brief   This file provides an external interface for NFC modules, 
 *          creating NFC threads and enable NFC HCE mode.
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
#include "nfc_service.h"
#include "nfc_hce_mode.h"
#include "user_threads_attrdef.h"

#include "tx_log.h"
#include "port.h"
#include "nfc_stage.h"
#include "port_hardware.h"

#include "time.h"

#undef LOG_TAG
#define LOG_TAG "NFC_TAG"

static nfc_mode_t current_mode = NFC_MODE_NONE;

extern uint8_t BSP_nfc_close();
extern uint8_t BSP_nfc_rw_close();

int get_nfc_mode()
{
    return (int)current_mode;
}

/**
  * @brief switch to NFC Mode
  * @retval true switch success, false switch failed
  */
bool switch_to_nfc_mode()
{
    if (current_mode == NFC_MODE_NFC)
    {
        LOGV("NFC mode is already active");
        return true;
    }

    if (current_mode == NFC_MODE_NFC_RW)
    {
        LOGV("RW mode is active, closing it first");
        if (!disable_nfc_rw())
        {
            LOGE("Failed to disable NFC RW mode");
            return false;
        }
    }

    if (current_mode != NFC_MODE_NONE) {
        LOGW("Unexpected active mode, forcing disable");
        current_mode = NFC_MODE_NONE;
    }

    bool result = enable_nfc();
    if (result)
    {
        current_mode = NFC_MODE_NFC;
        LOGV("Switched to NFC standard mode successfully");
    }
    else
    {
        LOGE("Failed to enable NFC standard mode");
        current_mode = NFC_MODE_NONE;
    }

    return result;
}

/**
  * @brief switch to NFC RW Mode
  * @retval true switch success, false switch failed
  */
bool switch_to_nfc_rw_mode()
{
    LOGD("%s: switching to nfc rw mode...", __func__);
    if (current_mode == NFC_MODE_NFC_RW)
    {
        LOGV("NFC RW mode is already active");
        return true;
    }

    if (current_mode == NFC_MODE_NFC)
    {
        LOGV("NFC mode is active, closing it first");
        if (!disable_nfc())
        {
            LOGE("Failed to disable NFC");
            return false;
        }
    }

    if (current_mode != NFC_MODE_NONE)
    {
        LOGW("Unexpected active mode, forcing disable");
        current_mode = NFC_MODE_NONE;
    }

    bool result = enable_nfc_rw();
    if (result)
    {
        current_mode = NFC_MODE_NFC_RW;
        LOGV("Switched to NFC RW mode successfully");
    }
    else
    {
        LOGE("Failed to enable NFC RW mode");
        current_mode = NFC_MODE_NONE;
    }

    return result;
}

/**
  * @brief disable all nfc
  * @retval true disable all nfc success, false isable all nfc failed
  */
bool disable_all_nfc_modes()
{
    bool result = true;

    switch (current_mode)
    {
        case NFC_MODE_NFC:
            result = disable_nfc();
            break;
        case NFC_MODE_NFC_RW:
            result = disable_nfc_rw();
            break;
        case NFC_MODE_NONE:
            LOGV("No NFC mode is active");
            return true;
        default:
            LOGW("Unknown NFC mode: %d", current_mode);
            disable_nfc();
            disable_nfc_rw();
            result = false;
            break;
    }

    if (result)
    {
        current_mode = NFC_MODE_NONE;
        LOGV("All NFC modes disabled successfully");
    } else {
        LOGE("Failed to disable NFC mode");
    }

    return result;
}

/**
  * @brief get current nfc mode
  * @retval 0-NFC_MODE_NONE, 1-NFC_MODE_NFC, 2-NFC_MODE_NFC_RW
  */
nfc_mode_t get_current_nfc_mode(void)
{
    return current_mode;
}

/**
  * @brief enable NFC function
  * @param null
  * @retval true Enable NFC successfully, false Enabling NFC failed
  */
bool enable_nfc()
{
    uint8_t result = 0;
    if (current_mode == NFC_MODE_NFC_RW)
    {
        NFCC_OpenCeMode();
        BSP_nfc_rw_close();
    }
    else if (current_mode == NFC_MODE_NFC)
    {
        BSP_nfc_close();
    }
    result = start_nfc_thread();
    LOGV("enable nfc result = %d", result);

    if (result)
    {
        current_mode = NFC_MODE_NFC;
    }
    else
    {
        current_mode = NFC_MODE_NONE;
    }
    return result;
}

/**
  * @brief disable NFC function
  * @param null
  * @retval true NFC disabled successfully, false Disabling NFC failed
  */
bool disable_nfc()
{
    uint8_t result = BSP_nfc_close();
    LOGV("disable nfc result = %d", result);
    current_mode = NFC_MODE_NONE;

    return result==0;
}

/**
  * @brief enable NFC RW function
  * @param null
  * @retval true Enable NFC RW successfully, false Enabling NFC failed
  */
bool enable_nfc_rw()
{
    uint8_t result = 0;
    if (current_mode == NFC_MODE_NFC_RW)
    {
        NFCC_OpenCeMode();
        BSP_nfc_rw_close();
    }
    else if (current_mode == NFC_MODE_NFC)
    {
        BSP_nfc_close();
    }
    result = start_nfc_rw_thread();
    LOGV("enable nfc rw result = %d", result);

    if (result)
    {
        current_mode = NFC_MODE_NFC_RW;
    }
    else
    {
        current_mode = NFC_MODE_NONE;
    }
    return result;
}

/**
  * @brief disable NFC function
  * @param null
  * @retval true NFC disabled successfully, false Disabling NFC failed
  */
bool disable_nfc_rw()
{
    NFCC_OpenCeMode();
    uint8_t result = BSP_nfc_rw_close();
    LOGV("disable nfc rw result = %d", result);
    current_mode = NFC_MODE_NONE;

    return result==0;
}

/**
  * @brief disable NFC function
  * @param null
  * @retval true NFC disabled successfully, false Disabling NFC failed
  */
void get_nfc_sw_version(uint8_t max_buff, uint8_t* version)
{
     char *ver_str = NFCC_GetVersion();
     size_t src_len = strlen(ver_str);
     size_t copy_len = (src_len < max_buff) ? src_len : (max_buff - 1);
     memcpy(version, ver_str, copy_len);
     version[copy_len] = '\0';
}

bool get_nfc_card_serial_number(char *uid)
{
    if(current_mode != NFC_MODE_NFC_RW)
    {
        LOGV("Get uid must be in RW mode!");
        return false;
    }

    CARD_PARA *pCard = NFCC_RwGetInfo();
    if (pCard == NULL || pCard->uid_length == 0) {
        LOGV("Can't get NFC card info!");
        return false;
    }

    char *p = uid;
    for (int i = 0; i < pCard->uid_length; i++) {
        if (i > 0) {
            *p++ = ':';
        }
        sprintf(p, "%02X", pCard->uid[i]);
        p += 2;
    }
    
    *p = '\0';
    return true;
}

bool wait_iso_dep_card()
{
    if (current_mode != NFC_MODE_NFC_RW)
    {
        LOGE("Not nfc rw mode, need open nfc rw mode!");
        return false;
    }
    LOGV("------Wait iso dep card------");
    int result = NFCC_RwWaitCard(NCI_RF_PROTOCOL_ISO_DEP, NO_TIMEOUT_FOREVER);
    return result == 0;
}