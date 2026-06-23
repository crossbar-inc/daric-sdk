/**
 ******************************************************************************
 * @file    daric_activecard_nto_lcm.c
 * @author  PERIPHERIAL BSP Team
 * @brief   Generic LCD interface for upper layer calls
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
#include "daric_errno.h"
#include "daric_activecard_nto_lcm.h"
#include "lcd_common.h"

static BSP_LCD_Ops_t* get_lcd_ops(uint32_t Instance) {
    return daric_panel_driver->ops;
}

static BSP_LCD_Cfg_t* get_lcd_info(uint32_t Instance) {
    return daric_panel_driver->info;
}

/**
 * @brief Initializes the LCD.
 *
 * @details This function initializes the LCD with the specified orientation.
 *
 * @param Instance The instance of the LCD.
 * @param Orientation The orientation of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the initialization is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_Init(uint32_t Instance, uint32_t Orientation)
{
    if (lcd_common_probe(Instance) != HAL_OK) {
        LCD_ERROR("BSP_LCD_Init: lcd_common_probe failed\n");
        return BSP_ERROR_NO_INIT; // Initialization failed
    }
    LCD_INFO("BSP_LCD_Init: lcd_common_probe success\n");
    return BSP_ERROR_NONE;
}


/**
 * @brief Fast initializes the LCD.
 *
 * @details This function fast initializes the LCD.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_FastInit(uint32_t Instance, uint8_t *pBmp)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->FastInit) {
        return ops->FastInit(Instance, pBmp);
    }
    LCD_ERROR("BSP_LCD_FastInit: ops->FastInit is NULL\n");
    return BSP_ERROR_NONE;
}


/** * @brief Switches the LCD mode.
 *
 * @details This function switches the LCD mode.
 *
 * @param Mode The mode to switch to.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_SwitchMode(uint32_t Instance, LCD_Mode Mode)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->SwitchMode) {
        return ops->SwitchMode(Instance, Mode);
    }
    LCD_ERROR("BSP_LCD_FastInit: ops->SwitchMode is NULL\n");
    return BSP_ERROR_NONE;
}

/**
 * @brief Deinitializes the LCD.
 *
 * @details This function deinitializes the LCD.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the deinitialization is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_DeInit(uint32_t Instance)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DeInit) {
        return ops->DeInit(Instance);
    }
    if (lcd_common_remove(Instance) != HAL_OK) {
        LCD_ERROR("BSP_LCD_DeInit: lcd_common_remove failed\n");
        return BSP_ERROR_NO_INIT; // Deinitialization failed
    }
    LCD_INFO("BSP_LCD_DeInit: ops->DeInit is NULL\n");
    return BSP_ERROR_NONE;
}

/**
 * @brief Turns on the LCD display.
 *
 * @details This function turns on the LCD display.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_DisplayOn(uint32_t Instance)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DisplayOn) {
        return ops->DisplayOn(Instance);
    }
    LCD_ERROR("BSP_LCD_DisplayOn: ops->DisplayOn is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Turns off the LCD display.
 *
 * @details This function turns off the LCD display.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_DisplayOff(uint32_t Instance)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DisplayOff) {
        return ops->DisplayOff(Instance);
    }
    LCD_ERROR("BSP_LCD_DisplayOff: ops->DisplayOff is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Resets the LCD.
 *
 * @details This function resets the LCD.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_Reset(uint32_t Instance)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->Reset) {
        return ops->Reset(Instance);
    }
    LCD_ERROR("BSP_LCD_Reset: ops->Reset is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief  Enters deep sleep mode.
 *
 * @details This function puts the LCD into deep sleep mode to save power.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_EnterDeepSleep(uint32_t Instance)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->EnterDeepSleep) {
        return ops->EnterDeepSleep(Instance);
    }
    LCD_ERROR("BSP_LCD_EnterDeepSleep: ops->EnterDeepSleep is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief  Exit deep sleep mode.
 *
 * @details This function puts the LCD exit deep sleep mode.
 *
 * @param Instance The instance of the LCD.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_ExitDeepSleep(uint32_t Instance)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->ExitDeepSleep) {
        return ops->ExitDeepSleep(Instance);
    }
    LCD_ERROR("BSP_LCD_ExitDeepSleep: ops->ExitDeepSleep is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Gets the X size of the LCD.
 *
 * @details This function gets the X size (width) of the LCD.
 *
 * @param Instance The instance of the LCD.
 * @param XSize Pointer to store the X size.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize)
{
    BSP_LCD_Cfg_t *info = get_lcd_info(Instance);
    if (info) {
        *XSize = info->width;
        return 0;
    }
    LCD_ERROR("BSP_LCD_GetXSize: info is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Gets the Y size of the LCD.
 *
 * @details This function gets the Y size (height) of the LCD.
 *
 * @param Instance The instance of the LCD.
 * @param YSize Pointer to store the Y size.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize)
{
    BSP_LCD_Cfg_t *info = get_lcd_info(Instance);
    if (info) {
        *YSize = info->height;
        return 0;
    }
    LCD_ERROR("BSP_LCD_GetYSize: info is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Displays a string on the LCD.
 *
 * @details This function displays a string at the specified position on the LCD.
 *
 * @param Instance The instance of the LCD.
 * @param str The string to display.
 * @param Xpos The X position to start displaying the string.
 * @param Ypos The Y position to start displaying the string.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_Display_String(uint32_t Instance, const char *str, uint32_t Xpos, uint32_t Ypos)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DisplayString) {
        return ops->DisplayString(Instance, str, Xpos, Ypos);
    }
    LCD_ERROR("BSP_LCD_Display_String: ops->DisplayString is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Draws a bitmap on the LCD.
 *
 * @details This function draws a bitmap at the specified position on the LCD.
 *
 * @param Instance The instance of the LCD.
 * @param Xpos The X position to start drawing the bitmap.
 * @param Ypos The Y position to start drawing the bitmap.
 * @param pBmp Pointer to the bitmap data.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DrawBitmap) {
        return ops->DrawBitmap(Instance, Xpos, Ypos, pBmp);
    }
    LCD_ERROR("BSP_LCD_DrawBitmap: ops->DrawBitmap is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Draws a horizontal line on the LCD.
 *
 * @details This function draws a horizontal line at the specified position on the LCD.
 *
 * @param Instance The instance of the LCD.
 * @param Xpos The X position to start drawing the line.
 * @param Ypos The Y position to start drawing the line.
 * @param Length The length of the line.
 * @param Color The color of the line.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DrawHLine) {
        return ops->DrawHLine(Instance, Xpos, Ypos, Length, Color);
    }
    LCD_ERROR("BSP_LCD_DrawHLine: ops->DrawHLine is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Draws a vertical line on the LCD.
 *
 * @details This function draws a vertical line at the specified position on the LCD.
 *
 * @param Instance The instance of the LCD.
 * @param Xpos The X position to start drawing the line.
 * @param Ypos The Y position to start drawing the line.
 * @param Length The length of the line.
 * @param Color The color of the line.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->DrawVLine) {
        return ops->DrawVLine(Instance, Xpos, Ypos, Length, Color);
    }
    LCD_ERROR("BSP_LCD_DrawVLine: ops->DrawVLine is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Fills a rectangle on the LCD.
 *
 * @details This function fills a rectangle at the specified position on the LCD with the specified color.
 *
 * @param Instance The instance of the LCD.
 * @param Xpos The X position to start filling the rectangle.
 * @param Ypos The Y position to start filling the rectangle.
 * @param Width The width of the rectangle.
 * @param Height The height of the rectangle.
 * @param Color The color to fill the rectangle with.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->FillRect) {
        return ops->FillRect(Instance, Xpos, Ypos, Width, Height, Color);
    }
    LCD_ERROR("BSP_LCD_FillRect: ops->FillRect is NULL\n");
    return BSP_ERROR_NO_INIT;
}

/**
 * @brief Fills a RGB rectangle on the LCD.
 *
 * @details This function fills a RGB rectangle at the specified position on the LCD with the specified color.
 *
 * @param Instance The instance of the LCD.
 * @param Xpos The X position to start filling the rectangle.
 * @param Ypos The Y position to start filling the rectangle.
 * @param Width The width of the rectangle.
 * @param Height The height of the rectangle.
 * @param pData The pointer to the RGB data.
 *
 * @return Returns BSP_ERROR_NONE if the operation is successful, BSP_ERROR_NO_INIT otherwise.
 */
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint8_t *pData)
{
    BSP_LCD_Ops_t *ops = get_lcd_ops(Instance);
    if (ops && ops->FillRGBRect) {
        return ops->FillRGBRect(Instance, Xpos, Ypos, Width, Height, pData);
    }
    LCD_ERROR("BSP_LCD_FillRGBRect: ops->FillRGBRect is NULL\n");
    return BSP_ERROR_NO_INIT;
}
