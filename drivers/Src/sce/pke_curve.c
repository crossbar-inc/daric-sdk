/**
 ******************************************************************************
 * @file    pke_curve.c
 * @author  SCE Team
 * @brief   PKE (Public Key Engine) curve driver implementation.
 *          This file provides firmware functions to manage the PKE
 *          peripheral for ECC and EdDSA operations.
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
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "daric.h"
#include "auth.h"
#include "daric_hal_atimer.h"
#include "sdma.h"
#include "hash.h"
#include "pke_curve.h"
#include "pke_rsa.h"
#include "bn_util.h"
#include "alu.h"
#include <assert.h>

/**
 * @brief Big-endian byte stream formatting 0x 11 22 33 44 55 ==> 0x 55 44 33 22 11
 *
 * @param inData Input data
 * @param outData Output data
 * @param len Length of input data
 * @param expectLen Expected length of output data
 * @return uint32_t Expected length of output data
 */
uint32_t pke_dataformat(const uint8_t *inData, uint8_t *outData, uint32_t len, uint32_t expectLen)
{
    uint32_t i, initLen;

    if ((expectLen % 4) != 0 || len > expectLen)
    {
        return 0;
    }

    for (i = 0; i < (len / 2); i++)
    {
        outData[i]             = inData[i] + inData[len - (i + 1)];
        outData[len - (i + 1)] = outData[i] - inData[len - (i + 1)];
        outData[i] -= outData[len - (i + 1)];
    }
    if (len % 2)
        outData[i] = inData[i];

    /* Input is not a multiple of word */
    if (len % 4)
    {
        for (i = 0; i < (4 - len % 4); i++)
        {
            outData[len + i] = 0;
        }
        initLen = len + (4 - len % 4);
    }
    else
    {
        initLen = len;
    }

    if (initLen != expectLen && expectLen != 0)
    {
        for (i = 0; i < (expectLen - initLen); i++)
        {
            outData[initLen + i] = 0;
        }
    }

    return expectLen;
}

/**
 * @brief Count the number of one bits in the input data
 *
 * @param inData Input data
 * @param inLen Length of input data
 * @param mode Mode of input data
 * @return uint16_t Number of one bits
 */
uint16_t count_one_bits(const uint8_t *inData, uint32_t inLen, uint8_t mode)
{
    uint32_t j, i;
    uint8_t  data;

    if (mode == littleEnd) /* Calculate K's bitLen, input data is K's little-endian BYTE data stream */
    {
        for (j = 0; j < inLen; j++)
        {
            data = inData[inLen - (j + 1)]; /* Little-endian mode takes the last byte, which is the highest byte of the big integer */

            if (data != 0)
            {
                for (i = 7; i > 0; i--)
                {
                    if (data & (1 << i))
                    {
                        break;
                    }
                }
                break;
            }
        }
    }
    else /* Calculate K's bitLen, input data is K's big-endian BYTE data stream */
    {
        for (j = 0; j < inLen; j++)
        {
            data = inData[j]; /* Big-endian mode takes the first byte, which is the highest byte of the big integer */

            if (data != 0)
            {
                for (i = 7; i > 0; i--)
                {
                    if (data & (1 << i))
                    {
                        break;
                    }
                }
                break;
            }
        }
    }
    if (j >= inLen)
        return 0; /* all bytes are zero */
    return (i + 1) + (inLen - (j + 1)) * 8;
}

/**
 * @brief Initialize PKE system
 *
 */
void sce_Sysinit_Pke(void)
{
    SET_SECMODE_NORMAL();
    SCE_SDMA_CLK_EN();
    SCE_PKE_CLK_EN();

    SCE_CLR_RESET_RAM();  /* This reset action will clear all scectrl configurations */
    SCE_RAM_CLEAR_DONE(); /* Clear RAM zeroing completion flag */

    /* Reconfigure */
    SET_SECMODE_NORMAL();
    SCE_SDMA_CLK_EN();
    SCE_PKE_CLK_EN();

    SCE_CLR_RAM();        /* Clear RAM data */
    SCE_RAM_CLEAR_DONE(); /* Clear RAM zeroing completion flag */
}

/********************************************************************ED MOUDLE*****************************************************************************/
const uint32_t ED25519_P_D[] = { 0xffffffed, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff,
                                 0x135978a3, 0x75eb4dca, 0x4141d8ab, 0x00700a4d, 0x7779e898, 0x8cc74079, 0x2b6ffe73, 0x52036cee };

//(15112221349535400772501151409588531511454012693041857206046113283949847762202,46316835694926478169428394003475163141307993866256225615783033603165251855960)
//(0x216936D3CD6E53FEC0A4E231FDD6DC5C692CC7609525A7B2C9562D608F25D51A,0x6666666666666666666666666666666666666666666666666666666666666658)
const uint32_t ED25519_BASE_POINT[] = { 0x8f25d51a, 0xc9562d60, 0x9525a7b2, 0x692cc760, 0xfdd6dc5c, 0xc0a4e231, 0xcd6e53fe, 0x216936d3,
                                        0x66666658, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666 };

const uint32_t ED25519_L[] = { 0x5cf5d3ed, 0x5812631a, 0xa2f79cd6, 0x14def9de, 0x00000000, 0x00000000, 0x00000000, 0x10000000 };

void pke_calc_H(uint32_t *outBuf, uint32_t bitLen)
{
    memset(outBuf, 0, bitLen / 8 + 2);
#ifdef PKE_LITTLE
    i = 1 << (bitLen % (8 * 4));
    i = ((i << 8) & 0xFF00FF00) | ((i >> 8) & 0xFF00FF); // The chip is in little-endian mode, outbuf[0] needs 0x11223344 <==> 0x44332211

    if (bitLen % 64 != 0)
    {
        outBuf[0] = 0;
        outBuf[1] = (i << 16) | (i >> 16);
    }
    else
    {
        outBuf[0] = (i << 16) | (i >> 16);
    }

#else
    outBuf[bitLen / 8 / 4] = 1 << (bitLen % (8 * 4));
#endif
}

/**
 * @brief PKE parameter initialization
 *
 * @param p
 * @param pLen word number
 * @param d
 * @param dLen word number
 * @param modLen
 */
void pke_init(const uint32_t *p, uint32_t pLen, const uint32_t *d, uint32_t dLen, uint32_t modLen)
{
    uint32_t bitLen;

    ASSERT_4BYTE_ALIGNED(p);
    ASSERT_4BYTE_ALIGNED(d);
    assert(modLen <= CURVE_521);
    assert(pLen < 128);
    assert(dLen < 128);

    bitLen = count_one_bits((const uint8_t *)p, pLen * 4, littleEnd);
    SCE_PKE_OPTNW(bitLen); /* N's effective BIT position, this value does not need to be changed for fixed curves */
    SCE_PKE_OPTEW(modLen); /* K's effective BIT position, which needs to be calculated according to different K values */
    memset((uint8_t *)ADDR_PKE_SEG_PCON, 0, (pLen * 4) + (dLen * 4) + bitLen / 8 + 4); /* Add one world to zero */
#ifdef PKE_LITTLE
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PCON | REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    if (pEndian == littleEnd)
    {
        pke_dataformat(p, tempBuf, pLen, pLen < 32 ? 32 : pLen);
    }
    else
    {
        memcpy(tempBuf, p, pLen);
    }
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PCON, tempBuf, pLen);

    if (dEndian == littleEnd)
    {
        pke_dataformat(d, tempBuf, dLen, dLen < 32 ? 32 : dLen);
    }
    else
    {
        memcpy(tempBuf, d, pLen);
    }

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PCON + pLen, tempBuf, dLen);
#else
    SCE_PKE_OPTLTX_MODE(0);
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PCON, p, pLen);
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PCON + pLen, d, dLen);
#endif

    pke_calc_H((uint32_t *)(ADDR_PKE_SEG_PCON + pLen + dLen), bitLen);

#ifdef PKE_DEBUG
    bnu_print_mem("p: ", p, pLen * 4);
    bnu_print_mem("d: ", d, pLen * 4);
    bnu_print_mem("pcon", (uint32_t *)ADDR_PKE_SEG_PCON, 30 * 4);
#endif
    SCE_PKE_SET_PCON(0);

    SCE_PKE_FUNC(PIR_ECINIT);
    SCE_PKE_AR();
    SCE_PKE_DONE();
}

/* modLen needs to pass the correct effective BIT position for point multiplication */
void ed25519_init(uint16_t modLen)
{
    pke_init(ED25519_P_D, 8, ED25519_P_D + 8, 8, modLen);
}

/**
 * @brief pointOut = pointA + pointB
 *
 * @param pointA ed point
 * @param pointB ed point
 * @param[out] pointOut output
 */
void ed25519_PointAdd(const pointDef *pointA, const pointDef *pointB, pointDef *pointOut)
{
    uint8_t A_x[32], A_y[32];
    uint8_t B_x[32], B_y[32];

#ifdef PKE_LITTLE
    if (aEndian == littleEnd)
    {
        pke_dataformat(pointA->x, A_x, 32, 32);
        pke_dataformat(pointA->y, A_y, 32, 32);
    }
    else
    {
        memcpy(A_x, pointA->x, 32);
        memcpy(A_y, pointA->y, 32);
    }

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)A_x, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + 32, (uint8_t *)A_y, 32);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
#else

    /* TODO: why separate A_x and A_y and do memcpy twice ?*/
    memcpy(A_x, pointA->x, 32);
    memcpy(A_y, pointA->y, 32);

    SCE_PKE_OPTLTX_MODE(0);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)A_x, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + 32, (uint8_t *)A_y, 32);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("pointA_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
    bnu_print_mem("pointA_y", (uint8_t *)ADDR_PKE_SEG_PIB + 32, 32);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointA(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 32, 32); /* Save converted data */

    ed25519_init(0);

#ifdef PKE_LITTLE
    if (bEndian == littleEnd)
    {
        pke_dataformat(pointB->x, B_x, 32, 32);
        pke_dataformat(pointB->y, B_y, 32, 32);
    }
    else
    {
        memcpy(B_x, pointB->x, 32);
        memcpy(B_y, pointB->y, 32);
    }

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)B_x, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + 32, (uint8_t *)B_y, 32);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
#else
    memcpy(B_x, pointB->x, 32);
    memcpy(B_y, pointB->y, 32);

    SCE_PKE_OPTLTX_MODE(0);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)B_x, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + 32, (uint8_t *)B_y, 32);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("pointB_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
    bnu_print_mem("pointB_y", (uint8_t *)ADDR_PKE_SEG_PIB + 32, 32);
#endif
    SCE_PKE_SET_PIB0(0);

    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointB(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1(32);

    SCE_PKE_FUNC(PIR_EDPA);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point_Result(EDPA)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_FUNC(PIR_EDM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
    bnu_print_mem("pointResult_y(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB + 32, 32);
#endif

    if (pointOut != NULL)
    {
#ifdef PKE_LITTLE /* Output (the chip's default output is the big-endian mode of the big integer) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);

        if (mode == littleEnd) /* Big integer little-endian mode output */
        {
            pke_dataformat(pointOut->x, pointOut->x, 32, 32);
            pke_dataformat(pointOut->y, pointOut->y, 32, 32);
        }
#else /* Output (the chip's default output is the big-endian mode of the big integer) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);
#endif
    }
}

/**
 * @brief pointOut = 2 * pointIn
 *
 * @param pointIn input point
 * @param[out] pointOut output point
 */
void ed25519_PointDouble(const pointDef *pointIn, pointDef *pointOut)
{
    ASSERT_4BYTE_ALIGNED(pointIn);
    ASSERT_4BYTE_ALIGNED(pointOut);

#ifdef PKE_LITTLE
    if (inEndian == littleEnd)
    {
        pke_dataformat(pointIn->x, x, 32, 32);
        pke_dataformat(pointIn->y, y, 32, 32);
    }
    else
    {
        memcpy(x, pointIn->x, 32);
        memcpy(y, pointIn->y, 32);
    }

    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)x, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + 32, (uint8_t *)y, 32);
#else
    SCE_PKE_OPTLTX_MODE(0);

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)pointIn, 64);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
    bnu_print_mem("point_y", (uint8_t *)ADDR_PKE_SEG_PIB + 32, 32);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_FUNC(PIR_EDPD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(EDPD)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_FUNC(PIR_EDM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
    bnu_print_mem("pointResult_y(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB + 32, 32);
#endif

    if (pointOut != NULL)
    {
#ifdef PKE_LITTLE /* Output (the chip's default output is the big-endian mode of the big integer) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);

        if (mode == littleEnd) /* Big integer little-endian mode output */
        {
            pke_dataformat(pointOut->x, pointOut->x, 32, 32);
            pke_dataformat(pointOut->y, pointOut->y, 32, 32);
        }
#else /* Output (the chip's default output is the big-endian mode of the big integer) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);
#endif
    }
}

/**
 * @brief pointOut = k * pointIn
 *
 * @param k 32 bytes number
 * @param pointIn ED point
 * @param pointOut output ED point
 */
void ed25519_PointMul(const uint8_t *k, const pointDef *pointIn, pointDef *pointOut)
{
    ASSERT_4BYTE_ALIGNED(k);
    ASSERT_4BYTE_ALIGNED(pointIn);
    ASSERT_4BYTE_ALIGNED(pointOut);

#ifdef PKE_LITTLE

    if (kEndian == littleEnd)
    {
        pke_dataformat(k, kTemp, 32, 32);
    }
    else
    {
        memcpy(kTemp, k, 32);
    }

    if (inEndian == littleEnd)
    {
        pke_dataformat(pointIn->x, x, 32, 32);
        pke_dataformat(pointIn->y, y, 32, 32);
    }
    else
    {
        memcpy(x, pointIn->x, 32);
        memcpy(y, pointIn->y, 32);
    }

    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)kTemp, kLen);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, x, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + 32, y, 32);
#else
    SCE_PKE_OPTLTX_MODE(0);

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)k, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, pointIn, 64);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("k", (uint8_t *)ADDR_PKE_SEG_PKB, 32);
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
    bnu_print_mem("point_y", (uint8_t *)ADDR_PKE_SEG_PIB + 32, 32);
#endif
    SCE_PKE_SET_PKB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);

    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point_x(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 64);
    bnu_print_mem("point_y(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB + 64, 64);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_FUNC(PIR_EDPD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point_x(EDPD)", (uint8_t *)ADDR_PKE_SEG_POB, 64);
    bnu_print_mem("point_y(EDPD)", (uint8_t *)ADDR_PKE_SEG_POB + 64, 64);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 32, 32);

    SCE_PKE_SET_PIB1(32);
    SCE_PKE_FUNC(PIR_EDPM);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point_x(EDPM)", (uint8_t *)ADDR_PKE_SEG_POB, 64);
    bnu_print_mem("point_y(EDPM)", (uint8_t *)ADDR_PKE_SEG_POB + 64, 64);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_FUNC(PIR_EDM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
    bnu_print_mem("pointResult_y(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB + 32, 32);
#endif

    if (pointOut != NULL)
    {
#ifdef PKE_LITTLE /* Output (the chip's default output is the big-endian mode of the big integer) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);

        if (mode == littleEnd) /* Big integer little-endian mode output */
        {
            pke_dataformat(pointOut->x, pointOut->x, 32, 32);
            pke_dataformat(pointOut->y, pointOut->y, 32, 32);
        }
#else /* Output (the chip's default output is the big-endian mode of the big integer) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);
#endif
    }
}

/**
 * @brief pointOut = in * ED25519_BASE_POINT
 *
 * @param in little endian 32 bytes number
 * @param inLen
 * @param inEndian bigEnd or littleEnd
 * @param pointOut output ED point
 * @param mode 0--output little endian 1--output big endian
 */
void ed25519_PointMul_Base(const uint8_t *in, uint32_t inLen, uint8_t inEndian, pointDef *pointOut, uint8_t mode)
{
    uint8_t kTemp[32];

    if (inEndian == bigEnd)
    {
        pke_dataformat(in, kTemp, 32, 32);
    }
    else
    {
        memcpy(kTemp, in, 32);
    }

    SCE_PKE_OPTLTX_MODE(0);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)kTemp, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)ED25519_BASE_POINT, sizeof(ED25519_BASE_POINT));

#ifdef PKE_DEBUG
    bnu_print_mem("k", (uint8_t *)ADDR_PKE_SEG_PKB, 32);
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
    bnu_print_mem("point_y", (uint8_t *)ADDR_PKE_SEG_PIB + 32, 32);
#endif

    SCE_PKE_SET_PKB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_FUNC(PIR_EDPD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(EDPD)", (uint8_t *)ADDR_PKE_SEG_POB, 64);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 32, 32);

    SCE_PKE_SET_PIB1(32);
    SCE_PKE_FUNC(PIR_EDPM);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(EDPM)", (uint8_t *)ADDR_PKE_SEG_POB, 128);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);
    SCE_PKE_FUNC(PIR_EDM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
    bnu_print_mem("pointResult_y(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB + 32, 32);
#endif

    if (pointOut != NULL)
    {
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, pointOut->x, 8);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + 8, pointOut->y, 8);

        if (mode == bigEnd) // big endian output
        {
            pke_dataformat(pointOut->x, pointOut->x, 32, 32);
            pke_dataformat(pointOut->y, pointOut->y, 32, 32);
        }
    }
}

/* Curve-ID: secp192k1 */
static const uint32_t SECP192K1_P_A[]
    = { 0xFFFFEE37, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static const uint32_t SECP192K1_G[]
    = { 0xEAE06C7D, 0x1DA5D1B1, 0x80B7F434, 0x26B07D02, 0xC057E9AE, 0xDB4FF10E, 0xD95E2F9D, 0x4082AA88, 0x15BE8634, 0x844163D0, 0x9C5628A7, 0x9B2F2F6D };

/* Curve-ID: secp192r1 */
static const uint32_t SECP192R1_P_A[]
    = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static const uint32_t SECP192R1_G[]
    = { 0x82FF1012, 0xF4FF0AFD, 0x43A18800, 0x7CBF20EB, 0xB03090F6, 0x188DA80E, 0x1E794811, 0x73F977A1, 0x6B24CDD5, 0x631011ED, 0xFFC8DA78, 0x07192B95 };

/* Curve-ID: secp224k1 */
static const uint32_t SECP224K1_P_A[] = { 0xFFFFE56D, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
                                          0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static const uint32_t SECP224K1_G[]
    = { 0xB6B7A45C, 0x0F7E650E, 0xE47075A9, 0x69A467E9, 0x30FC28A1, 0x4DF099DF, 0xA1455B33, 0x556D61A5, 0xE2CA4BDB, 0xC0B0BD59, 0xF7E319F7, 0x82CAFBD6, 0x7FBA3442, 0x7E089FED };

/* Curve-ID: secp224r1 */
static const uint32_t SECP224R1_P_A[] = { 0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
                                          0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
static const uint32_t SECP224R1_G[]
    = { 0x115C1D21, 0x343280D6, 0x56C21122, 0x4A03C1D3, 0x321390B9, 0x6BB4BF7F, 0xB70E0CBD, 0x85007E34, 0x44D58199, 0x5A074764, 0xCD4375A0, 0x4C22DFE6, 0xB5F723FB, 0xBD376388 };

/* Curve-ID: secp256k1 */
static const uint32_t SECP256K1_P_A[] = { 0xFFFFFC2F, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
                                          0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static const uint32_t SECP256K1_G[]   = { 0x16F81798, 0x59F2815B, 0x2DCE28D9, 0x029BFCDB, 0xCE870B07, 0x55A06295, 0xF9DCBBAC, 0x79BE667E,
                                          0xFB10D4B8, 0x9C47D08F, 0xA6855419, 0xFD17B448, 0x0E1108A8, 0x5DA4FBFC, 0x26A3C465, 0x483ADA77 };

/* Curve-ID: secp256r1 */
static const uint32_t SECP256R1_P_A[] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF,
                                          0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF };
static const uint32_t SECP256R1_G[]   = { 0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81, 0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2,
                                          0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357, 0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2 };

/* Curve-ID: secp384r1 */
static const uint32_t SECP384R1_P_A[]
    = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFC, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static const uint32_t SECP384R1_G[]
    = { 0x72760AB7, 0x3A545E38, 0xBF55296C, 0x5502F25D, 0x82542A38, 0x59F741E0, 0x8BA79B98, 0x6E1D3B62, 0xF320AD74, 0x8EB1C71E, 0xBE8B0537, 0xAA87CA22,
        0x90EA0E5F, 0x7A431D7C, 0x1D7E819D, 0x0A60B1CE, 0xB5F0B8C0, 0xE9DA3113, 0x289A147C, 0xF8F41DBD, 0x9292DC29, 0x5D9E98BF, 0x96262C6F, 0x3617DE4A };

/* Curve-ID: secp521r1 */
static const uint32_t SECP521R1_P_A[]
    = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF, 0x00000000, 0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF, 0x00000000 };
static const uint32_t SECP521R1_G[]
    = { 0xc2e5bd66, 0xf97e7e31, 0x856a429b, 0x3348b3c1, 0xa2ffa8de, 0xfe1dc127, 0xefe75928, 0xa14b5e77, 0x6b4d3dba, 0xf828af60, 0x053fb521, 0x9c648139,
        0x2395b442, 0x9e3ecb66, 0x0404e9cd, 0x858e06b7, 0x000000c6, 0x9fd16650, 0x88be9476, 0xa272c240, 0x353c7086, 0x3fad0761, 0xc550b901, 0x5ef42640,
        0x97ee7299, 0x273e662c, 0x17afbd17, 0x579b4468, 0x98f54449, 0x2c7d1bd9, 0x5c8a5fb4, 0x9a3bc004, 0x39296a78, 0x00000118 };

const uint32_t *ec_basePonitPtr;
bool            ecc_init(uint16_t modLen, uint8_t curveType)
{
    switch (curveType)
    {
        case SECP192K1:
            pke_init(SECP192K1_P_A, CURVE_192 / 32, (SECP192K1_P_A) + (CURVE_192 / 32), CURVE_192 / 32, modLen);
            ec_basePonitPtr = SECP192K1_G;
            break;

        case SECP192R1:
            pke_init(SECP192R1_P_A, CURVE_192 / 32, (SECP192R1_P_A) + (CURVE_192 / 32), CURVE_192 / 32, modLen);
            ec_basePonitPtr = SECP192R1_G;
            break;

        case SECP224K1: /* The algorithm core takes data in U64 format, so a U32 needs to be padded here */
            pke_init(SECP224K1_P_A, CURVE_224 / 32 + 1, (SECP224K1_P_A) + (CURVE_224 / 32) + 1, CURVE_224 / 32 + 1, modLen);
            ec_basePonitPtr = SECP224K1_G;
            break;

        case SECP224R1: /* The algorithm core takes data in U64 format, so a U32 needs to be padded here */
            pke_init(SECP224R1_P_A, CURVE_224 / 32 + 1, (SECP224R1_P_A) + (CURVE_224 / 32) + 1, CURVE_224 / 32 + 1, modLen);
            ec_basePonitPtr = SECP224R1_G;
            break;

        case SECP256K1:
            pke_init(SECP256K1_P_A, CURVE_256 / 32, (SECP256K1_P_A) + (CURVE_256 / 32), CURVE_256 / 32, modLen);
            ec_basePonitPtr = SECP256K1_G;
            break;

        case SECP256R1:
            pke_init(SECP256R1_P_A, CURVE_256 / 32, (SECP256R1_P_A) + (CURVE_256 / 32), CURVE_256 / 32, modLen);
            ec_basePonitPtr = SECP256R1_G;
            break;

        case SECP384R1:
            pke_init(SECP384R1_P_A, CURVE_384 / 32, (SECP384R1_P_A) + (CURVE_384 / 32), CURVE_384 / 32, modLen);
            ec_basePonitPtr = SECP384R1_G;
            break;

        case SECP521R1:
            pke_init(SECP521R1_P_A, 18, (SECP521R1_P_A) + 18, 18, modLen);
            /* Patch program, due to the existing SCE kernel exception causing PT value conflict, rewrite the PT value here. */
#ifndef NEW_VERSION
            memcpy_u32((void *)0x40025120, SECP521R1_P_A, sizeof(SECP521R1_P_A) / 2);
#endif

            ec_basePonitPtr = SECP521R1_G;
            break;
        default:
            return false;
    }
    return true;
}

/**
 * @brief pointOut = pointA + pointB.
 *
 * @note pointA and pointB must be distinct points (pointA != pointB).
 *       For the point doubling case (pointA == pointB), use ecc_PointDouble() instead.
 *       Passing equal points to this function triggers an assertion failure.
 *
 * @param pointA input ec point A
 * @param pointB input ec point B
 * @param pointOut output ec point
 * @param curveBitLen value = 192, 224, 256, 384, 521
 */
void ecc_PointAdd(const uint32_t *pointA, const uint32_t *pointB, uint32_t *pointOut, uint16_t curveBitLen)
{
    uint8_t  tempBuf[144]; /* max 576bit *2 */
    uint8_t  padByteNum;
    uint32_t curve_len, pad_len;

    ASSERT_4BYTE_ALIGNED(pointA);
    ASSERT_4BYTE_ALIGNED(pointB);
    ASSERT_4BYTE_ALIGNED(pointOut);
    assert(curveBitLen == CURVE_192 || curveBitLen == CURVE_224 || curveBitLen == CURVE_256 || curveBitLen == CURVE_384 || curveBitLen == CURVE_521);

    memset(tempBuf, 0, sizeof(tempBuf));

    /* 64 bit alignment */
    if (curveBitLen == CURVE_224 || curveBitLen == CURVE_521)
    {
        padByteNum = 4;
    }
    else
    {
        padByteNum = 0;
    }

    curve_len = curveBitLen / 8;
    pad_len   = curve_len + padByteNum;

    assert(memcmp(pointA, pointB, curve_len * 2u) != 0); /* P1==P2 not supported; use ecc_PointDouble() instead */

#ifdef PKE_DEBUG
    printf("padByteNum = %02x curve_len = %" PRIx32 " pad_len = %" PRIx32 "\r\n", padByteNum, curve_len, pad_len);
#endif

#ifdef PKE_LITTLE
    if (aEndian == littleEnd)
    {
        pke_dataformat((uint8_t *)pointA, tempBuf + padByteNum, curve_len, curve_len);
        pke_dataformat((uint8_t *)pointA + curve_len, tempBuf + padByteNum * 2 + curve_len, curve_len, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)tempBuf, curveBitLen * 2 / 8 + padByteNum * 2);
    }
    else
    {
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum, (uint8_t *)pointA, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum * 2 + curve_len, (uint8_t *)pointA + curve_len, curve_len);
    }
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
#else
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)pointA, curve_len);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + pad_len, (uint8_t *)pointA + curve_len, curve_len);
    SCE_PKE_OPTLTX_MODE(0);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("pointA_x", (uint8_t *)ADDR_PKE_SEG_PIB, pad_len);
    bnu_print_mem("pointA_y", (uint8_t *)ADDR_PKE_SEG_PIB + pad_len, pad_len);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_ECI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointA(ECI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, (pad_len) * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, (pad_len) * 3 / 4, (pad_len) * 3 / 4); /* save converted data */

    memset(tempBuf, 0, sizeof(tempBuf));
#ifdef PKE_LITTLE
    if (bEndian == littleEnd)
    {
        pke_dataformat((uint8_t *)pointB, tempBuf + padByteNum, curve_len, curve_len);
        pke_dataformat((uint8_t *)pointB + curve_len, tempBuf + padByteNum * 2 + curve_len, curve_len, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)tempBuf, curveBitLen * 2 / 8 + padByteNum * 2);
    }
    else
    {
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum, (uint8_t *)pointB, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum * 2 + curve_len, (uint8_t *)pointB + curve_len, curve_len);
    }
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
#else
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)pointB, curve_len);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + pad_len, (uint8_t *)pointB + curve_len, curve_len);

    SCE_PKE_OPTLTX_MODE(0);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("pointB_x", (uint8_t *)ADDR_PKE_SEG_PIB, pad_len);
    bnu_print_mem("pointB_y", (uint8_t *)ADDR_PKE_SEG_PIB + pad_len, pad_len);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_FUNC(PIR_ECI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointB(ECI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, (pad_len) * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, (pad_len) * 3 / 4);

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1((pad_len) * 3 / 4);

    SCE_PKE_FUNC(PIR_ECPA);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(ECPA)", (uint8_t *)ADDR_PKE_SEG_POB, (pad_len) * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, (pad_len) * 3 / 4);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_FUNC(PIR_ECM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(ECM2I)", (uint8_t *)ADDR_PKE_SEG_POB, pad_len);
    bnu_print_mem("pointResult_y(ECM2I)", (uint8_t *)ADDR_PKE_SEG_POB + pad_len, pad_len);
#endif

    if (pointOut != NULL)
    {
#ifdef PKE_LITTLE              /* Output (the chip's default output is big-endian mode) */
        if (mode == littleEnd) /* big endian output */
        {
            pke_dataformat((uint8_t *)ADDR_PKE_SEG_POB + padByteNum, (uint8_t *)pointOut, curve_len, curve_len);
            pke_dataformat((uint8_t *)ADDR_PKE_SEG_POB + pad_len * 2, (uint8_t *)pointOut + curve_len, curve_len, curve_len);
        }
        else
        {
            SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (padByteNum / 4), (uint8_t *)pointOut, curve_len / 4);
            SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (padByteNum * 2 + curve_len) / 4, (uint8_t *)pointOut + curve_len, curve_len / 4);
        }
#else /* Output (the chip's default output is big-endian mode) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, (uint8_t *)pointOut, curve_len / 4);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (pad_len) / 4, (uint8_t *)pointOut + curve_len, curve_len / 4);
#endif
    }
}

/**
 * @brief pointOut = 2 * pointIn
 *
 * @param pointIn input point
 * @param pointOut output point
 * @param[out] curveBitLen bit length of the curve, must be CURVE_192, CURVE_224, CURVE_256, CURVE_384 or CURVE_521
 */
void ecc_PointDouble(const uint32_t *pointIn, uint32_t *pointOut, uint16_t curveBitLen)
{
    uint8_t padByteNum;
    uint8_t curve_len, pad_len;

    ASSERT_4BYTE_ALIGNED(pointIn);
    ASSERT_4BYTE_ALIGNED(pointOut);
    assert(curveBitLen == CURVE_192 || curveBitLen == CURVE_224 || curveBitLen == CURVE_256 || curveBitLen == CURVE_384 || curveBitLen == CURVE_521);

    /* 64 bit alignment */
    if (curveBitLen == CURVE_224 || curveBitLen == CURVE_521)
    {
        padByteNum = 4;
    }
    else
    {
        padByteNum = 0;
    }

    curve_len = curveBitLen / 8;
    pad_len   = curve_len + padByteNum;

#ifdef PKE_DEBUG
    printf("padByteNum = %02x curve_len = %02x pad_len = %02x\r\n", padByteNum, curve_len, pad_len);
#endif

#ifdef PKE_LITTLE
    if (inEndian == littleEnd)
    {
        pke_dataformat((uint8_t *)pointIn, tempBuf + padByteNum, curve_len, curve_len);
        pke_dataformat((uint8_t *)pointIn + curve_len, tempBuf + pad_len * 2, curve_len, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)tempBuf, curveBitLen * 2 / 8 + padByteNum * 2);
    }
    else
    {
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum, (uint8_t *)pointIn, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum * 2 + curve_len, (uint8_t *)pointIn + curve_len, curve_len);
    }
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
#else
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)pointIn, curve_len);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + pad_len, (uint8_t *)pointIn + curve_len, curve_len);
    SCE_PKE_OPTLTX_MODE(0);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, pad_len);
    bnu_print_mem("point_y", (uint8_t *)ADDR_PKE_SEG_PIB + pad_len, pad_len);
#endif
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_ECI2MD);

    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point(ECI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, pad_len * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, pad_len * 3 / 4);

    SCE_PKE_FUNC(PIR_ECPD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(ECPD)", (uint8_t *)ADDR_PKE_SEG_POB, pad_len * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, pad_len * 3 / 4);

#ifdef ALG_TEST_MODE
    return; /* Concurrent test, not calculated here */
#endif
    SCE_PKE_FUNC(PIR_ECM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(ECM2I)", (uint8_t *)ADDR_PKE_SEG_POB, pad_len);
    bnu_print_mem("pointResult_y(ECM2I)", (uint8_t *)ADDR_PKE_SEG_POB + pad_len, pad_len);
#endif

    if (pointOut != NULL)
    {
#ifdef PKE_LITTLE              /* Output (the chip's default output is big-endian mode) */
        if (mode == littleEnd) /* big endian output */
        {
            pke_dataformat((uint8_t *)ADDR_PKE_SEG_POB + padByteNum, (uint8_t *)pointOut, curve_len, curve_len);
            pke_dataformat((uint8_t *)ADDR_PKE_SEG_POB + pad_len * 2, (uint8_t *)pointOut + curve_len, curve_len, curve_len);
        }
        else
        {
            SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (padByteNum / 4), (uint8_t *)pointOut, curve_len / 4);
            SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (padByteNum * 2 + curve_len) / 4, (uint8_t *)pointOut + curve_len, curve_len / 4);
        }
#else /* Output (the chip's default output is big-endian mode) */
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, (uint8_t *)pointOut, curve_len / 4);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + pad_len / 4, (uint8_t *)pointOut + curve_len, curve_len / 4);
#endif
    }
}

/**
 * @brief pointOut = k * pointIn. length of k must be equal to curveBitLen / 8.
 *
 * @note k must be >= 2. The hardware ECPM instruction uses OPTEW = bit-length of k to
 *       control its Montgomery-ladder loop count; when k=1 (OPTEW=1) the loop executes
 *       zero iterations and the output is garbage. Callers must handle k=1 explicitly
 *       (i.e., return pointIn unchanged) before invoking this function.
 *       Passing k=1 triggers an assertion failure.
 *
 * @param k scalar multiplier. length of k must be equal to curveBitLen / 8.
 * @param pointIn input ec point
 * @param pointOut output ec point
 * @param curveBitLen bit length of the curve, must be CURVE_192, CURVE_224, CURVE_256, CURVE_384 or CURVE_521
 */
void ecc_PointMul(const uint8_t *k, const uint32_t *pointIn, uint32_t *pointOut, uint16_t curveBitLen)
{
    uint16_t bitLen;
    uint8_t  tempBuf[144]; // max 576bit *2
    uint8_t  padByteNum;
    uint32_t curve_len, pad_len;

    ASSERT_4BYTE_ALIGNED(k);
    ASSERT_4BYTE_ALIGNED(pointIn);
    ASSERT_4BYTE_ALIGNED(pointOut);
    assert(curveBitLen == CURVE_192 || curveBitLen == CURVE_224 || curveBitLen == CURVE_256 || curveBitLen == CURVE_384 || curveBitLen == CURVE_521);

    memset(tempBuf, 0, sizeof(tempBuf));

    /* 64 bit alignment */
    if (curveBitLen == CURVE_224 || curveBitLen == CURVE_521)
    {
        padByteNum = 4;
    }
    else
    {
        padByteNum = 0;
    }

    curve_len = curveBitLen / 8;
    pad_len   = curve_len + padByteNum;

#ifdef PKE_DEBUG
    printf("padByteNum = %02x curve_len = %02" PRIx32 " pad_len = %02" PRIx32 "\r\n", padByteNum, curve_len, pad_len);
#endif

    memcpy(&tempBuf[144 / 2], (uint8_t *)k, curve_len);
    bitLen = count_one_bits(&tempBuf[144 / 2], curve_len, littleEnd);
    assert(bitLen > 1); /* k=1 (OPTEW=1) not supported by ECPM hardware; caller must handle k=1 */
    SCE_PKE_OPTEW(bitLen);

#ifdef PKE_LITTLE
    if (kEndian == littleEnd)
    {
        pke_dataformat(&tempBuf[144 / 2], tempBuf + padByteNum, curve_len, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, tempBuf, pad_len);
    }
    else
    {
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB + padByteNum, &tempBuf[144 / 2], curve_len);
    }

    memset(tempBuf, 0, sizeof(tempBuf));

    if (inEndian == littleEnd)
    {
        pke_dataformat((uint8_t *)pointIn, tempBuf + padByteNum, curve_len, curve_len);
        pke_dataformat((uint8_t *)pointIn + curve_len, tempBuf + pad_len * 2, curve_len, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)tempBuf, curveBitLen * 2 / 8 + padByteNum * 2);
    }
    else
    {
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum, (uint8_t *)pointIn, curve_len);
        memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + padByteNum * 2 + curve_len, (uint8_t *)pointIn + curve_len, curve_len);
    }

    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
#else

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, &tempBuf[144 / 2], curve_len);

    memset(tempBuf, 0, sizeof(tempBuf));

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)pointIn, curve_len);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + pad_len, (uint8_t *)pointIn + curve_len, curve_len);
    SCE_PKE_OPTLTX_MODE(0);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("k", (uint8_t *)ADDR_PKE_SEG_PKB, pad_len);
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, pad_len);
    bnu_print_mem("point_y", (uint8_t *)ADDR_PKE_SEG_PIB + pad_len, pad_len);
#endif

    SCE_PKE_SET_PKB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_ECI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point(ECI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, (pad_len) * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, (pad_len) * 3 / 4);
    SCE_PKE_FUNC(PIR_ECPD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(ECPD)", (uint8_t *)ADDR_PKE_SEG_POB, (pad_len) * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, (pad_len) * 3 / 4, (pad_len) * 3 / 4);

    SCE_PKE_SET_PIB1((pad_len) * 3 / 4);
    SCE_PKE_FUNC(PIR_ECPM);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(ECPM)", (uint8_t *)ADDR_PKE_SEG_POB, (pad_len) * 3);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, (pad_len) * 3 / 4);
    SCE_PKE_FUNC(PIR_ECM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult_x(ECM2I)", (uint8_t *)ADDR_PKE_SEG_POB, pad_len);
    bnu_print_mem("pointResult_y(ECM2I)", (uint8_t *)ADDR_PKE_SEG_POB + pad_len, pad_len);
#endif

    if (pointOut != NULL)
    {
#ifdef PKE_LITTLE              // Output (the chip's default output is big-endian mode)
        if (mode == littleEnd) // big endian output
        {
            pke_dataformat((uint8_t *)ADDR_PKE_SEG_POB + padByteNum, (uint8_t *)pointOut, curve_len, curve_len);
            pke_dataformat((uint8_t *)ADDR_PKE_SEG_POB + pad_len * 2, (uint8_t *)pointOut + curve_len, curve_len, curve_len);
        }
        else
        {
            SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (padByteNum / 4), (uint8_t *)pointOut, curve_len / 4);
            SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (padByteNum * 2 + curve_len) / 4, (uint8_t *)pointOut + curve_len, curve_len / 4);
        }
#else // Output (the chip's default output is big-endian mode)
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, (uint8_t *)pointOut, curve_len / 4);
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB + (pad_len) / 4, (uint8_t *)pointOut + curve_len, curve_len / 4);
#endif
    }
}

/**
 * @brief G point is defined in little-endian mode, so use little-endian mode to calculate k*G
 *
 * @param in input number
 * @param[out] pointOut output buffer
 * @param curveBitLen curve bit length
 */
void ecc_PointMul_Base(const uint8_t *in, uint32_t *pointOut, uint16_t curveBitLen)
{
    ecc_PointMul(in, ec_basePonitPtr, pointOut, curveBitLen);
}

void secp256k1_init(uint16_t modLen)
{
    ecc_init(0, SECP256K1);
}

void secp256r1_init(uint16_t modLen)
{
    ecc_init(0, SECP256R1);
}

/**
 * @brief out = x^y mod n (call RSA modular exponentiation, ec/ed curve has no this algorithm)
 *
 * @param n modulus
 * @param nLen modulus length
 * @param nEndian modulus endianness
 * @param x input number
 * @param xLen input number length
 * @param xEndian input number endianness
 * @param y exponent
 * @param yLen exponent length
 * @param yEndian exponent endianness
 * @param[out] out output buffer
 * @param mode output mode
 */
void curve_modular_expo(const uint8_t *n,
                        uint32_t       nLen,
                        uint8_t        nEndian,
                        const uint8_t *x,
                        uint32_t       xLen,
                        uint8_t        xEndian,
                        const uint8_t *y,
                        uint32_t       yLen,
                        uint8_t        yEndian,
                        uint8_t       *out,
                        uint8_t        mode)
{
    uint8_t      formart_n[32], formart_x[32], formart_y[32];
    bn_context_t ctx;

    memset(formart_n, 0, 32);
    memset(formart_x, 0, 32);
    memset(formart_y, 0, 32);

    if (nEndian == bigEnd)
    {
        pke_dataformat(n, formart_n, 32, 32);
    }
    else
    {
        memcpy(formart_n, n, 32);
    }

    // X,Y转小端

    if (xEndian == bigEnd)
    {
        pke_dataformat(x, formart_x, xLen, 32);
    }
    else
    {
        memcpy(formart_x, x, xLen);
    }

    if (yEndian == bigEnd)
    {
        pke_dataformat(y, formart_y, yLen, 32);
    }
    else
    {
        memcpy(formart_y, y, yLen);
    }

    // RSA function must clear RAM after calling
    SCE_CLR_RAM();
    REG_SCE_PKE_OPTLTX = 0;

    // mdelay_systick(1);
    for (int i = 0; i < 300; i++)
    {
        asm("nop");
    }

    printf("--------------------------------------------------------------------------\r\n");
    bnu_print_mem("formart_x", formart_x, 32);
    bnu_print_mem("formart_y", formart_y, 32);
    bnu_print_mem("formart_y", formart_n, 32);

    // This interface requires little-endian data, and the output is also little-endian
    bn_init(&ctx, (uint32_t *)formart_n, 8, true);
    bn_expo_mod_sn(&ctx, (uint32_t *)formart_x, (uint32_t *)formart_y, 32 / 4, 32 / 4, (uint32_t *)out);
    bnu_print_mem("out", out, 32);

    printf("--------------------------------------------------------------------------\r\n");

    if (mode == bigEnd)
    {
        pke_dataformat(out, out, 32, 32);
    }

    SCE_CLR_RAM(); // clear RAM data
    for (int i = 0; i < 500; i++)
    {
        asm("nop");
    }
}

/**
 * @brief out = 1/x mod p （abnormal case use RSA modular inverse instead, temporarily suspended）
 *
 * @param p modulus
 * @param pLen modulus length
 * @param x input number
 * @param xLen input number length
 * @param[out] out output buffer
 * @param curveType curve type
 * @param mode output mode
 */
void curve_modular_inverse(const uint8_t *p, uint32_t pLen, const uint8_t *x, uint32_t xLen, uint8_t *out, uint16_t curveType, uint8_t mode)
{
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)x, xLen);

    SCE_PKE_FUNC(PIR_ECINV);
    SCE_PKE_AR();
    SCE_PKE_DONE();
    SCE_PKE_INV_READY();
    SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, out, curveType / 8 / 4);

    if (mode == bigEnd) // big endian output
    {
        pke_dataformat(out, out, 32, 32);
    }
}

/**
 * @brief out = x + y mod n
 *
 * @param n order. 4 byte aligned.
 * @param x x. 4 byte aligned.
 * @param y y. 4 byte aligned.
 * @param[out] out result. 4 byte aligned.
 * @param curveType type = CURVE_192 | CURVE_256 | CURVE_384 | CURVE_512
 */
void curve_modular_add(const uint32_t *n, const uint32_t *x, const uint32_t *y, uint32_t *out, uint16_t curveType)
{
    uint32_t w_len = curveType / 32; // word length

    ASSERT_4BYTE_ALIGNED(n);
    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(out);
    assert(curveType == CURVE_192 || curveType == CURVE_256 || curveType == CURVE_384 || curveType == CURVE_521);

    // load N
    pke_init(n, w_len, n, w_len, 0);

#ifdef PKE_LITTLE
    // uint8_t tempBuf[CURVE_512 / 8];
    // memset(tempBuf, 0, sizeof(tempBuf));
    memcpy(tempBuf + curveType / 8 - xLen, x, xLen);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, tempBuf, curveType / 8);
#else
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, w_len);
#endif

#ifdef PKE_LITTLE
    // memset(tempBuf, 0, sizeof(tempBuf));
    memcpy(tempBuf + curveType / 8 - yLen, y, yLen);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + curveType / 8, tempBuf, curveType / 8);
#else
    SCE_PKE_OPTLTX_MODE(0);
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + w_len, y, w_len);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1(w_len);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_ECMA);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_LITTLE // Output (the chip's default output is big-endian mode)
    SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, out, curveType / 8 / 4);
#else // Output (the chip's default output is big-endian mode)
    bnu_memcpy_u32(out, (uint32_t *)ADDR_PKE_SEG_POB, w_len);
#endif
}

/**
 * @brief out = x - y mod n
 *
 * @param n order
 * @param x x. 4 byte aligned.
 * @param y y. 4 byte aligned.
 * @param[out] out result. 4 byte aligned.
 * @param curveType type = CURVE_192 | CURVE_256 | CURVE_384 | CURVE_512
 */
void curve_modular_sub(const uint32_t *n, const uint32_t *x, const uint32_t *y, uint32_t *out, uint16_t curveType)
{
    uint32_t w_len = curveType / 8 / 4; // word length

    ASSERT_4BYTE_ALIGNED(n);
    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(out);
    assert(curveType == CURVE_192 || curveType == CURVE_256 || curveType == CURVE_384 || curveType == CURVE_521);

    // load N
    pke_init(n, w_len, n, w_len, 0);

    // x
#ifdef PKE_LITTLE
    memcpy(tempBuf + curveType / 8 - xLen, x, xLen);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, tempBuf, curveType / 8);
#else
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, w_len);
#endif

    // y
#ifdef PKE_LITTLE
    memcpy(tempBuf + curveType / 8 - yLen, y, yLen);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + curveType / 8, tempBuf, curveType / 8);
#else
    SCE_PKE_OPTLTX_MODE(0);
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + w_len, y, w_len);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1(w_len);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_ECMS);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_LITTLE // Output (the chip's default output is big-endian mode)
    SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, out, curveType / 8 / 4);
#else // Output (the chip's default output is big-endian mode)
    bnu_memcpy_u32(out, (uint32_t *)ADDR_PKE_SEG_POB, w_len);
#endif
}

/**
 * @brief out = x * y mod n
 *
 * @param n order. 4 byte aligned
 * @param x x. 4 byte aligned
 * @param y y. 4 byte aligned
 * @param out result. 4 byte aligned
 * @param curveType type = CURVE_192 | CURVE_256 | CURVE_384 | CURVE_512
 */
void curve_modular_mul(const uint32_t *n, const uint32_t *x, const uint32_t *y, uint32_t *out, uint16_t curveType)
{
    uint32_t w_len = curveType / 8 / 4; // word length

    ASSERT_4BYTE_ALIGNED(n);
    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(out);
    assert(curveType == CURVE_192 || curveType == CURVE_256 || curveType == CURVE_384 || curveType == CURVE_521);

    // load N
    pke_init(n, w_len, n, w_len, 0);
    // x
#ifdef PKE_LITTLE
    memcpy(tempBuf + curveType / 8 - xLen, x, xLen);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, tempBuf, curveType / 8);
#else
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, w_len);
#endif

    // y
#ifdef PKE_LITTLE
    memcpy(tempBuf + curveType / 8 - yLen, y, yLen);
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB + curveType / 8, tempBuf, curveType / 8);
#else
    SCE_PKE_OPTLTX_MODE(0);
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + w_len, y, w_len);
#endif

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1(w_len);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_ECMM);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_LITTLE // Output (the chip's default output is big-endian mode)
    SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, out, curveType / 8 / 4);
#else // Output (the chip's default output is big-endian mode)
    bnu_memcpy_u32(out, (uint32_t *)ADDR_PKE_SEG_POB, w_len);
#endif
}

/**
 * @brief out = in mod p
 *
 * @param in input buffer
 * @param inLen byte length of input buffer. Must not greater than 64 bytes.
 * @param p modulo
 * @param pLen byte length of modulo. Must not greater than 64 bytes.
 * @param[out] outData output buffer. Must be 64 bytes.
 */
void curve_rmodL(const uint8_t *in, uint32_t inLen, uint8_t *p, uint32_t pLen, uint8_t *out)
{
    uint32_t bitLen;
    uint32_t w_len_p  = pLen / 4;
    uint32_t w_len_in = inLen / 4;

    ASSERT_4BYTE_ALIGNED(in);
    ASSERT_4BYTE_ALIGNED(p);
    ASSERT_4BYTE_ALIGNED(out);
    assert(inLen <= 64 && pLen <= 64);

    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PCON, (uint32_t *)p, w_len_p);
    bitLen = count_one_bits(p, pLen, littleEnd);
    SCE_PKE_OPTNW(bitLen);
    SCE_PKE_OPTEW(0);
    SCE_PKE_SET_PCON(0);
    SCE_PKE_OPTLTX_MODE(0); // default little-endian initialization
    SCE_PKE_SET_POB(0);

    SCE_PKE_FUNC(PIR_ECINIT);
    SCE_PKE_AR();
    SCE_PKE_DONE();

    SCE_PKE_OPTLTX_MODE(0);

    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, (uint32_t *)in, w_len_in);
    if (inLen < 64)
    {
        memset((uint32_t *)ADDR_PKE_SEG_PIB + w_len_in, 0, 64 - inLen);
    }

    // r2 (high 192bit)
    REG_SCE_PKE_PIB0 = 10;
    SCE_PKE_OPTEW(192 + 128); //
    SCE_PKE_OPTRW(192);
    REG_SCE_PKE_POB = 0;

    SCE_PKE_FUNC(PIR_MODL);
    SCE_PKE_AR();
    SCE_PKE_DONE();

    // r1 (middle 192bit)
    REG_SCE_PKE_PIB0 = 4;
    SCE_PKE_OPTEW(128); //
    SCE_PKE_OPTRW(192);
    REG_SCE_PKE_POB = 8;

    SCE_PKE_FUNC(PIR_MODL);
    SCE_PKE_AR();
    SCE_PKE_DONE();

    // r1 + r2 mod L
    Ich_TranData(0, 0x0B, 0, 0x09, 0, 8);
    Ich_TranData(0, 0x0B, 8, 0x09, 8, 8);

    SCE_PKE_SET_POB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1(8);
    SCE_PKE_FUNC(PIR_ECMA);
    SCE_PKE_AR();
    SCE_PKE_DONE();

    // r0 +r1 + r2 mod L  r0 low 128bit
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, (uint32_t *)in, 4);
    memset((uint32_t *)ADDR_PKE_SEG_PIB + 4, 0, 64 - 4);

    Ich_TranData(0, 0x0B, 0, 0x09, 8, 8);

    SCE_PKE_SET_POB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_PIB1(8);
    SCE_PKE_FUNC(PIR_ECMA);
    SCE_PKE_AR();
    SCE_PKE_DONE();

    bnu_memcpy_u32((uint32_t *)out, (uint32_t *)ADDR_PKE_SEG_POB, 8);
}

/********************************************************************X25519 MOUDLE*****************************************************************************/
// little-endian definition
static uint32_t X25519_P_D[] = { 0xffffffed, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff, 0x0001DB41, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

void x25519_init(uint16_t modLen)
{
    pke_init(X25519_P_D, 8, X25519_P_D + 8, 8, modLen);
}

/**
 * @brief out = k * G
 *
 * @param k a scalar. The length of k must be equal to 32 bytes.
 * @param[out] out ouput buffer. 32 bytes.
 */
void x25519_mul_base(const uint8_t *k, uint8_t *out)
{
    uint8_t temp[32];

    ASSERT_4BYTE_ALIGNED(k);
    ASSERT_4BYTE_ALIGNED(out);

#ifdef PKE_LITTLE
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);

    if (kEndian == littleEnd)
    {
        pke_dataformat(k, temp, 32, 32);
    }
    else
    {
        memcpy(temp, k, kLen);
    }
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)temp, 32);

    memset(temp, 0, sizeof(temp));
    temp[31] = 0x09;
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)temp, 32);
#else
    // uint32_t len;
    SCE_PKE_OPTLTX_MODE(0);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)k, 32);
    memset(temp, 0, sizeof(temp));
    temp[0] = 0x09;
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)temp, 32);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("k", (uint8_t *)ADDR_PKE_SEG_PKB, 32);
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
#endif

    SCE_PKE_SET_PKB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_X25519);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(X25519)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_FUNC(PIR_EDM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
#endif

    // Output (the chip's default output is big-endian mode)
    if (out != NULL)
    {
#ifdef PKE_LITTLE // Output (the chip's default output is big-endian mode)
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, outbuf, 8);

        if (mode == littleEnd) // big endian output
        {
            pke_dataformat(outbuf, outbuf, 32, 32);
        }
#else // Output (the chip's default output is big-endian mode)
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, out, 8);
#endif
    }
}

/**
 * @brief out = k * inbuf
 *
 * @param k private key, a scalar The length of k must be 32 bytes.
 * @param inbuf input buffer. 32 bytes.
 * @param[out] outbuf output buffer. 32 bytes.
 */
void x25519_mul(const uint8_t *k, const uint8_t *inbuf, uint8_t *out)
{
    ASSERT_4BYTE_ALIGNED(k);
    ASSERT_4BYTE_ALIGNED(inbuf);

#ifdef PKE_LITTLE
    SCE_PKE_OPTLTX_MODE(REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_POB_PSOB | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_PIB1_PSIB1);

    if (kEndian == littleEnd)
    {
        pke_dataformat(k, temp, 32, 32);
    }
    else
    {
        memcpy(temp, k, kLen);
    }

    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)temp, 32);

    if (inEndian == littleEnd)
    {
        pke_dataformat(inbuf, temp, 32, 32);
    }
    else
    {
        memcpy(temp, inbuf, inLen);
    }
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)temp, inLen);
#else
    SCE_PKE_OPTLTX_MODE(0);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PKB, (uint8_t *)k, 32);
    memcpy_u32((uint8_t *)ADDR_PKE_SEG_PIB, (uint8_t *)inbuf, 32);
#endif

#ifdef PKE_DEBUG
    bnu_print_mem("k", (uint8_t *)ADDR_PKE_SEG_PKB, 32);
    bnu_print_mem("point_x", (uint8_t *)ADDR_PKE_SEG_PIB, 32);
#endif

    SCE_PKE_SET_PKB(0);
    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_EDI2MD);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("point(EDI2MD)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_SET_POB(0);
    SCE_PKE_FUNC(PIR_X25519);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(X25519)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
#endif

    Ich_TranData(0, 0x0B, 0, 0x09, 0, 32);

    SCE_PKE_SET_PIB0(0);
    SCE_PKE_FUNC(PIR_EDM2I);
    SCE_PKE_AR();
    SCE_PKE_DONE();

#ifdef PKE_DEBUG
    bnu_print_mem("pointResult(EDM2I)", (uint8_t *)ADDR_PKE_SEG_POB, 32);
#endif

    // Output (the chip's default output is big-endian mode)
    if (out != NULL)
    {
#ifdef PKE_LITTLE // Output (the chip's default output is big-endian mode)
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, outbuf, 8);

        if (mode == littleEnd) // big endian output
        {
            pke_dataformat(outbuf, outbuf, 32, 32);
        }
#else // Output (the chip's default output is big-endian mode)
        SCE_U32_COPY((uint32_t *)ADDR_PKE_SEG_POB, out, 8);
#endif
    }
}

/**
 * @brief calculate the public key from the private key using X25519. private_key and public_key are both big-endian
 *
 * @param private_key 32 bytes private key
 * @param[out] public_key 32 bytes public key output buffer
 */
void x25519_publickey(const uint8_t *private_key, uint8_t *public_key)
{
    uint8_t  k[32];
    uint32_t bitLen;
    memcpy(k, private_key, 32);

    // Input private key is processed in little-endian mode
    k[0] &= 248;  // bit0-2 set to 0
    k[31] &= 127; // bit255 set to 0
    k[31] |= 64;  // bit254 set to 1

    bitLen = count_one_bits((uint8_t *)k, 32, littleEnd);
    x25519_init(bitLen);
    x25519_mul_base(k, public_key);
}

/**
 * @brief share key calculation. sharekey = private_key * other_public_key
 *
 * @param private_key 32 bytes private key
 * @param other_public_key public key. 32 bytes.
 * @param[out] sharkey share key output buffer. 32 bytes.
 */
void x25519_sharekey(const uint8_t *private_key, const uint8_t *other_public_key, uint8_t *sharekey)
{
    uint8_t  k[32], u[32];
    uint32_t bitLen;
    memcpy(k, private_key, 32);
    memcpy(u, other_public_key, 32);

    k[0] &= 248;  // bit0-2 set to 0
    k[31] &= 127; // bit255 set to 0
    k[31] |= 64;  // bit254 set to 1

    bitLen = count_one_bits((uint8_t *)k, 32, littleEnd);
    x25519_init(bitLen);
    x25519_mul(k, u, sharekey);
}
