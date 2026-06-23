/**
 ******************************************************************************
 * @file    eta4662.c
 * @author  PERIPHERIAL BSP Team
 * @brief   eta4662 driver.
 *          This file provides some functions for eta4662.
 *          charging functions
 *
 ******************************************************************************
 * @attention
 *
 * © Copyright CrossBar, Inc. 2024.
 *
 * All rights reserved.
 *
 * This software is the proprietary property of CrossBar,
Inc. and is protected  * by copyright laws. Any unauthorized
reproduction, distribution, or  * modification is strictly
prohibited.  *
 ******************************************************************************
 */

#include <stdbool.h>
#include <stdint.h>

#include "daric_errno.h"
#include "daric_hal.h"
#include "daric_hal_i2c.h"
#include "daric_log.h"

#include "eta4662.h"

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a > b ? b : a)

static I2C_HandleTypeDef I2C_Handle;
static bool              I2C_Inited = false;

#define I2C_ADDR CONFIG_BATT_I2C_ADDR

void ETA4662_I2C_init(void) {
    if (I2C_Inited) {
        return;
    }
    memset(&I2C_Handle, 0, sizeof(I2C_HandleTypeDef));
    I2C_Handle.instance_id   = CONFIG_BATT_I2C_ID;
    I2C_Handle.init.wait     = 1;
    I2C_Handle.init.repeat   = 1;
    I2C_Handle.init.baudrate = CONFIG_BATT_I2C_SPEED;
    I2C_Handle.init.rx_buf   = 0;
    I2C_Handle.init.rx_size  = 0;
    I2C_Handle.init.tx_buf   = 0;
    I2C_Handle.init.tx_size  = 0;
    I2C_Handle.init.cmd_buf  = 0;
    I2C_Handle.init.cmd_size = 0;

    HAL_StatusTypeDef result = HAL_I2C_Init(&I2C_Handle);
    if (result != HAL_OK) {
        LOGE("FAILED!\n");
        return;
    }
    I2C_Inited = true;
    LOGI("SUCCEED!\n");
}

static HAL_StatusTypeDef I2C_write(uint8_t reg_address, uint8_t reg_value) {
    int               i   = 5;
    HAL_StatusTypeDef ret = HAL_OK;
    do {
        HAL_Delay(1);
        ret = HAL_I2C_Mem_Write(&I2C_Handle, I2C_ADDR, reg_address, 1,
                                &reg_value, 1, 10);
    } while (ret == HAL_BUSY && i-- >= 0);
    if (ret != HAL_OK) {
        LOGE("FAILED! addr=0x%02X, value=0x%02X", reg_address, reg_value);
    }
    return ret;
}

static HAL_StatusTypeDef I2C_read(uint8_t reg_address, uint8_t *pdata) {
    int               i   = 5;
    HAL_StatusTypeDef ret = HAL_OK;
    do {
        HAL_Delay(1);
        ret = HAL_I2C_Mem_Read(&I2C_Handle, I2C_ADDR, reg_address, 1, pdata, 1,
                               10);
    } while (ret == HAL_BUSY && i-- >= 0);
    if (ret != HAL_OK) {
        LOGE("FAILED! addr=0x%02X", reg_address);
    }
    return ret;
}

static void eta4662_set_value(uint8_t reg, uint8_t reg_bit, uint8_t reg_shift,
                              uint8_t val) {
    HAL_StatusTypeDef ret = HAL_OK;
    uint8_t           tmp;
    ret = I2C_read(reg, &tmp);
    if (ret == HAL_OK) {
        tmp = (tmp & (~(reg_bit << reg_shift))) | (val << reg_shift);
        I2C_write(reg, tmp);
    }
}

HAL_StatusTypeDef eta4662_get_value(uint8_t reg, uint8_t reg_bit,
                                    uint8_t reg_shift, uint8_t *value) {
    HAL_StatusTypeDef ret = I2C_read(reg, value);
    if (ret == HAL_OK) {
        *value = (*value & reg_bit) >> reg_shift;
    }
    return ret;
}

void ETA4662_initRegister(const REG8_MAP *add_val_kv, uint8_t len) {
    if (add_val_kv == NULL || len > 11) {
        return;
    }

    for (int i = 0; i < len; i++) {
        I2C_write(add_val_kv[i].add, add_val_kv[i].val);
    }
}

void ETA4662_setPreChargeCurrent(uint16_t current) {
    uint8_t regval;
    current = MAX(1, MIN(current, 31));
    regval  = (current - 1) / 2;
    eta4662_set_value(0x03, 0x0F, 0, regval);
}

void ETA4662_setConstCurrent(uint16_t current) {
    uint8_t regval;
    current = MAX(8, MIN(current, 456));
    regval  = (current - 8) / 8;
    eta4662_set_value(0x02, 0x3F, 0, regval);
}

void ETA4662_setConstVoltage(uint16_t voltage) {
    uint8_t regval;
    voltage = MAX(3600, MIN(voltage, 4545));
    regval  = (voltage - 3600) / 15;
    eta4662_set_value(0x04, 0xFC, 2, regval);
}

void ETA4662_setTermCurrent(uint16_t current) {
    ETA4662_setPreChargeCurrent(current);
}

void ETA4662_enableCharge() { eta4662_set_value(0x01, 0x01, 3, 0); }

void ETA4662_disableCharge() { eta4662_set_value(0x01, 0x01, 3, 1); }

bool ETA4662_isChargerEnable() {
    uint8_t v = 0;
    eta4662_get_value(0x01, 0x03, 3, &v);
    return ((v&0x18) == 0x00);
}

void ETA4662_enable_hiz() {
    eta4662_set_value(0x01, 0x01, 4, 0);
}

void ETA4662_disable_hiz() {
    eta4662_set_value(0x01, 0x01, 4, 1);
}

void ETA4662_enableInterrup(ETA4662_IRQ_E flag, bool en) {
    uint8_t bit   = 0;
    uint8_t shift = 0;
    uint8_t value = value = en ? 0 : 0x1F;

    switch (flag) {
    case ETA4662_IRQ_BATT_OVT:
        bit   = 0x01;
        shift = 0;
        break;
    case ETA4662_IRQ_NTC:
        bit   = 0x02;
        shift = 1;
        break;
    case ETA4662_IRQ_CHG_STAT:
        bit   = 0x04;
        shift = 2;
        break;
    case ETA4662_IRQ_EOC:
        bit   = 0x08;
        shift = 3;
        break;
    case ETA4662_IRQ_PG:
        bit   = 0x10;
        shift = 4;
        break;
    case ETA4662_IRQ_ALL:
        bit   = 0x1F;
        shift = 0;
        break;
    }
    eta4662_set_value(0x06, bit, shift, value);
}

int ETA4662_getFaultStatus(uint16_t *flags) {
    uint8_t reg08 = 0;
    uint8_t reg09 = 0;
    HAL_StatusTypeDef ret = eta4662_get_value(0x09, 0x3F, 0, &reg09);
    ret |= eta4662_get_value(0x08, 0x3F, 0, &reg08);
    if (ret != HAL_OK) {
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    *flags = (reg08 << 8) + reg09;
    return BSP_ERROR_NONE;
}

int ETA4662_getChargeSt(ETA4662_CHG_ST_E *status) {
    uint8_t           st  = 0;
    HAL_StatusTypeDef ret = eta4662_get_value(0x08, 0x18, 3, &st);
    if (ret != HAL_OK) {
        return BSP_ERROR_COMPONENT_FAILURE;
    }
    *status = st;
    return BSP_ERROR_NONE;
}

void ETA4662_dumpRegister(void) {
    static const uint8_t addr[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
    char str[132] = {0};
    int idx = 0;
    for (int i = 0; i < sizeof(addr); i++) {
        uint8_t v = 0;
        I2C_read(addr[i], &v);
        idx += snprintf(str+idx, sizeof(str)-idx, "[%02X:%02X] ", addr[i], v);
    }
    LOGI("%s", str);
}
