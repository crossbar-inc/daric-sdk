/**
 ******************************************************************************
 * @file    battery_service.c
 * @author  AP Team
 * @brief   This file mainly defines some battery interfaces, 
 *          including handle the battery interrupt callback.
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
#include "battery_service.h"
#include "daric_errno.h"
#include "daric_pm.h"
#include "tx_log.h"
#include "common.h"
#include "factory_mode.h"
#undef LOG_TAG
#define LOG_TAG "BATTERY_SERV"

static bool isVibInit = false;
#define VIBRATOR_LOGN_DURATION 3000//3s

int32_t g_capacity = 0;
int32_t get_battery_capacity(void);

void bsp_pm_event_listener(BSP_PM_EVT_E event, uint32_t param) {
    LOGV("bsp_pm_event_listener enter status = %ld", event);
    switch (event) {
    case BSP_PM_EVT_BAT_INSERT:
    case BSP_PM_EVT_BAT_REMOVE:
        break;
    case BSP_PM_EVT_BAT_CHARGE:
        LOGV("bsp_pm_event_listener, battery charge=%ld", param);
        if (param == BSP_PM_CHARG_ST_NONE) {
            if (current_screen == (GX_WIDGET*)&factory_mode_charger_win)
            {
                factory_mode_update_test_status(FACTORY_MODE_ITME_CHARGER_TEST, BSP_PM_CHARG_ST_NONE, 0);
                return;
            }
        } else if (param == BSP_PM_CHARG_ST_CHGING) {
            if (current_screen == (GX_WIDGET*)&factory_mode_charger_win)
            {
                factory_mode_update_test_status(FACTORY_MODE_ITME_CHARGER_TEST, BSP_PM_CHARG_ST_CHGING, 0);
                return;
            }
        } else if (param == BSP_PM_CHARG_ST_DONE) {
            if (current_screen == (GX_WIDGET*)&factory_mode_charger_win)
            {
                factory_mode_update_test_status(FACTORY_MODE_ITME_CHARGER_TEST, BATTERY_STATUS_CHARGE_FULL, 0);
                return;
            }
        }
        break;
    case BSP_PM_EVT_BAT_CAPACITY:
        LOGV("bsp_pm_event_listener, battery capacity=%ld", param);
        g_capacity = param;
        if (current_screen == (GX_WIDGET*)&factory_mode_charger_win)
        {
            factory_mode_update_test_status(FACTORY_MODE_ITME_CHARGER_TEST, BSP_PM_EVT_BAT_CAPACITY, param);
        }
        break;
    case BSP_PM_EVT_BAT_DAMAGED:
    case BSP_PM_EVT_BAT_HOT_TEMP:
    case BSP_PM_EVT_BAT_COLD_TEMP:
        break;
    case BSP_PM_EVT_PEK_SHORT_PRESS:
        LOGV("bsp_pm_event_listener powerkey short press");
        if (current_screen == (GX_WIDGET*)&factory_mode_key_win)
        {
            factory_mode_update_test_status(FACTORY_MODE_ITME_KEY_TEST, BSP_PM_EVT_PEK_SHORT_PRESS, 0);
            return;
        }
        break;
    case BSP_PM_EVT_PEK_LONG_PRESS:
        LOGV("bsp_pm_event_listener powerkey long press");
        if (current_screen == (GX_WIDGET*)&factory_mode_key_win)
        {
            factory_mode_update_test_status(FACTORY_MODE_ITME_KEY_TEST, BSP_PM_EVT_PEK_LONG_PRESS, 0);
            return;
        }
        break;

    default:
        break;
    }
}

/*! \brief init battery service, register the interupt call back for battrey.
 * 
 * @param none
 * @retval int32_t.
 */
int32_t init_battery_service(){
    int32_t result = BSP_ERROR_NONE;

    uint32_t events = BSP_PM_EVT_BAT_CHARGE
                    | BSP_PM_EVT_BAT_CAPACITY
                    | BSP_PM_EVT_PEK_SHORT_PRESS
                    | BSP_PM_EVT_PEK_LONG_PRESS;
                    // | BSP_PM_EVT_BAT_LVL;
    BSP_PM_registerEventListener(bsp_pm_event_listener, events);
    g_capacity = get_battery_capacity();
    if (result == BSP_ERROR_NONE){
        LOGV("init_battery_service success");
        result = vibrator_init();
    }else{
        LOGV("init_battery_service fail, result: %ld", result);
    }
    // HAL_Register_BS_SendPWKShortPressEvent(bsp_pm_send_short_powerkey_event);
    // #ifdef CONFIG_SHOW_PD_WINDOW
    // HAL_Register_BS_SendUpdatePDModeWindowEvent(bsp_pm_send_update_pd_window_event);
    // #endif
    return result;
}

/*! \brief battery capacity.
 * 
 * @param none
 * @retval int32_t battery capacity value normal:0--100, -1:error.
 */
int32_t get_battery_capacity(void){
    // return BSP_BATT_CHG_Capacity();
    return BSP_PM_BAT_getCapacity();
}

/*! \brief charge status.
 * 
 * @param none
 * @retval int32_t 0:not charge, 1:charging, 2:full, -1:error.
 */
int32_t get_battery_charge_status(void){
    // return BSP_BATT_CHG_ChargeStatus();
    BSP_PM_CHARG_ST status = BSP_PM_BAT_getChargStatus();
    int32_t st = BATTERY_STATUS_NOT_CHARGE;
    switch (status) {
    case BSP_PM_CHARG_ST_NONE:
        st = BATTERY_STATUS_NOT_CHARGE;
        break;
    case BSP_PM_CHARG_ST_CHGING:
        st = BATTERY_STATUS_CHARGING;
        break;
    case BSP_PM_CHARG_ST_DONE:
        st = BATTERY_STATUS_CHARGE_FULL;
        break;
    }
    return st;
}

/*! \brief PMIC soft reset..
 * 
 * @param none
 * @retval none.
 */
void reset(void){
    vibrator_short();
    // BSP_BATT_SoftReset();
    BSP_PM_reset();
}

/*! \brief PMIC Power off. It would shutdown all power output except VCC-RTC.
 * 
 * @param none
 * @retval none.
 */
void power_off(void){
    vibrator_short();
    // BSP_BATT_PowerOff();
    BSP_PM_poweroff();
}

int32_t vibrator_init(void) {
    uint32_t initRst = BSP_Vibrator_Init();
    if (!isVibInit) {
        if (initRst != BSP_ERROR_NONE) {
            LOGI("Vibrator init failed!\n");
        }
    }
    return initRst;
}

int32_t vibrator_short()
{
    return BSP_Vibrator_Short();
}

int32_t vibrator_long()
{
   return BSP_Vibrator_Long(VIBRATOR_LOGN_DURATION);
}
