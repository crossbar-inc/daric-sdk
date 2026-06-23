/**
 ******************************************************************************
 * @file    daric_pm.h
 * @author  PERIPHERIAL BSP Team
 * @brief   This file contains the apis to power manager module
 ******************************************************************************
 * @attention
 *
 * © Copyright CrossBar, Inc. 2024.
 * All rights reserved.
 *
 * All rights reserved.
 *
 * This software is the proprietary property of CrossBar, Inc. and is protected
 * by copyright laws. Any unauthorized reproduction, distribution, or
 * modification is strictly prohibited.
 *
 ******************************************************************************
 */

#ifndef _DARIC_PM_H_
#define _DARIC_PM_H_

#include "tg28.h"

/**
 * @brief  Initialize the power manager module
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int BSP_PM_init(void);

/**
 * @brief  Reset the power manager module, which will cause the deveice repower
 * on.
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int BSP_PM_reset(void);

/**
 * @brief  Turn off the power manager module, which will cause the deveice power
 * down.
 * @retval BSP_ERROR_NONE if successfuly, failed else.
 */
int BSP_PM_poweroff(void);

/**
 * @brief  Disconnect the battery. The device will enter ship mode.
 */
void BSP_PM_enterShipMode(void);

/**
 * @brief The power channels supported.
 */
typedef enum {
    PM_PWR_DCDC1   = TG28_CH_DCDC1,   // 3.3V
    PM_PWR_DCDC2   = TG28_CH_DCDC2,   // 0.85V
    PM_PWR_DCDC3   = TG28_CH_DCDC3,   // 0.85V
    PM_PWR_DCDC4   = TG28_CH_DCDC4,   // 1.8V
    PM_PWR_DCDC5   = TG28_CH_DCDC5,   // 3.6V
    PM_PWR_ALDO1   = TG28_CH_ALDO1,   // 3.0V
    PM_PWR_ALDO2   = TG28_CH_ALDO2,   // 2.5V
    PM_PWR_ALDO3   = TG28_CH_ALDO3,   // 3.1V
    PM_PWR_ALDO4   = TG28_CH_ALDO4,   // 0.9V
    PM_PWR_BLDO1   = TG28_CH_BLDO1,   // 3.3V
    PM_PWR_BLDO2   = TG28_CH_BLDO2,   // 3.3V
    PM_PWR_CPUSLDO = TG28_CH_CPUSLDO, // 0.9V
    PM_PWR_DLDO1   = TG28_CH_DLDO1,   // 3.3V Same to DCDC1
    PM_PWR_DLDO2   = TG28_CH_DLDO2,   // 1.8V Same to DCDC4
    PM_PWR_OFF_DISCHARGE = TG28_CH_OFF_DISCHARGE,
    PM_PWR_BATFET = TG28_CH_BATFET,
} BSP_PM_PWR_E;

/**
 * @brief  Enable or disable the power output.
 * @param  ch the selected channel. Should be one of BSP_PM_PWR_E.
 * @param  en enable power output when true, disable otherwise.
 */
void BSP_PM_PWR_en(uint8_t ch, bool en);

/**
 * @brief  Set the channel voltage and enable the channel.
 * @param  ch the selected channel. Should be one of BSP_PM_PWR_E.
 * @param  mv the voltagbe(micro v) setted.
 */
void BSP_PM_PWR_set(uint8_t ch, uint16_t mv);

/**
 * @brief  Get the battery capacity
 * @retval The batter capacity. unit percentage
 */
uint8_t BSP_PM_BAT_getCapacity();

typedef enum {
    BSP_PM_CHARG_ST_NONE,   // Doesn't charging
    BSP_PM_CHARG_ST_CHGING, // Charging
    BSP_PM_CHARG_ST_DONE,   // Charge completed
} BSP_PM_CHARG_ST;

/**
 * @brief  Get the charging status
 * @retval charge status
 */
BSP_PM_CHARG_ST BSP_PM_BAT_getChargStatus();

#define BATTERY_POWEROFF_CHARGING_VOLTAGE 3400
/**
 * @brief  Get the battery voltage
 * @retval get battery voltage result: BSP_ERROR_NONE or BSP_ERROR_COMPONENT_FAILURE
 */
int BSP_PM_getVbatVoltage(uint16_t *mv);

/**
 * @brief Checks whether the Power Management (PM) has been initialized.
 * @return true  The Power Management (PM) subsystem is fully initialized and ready for use.
 * @return false The Power Management (PM) subsystem is **not** initialized.
 */
bool BSP_PM_inited();

typedef enum {
    BSP_PM_EVT_BAT_INSERT      = BIT(1),
    BSP_PM_EVT_BAT_REMOVE      = BIT(2),
    BSP_PM_EVT_BAT_CHARGE      = BIT(3), // Battery charge status changed
    BSP_PM_EVT_BAT_CAPACITY    = BIT(4), // Battery capacity changed
    BSP_PM_EVT_BAT_DAMAGED     = BIT(5), // Battery is damaged
    BSP_PM_EVT_BAT_HOT_TEMP    = BIT(6), // Charging exception hot temprature
    BSP_PM_EVT_BAT_COLD_TEMP   = BIT(7), // Charging exception cold temprature
    BSP_PM_EVT_PEK_SHORT_PRESS = BIT(8),
    BSP_PM_EVT_PEK_LONG_PRESS  = BIT(9),
} BSP_PM_EVT_E;

/**
 * @brief The callback that will be called when the registered event happened.
 */
typedef void (*BSP_PM_EvntListener)(BSP_PM_EVT_E event, uint32_t param);

/**
 * @brief  Register the battery event listener. Must un-register before
 * re-register.
 * @param  listener, callback
 * @param  events, registered events. OR combination of @BSP_PM_EVT_E
 */
void BSP_PM_registerEventListener(BSP_PM_EvntListener listener,
                                  uint32_t            events);
/**
 * @brief  Un-register the battery event listener
 * @param  listener, callback
 */
void BSP_PM_unRegisterEventListener(BSP_PM_EvntListener listener);
#endif //_DARIC_PM_H_