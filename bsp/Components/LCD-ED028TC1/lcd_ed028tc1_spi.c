/**
******************************************************************************
* @file    lcd_ed028tc1_spi.c
* @author  PERIPHERIAL BSP Team
* @brief   This file contains the common defines and functions prototypes for
*          the lcd_ed028tc1_spi.c driver.
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
#include "daric_gpio.h"
#include "daric_hal_def.h"
#include "daric_hal_spim.h"
#include "daric_hal_gpio.h"
#include "daric_udma_spim_v3.h"
#include "daric_filex_app.h"
#include "lcd_common.h"
#include "lcd_ed028tc1_spi.h"
#include <stdio.h>
#include <tx_api.h>
// #include "V520.h"

#ifdef CONFIG_LCD_USE_HEADER_FOR_LUT
#include "ED028TC1U2_bin_23temp.h"
#endif

#define LCD_VCIEN_PORT CONFIG_LCD_VCIEN_PORT
#define LCD_VCIEN_PIN  CONFIG_LCD_VCIEN_PIN
#define LCD_RST_PORT   CONFIG_LCD_RST_PORT
#define LCD_RST_PIN    CONFIG_LCD_RST_PIN
#define LCD_BUSY_PORT  CONFIG_LCD_BUSY_PORT
#define LCD_BUSY_PIN   CONFIG_LCD_BUSY_PIN
// extern BSP_LCD_Cfg_t       Lcd_Ctx[LCD_INSTANCES_NBR];

#define VDSC_ADDR      ((volatile uint8_t *)0x602C8000)
#define IO_BUFFER_SIZE 512 // 读写分块大小，保持对齐
// #define V520_FILE_NAME   "V520_WF.bin"
#define V520_FILE_NAME "\\lcd\\TempWF.bin" //---/lcd/TempWF.bin
#define MODE_COUNT     5                   // INIT, A2, DU2, GL16, GC16

// ================= 全局静态缓冲区 =================
BSP_LCD_Cfg_t lcd_ed028tc1_info;
// static FX_MEDIA *gpFileDisk = &gNandflashDisk;

static uint8_t data_gdos[] = { GDOS, 0x02 }; // 0xe0
static uint8_t tsc_data[2] = { 0 };

#ifndef CONFIG_LCD_USE_HEADER_FOR_LUT
static uint8_t    s_last_tsc_data[2] = { 0 }; // 保存最近一次温度数据
static LUT_Data_t g_LUT_Cache[MODE_COUNT];
static uint8_t    s_current_segment = 0xFF;
static LCD_Mode   s_lastMode        = LCD_MODE_INVALID;
static bool       s_lut_loaded      = false;
#endif
// ================= 全局静态缓冲区 =================
extern bool is_fast_boot();
static HAL_StatusTypeDef ED028TC1_Initcode_Config(uint32_t Instance, LCD_Mode Mode);

#ifdef CONFIG_LCD_VCIEN_IS_REQUIRED
#define LCD_PWR(enable) HAL_GPIO_WritePin(lcd_ed028tc1_info.vcien_port, lcd_ed028tc1_info.vcien_pin, (enable) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#else
#define LCD_PWR(enable)
#endif

#ifdef CONFIG_LCD_RST_IS_REQUIRED
#define LCD_RST(enable) HAL_GPIO_WritePin(lcd_ed028tc1_info.rst_port, lcd_ed028tc1_info.rst_pin, (enable) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#else
#define LCD_RST(enable)
#endif

#if defined(CONFIG_LCD_ULPS_MODE)
    #define SPIM_RST0_GPIO_Port    GPIOF
    #define SPIM_RST0_Pin          GPIO_PIN_3
    #define SetGpioReset()                                      \
    do{                                                         \
        GPIO_InitTypeDef GPIO_InitStruct = {0};                 \
        GPIO_InitStruct.Pin = SPIM_RST0_Pin;                       \
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;                \
        GPIO_InitStruct.Pull = GPIO_NOPULL;                     \
        GPIO_InitStruct.IsrHandler = NULL;                      \
        GPIO_InitStruct.UserData = NULL;                        \
        HAL_GPIO_Init(SPIM_RST0_GPIO_Port, &GPIO_InitStruct);  \
    }while(0);
    #define LCD_RST_F3(enable) HAL_GPIO_WritePin(SPIM_RST0_GPIO_Port, SPIM_RST0_Pin, (enable) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#else
    #define SetGpioReset()
    #define LCD_RST_F3(enable)
#endif

#ifdef CONFIG_LCD_DC_IS_REQUIRED
#define LCD_WR_RS(enable) HAL_GPIO_WritePin(lcd_ed028tc1_info.dc_port, lcd_ed028tc1_info.dc_pin, (enable) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#else
#define LCD_WR_RS(enable)
#endif

#if defined(CONFIG_LCD_ULPS_MODE)
//================= SPI UPDATE CS =================
#define SPIM_CSN0_GPIO_Port    GPIOF
#define SPIM_CSN0_Pin          GPIO_PIN_9
#define SPI_SetCs_F9()                                      \
do{                                                         \
    GPIO_InitTypeDef GPIO_InitStruct = {0};                 \
    GPIO_InitStruct.Pin = SPIM_CSN0_Pin;                    \
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;                \
    GPIO_InitStruct.Pull = GPIO_NOPULL;                     \
    GPIO_InitStruct.IsrHandler = NULL;                      \
    GPIO_InitStruct.UserData = NULL;                        \
    HAL_GPIO_Init(SPIM_CSN0_GPIO_Port, &GPIO_InitStruct);   \
}while(0);
#define SPI_CS_PIN_F9(enable) HAL_GPIO_WritePin(SPIM_CSN0_GPIO_Port, SPIM_CSN0_Pin, (enable) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#else
#define SPI_SetCs_F9()
#define SPI_CS_PIN_F9(enable)
#endif
//================= SPI UPDATE CS =================
//================= SPI CS =================
#define SPIM1_CSN0_GPIO_Port GPIOD
#define SPIM1_CSN0_Pin       GPIO_PIN_10
#define SPI_SetCs()                                            \
    do                                                         \
    {                                                          \
        GPIO_InitTypeDef GPIO_InitStruct = { 0 };              \
        GPIO_InitStruct.Pin              = SPIM1_CSN0_Pin;     \
        GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT;   \
        GPIO_InitStruct.Pull             = GPIO_NOPULL;        \
        GPIO_InitStruct.IsrHandler       = NULL;               \
        GPIO_InitStruct.UserData         = NULL;               \
        HAL_GPIO_Init(SPIM1_CSN0_GPIO_Port, &GPIO_InitStruct); \
    } while (0);
#define SPI_CS_PIN(enable) HAL_GPIO_WritePin(SPIM1_CSN0_GPIO_Port, SPIM1_CSN0_Pin, (enable) ? GPIO_PIN_SET : GPIO_PIN_RESET)
//================= SPI CS =================
void SPI_Write_One_Byte(uint8_t txbuf)
{
    SPI_Write_Mul_Byte(&txbuf, 1);
}

static HAL_StatusTypeDef Upload_Temperature_LUT(uint32_t Instance, LCD_Mode Mode);
static HAL_StatusTypeDef Check_Busy_High()
{

    while (HAL_GPIO_ReadPin(lcd_ed028tc1_info.busy_port, lcd_ed028tc1_info.busy_pin) == 0)
    { // 低电平表示忙
        HAL_Delay(1);
    }
    LCD_DEBUG("LCD busy pin is high!");
    return HAL_OK;
}

void ED028TC1_Read_Reg_Data(uint8_t reg, uint8_t *rxbuf, uint32_t rxlen) {
    // SPI_Transmit(&txbuf, rxbuf, rxlen);

    uint8_t tx_buf[1 + rxlen];
    uint8_t rx_tmp[1 + rxlen];

    tx_buf[0] = reg;
    memset(&tx_buf[1], 0x00, rxlen);

    HAL_SPIM_Transfer(&s_hspim_lcd,
                      tx_buf,
                      rx_tmp,
                      1 + rxlen,
                      SPIM_CS_NONE,
                      HAL_MAX_DELAY);

    /* 跳过第 1 字节无效数据 */
    memcpy(rxbuf, &rx_tmp[1], rxlen);
}

void Set_One_Reg(uint8_t reg)
{
    Check_Busy_High();
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(reg);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);
}

static HAL_StatusTypeDef ED028TC1_Reset(uint32_t Instance) {
    LCD_FUNC_ENTER();
#ifdef CONFIG_LCD_RST_IS_REQUIRED
    LCD_RST(1);
    HAL_Delay(10);
    LCD_RST(0);
    HAL_Delay(10);
    LCD_RST(1);
    HAL_Delay(10);
#endif
#if defined(CONFIG_LCD_ULPS_MODE)
    SetGpioReset();
    LCD_RST_F3(1);
    HAL_Delay(10);
    LCD_RST_F3(0);
    HAL_Delay(10);
    LCD_RST_F3(1);
    HAL_Delay(10);
#endif
    return Check_Busy_High();
}
static HAL_StatusTypeDef ED028TC1_Enter_DeepSleep(uint32_t Instance)
{
    LCD_FUNC_ENTER();
    // SPI_CS_PIN(0);
    // SPI_Write_One_Byte(DSLP);
    // SPI_Write_One_Byte(0x01);//enter deep sleep
    // HAL_Delay(1);
    // SPI_CS_PIN(1);
    return Check_Busy_High();
}

static HAL_StatusTypeDef ED028TC1_Exit_DeepSleep(uint32_t Instance)
{
    LCD_FUNC_ENTER();
    // ED028TC1_Init(Instance);
    // ED028TC1_Reset(Instance);
    // SPI_SetCs();
    // Check_Busy_High();
    // ED028TC1_Initcode_Config(Instance, GC16);
    return HAL_OK;
}

#if 0
HAL_StatusTypeDef LCD_WF_Write(void)
{
    UINT status;
    FX_FILE my_file;
    CHAR fileName[] = "V520_WF.bin";
    uint32_t totalBufSize = sizeof(V520);
    uint32_t fileOffset = 0;
    uint32_t writeSize = 0;
    uint8_t *pBuf = NULL;

    LCD_INFO("Welcome to %s test\r\n", __func__);
    LCD_INFO("target file = %s, size = %lu bytes\r\n",
           fileName, (unsigned long)totalBufSize);

    /* 尝试打开文件 */
    status = fx_file_open(gpFileDisk, &my_file, fileName, FX_OPEN_FOR_WRITE);
    if (status != FX_SUCCESS)
    {
        LCD_ERROR("file open FAIL, try to create file...\r\n");

        /* 文件不存在则创建 */
        status = fx_file_create(gpFileDisk, fileName);
        if (status != FX_SUCCESS)
        {
            LCD_ERROR("fx_file_create FAIL, status=%u\r\n", status);
            return HAL_ERROR;
        }

        /* 创建成功后再次打开 */
        status = fx_file_open(gpFileDisk, &my_file, fileName, FX_OPEN_FOR_WRITE);
        if (status != FX_SUCCESS)
        {
            LCD_ERROR("file open FAIL after create, status=%u\r\n", status);
            return HAL_ERROR;
        }
    }

    /* 清空旧内容 */
    fx_file_truncate(&my_file, 0);

    /* 分块写入 V520[] */
    while (totalBufSize > 0)
    {
        writeSize = (totalBufSize > IO_BUFFER_SIZE) ? IO_BUFFER_SIZE : totalBufSize;
        pBuf = (uint8_t *)V520 + fileOffset;

        status = fx_file_write(&my_file, pBuf, writeSize);
        if (status != FX_SUCCESS)
        {
            LCD_ERROR("file write FAIL at offset %lu\r\n", (unsigned long)fileOffset);
            fx_file_close(&my_file);
            return HAL_ERROR;
        }

        fileOffset += writeSize;
        totalBufSize -= writeSize;
    }

    /* 关闭文件并刷新 */
    status = fx_file_close(&my_file);
    if (status != FX_SUCCESS)
    {
        LCD_ERROR("closing file FAIL\r\n");
        return HAL_ERROR;
    }

    status = fx_media_flush(gpFileDisk);
    if (status != FX_SUCCESS)
    {
        LCD_ERROR("media flush FAIL\r\n");
        return HAL_ERROR;
    }

    LCD_INFO("create file SUCCESS\r\n");
    return HAL_OK;
}
#endif

static HAL_StatusTypeDef ED028TC1_Set_Display_Window(uint32_t Instance, uint8_t display_mode, uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    LCD_FUNC_ENTER();

    Check_Busy_High();
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(display_mode); // 0x83
    switch (display_mode)
    {
        case DRF:
            SPI_Write_One_Byte(0x08);
            break;
        default:
            break;
    }

    xStart += 8;
    SPI_Write_One_Byte((xStart >> 8) & 0xFF);
    SPI_Write_One_Byte(xStart & 0xFF);

    SPI_Write_One_Byte((yStart >> 8) & 0xFF);
    SPI_Write_One_Byte(yStart & 0xFF);

    SPI_Write_One_Byte((xEnd >> 8) & 0xFF);
    SPI_Write_One_Byte(xEnd & 0xFF);

    SPI_Write_One_Byte((yEnd >> 8) & 0xFF);
    SPI_Write_One_Byte(yEnd & 0xFF);

    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_Fill_RGB_Rect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint8_t *pData)
{
    LCD_FUNC_ENTER();
    Check_Busy_High();
    ED028TC1_Set_Display_Window(Instance, DTMW, Xpos, Ypos, Width, Height);

    Check_Busy_High();

    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(DTM1);
    switch (lcd_ed028tc1_info.bpp)
    {
        case 1:
            SPI_Write_One_Byte(0x00);
            break;
        case 2:
            SPI_Write_One_Byte(0x01);
            break;
        case 3:
            SPI_Write_One_Byte(0x02);
            break;
        case 4:
            SPI_Write_One_Byte(0x03);
            break;
        default:
            SPI_Write_One_Byte(0x00); // 00:1bpp 01:2bpp 02:3bpp 03:4bpp
            break;
    }
    SPI_Write_Mul_Byte(pData, (Width * Height * lcd_ed028tc1_info.bpp / 8));
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    Set_One_Reg(PON);

    ED028TC1_Set_Display_Window(Instance, DRF, Xpos, Ypos, Width, Height);

    Set_One_Reg(POF);

    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_Draw_Bitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
    LCD_FUNC_ENTER();
    Check_Busy_High();
    ED028TC1_Set_Display_Window(Instance, DTMW, Xpos, Ypos, lcd_ed028tc1_info.width, lcd_ed028tc1_info.height);

    Check_Busy_High();
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(DTM1);
    switch (lcd_ed028tc1_info.bpp)
    {
        case 1:
            SPI_Write_One_Byte(0x00);
            break;
        case 2:
            SPI_Write_One_Byte(0x01);
            break;
        case 3:
            SPI_Write_One_Byte(0x02);
            break;
        case 4:
            SPI_Write_One_Byte(0x03);
            break;
        default:
            SPI_Write_One_Byte(0x00); // 00:1bpp 01:2bpp 02:3bpp 03:4bpp
            break;
    }
    SPI_Write_Mul_Byte(pBmp, lcd_ed028tc1_info.one_frame_data_size);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    Set_One_Reg(0x11);

    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_Mul_Byte(data_gdos, 2);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    Set_One_Reg(PON);

    ED028TC1_Set_Display_Window(Instance, DRF, Xpos, Ypos, lcd_ed028tc1_info.width, lcd_ed028tc1_info.height);

    Set_One_Reg(POF);

    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    return HAL_OK;
}

static void LCD_Read_Temperature(void)
{
    HAL_SPIM_Control(&s_hspim_lcd, SPIM_CTRL_SET_BAUDRATE, s_hspim_lcd.wordsize, 1000000);

    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    ED028TC1_Read_Reg_Data(TSC, tsc_data, 1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);
    LCD_INFO("Read TSC data: 0x%02X", tsc_data[0]);

    Check_Busy_High();

    HAL_SPIM_Control(&s_hspim_lcd, SPIM_CTRL_SET_BAUDRATE, s_hspim_lcd.wordsize, CONFIG_LCD_SPI_BAUDRATE);
}

#ifdef CONFIG_LCD_USE_HEADER_FOR_LUT
static HAL_StatusTypeDef Upload_Temperature_LUT_From_Header(uint32_t Instance, LCD_Mode Mode)
{
    uint8_t  segment = 0;
    uint32_t offset  = 0;
    uint16_t frame;
    uint32_t LUTD_Count;

    LCD_FUNC_ENTER();

    Set_One_Reg(PON);

    LCD_Read_Temperature();
    Set_One_Reg(POF);

    Check_Busy_High();

    tsc_data[0] = 23;

    if (tsc_data[0] < 33)
    {
        segment = tsc_data[0] / 3; // 0-32: 每3个单位一段
    }
    else if (tsc_data[0] < 38)
    {
        segment = 11; // 33-37
    }
    else if (tsc_data[0] < 43)
    {
        segment = 12; // 38-42
    }
    else if (tsc_data[0] < 50)
    {
        segment = 13; // 43-49
    }
    else
    {
        segment = 7; // 超出范围//默认23°
    }
    LCD_INFO("segment = %d\n", segment);
    switch (Mode)
    {
        case INIT:
            offset = 0;
            break;
        case A2:
            offset = 6465 + 64 + 256; // 6785
            break;
        case DU2:
            offset = 6785 + 641 + 64 + 256; // 7746
            break;
        case GL16:
            offset = 7746 + 1409 + 64 + 256; // 9475
            break;
        case GC16:
        default:
            offset = 9475 + 2561 + 64 + 256; // 12356
            break;
    }

    frame      = ED028TC1U2_bin_23temp[offset] + 1;
    LUTD_Count = frame * 64 + 1;

    LCD_INFO("frame = %d", frame);
    LCD_INFO("LUTD_Count = %ld", LUTD_Count);

    if (LUTD_Count > 16385)
    {
        LCD_ERROR("LUTD_Count is too large");
        return HAL_ERROR;
    }

    // Write LUTD
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTD); // 0x21
    SPI_Write_Mul_Byte((void *)(ED028TC1U2_bin_23temp + offset), LUTD_Count);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    /// LUTC
    offset += LUTD_Count;
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTC); // 0x20
    SPI_Write_Mul_Byte((void *)(ED028TC1U2_bin_23temp + offset), 64);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    /// LUTR
    offset += 64;
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTR); // 0x22
    SPI_Write_Mul_Byte((void *)(ED028TC1U2_bin_23temp + offset), 256);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    return HAL_OK;
}
#else
static HAL_StatusTypeDef Upload_Temperature_LUT_23Temp(uint32_t Instance, LCD_Mode Mode)
{
    uint8_t  segment = 0;
    uint32_t offset  = 0;
    uint16_t frame;
    uint32_t LUTD_Count;

    // ReRAM起始地址
    const uint8_t *reram_base = (const uint8_t *)0x602C0000;

    LCD_FUNC_ENTER();

    if (tsc_data[0] < 33)
    {
        segment = tsc_data[0] / 3; // 0-32: 每3个单位一段
    }
    else if (tsc_data[0] < 38)
    {
        segment = 11; // 33-37
    }
    else if (tsc_data[0] < 43)
    {
        segment = 12; // 38-42
    }
    else if (tsc_data[0] < 50)
    {
        segment = 13; // 43-49
    }
    else
    {
        segment = 7; // 超出范围//默认23°
    }

    LCD_INFO("segment = %d\n", segment);
    switch (Mode)
    {
        case INIT:
            offset = 0;
            break;
        case A2:
            offset = 6465 + 64 + 256; // 6785
            break;
        case DU2:
            offset = 6785 + 641 + 64 + 256; // 7746
            break;
        case GL16:
            offset = 7746 + 1409 + 64 + 256; // 9475
            break;
        case GC16:
        default:
            offset = 9475 + 2561 + 64 + 256; // 12356
            break;
    }

    const uint8_t *lut_ptr = reram_base + offset;

    frame      = lut_ptr[0] + 1;
    LUTD_Count = frame * 64 + 1;

    LCD_INFO("frame = %d", frame);
    LCD_INFO("LUTD_Count = %ld", LUTD_Count);

    if (LUTD_Count > 16385)
    {
        LCD_ERROR("LUTD_Count is too large");
        return HAL_ERROR;
    }

    // Write LUTD
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTD); // 0x21
    SPI_Write_Mul_Byte((void *)lut_ptr, LUTD_Count);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    /// LUTC
    offset += LUTD_Count;
    lut_ptr = reram_base + offset;
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTC); // 0x20
    SPI_Write_Mul_Byte((void *)lut_ptr, 64);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    /// LUTR
    offset += 64;
    lut_ptr = reram_base + offset;
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTR); // 0x22
    SPI_Write_Mul_Byte((void *)lut_ptr, 256);
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    return HAL_OK;
}

// /* 辅助函数：映射 FileX 错误码到 HAL 错误码 */
// static HAL_StatusTypeDef filex_to_hal(UINT fx_status)
// {
//     switch (fx_status) {
//         case FX_SUCCESS:         return HAL_OK;
//         case FX_NO_MORE_SPACE:   return HAL_ERROR;   // 空间不足
//         case FX_NOT_FOUND:       return HAL_ERROR;   // 文件不存在
//         case FX_ACCESS_ERROR:    return HAL_ERROR;   // 访问错误
//         case FX_PTR_ERROR:       return HAL_ERROR;   // 空指针
//         case FX_CALLER_ERROR:    return HAL_ERROR;   // 调用错误
//         default:                 return HAL_ERROR;   // 其他错误一律 HAL_ERROR
//     }
// }

static HAL_StatusTypeDef Upload_Temperature_LUT_OtherTemp(uint32_t Instance, LCD_Mode Mode)
{
    LCD_FUNC_ENTER();

    bool reload_needed = false;
    bool fastboot_mode = is_fast_boot();
    static bool mode_flag = false;
    LCD_INFO("fastboot_mode = %d ,mode_flag=%d", fastboot_mode, mode_flag);

#if defined(CONFIG_LCD_ULPS_MODE)
    if(fastboot_mode && !mode_flag )
    {
        LCD_INFO("Fastboot mode detected, skipping LUT upload.");
        mode_flag = true;
        LCD_INFO("fastboot_mode = %d ,mode_flag=%d", fastboot_mode, mode_flag);
        // ED028TC1_Reset(Instance);
        return HAL_OK;
    }
#endif
    /* ----------- 根据模式读取温度 ----------- */
    if ((Mode == INIT) || (Mode == GC16) || (Mode == GL16))
    {
        LCD_Read_Temperature();

        if ((tsc_data[0] != s_last_tsc_data[0]) || !s_lut_loaded)
        {
            reload_needed      = true;
            s_last_tsc_data[0] = tsc_data[0];
            // s_last_tsc_data[0] = tsc_data[0];
            LCD_INFO("Temperature changed -> reload LUT");
        }
    }

    /* ----------- 判断 Mode 是否变化 ----------- */
    if (Mode != s_lastMode)
    {
        reload_needed = true;
        LCD_INFO("Mode changed: %d -> %d -> reload LUT", s_lastMode, Mode);
        s_lastMode = Mode;
    }

    /* ----------- 根据温度计算 segment ----------- */
    uint8_t segment = 0;
    if (tsc_data[0] < 33)
        segment = tsc_data[0] / 3;
    else if (tsc_data[0] < 38)
        segment = 11;
    else if (tsc_data[0] < 43)
        segment = 12;
    else if (tsc_data[0] < 50)
        segment = 13;
    else
        segment = 7;

    if (segment != s_current_segment)
    {
        s_current_segment = segment;
        reload_needed     = true;
        LCD_INFO("Temperature segment changed to %d", segment);
    }

    /* ----------- 根据 Mode 选择缓存 ----------- */
    LUT_Data_t *pLUT     = NULL;
    int         mode_idx = -1;
    switch (Mode)
    {
        case INIT:
            mode_idx = 0;
            pLUT     = &g_LUT_Cache[0];
            break;
        case A2:
            mode_idx = 1;
            pLUT     = &g_LUT_Cache[1];
            break;
        case DU2:
            mode_idx = 2;
            pLUT     = &g_LUT_Cache[2];
            break;
        case GL16:
            mode_idx = 3;
            pLUT     = &g_LUT_Cache[3];
            break;
        case GC16:
            mode_idx = 4;
            pLUT     = &g_LUT_Cache[4];
            break;
        default:
            return HAL_ERROR;
    }

    /* ----------- 按需从 Flash 读取 LUT ----------- */
    if (reload_needed || !pLUT->loaded)
    {
        LCD_INFO("Load LUT for mode_idx=%d, segment=%d", mode_idx, segment);

        FX_FILE lcd_wf_file;
        UINT    status = fx_file_open(&gNandflashDisk, &lcd_wf_file, V520_FILE_NAME, FX_OPEN_FOR_READ);
        if (status != FX_SUCCESS)
        {
            LCD_ERROR("open %s FAIL, status=%u", V520_FILE_NAME, status);
            return Upload_Temperature_LUT_23Temp(Instance, Mode);
        }

        uint32_t base_offset = 0;
        switch (mode_idx)
        {
            case 0:
                base_offset = 16705 * (segment);
                break; // INIT
            case 1:
                base_offset = 16705 * (14 + segment);
                break; // A2
            case 2:
                base_offset = 16705 * (28 + segment);
                break; // DU2
            case 3:
                base_offset = 16705 * (42 + segment);
                break; // GL16
            case 4:
                base_offset = 16705 * (56 + segment);
                break; // GC16
        }

        ULONG actualRead;

        // ---- 读取 LUTD ----
        status = fx_file_seek(&lcd_wf_file, base_offset);
        if (status != FX_SUCCESS)
            goto READ_FAIL;

        status = fx_file_read(&lcd_wf_file, pLUT->lutd, LUTD_SIZE, &actualRead);
        if (status != FX_SUCCESS || actualRead != LUTD_SIZE)
            goto READ_FAIL;

        uint16_t frame   = pLUT->lutd[0] + 1;
        pLUT->lutd_count = frame * 64 + 1;

        // ---- 读取 LUTC ----
        fx_file_seek(&lcd_wf_file, base_offset + LUTD_SIZE);
        fx_file_read(&lcd_wf_file, pLUT->lutc, LUTC_SIZE, &actualRead);

        // ---- 读取 LUTR ----
        fx_file_seek(&lcd_wf_file, base_offset + LUTD_SIZE + LUTC_SIZE);
        fx_file_read(&lcd_wf_file, pLUT->lutr, LUTR_SIZE, &actualRead);

        fx_file_close(&lcd_wf_file);
        pLUT->loaded = true;
        LCD_INFO("LUT loaded successfully (mode=%d, frame=%d)", Mode, frame);
        goto SEND_LUT;

    READ_FAIL:
        fx_file_close(&lcd_wf_file);
        LCD_ERROR("Read LUT from flash failed!");
        return HAL_ERROR;
    }

SEND_LUT:
    /* ---- 下发 LUTD ---- */
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTD);
    uint32_t remain = pLUT->lutd_count;
    uint32_t sent   = 0;
    while (remain > 0)
    {
        uint32_t chunk = (remain > 512) ? 512 : remain;
        SPI_Write_Mul_Byte(pLUT->lutd + sent, chunk);
        sent += chunk;
        remain -= chunk;
    }
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    /* ---- 下发 LUTC ---- */
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTC);
    SPI_Write_Mul_Byte(pLUT->lutc, sizeof(pLUT->lutc));
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    /* ---- 下发 LUTR ---- */
    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(LUTR);
    SPI_Write_Mul_Byte(pLUT->lutr, sizeof(pLUT->lutr));
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    s_lut_loaded = true;
    return HAL_OK;
}

static HAL_StatusTypeDef Upload_Temperature_LUT_From_NVM(uint32_t Instance, LCD_Mode Mode)
{
    LCD_FUNC_ENTER();

    Set_One_Reg(PON);

    LCD_Read_Temperature();

    Set_One_Reg(POF);
    Check_Busy_High();

    // tsc_data[0] = 22;

    if((23 == tsc_data[0]))
        return Upload_Temperature_LUT_23Temp(Instance, Mode);
    else
        return Upload_Temperature_LUT_OtherTemp(Instance, Mode);
}
#endif

static HAL_StatusTypeDef Upload_Temperature_LUT(uint32_t Instance, LCD_Mode Mode)
{

    LCD_FUNC_ENTER();
#ifdef CONFIG_LCD_USE_HEADER_FOR_LUT
    return Upload_Temperature_LUT_From_Header(Instance, Mode);
#else
    return Upload_Temperature_LUT_From_NVM(Instance, Mode);
#endif
}

void ED028TC1_DTM2_Initial(uint32_t Instance)
{
    LCD_FUNC_ENTER();
    ED028TC1_Set_Display_Window(Instance, DTMW, 0, 0, lcd_ed028tc1_info.width, lcd_ed028tc1_info.height);

    SPI_CS_PIN(0);
    SPI_CS_PIN_F9(0);
    SPI_Write_One_Byte(DTM1); // 0x10
    switch (lcd_ed028tc1_info.bpp)
    {
        case 1:
            SPI_Write_One_Byte(0x00);
            break;
        case 2:
            SPI_Write_One_Byte(0x01);
            break;
        case 3:
            SPI_Write_One_Byte(0x02);
            break;
        case 4:
            SPI_Write_One_Byte(0x03);
            break;
        default:
            SPI_Write_One_Byte(0x00); // 00:1bpp 01:2bpp 02:3bpp 03:4bpp
            break;
    }
    uint8_t *data_0xff = NULL;
    data_0xff          = malloc(lcd_ed028tc1_info.one_frame_data_size);
    if (data_0xff)
    {
        memset(data_0xff, 0xFF, lcd_ed028tc1_info.one_frame_data_size);
        SPI_Write_Mul_Byte(data_0xff, lcd_ed028tc1_info.one_frame_data_size);
        free(data_0xff);
    }
    HAL_Delay(1);
    SPI_CS_PIN(1);
    SPI_CS_PIN_F9(1);

    Set_One_Reg(0x11);
    LCD_FUNC_EXIT();
    return;
}

static HAL_StatusTypeDef ED028TC1_Initcode_Config(uint32_t Instance, LCD_Mode Mode)
{
    uint8_t spi_commands[][5] = {
        { PWR, 0x03, 0x04, 0x00, 0x00 },  // 5 bytes
        { PSR, 0x21, 0x00 },              // 3 bytes
        { 0x26, 0x82 },                   // 2 bytes
        { PFS, 0x03 },                    // 2 bytes
        { BTST, 0xEF, 0xEF, 0x28 },       // 4 bytes
        { GDOS, 0x02 },                   // 2 bytes
        { PLL, 0x0E },                    // 2 bytes
        { TSE, 0x00 },                    // 2 bytes
        { CDI, 0x01, 0x22 },              // 3 bytes
        { TCON, 0x3F, 0x09, 0x2D },       // 4 bytes
        { TRES, 0x02, 0x60, 0x01, 0xE0 }, // 5 bytes
        { GDOS, 0x02 },                   // 2 bytes
        { VDCS, 0x33 },                   // 2 bytes
        { 0x80, 0x50 },                   // 2 bytes
    };
    uint8_t command_lengths[] = { 5, 3, 2, 2, 4, 2, 2, 2, 3, 4, 5, 2, 2, 2 };
    LCD_FUNC_ENTER();

    uint32_t vdcs_raw = *VDSC_ADDR; // 从 0x602F0000 取出 1 个字节
    if ((vdcs_raw > 10) && (vdcs_raw <= 400))
    {
        uint8_t vdcs_val    = (vdcs_raw / 5) & 0xFF; // 除以 5 并取低 8 位
        spi_commands[12][1] = vdcs_val;              // 写入 VDCS 配置
        LCD_INFO("VDCS raw=%lu (/5 => %u, hex=0x%02X)", (unsigned long)vdcs_raw, vdcs_val, vdcs_val);
    }
    else
    {
        LCD_INFO("VDCS raw=%lu invalid, use default 0x33", (unsigned long)vdcs_raw);
    }

    for (uint8_t i = 0; i < sizeof(command_lengths) / sizeof(command_lengths[0]); i++)
    {
        SPI_CS_PIN(0);
        SPI_CS_PIN_F9(0);
        SPI_Write_Mul_Byte(spi_commands[i], command_lengths[i]);
        HAL_Delay(1);
        SPI_CS_PIN(1);
        SPI_CS_PIN_F9(1);
    }

    Upload_Temperature_LUT(Instance, Mode);
    ED028TC1_DTM2_Initial(Instance);

    Set_One_Reg(PON);
    ED028TC1_Set_Display_Window(Instance, DRF, 0, 0, lcd_ed028tc1_info.width, lcd_ed028tc1_info.height);

    Set_One_Reg(POF);
    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_Power(uint32_t Instance, bool power)
{
    LCD_FUNC_ENTER();
    if (power)
    {
        LCD_INFO("power on\r\n");
#ifdef CONFIG_LCD_VCIEN_IS_REQUIRED
        LCD_PWR(0);
        HAL_Delay(10);
        LCD_PWR(1);
#endif
    }
    else
    {
        LCD_INFO("power off\r\n");
#ifdef CONFIG_LCD_VCIEN_IS_REQUIRED
        LCD_PWR(0);
#endif
    }
    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_ReadId(uint32_t Instance, uint32_t *id)
{
    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_Init(uint32_t Instance)
{
    bool fastboot_mode = is_fast_boot();
    static bool init_flag = false;
    LCD_FUNC_ENTER();
    // LCD_WF_Write();
    // memset(data_0xff, 0xFF, sizeof(data_0xff)); //0xff---white
    LCD_INFO("fastboot_mode = %d ,init_flag=%d", fastboot_mode, init_flag);
#if defined(CONFIG_LCD_ULPS_MODE)
    if(fastboot_mode && !init_flag)
    {
        LCD_INFO("Fastboot mode detected, skip init.");
        init_flag = true;
        // ED028TC1_Reset(Instance);
        SPI_SetCs();
        SPI_SetCs_F9()
        Check_Busy_High();
        ED028TC1_Initcode_Config(Instance, INIT);   // Initialize LCD
        LCD_INFO("fastboot init ok\r\n");
        __NOP();
        return HAL_OK;
    }
#endif
    ED028TC1_Reset(Instance);
    SPI_SetCs();
    SPI_SetCs_F9()
    Check_Busy_High();
    ED028TC1_Initcode_Config(Instance, INIT); // Initialize LCD
    LCD_INFO("init ok\r\n");
    __NOP();
    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_DeInit(uint32_t Instance)
{
    LCD_FUNC_ENTER();
    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_Display_On(uint32_t Instance)
{
    /* do nothing */
    LCD_FUNC_ENTER();
    return HAL_OK;
}

static HAL_StatusTypeDef ED028TC1_Display_Off(uint32_t Instance)
{
    /* do nothing */
    LCD_FUNC_ENTER();
    return HAL_OK;
}

static BSP_LCD_Ops_t ED028TC1_ops = {
    .Init           = ED028TC1_Init,
    .DeInit         = ED028TC1_DeInit,
    .Power          = ED028TC1_Power,
    .ReadID         = ED028TC1_ReadId,
    .Reset          = ED028TC1_Reset,
    .EnterDeepSleep = ED028TC1_Enter_DeepSleep,
    .ExitDeepSleep  = ED028TC1_Exit_DeepSleep,
    .DisplayOn      = ED028TC1_Display_On,
    .DisplayOff     = ED028TC1_Display_Off,
    .DrawBitmap     = ED028TC1_Draw_Bitmap,
    .FillRGBRect    = ED028TC1_Fill_RGB_Rect,
    .SwitchMode     = Upload_Temperature_LUT,
};

BSP_LCD_Cfg_t lcd_ed028tc1_info = {
    .lcd_name      = "ink_ed028tc1_yuantai_spi_qvga",
    .width         = 600,
    .height        = 480,
    .bpp           = 4, // DTM1/DTM2 00b:1bpp 01b:2bpp 10b:3bpp 11b:4bpp
    .panel_if_type = LCD_PANEL_IF_TYPE_SPI,
#ifdef CONFIG_LCD_DC_PORT
    .dc_port = LCD_DC_PORT,
    .dc_pin  = LCD_DC_PIN,
#endif
#ifdef CONFIG_LCD_VCIEN_PORT
    .vcien_port = LCD_VCIEN_PORT,
    .vcien_pin  = LCD_VCIEN_PIN,
#endif
#ifdef CONFIG_LCD_RST_PORT
    .rst_port = LCD_RST_PORT,
    .rst_pin  = LCD_RST_PIN,
#endif
#ifdef CONFIG_LCD_BUSY_PORT
    .busy_port = LCD_BUSY_PORT,
    .busy_pin  = LCD_BUSY_PIN,
#endif
};

BSP_LCD_Driver lcd_ed028tc1_driver = {
    .info = &lcd_ed028tc1_info,
    .ops  = &ED028TC1_ops,
};
