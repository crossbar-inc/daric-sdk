/**
******************************************************************************
* @file    daric_fingerprint.c
* @author  PERIPHERIAL BSP Team
* @brief   This file includes the driver for fingerprint module.
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

#include <stdbool.h>
#include <stdint.h>

#include <tx_api.h>
#include <tx_low_power_user.h>

#include "daric_errno.h"
#include "daric_fingerprint.h"
#include "daric_hal_gpio.h"

#include "mlb_error_code.h"
#include "mlb_intf.h"
#include "mlb_platform.h"

#define LOG_LEVEL LOG_LEVEL_D
// #define LOG_STACK
#include "daric_log.h"

#define FP_MAX_USER_UID         MLB_USER_MAX_NUM     // Max users.
#define FP_MAX_USER_FID         MLB_MAX_FID_PER_USER // Max fingerprints every user.

#define FP_MAX_VIP              5
#define FP_MAX_ENROLL_SAMPLE    MLB_MAX_ENROLL_STEP // Max steps when enroll
#define FP_ENROLL_DUPAREA_TH    80
#define FP_ENROLL_DUPAREA_START 2
#define FP_TPL_SIZE             (12 * 1024)

#define STACK_SIZE              10240 // Task stack size
#define FP_MSG_MAX              8     // Message queue size

typedef enum : uint16_t {
    TA_E_ENROLL_START,
    TA_E_ENROLL_CANCEL,
    TA_E_DETECT_START,
    TA_E_DETECT_CONTINUE,
    TA_E_DETECT_STOP,
    TA_E_NAVI_START,
    TA_E_NAVI_STOP,
    TA_E_REFRESH_TO,
    TA_E_TOUCH_INT,
} TASK_ACTION;

#define TO_STR(item) #item

// clang-format off
static const char *act_str[] = {
    [TA_E_ENROLL_START]    = TO_STR(ENROLL_START),
    [TA_E_ENROLL_CANCEL]   = TO_STR(ENROLL_CANCEL),
    [TA_E_DETECT_START]    = TO_STR(DETECT_START),
    [TA_E_DETECT_CONTINUE] = TO_STR(DETECT_CONTINUE),
    [TA_E_DETECT_STOP]     = TO_STR(DETECT_STOP),
    [TA_E_NAVI_START]      = TO_STR(NAVI_START),
    [TA_E_NAVI_STOP]       = TO_STR(NAVI_STOP),
    [TA_E_REFRESH_TO]      = TO_STR(REFRESH_TO),
    [TA_E_TOUCH_INT]       = TO_STR(TOUCH_INT),
};
// clang-format on

typedef struct {
    TASK_ACTION action;
    uint16_t    fid;
} MSG_T;

typedef enum {
    FSM_STA_IDLE,   // Idle state
    FSM_STA_ENROLL, // Enroll
    FSM_STA_DETECT, // Detection
    FSM_STA_NAVI,   // Navigation
} FSM_STA;

static const char *fsm_sta[] = {
    [FSM_STA_IDLE]   = TO_STR(IDLE),
    [FSM_STA_ENROLL] = TO_STR(ENROLL),
    [FSM_STA_DETECT] = TO_STR(DETECT),
    [FSM_STA_NAVI]   = TO_STR(NAVI),
};

typedef enum {
    FSM_EVT_ENROLL_START, // Start enroll
    FSM_EVT_ENROLL_CANCE, // Cancel enroll
    FSM_EVT_ENROLL_FINSH, // Enroll finished
    FSM_EVT_DETECT_START, // Start detect
    FSM_EVT_DETECT_STOP,  // Stop detect
    FSM_EVT_DETECT_FINSH, // Detect finished
    FSM_EVT_NAVI_START,   // Start navigation
    FSM_EVT_NAVI_STOP,    // Stop navigation
} FSM_EVT;

// clang-format off
static const char *fsm_evt[] = {
    [FSM_EVT_ENROLL_START] = TO_STR(ENROLL_START),
    [FSM_EVT_ENROLL_CANCE] = TO_STR(ENROLL_CANCE),
    [FSM_EVT_ENROLL_FINSH] = TO_STR(ENROLL_FINSH),
    [FSM_EVT_DETECT_START] = TO_STR(DETECT_START),
    [FSM_EVT_DETECT_STOP]  = TO_STR(DETECT_STOP),
    [FSM_EVT_DETECT_FINSH] = TO_STR(DETECT_FINSH),
    [FSM_EVT_NAVI_START]   = TO_STR(NAVI_START),
    [FSM_EVT_NAVI_STOP]    = TO_STR(NAVI_STOP),
};
// clang-format on

static void fingerprint_main(ULONG arg);
static void send_msg(TASK_ACTION action, uint16_t fid);
static void irq_handler(void *data);
static UINT low_power_sleep_hook(void *param);
static UINT low_power_wakeup_hook(void *param);

#define INT_PORT CONFIG_FP_INT_PORT
#define INT_PIN  CONFIG_FP_INT_PIN

static GPIO_PinState read_int_pin() { return HAL_GPIO_ReadPin(INT_PORT, INT_PIN); }

static void interrup_init() {
    GPIO_InitTypeDef init = {.Pin        = INT_PIN,
                             .Mode       = GPIO_MODE_IT_RISING,
                             .Pull       = GPIO_NOPULL,
                             .IsrHandler = irq_handler};
    HAL_GPIO_Init(INT_PORT, &init);
    HAL_GPIO_EnableIT(INT_PORT, INT_PIN, false);
}
static void interrupt_enable(bool enable) {
    static bool pre_st = false;
    if (pre_st == enable) {
        return;
    } else {
        pre_st = enable;
        HAL_GPIO_EnableIT(INT_PORT, INT_PIN, enable);
    }
}

#ifdef CONFIG_FP_PW_CTRL
#define PW_PORT CONFIG_FP_PW_PORT
#define PW_PIN  CONFIG_FP_PW_PIN
static void power_init() {
    GPIO_InitTypeDef init = {.Pin = PW_PIN, .Mode = GPIO_MODE_OUTPUT, .Pull = GPIO_NOPULL};
    HAL_GPIO_Init(PW_PORT, &init);
}
static void power_enable(bool enable) {
    HAL_GPIO_WritePin(PW_PORT, PW_PIN, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
#endif

typedef struct {
    bool     inited;
    bool     thread_started;
    FSM_STA  state;
    bool     enrolling;
    uint8_t  enroll_update_times;
    uint16_t enroll_fid;
    bool     detecting;
    bool     detect_continue;
    bool     navigation;

    uint16_t user_fids[FP_MAX_USER_FID];
    uint8_t  user_fid_count;

    BSP_FP_LISENTER_T listener;

    uint8_t   msg_buf[FP_MSG_MAX * sizeof(MSG_T)];
    TX_QUEUE  msg_que;
    TX_TIMER  timer;
    TX_THREAD tcb;
    char      stack[STACK_SIZE];
} Context;

static Context ctx = {0};
#define CHECK_MODULE_INITED                                                                        \
    do {                                                                                           \
        if (!ctx.inited) {                                                                         \
            LOGE("Has't inited");                                                                  \
            return BSP_ERROR_NO_INIT;                                                              \
        }                                                                                          \
    } while (0)

int16_t BSP_FP_Init(uint16_t uid) {
    mlb_img_info_t   img_info = {0};
    mlb_sdk_info_t   sdk_info = {0};
    MLB_CHIPID       chipid   = 0;
    mlb_init_param_t param    = {0};
    MRT_T            ret      = MRT_OK;

    if (ctx.inited) {
        LOGI("Has inited before");
        return BSP_ERROR_NONE;
    }

// Turn on fingerprint sensor power
#ifdef CONFIG_FP_PW_CTRL
    power_init();
    power_enable(true);
#endif

    ret = mlb_intf_platform_init();
    if (ret != MRT_OK) {
        LOGE("Platform init failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("Platform inited");

    ret = mlb_intf_get_sdk_info(&sdk_info);
    if (ret != MRT_OK) {
        LOGE("Get sdk info failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("SDK ver: %s, com-id: %s", sdk_info.version, sdk_info.commit_id);

    ret = mlb_intf_sensor_read_chipid(&chipid);
    if (ret != MRT_OK) {
        LOGE("Read chipid failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("chipid: 0x%05X", chipid);
    if (chipid != MLB_CHIPID_E088N) {
        LOGE("Unknow chip");
        return FP_ERR_UNKNOW_CHIP;
    }

    mlb_sensor_reg_e088n();
    param.sensor_type          = MLB_SENSOR_TYPE_COATING;
    param.fp_tpl_size          = FP_TPL_SIZE;
    param.max_fp_number        = FP_MAX_USER_UID * FP_MAX_USER_FID;
    param.max_enroll_steps     = FP_MAX_ENROLL_SAMPLE;
    param.max_vip_cnt          = FP_MAX_VIP;
    param.duplicate_area_th    = FP_ENROLL_DUPAREA_TH;
    param.duplicate_area_start = FP_ENROLL_DUPAREA_START;
    param.algo_ram_level       = 1;
    param.algo_time_level      = 1;
    param.algo_far_leval       = 1;
    param.algo_run_mode        = 0;
#if LOG_LEVEL > LOG_LEVEL_D
    param.dbglvl = 1;
#else
    param.dbglvl = 2;
#endif
    ret = mlb_intf_init(&param);
    if (ret != MRT_OK) {
        LOGE("Interface init failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("Interface inited");

    ret = mlb_intf_get_image_info(&img_info);
    if (ret != MRT_OK) {
        LOGE("Get image info failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("img-inf, w: %d, h: %d, img-bit: %d, org-img-bit: %d", img_info.img_width,
         img_info.img_height, img_info.img_bit, img_info.org_img_bit);

    ret = mlb_intf_set_active_user(uid);
    if (ret != MRT_OK) {
        LOGE("Set uid failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("Setted uid: %d", uid);

    ret = mlb_intf_get_current_user_fids(ctx.user_fids, &ctx.user_fid_count);
    // TODO list current user fids

    LOGI("Create fingerprint thread");
    if (!ctx.thread_started) {
        UINT status;
        status = tx_thread_create(&ctx.tcb, "fingerprint_main", fingerprint_main, (ULONG)0,
                                  (VOID *)ctx.stack, STACK_SIZE, 14, 14, 10, TX_AUTO_START);
        LOGI("Creat thread: %s", status == TX_SUCCESS ? "succeed" : "failed");
    }

    // mlb_intf_power_down_mode();

    interrup_init();
    interrupt_enable(false);

    low_power_register_hook(0, low_power_sleep_hook, NULL, low_power_wakeup_hook, NULL);

    ctx.inited = true;
    // LOGV("-------------------- before Calibration, int-pin: %d", read_int_pin());
    // BSP_FP_Calibrate();
    // LOGV("-------------------- after Calibration, int-pin: %d", read_int_pin());
    // BSP_FP_Reset();
    // LOGV("-------------------- after reset, int-pin: %d", read_int_pin());
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_DeInit() {
    MRT_T ret;

    if (!ctx.inited) {
        LOGI("Has't inited");
        return BSP_ERROR_NONE;
    }

    ret = mlb_intf_deinit();
    if (ret != MRT_OK) {
        LOGE("Interface deinit failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }

    LOGI("interface deinited");
    ctx.inited = false;
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_Enter_LowPower() {
    MRT_T ret = mlb_intf_power_down_mode();
    if (ret != MRT_OK) {
        LOGE("failed, retl=%d", ret);
        return BSP_ERROR_PERIPH_FAILURE;
    }
    LOGI("Succeed");
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_Exit_LowPower() { return BSP_ERROR_NONE; }

int16_t BSP_FP_Calibrate() {
    MRT_T ret = MRT_OK;
    CHECK_MODULE_INITED;

    ret = mlb_intf_calc_param();
    if (ret != MRT_OK) {
        LOGE("Calibrate failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }

    if (MRT_OK != mlb_intf_power_down_mode()) {
        LOGW("Set power down mode failed");
    }

    LOGI("calibrated");

    return BSP_ERROR_NONE;
}

int16_t BSP_FP_SetUser(int16_t uid) {
    uint16_t org_uid;
    MRT_T    ret;
    (void)org_uid;
    CHECK_MODULE_INITED;

    org_uid = mlb_intf_get_current_user();
    if (org_uid == uid) {
        LOGI("uid not changed, uid=%d", uid);
        return BSP_ERROR_NONE;
    }
    ret = mlb_intf_set_active_user(uid);
    if (ret != MRT_OK) {
        LOGE("Set user failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("org-uid: %d, new-uid=%d", org_uid, uid);

    ret = mlb_intf_get_current_user_fids(ctx.user_fids, &ctx.user_fid_count);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_GetUser(int16_t *uid) {
    CHECK_MODULE_INITED;

    if (uid == NULL) {
        LOGE("Wrong param");
        return BSP_ERROR_WRONG_PARAM;
    }

    *uid = mlb_intf_get_current_user();
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_GetAllUsers(uint16_t *uids, uint16_t *uid_num) {
    CHECK_MODULE_INITED;
    mlb_intf_get_all_users(uids, uid_num);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_DltUser(int16_t uid) {
    MRT_T ret;
    CHECK_MODULE_INITED;

    ret = mlb_intf_delete_user(uid);
    if (ret != MRT_OK) {
        LOGE("Delete user failed, uid=%d, ret=%d", uid, ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("Delete user uid=%d", uid);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_GetCurrentUserFid(uint16_t *fids, uint8_t *fid_num) {
    CHECK_MODULE_INITED;

    mlb_intf_get_current_user_fids(fids, fid_num);
    LOGD("Get fids count=%d", *fid_num);
    for (int i = 0; i < *fid_num; i++) {
        LOGD("fid %d", fids[i]);
    }
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_GetAUserFid(uint16_t uid, uint16_t *fids, uint8_t *fid_num) {
    MRT_T ret;
    CHECK_MODULE_INITED;

    ret = mlb_intf_get_a_user_fids(uid, fids, fid_num);
    if (ret != MRT_OK) {
        LOGW("Get a user fids failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }

    LOGD("Get a user fids, uid=%d, count=%d", uid, *fid_num);
    for (int i = 0; i < *fid_num; i++) {
        LOGD("fid %d", fids[i]);
    }
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_DltUsrFid(uint16_t fid) {
    MRT_T ret;
    CHECK_MODULE_INITED;

    ret = mlb_intf_delete_user_fid(fid);
    if (ret != MRT_OK) {
        LOGE("Delete fid failed, fid: %d, ret=%d", fid, ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("Delete fid: %d", fid);

    mlb_intf_get_current_user_fids(ctx.user_fids, &ctx.user_fid_count);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_Reset() {
    MRT_T    ret;
    uint16_t uids[FP_MAX_USER_UID];
    uint16_t uid_num;
    uint16_t uid_c;

    CHECK_MODULE_INITED;

    LOGI("Begin");

    uid_c = mlb_intf_get_current_user();

    mlb_intf_get_all_users(uids, &uid_num);
    for (int i = 0; i < uid_num; i++) {
        if (uids[i] != uid_c) {
            ret = mlb_intf_delete_user(uids[i]);
            if (ret != MRT_OK) {
                LOGE("Delete user failed. uid=%d, ret=%d", uids[i], ret);
                return BSP_ERROR_COMPONENT_FAILURE;
            }
        }
    }

    ret = mlb_intf_delete_user_all_fids();
    if (ret != MRT_OK) {
        LOGE("Delete current user's all fid failed. ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }

    LOGI("End");
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_EnrollStart(uint16_t *fid) {
    uint16_t tmp_fid;
    MRT_T    ret;
    CHECK_MODULE_INITED;

    if (ctx.user_fid_count >= FP_MAX_USER_FID) {
        LOGW("User template is full. fid_cnt=%d, max=%d", ctx.user_fid_count, FP_MAX_USER_FID);
        return FP_ERR_ENROLL_FULL;
    }

    ret = mlb_intf_generate_new_fid(&tmp_fid);
    if (ret != MRT_OK) {
        LOGW("Generate new fid failed, ret=%d", ret);
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    LOGI("new fid: %d", tmp_fid);

    *fid = tmp_fid;
    LOGD("Send msg: %s, fid: %d", act_str[TA_E_ENROLL_START], tmp_fid);
    send_msg(TA_E_ENROLL_START, tmp_fid);

    return BSP_ERROR_NONE;
}

int16_t BSP_FP_EnrollCancel(uint16_t fid) {
    CHECK_MODULE_INITED;
    LOGD("Send msg: %s, fid: %d", act_str[TA_E_ENROLL_CANCEL], fid);
    send_msg(TA_E_ENROLL_CANCEL, fid);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_DetectStart(bool continoues) {
    CHECK_MODULE_INITED;
    TASK_ACTION action = continoues ? TA_E_DETECT_CONTINUE : TA_E_DETECT_START;
    LOGD("Send msg: %s, fid: %d", act_str[action], 0xFF);
    send_msg(action, 0xFF);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_DetectStop() {
    CHECK_MODULE_INITED;
    LOGD("Send msg: %s, fid: %d", act_str[TA_E_DETECT_STOP], 0xFF);
    send_msg(TA_E_DETECT_STOP, 0xFF);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_NaviStart() {
    CHECK_MODULE_INITED;
    LOGD("Send msg: %s, fid: %d", act_str[TA_E_NAVI_START], 0xFF);
    send_msg(TA_E_NAVI_START, 0xFF);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_NaviStop() {
    CHECK_MODULE_INITED;
    LOGD("Send msg: %s, fid: %d", act_str[TA_E_NAVI_STOP], 0xFF);
    send_msg(TA_E_NAVI_STOP, 0xFF);
    return BSP_ERROR_NONE;
}

int16_t BSP_FP_Reg(BSP_FP_LISENTER_T listenor) {
    CHECK_MODULE_INITED;
    ctx.listener = listenor;
    return BSP_ERROR_NONE;
}

static inline void send_msg(TASK_ACTION action, uint16_t fid) {
    MSG_T msg = {action, fid};
    tx_queue_send(&ctx.msg_que, &msg, TX_NO_WAIT);
}

static bool is_fingerTouched() {
    uint8_t i = 0;
    while (i < 5) {
        i++;
        if (read_int_pin() == GPIO_PIN_SET) {
            return true;
        }
        tx_thread_sleep(10);
    }
    // LOGW("Finger doesn't touch");
    return false;
}

static bool detect_finger_image() {
    uint8_t i = 0;
    MRT_T   ret;

    while (i < 5) {
        i++;
        ret = mlb_intf_finger_detect();
        if (ret == MRT_OK) {
            LOGD("Finger detected, try count: %d", i);
            return true;
        }
        tx_thread_sleep(10);
    }
    LOGW("Finger detect failed, try count: %d. ret=%d", i, ret);
    return false;
}

static inline void notify_event(uint16_t evt, uint16_t fid) {
    if (ctx.listener != NULL) {
        ctx.listener(evt, fid);
    }
}

static inline void wait_finger_left() {
    while (read_int_pin() == GPIO_PIN_SET) {
        tx_thread_sleep(50);
        LOGV("-------------------- wait for finger left");
    }
    LOGD("Finger left");
}

static void irq_handler(void *data) {
    interrupt_enable(false);
    send_msg(TA_E_TOUCH_INT, 0xFF);
}

#define TIMER_FIRST_TIME   (CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define TIMER_REPEAT_CYCLE (CONFIG_SYS_CLOCK_TICKS_PER_SEC * 60 * 10)

static void refresh_hadle(ULONG arg) {
    LOGV("-------------------- refresh timeout, int-pin: %d", read_int_pin());
    LOGI("Refresh interrupt parameter");
    send_msg(TA_E_REFRESH_TO, 0xFF);
}

static void switch_mode() {
    mlb_intf_detect_mode_low_power();
    if (ctx.state == FSM_STA_IDLE) {
        mlb_intf_power_down_mode();
    }
}

static void fsm_init_IDLE() {
    ctx.enrolling       = false;
    ctx.detecting       = false;
    ctx.detect_continue = false;
    ctx.navigation      = false;
    ctx.state           = FSM_STA_IDLE;
}

static bool fsm_entr_IDLE() {
    ctx.state = FSM_STA_IDLE;
    return true;
}

static bool fsm_exit_IDLE() { return true; }

static bool fsm_entr_ENROLL() {
    LOGV("-------------------- before enroll start, int-pin: %d", read_int_pin());
    mlb_intf_enroll_start(ctx.enroll_fid);
    LOGV("-------------------- after enroll start, int-pin: %d", read_int_pin());
    ctx.enroll_update_times = 0;
    ctx.state               = FSM_STA_ENROLL;
    return true;
}

static bool fsm_exit_ENROLL() {
    if (ctx.enroll_update_times > 0) {
        LOGV("-------------------- before enroll discard, int-pin: %d", read_int_pin());
        mlb_intf_enroll_discard();
        LOGV("-------------------- after enroll discard, int-pin: %d", read_int_pin());
    }
    return true;
}

static bool fsm_entr_DETECT() {
    ctx.state = FSM_STA_DETECT;
    return true;
}

static bool fsm_exit_DETECT() { return true; }

static bool fsm_entr_NAVI() {
    LOGV("-------------------- before navi init, int-pin: %d", read_int_pin());
    mlb_intf_navigation_init();
    LOGV("-------------------- after navi init, int-pin: %d", read_int_pin());
    ctx.state = FSM_STA_NAVI;
    return true;
}

static bool fsm_exit_NAVI() {
    LOGV("-------------------- before navi deinit, int-pin: %d", read_int_pin());
    mlb_intf_navigation_deinit();
    LOGV("-------------------- after navi deinit, int-pin: %d", read_int_pin());
    return true;
}

typedef bool (*fsm_entr_exit)(void);
typedef struct {
    fsm_entr_exit entr;
    fsm_entr_exit exit;
} FSM_STATE;
static const FSM_STATE fsm_state[] = {
    [FSM_STA_IDLE]   = {fsm_entr_IDLE, fsm_exit_IDLE},
    [FSM_STA_ENROLL] = {fsm_entr_ENROLL, fsm_exit_ENROLL},
    [FSM_STA_DETECT] = {fsm_entr_DETECT, fsm_exit_DETECT},
    [FSM_STA_NAVI]   = {fsm_entr_NAVI, fsm_exit_NAVI},
};

static void fsm_sta_switch(FSM_STA new) {
    FSM_STA old = ctx.state;
    if (old == new) {
        LOGI("Same state, %s", fsm_sta[old]);
        return;
    }

    LOGI("switch: %s -> %s", fsm_sta[old], fsm_sta[new]);
    fsm_state[old].exit();
    fsm_state[new].entr();
    switch_mode();
    interrupt_enable((new == FSM_STA_IDLE) ? false : true);
    return;
}

static void fsm_IDLE_handle(FSM_EVT event) {
    switch (event) {
    case FSM_EVT_ENROLL_START:
        fsm_sta_switch(FSM_STA_ENROLL);
        break;
    case FSM_EVT_DETECT_START:
        fsm_sta_switch(FSM_STA_DETECT);
        break;
    case FSM_EVT_NAVI_START:
        fsm_sta_switch(FSM_STA_NAVI);
        break;
    default:
        break;
    }
}

static void fsm_ENROLL_handle(FSM_EVT event) {
    switch (event) {
    case FSM_EVT_ENROLL_CANCE:
    case FSM_EVT_ENROLL_FINSH:
        if (ctx.detecting) {
            fsm_sta_switch(FSM_STA_DETECT);
        } else if (ctx.navigation) {
            fsm_sta_switch(FSM_STA_NAVI);
        } else {
            fsm_sta_switch(FSM_STA_IDLE);
        }
        break;
    default:
        break;
    }
}

static void fsm_DETECT_handle(FSM_EVT event) {
    switch (event) {
    case FSM_EVT_ENROLL_START:
        fsm_sta_switch(FSM_STA_ENROLL);
        break;
    case FSM_EVT_DETECT_STOP:
    case FSM_EVT_DETECT_FINSH:
        if (ctx.navigation) {
            fsm_sta_switch(FSM_STA_NAVI);
        } else {
            fsm_sta_switch(FSM_STA_IDLE);
        }
        break;
    default:
        break;
    }
}

static void fsm_NAVI_handle(FSM_EVT event) {
    switch (event) {
    case FSM_EVT_ENROLL_START:
        fsm_sta_switch(FSM_STA_ENROLL);
        break;
    case FSM_EVT_DETECT_START:
        fsm_sta_switch(FSM_STA_DETECT);
        break;
    case FSM_EVT_NAVI_STOP:
        fsm_sta_switch(FSM_STA_IDLE);
        break;
    default:
        break;
    }
}

typedef void (*fsm_handle_fun)(FSM_EVT event);
static const fsm_handle_fun fsm_sta_handle[] = {
    [FSM_STA_IDLE]   = fsm_IDLE_handle,
    [FSM_STA_ENROLL] = fsm_ENROLL_handle,
    [FSM_STA_DETECT] = fsm_DETECT_handle,
    [FSM_STA_NAVI]   = fsm_NAVI_handle,
};

static void fsm_handle_event(FSM_EVT event) {
    LOGI("state: %s, event: %s", fsm_sta[ctx.state], fsm_evt[event]);
    fsm_sta_handle[ctx.state](event);
}

static void enroll_int() {
    uint8_t  tpl_ok = 0;
    MRT_T    ret;
    uint16_t fid = ctx.enroll_fid;

    if (!detect_finger_image()) {
        notify_event(FP_EVENT_ENROLL_NO_FINGER, fid);
        return;
    }

    // Update finger template
    ctx.enroll_update_times++;
#if (FP_DUPLICATE_CHECK_T == FP_DUPLICATE_CHECK_USER)
    if (ctx.user_fid_count == 0) {
        ret = mlb_intf_enroll_update(0, &tpl_ok);
    } else {
        ret =
            mlb_intf_enroll_update_and_check_duplicate(ctx.user_fids, ctx.user_fid_count, &tpl_ok);
    }
#elif (FP_DUPLICATE_CHECK_T == FP_DUPLICATE_CHECK_ALL)
    ret          = mlb_intf_enroll_update(1, &tpl_ok);
#elif (FP_DUPLICATE_CHECK_T == FP_DUPLICATE_CHECK_NONE)
    ret = mlb_intf_enroll_update(0, &tpl_ok);
#endif
    LOGD("update, ret=%d, tpl_ok=%d, fid_count=%d", ret, tpl_ok, ctx.user_fid_count);
    if (ret == MRT_ENROLL_DUP_AREA) {
        LOGW("update failed, duplicate area");
        notify_event(FP_EVENT_ENROLL_AREA_DUP, fid);
        wait_finger_left();
        return;
    } else if (ret == MRT_ENROLL_DUP_FINGER) {
        LOGW("update failed, duplicate finger");
        notify_event(FP_EVENT_ENROLL_FINGER_DUP, fid);
        wait_finger_left();
        return;
    } else {
        LOGI("update succeed, continue times: %d, ret:%d", ctx.enroll_update_times, ret);
        if (!tpl_ok) {
            notify_event(FP_EVENT_ENROLL_CONTINUE, fid);
            wait_finger_left();
            return;
        }
    }

    // Commit finger
    ctx.enroll_update_times = 0;
    mlb_intf_enroll_commit();

    // Set finger to user
    LOGD("Set user fid");
    mlb_intf_set_user_fid(fid); // Allways succeed if user finger isn't full

    // Update user fid info
    mlb_intf_get_current_user_fids(ctx.user_fids, &ctx.user_fid_count);

    LOGI("Enroll done. fid=%d", fid);
    notify_event(FP_EVENT_ENROLL_DONE, fid);
    wait_finger_left();
    ctx.enrolling = false;
    fsm_handle_event(FSM_EVT_ENROLL_FINSH);
}

static void detect_int() {
    uint8_t        update       = 0;
    bool           detected     = false;
    uint16_t       detected_fid = 0;
    static uint8_t failed_count = 0;
    MRT_T          ret;

    if (!detect_finger_image()) {
        return;
    }

    mlb_intf_verify_start();
    for (int i = 0; i < ctx.user_fid_count; i++) {
        ret = mlb_intf_verify_fid(ctx.user_fids[i], &update);
        if (ret == MRT_OK) {
            // if (update) {
            //     mlb_intf_study(); // This api is called in library
            // }
            detected     = true;
            detected_fid = ctx.user_fids[i];
            if (i != 0) {
                ctx.user_fids[i] = ctx.user_fids[0];
                ctx.user_fids[0] = detected_fid;
            }
            LOGD("verify succeed, fid=%d", detected_fid);
            break;
        } else {
            LOGD("verify failed, fid=%d, ret=%d", ctx.user_fids[i], ret);
        }
    }

    if (detected) {
        failed_count = 0;
        notify_event(FP_EVENT_DETECT_SUCCEED, detected_fid);
        wait_finger_left();
        if (!ctx.detect_continue) {
            ctx.detecting = false;
            fsm_handle_event(FSM_EVT_DETECT_FINSH);
        }
    } else if (++failed_count >= 3) {
        failed_count = 0;
        notify_event(FP_EVENT_DETECT_FAILED, detected_fid);
        wait_finger_left();
    }
    return;
}

static void navi_int() {
    uint32_t result = 0;
    MRT_T    ret;

    STACK_DUMP(ctx.stack, STACK_SIZE);
    while (1) {
        LOGV("-------------------- before tap, result: %08lX, int-pin: %d", result, read_int_pin());
        ret = mlb_intf_navigation_tap(&result);
        LOGV("-------------------- after tap, result: %08lX, int-pin: %d", result, read_int_pin());
        if ((ret == MRT_OK) && (result & NAVI_TAP_DOWN)) {
            continue;
        }
        break;
    }
    STACK_DUMP(ctx.stack, STACK_SIZE);
    ret = mlb_intf_navigation_direction(&result);
    LOGV("-------------------- after dir int-pin: %d", read_int_pin());
    LOGD("navi dir result: 0x%08lX, ret:%d", result, ret);
    STACK_DUMP(ctx.stack, STACK_SIZE);
    if (ret == MRT_OK) {
        if (result & FP_NAVI_0) {
            notify_event(FP_EVENT_NAVI, FP_NAVI_0);
        } else if (result & FP_NAVI_1) {
            notify_event(FP_EVENT_NAVI, FP_NAVI_1);
        } else if (result & FP_NAVI_2) {
            notify_event(FP_EVENT_NAVI, FP_NAVI_2);
        } else if (result & FP_NAVI_3) {
            notify_event(FP_EVENT_NAVI, FP_NAVI_3);
        }
    }
}

static void handle_touch_int() {
    static uint8_t abnormal = 0;
    LOGV("-------------------- before check touch, int-pin: %d", read_int_pin());
    if (!is_fingerTouched()) {
        abnormal++;
        LOGW("Finger interrupt abnormal: %d", abnormal);
        if (abnormal >= 50) {
            abnormal = 0;
            LOGI("Update fingerprint interrupt parameter 0");
            mlb_intf_priv_refresh_interrupt(0);
            switch_mode();
        }
    } else {
        abnormal = 0;
        LOGV("-------------------- state: %s, int-pin: %d", fsm_sta[ctx.state], read_int_pin());
        switch (ctx.state) {
        case FSM_STA_IDLE:
            break;
        case FSM_STA_ENROLL:
            enroll_int();
            break;
        case FSM_STA_DETECT:
            detect_int();
            break;
        case FSM_STA_NAVI:
            navi_int();
            break;
        }
        switch_mode(); // Switch low power detect or power down mode
    }

    LOGV("-------------------- after touch int, int-pin: %d", read_int_pin());
    interrupt_enable((ctx.state == FSM_STA_IDLE) ? false : true);
}

static void handle_refresh_to() {
    if (!is_fingerTouched()) {
        LOGV("-------------------- before refresh, int-pin: %d", read_int_pin());
        LOGI("Update fingerprint interrupt parameter: 1");
        mlb_intf_priv_refresh_interrupt(1);
        LOGV("-------------------- after refresh, int-pin: %d", read_int_pin());

        switch_mode();
        LOGV("-------------------- after switch mode, int-pin: %d", read_int_pin());
    }
}

static void fingerprint_main(ULONG arg) {
    MSG_T message;
    UINT  status = TX_SUCCESS;

    LOGI("Thread start");
    ctx.thread_started = true;
    status = tx_queue_create(&ctx.msg_que, "fingerprint queue", sizeof(MSG_T) / 4, &ctx.msg_buf,
                             FP_MSG_MAX * sizeof(MSG_T));
    if (status != TX_SUCCESS) {
        LOGE("Create queue failed status=%d", status);
    } else {
        LOGI("MSG queue created");
    }

    status = tx_timer_create(&ctx.timer, "finger_refresh_timer", refresh_hadle, (ULONG)0,
                             TIMER_FIRST_TIME, TIMER_REPEAT_CYCLE, TX_AUTO_ACTIVATE);
    if (status != TX_SUCCESS) {
        LOGE("Create interrupt refresh timer failed status=%d", status);
    } else {
        LOGI("Interrupt refresh timer created");
    }

    fsm_init_IDLE();
    switch_mode();
    LOGV("-------------------- after switch mode, int-pin: %d", read_int_pin());

    while (1) {
        status = tx_queue_receive(&ctx.msg_que, &message, TX_WAIT_FOREVER);
        LOGD("Receive msg, action=%s, fid=%d, status=%d", act_str[message.action], message.fid,
             status);
        if (status == TX_SUCCESS) {
            switch (message.action) {
            case TA_E_ENROLL_START:
                ctx.enroll_fid = message.fid;
                ctx.enrolling  = true;
                fsm_handle_event(FSM_EVT_ENROLL_START);
                break;
            case TA_E_ENROLL_CANCEL:
                ctx.enrolling = false;
                fsm_handle_event(FSM_EVT_ENROLL_CANCE);
                break;
            case TA_E_DETECT_START:
                ctx.detecting       = true;
                ctx.detect_continue = false;
                fsm_handle_event(FSM_EVT_DETECT_START);
                break;
            case TA_E_DETECT_CONTINUE:
                ctx.detecting       = true;
                ctx.detect_continue = true;
                fsm_handle_event(FSM_EVT_DETECT_START);
                break;
            case TA_E_DETECT_STOP:
                ctx.detecting       = false;
                ctx.detect_continue = false;
                fsm_handle_event(FSM_EVT_DETECT_STOP);
                break;
            case TA_E_NAVI_START:
                ctx.navigation = true;
                fsm_handle_event(FSM_EVT_NAVI_START);
                break;
            case TA_E_NAVI_STOP:
                ctx.navigation = false;
                fsm_handle_event(FSM_EVT_NAVI_STOP);
                break;
            case TA_E_TOUCH_INT:
                handle_touch_int();
                break;
            case TA_E_REFRESH_TO:
                handle_refresh_to();
                break;
            default:
                break;
            }
        }
    }

    ctx.thread_started = false;
}

static UINT low_power_sleep_hook(void *param) {
    lp_func_param_t *wakeup_param = (lp_func_param_t *)param;

    switch (wakeup_param->state) {
    case LOW_POWER_SLEEP:
        // LOGD("LOW_POWER_SLEEP");
        // BSP_FP_Enter_LowPower();
        break;
    case LOW_POWER_DEEP_SLEEP_FD:
        // LOGD("LOW_POWER_DEEP_SLEEP_FD");
        // BSP_FP_Enter_LowPower();
        break;
    case LOW_POWER_DEEP_SLEEP_IP:
        // LOGD("LOW_POWER_DEEP_SLEEP_IP");
        // BSP_FP_Enter_LowPower();
        break;
    case LOW_POWER_DEEP_SLEEP_DEEP:
        // LOGD("LOW_POWER_DEEP_SLEEP_DEEP");
        // BSP_FP_Enter_LowPower();
        break;
    case LOW_POWER_POWER_DOWN:
        // LOGD("LOW_POWER_POWER_DOWN");
        break;
    default:
        // LOGW("UNKNOWN Mode");
        break;
    }

    return 0;
}

static UINT low_power_wakeup_hook(VOID *param) {
    lp_func_param_t *wakeup_param = (lp_func_param_t *)param;

    switch (wakeup_param->state) {
    case LOW_POWER_SLEEP:
        // LOGD("LOW_POWER_SLEEP");
        // BSP_FP_Exit_LowPower();
        break;
    case LOW_POWER_DEEP_SLEEP_FD:
        // LOGD("LOW_POWER_DEEP_SLEEP_FD");
        // BSP_FP_Exit_LowPower();
        break;
    case LOW_POWER_DEEP_SLEEP_IP:
        // LOGD("LOW_POWER_DEEP_SLEEP_IP");
        // BSP_FP_Exit_LowPower();
        break;
    case LOW_POWER_DEEP_SLEEP_DEEP:
        // LOGD("LOW_POWER_DEEP_SLEEP_DEEP");
        // BSP_FP_Exit_LowPower();
        break;
    case LOW_POWER_POWER_DOWN:
        // LOGD("LOW_POWER_POWER_DOWN");
        break;
    default:
        // LOGW("UNKNOWN Mode");
        break;
    }

    return 0;
}