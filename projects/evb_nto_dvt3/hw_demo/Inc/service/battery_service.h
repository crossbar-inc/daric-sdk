/**
 *****************************************************************************
 * @file    battery_service.h
 * @author  AP Team
 * @brief   Header file of battery_service.
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
#include <stdint.h>
#include <stdio.h>
#include "daric_hal.h"
#include "daric_pm.h"
#include "tx_api.h"

#ifdef CONFIG_BOARD_EVB_NTO
#include "daric_evb_nto_vibrator.h"
#endif
#if defined (CONFIG_BOARD_ACTIVECARD_NTO) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT2) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT3)
#include "daric_activecard_nto_vibrator.h"
#endif
#ifdef CONFIG_BOARD_EVB2
#include "daric_evb2_vibrator.h"
#endif


#if defined(CONFIG_BOARD_EVB2)
#include "daric_evb2_battery_charge.h"
#elif defined(CONFIG_BOARD_ACTIVECARD_NTO) || defined(CONFIG_BOARD_ACTIVECARD_NTO_DVT2) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT3)
#include "daric_activecard_nto_battery_charge.h"

/* define a funtion battery charge capacity callback */
// typedef uint8_t (*batter_capacity_callback)(int32_t, int32_t);

// int32_t register_battery_capacity_callback(batter_capacity_callback callback);
int32_t init_battery_service();
int32_t get_battery_capacity(void);
int32_t get_battery_charge_status(void);
void reset(void);
void power_off(void);
#endif

int32_t vibrator_init(void);
int32_t vibrator_short(void);
int32_t vibrator_long(void);

#define MESSAGE_TYPE_BATT_VOLTAGE 4

#define BATTERY_STATUS_NOT_CHARGE  0
#define BATTERY_STATUS_CHARGING    1
#define BATTERY_STATUS_CHARGE_FULL 2
#define POWER_KEY_SHORT_PRESS      0
#define TP_POWER_KEY_SHORT_PRESS   2
#define POWER_KEY_LONG_PRESS       1

