/**
******************************************************************************
* @file    hyn_cst92xx.c
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

#include "daric_pm.h"
#include "hyn_core.h"
#include "cst92xx_fw.h"
#include "tx_work_item_queue.h"
#if defined(CONFIG_BOARD_ACTIVECARD)
#include "axp223.h"
#endif
#include <stdbool.h>
#include "daric_pmu.h"

#define BOOT_I2C_ADDR (0x5A)
#define MAIN_I2C_ADDR (0x5A)
#define RW_REG_LEN    (2)

#define CST92XX_BIN_SIZE (0x7F80)

#define HYNITRON_PROGRAM_PAGE_SIZE (64)

struct hyn_ts_data *hyn_92xxdata = NULL;

// #define CONFIG_TOUCH_TASK_STACK_SIZE 1024

// TX_THREAD thread_touch_int_handler;
// TX_SEMAPHORE sem_touch_int_handler;
// static char thread_touch_int_stack[CONFIG_TOUCH_TASK_STACK_SIZE];

#if 0
static const struct hyn_chip_series hyn_6xx_fw[] = {
    {0xCACA9217, 0xffffffff, "cst9217", (u8 *)fw_bin}, // if PART_NO_EN==0 use default chip
    {0xCACA9220, 0xffffffff, "cst9220", (u8 *)fw_bin},
    {0, 0, "", NULL}};
#endif
#if HYN_POWER_ON_UPDATA
static int cst92xx_updata_judge(u8 *p_fw, u16 len);
static int cst92xx_read_chip_id(void);
#endif
static u32  cst92xx_read_checksum(void);
static int  cst92xx_updata_tpinfo(void);
static int  cst92xx_enter_boot(void);
static void cst92xx_rst(void);
static int  cst92xx_set_workmode(enum work_mode mode, u8 enable);
static int  cst92xx_updata_fw(u8 *bin_addr, u16 len);

static GPIO_TypeDef *const s_LinkedInputPx  = CONFIG_TOUCH_INT_PORT; // tp_int
static uint32_t            s_LinkedInputPIN = CONFIG_TOUCH_INT_PIN;
static GPIO_TypeDef *const s_OutputPx       = CONFIG_TOUCH_RST_PORT; // tp_rst
static uint32_t            s_OutputPIN      = CONFIG_TOUCH_RST_PIN;

static pos_info *gPosInfo = NULL;

#if HYN_POWER_ON_UPDATA
static int32_t cst92xx_init(uint32_t instance, struct hyn_ts_data *ts_data)
{
    int ret = 0;
    HYN_ENTER();
    hyn_92xxdata                          = ts_data;
    hyn_92xxdata->plat_data.max_touch_num = 1;                         // 最大手指数
    hyn_92xxdata->plat_data.x_resolution  = CONFIG_TOUCH_X_RESOLUTION; // x最大分辨率
    hyn_92xxdata->plat_data.y_resolution  = CONFIG_TOUCH_Y_RESOLUTION; // y最大分辨率
    hyn_92xxdata->plat_data.swap_xy       = CONFIG_TOUCH_SWAP_XY;      // xy坐标交换
    hyn_92xxdata->plat_data.reverse_x     = CONFIG_TOUCH_REVERSE_X;    // x坐标反向
    hyn_92xxdata->plat_data.reverse_y     = CONFIG_TOUCH_REVERSE_Y;    // y坐标反向

#ifdef CONFIG_TOUCH_POWR
    BSP_PM_PWR_set(CONFIG_TOUCH_POWR, 3300);
#endif
    hyn_set_i2c_addr(hyn_92xxdata, BOOT_I2C_ADDR);
    ret = Touch_iic_Init();
    if (0 != ret)
    {
        printf("Touch_iic_Init fail");
        return -1;
    }
    ret = cst92xx_int_set(instance);
    if (0 != ret)
    {
        printf("cst92xx_int_set faile \n");
        return -1;
    }
    ret = set_rst_gpio_init();
    if (0 != ret)
    {
        printf("set_rst_gpio_init faile \n");
        return -1;
    }
    cst92xx_int_deinit(0);
    ret = cst92xx_read_chip_id();
    if (ret == FALSE)
    {
        HYN_INFO("cst92xx_read_chip_id failed");
        return FALSE;
    }
    cst92xx_rst();
    msleep(60);
    ret = cst92xx_updata_tpinfo();
    if (ret == FALSE)
    {
        HYN_INFO("cst92xx_updata_tpinfo failed");
    }
    hyn_92xxdata->fw_updata_addr = (u8 *)fw_bin;
    hyn_92xxdata->fw_updata_len  = CST92XX_BIN_SIZE;
    hyn_92xxdata->need_updata_fw = cst92xx_updata_judge((u8 *)fw_bin, CST92XX_BIN_SIZE);
    if (hyn_92xxdata->need_updata_fw)
    {
        HYN_INFO("need updata FW !!!");
        // hyn_data->fw_file_name[0] = 0; //use .h to updata
        // hyn_data->hyn_fuc_used->tp_updata_fw(hyn_data->fw_updata_addr,hyn_data->fw_updata_len);
        hyn_92xxdata->fw_file_name[0] = 0;
        cst92xx_updata_fw(hyn_92xxdata->fw_updata_addr, hyn_92xxdata->fw_updata_len);
    }
    HYN_INFO("hyn_92xxdata->need_updata_fw : %d", hyn_92xxdata->need_updata_fw);
    HYN_INFO("cst92xx_init done !!!");
    HYN_INFO("LAST END update,need set int enable");
    HAL_GPIO_EnableIT(s_LinkedInputPx, s_LinkedInputPIN, true);
    cst92xx_rst();
    mdelay(50);
    hyn_set_i2c_addr(hyn_92xxdata, MAIN_I2C_ADDR);
    ret = cst92xx_updata_tpinfo();
    return ret;
}
#else
static int32_t cst92xx_init(uint32_t instance, struct hyn_ts_data *ts_data)
{
    int ret = 0;
    HYN_ENTER();
    hyn_92xxdata                          = ts_data;
    hyn_92xxdata->plat_data.max_touch_num = 1;   // 最大手指数
    hyn_92xxdata->plat_data.x_resolution  = 200; // x最大分辨率
    hyn_92xxdata->plat_data.y_resolution  = 300; // y最大分辨率
    hyn_92xxdata->plat_data.swap_xy       = 0;   // xy坐标交换
    hyn_92xxdata->plat_data.reverse_x     = 0;   // x坐标反向
    hyn_92xxdata->plat_data.reverse_y     = 0;   // y坐标反向
#ifdef CONFIG_BOARD_ACTIVECARD
    {
        HYN_INFO("NEED SET AXP223_ALDO1");
        AXP223_LDO_DCDC_Set_Voltage(AXP223_ALDO1, 3300); // 开启ALD01电路配置
    }
#else
    {
        HYN_INFO("NOT SET AXP223_ALDO1");
    }
#endif
    hyn_set_i2c_addr(hyn_92xxdata, MAIN_I2C_ADDR);
    ret = Touch_iic_Init();
    if (0 != ret)
    {
        printf("Touch_iic_Init fail");
        return -1;
    }
    ret = cst92xx_int_set(instance);
    if (0 != ret)
    {
        printf("cst92xx_int_set faile \n");
        return -1;
    }
    ret = set_rst_gpio_init();
    if (0 != ret)
    {
        printf("set_rst_gpio_init faile \n");
        return -1;
    }
    cst92xx_rst();
    msleep(40);
    ret = cst92xx_updata_tpinfo();
    if (ret == FALSE)
    {
        HYN_INFO("cst92xx_updata_tpinfo failed");
    }
    ret |= cst92xx_set_workmode(NOMAL_MODE, 0);
    return ret;
}
#endif

static int cst92xx_enter_boot(void)
{
    int     ok         = FALSE, t;
    uint8_t i2c_buf[4] = { 0 };
    HYN_ENTER();
    for (t = 10;; t += 2)
    {
        if (t >= 30)
        {
            return FALSE;
        }

        cst92xx_rst();
        mdelay(t);

        ok = hyn_wr_reg(hyn_92xxdata, 0xA001AA, 3, i2c_buf, 0);
        HYN_INFO("cst92xx_enter_boot write 0xA001AA result:%d\n", ok);
        if (ok == FALSE)
        {
            continue;
        }

        mdelay(t);
        ok = hyn_wr_reg(hyn_92xxdata, 0xA002, 2, i2c_buf, 2);
        if (ok == FALSE)
        {
            continue;
        }
        HYN_INFO("read i2c_buf[0]:0x%02x,i2c_buf[1]:0x%02x", i2c_buf[0], i2c_buf[1]);
        if ((i2c_buf[0] == 0x55) && (i2c_buf[1] == 0xB0))
        {
            break;
        }
    }

    // ok = hyn_wr_reg(hyn_92xxdata, 0xA00100, 3, i2c_buf, 0);
    // if (ok == FALSE)
    // {
    //     return FALSE;
    // }

    return TRUE;
}

static int erase_all_mem(void)
{
    int ok = FALSE, t;
    u8  i2c_buf[8];

    // erase_all_mem
    ok = hyn_wr_reg(hyn_92xxdata, 0xA0140000, 4, i2c_buf, 0);
    if (ok == FALSE)
    {
        return FALSE;
    }
    ok = hyn_wr_reg(hyn_92xxdata, 0xA00C807F, 4, i2c_buf, 0);
    if (ok == FALSE)
    {
        return FALSE;
    }
    ok = hyn_wr_reg(hyn_92xxdata, 0xA004EC, 3, i2c_buf, 0);
    if (ok == FALSE)
    {
        return FALSE;
    }

    mdelay(300);
    for (t = 0;; t += 10)
    {
        if (t >= 1000)
        {
            return FALSE;
        }

        mdelay(10);

        ok = hyn_wr_reg(hyn_92xxdata, 0xA005, 2, i2c_buf, 1);
        if (ok == FALSE)
        {
            continue;
        }

        if (i2c_buf[0] == 0x88)
        {
            break;
        }
    }

    return TRUE;
}

static int write_mem_page(uint16_t addr, uint8_t *buf, uint16_t len)
{
    int     ok                = FALSE, t;
    uint8_t i2c_buf[1024 + 2] = { 0 };

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x0C;
    i2c_buf[2] = len;
    i2c_buf[3] = len >> 8;
    // ok = hyn_i2c_write_r16(HYN_BOOT_I2C_ADDR, 0xA00C, i2c_buf, 2);
    ok = hyn_write_data(hyn_92xxdata, i2c_buf, RW_REG_LEN, 4);
    if (ok == FALSE)
    {
        return FALSE;
    }

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x14;
    i2c_buf[2] = addr;
    i2c_buf[3] = addr >> 8;
    ok         = hyn_write_data(hyn_92xxdata, i2c_buf, RW_REG_LEN, 4);
    if (ok == FALSE)
    {
        return FALSE;
    }

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x18;
    memcpy(i2c_buf + 2, buf, len);
    ok = hyn_write_data(hyn_92xxdata, i2c_buf, RW_REG_LEN, len + 2);
    if (ok == FALSE)
    {
        return FALSE;
    }

    ok = hyn_wr_reg(hyn_92xxdata, 0xA004EE, 3, i2c_buf, 0);
    if (ok == FALSE)
    {
        return FALSE;
    }

    for (t = 0;; t += 10)
    {
        if (t >= 1000)
        {
            return FALSE;
        }

        mdelay(5);

        ok = hyn_wr_reg(hyn_92xxdata, 0xA005, 2, i2c_buf, 1);
        if (ok == FALSE)
        {
            continue;
        }

        if (i2c_buf[0] == 0x55)
        {
            break;
        }
    }

    return TRUE;
}

static int write_code(u8 *bin_addr, uint8_t retry)
{
    uint8_t  data[HYNITRON_PROGRAM_PAGE_SIZE + 4]; //= (uint8_t *)bin_addr;
    uint16_t addr       = 0;
    uint16_t remain_len = CST92XX_BIN_SIZE;
    int      ret;

    while (remain_len > 0)
    {
        uint16_t cur_len = remain_len;
        if (cur_len > HYNITRON_PROGRAM_PAGE_SIZE)
        {
            cur_len = HYNITRON_PROGRAM_PAGE_SIZE;
        }

        if (0 == hyn_92xxdata->fw_file_name[0])
        {
            memcpy(data, bin_addr + addr, HYNITRON_PROGRAM_PAGE_SIZE);
        }
        else
        {
            ret = copy_for_updata(hyn_92xxdata, data, addr, HYNITRON_PROGRAM_PAGE_SIZE);
            if (ret == FALSE)
            {
                HYN_ERROR("copy_for_updata error");
                return FALSE;
            }
        }
        // HYN_INFO("write_code addr 0x%x 0x%x",addr,*data);
        if (write_mem_page(addr, data, cur_len) == FALSE)
        {
            return FALSE;
        }
        // data += cur_len;
        addr += cur_len;
        remain_len -= cur_len;
    }
    return TRUE;
}

static uint32_t cst92xx_read_checksum(void)
{
    int      ok            = FALSE;
    uint8_t  i2c_buf[4]    = { 0 };
    uint32_t chip_checksum = 0;
    uint8_t  retry         = 5;

    hyn_92xxdata->boot_is_pass = 0;
    ok                         = hyn_wr_reg(hyn_92xxdata, 0xA00300, 3, i2c_buf, 0);
    if (ok == FALSE)
    {
        return FALSE;
    }
    mdelay(2);
    while (retry--)
    {
        mdelay(5);
        ok = hyn_wr_reg(hyn_92xxdata, 0xA000, 2, i2c_buf, 1);
        if (ok == FALSE)
        {
            continue;
        }
        if (i2c_buf[0] != 0)
            break;
    }

    mdelay(1);
    if (i2c_buf[0] == 0x01)
    {
        hyn_92xxdata->boot_is_pass = 1;
        memset(i2c_buf, 0, sizeof(i2c_buf));
        ok = hyn_wr_reg(hyn_92xxdata, 0xA008, 2, i2c_buf, 4);
        if (ok == FALSE)
        {
            return FALSE;
        }

        chip_checksum = ((uint32_t)(i2c_buf[0])) | (((uint32_t)(i2c_buf[1])) << 8) | (((uint32_t)(i2c_buf[2])) << 16) | (((uint32_t)(i2c_buf[3])) << 24);
    }
    else
    {
        hyn_92xxdata->need_updata_fw = 1;
    }

    return chip_checksum;
}

static int cst92xx_updata_fw(u8 *bin_addr, u16 len)
{
#define CHECKSUM_OFFECT (0x7F6C)
    int retry   = 0;
    int ok_copy = TRUE;
    int ok      = FALSE;
    u8  i2c_buf[4];

    u32 fw_checksum = 0;
    HYN_ENTER();

    if (len < CST92XX_BIN_SIZE)
    {
        HYN_ERROR("len = %d", len);
        goto UPDATA_END;
    }
    if (len > CST92XX_BIN_SIZE)
        len = CST92XX_BIN_SIZE;

    if (0 != hyn_92xxdata->fw_file_name[0])
    {
        // node to update
        ok          = copy_for_updata(hyn_92xxdata, i2c_buf, CST92XX_BIN_SIZE - 20, 4);
        fw_checksum = U8TO32(i2c_buf[3], i2c_buf[2], i2c_buf[1], i2c_buf[0]);
        if (hyn_92xxdata->hw_info.ic_fw_checksum == fw_checksum || ok != 0)
        {
            HYN_INFO("no update,fw_checksum is same:0x%04lx", fw_checksum);
            goto UPDATA_END;
        }
    }
    else
    {
        fw_checksum = U8TO32(bin_addr[CHECKSUM_OFFECT + 3], bin_addr[CHECKSUM_OFFECT + 2], bin_addr[CHECKSUM_OFFECT + 1], bin_addr[CHECKSUM_OFFECT + 0]);
    }
    HYN_INFO("updating fw checksum:0x%04lx", fw_checksum);

    hyn_irq_set(hyn_92xxdata, DISABLE);
    hyn_esdcheck_switch(hyn_92xxdata, DISABLE);
    hyn_set_i2c_addr(hyn_92xxdata, BOOT_I2C_ADDR);

    HYN_INFO("updata_fw start");
    for (retry = 1; retry < 5; retry++)
    {
        hyn_92xxdata->fw_updata_process = 0;
        ok                              = cst92xx_enter_boot();
        if (ok == FALSE)
        {
            continue;
        }
        hyn_92xxdata->fw_updata_process = 10;
        ok                              = erase_all_mem();
        if (ok == FALSE)
        {
            continue;
        }
        hyn_92xxdata->fw_updata_process = 20;
        ok                              = write_code(bin_addr, retry);
        if (ok == FALSE)
        {
            continue;
        }
        hyn_92xxdata->fw_updata_process      = 30;
        hyn_92xxdata->hw_info.ic_fw_checksum = cst92xx_read_checksum();
        if (fw_checksum != hyn_92xxdata->hw_info.ic_fw_checksum)
        {
            HYN_INFO("out data fw checksum err:0x%04lx", hyn_92xxdata->hw_info.ic_fw_checksum);
            hyn_92xxdata->fw_updata_process |= 0x80;
            continue;
        }
        hyn_92xxdata->fw_updata_process = 100;
        if (retry >= 5)
        {
            ok_copy = FALSE;
            break;
        }
        break;
    }

    hyn_wr_reg(hyn_92xxdata, 0xA00100, 3, i2c_buf, 0);
    hyn_wr_reg(hyn_92xxdata, 0xA006EE, 3, i2c_buf, 0); // exit boot
    mdelay(2);

UPDATA_END:
    cst92xx_rst();
    mdelay(50);

    hyn_set_i2c_addr(hyn_92xxdata, MAIN_I2C_ADDR);
    HYN_INFO("last config ok_copy:%d", ok_copy);
    if (ok_copy == TRUE)
    {
        cst92xx_updata_tpinfo();
        HYN_INFO("updata_fw success");
    }
    else
    {
        HYN_INFO("updata_fw failed");
    }

    hyn_irq_set(hyn_92xxdata, ENABLE);
    HAL_GPIO_EnableIT(s_LinkedInputPx, s_LinkedInputPIN, true);

    return ok_copy;
}
#if HYN_POWER_ON_UPDATA
static int16_t read_word_from_mem(uint8_t type, uint16_t addr, uint32_t *value)
{
    int16_t ret        = 0;
    uint8_t i2c_buf[4] = { 0 }, t;

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x10;
    i2c_buf[2] = type;
    ret        = hyn_write_data(hyn_92xxdata, i2c_buf, 2, 3);
    if (ret)
    {
        return -1;
    }

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x0C;
    i2c_buf[2] = addr;
    i2c_buf[3] = addr >> 8;
    ret        = hyn_write_data(hyn_92xxdata, i2c_buf, 2, 4);
    if (ret)
    {
        return -1;
    }

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x04;
    i2c_buf[2] = 0xE4;
    ret        = hyn_write_data(hyn_92xxdata, i2c_buf, 2, 3);
    if (ret)
    {
        return -1;
    }

    for (t = 0;; t++)
    {
        if (t >= 100)
        {
            return -1;
        }
        ret = hyn_wr_reg(hyn_92xxdata, 0xA004, 2, i2c_buf, 1);
        if (ret)
        {
            continue;
        }
        if (i2c_buf[0] == 0x00)
        {
            break;
        }
    }
    ret = hyn_wr_reg(hyn_92xxdata, 0xA018, 2, i2c_buf, 4);
    if (ret)
    {
        return -1;
    }
    *value = ((uint32_t)(i2c_buf[0])) | (((uint32_t)(i2c_buf[1])) << 8) | (((uint32_t)(i2c_buf[2])) << 16) | (((uint32_t)(i2c_buf[3])) << 24);

    return 0;
}
#endif
#if HYN_POWER_ON_UPDATA
static int cst92xx_read_chip_id(void)
{
    int16_t  ret   = 0;
    uint8_t  retry = 3;
    uint32_t partno_chip_type, module_id;

    ret = cst92xx_enter_boot();
    if (ret == FALSE)
    {
        HYN_ERROR("enter_bootloader error");
        return -1;
    }
    for (; retry > 0; retry--)
    {
        // partno
        ret = read_word_from_mem(1, 0x077C, &partno_chip_type);
        if (ret)
        {
            continue;
        }
        // module id
        ret = read_word_from_mem(0, 0x7FC0, &module_id);
        if (ret)
        {
            continue;
        }
        if ((partno_chip_type >> 16) == 0xCACA)
        {
            partno_chip_type &= 0xffff;
            break;
        }
    }
    cst92xx_rst();
    msleep(30);
    HYN_INFO("partno_chip_type: 0x%04lx", partno_chip_type);
    HYN_INFO("module_id: 0x%04lx", module_id);
    if ((partno_chip_type != 0x9217) && (partno_chip_type != 0x9220))
    {
        HYN_ERROR("partno_chip_type error 0x%04lx", partno_chip_type);
        // return -1;
    }
    return 0;
}
#endif
static int cst92xx_updata_tpinfo(void)
{
    u8              buf[30];
    struct tp_info *ic  = &hyn_92xxdata->hw_info;
    int             ret = 0;

    cst92xx_set_workmode(0xff, DISABLE);
    ret = hyn_wr_reg(hyn_92xxdata, 0xD101, 2, buf, 0);
    if (ret == FALSE)
    {
        return FALSE;
    }
    mdelay(5);

    // firmware_project_id   firmware_ic_type
    ret = hyn_wr_reg(hyn_92xxdata, 0xD1F4, 2, buf, 28);
    if (ret == FALSE)
    {
        return FALSE;
    }
    ic->fw_project_id = ((uint16_t)buf[17] << 8) + buf[16];
    ic->fw_chip_type  = ((uint16_t)buf[19] << 8) + buf[18];

    // firmware_version
    ic->fw_ver = (buf[23] << 24) | (buf[22] << 16) | (buf[21] << 8) | buf[20];

    // tx_num   rx_num   key_num
    ic->fw_sensor_txnum = ((uint16_t)buf[1] << 8) + buf[0];
    ic->fw_sensor_rxnum = buf[2];
    ic->fw_key_num      = buf[3];

    ic->fw_res_y = (buf[7] << 8) | buf[6];
    ic->fw_res_x = (buf[5] << 8) | buf[4];
    HYN_INFO("IC_info fw_project_id:%04lx ictype:%04lx fw_ver:%lx checksum:%#lx", ic->fw_project_id, ic->fw_chip_type, ic->fw_ver, ic->ic_fw_checksum);

    cst92xx_set_workmode(NOMAL_MODE, ENABLE);

    return TRUE;
}
#if HYN_POWER_ON_UPDATA
static int cst92xx_updata_judge(u8 *p_fw, u16 len)
{
    u32             f_checksum, f_fw_ver, f_ictype, f_fw_project_id;
    u8             *p_data = p_fw + len - 28; // 7F64
    struct tp_info *ic     = &hyn_92xxdata->hw_info;
    int             ret;

    ret = cst92xx_enter_boot();
    if (ret == FALSE)
    {
        HYN_INFO("cst92xx_enter_boot fail,need update");
        return 1;
    }
    hyn_92xxdata->hw_info.ic_fw_checksum = cst92xx_read_checksum();
    if (hyn_92xxdata->boot_is_pass == 0)
    {
        HYN_INFO("boot_is_pass %d,need force update", hyn_92xxdata->boot_is_pass);
        return 1; // need updata
    }

    f_fw_project_id = U8TO16(p_data[1], p_data[0]);
    f_ictype        = U8TO16(p_data[3], p_data[2]);

    f_fw_ver = U8TO16(p_data[7], p_data[6]);
    f_fw_ver = (f_fw_ver << 16) | U8TO16(p_data[5], p_data[4]);

    f_checksum = U8TO16(p_data[11], p_data[10]);
    f_checksum = (f_checksum << 16) | U8TO16(p_data[9], p_data[8]);

    HYN_INFO("Bin_info fw_project_id:0x%04lx ictype:0x%04lx fw_ver:0x%lx checksum:0x%lx", f_fw_project_id, f_ictype, f_fw_ver, f_checksum);
    if (f_ictype != ic->fw_chip_type || f_fw_project_id != ic->fw_project_id)
    {
        HYN_ERROR("not update,please confirm:f_ictype 0x%04lx,f_fw_project_id 0x%04lx", f_ictype, f_fw_project_id);
        return 0; // not updata
    }
    if (f_checksum != ic->ic_fw_checksum && f_fw_ver > ic->fw_ver)
    {
        HYN_ERROR("need update,please confirm:f_checksum 0x%04lx,f_fw_ver 0x%04lx", f_checksum, f_fw_ver);
        return 1; // need updata
    }
    HYN_INFO("cst92xx_updata_judge done, no need update");

    return 0;
}
#endif
//------------------------------------------------------------------------------//

static int cst92xx_set_workmode(enum work_mode mode, u8 enable)
{
    int     ok              = FALSE;
    uint8_t i2c_buf[4]      = { 0 };
    uint8_t i               = 0;
    hyn_92xxdata->work_mode = mode;
    if (mode != NOMAL_MODE)
    {
        hyn_esdcheck_switch(hyn_92xxdata, DISABLE);
    }
    for (i = 0; i < 3; i++)
    {
        ok = hyn_wr_reg(hyn_92xxdata, 0xD11E, 2, i2c_buf, 0);
        if (ok == FALSE)
        {
            msleep(1);
            continue;
        }
        msleep(1);
        ok = hyn_wr_reg(hyn_92xxdata, 0x0002, 2, i2c_buf, 2);
        if (ok == FALSE)
        {
            msleep(1);
            continue;
        }
        if (i2c_buf[1] == 0x1E)
        {
            break;
        }
    }

    switch (mode)
    {
        case NOMAL_MODE:
            hyn_irq_set(hyn_92xxdata, ENABLE);
            hyn_esdcheck_switch(hyn_92xxdata, ENABLE);
            ok = hyn_wr_reg(hyn_92xxdata, 0xD109, 2, i2c_buf, 0);
            if (ok == FALSE)
            {
                return FALSE;
            }
            break;
        case GESTURE_MODE:
            ok = hyn_wr_reg(hyn_92xxdata, 0xD104, 2, i2c_buf, 0);
            if (ok == FALSE)
            {
                return FALSE;
            }
            break;
        case LP_MODE:
            ok = hyn_wr_reg(hyn_92xxdata, 0xD107, 2, i2c_buf, 0);
            if (ok == FALSE)
            {
                return FALSE;
            }
            break;
        case DIFF_MODE:
            ok = hyn_wr_reg(hyn_92xxdata, 0xD10D, 2, i2c_buf, 0);
            if (ok == FALSE)
            {
                return FALSE;
            }
            break;

        case RAWDATA_MODE:
            ok = hyn_wr_reg(hyn_92xxdata, 0xD10A, 2, i2c_buf, 0);
            if (ok == FALSE)
            {
                return FALSE;
            }
            break;
        case FAC_TEST_MODE:
            hyn_wr_reg(hyn_92xxdata, 0xD114, 2, i2c_buf, 0);
            break;
        case DEEPSLEEP:
            hyn_wr_reg(hyn_92xxdata, 0xD105, 2, i2c_buf, 0);
            break;
        default:
            hyn_92xxdata->work_mode = NOMAL_MODE;
            break;
    }

    return TRUE;
}

int32_t cst92xx_supend(uint32_t instance)
{
    HYN_ENTER();
    cst92xx_set_workmode(DEEPSLEEP, 0);
    return 0;
}

int32_t cst92xx_resume(uint32_t instance)
{
    cst92xx_rst();
    msleep(50);
    cst92xx_set_workmode(NOMAL_MODE, 0);
    return 0;
}

static int32_t cst92xx_report(uint32_t Instance)
{
    // HYN_ENTER();
    int     ok                                 = FALSE;
    uint8_t i2c_buf[MAX_POINTS_REPORT * 5 + 5] = { 0 };
    uint8_t finger_num                         = 0;
    // uint8_t key_state = 0, key_id = 0;

    hyn_92xxdata->rp_buf.report_need = REPORT_NONE;

    ok = hyn_wr_reg(hyn_92xxdata, 0xD000, 2, i2c_buf, sizeof(i2c_buf));
    if (ok == FALSE)
    {
        return FALSE;
    }

    if (i2c_buf[6] != 0xAB)
    {
        HYN_INFO("fail buf[6]=0x%02x", i2c_buf[6]);
        return FALSE;
    }

    finger_num = i2c_buf[5] & 0x7F;

    if (finger_num > MAX_POINTS_REPORT)
    {
        HYN_INFO("fail finger_num=%d", finger_num);
        return TRUE;
    }
    hyn_92xxdata->rp_buf.rep_num = finger_num;
    hyn_92xxdata->rp_buf.report_need |= REPORT_POS;

    uint8_t switch_ = i2c_buf[0] & 0x0F;

    if (switch_ == 0x06 || switch_ == 0x07)
    {
        uint8_t *data = i2c_buf;
        // uint8_t *data_ges = i2c_buf + finger_num * 5 + 2;
        uint8_t id = data[0] >> 4;

        uint16_t x = ((uint16_t)(data[1]) << 4) | (data[3] >> 4);
        uint16_t y = ((uint16_t)(data[2]) << 4) | (data[3] & 0x0F);
        uint16_t z = (data[3] & 0x1F) + 0x03;
        // HYN_INFO("finger=%d id=%d x=%d y=%d z=%d", finger_num, id, x, y, z);

        hyn_92xxdata->rp_buf.pos_info[0].pos_id = id;
        hyn_92xxdata->rp_buf.pos_info[0].event  = 1;
        hyn_92xxdata->rp_buf.pos_info[0].pos_x  = x;
        hyn_92xxdata->rp_buf.pos_info[0].pos_y  = y;
        hyn_92xxdata->rp_buf.pos_info[0].pres_z = z;
    }
    else
    {
        hyn_92xxdata->rp_buf.pos_info[0].event = 0;
    }

    // 根据配置修改坐标原点
    if (gPosInfo == NULL)
    {
        HYN_ERROR("gPosInfo is not initialized");
        return FALSE;
    }
    if (hyn_92xxdata->plat_data.swap_xy)
    {
        u16 tmp         = hyn_92xxdata->rp_buf.pos_info[0].pos_x;
        gPosInfo->pos_x = hyn_92xxdata->rp_buf.pos_info[0].pos_y;
        gPosInfo->pos_y = tmp;
    }
    else
    {
        gPosInfo->pos_x = hyn_92xxdata->rp_buf.pos_info[0].pos_x;
        gPosInfo->pos_y = hyn_92xxdata->rp_buf.pos_info[0].pos_y;
    }
    if (hyn_92xxdata->plat_data.reverse_x)
        gPosInfo->pos_x = hyn_92xxdata->plat_data.x_resolution - gPosInfo->pos_x;
    if (hyn_92xxdata->plat_data.reverse_y)
        gPosInfo->pos_y = hyn_92xxdata->plat_data.y_resolution - gPosInfo->pos_y;
    gPosInfo->pos_id = hyn_92xxdata->rp_buf.pos_info[0].pos_id;

    gPosInfo->pres_z = hyn_92xxdata->rp_buf.pos_info[0].pres_z;
    gPosInfo->event  = hyn_92xxdata->rp_buf.pos_info[0].event;

    tx_event_flags_set(&gTouchEventGroup, TOUCH_PANEL_EVENT, TX_OR);
    // HYN_INFO("xc add debug info 2024-11-12 remove write 0xD024AB");
#if 0
    ok = hyn_wr_reg(hyn_92xxdata, 0xD024AB, 3, i2c_buf, 0);
    if (ok == FALSE)
    {
        return FALSE;
    }
#endif
    return TRUE;
}

void workItemHandle(void *param)
{
    cst92xx_report(0);
}

static void GPIO_InterruptHandler(void *)
{
    // HYN_ENTER();
    submitWorkItem(workItemHandle, NULL, DEV_ID_TP);
}

// void thread_touch_int_handler_entry(ULONG thread_input) {
//   while (1) {
//     tx_semaphore_get(&sem_touch_int_handler, TX_WAIT_FOREVER);
//     printf("thread_batt_int_handler_entry\n");
//     cst92xx_report(0);
//   }
// }

int32_t Touch_info_handle(uint32_t Instance, pos_info *touch_info)
{
    HYN_ENTER();
    gPosInfo = touch_info;
    HAL_GPIO_EnableIT(s_LinkedInputPx, s_LinkedInputPIN, true);
    // tx_semaphore_create(&sem_touch_int_handler, "sem_touch_int_handler", 0);
    // tx_thread_create(&thread_touch_int_handler, "touch_int_handler", thread_touch_int_handler_entry, 0,
    //         thread_touch_int_stack, CONFIG_TOUCH_TASK_STACK_SIZE,
    //         1, 1, 4, TX_AUTO_START);
    HYN_INFO("event:%d xy:(%d,%d)\n", gPosInfo->event, gPosInfo->pos_x, gPosInfo->pos_y);
    return 0;
}

int32_t cst92xx_int_set(uint32_t instance)
{
    printf("hyn_cst 9200 interrupt gpio config int\n");
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin              = s_LinkedInputPIN;
    GPIO_InitStruct.Mode             = GPIO_MODE_IT_FALLING | EXTI_WAKEUP;
    GPIO_InitStruct.Pull             = GPIO_NOPULL;
    GPIO_InitStruct.IsrHandler       = GPIO_InterruptHandler;
    GPIO_InitStruct.UserData         = NULL;
    HAL_GPIO_Init(s_LinkedInputPx, &GPIO_InitStruct);
    HAL_GPIO_EnableIT(s_LinkedInputPx, s_LinkedInputPIN, false);
    return 0;
}

int set_rst_gpio_init(void)
{
    int              ret             = 0;
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin              = s_OutputPIN;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull             = GPIO_NOPULL;
    HAL_GPIO_Init(s_OutputPx, &GPIO_InitStruct);
    HAL_GPIO_WritePin(s_OutputPx, s_OutputPIN, GPIO_PIN_SET);
    GPIO_PinState state = HAL_GPIO_ReadPin(s_OutputPx, s_OutputPIN);
    if (state == GPIO_PIN_SET)
    {
        printf("set_rst_gpio_init: SUCCESS (PIN is set high as expected)\n");
    }
    else
    {
        printf("set_rst_gpio_init: FAILURE (PIN is not high)\n");
        ret = 1;
    }
    return ret;
}

static void cst92xx_rst(void)
{
    HYN_ENTER();
    gpio_set_value(0);
    msleep(8);
    gpio_set_value(1);
}

int gpio_set_value(int value)
{
    int ret = 0;

    if (1 == value)
    {
        printf("will set gpio_set_value to 1\n");
        HAL_GPIO_WritePin(s_OutputPx, s_OutputPIN, GPIO_PIN_SET);
        GPIO_PinState state = HAL_GPIO_ReadPin(s_OutputPx, s_OutputPIN);
        if (state != GPIO_PIN_SET)
        {
            printf("gpio_set_value to 1 faile\n");
            ret = -1;
        }
    }
    else if (0 == value)
    {
        printf("will set gpio_set_value to 0\n");
        HAL_GPIO_WritePin(s_OutputPx, s_OutputPIN, GPIO_PIN_RESET);
        GPIO_PinState state = HAL_GPIO_ReadPin(s_OutputPx, s_OutputPIN);
        if (state != GPIO_PIN_RESET)
        {
            printf("gpio_set_value to 0 faile\n");
            ret = -1;
        }
    }
    else
    {
        printf("the set value is invalid");
    }
    GPIO_PinState state = HAL_GPIO_ReadPin(s_OutputPx, s_OutputPIN);
    printf("gpio_set_value to %d\n", state);
    return ret;
}

int32_t cst92xx_int_deinit(uint32_t instance)
{
    printf("hyn_cst  interrupt disable\n");
    HAL_GPIO_EnableIT(s_LinkedInputPx, s_LinkedInputPIN, false);
    return 0;
}

int32_t cst92xx_Deint(uint32_t Instance)
{
    cst92xx_int_deinit(Instance);
    printf("cst92xx_Deint");
    return 0;
}

const struct hyn_ts_fuc cst92xx_fuc = { .tp_rest            = cst92xx_rst,
                                        .tp_report          = cst92xx_report,
                                        .tp_supend          = cst92xx_supend,
                                        .tp_resum           = cst92xx_resume,
                                        .tp_chip_init       = cst92xx_init,
                                        .tp_updata_fw       = cst92xx_updata_fw,
                                        .tp_set_workmode    = cst92xx_set_workmode,
                                        .tp_check_esd       = NULL,
                                        .tp_prox_handle     = NULL,
                                        .tp_get_dbg_data    = NULL,
                                        .tp_get_test_result = NULL };

const TS_Drv_t cst92xx_fuc_using = {
    .GetMultiTouchState = Touch_info_handle,
    .Init               = cst92xx_init,
    .DeInit             = cst92xx_Deint,
    .EnableIT           = cst92xx_int_set,
    .DisableIT          = cst92xx_int_deinit,
    .EnterSleep         = cst92xx_supend,
    .ExitSleep          = cst92xx_resume,
};
