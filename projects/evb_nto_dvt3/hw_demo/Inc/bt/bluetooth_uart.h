/**
 ******************************************************************************
 * @file    bluetooth_uart.h
 * @author  AP Team
 * @brief   This file provides an external interface for BT modules.
******************************************************************************
* @attention
*
* @ Copyright CrossBar, Inc. 2024.
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
#ifndef __BLUETOOTH_UART_H
#define __BLUETOOTH_UART_H
#include   "common.h"

#ifndef SUPPORT_BLE
#define SUPPORT_BLE
#endif

bool disable_bluetooth();
int set_ble_name(const char *name);
int get_ble_name(char* name);
int get_ble_addr(char* address);
bool enable_bluetooth(void);
bool set_ble_invisible();
uint8_t get_ble_address(char* address);
uint8_t set_bt_visibility(uint8_t value);
uint8_t send_bluetooth_packets(const uint8_t *data, uint8_t size);
uint8_t get_bluetooth_version(char* version);
bool is_ble_enabled();
bool is_ble_connected();
int get_ble_version(char *version);
int get_ble_baud(char *baud);
int get_ble_manufacturer_name(char *manufacturer_name);
int get_ble_model_name(char *model_name);
int get_ble_firmware_name(char *firmware_name);
int get_ble_addr(char* address);
int send_ble_packets(const char* handle, const uint8_t *data, uint8_t size);
int set_ble_adv_status(uint8_t adv_status);
int get_ble_adv_status(char *adv_status);
int get_ble_adv_param(char *adv_param);
int get_ble_adv_data(char *adv_data);
int set_ble_adv_param(uint16_t adv_interval);
int set_ble_adv_data(const uint8_t *data, uint8_t length);
bool bluetooth_reset();
void update_name_with_addr();
bool update_ble_adv_data(const char *name);
bool update_name(const char *name);
bool unpair_device();
bool bluetooth_reset();
void reset_uart_software_state();
void set_ble_enabled(bool enabled);
void set_ble_connected(bool connected);
bool unbond_device();
bool disconnect_ble_connection();
bool set_ble_addr(const char *addr_data, uint8_t addr_len);
/**
 * @brief send remove bond cmd
 * @param null
 * @retval 0 success,-1 fail.
 */
int send_remove_bond_cmd();
typedef struct {
    uint32_t msg_type;
} BT_SERVICE_MESSAGE;

typedef struct {
    uint8_t len;
    uint8_t buf[32];
} ble_msg_t;

#endif
