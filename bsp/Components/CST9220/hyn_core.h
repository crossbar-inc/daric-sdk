/**
******************************************************************************
* @file    hyn_core.h
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

#ifndef HYNITRON_CORE_H
#define HYNITRON_CORE_H

#include "daric_hal_def.h"
#include "daric_hal_i2c.h"
#include "daric_hal.h"
// #include "daric_hal_rtc.h"
#include "system_daric.h"
#include "tx_api.h"
#include "daric_hal_gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "hyn_cfg.h"
#ifdef CONFIG_BOARD_ACTIVECARD
#include "daric_activecard_ts.h"
#endif
#ifdef CONFIG_BOARD_EVB
#include "daric_evb_ts.h"
#endif
#ifdef CONFIG_BOARD_EVB2
#include "daric_evb2_ts.h"
#endif
#if defined (CONFIG_BOARD_ACTIVECARD_NTO) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT2) || defined (CONFIG_BOARD_ACTIVECARD_NTO_DVT3)
#include "daric_activecard_nto_ts.h"
#endif
#ifdef CONFIG_BOARD_EVB_NTO
#include "daric_evb_nto_ts.h"
#endif
#include "Touch_common.h"

#define HYN_INFO(fmt, ...) printf("[HYN]" fmt "\n", ##__VA_ARGS__)
#define HYN_INFO2(fmt, args...)  \
    if (hyn_data->log_level > 0) \
    printf("[HYN]" fmt "\n", ##args)
#define HYN_INFO3(fmt, args...)  \
    if (hyn_data->log_level > 1) \
    printf("[HYN]" fmt "\n", ##args)
#define HYN_INFO4(fmt, args...)  \
    if (hyn_data->log_level > 2) \
    printf("[HYN]" fmt "\n", ##args)
#define HYN_ERROR(fmt, args...) printf("[HYN][Error]%s:" fmt "\n", __func__, ##args)
#define HYN_ENTER()             printf("[HYN][enter]%s\n", __func__)

/**delay ms */
#define mdelay(ms) hyn_delay_ms(ms)
#define msleep(ms) hyn_delay_ms(ms)

// #define IS_ERR_OR_NULL(x)  (x <= 0)
#define U8TO16(x1, x2)         ((((x1) & 0xFF) << 8) | ((x2) & 0xFF))
#define U8TO32(x1, x2, x3, x4) ((((x1) & 0xFF) << 24) | (((x2) & 0xFF) << 16) | (((x3) & 0xFF) << 8) | ((x4) & 0xFF))
#define U16REV(x)              ((((x) << 8) & 0xFF00) | (((x) >> 8) & 0x00FF))

#undef NULL
#undef FALSE
#undef TRUE
#undef DISABLE
#undef ENABLE
#define NULL    ((void *)0)
#define FALSE   (-1)
#define TRUE    (0)
#define DISABLE (0)
#define ENABLE  (1)

#define PS_FAR_AWAY 1
#define PS_NEAR     0

#define MULTI_OPEN_TEST  (0x80)
#define MULTI_SHORT_TEST (0x01)
#define MULTI_SCAP_TEST  (0x02)

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;

enum work_mode
{
    NOMAL_MODE      = 0,
    GESTURE_MODE    = 1,
    LP_MODE         = 2,
    DEEPSLEEP       = 3,
    DIFF_MODE       = 4,
    RAWDATA_MODE    = 5,
    BASELINE_MODE   = 6,
    CALIBRATE_MODE  = 7,
    FAC_TEST_MODE   = 8,
    ENTER_BOOT_MODE = 0xCA,
};

enum report_typ
{
    REPORT_NONE = 0,
    REPORT_POS  = 0x01,
    REPORT_KEY  = 0x02,
    REPORT_GES  = 0x04,
    REPORT_PROX = 0x08
};

enum fac_test_ero
{
    FAC_TEST_PASS       = 0,
    FAC_GET_DATA_FAIL   = -1,
    FAC_GET_CFG_FAIL    = -4,
    FAC_TEST_OPENL_FAIL = -5,
    FAC_TEST_OPENH_FAIL = -7,
    FAC_TEST_SHORT_FAIL = -6,
    FAC_TEST_SCAP_FAIL  = -8,
};

enum ges_idx
{
    IDX_U = 0,
    IDX_UP,
    IDX_DOWN,
    IDX_LEFT,
    IDX_RIGHT,
    IDX_O,
    IDX_e,
    IDX_M,
    IDX_L,
    IDX_W,
    IDX_S,
    IDX_V,
    IDX_C,
    IDX_Z,
    IDX_POWER,
    IDX_NULL = 0xFF,
};

struct hyn_plat_data
{
    int reset_gpio;
    int irq_gpio;

    u32 x_resolution;
    u32 y_resolution;
    int swap_xy;
    int reverse_x;
    int reverse_y;

    int max_touch_num;
    int key_num;
    u32 key_x_coords[8]; // max support 8 keys
    u32 key_y_coords;
    u32 key_code[8];
};

struct hyn_chip_series
{
    u32 part_no;
    u32 moudle_id;
    u8  chip_name[20];
    u8 *fw_bin;
};

struct ts_frame
{
    u8              rep_num;
    enum report_typ report_need;
    u8              key_id;
    u8              key_state;

    struct
    {
        u8  pos_id;
        u8  event;
        u16 pos_x;
        u16 pos_y;
        u16 pres_z;
    } pos_info[MAX_POINTS_REPORT];
};

struct tp_info
{
    u8  fw_sensor_txnum;
    u8  fw_sensor_rxnum;
    u8  fw_key_num;
    u8  reserve;
    u16 fw_res_y;
    u16 fw_res_x;
    u32 fw_boot_time;
    u32 fw_project_id;
    u32 fw_chip_type;
    u32 fw_ver;
    u32 ic_fw_checksum;
    u32 fw_module_id;
};

struct hyn_ts_data
{
    u16            bus_type;
    u8             salve_addr;
    int            gpio_irq;
    int            esd_fail_cnt;
    u32            esd_last_value;
    enum work_mode work_mode;

    int                  power_is_on;
    u8                   hyn_irq_flg;
    struct hyn_plat_data plat_data;
    struct tp_info       hw_info;
    struct ts_frame      rp_buf;

    int boot_is_pass;
    int need_updata_fw;
    u8  fw_file_name[128];
    u8 *fw_updata_addr;
    int fw_updata_len;
    int fw_dump_state;
    u8  fw_updata_process;
    u8  host_cmd_save[16];

    u8 log_level;
    u8 prox_is_enable;
    u8 prox_state;

    u8                       gesture_is_enable;
    u8                       gesture_id;
    const struct hyn_ts_fuc *hyn_fuc_used;
};

struct hyn_ts_fuc
{
    void (*tp_rest)(void);
    int32_t (*tp_report)(uint32_t instance);
    int32_t (*tp_supend)(uint32_t instance);
    int32_t (*tp_resum)(uint32_t instance);
    int32_t (*tp_chip_init)(uint32_t instance, struct hyn_ts_data *);
    int (*tp_updata_fw)(u8 *bin_addr, u16 len);
    int (*tp_set_workmode)(enum work_mode mode, u8 enable);
    u32 (*tp_check_esd)(void);
    int (*tp_prox_handle)(u8 cmd);
    int (*tp_get_dbg_data)(u8 *buf, u16 len);
    int (*tp_get_test_result)(u8 *buf, u16 len);
};

int hyn_write_data(struct hyn_ts_data *ts_data, u8 *buf, u8 reg_len, u16 len);
int hyn_read_data(struct hyn_ts_data *ts_data, u8 *buf, u16 len);
int hyn_wr_reg(struct hyn_ts_data *ts_data, u32 reg_addr, u8 reg_len, u8 *rbuf, u16 rlen);

void hyn_irq_set(struct hyn_ts_data *ts_data, u8 value);
void hyn_esdcheck_switch(struct hyn_ts_data *ts_data, u8 enable);
int  copy_for_updata(struct hyn_ts_data *ts_data, u8 *buf, u32 offset, u16 len);
void hyn_set_i2c_addr(struct hyn_ts_data *ts_data, u8 addr);

int hyn_wait_irq_timeout(struct hyn_ts_data *ts_data, int msec);
int factory_multitest(struct hyn_ts_data *ts_data, char *cfg_path, u8 *data, s16 *test_th, u8 test_item);
int fac_test_log_save(char *log_name, struct hyn_ts_data *ts_data, s16 *test_data, int test_ret, u8 test_item);
u16 hyn_sum16(int val, u8 *buf, u16 len);
u32 hyn_sum32(int val, u32 *buf, u16 len);

int     gpio_set_value(int value);
int     set_rst_gpio_init(void);
int32_t cst92xx_int_set(uint32_t instance);
void    hyn_delay_ms(int);
int     Touch_iic_Init(void);
int32_t cst92xx_int_deinit(uint32_t instance);
void    thread_init(void);

extern const struct hyn_ts_fuc cst92xx_fuc;
extern const TS_Drv_t          cst92xx_fuc_using;

#endif
