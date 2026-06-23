/**
 ******************************************************************************
 * @file    bluetooth_uart.c
 * @author  AP Team
 * @brief   This file provides an external interface for BT modules,
 *          creating BT threads and trans, receive and transmit Bluetooth
 *          data through interrupt mode.
 ******************************************************************************
 * @attention
 *
 * Copyright CrossBar, Inc. 2024.
 *
 * All rights reserved.
 *
 * This software is the proprietary property of CrossBar, Inc. and is protected
 * by copyright laws. Any unauthorized reproduction, distribution, or
 * modification is strictly prohibited.
 *
 ******************************************************************************
 */
#include "bluetooth_uart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "daric_hal_def.h"
#include "tx_api.h"
#include "bluetooth_uart.h"
#include <stdbool.h>

#include "user_threads_attrdef.h"
#include "tx_log.h"
// #include "message_type.h"

#include "daric_pmu.h"

#include "daric_bt.h"

#undef LOG_TAG
#define LOG_TAG "BT_TAG"

#define BT_SERVICE_MSG_TYPE_SYSTEM_READY 0
#define BT_SERVICE_MSG_TYPE_CONNECTED 1
#define BT_SERVICE_MSG_TYPE_DISCONNECTED 2
#define BT_SERVICE_MSG_TYPE_PASSKEY 3
#define BT_SERVICE_MSG_TYPE_MPC_CONTINUOUS 4
#define BT_SERVICE_MSG_TYPE_MPC_CONT_RECEIVE 5
#define BT_SERVICE_MSG_TYPE_MPC 6
#define BT_SERVICE_MSG_TYPE_FIDO 7
#define BT_SERVICE_MSG_TYPE_MPC_RECV_ACK 9 
#define BT_SERVICE_MSG_TYPE_RAW_DATA 10
#define BT_SERVICE_MSG_RECEIVE_ERROR 11

#define RX_BUFF_LEN 32
#define BLE_NAME_MAX_LEN 29
#define MPC_RECV_BLE_FINISH 0X10
#define MPC_BLE_SEND_ACK 0x100

#define BT_RESPONSE_MSG_READY 0
#define BT_RESPONSE_MSG_CONNECT 1
#define BT_RESPONSE_MSG_DISCONNECT 2
#define BT_RESPONSE_MSG_PASSKEY 3
#define BT_RESPONSE_MSG_FFF2 4
#define BT_RESPONSE_MSG_GVER 5
#define BT_RESPONSE_MSG_GCFGVER 6
#define BT_RESPONSE_MSG_MANUFACTURER 7
#define BT_RESPONSE_MSG_MODEL 8
#define BT_RESPONSE_MSG_GET_NAME 9
#define BT_RESPONSE_MSG_ADDRESS 10
#define BT_RESPONSE_MSG_ADDRESS_ERROR 11
#define BT_RESPONSE_MSG_BAUD 12
#define BT_RESPONSE_MSG_UNBOUND 13
#define BT_RESPONSE_MSG_SET_ADV_STATE 14
#define BT_RESPONSE_MSG_GET_ADV_STATE 15
#define BT_RESPONSE_MSG_SET_ADVPARAM 16
#define BT_RESPONSE_MSG_GET_ADVPARAM 17
#define BT_RESPONSE_MSG_SET_ADVDATA 18
#define BT_RESPONSE_MSG_GET_ADVDATA 19
#define GET_MPC_TIME 22
#define START_MPC 23

#define BT_RESPONSE_MSG_SET_NAME 24
#define BT_RESPONSE_MSG_SET_ADDRESS 25
#define BT_RESPONSE_MSG_BOND_STATE 26

#define BLE_GET_NAME_EVENT 0x01
#define BLE_GET_ADDR_EVENT 0x02
#define BLE_GET_GVER_EVENT 0x03
#define BLE_GET_GCFGVER_EVENT 0x04
#define BLE_GET_MANUFACTURER_EVENT 0x05
#define BLE_GET_MODEL_EVENT 0x06
#define BLE_GET_BAUD_EVENT 0x07
#define BLE_SET_ADV_EVENT 0x08
#define BLE_GET_ADV_EVENT 0x09
#define BLE_SET_ADV_DATA_EVENT 0x0A
#define BLE_GET_ADV_DATA_EVENT 0x0B
#define BLE_FIDO_DATA_EVENT 0x0C
#define BLE_FFF2_REESPONSE_EVENT 0x0D
#define BLE_SET_NAME_EVENT 0x0E
#define BLE_SET_UNBOUND_EVENT 0x10
#define BLE_SET_ADV_PARAM_EVENT 0x11
#define BLE_GET_ADV_PARAM_EVENT 0x12
#define BLE_SET_ADV_ADDR_EVENT 0x13
#define BLE_SET_ADDR_EVENT 0x14

#define CONTROL_POINT_FLAG 1
#define STATUS_FLAG 2
#define CONTROL_POINT_LEN_FLAG 3
#define SERVICE_REVISION_FLAG 4
#define BLE_SYSTEM_SERVICE_FLAG 5
#define BLE_MTU_512 512
#define BLE_MTU_488 488
#define BLE_MTU_244 244
#define BLE_MTU_200 200
#define SUCCESS_ACK_LEN 8
#define READ_DATA_DELAY_TIME 5000
#define AT_COM_PRE_LEN 7

#define BLE_CMD_TIMEOUT_MS  2000

static ble_msg_t g_ble_name = {0};
static ble_msg_t g_ble_addr = {0};
static ble_msg_t g_ble_gver = {0};
static ble_msg_t g_ble_gcfgver = {0};
static ble_msg_t g_ble_manufacturer = {0};
static ble_msg_t g_ble_model = {0};
static ble_msg_t g_ble_baud = {0};
static ble_msg_t g_ble_unbound = {0};
static ble_msg_t g_ble_adv = {0};
static ble_msg_t g_ble_adv_param = {0};
static ble_msg_t g_ble_adv_data = {0};
static ble_msg_t s_ble_name = {0};
static ble_msg_t s_ble_addr = {0};
static ble_msg_t s_ble_adv = {0};
static ble_msg_t s_ble_adv_data = {0};
static ble_msg_t s_ble_adv_param = {0};

extern TX_EVENT_FLAGS_GROUP event_flags;
TX_QUEUE bluetooth_queue;
TX_EVENT_FLAGS_GROUP ble_stream_switch_flags;
static TX_THREAD bluetooth_thread;
TX_EVENT_FLAGS_GROUP ble_event_flags;
extern TX_TIMER ble_resend_timer;

static char g_fff2_response[RX_BUFF_LEN] = {0};
static bool is_bluetooth_enabled = false;
static bool is_bluetooth_connected = false;

// static int gTotalPackages = 0;
 int gRequestMtu = 488;
 int gRealMtu = 0;

static int gRecvTime = 0;
// static uint8_t rx_buffer[RX_BUFF_LEN] = {0};
static uint8_t rx_temp_buffer[RX_BUFF_LEN] = {0};

typedef enum {
    BLE_OK = 0,

    BLE_ERR_FAIL = -1,
    BLE_ERR_INVALID_PARAM = -2,
    BLE_ERR_TIMEOUT = -3,
    BLE_ERR_BUSY = -4,
    BLE_ERR_NO_MEMORY = -5,
    BLE_ERR_RESP_BAD = -6,

    BLE_ERR_NAME_INVALID_LEN = -10,
    BLE_ERR_SET_NAME_ENCODE = -11,
    BLE_ERR_SET_NAME_TX_FAIL = -12,
    BLE_ERR_GET_NAME_TX_FAIL = -13,
    BLE_ERR_GET_ADDR_TX_FAIL = -14,
    BLE_ERR_GET_ADDR_RX_LEN_INVALID = -15,
    BLE_ERR_GET_BLE_VERSION_TX_FAIL = -16,
    BLE_ERR_GET_BLE_FIRMWARE_NAME_TX_FAIL = -17,
    BLE_ERR_GET_BLE_MANUFACTURER_NAME_TX_FAIL = -18,
    BLE_ERR_GET_BLE_MODEL_NAME_TX_FAIL = -19,
    BLE_ERR_GET_BLE_BAUD_TX_FAIL = -20,
    BLE_ERR_BAUD_ENCODE = -21,
    BLE_ERR_SET_BLE_BAUD_TX_FAIL = -22,
    BLE_ERR_ADV_STATUS_ENCODE = -23,
    BLE_ERR_SET_BLE_ADV_STATUS_TX_FAIL = -24,
    BLE_ERR_GET_BLE_ADV_STATUS_TX_FAIL = -25,
    BLE_ERR_SET_ADV_PARAM_ENCODE = -26,
    BLE_ERR_SET_BLE_ADV_PARAM_TX_FAIL = -27,
    BLE_ERR_GET_BLE_ADV_PARAM_TX_FAIL = -28,
    BLE_ERR_SET_BLE_ADV_DATA_TX_FAIL = -29,
    BLE_ERR_GET_BLE_ADV_DATA_TX_FAIL = -30,

    BLE_ERR_UART_TX = -200,
    BLE_ERR_UART_RX = -201,
    BLE_ERR_UART_INIT = -202,

    BLE_ERR_EVENT_GET = -300,
    BLE_ERR_EVENT_SET = -301,
} ble_err_t;

static inline void ble_msg_store(ble_msg_t *msg,
                                 const uint8_t *src,
                                 uint8_t        len)
{
    if (len > sizeof(msg->buf)) len = sizeof(msg->buf);
    msg->len = len;
    memcpy(msg->buf, src, len);
}

uint8_t get_handle_flag(uint8_t *packet_data)
{
    uint16_t handle = (packet_data[0] << 8) | packet_data[1];
    LOGV("# [%s] Parsed handle: 0x%04X", __func__, handle);
    uint8_t handle_flag = 0;

    if (handle == 0x8800)
    {
        handle_flag = 1;
    }
    else if (handle == 0x8a00)
    {
        handle_flag = 2;
    }
    else if (handle == 0x8d00)
    {
        handle_flag = 3;
    }
    else if (handle == 0x9100)
    {
        handle_flag = 4;
    }
    else if (handle == 0x1800)
    {
        handle_flag = 5; // Generic Access
    }

    return handle_flag;
}

void print_complete_data_packet(const char *func_name, const uint8_t *packet_data, uint8_t data_length)
{
    LOGV("# [%s] Complete data packet: ", func_name);
    for (uint8_t i = 0; i < data_length; i++)
    {
        LOGV_RAW("%02X ", packet_data[i]);
    }
    LOGV_RAW("\r\n");
}
void postMessage(BT_SERVICE_MESSAGE *message)
{
    UINT status = tx_queue_send(&bluetooth_queue, message, TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        printf("bt_msg,sendmessage error! %d", status);
    }
}
void create_ble_event_flags()
{
    UINT status = tx_event_flags_create(&ble_event_flags, "BLE Event Flags");
    if (status != TX_SUCCESS)
    {
      LOGE("Error: Failed to create BLE Event Flags!");
    }
    else
    {
      LOGV("BLE Event Flags created successfully.");
    }
}

void create_ble_stream_event_flags()
{
    UINT status = tx_event_flags_create(&ble_stream_switch_flags, "BLE Stream switch Flags");

    if (status != TX_SUCCESS)
    {
        LOGE("Error: Failed to create BLE Stream switch Flags!");
    }
    else
    {
        tx_event_flags_set(&ble_stream_switch_flags, MPC_RECV_BLE_FINISH, TX_OR);
        printf("\nBLE Stream switch Flags created successfully.\n");
    }
}

void report_ble_status_data(uint8_t *data)
{
    BT_SERVICE_MESSAGE send_message;
    if (strstr((char *)data, "IM_CONN"))
    {
        send_message.msg_type = BT_SERVICE_MSG_TYPE_CONNECTED;
        postMessage(&send_message);
    }
    else if (strstr((char *)data, "IM_DISC"))
    {
        send_message.msg_type = BT_SERVICE_MSG_TYPE_DISCONNECTED;
        postMessage(&send_message);
    }
    else if (strstr((char *)data, "IM_READY"))
    {
        send_message.msg_type = BT_SERVICE_MSG_TYPE_SYSTEM_READY;
        postMessage(&send_message);
    }
}

void mpc_rcv_data_ack_handle(const uint8_t *data)
{
    
    BT_SERVICE_MESSAGE send_message;
    gRecvTime = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];

    send_message.msg_type = BT_SERVICE_MSG_TYPE_MPC_RECV_ACK;
    postMessage(&send_message);
}

void process_received_data(uint8_t *data, uint8_t length)
{
    switch (data[1])
        {
        case BT_RESPONSE_MSG_READY:
        case BT_RESPONSE_MSG_CONNECT:
        case BT_RESPONSE_MSG_DISCONNECT:
            uint8_t bt_status_len = data[2];
            uint8_t bt_status[30] = {0};
            memcpy(bt_status, &data[3], bt_status_len);
            LOGV("bt_status_len = %d", bt_status_len);
            report_ble_status_data(bt_status);
            break;
        case BT_RESPONSE_MSG_PASSKEY:
            uint8_t passkey_len = data[2];
            uint8_t passkey[6] = {0};
            memcpy(passkey, &data[3], passkey_len);
            LOGV("passkey_len = %d", passkey_len);
            // report_ble_passkey(passkey, passkey_len);
            break;
        case GET_MPC_TIME:
            uint8_t mpc_time_len = data[2];
            uint8_t mpc_time[RX_BUFF_LEN] = {0};
            LOGV("GET_MPC_TIME mpc_time_len = %d", mpc_time_len);
            if (mpc_time_len > 0 && (3 + mpc_time_len) <= length)
            {
                memset(mpc_time, 0, sizeof(mpc_time));
                memcpy(mpc_time, &data[3], mpc_time_len);
                mpc_time[mpc_time_len]='\0';
                mpc_rcv_data_ack_handle(mpc_time);
                printf("GET_MPC_TIME mpc_time = ");
                for (int i=0;i<mpc_time_len;i++)
                {
                    printf("%0x ",mpc_time[i]);
                }
                printf("\r\n");
            } else {
                LOGE("GET_MPC_TIME: invalid length %u", mpc_time_len);
            }
            break;
        default:
            for (int i = 0; i < length; i++)
            {
                printf("rx_buffer[%d] = %0x\n", i, data[i]);
            }
            bt_uart_reset();
            break;
    }
}

void loop_bt_message()
{
    BT_SERVICE_MESSAGE received_message;

    // uint8_t *received_cont_data = NULL;

    while (1)
    {
        UINT status = tx_queue_receive(&bluetooth_queue, &received_message, TX_WAIT_FOREVER);
        if (status == TX_SUCCESS)
        {
            switch (received_message.msg_type)
            {
            case BT_SERVICE_MSG_RECEIVE_ERROR:
                LOGV("Receive header error, data[0] = %d, data[1] = %d", rx_temp_buffer[0], rx_temp_buffer[1]);
                bt_uart_reset();
                break;
            case BT_SERVICE_MSG_TYPE_RAW_DATA:
                LOGV("BT_SERVICE_MSG_TYPE_RAW_DATA");
                process_received_data(rx_temp_buffer, RX_BUFF_LEN);
                break;
            case BT_SERVICE_MSG_TYPE_SYSTEM_READY:
                LOGV("BLE system ready");
                // send_packets_to_data_service(NOTIFY_BLE_SYSTEM_READY, NULL, 0);
                break;
            case BT_SERVICE_MSG_TYPE_CONNECTED:
                set_ble_connected(true);

                LOGV("BLE connection established");
                break;
            case BT_SERVICE_MSG_TYPE_DISCONNECTED:
                LOGV("BLE connection disconnected");
                set_ble_connected(false);
                break;
            case BT_SERVICE_MSG_TYPE_PASSKEY:
                LOGV("Obtained BLE pairing key");
                break;
            default:
                break;
            }
        }
    }
}

bool create_bt_queue()
{
    VOID *pointer = NULL;
    pointer = malloc(10 * sizeof(BT_SERVICE_MESSAGE));
    if (pointer == NULL) {
        LOGE("malloc failed");
        free(pointer);
        return false;
    }

    /*Create the message queue shared by threads BT*/
    UINT status = tx_queue_create(&bluetooth_queue, "BTservice Queue", sizeof(BT_SERVICE_MESSAGE) / 4, pointer, 10 * sizeof(BT_SERVICE_MESSAGE));

    if (status == TX_SUCCESS)
    {
        LOGV("create BT Queue success");
    }
    else
    {
        LOGE("create BT Queue fail");
        free(pointer);
        return false;
    }
    return true;
}

static void bt_uart_rx_callback(const uint8_t *data, uint16_t len)
{
    if (data[0] == 0x63)
    {
        // start_mpc_continuous(data, len);
    }
    else if (data[0] == 0x5A)
    {
        size_t copy_len = (len < RX_BUFF_LEN) ? len : RX_BUFF_LEN;
        memcpy(rx_temp_buffer, data, copy_len);
        if (len < 2)
        {
            LOGE("RX data too short");
            return;
        }

        bt_uart_receive_it();
        switch(rx_temp_buffer[1])
        {
        case BT_RESPONSE_MSG_FFF2:
            if (len < 3)
            {
                LOGE("RX data too short for FFF2");
                break;
            }
            if (rx_temp_buffer[2] > 0 && rx_temp_buffer[2] < RX_BUFF_LEN) {
                memset(g_fff2_response, 0, sizeof(g_fff2_response));
                memcpy(g_fff2_response, &rx_temp_buffer[3], rx_temp_buffer[2]);
                tx_event_flags_set(&ble_event_flags, BLE_FFF2_REESPONSE_EVENT, TX_OR);
            } else {
                LOGE("GET_ADDR: invalid length %u", rx_temp_buffer[2]);
            }
            break;
        case BT_RESPONSE_MSG_SET_NAME:
            ble_msg_store(&s_ble_name, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_SET_NAME_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_GET_NAME:
            ble_msg_store(&g_ble_name, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_NAME_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_ADDRESS:
        case BT_RESPONSE_MSG_ADDRESS_ERROR:
            ble_msg_store(&g_ble_addr, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_ADDR_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_SET_ADDRESS:
            ble_msg_store(&s_ble_addr, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_SET_ADDR_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_GVER:
            ble_msg_store(&g_ble_gver, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_GVER_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_GCFGVER:
            ble_msg_store(&g_ble_gcfgver, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_GCFGVER_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_MANUFACTURER:
            ble_msg_store(&g_ble_manufacturer, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_MANUFACTURER_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_MODEL:
            ble_msg_store(&g_ble_model, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_MODEL_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_BAUD:
            ble_msg_store(&g_ble_baud, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_BAUD_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_UNBOUND:
            ble_msg_store(&g_ble_unbound, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_SET_UNBOUND_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_SET_ADV_STATE:
            ble_msg_store(&s_ble_adv, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_SET_ADV_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_GET_ADV_STATE:
            ble_msg_store(&g_ble_adv, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_ADV_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_SET_ADVPARAM:
            ble_msg_store(&s_ble_adv_param, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_SET_ADV_PARAM_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_GET_ADVPARAM:
            ble_msg_store(&g_ble_adv_param, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_ADV_PARAM_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_SET_ADVDATA:
            ble_msg_store(&s_ble_adv_data, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_SET_ADV_DATA_EVENT, TX_OR);
            break;
        case BT_RESPONSE_MSG_GET_ADVDATA:
            ble_msg_store(&g_ble_adv_data, &rx_temp_buffer[3], rx_temp_buffer[2]);
            tx_event_flags_set(&ble_event_flags, BLE_GET_ADV_DATA_EVENT, TX_OR);
            break;
        default:
            BT_SERVICE_MESSAGE rx_message;
            rx_message.msg_type = BT_SERVICE_MSG_TYPE_RAW_DATA;
            tx_queue_send(&bluetooth_queue, &rx_message, TX_NO_WAIT);
            break;
        }
    }
    else
    {
        BT_SERVICE_MESSAGE rx_message;
        rx_message.msg_type = BT_SERVICE_MSG_RECEIVE_ERROR;
        tx_queue_send(&bluetooth_queue, &rx_message, TX_NO_WAIT);
    }
}

void bluetooth_entry(ULONG arg)
{
    T_ThreadsDepType *config = (T_ThreadsDepType *)arg;
    bluetooth_thread = config->thread;

    int result = bt_uart_init();
    if (result != HAL_OK)
    {
        LOGE("Bluetooth initialization failed");
        return;
    }

    /*Turn Bluetooth off first after power on*/
    disable_bluetooth();

    printf("\r\nbluetooth_entry tx_event_flags_set MPC_RECV_BLE_FINISH\r\n");
    create_ble_event_flags();
    create_ble_stream_event_flags();

    bt_uart_register_rx_callback(bt_uart_rx_callback);
    bt_uart_receive_it();

    create_bt_queue();
    loop_bt_message();
    LOGV("bt thread exit!");
}

static bool enable_disable_bt_in_progress = false;
static volatile ULONG last_power_operation_time = 0;
#define MIN_POWER_OPERATION_INTERVAL (500 / TX_TIMER_TICKS_PER_SECOND) // 500ms

bool enable_bluetooth(void)
{
    ULONG current_time = tx_time_get();
    if (current_time - last_power_operation_time < MIN_POWER_OPERATION_INTERVAL) {
        LOGW("Bluetooth power operation too frequent, please wait");
        return false;
    }

    if (enable_disable_bt_in_progress) {
        LOGW("Bluetooth power operation already in progress");
        return false;
    }

    enable_disable_bt_in_progress = true;

    bt_power_on();

    enable_disable_bt_in_progress = false;
    last_power_operation_time = current_time;
    set_ble_enabled(true);

    return true;
}

/**
 * @brief disconnect ble
 * @param null
 * @retval true - success, false - fail
 */
bool disable_bluetooth()
{
    ULONG current_time = tx_time_get();

    if (current_time - last_power_operation_time < MIN_POWER_OPERATION_INTERVAL) {
        LOGW("Bluetooth power operation too frequent, please wait");
        return false;
    }

    if (enable_disable_bt_in_progress) {
        LOGW("Bluetooth power operation already in progress");
        return false;
    }

    bt_power_off();

    enable_disable_bt_in_progress = false;
    last_power_operation_time = current_time;
    set_ble_enabled(false);
    set_ble_connected(false);
    return true;
}

/**
 * @brief Set the name of the Bluetooth chip
 * @param name - Name that needs to be set
 * @retval 0 success, otherwise failure
 */
int set_ble_name(const char *name)
{
    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > 29)
    {
        LOGE("Invalid name length: %zu", name_len);
        return BLE_ERR_NAME_INVALID_LEN;
    }

    char command[40] = {0};
    int len = snprintf(command, sizeof(command), "AT+NAME=%s\r", name);
    if (len < 0 || len >= sizeof(command))
    {
        LOGE("Failed to set name: encoding error");
        return BLE_ERR_SET_NAME_ENCODE;
    }

    int status = bt_uart_transmit((uint8_t *)command, len, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to set name, transmit status = %d", status);
        return BLE_ERR_SET_NAME_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_SET_NAME_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for set name response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_SET_NAME_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    if (!(s_ble_name.len == 2 && s_ble_name.buf[0] == 'O' && s_ble_name.buf[1] == 'K'))
    {
        LOGE("Response not OK");
        return BLE_ERR_RESP_BAD;
    }
    return BLE_OK;
}

static int retry_get_name(char *name)
{
    if (name == NULL)
    {
        LOGE("Error: name pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    ULONG actual_flags;
    tx_event_flags_get(&ble_event_flags, BLE_GET_NAME_EVENT, TX_OR_CLEAR, &actual_flags, TX_NO_WAIT);

    if (bt_uart_reset() != HAL_OK)
    {
        LOGE("UART reset failed");
        return BLE_ERR_UART_TX;
    }
    tx_thread_sleep(500);

    const char *command = "AT+NAME?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Retry to get name failed, transmit status = %d", status);
        return BLE_ERR_GET_NAME_TX_FAIL;
    }

    status = tx_event_flags_get(&ble_event_flags, BLE_GET_NAME_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get name response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_NAME_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }

    size_t len = (g_ble_name.len >= BLE_NAME_MAX_LEN) ? BLE_NAME_MAX_LEN - 1 : g_ble_name.len;
    memcpy(name, g_ble_name.buf, len);
    name[len] = '\0';
    LOGV("Successfully got ble name: %s", name);
    return BLE_OK;
}

/**
 * @brief Get the name of the Bluetooth chip
 * @param name - Bluetooth chip name
 * @retval 0 success, otherwise failure
 */
int get_ble_name(char *name)
{
    if (name == NULL) {
        LOGE("Error: name pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+NAME?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get name, transmit status = %d", status);
        return BLE_ERR_GET_NAME_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_NAME_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status == TX_SUCCESS && (actual_flags & BLE_GET_NAME_EVENT))
    {
        size_t len = (g_ble_name.len >= BLE_NAME_MAX_LEN) ? BLE_NAME_MAX_LEN - 1 : g_ble_name.len;
        memcpy(name, g_ble_name.buf, len);
        name[len] = '\0';
        LOGV("Successfully got ble name: %s", name);
        return BLE_OK;
    }
    else
    {
        LOGW("Failed to get name, retrying to get BLE name.");
        return retry_get_name(name);
    }
}

bool set_ble_addr(const char *addr_data, uint8_t addr_len)
{
    if (addr_len != 12)
    {
        LOGE("Set BLE address failed due to invalid length");
        return false;
    }

    char command[30] = {0};
    int len = snprintf(command, sizeof(command), "AT+LBDADDR=%s\r", addr_data);
    if (len < 0 || len >= sizeof(command))
    {
        LOGE("Failed to set addr: encoding error");
        return false;
    }

    int status = bt_uart_transmit((uint8_t *)command, len, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("BLE address set transmit failed, status = %d", status);
        return false;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_SET_ADDR_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for set addr response.");
        return false;
    }
    if (!(actual_flags & BLE_SET_ADDR_EVENT))
    {
        LOGE("Wrong event flag received");
        return false;
    }
    if (!(s_ble_addr.len == 2 && s_ble_addr.buf[0] == 'O' && s_ble_addr.buf[1] == 'K'))
    {
        LOGE("Response not OK");
        return false;
    }
    return true;
}

void parse_bluetooth_address(char *mac, const uint8_t *input, uint8_t length)
{
    int i, j = 0;
    for (i = 0; i < length; i += 2)
    {
        mac[j++] = input[i];
        mac[j++] = input[i + 1];
        mac[j++] = ':';
    }
    mac[j - 1] = '\0';
}

static int retry_get_addr(char *addr)
{
    if (addr == NULL)
    {
        LOGE("Error: addr pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    ULONG actual_flags;
    tx_event_flags_get(&ble_event_flags, BLE_GET_ADDR_EVENT, TX_OR_CLEAR, &actual_flags, TX_NO_WAIT);

    if (bt_uart_reset() != HAL_OK)
    {
        LOGE("UART reset failed");
        return BLE_ERR_UART_TX;
    }
    tx_thread_sleep(500);

    const char *command = "AT+LBDADDR?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGV("Failed to get addr, transmit status =  %d", status);
        return BLE_ERR_GET_ADDR_TX_FAIL;
    }

    status = tx_event_flags_get(&ble_event_flags, BLE_GET_ADDR_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get addr response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_ADDR_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    if (g_ble_addr.len != 12) {
        LOGE("Invalid address length: %d", g_ble_addr.len);
        return BLE_ERR_GET_ADDR_RX_LEN_INVALID;
    }
    parse_bluetooth_address(addr, g_ble_addr.buf, g_ble_addr.len);
    LOGV("Successfully got ble addr: %s", addr);
    return BLE_OK;
}

/**
 * @brief Obtain the address of the Bluetooth chip
 * @param addr - Bluetooth chip address
 * @retval 0 success, otherwise failure
 */
int get_ble_addr(char *addr)
{
    if (addr == NULL)
    {
        LOGE("Error: addr pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+LBDADDR?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get addr, transmit status = %d", status);
        return BLE_ERR_GET_ADDR_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_ADDR_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status == TX_SUCCESS && (actual_flags & BLE_GET_ADDR_EVENT) && (g_ble_addr.len==12))
    {
        parse_bluetooth_address(addr, g_ble_addr.buf, g_ble_addr.len);
        LOGV("Successfully got ble addr: %s", addr);
        return BLE_OK;
    }
    else
    {
        LOGW("Failed to get addr, retrying to get ble addr.");
        return retry_get_addr(addr);
    }
}

/**
 * @brief Obtain the version number of the Bluetooth chip
 * @param version - Bluetooth chip name
 * @retval 0 success, otherwise failure
 */
int get_ble_version(char *version)
{
    if (version == NULL)
    {
        LOGE("Error: version pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+GVER\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble version, transmit status = %d", status);
        return BLE_ERR_GET_BLE_VERSION_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_GVER_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble version response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_GVER_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(version, g_ble_gver.buf, g_ble_gver.len);
    version[g_ble_gver.len]='\0';
    LOGV("Successfully got ble version: %s", version);
    return BLE_OK;
}

/**
 * @brief Get the Firmware Revision of the Bluetooth chip
 * @param firmware_name - Bluetooth chip Firmware Revision
 * @retval 0 success, otherwise failure
 */
int get_ble_firmware_name(char *firmware_name)
{
    if (firmware_name == NULL)
    {
        LOGE("Error: firmware_name pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+GCFGVER?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble firmware name, transmit status = %d", status);
        return BLE_ERR_GET_BLE_FIRMWARE_NAME_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_GCFGVER_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble firmware name response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_GCFGVER_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(firmware_name, g_ble_gcfgver.buf, g_ble_gcfgver.len);
    firmware_name[g_ble_gcfgver.len]='\0';
    LOGV("Successfully got ble firmware name: %s", firmware_name);
    return BLE_OK;
}

/**
 * @brief Get the Manufacturer of the Bluetooth chip
 * @param manufacturer_name - Bluetooth chip Manufacturer Number
 * @retval 0 success, otherwise failure
 */
int get_ble_manufacturer_name(char *manufacturer_name)
{
    if (manufacturer_name == NULL)
    {
        LOGE("Error: manufacturer_name pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+MANUFACTURER?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble manufacturer name, transmit status = %d", status);
        return BLE_ERR_GET_BLE_MANUFACTURER_NAME_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_MANUFACTURER_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble manufacturer name response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_MANUFACTURER_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(manufacturer_name, g_ble_manufacturer.buf, g_ble_manufacturer.len);
    manufacturer_name[g_ble_manufacturer.len]='\0';
    LOGV("Successfully got ble manufacturer name: %s", manufacturer_name);
    return BLE_OK;
}

/**
 * @brief Get the Model Number of the Bluetooth chip
 * @param model_name - Bluetooth chip Model Number
 * @retval 0 success, otherwise failure
 */
int get_ble_model_name(char *model_name)
{
    if (model_name == NULL)
    {
        LOGE("Error: model_name pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+MODEL?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble model name, transmit status = %d", status);
        return BLE_ERR_GET_BLE_MODEL_NAME_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_MODEL_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble model name response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_MODEL_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(model_name, g_ble_model.buf, g_ble_model.len);
    model_name[g_ble_model.len]='\0';
    LOGV("Successfully got ble model name: %s", model_name);
    return BLE_OK;
}

/**
 * @brief Get the baud of the Bluetooth chip
 * @param baud - Bluetooth chip baud
 * @retval 0 success, otherwise failure
 */
int get_ble_baud(char *baud)
{
    if (baud == NULL)
    {
        LOGE("Error: baud pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+BAUD?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble baud, transmit status = %d", status);
        return BLE_ERR_GET_BLE_BAUD_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_BAUD_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble baud response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_BAUD_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(baud, g_ble_baud.buf, g_ble_baud.len);
    baud[g_ble_baud.len]='\0';
    LOGV("Successfully got ble baud: %s", baud);
    return BLE_OK;
}

/**
 * @brief Set whether the Bluetooth chip sends broadcasts
 * @param adv_status - 0 Do not send broadcasts, 1 Send broadcast
 * @retval 0 success, otherwise failure
 */
int set_ble_adv_status(uint8_t adv_status)
{
    if (adv_status != 0 && adv_status != 1) {
        LOGE("Invalid adv_status: %d", adv_status);
        return BLE_ERR_INVALID_PARAM;
    }

    char command[12] = {0};
    int len = snprintf(command, sizeof(command), "AT+ADV=%d\r", adv_status);
    if (len < 0 || len >= sizeof(command))
    {
        LOGE("Failed to set adv status: encoding error");
        return BLE_ERR_ADV_STATUS_ENCODE;
    }

    int status = bt_uart_transmit((uint8_t *)command, len, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to set ble adv status, transmit status = %d", status);
        return BLE_ERR_SET_BLE_ADV_STATUS_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_SET_ADV_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for set ble adv status response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_SET_ADV_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    if (!(s_ble_adv.len == 2 && s_ble_adv.buf[0] == 'O' && s_ble_adv.buf[1] == 'K'))
    {
        LOGE("Response not OK");
        return BLE_ERR_RESP_BAD;
    }
    return BLE_OK;
}

/**
 * @brief Get whether the Bluetooth chip sends a broadcast
 * @param adv_status - 0 no broadcast, 1 broadcast
 * @retval 0 success, otherwise failure
 */
int get_ble_adv_status(char *adv_status)
{
    if (adv_status == NULL)
    {
        LOGE("Error: adv_status pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+ADV?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble adv status, transmit status = %d", status);
        return BLE_ERR_GET_BLE_ADV_STATUS_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_ADV_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble adv status response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_ADV_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    if (g_ble_adv.len != 1 || (g_ble_adv.buf[0] != '0' && g_ble_adv.buf[0] != '1')) {
        LOGE("Unexpected adv status response: %.*s", g_ble_adv.len, g_ble_adv.buf);
        return BLE_ERR_RESP_BAD;
    }
    memcpy(adv_status, g_ble_adv.buf, g_ble_adv.len);
    adv_status[g_ble_adv.len]='\0';
    LOGV("Successfully got ble adv status: %s", adv_status);
    return BLE_OK;
}

/**
 * @brief Set the broadcast interval for Bluetooth chips
 * @param adv_interval - Bluetooth chips broadcast interval
 * @retval 0 success, otherwise failure
 */
int set_ble_adv_param(uint16_t adv_interval)
{
    char command[20] = {0};
    int len = snprintf(command, sizeof(command), "AT+ADVPARAM=%lu\r", (unsigned long)adv_interval);
    if (len < 0 || len >= sizeof(command))
    {
        LOGE("Failed to set ble adv param: encoding error");
        return BLE_ERR_SET_ADV_PARAM_ENCODE;
    }

    int status = bt_uart_transmit((uint8_t *)command, len, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to set ble adv param, transmit status = %d", status);
        return BLE_ERR_SET_BLE_ADV_PARAM_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_SET_ADV_PARAM_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for set ble adv param response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_SET_ADV_PARAM_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    if (!(s_ble_adv_param.len == 2 && s_ble_adv_param.buf[0] == 'O' && s_ble_adv_param.buf[1] == 'K')) {
        LOGE("Response not OK");
        return BLE_ERR_RESP_BAD;
    }
    return BLE_OK;
}

/**
 * @brief Get the broadcast interval for Bluetooth chips
 * @param adv_interval - Bluetooth chips broadcast interval
 * @retval 0 success, otherwise failure
 */
int get_ble_adv_param(char *adv_interval)
{
    if (adv_interval == NULL)
    {
        LOGE("Error: adv_interval pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+ADVPARAM?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble adv param, transmit status = %d", status);
        return BLE_ERR_GET_BLE_ADV_PARAM_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_ADV_PARAM_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble adv param response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_ADV_PARAM_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(adv_interval, g_ble_adv_param.buf, g_ble_adv_param.len);
    adv_interval[g_ble_adv_param.len]='\0';
    LOGV("Successfully got ble adv param: %s", adv_interval);
    return BLE_OK;
}

/**
 * @brief Get the broadcast parameters of the Bluetooth chip
 * @param adv_data -  Bluetooth chip broadcast parameters
 * @retval 0 success, otherwise failure
 */
int set_ble_adv_data(const uint8_t *data, uint8_t length)
{
    if (data == NULL) {
        LOGE("Error: data pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }
    if (length == 0 || length > 67) {
        LOGE("Invalid data length: %d", length);
        return BLE_ERR_INVALID_PARAM;
    }

    uint8_t command[80] = {0};
    command[0] = 0x41;
    command[1] = 0x54;
    command[2] = 0x2b;
    command[3] = 0x41;
    command[4] = 0x44;
    command[5] = 0x56;
    command[6] = 0x44;
    command[7] = 0x41;
    command[8] = 0x54;
    command[9] = 0x41;
    command[10] = 0x3d;

    memcpy(&command[11], data, length);

    command[length + 11] = 0x0d;
    command[length + 12] = 0x0a;
    int status = bt_uart_transmit((uint8_t *)command, length + 13, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to set ble adv data, transmit status = %d", status);
        return BLE_ERR_SET_BLE_ADV_DATA_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_SET_ADV_DATA_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for set ble adv data response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_SET_ADV_DATA_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    if (!(s_ble_adv_data.len == 2 && s_ble_adv_data.buf[0] == 'O' && s_ble_adv_data.buf[1] == 'K')) {
        LOGE("Response not OK");
        return BLE_ERR_RESP_BAD;
    }
    return BLE_OK;
}

/**
 * @brief Obtain the broadcast parameters of the Bluetooth chip
 * @param adv_data - Bluetooth chip broadcast parameters
 * @retval 0 success, otherwise failure
 */
int get_ble_adv_data(char *adv_data)
{
    if (adv_data == NULL)
    {
        LOGE("Error: adv_data pointer is NULL");
        return BLE_ERR_INVALID_PARAM;
    }

    const char *command = "AT+ADVDATA?\r";
    int status = bt_uart_transmit((uint8_t *)command, strlen(command), HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        LOGE("Failed to get ble adv data, transmit status = %d", status);
        return BLE_ERR_GET_BLE_ADV_DATA_TX_FAIL;
    }

    ULONG actual_flags;
    status = tx_event_flags_get(&ble_event_flags, BLE_GET_ADV_DATA_EVENT, TX_OR_CLEAR, &actual_flags, BLE_CMD_TIMEOUT_MS);
    if (status != TX_SUCCESS)
    {
        LOGE("Timeout waiting for get ble adv data response.");
        return BLE_ERR_TIMEOUT;
    }
    if (!(actual_flags & BLE_GET_ADV_DATA_EVENT))
    {
        LOGE("Wrong event flag received");
        return BLE_ERR_EVENT_GET;
    }
    memcpy(adv_data, g_ble_adv_data.buf, g_ble_adv_data.len);
    adv_data[g_ble_adv_data.len]='\0';
    LOGV("Successfully got ble adv data: %s", adv_data);
    return BLE_OK;
}

void set_ble_enabled(bool enabled)
{
    is_bluetooth_enabled = enabled;
}

/**
 * @brief Is ble enabled
 * @param None
 * @retval true-enable false-disable
 */
bool is_ble_enabled()
{
    return is_bluetooth_enabled;
}

void set_ble_connected(bool connected)
{
    is_bluetooth_connected = connected;
}

/**
 * @brief Is ble connected
 * @param None
 * @retval true-connect false-disconnect
 */
bool is_ble_connected()
{
    return is_bluetooth_connected;
}

void update_name_with_addr()
{
    char ble_address[18] = {0};
    int status = get_ble_addr(ble_address);
    LOGV("update_name_with_addr get_ble_addr status = %d", status);

    if (status == BLE_OK)
    {
        LOGV("update_name_with_addr ble_address = %s", ble_address);
        char name[32];
        unsigned int byte1, byte2;

        if (sscanf(ble_address + 12, "%2x:%2x", &byte1, &byte2) == 2)
        {
            sprintf(name, "CBKEY%02X%02X", byte1, byte2);
            LOGV("set ble name name = %s", name);
            int result = set_ble_name(name);
            LOGV("update_name_with_addr set_ble_name result = %d", result);
            if (result == BLE_OK)
            {
                update_ble_adv_data(name);
            }
        }
        else
        {
            LOGE("Failed to parse BLE address");
        }
    }
}

bool update_ble_adv_data(const char *name)
{
    // Advertising data to be broadcasted, in little-endian format
    uint8_t general_discoverable[6] = {0x30, 0x32, 0x30, 0x31, 0x30, 0x36};
    uint8_t ble_uuid[16] = {0x30, 0x37, 0x30, 0x33, 0x46, 0x44, 0x46, 0x46, 0x62, 0x33, 0x66, 0x64, 0x30, 0x41, 0x31, 0x38};
    uint8_t adv_data[62] = {0};
    int index = 0;

    memcpy(&adv_data, general_discoverable, 6);
    index += 6;

    int name_len = strlen(name);
    if (name_len > 18) {
        name_len = 18;
    }
    uint8_t name_field_length = 1 + name_len;
    LOGV("set_ble_adv_and_scan_data name_field_length = %d", name_field_length);

    char hex_byte[3];
    sprintf(hex_byte, "%02X", name_field_length);
    adv_data[index++] = hex_byte[0];
    adv_data[index++] = hex_byte[1];
    adv_data[index++] = 0x30;
    adv_data[index++] = 0x38;

    for (int i = 0; i < name_len; i++) {
        sprintf(hex_byte, "%02X", name[i]);
        adv_data[index++] = hex_byte[0];
        adv_data[index++] = hex_byte[1];
    }

    memcpy(&adv_data[index], ble_uuid, 16);
    index += 16;
    LOGV("###adv_data_length = %d", index);

    // Set the advertising data using the function
    int result = set_ble_adv_data(adv_data, index);
    if (result != BLE_OK)
    {
        LOGE("Failed to broadcast advertising data");
    }
    else
    {
        LOGV("Broadcast advertising data successfully");
    }
    return result==BLE_OK;
}

bool update_name(const char *name)
{
    LOGV("update_name name = %s", name);
    int result = set_ble_name(name);
    if (result != BLE_OK)
    {
        return false;
    }

    result = disable_bluetooth();
    if (!result)
    {
        LOGE("disable bluetooth fail.");
        return false;
    }

    result = enable_bluetooth();
    if (!result)
    {
        LOGE("enable bluetooth fail.");
        return false;
    }

    return true;
}
