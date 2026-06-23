

#include "daric_errno.h"
#include "daric_hal_gpio.h"
#include "daric_log.h"
// #include "nfc_stage.h"
#include "tx_port.h"
#include <stdint.h>
#include <tx_api.h>

TX_TIMER timer;

#define DURATION(ms)       ((ms) * (CONFIG_SYS_CLOCK_TICKS_PER_SEC) / 1000)
#define SHORT_DURATION     (300)
#define REPEAT_ON_DURATION (500)
#define REPEAT_OF_DURATION (500)

#define PORT               GPIOE
#define PIN                GPIO_PIN_3

typedef enum {
    ST_STOP,
    ST_SHORT,
    ST_LONG,
    ST_REP_ON,
    ST_REP_OF,
} Status;

Status run_status = ST_STOP;

static void init_gpio() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin  = PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_RESET);
}

static inline void start_vib() { HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_SET); }

static inline void stop_vib() { HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_RESET); }

static inline void start_timer(uint16_t ms) {
    tx_timer_change(&timer, DURATION(ms), 0);
    tx_timer_activate(&timer);
}

static inline void stop_timer() { tx_timer_deactivate(&timer); }

static void timerCallback(ULONG arg) {
    stop_timer();
    switch (run_status) {
    case ST_SHORT:
    case ST_LONG:
        run_status = ST_STOP;
        stop_vib();
        break;
    case ST_REP_ON:
        run_status = ST_REP_OF;
        start_timer(REPEAT_OF_DURATION);
        stop_vib();
        break;
    case ST_REP_OF:
        run_status = ST_REP_ON;
        start_timer(REPEAT_ON_DURATION);
        start_vib();
        break;
    case ST_STOP:
    default:
        break;
    }
}

static void init_timer() {
    UINT status = tx_timer_create(&timer, "rotate_vib_timer", timerCallback, (ULONG)0,
                                  DURATION(SHORT_DURATION), 0, TX_NO_ACTIVATE);
    (void)status;
    LOGD("STATUS: %d", status);
}

int BSP_Vibrator_Init_rotary(void) {
    init_gpio();
    init_timer();
    return BSP_ERROR_NONE;
}

int BSP_Vibrator_Short_rotary() {
    if (run_status != ST_STOP) {
        stop_timer();
        stop_vib();
    }
    run_status = ST_SHORT;
    start_timer(SHORT_DURATION);
    start_vib();
    return BSP_ERROR_NONE;
}

int BSP_Vibrator_Long_rotary(uint32_t ms) {
    if (run_status != ST_STOP) {
        stop_timer();
        stop_vib();
    }
    run_status = ST_LONG;
    start_timer(ms);
    start_vib();
    return BSP_ERROR_NONE;
}

int BSP_Vibrator_Rtp_Play_rotary(void) {
    if (run_status != ST_STOP) {
        stop_timer();
        stop_vib();
    }
    run_status = ST_REP_ON;
    start_timer(REPEAT_ON_DURATION);
    start_vib();
    return BSP_ERROR_NONE;
}