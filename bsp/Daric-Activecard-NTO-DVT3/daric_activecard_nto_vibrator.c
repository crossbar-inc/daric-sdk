/**
 ******************************************************************************
 * @file    daric_activecard_nto_vibrator.c
 * @author  PERIPHERIAL BSP Team
 * @brief   file of vibrator logic layer.
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
#include "daric_vibrator_linear.h"
#include "daric_errno.h"
#include "daric_vibrator_rot.h"
#include <stdint.h>

#define LINEAR_MOTOR 0
#define ROTARY_MOTOR 1
static int vibrator_type = LINEAR_MOTOR;

typedef struct {
    int (*vib_init)();
    int (*vib_short)();
    int (*vib_long)(uint32_t);
    int (*vib_repeat)();
} Vibrator;

static Vibrator linear_vib = {
    .vib_init   = BSP_Vibrator_Init_linear,
    .vib_short  = BSP_Vibrator_Short_linear,
    .vib_long   = BSP_Vibrator_Long_linear,
    .vib_repeat = BSP_Vibrator_Rtp_Play_linear,
};

static Vibrator rotary_vib = {
    .vib_init   = BSP_Vibrator_Init_rotary,
    .vib_short  = BSP_Vibrator_Short_rotary,
    .vib_long   = BSP_Vibrator_Long_rotary,
    .vib_repeat = BSP_Vibrator_Rtp_Play_rotary,
};

int BSP_Vibrator_Init() {
    int ret = BSP_Vibrator_Init_linear();

    if (BSP_ERROR_NONE == ret) {
        vibrator_type = LINEAR_MOTOR;
    } else {
        vibrator_type = ROTARY_MOTOR;
        ret           = BSP_Vibrator_Init_rotary();
    }
    return ret;
}

int BSP_Vibrator_Short() {
    Vibrator *v = vibrator_type == LINEAR_MOTOR ? &linear_vib : &rotary_vib;
    return v->vib_short();
}

int BSP_Vibrator_Long(uint32_t ms) {
    Vibrator *v = vibrator_type == LINEAR_MOTOR ? &linear_vib : &rotary_vib;
    return v->vib_long(ms);
}

int BSP_Vibrator_Rtp_Play(void) {
    Vibrator *v = vibrator_type == LINEAR_MOTOR ? &linear_vib : &rotary_vib;
    return v->vib_repeat();
}
