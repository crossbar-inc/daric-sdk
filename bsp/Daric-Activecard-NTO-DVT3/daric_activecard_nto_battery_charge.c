/**
 ******************************************************************************
 * @file    daric_activecard_nto_battery_charge.c
 * @author  PERIPHERIAL BSP Team
 * @brief   file of battery and charge logic layer.
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
#include "daric_activecard_nto_battery_charge.h"
#include <daric_errno.h>
#include <daric_hal_gpio.h>
#include <daric_log.h>
#include <stdio.h>
#include <tx_api.h>

#ifndef CONFIG_BATT_CHG_TASK_STACK_SIZE
#define CONFIG_BATT_CHG_TASK_STACK_SIZE 2048
#endif
#define CONFIG_BATT_CHG_TIMER_FIRST_TIME (CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define CONFIG_BATT_CHG_TIMER_REPEAT_TIME (CONFIG_SYS_CLOCK_TICKS_PER_SEC * 30)

#define CONFIG_BATT_CHG_CONSTANT_CURRENT 300
#define CONFIG_BATT_CHG_PRE_CHG_CURRENT 10
#define CONFIG_BATT_CHG_TERM_CURRENT 10

#define BATT_CHG_INT_GPIO_PIN GPIO_PIN_0
#define BATT_CHG_INT_GPIO GPIOF

// #define CONFIG_BATT_CHG_UI_CALLBACK_TEST
//  #define CONFIG_BATT_IRQ_CALLBACK_TEST

static TX_TIMER BattChgTimer;
TX_THREAD thread_batt_int_handler;
TX_SEMAPHORE sem_batt_int_handler;
static char thread_batt_int_stack[CONFIG_BATT_CHG_TASK_STACK_SIZE];

BATT_CHG_FUN_T *Batt_Chg_Fun = NULL;
BattChgCapStsCallback_t Batt_Chg_Cap_Sts_Callback = NULL;
BattChgIntCallback_t Batt_Chg_Int_Callback = NULL;
static void Batt_Chg_TimerCallback();

static void TG28_Init(void) {
    TG28_I2C_init();
}

static BATT_CHG_FUN_T TG28_BAT_CHG_FUN = {
    .Init = TG28_Init,
    .SetChargeConstantCurrent = TG28_SetChargeConCurrent,
    .SetChargeConstantVoltage = TG28_SetChargeConVoltage,
    .SetChargePrechgCurrent = TG28_SetChargePreCurrent,
    .SetChargeTermCurrent = TG28_SetChargeTerCurrent,
    .ChargeEnable = TG28_SetChargeEnable,
    .GetChargeStatus = TG28_Charge_Status_Read,
    .GetBattCapacity = TG28_BAT_CAP_Read,
    .GetInterruptStatus = TG28_IRQ_Read,
    .ClearInterruptStatus = TG28_IRQ_Clear,
    .SoftReset = TG28_Soft_Reset,
    .PowerOff = TG28_Soft_Power_Off,
    .IntEnable = TG28_IRQ_Enable,
    .PEKAutoPowerOffConfig = TG28_POK_Config,
};

#ifdef CONFIG_BATT_CHG_UI_CALLBACK_TEST // for callback test
uint8_t UI_Get_Bat_Chg_Cap_Sts(uint8_t cap, uint8_t sts) {
    printf("UI_Get_Bat_Chg_Cap_Sts,cap=%d%%,sts=%d\n", cap, sts);
    return 0;
}
#endif

#ifdef CONFIG_BATT_IRQ_CALLBACK_TEST
static void short_press_irq_callback(void) {
    printf("short_press_irq_callback\n");
    Batt_Chg_Fun->SoftReset();
}

static void long_press_irq_callback(void) {
    printf("long_press_irq_callback\n");
    Batt_Chg_Fun->PowerOff();
}
#endif

static BSP_BATT_IRQ_STATUS BSP_BATT_Irq_status_map(TG28_IRQ_T status) {
    switch (status) {
    case TG28_IRQ_PO_SHORT:
        return BSP_BATT_IRQ_STATUS_PEK_SHORT_PRESS;
    case TG28_IRQ_PO_LONG:
        return BSP_BATT_IRQ_STATUS_PEK_LONG_PRESS;
    case TG28_IRQ_CHG_DONE:
        return BSP_BATT_IRQ_STATUS_BATT_CHARGED;
    case TG28_IRQ_CHG_STAR:
        return BSP_BATT_IRQ_STATUS_BATT_CHARGING;
    case TG28_IRQ_VBUS_RMV:
        return BSP_BATT_IRQ_STATUS_VBUS_REMOVED;
    default:
        return BSP_BATT_IRQ_STATUS_NONE;
    }
}

static void BATT_CHG_HAL_GPIO_EXTI_Callback(void *UserData) {
    // printf("BATT_CHG_HAL_GPIO_EXTI_Callback\n");
    tx_semaphore_put(&sem_batt_int_handler);
}

void thread_batt_int_handler_entry(ULONG thread_input) {
    int int_status;
    while (1) {
        tx_semaphore_get(&sem_batt_int_handler, TX_WAIT_FOREVER);
        printf("thread_batt_int_handler_entry\n");
        if (Batt_Chg_Fun == NULL) {
            // printf("Batt_Chg_Fun is NULL\n");
            continue;
        }
        do {
            int_status = Batt_Chg_Fun->GetInterruptStatus();
            printf("int_status = %d\n", int_status);
            if (int_status <= 0)
                continue;
            Batt_Chg_Fun->ClearInterruptStatus(int_status);

            if (Batt_Chg_Int_Callback) {
                Batt_Chg_Int_Callback(BSP_BATT_Irq_status_map(int_status));
            }

            if (int_status == TG28_IRQ_VBUS_RMV || int_status == TG28_IRQ_CHG_STAR || int_status == TG28_IRQ_CHG_DONE) {
                Batt_Chg_TimerCallback(0);
            }
        } while (HAL_GPIO_ReadPin(BATT_CHG_INT_GPIO, BATT_CHG_INT_GPIO_PIN) == GPIO_PIN_RESET);
    }
}

/**
 * @brief get battery and charge function table.
 * @param  none
 * @retval function table.
 */
BATT_CHG_FUN_T *Batt_Chg_Get_Fun(void) { return &TG28_BAT_CHG_FUN; }

/**
 * @brief battery charge init.
 * @param  none
 * @retval none
 */
static void Batt_Chg_Entry(void) {
    printf("Batt_Chgentry Entry\n");
    Batt_Chg_Fun = Batt_Chg_Get_Fun();
    if (Batt_Chg_Fun == NULL) {
        printf("Batt_Chg_Fun is NULL\n");
        return;
    }
    tx_semaphore_create(&sem_batt_int_handler, "sem_batt_int_handler", 0);
    tx_thread_create(&thread_batt_int_handler, "batt_int_handler", thread_batt_int_handler_entry, 0,
                     thread_batt_int_stack, CONFIG_BATT_CHG_TASK_STACK_SIZE, 1, 1, 4, TX_AUTO_START);
    if (Batt_Chg_Fun->Init) {
        Batt_Chg_Fun->Init();
    } else {
        printf("Batt_Chg_Fun->Init is NULL\n");
        return;
    }

    if (Batt_Chg_Fun->SetChargePrechgCurrent) {
      Batt_Chg_Fun->SetChargePrechgCurrent(TG28_PRE_C_25);
    } else {
      printf("Batt_Chg_Fun->SetChargePrechgCurrent is NULL\n");
    }
    if (Batt_Chg_Fun->SetChargeConstantCurrent) {
      Batt_Chg_Fun->SetChargeConstantCurrent(TG28_CON_C_100);
    } else {
      printf("Batt_Chg_Fun->SetChargeConstantCurrent is NULL\n");
    }
    if (Batt_Chg_Fun->SetChargeConstantVoltage) {
      Batt_Chg_Fun->SetChargeConstantVoltage(TG28_CON_V_4V4);
    } else {
      printf("Batt_Chg_Fun->SetChargeConstantVoltage is NULL\n");
    }

    if (Batt_Chg_Fun->SetChargeTermCurrent) {
      Batt_Chg_Fun->SetChargeTermCurrent(TG28_TER_C_25);
    } else {
      printf("Batt_Chg_Fun->SetChargeTermCurrent is NULL\n");
    }
    if (Batt_Chg_Fun->ChargeEnable) {
      Batt_Chg_Fun->ChargeEnable(1);
    } else {
      printf("Batt_Chg_Fun->ChargeEnable is NULL\n");
    }

    if (Batt_Chg_Fun->IntEnable) {
        Batt_Chg_Fun->IntEnable(TG28_IRQ_VBUS_RMV, 1);
    }

    if (Batt_Chg_Fun->PEKAutoPowerOffConfig)
        Batt_Chg_Fun->PEKAutoPowerOffConfig(1, TG28_POWEROFF_TIME_10S, TG28_POWERON_TIME_2S);

    // init interrupt gpio after init pmic
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BATT_CHG_INT_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.IsrHandler = BATT_CHG_HAL_GPIO_EXTI_Callback;
    GPIO_InitStruct.UserData = NULL;
    HAL_GPIO_Init(BATT_CHG_INT_GPIO, &GPIO_InitStruct);
    Batt_Chg_Fun->ClearInterruptStatus(TG28_IRQ_MAX);

// for callback test
#ifdef CONFIG_BATT_CHG_UI_CALLBACK_TEST
    BSP_BATT_CHG_CapacityChargeStatus_Callback_Register(UI_Get_Bat_Chg_Cap_Sts);
#endif

    printf("Batt_Chgentry Exit\n");
}

/**
 * @brief battery charge timer callback to get battery capacity and charge
 * status.
 * @param  arg
 * @retval none.
 */
static void Batt_Chg_TimerCallback(ULONG arg) {
    static int32_t batt_capacity0 = 0, chg_status0 = 0;
    int32_t batt_capacity = 0, chg_status = 0;

    batt_capacity = BSP_BATT_CHG_Capacity();
    printf("Batt_Chg_TimerCallback->batt_capacity = %ld\n", batt_capacity);

    chg_status = BSP_BATT_CHG_ChargeStatus();
    printf("Batt_Chg_TimerCallback->chg_status = %ld\n", chg_status);

    if (batt_capacity0 != batt_capacity || chg_status0 != chg_status) {
        batt_capacity0 = batt_capacity;
        chg_status0 = chg_status;
        if (Batt_Chg_Cap_Sts_Callback) {
            Batt_Chg_Cap_Sts_Callback(batt_capacity, chg_status);
        } else {
            printf("Batt_Chg_Cap_Sts_Callback is NULL\n");
        }
    }
}

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
int32_t BSP_BATT_CHG_CapacityChargeStatus_Callback_Register(BattChgCapStsCallback_t callback) {
    if (callback) {
        Batt_Chg_Cap_Sts_Callback = callback;
        return BSP_ERROR_NONE;
    } else {
        return BSP_ERROR_WRONG_PARAM;
    }
}

/**
 * @brief  Registers a callback function for the pmic interrupt.
 * @param  callback The callback function to be registered, which accepts status
 * as an argument.
 * @retval BSP status.
 */
int32_t BSP_BATT_Interrupt_Callback_Register(BattChgIntCallback_t callback) {
    if (callback) {
        Batt_Chg_Int_Callback = callback;
        return BSP_ERROR_NONE;
    } else {
        return BSP_ERROR_WRONG_PARAM;
    }
}

/**
 * @brief PMIC Power off. It would shutdown all power output except VCC-RTC.
 */
void BSP_BATT_PowerOff(void) {
    if (Batt_Chg_Fun && Batt_Chg_Fun->PowerOff) {
        Batt_Chg_Fun->PowerOff();
    } else {
        printf("Batt_Chg_Fun->PowerOff is NULL\n");
    }
}

/**
 * @brief PMIC soft reset.
 */
void BSP_BATT_SoftReset(void) {
    if (Batt_Chg_Fun && Batt_Chg_Fun->SoftReset) {
        Batt_Chg_Fun->SoftReset();
    } else {
        printf("Batt_Chg_Fun->SoftReset is NULL\n");
    }
}

static int charge_status_cv(uint8_t s) {
    switch (s) {
    case 0:     // tri_charge
    case 0b101: // not charging
        return 0;
    case 0b001: // precharge
    case 0b010: // constant current
    case 0b011: // constant voltage
        return 1;
    case 0b100: // full
        return 2;
    default:
        return -1; // error
    }
}
/**
 * @brief charge status.
 * @param none
 * @retval 0:not charge, 1:charging, 2:full.
 */
int32_t BSP_BATT_CHG_ChargeStatus(void) {
    uint8_t status = 0;
    if (Batt_Chg_Fun->GetChargeStatus && !Batt_Chg_Fun->GetChargeStatus(&status)) {
        LOGD("ChargeStatus = 0x%02X\n", status);
        return charge_status_cv(status);
    } else {
        printf("Batt_Chg_Fun->GetChargeStatus is NULL\n");
        return BSP_ERROR_NO_INIT;
    }
}

/**
 * @brief battery capacity.
 * @param none
 * @retval battery capacity value 0--100.
 */
int32_t BSP_BATT_CHG_Capacity(void) {
    uint8_t capacity = 0;
    if (Batt_Chg_Fun->GetBattCapacity) {
        if (Batt_Chg_Fun->GetBattCapacity(&capacity))
            LOGE("BSP_BATT_CHG_Capacity: GetBattCapacity failed\n");
        return capacity;
    } else {
        printf("Batt_Chg_Fun->GetBattCapacity is NULL\n");
        return BSP_ERROR_NO_INIT;
    }
}

/**
 * @brief battery charge entry.
 *        This function is used to do battery and charge init and enable charge
 *        create a timer "batt_chg_timer" to monitor the battery capacity and
 * charge status.
 * @param  none
 * @retval none.
 */
void BSP_BATT_CHG_Entry(void) {
    Batt_Chg_Entry();
    tx_timer_create(&BattChgTimer, "batt_chg_timer", Batt_Chg_TimerCallback, (ULONG)0, CONFIG_BATT_CHG_TIMER_FIRST_TIME,
                    CONFIG_BATT_CHG_TIMER_REPEAT_TIME, TX_AUTO_ACTIVATE);
}
