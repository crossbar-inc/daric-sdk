/**
 ******************************************************************************
 * @file    device_manager.c
 * @author  AP Team
 * @brief   This source file contains the implementation of functions related
 *          to device manager operations. It processes various device-related
 *          actions based on message types, including generating device keys,
 *          device validation, device binding, and more.
 *          Each action is handled by specific logic depending on the message
 *          type received.
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

#include <string.h>
#include "tx_api.h"
#include "tx_log.h"

#include "device_manager.h"

#include "common.h"
#include "daric_hal_reram.h"
#include "daric_gui.h"
#include "version.h"

#undef LOG_TAG
#define LOG_TAG "DEVICE_MGR"

#define LOG_FUNC_START() LOGD("%s: start", __func__)
#define LOG_FUNC_END(ret) LOGD("%s: end, ret = %d", __func__, ret)
#define SN_LEN 16

/*! \brief Retrieve the serial number of the device.
 *
 *  @param serial_number Pointer to a character buffer where the serial number will be stored.
 *  @param max_len Maximum length (in characters) of the serial number buffer.
 *  @return `RESULT_SUCCESS` on success.
 */
INT get_serial_number(CHAR* serial_number, UINT max_len) {
    LOG_FUNC_START();

    INT ret = RESULT_SUCCESS;
    CHAR sn[SN_LEN + 1];
    char *default_sn = "AC01000000000000";
    if (HAL_RERAM_Read(APP_DATA_SLOT_ID_SERIAL_NUM, (UCHAR *)sn, SN_LEN))
    {
        sn[SN_LEN] = '\0';
        if (sn[0] == 0x41 && sn[1] == 0x43)
        {
            strncpy(serial_number, sn, max_len);
        }
        else
        {
            strncpy(serial_number, default_sn, max_len);
        }
        serial_number[max_len - 1] = '\0';
    }
    else
    {
        strncpy(serial_number, default_sn, max_len);
        serial_number[max_len - 1] = '\0';
        ret = RESULT_FAILURE;
    }

    LOG_FUNC_END(ret);
    return ret;
}

/*! \brief Get the Firmware Verify Flag Store ReRAM address
 *
 * @return ReRAM address
 */
int32_t get_bloader_firmware_verify_result_addr()
{
    return BOOTLOADER_DATA_SLOT_FW_VERIFY_FLAG_ADDR;
}

/*! \brief Retrieve the bootloader version of the device.
 *
 *  @param bootlaoder_version Pointer to a character buffer where the bootloader version will be stored.
 *  @param max_len Maximum length (in characters) of the bootlaoder version buffer.
 *  @return `RESULT_SUCCESS` on success.
 */
INT get_bootloader_version(CHAR* bootlaoder_version, UINT max_len) {
    LOG_FUNC_START();

    INT ret = RESULT_SUCCESS;
    CHAR version[BOOTLOADER_VERSION_LEN + 1];
    char *default_version = "V202500000000";
    if (HAL_RERAM_Read(BOOTLOADER_DATA_SLOT_BLOADER_VERSION_ADDR, (UCHAR *)version, BOOTLOADER_VERSION_LEN))
    {
        version[BOOTLOADER_VERSION_LEN] = '\0';
        if (version[0] == 0x56)
        {
            strncpy(bootlaoder_version, version, max_len);
        }
        else
        {
            strncpy(bootlaoder_version, default_version, max_len);
        }
        bootlaoder_version[max_len - 1] = '\0';
    }
    else
    {
        strncpy(bootlaoder_version, default_version, max_len);
        ret = RESULT_SUCCESS;
    }

    LOG_FUNC_END(ret);
    return ret;
}

/*! \brief Retrieve the bluetooth addresss from ReRAM.
 *
 *  @param bt_addr Pointer to a character buffer where the bluetooth address will be stored.
 *  @param max_len Maximum length (in characters) of the bluetooth address buffer.
 *  @return `RESULT_SUCCESS` on success.
 */
INT get_bluetooth_address_from_reram(CHAR* bt_addr, UINT max_len) {
    LOG_FUNC_START();

    INT ret = RESULT_SUCCESS;
    CHAR address[BT_ADDR_SIZE + 5];
    memset(address, '\0', BT_ADDR_SIZE + 5);
    if (HAL_RERAM_Read(APP_PARAMETERS_BLUETOOTH_ADDRESS_ADDR, (UCHAR *)address, BT_ADDR_SIZE + 5))
    {
        LOGV("get bt address from ReRAM: %s", address);
        if (address[BT_ADDR_SIZE] == 0x41 && address[BT_ADDR_SIZE + 1] == 0x43
            && address[BT_ADDR_SIZE + 2] == 0x30 && address[BT_ADDR_SIZE + 3] == 0x31) ///< AC01
        {
            strncpy(bt_addr, address, max_len);
            bt_addr[max_len - 1] = '\0';
        }
        else
        {
            ret = RESULT_FAILURE;
        }
    }
    else
    {
        ret = RESULT_FAILURE;
    }

    LOG_FUNC_END(ret);
    return ret;
}

/*! \brief Writes the firmware version string into the ReRAM.
 *
 *  @param fw_version Pointer to a null-terminated string containing the firmware
 *                         version to be stored.
 *  @param version_len Length of the firmware version string in bytes.
 *  @return `RESULT_SUCCESS` on success.
 */
INT set_fw_version_to_reram(CHAR* fw_version, UINT version_len) {
    LOG_FUNC_START();

    INT ret = RESULT_SUCCESS;
    if (version_len > FW_VERSION_SIZE_MAX)
    {
        goto __exit;
    }
    CHAR stored_fw_version[FW_VERSION_SIZE_MAX + 1];
    memset(stored_fw_version, '\0', FW_VERSION_SIZE_MAX + 1);
    if (HAL_RERAM_Read(APP_PARAMETERS_FW_VERSION_ADDR, (UCHAR *)stored_fw_version, FW_VERSION_SIZE_MAX))
    {
        stored_fw_version[FW_VERSION_SIZE_MAX] = '\0';
        if (strcasecmp(stored_fw_version, fw_version) == 0)
        {
            ret = RESULT_FAILURE;
            goto __exit;
        }
    }

    LOGV("update fw version to ReRAM");
    memset(stored_fw_version, '\0', FW_VERSION_SIZE_MAX + 1);
    memcpy(stored_fw_version, (uint8_t *)fw_version, version_len);
    if (!HAL_RERAM_Write(APP_PARAMETERS_FW_VERSION_ADDR, (uint8_t *)stored_fw_version, FW_VERSION_SIZE_MAX))
    {
        ret = RESULT_FAILURE;
    }

__exit:
    LOG_FUNC_END(ret);
    return ret;
}

/*! \brief Retrieve the factory SN from ReRAM.
 *
 *  @param factory_sn Pointer to a character buffer where the factory SN will be stored.
 *  @param max_len Maximum length (in characters) of the factory SN buffer.
 *  @return `RESULT_SUCCESS` on success.
 */
INT get_factory_sn_from_reram(CHAR* factory_sn, UINT max_len) {
    LOG_FUNC_START();

    INT ret = RESULT_SUCCESS;
    CHAR serialNum[FACTORY_SN_SIZE_MAX + 1];
    memset(serialNum, '\0', FACTORY_SN_SIZE_MAX + 1);
    if (HAL_RERAM_Read(APP_PARAMETERS_FACTORY_SN_ADDR, (UCHAR *)serialNum, FACTORY_SN_SIZE_MAX + 1))
    {
        LOGV("get factory sn from ReRAM: %s", serialNum);
        if (serialNum[0] == 0x44 && serialNum[1] == 0x41) ///< DA
        {
            strncpy(factory_sn, serialNum, max_len);
            factory_sn[max_len - 1] = '\0';
        }
        else
        {
            ret = RESULT_FAILURE;
        }
    }
    else
    {
        ret = RESULT_FAILURE;
    }

    LOG_FUNC_END(ret);
    return ret;
}

/*! \brief Writes the bluetooth firmware version string into the ReRAM.
 *
 *  @param fw_version Pointer to a null-terminated string containing the bluetooth firmware
 *                         version to be stored.
 *  @param version_len Length of the bluetooth firmware version string in bytes.
 *  @return `RESULT_SUCCESS` on success.
 */
INT set_bt_fw_version_to_reram(CHAR* fw_version, UINT version_len) {
    LOG_FUNC_START();

    INT ret = RESULT_SUCCESS;
    if (version_len > BT_FW_VERSION_SIZE_MAX)
    {
        goto __exit;
    }
    CHAR stored_fw_version[BT_FW_VERSION_SIZE_MAX + 1];
    memset(stored_fw_version, '\0', BT_FW_VERSION_SIZE_MAX + 1);
    if (HAL_RERAM_Read(APP_PARAMETERS_BLUETOOTH_FIRMWARE_VERSION_ADDR, (UCHAR *)stored_fw_version, version_len))
    {
        stored_fw_version[BT_FW_VERSION_SIZE_MAX] = '\0';
        if (strcasecmp(stored_fw_version, fw_version) == 0)
        {
            ret = RESULT_FAILURE;
            goto __exit;
        }
    }

    LOGV("update bluetooth fw version to ReRAM");
    if (!HAL_RERAM_Write(APP_PARAMETERS_BLUETOOTH_FIRMWARE_VERSION_ADDR, (uint8_t *)fw_version, version_len))
    {
        ret = RESULT_FAILURE;
    }

__exit:
    LOG_FUNC_END(ret);
    return ret;
}