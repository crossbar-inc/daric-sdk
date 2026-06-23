/**
 ******************************************************************************
 * @file    sdma.c
 * @author  SCE Team
 * @brief   SCE SDMA driver implementation.
 *          This file provides firmware functions to manage the SCE SDMA
 *          peripheral for data transfer between memory and SCE RAM.
 *
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
#include <stdio.h>
#include <string.h>
#include "daric.h"
#include "auth.h"
#include "daric_util.h"
#include "bn_util.h"

/**
 * @brief Transfer data by X channel.
 *
 * @param type
 * @param opmode
 * @param opt
 * @param address
 * @param segId
 * @param segId_off
 * @param size
 */
void Xch_Sch_TranData(uint8_t type, uint8_t opmode, uint8_t opt, uint32_t address, uint8_t segId, uint8_t segId_off, uint32_t size)
{
    ASSERT_4BYTE_ALIGNED(address);

    DARIC_RERAM->RRC_FR.rrcFrVal = 0xFF;      // clear RRC error flag register

    /* write : sceram to other   read:  other to sceram */
    SCE_XCH_SCH_SDMA_FUNC(type, opmode);

    /* opt */
    SCE_XCH_SCH_SDMA_OPT_SET(type, opt);

    /* addr */
    SCE_XCH_SCH_SDMA_ADDR(type, address);

    /* segid + segid offset */
    SCE_XCH_SCH_SDMA_SEG_ID(type, segId);
    SCE_XCH_SCH_SDMA_SEG_ADDR(type, segId_off);

    /* size */
    SCE_XCH_SCH_SDMA_SIZE(type, size);

    SCE_SDMA_START(type == SEC_XCH_DMA_TYPE ? XCH_START : SCH_START);

    SCE_SDMA_DONE(type == SEC_XCH_DMA_TYPE ? SCE_SDMA_XCH_DONE : SCE_SDMA_SCH_DONE);
}

/**
 * @brief Transfer data by I channel. Size and offset are in word.
 *
 * @param opt
 * @param segId_r
 * @param segId_r_off
 * @param segId_w
 * @param segId_w_off
 * @param size
 */
void Ich_TranData(uint8_t opt, uint8_t segId_r, uint16_t segId_r_off, uint8_t segId_w, uint16_t segId_w_off, uint16_t size)
{
    SCE_ICH_SDMA_OPT_SET(opt);

    SCE_ICH_SDMA_READ_SEG_ID(segId_r);
    SCE_ICH_SDMA_READ_SEG_ADDR(segId_r_off);

    SCE_ICH_SDMA_WRITE_SEG_ID(segId_w);
    SCE_ICH_SDMA_WRITE_SEG_ADDR(segId_w_off);

    SCE_ICH_SDMA_SIZE(size);

    SCE_SDMA_START(ICH_START);

    SCE_SDMA_DONE(SCE_SDMA_ICH_DONE);
}

/**
 * @brief Copy data from src to dest. Size is in bytes.
 *
 * @param dest
 * @param src
 * @param n
 */
void memcpy_u32(void *dest, const void *src, size_t n)
{
    /* convert source and destination addresses to pointers, aligned by 32 bits */
    uint32_t       *d = (uint32_t *)dest;
    const uint32_t *s = (const uint32_t *)src;

    /* copy in units of 32 bits (4 bytes) */
    for (size_t i = 0; i < n / 4; ++i)
    {
        d[i] = s[i];
    }

    /* handle remaining bytes if n is not a multiple of 4 */
    uint8_t       *d_byte = (uint8_t *)d + (n / 4) * 4;
    const uint8_t *s_byte = (const uint8_t *)s + (n / 4) * 4;
    for (size_t i = 0; i < n % 4; ++i)
    {
        d_byte[i] = s_byte[i];
    }
}
