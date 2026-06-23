/**
******************************************************************************
* @file    daric_guix_app.c
* @author  OS Team
* @brief   This file's contents are the adaptation of Guix for the SSD1685 LCD.
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

/* Include necessary system files. */
#include "tx_api.h"
#include "gx_api.h"
#include "gx_system.h"
#include "gx_display.h"
#include "gx_utility.h"
#include "daric_activecard_nto_lcm.h"
#include "daric_gui_specifications.h"
#include "daric_gui_resources.h"
#include "daric_guix_app.h"
#include "user_threads_attrdef.h"
#include "core_cm7.h"
#include "tx_log.h"
#include "common.h"

#undef LOG_TAG
#define LOG_TAG "GUI_TAG"

/* Define the display dimentions specific to this implemenation. */
#define GUIX_DISPLAY_WIDTH      480
#define GUIX_DISPLAY_HEIGHT     600
#define OUTPUT_LCD_WIDTH        GUIX_DISPLAY_HEIGHT
#define OUTPUT_LCD_HEIGHT       GUIX_DISPLAY_WIDTH

/* Define the buffer size on the canvas, 1bit per pixel */
#define GUIX_BUFFER_SIZE (GUIX_DISPLAY_WIDTH * GUIX_DISPLAY_HEIGHT / 2)

/* Memory for the frame buffer. */
UCHAR gGuixCanvasBuff[GUIX_BUFFER_SIZE] __attribute__((aligned(4)));

/* [TODO]This buffer would be best dynamically allocated. 
 * However, UI updates are quite frequent, 
 * which might generate too many memory fragments and affect system performance. 
 * Considering that the buffer is not too large, 
 * we'll temporarily use statically allocated memory.
 * */
// UCHAR gExtrRegionBuff[GUIX_BUFFER_SIZE];
UCHAR gExtrRegionBkg[GUIX_BUFFER_SIZE] __attribute__((aligned(4)));

GX_WINDOW *pScreen;
extern GX_WINDOW_ROOT *root;

// TX_THREAD gGuiThread;

/* Lcd driver instance*/
uint32_t gLcdInstance = 0;

static LCD_Mode gCurLcdMode = GC16;
static LCD_Mode gPreLcdMode = INIT;

void guiThreadEntry(ULONG thread_input);
void touch_thread_entry(ULONG thread_input);
extern GX_STUDIO_DISPLAY_INFO daric_gui_display_table[1];

void daricGuiInit(void)
{
    
    /* Create the GUI Thread. */
    // tx_thread_create(&gGuiThread, 
    //                  gUsrThreads_cfg_table[USER_THREAD_GUI].name, 
    //                  guiThreadEntry, 0,
    //                  gUsrThreads_cfg_table[USER_THREAD_GUI].pStack,
    //                  gUsrThreads_cfg_table[USER_THREAD_GUI].stackSize,
    //                  gUsrThreads_cfg_table[USER_THREAD_GUI].priority,
    //                  gUsrThreads_cfg_table[USER_THREAD_GUI].preemptThod,
    //                  gUsrThreads_cfg_table[USER_THREAD_GUI].timeSlice,
    //                  TX_AUTO_START);
}

/* The function is to refresh the specified area of the display, 
 * with each pixel's bit compactly arranged in each bit of each byte, 
 * without any alignment processing.
 * */
void extractMonoRegionFromImageCompact(uint8_t *destBuff, uint8_t *srcBuff, 
                                uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    uint32_t stride;
    uint32_t row;
    uint32_t column;
    uint32_t srcBitInByte;
    uint32_t dstBitInByte;
    uint8_t *srcRow = NULL;
    uint8_t *src = NULL;
    uint8_t *dst = NULL;
    uint8_t mask;
    uint8_t tmpPixel;


    stride = (GUIX_DISPLAY_WIDTH + 7) >> 3;
    srcRow = srcBuff;

    srcRow += top * stride;
    srcRow += (left >> 3);

    dst = destBuff;
    srcBitInByte = left & 0x07;
    mask = (GX_UBYTE)(0x80 >> srcBitInByte);

    if ((mask == 0x80) && ((right - left + 1) % 8 == 0))
    {
        for (row = top; row <= bottom; row++)
        {        
            src = srcRow;

            column = left;
            while (right - column + 1 >= 8)
            {
                *dst++ = *src++;
                column += 8;
            }

            srcRow += stride;
        }
    }
    else
    {
        *dst = 0;
        dstBitInByte = 0;
        
        for (row = top; row <= bottom; row++)
        {
            src = srcRow;
            column = left;
            srcBitInByte = left & 0x07;

            while (column <= right)
            {
                mask = (GX_UBYTE)(0x80 >> srcBitInByte);
                tmpPixel = ((*src) & mask) >> (7 - srcBitInByte);
                tmpPixel <<= (7 - dstBitInByte);
                (*dst) |= tmpPixel;

                srcBitInByte++;
                if (srcBitInByte >= 8)
                {
                    src++;
                    srcBitInByte = 0;
                }

                dstBitInByte++;
                if (dstBitInByte >= 8)
                {
                    dst++;
                    *dst = 0;
                    dstBitInByte = 0;
                }

                column++;

            }
            srcRow += stride;
        }
    }
}

/* The function is to refresh the specified area of the display. 
 * The starting position coordinates of the image area have been aligned, 
 * ensuring that all pixels are 8 pixels aligned.
 * */
void extractMonoRegionFromImageByteAlign(uint8_t *destBuff, uint8_t *srcBuff, 
                                uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    uint32_t stride;
    uint32_t row;
    uint32_t column;
    uint32_t srcBitInByte;
    uint8_t *srcRow = NULL;
    uint8_t *src = NULL;
    uint8_t *dst = NULL;
    uint8_t mask;


    stride = (GUIX_DISPLAY_WIDTH + 7) >> 3;
    srcRow = srcBuff;

    srcRow += top * stride;
    srcRow += (left >> 3);

    dst = destBuff;
    srcBitInByte = left & 0x07;
    mask = (GX_UBYTE)(0x80 >> srcBitInByte);

    if ((mask == 0x80) && ((right - left + 1) % 8 == 0))
    {
        for (row = top; row <= bottom; row++)
        {        
            src = srcRow;

            column = left;
            while (right - column + 1 >= 8)
            {
                *dst++ = *src++;
                column += 8;
            }

            srcRow += stride;
        }
    }
}

__attribute__((section(".SRAMCODE"), aligned(4)))
void rotate4BppGrayscaleBufferV8(const uint8_t* srcBuffer, uint8_t* outBuffer) {
    uint8_t pixelValue1;
    uint8_t pixelValue2;

    int srcByteIdx1;
    int srcByteIdx2;
    int srcByteIdx3;
    int srcByteIdx4;
    int srcByteIdx5;
    int srcByteIdx6;
    int srcByteIdx7;
    int srcByteIdx8;

    uint32_t pixelU32Val1;
    uint32_t pixelU32Val2;
    uint32_t pixelU32Val3;
    uint32_t pixelU32Val4;
    uint32_t pixelU32Val5;
    uint32_t pixelU32Val6;
    uint32_t pixelU32Val7;
    uint32_t pixelU32Val8;
    uint32_t valueU32;
    int outByteIdx;
    int tmpY1;
    int tmpY2;
    int tmpY3;
    int tmpY4;
    int tmpY5;
    int tmpY6;
    int tmpY7;
    int tmpY8;

    int halfxRot1;
    int yRot;

    int halfDispWidth = GUIX_DISPLAY_WIDTH >> 1;
    int halfx;

    for (int y = 0; y < GUIX_DISPLAY_HEIGHT; y += 8)
    {
        halfxRot1 = ((GUIX_DISPLAY_HEIGHT - 1) - y - 6) >> 1;
        tmpY8 = y * (halfDispWidth);
        tmpY7 = tmpY8 + halfDispWidth;
        tmpY6 = tmpY7 + halfDispWidth;
        tmpY5 = tmpY6 + halfDispWidth;
        tmpY4 = tmpY5 + halfDispWidth;
        tmpY3 = tmpY4 + halfDispWidth;
        tmpY2 = tmpY3 + halfDispWidth;
        tmpY1 = tmpY2 + halfDispWidth;

        for (int x = 0; x < GUIX_DISPLAY_WIDTH; x += 8) 
        {
            halfx = x >> 1;
            srcByteIdx1 = tmpY1 + (halfx);
            srcByteIdx2 = tmpY2 + (halfx);
            srcByteIdx3 = tmpY3 + (halfx);
            srcByteIdx4 = tmpY4 + (halfx);
            srcByteIdx5 = tmpY5 + (halfx);
            srcByteIdx6 = tmpY6 + (halfx);
            srcByteIdx7 = tmpY7 + (halfx);
            srcByteIdx8 = tmpY8 + (halfx);

            pixelU32Val1 = *(uint32_t*)&srcBuffer[srcByteIdx1];
            pixelU32Val2 = *(uint32_t*)&srcBuffer[srcByteIdx2];
            pixelU32Val3 = *(uint32_t*)&srcBuffer[srcByteIdx3];
            pixelU32Val4 = *(uint32_t*)&srcBuffer[srcByteIdx4];
            pixelU32Val5 = *(uint32_t*)&srcBuffer[srcByteIdx5];
            pixelU32Val6 = *(uint32_t*)&srcBuffer[srcByteIdx6];
            pixelU32Val7 = *(uint32_t*)&srcBuffer[srcByteIdx7];
            pixelU32Val8 = *(uint32_t*)&srcBuffer[srcByteIdx8];

            yRot = x;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 4) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1) & 0x0f;
            pixelValue2 = (pixelU32Val2) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3) & 0x0f;
            pixelValue2 = (pixelU32Val4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5) & 0x0f;
            pixelValue2 = (pixelU32Val6) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7) & 0x0f;
            pixelValue2 = (pixelU32Val8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 12) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 12) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 12) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 12) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 8) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 20) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 20) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 20) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 20) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 16) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 16) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 16) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 16) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 28) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 28) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 28) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 28) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            yRot += 1;
            outByteIdx = yRot * (OUTPUT_LCD_WIDTH >> 1) + (halfxRot1);
            pixelValue1 = (pixelU32Val1 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 24) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 24) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 24) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 24) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

        }
    }
}

void getRotate4BppAlignPos( GX_RECTANGLE* orgiRect, 
                            GX_RECTANGLE* alignRect,
                            GX_RECTANGLE* rotaAlignRect) 
{

    alignRect->gx_rectangle_left     = (orgiRect->gx_rectangle_left & (~7));
    alignRect->gx_rectangle_top      = (orgiRect->gx_rectangle_top & (~7));
    alignRect->gx_rectangle_right    = ((orgiRect->gx_rectangle_right + 8) & (~7)) - 1;
    alignRect->gx_rectangle_bottom   = ((orgiRect->gx_rectangle_bottom + 8) & (~7)) - 1;

    rotaAlignRect->gx_rectangle_left      = (GUIX_DISPLAY_HEIGHT - 1) - alignRect->gx_rectangle_bottom;
    rotaAlignRect->gx_rectangle_top       = alignRect->gx_rectangle_left;
    rotaAlignRect->gx_rectangle_right     = (GUIX_DISPLAY_HEIGHT - 1) - alignRect->gx_rectangle_top;
    rotaAlignRect->gx_rectangle_bottom    = alignRect->gx_rectangle_right;
}


__attribute__((section(".SRAMCODE"), aligned(4)))
void rotate4BppGrayscalePartialAlignBufferV3(uint8_t *outBuffer, 
                                                  uint8_t *srcBuffer, 
                                                  GX_RECTANGLE* alignRect, 
                                                  GX_RECTANGLE* rotaAlignRect) {
    int halfRotaRectWidth;
    uint32_t pixelU32Val1;
    uint32_t pixelU32Val2;
    uint32_t pixelU32Val3;
    uint32_t pixelU32Val4;
    uint32_t pixelU32Val5;
    uint32_t pixelU32Val6;
    uint32_t pixelU32Val7;
    uint32_t pixelU32Val8;
    uint32_t valueU32;
    uint8_t pixelValue1;
    uint8_t pixelValue2;
    
    int srcByteIdx1;
    int srcByteIdx2;
    int srcByteIdx3;
    int srcByteIdx4;
    int srcByteIdx5;
    int srcByteIdx6;
    int srcByteIdx7;
    int srcByteIdx8;
    
    int outByteIdx;
    int tmpY1;
    int tmpY2;
    int tmpY3;
    int tmpY4;
    int tmpY5;
    int tmpY6;
    int tmpY7;
    int tmpY8;

    int halfRelativeX1;
    int relativeY;
    int xRot1;
    int yRot;
    int halfDispWidth = GUIX_DISPLAY_WIDTH >> 1;

    halfRotaRectWidth = (rotaAlignRect->gx_rectangle_right 
                        - rotaAlignRect->gx_rectangle_left 
                        + 1) >> 1;

    for (int y = alignRect->gx_rectangle_top; y <= alignRect->gx_rectangle_bottom; y += 8)
    {
        xRot1 = (GUIX_DISPLAY_HEIGHT - 1) - y - 6;
        halfRelativeX1 = (xRot1 - rotaAlignRect->gx_rectangle_left) >> 1;

        tmpY8 = y * (halfDispWidth);
        tmpY7 = tmpY8 + halfDispWidth;
        tmpY6 = tmpY7 + halfDispWidth;
        tmpY5 = tmpY6 + halfDispWidth;
        tmpY4 = tmpY5 + halfDispWidth;
        tmpY3 = tmpY4 + halfDispWidth;
        tmpY2 = tmpY3 + halfDispWidth;
        tmpY1 = tmpY2 + halfDispWidth;

        for (int x = alignRect->gx_rectangle_left; x <= alignRect->gx_rectangle_right; x += 8)
        {
            srcByteIdx1 = tmpY1 + (x >> 1);
            srcByteIdx2 = tmpY2 + (x >> 1);
            srcByteIdx3 = tmpY3 + (x >> 1);
            srcByteIdx4 = tmpY4 + (x >> 1);
            srcByteIdx5 = tmpY5 + (x >> 1);
            srcByteIdx6 = tmpY6 + (x >> 1);
            srcByteIdx7 = tmpY7 + (x >> 1);
            srcByteIdx8 = tmpY8 + (x >> 1);

            pixelU32Val1 = *(uint32_t*)&srcBuffer[srcByteIdx1];
            pixelU32Val2 = *(uint32_t*)&srcBuffer[srcByteIdx2];
            pixelU32Val3 = *(uint32_t*)&srcBuffer[srcByteIdx3];
            pixelU32Val4 = *(uint32_t*)&srcBuffer[srcByteIdx4];
            pixelU32Val5 = *(uint32_t*)&srcBuffer[srcByteIdx5];
            pixelU32Val6 = *(uint32_t*)&srcBuffer[srcByteIdx6];
            pixelU32Val7 = *(uint32_t*)&srcBuffer[srcByteIdx7];
            pixelU32Val8 = *(uint32_t*)&srcBuffer[srcByteIdx8];


            //0            
            yRot = x;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 4) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 4) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //1            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1) & 0x0f;
            pixelValue2 = (pixelU32Val2) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3) & 0x0f;
            pixelValue2 = (pixelU32Val4) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5) & 0x0f;
            pixelValue2 = (pixelU32Val6) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7) & 0x0f;
            pixelValue2 = (pixelU32Val8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //2            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 12) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 12) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 12) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 12) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 12) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //3            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 8) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 8) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 8) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //4            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 20) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 20) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 20) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 20) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 20) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //5            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 16) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 16) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 16) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 16) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 16) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //6            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 28) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 28) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 28) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 28) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 28) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

            //7            
            yRot += 1;
            relativeY = yRot - rotaAlignRect->gx_rectangle_top;
            outByteIdx = relativeY * (halfRotaRectWidth) + (halfRelativeX1);
            pixelValue1 = (pixelU32Val1 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val2 >> 24) & 0x0f;
            valueU32 = (pixelValue1 << 4) | pixelValue2;
            pixelValue1 = (pixelU32Val3 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val4 >> 24) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 8;
            pixelValue1 = (pixelU32Val5 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val6 >> 24) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 16;
            pixelValue1 = (pixelU32Val7 >> 24) & 0x0f;
            pixelValue2 = (pixelU32Val8 >> 24) & 0x0f;
            valueU32 |= ((pixelValue1 << 4) | pixelValue2) << 24;
            *(uint32_t *)&outBuffer[outByteIdx] = valueU32;

        }
    }

}

void daricGuiSwitchMode(LCD_Mode mode) 
{
    gCurLcdMode = mode;
}

/* Define driver function pointer for image buffer toggle */
static void daric_4bppgrayscale_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
    GX_RECTANGLE fullRect;
    GX_RECTANGLE ovRect;
    static uint32_t partialCnt = 0;
    static uint32_t first = 0;
    // uint32_t cycnt = 0;
    // uint32_t totalCycnt = 0;
    // uint32_t cpuFreq = DARIC_CGU->cgufsfreq2 * 1000;

    gx_utility_rectangle_define(&fullRect, 0, 0,
                                canvas->gx_canvas_x_resolution - 1,
                                canvas->gx_canvas_y_resolution - 1);

    if (gx_utility_rectangle_overlap_detect(&fullRect, &canvas->gx_canvas_dirty_area, &ovRect))
    {
        LOGV("guix gPreLcdMode=%d, gCurLcdMode=%d", gPreLcdMode, gCurLcdMode);
        if (ovRect.gx_rectangle_left == 0
            && ovRect.gx_rectangle_right == (GUIX_DISPLAY_WIDTH - 1)
            && ovRect.gx_rectangle_top == 0
            && ovRect.gx_rectangle_bottom == (GUIX_DISPLAY_HEIGHT - 1))
        {
            //for(;;)
            {
            switch(gCurLcdMode)
            {
            case GC16:
            case GL16:
                /*
                    When ​gCurLcdMode​ and ​gPreLcdMode​ are not equal, 
                    update ​gPreLcdMode​ to the ​gCurLcdMode​ and invoke ​SwitchMode​ to 
                    switch to the ​gCurLcdMode​.
                 */
                if (gPreLcdMode != gCurLcdMode)
                {
                    gPreLcdMode = gCurLcdMode;
                    BSP_LCD_SwitchMode(gLcdInstance, gCurLcdMode);
                }
                // DWT->CYCCNT = 0;
                rotate4BppGrayscaleBufferV8((uint8_t *)canvas->gx_canvas_memory, gExtrRegionBkg);
                // cycnt = DWT->CYCCNT / cpuFreq;
                // totalCycnt += cycnt;
                // LOGV("guix rotateBuffer ms=%ld", cycnt);
                BSP_LCD_DrawBitmap(gLcdInstance, 0, 0, gExtrRegionBkg);
                break;

            default:
                /*
                    For modes other than GC16 and GL16, 
                    if ​gCurLcdMode​ and ​gPreLcdMode​ are not equal,
                    update gPreLcdMode to match gCurLcdMode,
                    then invoke SwitchMode to switch to DU2. 
                    First perform a full-screen white refresh using DU2, 
                    then switch to gCurLcdMode.
                */
                if (gPreLcdMode != gCurLcdMode)
                {
                    gPreLcdMode = gCurLcdMode;
                    BSP_LCD_SwitchMode(gLcdInstance, DU2);
                    memset(gExtrRegionBkg, 0xff, sizeof(gExtrRegionBkg));
                    BSP_LCD_DrawBitmap(gLcdInstance, 0, 0, gExtrRegionBkg);

                    if (gCurLcdMode != DU2)
                    {
                        BSP_LCD_SwitchMode(gLcdInstance, gCurLcdMode);
                    }
                }

                rotate4BppGrayscaleBufferV8((uint8_t *)canvas->gx_canvas_memory, gExtrRegionBkg);
                BSP_LCD_DrawBitmap(gLcdInstance, 0, 0, gExtrRegionBkg);
            }
            
            #if 0 //for debug only
            //GX_RECTANGLE orgiRect = {5, 3, 476, 293};
            GX_RECTANGLE orgiRect = {0, 0, 476, 597};
            GX_RECTANGLE alignRect;
            GX_RECTANGLE roteRect;

            getRotate4BppAlignPos(&orgiRect, &alignRect, &roteRect);
            DWT->CYCCNT = 0;
            rotate4BppGrayscalePartialAlignBufferV3(gExtrRegionBkg, (uint8_t *)canvas->gx_canvas_memory, &alignRect, &roteRect);
            cycnt = DWT->CYCCNT / cpuFreq;
            //totalCycnt += cycnt;
            LOGV("guix rotatePartial ms=%ld", cycnt);

            LOGV("");
            int roteWidth = roteRect.gx_rectangle_right - roteRect.gx_rectangle_left + 1;
            int roteHeight = roteRect.gx_rectangle_bottom - roteRect.gx_rectangle_top + 1;

            BSP_LCD_FillRGBRect(gLcdInstance, 
                                0,
                                0,
                                //roteRect.gx_rectangle_left, 
                                //roteRect.gx_rectangle_top, 
                                roteWidth, 
                                roteHeight, 
                                gExtrRegionBkg);
                    
            #endif
            }
            LOGV("guix BSP_LCD_DrawBitmap_1 full");
            partialCnt = 0;
        }
        else
        {
            //GX_RECTANGLE orgiRect = {5, 3, 476, 293};
            GX_RECTANGLE orgiRect = ovRect;
            GX_RECTANGLE alignRect;
            GX_RECTANGLE roteRect;
            int roteWidth;
            int roteHeight;

            getRotate4BppAlignPos(&orgiRect, &alignRect, &roteRect);
            roteWidth = roteRect.gx_rectangle_right - roteRect.gx_rectangle_left + 1;
            roteHeight = roteRect.gx_rectangle_bottom - roteRect.gx_rectangle_top + 1;
            
            if (partialCnt < 10)
            {
                switch(gCurLcdMode)
                {
                case GC16:
                case GL16:
                    /*
                        When ​gCurLcdMode​ and ​gPreLcdMode​ are not equal, 
                        update ​gPreLcdMode​ to the ​gCurLcdMode​ and invoke ​SwitchMode​ to 
                        switch to the ​gCurLcdMode​.
                     */
                    if (gPreLcdMode != gCurLcdMode)
                    {
                        gPreLcdMode = gCurLcdMode;
                        BSP_LCD_SwitchMode(gLcdInstance, gCurLcdMode);
                    }
                    break;
                
                case DU2:
                    /*
                        When ​gCurLcdMode​ and ​gPreLcdMode​ are not equal, 
                        update ​gPreLcdMode​ to the ​gCurLcdMode​ and invoke ​SwitchMode​ to 
                        switch to the ​gCurLcdMode​.
                     */
                    if (gPreLcdMode != gCurLcdMode)
                    {
                        gPreLcdMode = gCurLcdMode;
                        BSP_LCD_SwitchMode(gLcdInstance, gCurLcdMode);
                    }
                    break;

                case A2:
                    /*
                        If ​gCurLcdMode​ switches to ​A2, and ​gCurLcdMode​ and ​gPreLcdMode​ are not equal, 
                        update ​gPreLcdMode​ to ​gCurLcdMode, then call ​SwitchMode​ to switch to ​DU2. 
                        First, use ​DU2​ to refresh the region to pure white, 
                        and then switch to ​gCurLcdMode.
                     */
                    if (gPreLcdMode != gCurLcdMode)
                    {
                        gPreLcdMode = gCurLcdMode;
                        BSP_LCD_SwitchMode(gLcdInstance, DU2);
                        memset(gExtrRegionBkg, 0xff, sizeof(gExtrRegionBkg));
                        BSP_LCD_FillRGBRect(gLcdInstance, 
                            roteRect.gx_rectangle_left, 
                            roteRect.gx_rectangle_top, 
                            roteWidth, 
                            roteHeight, 
                            gExtrRegionBkg);
                        BSP_LCD_SwitchMode(gLcdInstance, gCurLcdMode);
                    }
                    break;

                default:
                    gCurLcdMode = GC16;
                    gPreLcdMode = gCurLcdMode;
                    BSP_LCD_SwitchMode(gLcdInstance, gCurLcdMode);
                }

                rotate4BppGrayscalePartialAlignBufferV3(gExtrRegionBkg, (uint8_t *)canvas->gx_canvas_memory, &alignRect, &roteRect);
                BSP_LCD_FillRGBRect(gLcdInstance, 
                                    roteRect.gx_rectangle_left, 
                                    roteRect.gx_rectangle_top, 
                                    roteWidth, 
                                    roteHeight, 
                                    gExtrRegionBkg);
                LOGV("guix BSP_LCD_FillRGBRect partialCnt=%ld", partialCnt);    
                partialCnt++;
            }
            else
            {
                if (gPreLcdMode != GC16)
                {
                    gPreLcdMode = GC16;
                    BSP_LCD_SwitchMode(gLcdInstance, gPreLcdMode);
                }
                rotate4BppGrayscaleBufferV8((uint8_t *)canvas->gx_canvas_memory, gExtrRegionBkg);
                BSP_LCD_DrawBitmap(gLcdInstance, 0, 0, gExtrRegionBkg);

                LOGV("guix BSP_LCD_DrawBitmap2 partialCnt=%ld", partialCnt);
                partialCnt = 0;
            }
            
        }
    }
    if (first == 0)
    {
        first = 1;
        tx_event_flags_set(&threadInitEvent, 1UL, TX_OR);
    }
}

UINT daric_graphics_driver_4bppgrayscale(GX_DISPLAY *display)
{
    _gx_display_driver_4bpp_grayscale_setup(display, NULL, daric_4bppgrayscale_buffer_toggle);

    /* On daric, the following Guix interfaces do not need to be implemented. */
#if 0	
    display->gx_display_driver_pixelmap_blend     = gx_daric_pixelmap_blend;
    display->gx_display_driver_pixelmap_draw      = gx_daric_pixelmap_draw;
    display->gx_display_driver_canvas_copy        = gx_daric_canvas_copy;
    display->gx_display_driver_horizontal_line_draw = gx_daric_horizontal_line_draw;
    display->gx_display_driver_vertical_line_draw = gx_daric_vertical_line_draw;
    display->gx_display_driver_1bit_glyph_draw    = gx_daric_glyph_1bit_draw;
#endif

    return (GX_SUCCESS);
}

/* Callback functions for Guix, 
 * which uses it to allocate and free memory. 
 */
void *guixBuffAllocate(ULONG size)
{
    void *memptr = malloc(size);

    if (memptr)
    {
        return memptr;
    }
    return NULL;
}

/* Callback functions for Guix, 
 * which uses it to allocate and free memory. 
 */
void guixBuffFree(void *mem)
{
    free(mem);
}

/* GUI thread function, 
 * ED028TC1LCD_LCD driver initialization needs to be done here, 
 * because LCD driver initialization is time-consuming, 
 * so use the GUI thread to handle them, 
 * while also starting the GUIX processing thread 
 * */
void guiThreadEntry(ULONG thread_input)
{
    //const char *test_string = "Daric";
    // uint32_t cycnt = 0;
    // uint32_t totalCycnt = 0;
    // uint32_t cpuFreq = DARIC_CGU->cgufsfreq2 * 1000;

    // CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    // DWT->CYCCNT = 0;
    /* Init ED028TC1LCD */
    BSP_LCD_Init(gLcdInstance, 0);
    // cycnt = DWT->CYCCNT / cpuFreq;
    // totalCycnt += cycnt;
    // printf(">>>>>> LCDInit ms=%ld\r\n", cycnt);
    //BSP_LCD_Reset(gLcdInstance);
    //BSP_LCD_Display_String(gLcdInstance, test_string, 0, 0);
    // DWT->CYCCNT = 0;
    gx_system_initialize();
    gx_system_memory_allocator_set(guixBuffAllocate, guixBuffFree);

    daric_gui_display_table[0].canvas_memory = (GX_COLOR *)gGuixCanvasBuff;

    gx_studio_display_configure(MAIN_DISPLAY, daric_graphics_driver_4bppgrayscale,
                                LANGUAGE_ENGLISH, MAIN_DISPLAY_THEME_1, &root);

    start_guix();

    touch_thread_entry(0);

}
