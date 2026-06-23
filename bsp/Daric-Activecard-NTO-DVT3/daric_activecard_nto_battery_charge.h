/**
******************************************************************************
* @file    daric_activecard_nto_battery_charge.h
* @author  PERIPHERIAL BSP Team
* @brief   Header file of battery and charge logic layer.
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

#ifndef _DARIC_ACTIVECARD_NTO_BATTERY_CHARGE_H_
#define _DARIC_ACTIVECARD_NTO_BATTERY_CHARGE_H_

#include "tg28.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum BSP_BATT_IRQ_STATUS {
    BSP_BATT_IRQ_STATUS_NONE = 0,
    BSP_BATT_IRQ_STATUS_ACIN_OVERVOLTAGE,
    BSP_BATT_IRQ_STATUS_ACIN_CONNECT,
    BSP_BATT_IRQ_STATUS_ACIN_REMOVED,
    BSP_BATT_IRQ_STATUS_VBUS_OVERVOLTAGE,
    BSP_BATT_IRQ_STATUS_VBUS_CONNECT,
    BSP_BATT_IRQ_STATUS_VBUS_REMOVED,
    BSP_BATT_IRQ_STATUS_VBUS_LESS_VHOLD,
    BSP_BATT_IRQ_STATUS_BATT_CONNECT,
    BSP_BATT_IRQ_STATUS_BATT_REMOVED,
    BSP_BATT_IRQ_STATUS_BATT_ACTIVATE_MODE,
    BSP_BATT_IRQ_STATUS_BATT_EXIT_ACTIVATE_MODE,
    BSP_BATT_IRQ_STATUS_BATT_CHARGING,
    BSP_BATT_IRQ_STATUS_BATT_CHARGED,
    BSP_BATT_IRQ_STATUS_BATT_TEMP_HIGH,
    BSP_BATT_IRQ_STATUS_BATT_TEMP_LOW,
    BSP_BATT_IRQ_STATUS_INTERNAL_TEMP_HIGH,
    BSP_BATT_IRQ_STATUS_PEK_SHORT_PRESS,
    BSP_BATT_IRQ_STATUS_PEK_LONG_PRESS,
    BSP_BATT_IRQ_STATUS_BATT_THRED1,
    BSP_BATT_IRQ_STATUS_BATT_THRED2,
    BSP_BATT_IRQ_STATUS_TIMER_TIMEOUT,
    BSP_BATT_IRQ_STATUS_PEK_RISING,
    BSP_BATT_IRQ_STATUS_PEK_FALLING,
    BSP_BATT_IRQ_STATUS_GPIO1_EDGE,
    BSP_BATT_IRQ_STATUS_GPIO0_EDGE,
} BSP_BATT_IRQ_STATUS;

typedef struct {
    void (*Init)(void);                                           // init
    void (*SetChargeConstantCurrent)(TG28_CHARG_CON_C_T current); // set charge constant current
    void (*SetChargeConstantVoltage)(TG28_CHARG_CON_V_T voltage); // set charge constant voltage
    void (*SetChargePrechgCurrent)(TG28_CHARG_PRE_C_T current);   // set pre-charge current
    void (*SetChargeTermCurrent)(TG28_CHARG_TER_C_T current);     // set terminal current
    int (*ChargeEnable)(bool enable);                             // enable charge
    int (*GetChargeStatus)(uint8_t *);                            // get charge status
    int (*GetBattCapacity)(uint8_t *);                            // get battery capacity
    int (*GetInterruptStatus)(void);                              // get irq status
    int (*ClearInterruptStatus)(TG28_IRQ_T status);               // clear irq status
    int (*SoftReset)(void);                                       // soft reset
    int (*PowerOff)(void);                                        // power off
    void (*IntEnable)(TG28_IRQ_T status,
                      bool enable); // enable or disable interrupt
    int (*PEKAutoPowerOffConfig)(uint8_t enable, uint8_t poweroff_time,
                                 uint8_t poweron_time); // enable or disable the auto power-off feature
} BATT_CHG_FUN_T;

/**define a funtion battery charge capacity and charge status callback */
typedef uint8_t (*BattChgCapStsCallback_t)(int32_t, int32_t);

/**
 * @brief define a funtion battery charge interrupt callback.
 * @param status interrupt status. refer to BSP_BATT_IRQ_STATUS.
 */
typedef void (*BattChgIntCallback_t)(uint32_t status);

/**
 * @brief battery charge capacity and charge status register for UI.
 *        if UI need to get battery capacity and charge status, it should
 * register callback. e.g. uint8_t UI_Get_Bat_Chg_Cap_Sts(int32_t cap, int32_t
 * sts) { printf("UI_Get_Bat_Chg_Cap_Sts,cap=%d%%,sts=%d\n",cap,sts); return 0;
 *        }
 *        BSP_Batt_Chg_CapacityChargeStatus_Callback_Register(UI_Get_Bat_Chg_Cap_Sts);
 *        cap: 0---100
 *        sts: 0---discharging, 1---charging, 2---full
 * @param  none
 * @retval BSP status.
 */
int32_t BSP_BATT_CHG_CapacityChargeStatus_Callback_Register(BattChgCapStsCallback_t callback);

int32_t BSP_BATT_Interrupt_Callback_Register(BattChgIntCallback_t callback);
void BSP_BATT_PowerOff(void);
void BSP_BATT_SoftReset(void);

/**
 * @brief charge status.
 * @param none
 * @retval 0:not charge, 1:charging, 2:full.
 */
int32_t BSP_BATT_CHG_ChargeStatus(void);

/**
 * @brief battery capacity.
 * @param none
 * @retval battery capacity value 0--100.
 */
int32_t BSP_BATT_CHG_Capacity(void);
/**
 * @brief battery charge entry.
 *        This function is used to do battery and charge init and enable charge
 *        create a timer "batt_chg_timer" to monitor the battery capacity and
 * charge status.
 * @param  none
 * @retval none.
 */
void BSP_BATT_CHG_Entry(void);

#endif
