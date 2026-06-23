/**
 ******************************************************************************
 * @file    daric_pm.c
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
#define LOG_LEVEL LOG_LEVEL_D

#include <stdint.h>

#include <tx_api.h>

#include "daric_errno.h"
#include "daric_hal_gpio.h"
#include "daric_log.h"

#include "eta4662.h"
#include "tg28.h"

#include "daric_pm.h"
#include "daric_hal.h"
#include "tx_low_power_user.h"

#define SIZE_ALIAGNMENT(type, n) ((sizeof(type) + n - 1) / n)

#define STACK_SIZE               2048
#define MAX_MSG                  8
#define TIMER_FIRST_TIME         (CONFIG_SYS_CLOCK_TICKS_PER_SEC)
// #define TIMER_REPEAT_CYCLE       (CONFIG_SYS_CLOCK_TICKS_PER_SEC * 43)
#define TIMER_REPEAT_CYCLE       (CONFIG_SYS_CLOCK_TICKS_PER_SEC * 3)
#define MAX_LISTENERS            2

typedef enum {
    MSG_TIMER,       // Timer message
    MSG_TG28_INT,    // TG28 Interrupt
    MSG_ETA4662_INT, // ETA4662 Interrupt
} MESSAGE;

typedef struct {
    MESSAGE  msg;
    uint32_t data;
} MSG_T;

/**
 * This enumeration indicate the different charging temperature zone.
 */
typedef enum {
    CHG_TZ01, // (--,  0], Cold tz, Disable charging
    CHG_TZ12, // ( 0, 10], Low tz, Enable charging
    CHG_TZ23, // (10, 18], Normal tz, Enable charging
    CHG_TZ34, // (18, 45], Warm tz, Enable charging
    CHG_TZ45, // (45, --), Hot tz, Disable charging
} CHG_TZ_E;

/**
 * This enumeration indicate the different charging status
 */
typedef enum {
    CHG_ST_NONE, // Doesn't charging
    CHG_ST_PRE,  // Pre-charging
    CHG_ST_CC,   // Const current charging
    CHG_ST_CV,   // Const voltage charging
    CHG_ST_CHG,  // Charging status. Some charge IC can't differ cc and cv.
    CHG_ST_DONE, // Charge completed
} CHG_ST_E;

/**
 * This enumeartion indicate the different chargeing parth
 */
typedef enum{
    CHG_PATH_NONE,
    CHG_PATH_ETA4662,
    CHG_PATH_TG28,
}CHG_PATH_E;

typedef struct {
    BSP_PM_EvntListener listener;
    uint32_t            event;
} LISTENER;

typedef struct {
    bool inited;
    bool threadRan;

    uint8_t   stack[STACK_SIZE];
    TX_THREAD tcb;

    uint8_t  msg[MAX_MSG * SIZE_ALIAGNMENT(MSG_T, 4)];
    TX_QUEUE msg_que;

    TX_TIMER timer;
    uint8_t  batt_capacity;
    CHG_TZ_E chg_tz;
    CHG_ST_E chg_st;
    CHG_PATH_E chg_path;

    LISTENER listeners[MAX_LISTENERS];
} PM_Ctx;

static void init_perpherial();
static void init_batteryCapacity();
static void init_charge();

static void timerCallback(ULONG arg);
static void thread_main(ULONG thread_input);

static BSP_PM_CHARG_ST convert_chargStatus(CHG_ST_E st);

static PM_Ctx ctx = {0};

bool g_is_sleep = false;
extern void bsp_pm_send_short_powerkey_event();
extern bool sleep_mode_is_screen_off();
extern void aw2023_led_off(void);
extern void aw2023_enter_lowpower();

extern void sleep_mode_registerCallbackListener(void (*listener)(bool));
void bsp_pm_sleep_mode_listener_callback(bool sleep)
{
    printf("%s, sleep mode: %d\r\n", __FUNCTION__, sleep);
    g_is_sleep = sleep;
    if (sleep)
    {
        tx_timer_deactivate(&ctx.timer);
    }
    else
    {
        tx_timer_change(&ctx.timer,
                TIMER_FIRST_TIME,
                TIMER_REPEAT_CYCLE);
        tx_timer_activate(&ctx.timer);
    }
}

int BSP_PM_init(void) {
    UINT status = TX_SUCCESS;

    if (ctx.inited) {
        LOGW("Has initialized before");
        return BSP_ERROR_NONE;
    }

    status = tx_queue_create(&ctx.msg_que, "power manaager queue",
                             SIZE_ALIAGNMENT(MSG_T, 4), &ctx.msg,
                             MAX_MSG * SIZE_ALIAGNMENT(MSG_T, 4));
    if (status != TX_SUCCESS) {
        LOGE("Create queue failed status=%d", status);
        return BSP_ERROR_UNKNOWN_FAILURE;
    } else {
        LOGI("MSG queue created");
    }

    status = tx_thread_create(&ctx.tcb, "power manager task", thread_main,
                              (ULONG)0, (VOID *)ctx.stack, STACK_SIZE, 14, 14,
                              10, TX_AUTO_START);
    if (status != TX_SUCCESS) {
        LOGE("Create thread failed status=%d", status);
        goto create_thread_failed;
    } else {
        LOGI("Thread created");
    }
    // status = tx_timer_create(&ctx.timer, "batt_capa_timer", timerCallback, (ULONG)0,
    //                 TIMER_FIRST_TIME, TIMER_REPEAT_CYCLE, TX_NO_ACTIVATE);
    // tx_timer_activate(&ctx.timer);
    // if (status != TX_SUCCESS) {
    //     LOGE("Create battery timer failed status=%d", status);
    //     goto timer_failed;
    // } else {
    //     LOGI("Battery timer created");
    // }
    // sleep_mode_registerCallbackListener(bsp_pm_sleep_mode_listener_callback);
    // init_perpherial();
    // init_batteryCapacity();
    // init_charge();
    ctx.inited = true;
    return BSP_ERROR_NONE;

// timer_failed:
    // TODO delete thread
create_thread_failed:
    // TODO delete queue

    return BSP_ERROR_UNKNOWN_FAILURE;
}

int BSP_PM_reset(void) {
    //BSP_PM_PWR_en(PM_PWR_DCDC4, false);
    //tx_thread_sleep(500);
    aw2023_led_off();
    aw2023_enter_lowpower();
    BSP_PM_PWR_en(PM_PWR_OFF_DISCHARGE, true);
    TG28_Soft_Reset();
    return BSP_ERROR_NONE;
}

int BSP_PM_poweroff(void) {
    aw2023_led_off();
    aw2023_enter_lowpower();
    switchDffV33AO(false);
    tx_thread_sleep(200);
    BSP_PM_PWR_en(PM_PWR_BATFET, false);
    tx_thread_sleep(200);
    TG28_Soft_Power_Off();
    return BSP_ERROR_NONE;
}

void BSP_PM_PWR_en(uint8_t ch, bool en) {
    if (en) {
        TG28_Ch_Power_On(ch);
    } else {
        TG28_Ch_Power_Off(ch);
    }
}

void BSP_PM_PWR_set(uint8_t ch, uint16_t mv) { TG28_Ch_Power_Set(ch, mv); }

uint8_t BSP_PM_BAT_getCapacity() { return ctx.batt_capacity; }

BSP_PM_CHARG_ST BSP_PM_BAT_getChargStatus() {
    return (convert_chargStatus(ctx.chg_st));
}

void BSP_PM_registerEventListener(BSP_PM_EvntListener listener,
                                  uint32_t            event) {
    LISTENER *p = NULL;

    if (listener == NULL) {
        LOGE("The listener is NULL");
        return;
    }

    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (ctx.listeners[i].listener == NULL) {
            p = &ctx.listeners[i];
            break;
        }
    }
    if (p == NULL) {
        LOGE("Listener is full. Max listener count is  %d", MAX_LISTENERS);
        return;
    }

    p->listener = listener;
    p->event    = event;
}

void BSP_PM_unRegisterEventListener(BSP_PM_EvntListener listener) {
    LISTENER *p = NULL;
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (ctx.listeners[i].listener == listener) {
            p = &ctx.listeners[i];
        }
    }
    if (p != NULL) {
        p->listener = NULL;
        p->event    = 0;
    } else {
        LOGW("The listener hasn't been registered");
    }
}

//-----------------------------------------------------------------------------
// The static function begin
//-----------------------------------------------------------------------------
#define TG28_INT_PORT CONFIG_PMIC_TG28_INT_PORT
#define TG28_INT_PIN  CONFIG_PMIC_TG28_INT_PIN

#define CHG_THRE_T1   2272 ///<  0 degrees ntc voltages
#define CHG_THRE_T2   1973 ///< 10 degrees ntc voltages
#define CHG_THRE_T3   1312 ///< 18 degrees ntc voltages
#define CHG_THRE_T4   492  ///< 45 degrees ntc voltages

#define CHG_RISE_D1   (-32) ///<  0 degrees rise hysteresis voltages
#define CHG_RISE_D2   (-32) ///< 10 degrees rise hysteresis voltages
#define CHG_RISE_D3   (-32) ///< 18 degrees rise hysteresis voltages
#define CHG_RISE_D4   (0)   ///< 45 degrees rise hysteresis voltages

#define CHG_DROP_D1   0 ///<  0 degrees drop hysteresis voltages
#define CHG_DROP_D2   4 ///< 10 degrees drop hysteresis voltages
#define CHG_DROP_D3   4 ///< 18 degrees drop hysteresis voltages
#define CHG_DROP_D4   4 ///< 45 degrees drop hysteresis voltages

#define CHG_RISE_T1   (CHG_THRE_T1 + CHG_RISE_D1)
#define CHG_RISE_T2   (CHG_THRE_T2 + CHG_RISE_D2)
#define CHG_RISE_T3   (CHG_THRE_T3 + CHG_RISE_D3)
#define CHG_RISE_T4   (CHG_THRE_T4 + CHG_RISE_D4)

#define CHG_DROP_T1   (CHG_THRE_T1 + CHG_DROP_D1)
#define CHG_DROP_T2   (CHG_THRE_T2 + CHG_DROP_D2)
#define CHG_DROP_T3   (CHG_THRE_T3 + CHG_DROP_D3)
#define CHG_DROP_T4   (CHG_THRE_T4 + CHG_DROP_D4)

// clang-format off
#define CHARG_PC   3    // Pre-charge current 0.01C, 3mA
// #define CHARG_TC   11   // Termination current 0.05C, 11mA
// #define CHARG_CC12 48   //  0~10 deg, Const current 0.2C, 48mA
// #define CHARG_CC23 112  // 10~18 deg, Const current 0.5C, 112mA
// #define CHARG_CC34 224  // 18~45 deg, Const current 1C, 224mA

// #define CHARG_PC   TG28_PRE_C_25    // Pre-charge current 0.01C, 3mA
#define CHARG_TC   TG28_TER_C_25    // Termination current 0.05C, 11mA
#define CHARG_CC12 TG28_CON_C_50    //  0~10 deg, Const current 0.2C, 48mA
#define CHARG_CC23 TG28_CON_C_125   // 10~18 deg, Const current 0.5C, 112mA
#define CHARG_CC34 TG28_CON_C_200   // 18~45 deg, Const current 1C, 224mA
// clang-format on

static void tg28_int_handle(void *UserData) {
    uint32_t irq = 0;
    do {
        irq |= TG28_readIrq();
        TG28_clearIrq(irq);
    } while (HAL_GPIO_ReadPin(TG28_INT_PORT, TG28_INT_PIN) == GPIO_PIN_RESET);
    MSG_T msg = {MSG_TG28_INT, irq};
    tx_queue_send(&ctx.msg_que, &msg, TX_NO_WAIT);
}

static void tg28_int_init() {
    GPIO_InitTypeDef init = {0};
    init.Pin              = TG28_INT_PIN;
    init.Mode             = GPIO_MODE_IT_LOW_LEVEL;
    init.Pull             = GPIO_PULLUP;
    init.IsrHandler       = tg28_int_handle;
    init.UserData         = NULL;
    HAL_GPIO_Init(TG28_INT_PORT, &init);
}

// clang-format off
static const uint8_t bat_param[] = {
    0x01,0xf5,0x40,0x00,0x1b,0x1e,0x28,0x0f,0x0c,0x1e,0x32,0x02,0x14,0x05,0x0a,0x04,
    0x74,0xfc,0x18,0x0d,0x43,0x10,0xfb,0xfb,0x50,0x01,0xea,0x03,0x1c,0x06,0xfb,0x06,
    0xd9,0x0b,0xa2,0x10,0x2f,0x0f,0xd1,0x0a,0x6d,0x0f,0x17,0x0f,0x0b,0x04,0xf8,0x04,
    0xe6,0x09,0xd6,0x0e,0xbf,0x0e,0xb5,0x09,0xa7,0x0e,0x90,0x0e,0x89,0x04,0x77,0x04,
    0x6c,0x09,0x68,0x0e,0x44,0x0d,0xdf,0x07,0xf9,0x66,0x34,0x2e,0x23,0x11,0x17,0x0b,
    0xc5,0x98,0x7e,0x66,0x4e,0x44,0x38,0x1a,0x12,0x0a,0xf6,0x00,0x00,0xf6,0x00,0xf6,
    0x00,0xfb,0x00,0x00,0xfb,0x00,0x00,0xfb,0x00,0x00,0xf6,0x00,0x00,0xf6,0x00,0xf6,
    0x00,0xfb,0x00,0x00,0xfb,0x00,0x00,0xfb,0x00,0x00,0xf6,0x00,0x00,0xf6,0x00,0xf6,
};

static const REG8_MAP tg28_regs[] = {
    // [3] (1), Guage Module: enable; 
    // [2] (0), Button battery charge: disable; 
    //*[1] (0), Cell battery charge: disable;
    // [0] (0), Watchdog module: disable;
    {0x18, 0x08}, 

    //*[5:4] (00), IRQLEVEL configuration: 1s; 
    //*[3:2] (11), OFFLEVEL configuration: 10s; 
    //*[1:0] (10), ONLevel configuration: 1s;
    {0x27, 0x0E}, 

    //*[4]   (0),  TS PIN function select: battery temperature;
    //*[3:2] (11), TS current source on/off enable: always on; 
    // [1:0] (10), TS current source: 50uA;
    {0x50, 0x0E}, 

    //*[7:0] (-), Set VLTFCHG 1344mV=n*32mV, 0 degree;
    {0x54, 0x2A}, 

    //*[7:0] (-), Set VHTFCHG 240mV=n*2mV, 45 degree;
    {0x55, 0x78}, 

    //*[3:0] (0001), Precharge current: 25mA;
    {0x61, 0x01},

    //*[4:0] (00010), Const current charge: 50mA;
    {0x62, 0x02}, 

    // [4]   (1),    Termination charging: enable; 
    //*[3:0] (0001), Termination current: 25mA;
    {0x63, 0x11}, 

    //*[2:0] (101), Charge voltage limit: 4.4V
    {0x64, 0x05}, 
};
// clang-format on

static void init_tg28() {
    TG28_I2C_init();
    // TG28_dumpRegister();
    TG28_initRegister(tg28_regs, ARR_SIZE(tg28_regs));
    //BSP_PM_PWR_en(PM_PWR_OFF_DISCHARGE, true);
    TG28_disableIrq(TG28_IRQ_MASK_ALL);
    uint32_t irq_mask = TG28_IRQ_MASK_BAT_OT_CHG | TG28_IRQ_MASK_BAT_UT_CHG |
                        TG28_IRQ_MASK_VBUS_INSER | TG28_IRQ_MASK_VBUS_RMV |
                        TG28_IRQ_MASK_BAT_INSER | TG28_IRQ_MASK_BAT_RMV |
                        TG28_IRQ_MASK_PO_SHORT | TG28_IRQ_MASK_PO_LONG | 
                        TG28_IRQ_MASK_WDG_TIME |
                        TG28_IRQ_MASK_CHG_DONE | TG28_IRQ_MASK_CHG_STAR;
    TG28_enableIrq(irq_mask);
    TG28_clearIrq(TG28_IRQ_MASK_ALL);
    TG28_InitBatteryParam(bat_param, ARR_SIZE(bat_param));
    tg28_int_init();
}

#define ETA4662_INT_PORT CONFIG_BATT_ETA4662_INT_PORT
#define ETA4662_INT_PIN  CONFIG_BATT_ETA4662_INT_PIN

static void eta4662_int_handle(void *UserData) {
    uint16_t f_st = 0;
    if (BSP_ERROR_NONE == ETA4662_getFaultStatus(&f_st)) {
        MSG_T msg = {MSG_ETA4662_INT, f_st};
        tx_queue_send(&ctx.msg_que, &msg, TX_NO_WAIT);
    }
}

static void eta4662_int_init() {
    GPIO_InitTypeDef init = {0};
    init.Pin              = ETA4662_INT_PIN;
    init.Mode             = GPIO_MODE_IT_FALLING;
    init.Pull             = GPIO_PULLUP;
    init.IsrHandler       = eta4662_int_handle;
    init.UserData         = NULL;
    HAL_GPIO_Init(ETA4662_INT_PORT, &init);
}

// clang-format off
const REG8_MAP eta4662_regs[] = {
    // [7:4] (1001), Vindpm 4.6V; 
    // [3:0] (1111), Iin_lim 500mA
    {0x00, 0x9F},

    // [7:6] (10),  Trst_dgl 16s; 
    // [5]   (1),   Trst_dur 4s; 
    // [4]   (0),   EN_HIZ enable; 
    // [3]   (1),   CEB: Charge disable; 
    //*[2:0] (111), Vbatt_uvlo 3.03v;
    {0x01, 0xAF},

    // [7]   (0),      REG_RST keep current setting; 
    // [6]   (0),      WD_RST normal; 
    //*[5:0] (000101), Fast charding current: 48mA;
    {0x02, 0x05},

    //*[7:4] (0000), Discharge current limit: 200mA; 
    // [3:0] (0001), Termination & Precondition current: 3mA;
    {0x03, 0x01},

    //*[7:2] (-), Battery termination voltage: 4.395V;
    // [1]   (1), Pre-charge to Fast charge threshold: 3V;
    // [0]   (1), Recharge threshold: 200mV;
    {0x04, 0xD7},

    // [7]   (0),  Watchdog control in discharge: disable; 
    //*[6:5] (00), Watchdog: disable; 
    // [4]   (1),  Charge termination: enable; 
    // [3]   (1),  Safely timer: enable; 
    // [2:1] (01), Fast charge timer: 5hrs; 
    // [0]   (0),  Termination timer: disable;
    {0x05, 0x1A},

    //*[7] (0), Battery thermal monitor: disable; 
    // [6] (1), Long charge timer: 2X extended safety timer;

    //*[5] (0), Battery FET: enable; 
    //*[4] (1), Powe good interrupt: mask; 
    //*[3] (1), Charge complete interrupt: mask; 
    // [2] (0), Charge status change interrupt: no mask; 
    //*[1] (1), NTC falut interrupt: mask; 
    //*[0] (1), Battery OVP interrupt: mask;
    {0x06, 0x5B},

    //*[7]   (1),    PCB over-temperature: disable;
    // [6]   (0),    VIN dpm loop: enable; 
    // [5:4] (11),   Juction thermal threshold: 120; 
    // [3:0] (0111), System voltage regulation: 4.55V;
    {0x07, 0xB7},

    // Read only
    //{0x08, 0x00},

    // [7:6]SHIP_DGL 1s. Do not use
    // {0x09, 0x00},
    
    // [7:5] (111), I2C address: 0E; 
    // [4] (0),     Control BATFET: not reset; 
    // [3] (0),     Switch mode: normal power path;
    // [2] (0),     VDD output voltage pin setting: enable to battery power;
    // [1] (0),     VIN over voltage lock out: enable; 
    // [0] (0),     Finer turn charge current: keep default Ichrg as Ichrg[5:0]
    {0x0A, 0xE0},
    
    // Read only
    // {0x0B, 0x00},
};
// clang-format on

static void init_eta4662() {
    ETA4662_I2C_init();
    // ETA4662_dumpRegister();
    ETA4662_initRegister(eta4662_regs, ARR_SIZE(eta4662_regs));
    eta4662_int_init();
}

static void init_perpherial() {
    init_tg28();
    init_eta4662();
}

#define CAPA_FILT_LEN 1 // Battery capacity filter length
static uint8_t s_capacity[CAPA_FILT_LEN] = {0};
static uint8_t s_capa_idx                = 0;

static uint8_t filter_batterCapacity() {
    uint16_t capa = 0;
    for (int i = 0; i < CAPA_FILT_LEN; i++) {
        capa += s_capacity[i];
    }
    return capa / CAPA_FILT_LEN;
}

static void init_batteryCapacity() {
    uint8_t capa     = 0;
    uint8_t try_cnt  = 0;
    int     read_cnt = 0;
    do {
        if (BSP_ERROR_NONE == TG28_BAT_CAP_Read(&capa)) {
            s_capacity[read_cnt++] = capa;
        } else {
            LOGW("Failed, try count: %d, read count: %d", try_cnt, read_cnt);
        }
        try_cnt++;
    } while ((read_cnt < CAPA_FILT_LEN) && try_cnt < 50);

    if (read_cnt < CAPA_FILT_LEN) {
        LOGW("Read capacity failed.");
    }

    s_capa_idx        = 0;
    ctx.batt_capacity = filter_batterCapacity();
}

static CHG_TZ_E get_chargeTempZone(uint16_t mv) {
    CHG_TZ_E range;
    if (mv > CHG_THRE_T1) {
        range = CHG_TZ01;
    } else if (mv > CHG_THRE_T2) {
        range = CHG_TZ12;
    } else if (mv > CHG_THRE_T3) {
        range = CHG_TZ23;
    } else if (mv > CHG_THRE_T4) {
        range = CHG_TZ34;
    } else {
        range = CHG_TZ45;
    }
    return range;
}

static CHG_TZ_E init_chargeTempZone() {
    CHG_TZ_E tz = CHG_TZ01;
    uint16_t mv = 0;
    if (BSP_ERROR_NONE != TG28_getTsVoltage(&mv)) {
        LOGW("Get temperature error!");
    } else {
        tz = get_chargeTempZone(mv);
    }
    return tz;
}

static CHG_TZ_E regulate_chargeTempZone(CHG_TZ_E pre_tz, uint16_t temp_vm) {
    CHG_TZ_E tz = pre_tz;
    switch (pre_tz) {
    case CHG_TZ01:
        if (temp_vm > CHG_RISE_T1) {
            tz = CHG_TZ01;
        } else {
            tz = get_chargeTempZone(temp_vm);
        }
        break;
    case CHG_TZ12:
        if (CHG_RISE_T2 < temp_vm && temp_vm < CHG_DROP_T1) {
            tz = CHG_TZ12;
        } else {
            tz = get_chargeTempZone(temp_vm);
        }
        break;
    case CHG_TZ23:
        if (CHG_RISE_T3 < temp_vm && temp_vm < CHG_DROP_T2) {
            tz = CHG_TZ23;
        } else {
            tz = get_chargeTempZone(temp_vm);
        }
        break;
    case CHG_TZ34:
        if (CHG_RISE_T4 < temp_vm && temp_vm < CHG_DROP_T3) {
            tz = CHG_TZ34;
        } else {
            tz = get_chargeTempZone(temp_vm);
        }
        break;
    case CHG_TZ45:
        if (temp_vm < CHG_DROP_T4) {
            tz = CHG_TZ45;
        } else {
            tz = get_chargeTempZone(temp_vm);
        }
        break;
    }
    return tz;
}

static const uint16_t PRE_CHG_THRE = 3000; // Pre-charge battery threshold 3000 mV
static CHG_ST_E pre_chargeStatus() {
    bool vBusGood = false;
    // bool batPresent = false;
    uint16_t batVoltage = 0;

    // Check vBus status
    int ret = TG28_getVbusGood(&vBusGood);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get vBus status error!");
        return CHG_ST_NONE;
    }
    if (!vBusGood) {
        LOGI("vBus isn't good");
        return CHG_ST_NONE;
    }

#if  0 // Battery always doesn't present when battery is lower than 2.9V
    // Check battery status
    ret = TG28_getVbatPresent(&batPresent);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get battery status error!");
        return CHG_ST_NONE;
    }
    if (!batPresent) {
        LOGI("Battery isn't present");
        return CHG_ST_NONE;
    }
#endif

    // Check battery voltage
    ret = TG28_getVbatVoltage(&batVoltage);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get battery voltage error!");
        return CHG_ST_NONE;
    }

    CHG_ST_E status = (batVoltage <= PRE_CHG_THRE) ? CHG_ST_PRE : CHG_ST_CHG;
    LOGI("vBus good: TRUE; battery present: TRUE; battery voltage: %d; charge status: %d", batVoltage, status);

    return status;
}

static void set_chargePath(CHG_ST_E status) {
    switch (status) {
    case CHG_ST_NONE:
        ETA4662_disableCharge();
        TG28_enableCharge(false);
        ctx.chg_path = CHG_PATH_NONE;
        break;
    case CHG_ST_PRE:
        TG28_enableCharge(false);
        ETA4662_disable_hiz();
        ETA4662_enableCharge();
        ETA4662_enable_hiz();
        ctx.chg_path = CHG_PATH_ETA4662;
        break;
    case CHG_ST_CC:
    case CHG_ST_CV:
    case CHG_ST_CHG:
    case CHG_ST_DONE:
        ETA4662_disableCharge();
        TG28_enableCharge(true);
        ctx.chg_path = CHG_PATH_TG28;
        break;
    }
}

static CHG_PATH_E chg_st2Path(CHG_ST_E status) {
    CHG_PATH_E path = CHG_PATH_NONE;
    switch (status) {
    case CHG_ST_NONE:
        path = CHG_PATH_NONE;
        break;
    case CHG_ST_PRE:
        path = CHG_PATH_ETA4662;
        break;
    case CHG_ST_CC:
    case CHG_ST_CV:
    case CHG_ST_CHG:
    case CHG_ST_DONE:
        path = CHG_PATH_TG28;
        break;
    }
    return path;
}

static CHG_ST_E get_chargeStatus_eta4662() {
    CHG_ST_E         status         = CHG_ST_NONE;
    ETA4662_CHG_ST_E eta4662_chg_st = ETA4662_CHG_ST_NONE;
    if (BSP_ERROR_NONE == ETA4662_getChargeSt(&eta4662_chg_st)) {
        switch (eta4662_chg_st) {
        case ETA4662_CHG_ST_NONE:
            status = CHG_ST_NONE;
            break;
        case ETA4662_CHG_ST_PRE:
            status = CHG_ST_PRE;
            break;
        case ETA4662_CHG_ST_CHG:
            status = CHG_ST_CHG;
            break;
        case ETA4662_CHG_ST_DONE:
            status = CHG_ST_DONE;
            break;
        }
            }
    return status;
}

static CHG_ST_E get_chargeStatus_tg28() {
    CHG_ST_E status = CHG_ST_NONE;
    TG28_CHG_ST tg28_status = TG28_getChargeStatus();
    switch (tg28_status) {
    case TG28_CHG_ST_NONE:
        status = CHG_ST_NONE;
        break;
    case TG28_CHG_ST_TRI:
    case TG28_CHG_ST_PRE:
        status = CHG_ST_PRE;
        break;
    case TG28_CHG_ST_CC:
        status = CHG_ST_CC;
        break;
    case TG28_CHG_ST_CV:
        status = CHG_ST_CV;
        break;
    case TG28_CHG_ST_DONE:
        status = CHG_ST_DONE;
        break;
    }
    return status;
}

static CHG_ST_E get_chargeStatus() {
    CHG_ST_E status = CHG_ST_NONE;
    switch (ctx.chg_path) {
    case CHG_PATH_NONE:
        status = CHG_ST_NONE;
        break;
    case CHG_PATH_ETA4662:
        status = get_chargeStatus_eta4662();
        break;
    case CHG_PATH_TG28:
        status = get_chargeStatus_tg28();
        break;
    }

    return status;
}

static BSP_PM_CHARG_ST convert_chargStatus(CHG_ST_E st) {
    BSP_PM_CHARG_ST status = BSP_PM_CHARG_ST_NONE;
    switch (st) {
    case CHG_ST_NONE:
        status = BSP_PM_CHARG_ST_NONE;
        break;
    case CHG_ST_PRE:
    case CHG_ST_CC:
    case CHG_ST_CV:
    case CHG_ST_CHG:
        status = BSP_PM_CHARG_ST_CHGING;
        break;
    case CHG_ST_DONE:
        status = BSP_PM_CHARG_ST_DONE;
        break;
    }
    return status;
}

static inline void set_charge_pc(CHG_TZ_E tz) {
    // TG28_SetChargePreCurrent(CHARG_PC);
    // ETA4662_setPreChargeCurrent(CHARG_PC);
}

static inline void set_charge_tc(CHG_TZ_E tz) {
    // TG28_SetChargeTerCurrent(CHARG_TC);
    // ETA4662_setTermCurrent(CHARG_TC);
}

static void set_charge_cc_tg28(CHG_TZ_E tz) {
    switch (tz) {
    case CHG_TZ01:
    case CHG_TZ12:
        TG28_SetChargeConCurrent(CHARG_CC12);
        // ETA4662_setConstCurrent(CHARG_CC12);
        break;
    case CHG_TZ23:
        TG28_SetChargeConCurrent(CHARG_CC23);
        // ETA4662_setConstCurrent(CHARG_CC23);
        break;
    case CHG_TZ34:
    case CHG_TZ45:
        TG28_SetChargeConCurrent(CHARG_CC34);
        // ETA4662_setConstCurrent(CHARG_CC34);
        break;
    }
}

static void set_charge_cc(CHG_TZ_E tz) {
    set_charge_cc_tg28(tz);
}

static void set_chargeCurrent(CHG_TZ_E tz, CHG_ST_E st) {
    switch (st) {
    case CHG_ST_NONE:
    case CHG_ST_PRE:
        set_charge_pc(tz);
        break;
    case CHG_ST_CC:
    case CHG_ST_CV:
    case CHG_ST_CHG:
    case CHG_ST_DONE:
        set_charge_tc(tz);
        break;
    }
    set_charge_cc(tz);
}

static void init_charge() {
    ctx.chg_tz = init_chargeTempZone();
    CHG_ST_E pre_st =  pre_chargeStatus();
    set_chargePath(pre_st);
    set_chargeCurrent(ctx.chg_tz, pre_st);
    ctx.chg_st = get_chargeStatus();
    LOGI("TZ: %d, ST: %d, PATH: %d", ctx.chg_tz, ctx.chg_st, ctx.chg_path);
}

static void timerCallback(ULONG arg) {
    MSG_T msg = {MSG_TIMER};
    tx_queue_send(&ctx.msg_que, &msg, TX_NO_WAIT);
}

#define NOTIFY_EVENT(e, param)                                                 \
    do {                                                                       \
        for (int i = 0; i < MAX_LISTENERS; i++) {                              \
            LISTENER *p = &ctx.listeners[i];                                   \
            if (p->listener != NULL && (p->event & e)) {                       \
                p->listener(e, param);                                         \
            }                                                                  \
        }                                                                      \
    } while (0)

static void timer_handle_chargeStatus() {
    uint16_t ts = 0;
    uint16_t bat = 0;
    CHG_PATH_E new_path, old_path = ctx.chg_path;
    CHG_TZ_E new_tz, old_tz = ctx.chg_tz;
    CHG_ST_E new_st, old_st = ctx.chg_st;
    int ret = BSP_ERROR_NONE;
    bool vBusGood = false;
    // bool batPresent = false;

    // Check vBus status
    ret = TG28_getVbusGood(&vBusGood);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get vBus status error!");
        return;
    }
    if (!vBusGood) {
        LOGD("vBus isn't good");
        return;
    }

#if  0 // Battery always doesn't present when lower than 2.9V
    // Check battery status
    ret = TG28_getVbatPresent(&batPresent);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get battery status error!");
        return;
    }
    if (!batPresent) {
        LOGD("Battery isn't present");
        return;
    }
#endif

    // Check ts temperature
    ret = TG28_getTsVoltage(&ts);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get ts temperature failed.");
        return;
    }

    ret = TG28_getVbatVoltage(&bat);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get battery voltage failed.");
    }

    new_tz = regulate_chargeTempZone(old_tz, ts);
    new_st = get_chargeStatus();
    new_path = chg_st2Path(new_st);
    if (new_path != old_path && new_path != CHG_PATH_NONE) {
        set_chargePath(new_st);
    }

    LOGD("Charge, TS: %d; BAT: %d, TEMP-ZONE: %d->%d; STATE: %d->%d, PATH: %d->%d", ts, bat, old_tz, new_tz, old_st, new_st, old_path, new_path);
    if (new_tz != old_tz || new_st != old_st) {
        LOGI("Charge, TS: %d; BAT: %d, TEMP-ZONE: %d->%d; STATE: %d->%d, PATH: %d->%d", ts, bat, old_tz, new_tz, old_st, new_st, old_path, new_path);
        set_chargeCurrent(new_tz, new_st);
        BSP_PM_CHARG_ST old_st_report = convert_chargStatus(old_st);
        BSP_PM_CHARG_ST new_st_report = convert_chargStatus(new_st);
        if (old_st_report != new_st_report) {
            NOTIFY_EVENT(BSP_PM_EVT_BAT_CHARGE, new_st_report);
        }
        ctx.chg_tz = new_tz;
        ctx.chg_st = new_st;
    }
}

static void timer_handle_battCapacity() {
    uint8_t new_capa;
    uint8_t fil_capa;
    uint16_t bat = 0;
    int ret = BSP_ERROR_NONE;

    ret = TG28_BAT_CAP_Read(&new_capa);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get capacity failed");
        return;
    }

    ret = TG28_getVbatVoltage(&bat);
    if (BSP_ERROR_NONE != ret) {
        LOGW("Get battery voltage failed.");
    }

    s_capacity[s_capa_idx++] = new_capa;
    s_capa_idx %= CAPA_FILT_LEN;
    fil_capa = filter_batterCapacity();
    LOGD("Capacity: bat %04d, pre %02d, new %02d, filter %02d", bat, ctx.batt_capacity, new_capa, fil_capa);
    if (fil_capa != ctx.batt_capacity) {
        LOGI("Capacity: bat %04d, pre %02d, new %02d, filter %02d", bat, ctx.batt_capacity, new_capa, fil_capa);
        ctx.batt_capacity = fil_capa;
        NOTIFY_EVENT(BSP_PM_EVT_BAT_CAPACITY, ctx.batt_capacity);
    }
}

static void timer_process() {
    timer_handle_chargeStatus();
    timer_handle_battCapacity();
}

static void irq_handle_chargeStatus() {
    CHG_ST_E old_st = ctx.chg_st;
    CHG_ST_E mid_st;
    CHG_ST_E new_st = old_st;
    
    tx_thread_sleep(20);    // Wait for the vBus and battery to stabilize
    mid_st = pre_chargeStatus();
    set_chargePath(mid_st);
    set_chargeCurrent(ctx.chg_tz, mid_st);
    new_st = get_chargeStatus();
    BSP_PM_CHARG_ST old_st_rp = convert_chargStatus(old_st);
    BSP_PM_CHARG_ST new_st_rp = convert_chargStatus(new_st);
    if (old_st_rp != new_st_rp) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_CHARGE, new_st_rp);
    }
    ctx.chg_st = new_st;

    LOGI("old_st=%d, pre_st=%d, new_st=%d", old_st, mid_st, new_st);
}

static void tg28_iqr(uint32_t irq) {
    LOGI("IRQ: 0x%06lX", irq);

    if (irq & TG28_IRQ_MASK_BAT_OT_CHG) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_HOT_TEMP, 0);
    }
    if (irq & TG28_IRQ_MASK_BAT_UT_CHG) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_COLD_TEMP, 0);
    }
    if (irq & TG28_IRQ_MASK_VBUS_INSER) {
        LOGI("vBus inserted");
        irq_handle_chargeStatus();
        if (sleep_mode_is_screen_off()) {
            #ifdef CONFIG_DARIC_GUIX
            bsp_pm_send_short_powerkey_event();
            #endif
        }
    }
    if (irq & TG28_IRQ_MASK_VBUS_RMV) {
        LOGI("vBus removed");
        irq_handle_chargeStatus();
        if (sleep_mode_is_screen_off()) {
            #ifdef CONFIG_DARIC_GUIX
            bsp_pm_send_short_powerkey_event();
            #endif
        }
    }
    if (irq & TG28_IRQ_MASK_BAT_INSER) {
        LOGI("Battery inserted");
        irq_handle_chargeStatus();
    }
    if (irq & TG28_IRQ_MASK_BAT_RMV) {
        LOGI("Battery removed");
        irq_handle_chargeStatus();
    }
    if (irq & TG28_IRQ_MASK_PO_SHORT) {
        NOTIFY_EVENT(BSP_PM_EVT_PEK_SHORT_PRESS, 0);
    }
    if (irq & TG28_IRQ_MASK_PO_LONG) {
        NOTIFY_EVENT(BSP_PM_EVT_PEK_LONG_PRESS, 0);
    }
    if (irq & TG28_IRQ_MASK_WDG_TIME) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_DAMAGED, 0);
    }
    if (irq & TG28_IRQ_MASK_CHG_DONE || irq & TG28_IRQ_MASK_CHG_STAR) {
        // Do nothing
    }
}

static void eta4662_iqr(uint32_t irq) {
    LOGI("IRQ: 0x%04lX", irq);

    if (irq & ETA4662_FAULT_WTD) {
    }
    if (irq & ETA4662_FAULT_PPM) {
    }
    if (irq & ETA4662_FAULT_PG) {
    }
    if (irq & ETA4662_FAULT_THERM) {
    }
    if (irq & ETA4662_FAULT_VIN) {
    }
    if (irq & ETA4662_FAULT_THERMSD) {
    }
    if (irq & ETA4662_FAULT_BATTOVP) {
    }
    if (irq & ETA4662_FAULT_CHGTO) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_DAMAGED, 0);
    }
    if (irq & ETA4662_FAULT_NTCHOT) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_HOT_TEMP, 0);
    }
    if (irq & ETA4662_FAULT_NTCCOLD) {
        NOTIFY_EVENT(BSP_PM_EVT_BAT_COLD_TEMP, 0);
    }
}

static void thread_main(ULONG thread_input) {
    UINT status = TX_SUCCESS;
    MSG_T message;
    LOGI("start pm thread");
    status = tx_timer_create(&ctx.timer, "batt_capa_timer", timerCallback, (ULONG)0,
                    TIMER_FIRST_TIME, TIMER_REPEAT_CYCLE, TX_NO_ACTIVATE);
    if (status != TX_SUCCESS) {
        LOGE("Create battery timer failed status=%d", status);
    } else {
        LOGI("Battery timer created");
    }
    init_perpherial();
    tx_thread_sleep(20); // Wait for TG28 to stabilize
    init_batteryCapacity();
    init_charge();
    ctx.threadRan = true;
    tx_timer_activate(&ctx.timer);
    sleep_mode_registerCallbackListener(bsp_pm_sleep_mode_listener_callback);

    while (1) {
        status = tx_queue_receive(&ctx.msg_que, &message, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS) {
            LOGE("Receive message failed. status:%d", status);
            continue;
        }
        // LOGI("Receive message, msg=%d", message.msg);
        switch (message.msg) {
        case MSG_TIMER:
            timer_process();
            break;
        case MSG_TG28_INT:
            tg28_iqr(message.data);
            break;
        case MSG_ETA4662_INT:
            eta4662_iqr(message.data);
            break;
        }
    }
}

/**
 * @brief  Get the battery voltage
 * @retval get battery voltage result: BSP_ERROR_NONE or BSP_ERROR_COMPONENT_FAILURE
 */
int BSP_PM_getVbatVoltage(uint16_t *mv)
{
    return TG28_getVbatVoltage(mv);
}

/**
 * @brief Checks whether the Power Management (PM) has been initialized.
 * @return true  The Power Management (PM) subsystem is fully initialized and ready for use.
 * @return false The Power Management (PM) subsystem is **not** initialized.
 */
bool BSP_PM_inited()
{
    return ctx.inited && ctx.threadRan;
}
