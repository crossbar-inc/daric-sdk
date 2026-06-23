/**
******************************************************************************
* @file    hyn_i2c.c
* @author  PERIPHERIAL BSP Team
* @brief   HAL TOUCH driver
           This file contains the common defines and functions prototypes for
           the hyn_cst92xx.c driver.
******************************************************************************
 * @attention
 *
 * Copyright 2024-2026 CrossBar, Inc.
 * This file has been modified by CrossBar, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************
 */

#include "hyn_core.h"
#include "Touch_common.h"

void hyn_delay_ms(int time)
{
    // HAL_Delay(4 * time);
    HAL_Delay(time);
}

static int i2c_master_send(u8 addr, u8 *buf, u16 len)
{
    int               ret    = TRUE;
    HAL_StatusTypeDef result = HAL_OK;
    if (Touch_IIC.state != HAL_I2C_STATE_READY)
    {
        printf("%s: i2c state is not ready!\n", __func__);
        return -1;
    }
    result = HAL_I2C_Transmit(&Touch_IIC,addr,buf,len,2000);
    if (HAL_OK != result)
    {
        printf("HAL_I2C_Transmit faile\n");
        ret = -1;
    }
    mdelay(5);
    return ret;
}

static int i2c_master_recv(u8 addr, u8 *buf, u16 len)
{
    int               ret    = TRUE;
    HAL_StatusTypeDef result = HAL_OK;
    if (Touch_IIC.state != HAL_I2C_STATE_READY)
    {
        printf("%s: i2c state is not ready!\n", __func__);
        return -1;
    }

    result = HAL_I2C_Receive(&Touch_IIC,addr,buf,len,2000);
    if (HAL_OK != result)
    {
        printf("HAL_I2C_Receive faile\n");
        ret = -1;
    }
    mdelay(5);
    return ret;
}

int hyn_write_data(struct hyn_ts_data *ts_data, u8 *buf, u8 reg_len, u16 len)
{
    int ret = 0;
    ret     = i2c_master_send(ts_data->salve_addr, buf, len);
    return ret < 0 ? -1 : 0;
}

int hyn_read_data(struct hyn_ts_data *ts_data, u8 *buf, u16 len)
{
    int ret = 0;
    ret     = i2c_master_recv(ts_data->salve_addr, buf, len);
    return ret < 0 ? -1 : 0;
}

int hyn_wr_reg(struct hyn_ts_data *ts_data, u32 reg_addr, u8 reg_len, u8 *rbuf, u16 rlen)
{

    int ret = 0, i = 0;
    u8  wbuf[4];
    reg_len = reg_len & 0x0F;
    memset(wbuf, 0, sizeof(wbuf));
    i = reg_len;
    while (i)
    {
        i--;
        wbuf[i] = reg_addr;
        reg_addr >>= 8;
    }
    ret = i2c_master_send(ts_data->salve_addr, wbuf, reg_len);
    if (rlen)
    {
        ret |= i2c_master_recv(ts_data->salve_addr, rbuf, rlen);
    }
    return ret < 0 ? -1 : 0;
}
