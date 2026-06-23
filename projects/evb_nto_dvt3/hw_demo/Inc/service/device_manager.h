/**
 ******************************************************************************
 * @file    device_manager.h
 * @author  AP Team
 * @brief   This header file contains the declarations for the device manager
 *          functionality, including macros, data structures, message types,
 *          result codes, and function prototypes. It provides the interface
 *          for sending messages to the device manager.
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

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "common.h"
#include "daric_hal.h"

#define BOOTLOADER_VERSION_LEN                                13 ///< bootloader version length
#define BT_ADDR_SIZE                                          12 ///< bluetooth address length
#define BT_FW_VERSION_SIZE_MAX                                10 ///< bluetooth firmware version length
#define FW_VERSION_SIZE_MAX                                   10 ///< firmware version length
#define FACTORY_SN_SIZE_MAX                                   14 ///< factory sn length

#define KEY_SLOT_BASE_ADDR                           (0x603F0000) // Start of key slots, total 64KB
#define KEY_SLOT_ITEM_SIZE                           (0x20)       // 32 bytes for each key slot
#define BOOTLOADER_KEY_SLOT_ID0                      (0x140)        // First key slot of BootROM
#define BOOTLOADER_KEY_SLOT_BASE_ADDR                (KEY_SLOT_BASE_ADDR + BOOTLOADER_KEY_SLOT_ID0 * KEY_SLOT_ITEM_SIZE)
#define BOOTLOADER_KEY_SLOT_ID_ED_PUBKEY             (0x0)        //five public keys for ED25519, each 32 bytes for one slot
#define BOOTLOADER_KEY_SLOT_ID_DEVICE_ED_PUBKEY      (0x5)        //device's public key for ED25519, each 32 bytes for one slot
#define BOOTLOADER_KEY_SLOT_ID_DEVICE_ED_PRIVKEY     (0x6)        //device's private key for ED25519, each 32 bytes for one slot
#define BOOTLOADER_KEY_SLOT_ID_VALID_SERVER_PUBKEY   (0x7)        //valid server's public key for ED25519, each 32 bytes for one slot
#define BOOTLOADER_DEVICE_ED_PUBKEY_ADDR             (BOOTLOADER_KEY_SLOT_BASE_ADDR + BOOTLOADER_KEY_SLOT_ID_DEVICE_ED_PUBKEY * KEY_SLOT_ITEM_SIZE)
#define BOOTLOADER_DEVICE_ED_PRIVKEY_ADDR            (BOOTLOADER_KEY_SLOT_BASE_ADDR + BOOTLOADER_KEY_SLOT_ID_DEVICE_ED_PRIVKEY * KEY_SLOT_ITEM_SIZE)
#define BOOTLOADER_VALID_SERVER_PUBKEY_ADDR          (BOOTLOADER_KEY_SLOT_BASE_ADDR + BOOTLOADER_KEY_SLOT_ID_VALID_SERVER_PUBKEY * KEY_SLOT_ITEM_SIZE)

#define DATA_SLOT_BASE_ADDR                          (0x603E0000) // Start of data slots, total 64KB
#define DATA_SLOT_ITEM_SIZE                          (0x20)       // 32 bytes for each data slot
#define BOOTLOADER_DATA_SLOT_APP_INFO                (0x140)      // app info data slot of BootLoader
#define BOOTLOADER_DATA_SLOT_BASE_ADDR               (DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_APP_INFO * DATA_SLOT_ITEM_SIZE)
#define BOOTLOADER_DATA_SLOT_ID_APP_HEADER           (0x0)    //magic+date+time+version+size
#define BOOTLOADER_DATA_SLOT_ID_APP_SHA256           (0x1)
#define BOOTLOADER_DATA_SLOT_ID_SERIAL_NUM           (0x3)    //32 bytes, 1 data slots
#define BOOTLOADER_DATA_SLOT_ID_CERTIFICATION        (0x4)    //certification size: 256 bytes, 8 data slots
#define BOOTLOADER_DATA_SLOT_ID_FW_VERIFY_FLAG       (0x10)   //Firmware verify flag for secure download: 32 bytes, 1 data slot
#define BOOTLOADER_DATA_SLOT_ID_VERSION              (0x12)   //Store bootlaoder version: 32 bytes, 1 data slot
#define BOOTLOADER_DATA_SLOT_FW_VERIFY_FLAG_ADDR     (BOOTLOADER_DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_ID_FW_VERIFY_FLAG * DATA_SLOT_ITEM_SIZE)
#define BOOTLOADER_DATA_SLOT_BLOADER_VERSION_ADDR    (BOOTLOADER_DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_ID_VERSION * DATA_SLOT_ITEM_SIZE)
#define BOOTLOADER_ED_PUBKEY_ADDR                    (BOOTLOADER_KEY_SLOT_BASE_ADDR + BOOTLOADER_KEY_SLOT_ID_ED_PUBKEY * KEY_SLOT_ITEM_SIZE)
#define APP_IMAGE_INFO_ADDR                          (BOOTLOADER_DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_ID_APP_HEADER * DATA_SLOT_ITEM_SIZE)
#define APP_IMAGE_HASH_ADDR                          (BOOTLOADER_DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_ID_APP_SHA256 * DATA_SLOT_ITEM_SIZE)
#define APP_DATA_SLOT_ID_SERIAL_NUM                  (BOOTLOADER_DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_ID_SERIAL_NUM*DATA_SLOT_ITEM_SIZE)
#define APP_DATA_SLOT_ID_CERTIFICATION               (BOOTLOADER_DATA_SLOT_BASE_ADDR + BOOTLOADER_DATA_SLOT_ID_CERTIFICATION*DATA_SLOT_ITEM_SIZE)

#define BOOTROM_SYS_IFR_ID0                          (0xD)
#define BOOTROM_SYS_IFR_CNT                          (0x4)   // 4 words reserved by BootROM
#define BOOTROM_SYS_IFR_BASE_ADDR                    (SYS_IFR_BASE_ADDR + BOOTROM_SYS_IFR_ID0 * SYS_IFR_WORD_SIZE)
#define BOOTROM_SYS_IFR_ID_SECURE_FEATURE            (0x0)   //secure feature 0: disable; 1: enable
#define BOOTROM_SYS_IFR_ID_GUID                      (0x2)   //32 bytes, two IFR words (only 16 bytes IP0 field of each IFR word available for data storage)

#define BOOTLOADER_SYS_IFR_ID0                       (0x11)
#define BOOTLOADER_SYS_IFR_CNT                       (0x1)   // 1 words reserved by Bootloader
#define BOOTLOADER_SYS_IFR_BASE_ADDR                 (SYS_IFR_BASE_ADDR + BOOTLOADER_SYS_IFR_ID0 * SYS_IFR_WORD_SIZE)
#define BOOTLOADER_SYS_IFR_ID_PUBKEY_PRESENT         (0x0)   //public key present 0: false; 1: true

#define APP_PARAMETERS_START_ADDR                    (0x602C0000)
#define APP_PARAMETERS_END_ADDR                      (0x602D9FFF)
#define APP_PARAMETERS_ITEM_SIZE                     (0x20)
#define APP_PARAMETERS_LCD_VOLATAGE_ADDR             (0x602C8000)
#define APP_PARAMETERS_BLUETOOTH_ADDRESS_ADDR        (APP_PARAMETERS_LCD_VOLATAGE_ADDR + APP_PARAMETERS_ITEM_SIZE)
#define APP_PARAMETERS_BLUETOOTH_ADDRESS_WRITED_FLAG_ADDR (APP_PARAMETERS_BLUETOOTH_ADDRESS_ADDR + BT_ADDR_SIZE)
#define APP_PARAMETERS_BLUETOOTH_FIRMWARE_VERSION_ADDR (APP_PARAMETERS_BLUETOOTH_ADDRESS_WRITED_FLAG_ADDR + 4)
#define APP_PARAMETERS_FACTORY_SN_ADDR               (APP_PARAMETERS_BLUETOOTH_ADDRESS_ADDR + APP_PARAMETERS_ITEM_SIZE)
#define APP_PARAMETERS_FW_VERSION_ADDR               (APP_PARAMETERS_FACTORY_SN_ADDR + APP_PARAMETERS_ITEM_SIZE)
#define APP_PARAMETERS_DEVICE_VALIDATION_STATUS_ADDR (APP_PARAMETERS_FW_VERSION_ADDR + APP_PARAMETERS_ITEM_SIZE)

/// Device result codes.
typedef enum {
    RESULT_SUCCESS = 0,                        ///< success
    RESULT_FAILURE = -1,                       ///< failure
    RESULT_MALLOC_FAILED = -2,                 ///< memory allocation failed
    RESULT_FILE_EMPTY = -3,                    ///< file is empty
    RESULT_READ_FILE_FAILED = -4,              ///< file read failed
    RESULT_WRITE_FILE_FAILED = -5,             ///< file write failed
    RESULT_DELETE_FILE_FAILED = -6,            ///< file deletion failed
    RESULT_KEYPAIR_GENERATION_FAILED = -7,     ///< keypair generation failed
    RESULT_GET_SERIAL_NUMBER_FAILED = -8,      ///< get serial number failed
    RESULT_SERIALIZATION_FAILED = -9,          ///< serialization failed
    RESULT_DESERIALIZATION_FAILED = -10,       ///< deserialization failed
} DeviceResultCode;

INT get_serial_number(CHAR* serial_number, UINT max_len);
int32_t get_bloader_firmware_verify_result_addr();

INT get_bootloader_version(CHAR* bootlaoder_version, UINT max_len);
INT get_bluetooth_address_from_reram(CHAR* bt_addr, UINT max_len);
INT set_bt_fw_version_to_reram(CHAR* fw_version, UINT version_len);
INT set_fw_version_to_reram(CHAR* fw_version, UINT version_len);
INT get_factory_sn_from_reram(CHAR* factory_sn, UINT max_len);
#endif  // DEVICE_MANAGER_H
