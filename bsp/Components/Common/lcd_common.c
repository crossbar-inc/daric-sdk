/**
******************************************************************************
* @file    lcd_common.c
* @author  PERIPHERIAL BSP Team
* @brief   This file contains the common defines and functions prototypes for
*          the lcd_common.c driver.
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

#include "lcd_common.h"
#include "lcd_panel_cfg.h"
//----------------------------------------------------------
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
// lcd define config
#ifdef CONFIG_LCD_DC_IS_REQUIRED
#define LCD_DC_IS_REQUIRED CONFIG_LCD_DC_IS_REQUIRED
#endif
#ifdef CONFIG_LCD_RST_IS_REQUIRED
#define LCD_RST_IS_REQUIRED CONFIG_LCD_RST_IS_REQUIRED
#endif
#ifdef CONFIG_LCD_BUSY_IS_REQUIRED
#define LCD_BUSY_IS_REQUIRED CONFIG_LCD_BUSY_IS_REQUIRED
#endif
#ifdef CONFIG_LCD_VCIEN_IS_REQUIRED
#define LCD_VCIEN_IS_REQUIRED CONFIG_LCD_VCIEN_IS_REQUIRED
#endif

#define LCD_SPI_ID       CONFIG_LCD_SPI_ID
#define LCD_SPI_BAUDRATE CONFIG_LCD_SPI_BAUDRATE
//----------------------------------------------------------
// lcd spi handle
SPIM_HandleTypeDef s_hspim_lcd;
BSP_LCD_Driver    *daric_panel_driver = NULL;
BSP_LCD_Cfg_t      Lcd_Ctx[LCD_INSTANCES_NBR];
extern bool is_fast_boot();
//----------------------------------------------------------
void SPI_WriteByte(uint8_t *txbuf, uint32_t txlen)
{
    HAL_SPIM_Send(&s_hspim_lcd, txbuf, txlen, SPIM_CS_AUTO, HAL_MAX_DELAY);
}
void SPI_ReadByte(uint8_t *buf, uint32_t len)
{
    HAL_SPIM_Receive(&s_hspim_lcd, buf, len, SPIM_CS_NONE, HAL_MAX_DELAY);
}

void SPI_Write_Mul_Byte(uint8_t *txbuf, uint32_t txlen)
{
    HAL_SPIM_Send(&s_hspim_lcd, txbuf, txlen, SPIM_CS_NONE, HAL_MAX_DELAY);
}

void SPI_Transmit(uint8_t *txbuf, uint8_t *rxbuf, uint32_t len)
{
    HAL_SPIM_Transfer(&s_hspim_lcd, txbuf, rxbuf, len, SPIM_CS_NONE, HAL_MAX_DELAY);
}
//----------------------------------------------------------
const char *lcd_panel_get_name(void)
{
    if (daric_panel_driver && daric_panel_driver->info)
        return daric_panel_driver->info->lcd_name;
    else
        return NULL;
}

HAL_StatusTypeDef lcd_spi_init(uint32_t Instance)
{
    LCD_FUNC_ENTER();
    s_hspim_lcd.baudrate   = LCD_SPI_BAUDRATE; // Set baud rate to 32MHz
    s_hspim_lcd.id         = LCD_SPI_ID;       // SPI interface ID
    s_hspim_lcd.cs_gpio    = -1;               // Don't use GPIO as chip select
    s_hspim_lcd.cs         = 0;
    s_hspim_lcd.qspi       = 0;                    // Don't use QSPI mode
    s_hspim_lcd.polarity   = SPI_CMD_CFG_CPOL_POS; // Clock polarity is positive
    s_hspim_lcd.phase      = SPI_CMD_CFG_CPHA_STD; // Clock phase is standard mode
    s_hspim_lcd.big_endian = 1;                    // Use big endian mode
    s_hspim_lcd.wordsize   = SPIM_WORDSIZE_8;      // 8-bit word length
    if (HAL_SPIM_Init(&s_hspim_lcd) != HAL_OK)
    {
        LCD_ERROR("HAL_SPIM_Init Failed!!!\n");
        return HAL_ERROR;
    }
    LCD_FUNC_EXIT();
    return HAL_OK;
}

HAL_StatusTypeDef lcd_gpio_init(uint32_t Instance)
{
    __attribute__((unused)) BSP_LCD_Cfg_t *lcd_info = daric_panel_driver->info;
    LCD_FUNC_ENTER();
    LCD_GPIO_Config_t gpio_configs[] = {
#ifdef CONFIG_LCD_DC_IS_REQUIRED
        { &lcd_info->dc_pin, lcd_info->dc_port, GPIO_MODE_OUTPUT, GPIO_NOPULL, "LCD_DC", LCD_DC_IS_REQUIRED },
#endif
#ifdef CONFIG_LCD_VCIEN_IS_REQUIRED
        { &lcd_info->vcien_pin, lcd_info->vcien_port, GPIO_MODE_OUTPUT, GPIO_NOPULL, "LCD_VCIEN", LCD_VCIEN_IS_REQUIRED },
#endif
#ifdef CONFIG_LCD_RST_IS_REQUIRED
        {&lcd_info->rst_pin, lcd_info->rst_port,     GPIO_MODE_OUTPUT, GPIO_PULLUP, "LCD_RST",   LCD_RST_IS_REQUIRED},
#endif
#ifdef CONFIG_LCD_BUSY_IS_REQUIRED
        { &lcd_info->busy_pin, lcd_info->busy_port, GPIO_MODE_INPUT, GPIO_NOPULL, "LCD_BUSY", LCD_BUSY_IS_REQUIRED },
#endif
#ifdef CONFIG_LCD_VDD1_IS_REQUIRED
        { &lcd_info->vdd1_pin, lcd_info->vdd1_port, GPIO_MODE_OUTPUT, GPIO_NOPULL, "LCD_VDD1", CONFIG_LCD_VDD1_IS_REQUIRED },
#endif
#ifdef CONFIG_LCD_VDD2_IS_REQUIRED
        { &lcd_info->vdd2_pin, lcd_info->vdd2_port, GPIO_MODE_OUTPUT, GPIO_NOPULL, "LCD_VDD2", CONFIG_LCD_VDD2_IS_REQUIRED },
#endif
#ifdef CONFIG_LCD_VPP1_IS_REQUIRED
        { &lcd_info->vpp1_pin, lcd_info->vpp1_port, GPIO_MODE_OUTPUT, GPIO_NOPULL, "LCD_VPP1", CONFIG_LCD_VPP1_IS_REQUIRED },
#endif
#ifdef CONFIG_LCD_VPP2_IS_REQUIRED
        { &lcd_info->vpp2_pin, lcd_info->vpp2_port, GPIO_MODE_OUTPUT, GPIO_NOPULL, "LCD_VPP2", CONFIG_LCD_VPP2_IS_REQUIRED },
#endif
    };
    // Iterate through the array to initialize each GPIO
    for (int i = 0; i < ARRAY_SIZE(gpio_configs); i++)
    {
        if (gpio_configs[i].pin && *gpio_configs[i].pin && gpio_configs[i].port)
        {
            GPIO_InitTypeDef gpio_init = { .Pin = *gpio_configs[i].pin, .Mode = gpio_configs[i].mode, .Pull = gpio_configs[i].pull };
            HAL_GPIO_Init(gpio_configs[i].port, &gpio_init);
        }
        else
        {
            if (gpio_configs[i].is_required)
            {
                LCD_ERROR("Error: %s pin is not defined\n", gpio_configs[i].name);
                return HAL_ERROR;
            }
            else
            {
                LCD_WARN("Warning: %s pin is not defined, continuing without it\n", gpio_configs[i].name);
            }
        }
    }
    LCD_FUNC_EXIT();
    return HAL_OK;
}

static HAL_StatusTypeDef panel_if_init(uint32_t Instance)
{
    BSP_LCD_Cfg_t *lcd_info = daric_panel_driver->info;
    LCD_FUNC_ENTER();
    switch (lcd_info->panel_if_type)
    {
        case LCD_PANEL_IF_TYPE_SPI:
            if (lcd_spi_init(Instance) != HAL_OK)
            {
                LCD_ERROR("Failed to initialize lcd_SPI\n");
                return HAL_ERROR;
            }
            if (lcd_gpio_init(Instance) != HAL_OK)
            {
                LCD_ERROR("Failed to initialize lcd_GPIO\n");
                return HAL_ERROR;
            }
            return HAL_OK;

        case LCD_PANEL_IF_TYPE_MIPI:
            return HAL_ERROR;

        case LCD_PANEL_IF_TYPE_EDP:
            return HAL_ERROR;

        case LCD_PANEL_IF_TYPE_DSI:
            return HAL_ERROR;

        default:
            return HAL_ERROR;
    }
    LCD_FUNC_EXIT();
}

HAL_StatusTypeDef lcd_common_probe(uint32_t Instance)
{
    BSP_LCD_Cfg_t    *info;
    BSP_LCD_Ops_t    *ops;
    HAL_StatusTypeDef ret;
    int               i;
    bool fastboot_mode = is_fast_boot();
    static bool mode_flag = false;
    LCD_FUNC_ENTER();

    daric_panel_driver = (BSP_LCD_Driver *)malloc(sizeof(BSP_LCD_Driver));
    if (daric_panel_driver == NULL)
    {
        LCD_ERROR("Failed to allocate memory for daric_panel_driver\n");
        return HAL_ERROR;
    }

    LCD_INFO("lcd_common_probe:fastboot_mode = %d ,mode_flag=%d", fastboot_mode, mode_flag);
    for (i = 0; i < ARRAY_SIZE(supported_lcd); i++)
    {
        daric_panel_driver = supported_lcd[i].drv;
        info               = daric_panel_driver->info;
        ops                = daric_panel_driver->ops;

        if (panel_if_init(Instance) != HAL_OK)
        {
            LCD_ERROR("Failed to initialize panel_if\n");
            return HAL_ERROR;
        }
#if defined(CONFIG_LCD_ULPS_MODE)
        if(!fastboot_mode && !mode_flag) {
            LCD_INFO("Not Fastboot mode, powering on\n");
            mode_flag = true;
            if (ops && ops->Power)
                ops->Power(Instance, true);
        }
#else
        if (ops && ops->Power)
            ops->Power(Instance, true);
#endif
        if (ops && ops->ReadID)
        {
            uint32_t id;
            ret = ops->ReadID(Instance, &id);
            if (ret == HAL_OK)
            {
                LCD_INFO("Found matching lcd panel, ID: 0x%lx\n", (unsigned long)supported_lcd[i].lcd_id);
                break;
            }
        }
#if !defined(CONFIG_LCD_ULPS_MODE)
        if (ops && ops->Power)
            ops->Power(Instance, false);
#endif

        LCD_INFO("lcd panel ID 0x%lx doesn't match, trying next...\n", (unsigned long)supported_lcd[i].lcd_id);
    }

    if (i == ARRAY_SIZE(supported_lcd))
    {
        LCD_ERROR("No matching lcd panel found\n");
        return HAL_ERROR;
    }

    info->one_frame_data_size = info->width * info->height * info->bpp / 8; // Calculate the size of one frame data
    if (ops && ops->Init)
        ret = ops->Init(Instance);

    if (ret != HAL_OK)
    {
        LCD_ERROR("lcd initialization failed\n");
        return ret;
    }

    LCD_INFO("lcd_name = %s\n", info->lcd_name);
    LCD_INFO("lcd.width = %d\n", info->width);
    LCD_INFO("lcd.height = %d\n", info->height);
    LCD_INFO("lcd.bpp = %d\n", info->bpp);
    LCD_INFO("lcd.one_frame_data_size = %lu\n", info->one_frame_data_size);
    LCD_INFO("fastboot_mode = %d ,mode_flag=%d", fastboot_mode, mode_flag);
    LCD_INFO("lcd panel initialization successful\n");
    LCD_FUNC_EXIT();
    return HAL_OK;
}

HAL_StatusTypeDef lcd_spi_deinit(uint32_t Instance)
{
    HAL_SPIM_Deinit(&s_hspim_lcd);
    return HAL_OK;
}

HAL_StatusTypeDef lcd_common_remove(uint32_t Instance)
{
    LCD_FUNC_ENTER();
    if (lcd_spi_deinit(Instance) != HAL_OK)
    {
        LCD_ERROR("Failed to deinitialize lcd SPI\n");
        return HAL_ERROR;
    }
    if (daric_panel_driver)
    {
        free(daric_panel_driver);
        daric_panel_driver = NULL;
    }
    LCD_FUNC_EXIT();
    return HAL_OK;
}
