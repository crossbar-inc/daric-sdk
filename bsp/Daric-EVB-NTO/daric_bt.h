#ifndef __DARIC_BT_H
#define __DARIC_BT_H

#include "stdbool.h"
#include "stdint.h"
#include "daric_hal_def.h"

typedef void (*bt_uart_rx_cb_t)(const uint8_t *data, uint16_t len);

HAL_StatusTypeDef bt_uart_init(void);
void bt_uart_register_rx_callback(bt_uart_rx_cb_t cb);
bool bt_power_on();
bool bt_power_off();
HAL_StatusTypeDef bt_uart_transmit(const uint8_t *data, uint32_t len, uint32_t timeout_ms);
HAL_StatusTypeDef bt_uart_transmit_stop();
HAL_StatusTypeDef bt_uart_receive_it(void);
HAL_StatusTypeDef bt_uart_start_continuous_poll(void);
HAL_StatusTypeDef bt_uart_stop_continuous_poll(void);
int bt_uart_continuous_read(uint8_t *buf, uint32_t len, uint32_t timeout_ms);
HAL_StatusTypeDef bt_uart_reset();

#endif