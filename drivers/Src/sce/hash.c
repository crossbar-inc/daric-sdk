/**
 ******************************************************************************
 * @file    hash.c
 * @author  SCE Team
 * @brief   HASH driver implementation.
 *          This file provides firmware functions to manage the HASH
 *          peripheral, including SHA256, SHA512, HMAC, and BLAKE series.
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
#include <string.h>
#include "daric.h"
#include "auth.h"
#include "sdma.h"
#include "hash.h"
#include "bn_util.h"

static const uint32_t SHA256_H[8] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };

static const uint32_t SHA256_K[]
    = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74,
        0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d,
        0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e,
        0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

static const uint32_t SHA512_H[] = { 0x6a09e667, 0xf3bcc908, 0xbb67ae85, 0x84caa73b, 0x3c6ef372, 0xfe94f82b, 0xa54ff53a, 0x5f1d36f1,
                                     0x510e527f, 0xade682d1, 0x9b05688c, 0x2b3e6c1f, 0x1f83d9ab, 0xfb41bd6b, 0x5be0cd19, 0x137e2179 };

static const uint32_t SHA512_K[]
    = { 0x428A2F98, 0xD728AE22, 0x71374491, 0x23EF65CD, 0xB5C0FBCF, 0xEC4D3B2F, 0xE9B5DBA5, 0x8189DBBC, 0x3956C25B, 0xF348B538, 0x59F111F1, 0xB605D019, 0x923F82A4, 0xAF194F9B,
        0xAB1C5ED5, 0xDA6D8118, 0xD807AA98, 0xA3030242, 0x12835B01, 0x45706FBE, 0x243185BE, 0x4EE4B28C, 0x550C7DC3, 0xD5FFB4E2, 0x72BE5D74, 0xF27B896F, 0x80DEB1FE, 0x3B1696B1,
        0x9BDC06A7, 0x25C71235, 0xC19BF174, 0xCF692694, 0xE49B69C1, 0x9EF14AD2, 0xEFBE4786, 0x384F25E3, 0x0FC19DC6, 0x8B8CD5B5, 0x240CA1CC, 0x77AC9C65, 0x2DE92C6F, 0x592B0275,
        0x4A7484AA, 0x6EA6E483, 0x5CB0A9DC, 0xBD41FBD4, 0x76F988DA, 0x831153B5, 0x983E5152, 0xEE66DFAB, 0xA831C66D, 0x2DB43210, 0xB00327C8, 0x98FB213F, 0xBF597FC7, 0xBEEF0EE4,
        0xC6E00BF3, 0x3DA88FC2, 0xD5A79147, 0x930AA725, 0x06CA6351, 0xE003826F, 0x14292967, 0x0A0E6E70, 0x27B70A85, 0x46D22FFC, 0x2E1B2138, 0x5C26C926, 0x4D2C6DFC, 0x5AC42AED,
        0x53380D13, 0x9D95B3DF, 0x650A7354, 0x8BAF63DE, 0x766A0ABB, 0x3C77B2A8, 0x81C2C92E, 0x47EDAEE6, 0x92722C85, 0x1482353B, 0xA2BFE8A1, 0x4CF10364, 0xA81A664B, 0xBC423001,
        0xC24B8B70, 0xD0F89791, 0xC76C51A3, 0x0654BE30, 0xD192E819, 0xD6EF5218, 0xD6990624, 0x5565A910, 0xF40E3585, 0x5771202A, 0x106AA070, 0x32BBD1B8, 0x19A4C116, 0xB8D2D0C8,
        0x1E376C08, 0x5141AB53, 0x2748774C, 0xDF8EEB99, 0x34B0BCB5, 0xE19B48A8, 0x391C0CB3, 0xC5C95A63, 0x4ED8AA4A, 0xE3418ACB, 0x5B9CCA4F, 0x7763E373, 0x682E6FF3, 0xD6B2B8A3,
        0x748F82EE, 0x5DEFB2FC, 0x78A5636F, 0x43172F60, 0x84C87814, 0xA1F0AB72, 0x8CC70208, 0x1A6439EC, 0x90BEFFFA, 0x23631E28, 0xA4506CEB, 0xDE82BDE9, 0xBEF9A3F7, 0xB2C67915,
        0xC67178F2, 0xE372532B, 0xCA273ECE, 0xEA26619C, 0xD186B8C7, 0x21C0C207, 0xEADA7DD6, 0xCDE0EB1E, 0xF57D4F7F, 0xEE6ED178, 0x06F067AA, 0x72176FBA, 0x0A637DC5, 0xA2C898A6,
        0x113F9804, 0xBEF90DAE, 0x1B710B35, 0x131C471B, 0x28DB77F5, 0x23047D84, 0x32CAAB7B, 0x40C72493, 0x3C9EBE0A, 0x15C9BEBC, 0x431D67C4, 0x9C100D4C, 0x4CC5D4BE, 0xCB3E42B6,
        0x597F299C, 0xFC657E2A, 0x5FCB6FAB, 0x3AD6FAEC, 0x6C44198C, 0x4A475817 };

static const uint32_t BLK2S_H[] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };

/* 0x6B08E647 = 0x6A09E667 ^ 0x01010020  -- 01(depth)  01(fanout)  00(keyLen) 20(digest_length) */
static const uint32_t BLK2S_EX[] = { 0x6B08E647 /*0x6A09E667*/, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };

static const uint32_t BLK2B_H[] = { 0x6A09E667, 0xF3BCC908, 0xBB67AE85, 0x84CAA73B, 0x3C6EF372, 0xFE94F82B, 0xA54FF53A, 0x5F1D36F1,
                                    0x510E527F, 0xADE682D1, 0x9B05688C, 0x2B3E6C1F, 0x1F83D9AB, 0xFB41BD6B, 0x5BE0CD19, 0x137E2179 };

/* 0x0x6a09e667f3bcc908 ^  0000000001010040 = 6A09E667F2BDC948 */
static const uint32_t BLK2B_EX[] = {
    /* U64 Little-endian format input, the algorithm core expects data 0x6A09E667F2bDC948 */
    0xF2bDC948, 0x6A09E667, 0x84CAA73B, 0xBB67AE85, 0xFE94F82B, 0x3C6EF372, 0x5F1D36F1, 0xA54FF53A,
    0xADE682D1, 0x510E527F, 0x2B3E6C1F, 0x9B05688C, 0xFB41BD6B, 0x1F83D9AB, 0x137E2179, 0x5BE0CD19
};

static const uint32_t BLK2_X[] = { 0x01234567, 0x89ABCDEF, 0xEA489FD6, 0x1C02B753, 0xB8C052FD, 0xAE367194, 0x7931DCBE, 0x265A40F8, 0x905724AF, 0xE1BC683D,
                                   0x2C6A0B83, 0x4D75FE19, 0xC51FED4A, 0x0763928B, 0xDB7EC139, 0x50F4862A, 0x6FE9B308, 0xC2D714A5, 0xA2847615, 0xFB9E3CD0 };

static const uint32_t BLK3_H[] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };

static const uint32_t BLK3_X[]
    = { 0x01234567, 0x89ABCDEF, 0x263A704D, 0x1BC59EF8, 0x34ACD27E, 0x6590BF81, 0xA7C9E3DF, 0x40B25816, 0xCD9BFAE8, 0x72530164, 0x9EB58CF1, 0xD30A2647, 0xBF501986, 0xEA2C347D };

/* RAMSEG_RIPMD_H */
static const uint32_t RIPMD_H[] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0, 0x00000000 };

/* RAMSEG_RIPMD_K */
static const uint32_t RIPMD_K[] = { 0x00000000, 0x50A28BE6, 0x5A827999, 0x5C4DD124, 0x6ED9EBA1, 0x6D703EF3, 0x8F1BBCDC, 0x7A6D76E9, 0xA953FD4E, 0x00000000 };

/* RAMSEG_RIPMD_X */
static const uint32_t RIPMD_X[]
    = { 0xBEFC5879, 0xBDEF6798, 0x01234567, 0x89ABCDEF, 0x899BDFF5, 0x778BEEC6, 0x5E7092B4, 0xD6F81A3C, 0x768DB97F, 0x7CF9B7DC, 0x74D1A6F3, 0xC0952EB8, 0x9DF7C89B, 0x77C76FDB,
        0x6B370D5A, 0xEF8C4912, 0xBD67E9DF, 0xE8D65C75, 0x3AE49F81, 0x2706DB5C, 0x97FB866E, 0xCD5EDD75, 0xF5137E69, 0xB8C2A04D, 0xBCEFEF98, 0x9E56865C, 0x19BA08C4, 0xD37FE562,
        0xF58BEE6E, 0x69C9C5F8, 0x86413BF0, 0x5C2D97AE, 0x9F5B68DC, 0x5CDEB856, 0x40597C2A, 0xE138B6FD, 0x85C9C5E6, 0x8D65FDBB, 0xCFA41587, 0x62DE039B };

/* RAMSEG_SHA3 */
static const uint32_t RAMSEG_SHA3[]
    = { 0x00082d35, 0x80903a52, 0x00853506, 0x01b0ab42, 0x01072925, 0x7e1ab3fd, 0x004930f5, 0xdcdd9578, 0x00c63915, 0x1b52720e, 0x00000000, 0x00000001,
        0x00000000, 0x00008082, 0x80000000, 0x0000808a, 0x80000000, 0x80008000, 0x00000000, 0x0000808b, 0x00000000, 0x80000001, 0x80000000, 0x80008081,
        0x80000000, 0x00008009, 0x00000000, 0x0000008a, 0x00000000, 0x00000088, 0x00000000, 0x80008009, 0x00000000, 0x8000000a, 0x00000000, 0x8000808b,
        0x80000000, 0x0000008b, 0x80000000, 0x00008089, 0x80000000, 0x00008003, 0x80000000, 0x00008002, 0x80000000, 0x00000080, 0x00000000, 0x0000800a,
        0x80000000, 0x8000000a, 0x80000000, 0x80008081, 0x80000000, 0x00008080, 0x00000000, 0x80000001, 0x80000000, 0x80008008 };

SHA_CTX SHA_INFO;

SHA_CTX HMAC_INFO;

/* Convert memory U8 to U32 in little-endian mode */
uint32_t SCE_U8ToU32_le(const uint8_t *input, uint32_t *output, uint32_t num)
{
    uint32_t i, j = 0;
    for (i = 0; i < num;)
    {
        output[j] = ((uint32_t)input[i + 3] << 24) | ((uint32_t)input[i + 2] << 16) |
                    ((uint32_t)input[i + 1] << 8)  |  (uint32_t)input[i + 0];
        i += 4;
        j++;
    }
    return j;
}

/* Convert memory U8 to U32 in big-endian mode */
uint32_t SCE_U8ToU32(const uint8_t *input, uint32_t *output, uint32_t num)
{
    uint32_t i, j = 0;
    for (i = 0; i < num;)
    {
        output[j] = ((uint32_t)input[i + 0] << 24) | ((uint32_t)input[i + 1] << 16) |
                    ((uint32_t)input[i + 2] << 8)  |  (uint32_t)input[i + 3];
        i += 4;
        j++;
    }
    return j;
}

/* Convert U32 data to memory in big-endian mode */
uint32_t SCE_U32ToU8(const uint32_t *input, uint8_t *output, uint32_t num)
{
    uint32_t i, j = 0;
    for (i = 0; i < num;)
    {
        output[j]     = (uint8_t)(input[i] >> 24);
        output[j + 1] = (uint8_t)(input[i] >> 16);
        output[j + 2] = (uint8_t)(input[i] >> 8);
        output[j + 3] = (uint8_t)(input[i]);

        i++;
        j += 4;
    }
    return j;
}

/* Convert big-endian U64 to U32 */
uint32_t SCE_U64ToU32(const uint64_t *input, uint32_t *output, uint32_t num)
{
    uint32_t i, j = 0;
    for (i = 0; i < num;)
    {
        output[j]     = (uint32_t)(input[i] >> 32);
        output[j + 1] = (uint32_t)(input[i]);

        i++;
        j += 2;
    }
    return j;
}

/* Convert U32 data to memory in little-endian mode */
uint32_t SCE_U32_COPY(const uint32_t *input, uint8_t *output, uint32_t num)
{
    uint32_t i, j = 0;
    for (i = 0; i < num;)
    {
        output[j]     = (uint8_t)(input[i]);
        output[j + 1] = (uint8_t)(input[i] >> 8);
        output[j + 2] = (uint8_t)(input[i] >> 16);
        output[j + 3] = (uint8_t)(input[i] >> 24);

        i++;
        j += 4;
    }
    return j;
}

static inline void hash_fun_Execute(uint32_t opt1, uint32_t opt2, uint8_t fun)
{
    SCE_HASH_OPT1_CNT(opt1);
    SCE_HASH_OPT2_MODE(opt2);

    SCE_HASH_FUNC(fun);

#ifdef HASH_DEBUG
    printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
    printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
    printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
    printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif
    SCE_HASH_AR()
    SCE_HASH_MFSM_DONE();
}

void sce_Sysinit_Hash(void)
{
    SET_SECMODE_NORMAL();
    SCE_HASH_CLK_EN();
    __DMB();
}

/**
 * @brief Hash module initialization
 *
 */
void hash_init(void)
{
    uint32_t offset = 0;
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)SHA256_H, sizeof(SHA256_H));
    offset += sizeof(SHA256_H);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)SHA256_K, sizeof(SHA256_K));
    offset += sizeof(SHA256_K);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)SHA512_H, sizeof(SHA512_H));
    offset += sizeof(SHA512_H);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)SHA512_K, sizeof(SHA512_K));
    offset += sizeof(SHA512_K);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)BLK2S_H, sizeof(BLK2S_H));
    offset += sizeof(BLK2S_H);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)BLK2B_H, sizeof(BLK2B_H));
    offset += sizeof(BLK2B_H);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)BLK2_X, sizeof(BLK2_X));
    offset += sizeof(BLK2_X);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)BLK3_H, sizeof(BLK3_H));
    offset += sizeof(BLK3_H);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)BLK3_X, sizeof(BLK3_X));
    offset += sizeof(BLK3_X);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)RIPMD_H, sizeof(RIPMD_H));
    offset += sizeof(RIPMD_H);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)RIPMD_K, sizeof(RIPMD_K));
    offset += sizeof(RIPMD_K);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)RIPMD_X, sizeof(RIPMD_X));
    offset += sizeof(RIPMD_X);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + offset, (uint8_t *)RAMSEG_SHA3, sizeof(RAMSEG_SHA3));
    offset += sizeof(RAMSEG_SHA3);

    SCE_HASH_OPT3_MODE(0); /* U32 big-endian load constant */
    SCE_HASH_FUNC(HF_INIT);

    SCE_HASH_AR();

    SCE_HASH_MFSM_DONE();
}

static void shaLenEncode(const uint64_t input, uint8_t *output, uint32_t idx)
{
    output[idx + 0] = (uint8_t)(input >> 56);
    output[idx + 1] = (uint8_t)(input >> 48);
    output[idx + 2] = (uint8_t)(input >> 40);
    output[idx + 3] = (uint8_t)(input >> 32);
    output[idx + 4] = (uint8_t)(input >> 24);
    output[idx + 5] = (uint8_t)(input >> 16);
    output[idx + 6] = (uint8_t)(input >> 8);
    output[idx + 7] = (uint8_t)(input >> 0);
}

/**
 * @brief SHA256/SHA512 initialization
 *
 */
void sha256_512_Init(void)
{
    memset(&SHA_INFO, 0, sizeof(SHA_CTX));
    SHA_INFO.sign = 1; /* First block identifier */
}

/**
 * @brief SHA256/SHA512 input
 *
 */
void sha256_512_Input(const uint8_t *indata, uint32_t indataLen, uint8_t cacl_mode)
{
    uint64_t temp;
    uint32_t blockSize, function, inOffset, digestSize;
    uint32_t maxCount, count;

    uint8_t hash_opt2;
    if (indata == NULL)
        return;
    if (indataLen == 0)
        return;

    blockSize  = (cacl_mode == SHA256_MODE) ? SHA256_BLOCK_SIZE : SHA512_BLOCK_SIZE;
    digestSize = (cacl_mode == SHA256_MODE) ? SHA256_DIGEST_SIZE : SHA512_DIGEST_SIZE;
    function   = (cacl_mode == SHA256_MODE) ? HF_SHA256 : HF_SHA512;

    maxCount = 512 / blockSize;
    count    = 0;

    inOffset = 0;
    while ((indataLen + SHA_INFO.dataLen) >= blockSize)
    {
        memcpy(SHA_INFO.data + SHA_INFO.dataLen, indata + inOffset, blockSize - SHA_INFO.dataLen);

        temp = SHA_INFO.msgBit_L + (blockSize - SHA_INFO.dataLen) * 8;

        if (temp < SHA_INFO.msgBit_L)
        {
            SHA_INFO.msgBit_H++;
        }
        SHA_INFO.msgBit_L = temp;

        /* Fill data to ADDR_HASH_SEG_MSG */
#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, SHA_INFO.data, blockSize);
#else
        SCE_U8ToU32(SHA_INFO.data, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif

        count++;

        inOffset += blockSize - SHA_INFO.dataLen;
        indataLen -= blockSize - SHA_INFO.dataLen;
        SHA_INFO.dataLen = 0;
        memset(SHA_INFO.data, 0, sizeof(SHA_INFO.data));

        if (count >= maxCount) /* calculate every 512 bytes */
        {
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
            printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

            if (SHA_INFO.sign == 1)
            {
                hash_opt2     = HASH_IF_START;
                SHA_INFO.sign = 0;
            }
            else
            {
                hash_opt2 = 0;
            }
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);

#ifdef HASH_LITTLE
            SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
            SCE_HASH_OPT3_MODE(0);
#endif
            hash_fun_Execute(count - 1, hash_opt2, function);

            count = 0;

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
            printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif

#endif

            /* Record hash process calculation value */
#ifdef HASH_LITTLE
            SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, SHA_INFO.h, digestSize / 4);
#else
            SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, SHA_INFO.h, digestSize / 4);
#endif
        }
    }

    if (count != 0)
    {
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
        printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif
        SCE_HASH_OPT1_CNT(count - 1);
        count = 0;
        if (SHA_INFO.sign == 1)
        {
            SCE_HASH_OPT2_FIRT_BLOCK();
            SHA_INFO.sign = 0;
        }
        else
        {
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
        }
        SCE_HASH_SET_MSG(0);
        SCE_HASH_FUNC(function);

#ifdef HASH_LITTLE
        SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
        SCE_HASH_OPT3_MODE(0);
#endif

#ifdef HASH_DEBUG
        printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
        printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
        printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
        printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

        SCE_HASH_AR();
        SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
        printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif

#endif

        /* Record hash process calculation value */
#ifdef HASH_LITTLE
        SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, SHA_INFO.h, digestSize / 4);
#else
        SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, SHA_INFO.h, digestSize / 4);
#endif
    }

    temp = SHA_INFO.msgBit_L + indataLen * 8;

    if (temp < SHA_INFO.msgBit_L)
    {
        SHA_INFO.msgBit_H++;
    }
    SHA_INFO.msgBit_L = temp;

    memcpy(SHA_INFO.data + SHA_INFO.dataLen, indata + inOffset, indataLen);
    SHA_INFO.dataLen += indataLen;
}

/**
 * @brief SHA256/SHA512 final result
 *
 */
void sha256_512_Result(const uint8_t *indata, uint32_t indataLen, uint8_t cacl_mode, uint8_t *hash)
{
    uint32_t blockSize, function, digestSize;

    if (hash == NULL)
        return;

    if (indata != NULL && indataLen != 0)
    {
        sha256_512_Input(indata, indataLen, cacl_mode); // Calculate and leave only the last packet of data
    }

    blockSize  = (cacl_mode == SHA256_MODE) ? SHA256_BLOCK_SIZE : SHA512_BLOCK_SIZE;
    digestSize = (cacl_mode == SHA256_MODE) ? SHA256_DIGEST_SIZE : SHA512_DIGEST_SIZE;
    function   = (cacl_mode == SHA256_MODE) ? HF_SHA256 : HF_SHA512;

    /* pad msg */
    if (SHA_INFO.dataLen < ((cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16)))
    {
        SHA_INFO.data[SHA_INFO.dataLen] = HASH_PAD;
    }
    else
    {
        SHA_INFO.data[SHA_INFO.dataLen] = HASH_PAD;

        if (SHA_INFO.sign == 1)
        {
            SCE_HASH_OPT2_FIRT_BLOCK();
            SHA_INFO.sign = 0;
        }
        else
        {
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
        }

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, SHA_INFO.data, blockSize);
#else
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(SHA_INFO.data, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, calcBuf, blockSize);
#endif

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize);
#else
        printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, blockSize / 4);
#endif
#endif

        SCE_HASH_OPT1_CNT(0);
        SCE_HASH_SET_MSG(0);
        SCE_HASH_OUTPUT(0);

#ifdef HASH_LITTLE
        SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
        SCE_HASH_OPT3_MODE(0);
#endif
        SCE_HASH_FUNC(function);

        SCE_HASH_AR();
        SCE_HASH_MFSM_DONE();
        memset(SHA_INFO.data, 0, sizeof(SHA_INFO.data));
    }

#ifndef HASH_LITTLE
    /* Convert BYTE array to U32 array, then write to HASH_MSG */
    SCE_U8ToU32(SHA_INFO.data, calcBuf, (cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16));
#endif

    if (cacl_mode == SHA256_MODE)
    {
#ifdef HASH_LITTLE
        shaLenEncode(SHA_INFO.msgBit_L, SHA_INFO.data, blockSize - 8);
#else
        /* pad 64 bit msg len */
        SCE_U64ToU32(&SHA_INFO.msgBit_L, &calcBuf[14], 1);
#endif
    }
    else
    {
#ifdef HASH_LITTLE
        shaLenEncode(SHA_INFO.msgBit_H, SHA_INFO.data, blockSize - 16);
        shaLenEncode(SHA_INFO.msgBit_L, SHA_INFO.data, blockSize - 8);
#else
        /* pad 128 bit msg len */
        SCE_U64ToU32(&SHA_INFO.msgBit_H, &calcBuf[28], 1);
        SCE_U64ToU32(&SHA_INFO.msgBit_L, &calcBuf[30], 1);
#endif
    }

    if (SHA_INFO.sign == 1)
    {
        SCE_HASH_OPT2_FIRT_BLOCK();
        SHA_INFO.sign = 0;
    }
    else
    {
        SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    }

#ifdef HASH_LITTLE
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, SHA_INFO.data, blockSize);
#else
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, calcBuf, blockSize);
#endif

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize);
#else
    printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, blockSize / 4);
#endif
#endif

    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_FUNC(function);
#ifdef HASH_LITTLE
    SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
    SCE_HASH_OPT3_MODE(0);
#endif
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

#ifdef HASH_LITTLE
    SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, hash, digestSize / 4);
#else
    SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, hash, digestSize / 4);
#endif

#ifdef HASH_DEBUG
    bnu_print_mem("HASH_RESULT", hash, digestSize);
#endif
}

/**
 * @brief SHA256/SHA512 one-time calculation
 *
 */
void sha256_512_calc(const uint8_t *indata, uint32_t indataLen, uint8_t cacl_mode, uint8_t *hash)
{
    uint64_t msgBitLen, msg_512_high, msg_512_low, temp;
    uint32_t i, loop, remainLen, blockSize, digestSize;
    uint8_t  firstSign;
    firstSign = 1;
    uint8_t  tempBuf[128];
    uint32_t maxCount, count;

    if (hash == NULL)
        return;

    i = 0;
    if (cacl_mode == SHA256_MODE)
    {
        blockSize  = SHA256_BLOCK_SIZE;
        digestSize = SHA256_DIGEST_SIZE;
        loop       = indataLen / SHA256_BLOCK_SIZE;
        remainLen  = indataLen % SHA256_BLOCK_SIZE;
        msgBitLen  = indataLen * 8;
    }
    else
    {
        blockSize    = SHA512_BLOCK_SIZE;
        digestSize   = SHA512_DIGEST_SIZE;
        loop         = indataLen / SHA512_BLOCK_SIZE;
        remainLen    = indataLen % SHA512_BLOCK_SIZE;
        msg_512_high = 0;
        msg_512_low  = 0;
    }

    maxCount = 512 / blockSize;
    count    = 0;

    for (i = 0; i < loop; i++)
    {
        if (cacl_mode == SHA512_MODE) // SHA512 needs to calculate the bit length of the MSG message
        {
            temp = msg_512_low + SHA512_BLOCK_SIZE * 8;
            if (temp < msg_512_low)
            {
                msg_512_high++;
            }
            msg_512_low = temp;
        }

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, indata + i * blockSize, blockSize);
#else
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(indata + i * blockSize, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif

        count++;

        if (count >= maxCount) /* Calculate every 512 bytes */
        {
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
            printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

            if (firstSign == 1)
            {
                SCE_HASH_OPT2_FIRT_BLOCK();
                firstSign = 0;
            }
            else
            {
                SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            }

            SCE_HASH_OPT1_CNT(count - 1);
            count = 0;
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);

#ifdef HASH_LITTLE
            SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
            SCE_HASH_OPT3_MODE(0);
#endif
            SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_SHA256 : HF_SHA512);

#ifdef HASH_DEBUG
            printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
            printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
            printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
            printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
            printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif

#endif
        }
    }
    /* pad msg */
    if (cacl_mode == SHA512_MODE) /* SHA512 needs to calculate the bit length of the MSG message */
    {
        temp = msg_512_low + remainLen * 8;
        if (temp < msg_512_low)
        {
            msg_512_high++;
        }
        msg_512_low = temp;
    }

    if (remainLen < ((cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16)))
    {
        memset(tempBuf, 0, sizeof(tempBuf));
        memcpy(tempBuf, indata + i * blockSize, remainLen);
        tempBuf[remainLen] = HASH_PAD;
    }
    else
    {
        memset(tempBuf, 0, sizeof(tempBuf));
        memcpy(tempBuf, indata + i * blockSize, remainLen);
        tempBuf[remainLen] = HASH_PAD;

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, tempBuf, blockSize);
#else
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(tempBuf, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif

        count++;

        if (count >= maxCount) /* calculate once when msg is full of 512 bytes */
        {
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
            printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

            if (firstSign == 1)
            {
                SCE_HASH_OPT2_FIRT_BLOCK();
                firstSign = 0;
            }
            else
            {
                SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            }

            SCE_HASH_OPT1_CNT(count - 1);
#ifdef HASH_LITTLE
            SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
            SCE_HASH_OPT3_MODE(0);
#endif
            count = 0;
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);
            SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_SHA256 : HF_SHA512);

#ifdef HASH_DEBUG
            printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
            printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
            printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
            printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

#ifdef ALG_TEST_MODE
            return; /* Concurrent test, not calculated here */
#endif
            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
            printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif

#endif
        }
        memset(tempBuf, 0, sizeof(tempBuf));
    }

#ifndef HASH_LITTLE
    /* Convert BYTE array to U32 array, then write to HASH_MSG */
    SCE_U8ToU32(tempBuf, calcBuf, (cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16));
#endif

    if (cacl_mode == SHA256_MODE)
    {
#ifdef HASH_LITTLE
        shaLenEncode(msgBitLen, tempBuf, blockSize - 8);
#else
        /* pad 64 bit msg len */
        SCE_U64ToU32(&msgBitLen, &calcBuf[14], 1);
#endif
    }
    else
    {
#ifdef HASH_LITTLE
        shaLenEncode(msg_512_high, tempBuf, blockSize - 16);
        shaLenEncode(msg_512_low, tempBuf, blockSize - 8);
#else
        /* pad 128 bit msg len */
        SCE_U64ToU32(&msg_512_high, &calcBuf[28], 1);
        SCE_U64ToU32(&msg_512_low, &calcBuf[30], 1);
#endif
    }
#ifdef HASH_LITTLE
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, tempBuf, blockSize);
#else
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif
    count++;

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
    printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

    if (firstSign == 1)
    {
        SCE_HASH_OPT2_FIRT_BLOCK();
        firstSign = 0;
    }
    else
    {
        SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    }
    SCE_HASH_OPT1_CNT(count - 1);
    count = 0;
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);

#ifdef HASH_LITTLE
    SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_RESULT);
#else
    SCE_HASH_OPT3_MODE(0);
#endif

    SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_SHA256 : HF_SHA512);

#ifdef HASH_DEBUG
    printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
    printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
    printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
    printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
    printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif

#ifdef HASH_LITTLE
    SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, hash, digestSize / 4);
#else
    SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, hash, digestSize / 4);
#endif

#ifdef HASH_DEBUG
    bnu_print_mem("HASH_RESULT", hash, digestSize);
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief HMAC-SHA256/SHA512 key initialization
 *
 */
void hmac_sha256_512_KeyInit(const uint8_t *key, uint32_t keylen, uint8_t cacl_mode)
{
    uint64_t msgBitLen, msg_512_high, msg_512_low, temp;
    uint32_t i, loop, remainLen, blockSize, digestSize;
    uint8_t  firstSign;
    firstSign = 1;
    uint8_t  tempBuf[128];
    uint32_t maxCount, count;

    if (cacl_mode == SHA256_MODE)
    {
        blockSize  = SHA256_BLOCK_SIZE;
        digestSize = SHA256_DIGEST_SIZE;
        loop       = keylen / SHA256_BLOCK_SIZE;
        remainLen  = keylen % SHA256_BLOCK_SIZE;
        msgBitLen  = keylen * 8;
    }
    else
    {
        blockSize    = SHA512_BLOCK_SIZE;
        digestSize   = SHA512_DIGEST_SIZE;
        loop         = keylen / SHA512_BLOCK_SIZE;
        remainLen    = keylen % SHA512_BLOCK_SIZE;
        msg_512_high = 0;
        msg_512_low  = 0;
    }

    if (keylen > blockSize)
    {
        /* According to SHA algorithm rules, pad and move data to SEG_LKEY, then call KEYHASH */
        i = 0;

        maxCount = 256 / blockSize;
        count    = 0;

        for (i = 0; i < loop; i++)
        {
            if (cacl_mode == SHA512_MODE) /* SHA512 needs to calculate the bit length of the MSG message */
            {
                temp = msg_512_low + SHA512_BLOCK_SIZE * 8;
                if (temp < msg_512_low)
                {
                    msg_512_high++;
                }
                msg_512_low = temp;
            }

#ifdef HASH_LITTLE
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + count * blockSize, key + i * blockSize, blockSize);
#else
            /* Convert BYTE array to U32 array, then write to HASH_MSG */
            SCE_U8ToU32(key + i * blockSize, calcBuf, blockSize);
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + count * blockSize, calcBuf, blockSize);
#endif
            count++;

            if (count >= maxCount) /* Calculate every 256 bytes */
            {

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
                bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_LKEY, blockSize * count);
#else
                printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_LKEY, (blockSize / 4) * count);
#endif
#endif
                if (firstSign == 1)
                {
                    SCE_HASH_OPT2_FIRT_BLOCK();
                    firstSign = 0;
                }
                else
                {
                    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
                }

                SCE_HASH_OPT1_CNT(count - 1);
                count = 0;
                SCE_HASH_SET_SEG_LKEY(0);
                SCE_HASH_SET_SEG_KEY(0);
#ifdef HASH_LITTLE
                SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_LKEY | SCE_HASH_OPT3_SEG_RESULT);
#else
                SCE_HASH_OPT3_MODE(0);
#endif
                SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_HMAC256_KEYHASH : HF_HMAC512_KEYHASH);

#ifdef HASH_DEBUG
                printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
                printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
                printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
                printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

                SCE_HASH_AR();
                SCE_HASH_MFSM_DONE();
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
                bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_KEY, digestSize);
#else
                printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_KEY, digestSize / 4);
#endif
#endif
            }
        }
        /* pad msg */
        if (cacl_mode == SHA512_MODE) /* SHA512 needs to calculate the bit length of the MSG message */
        {
            temp = msg_512_low + remainLen * 8;
            if (temp < msg_512_low)
            {
                msg_512_high++;
            }
            msg_512_low = temp;
        }

        if (remainLen < ((cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16)))
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            memcpy(tempBuf, key + i * blockSize, remainLen);
            tempBuf[remainLen] = HASH_PAD;
        }
        else
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            memcpy(tempBuf, key + i * blockSize, remainLen);
            tempBuf[remainLen] = HASH_PAD;

#ifdef HASH_LITTLE
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + count * blockSize, tempBuf, blockSize);
#else
            /* Convert BYTE array to U32 array, then write to HASH_MSG */
            SCE_U8ToU32(tempBuf, calcBuf, blockSize);
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + count * blockSize, calcBuf, blockSize);
#endif

            count++;

            if (count >= maxCount) /* Calculate every 512 bytes */
            {

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
                bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_LKEY, blockSize * count);
#else
                printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_LKEY, (blockSize / 4) * count);
#endif
#endif

                if (firstSign == 1)
                {
                    SCE_HASH_OPT2_FIRT_BLOCK();
                    firstSign = 0;
                }
                else
                {
                    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
                }

                SCE_HASH_OPT1_CNT(count - 1);
                count = 0;
                SCE_HASH_SET_SEG_LKEY(0);
                SCE_HASH_SET_SEG_KEY(0);
#ifdef HASH_LITTLE
                SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_LKEY | SCE_HASH_OPT3_SEG_RESULT);
#else
                SCE_HASH_OPT3_MODE(0);
#endif
                SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_HMAC256_KEYHASH : HF_HMAC512_KEYHASH);

#ifdef HASH_DEBUG
                printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
                printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
                printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
                printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

                SCE_HASH_AR();
                SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
                bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_KEY, digestSize);
#else
                printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_KEY, digestSize / 4);
#endif
#endif
            }

            memset(tempBuf, 0, sizeof(tempBuf));
        }
#ifndef HASH_LITTLE
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(tempBuf, calcBuf, (cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16));
#endif

        if (cacl_mode == SHA256_MODE)
        {
#ifdef HASH_LITTLE
            shaLenEncode(msgBitLen, tempBuf, blockSize - 8);
#else
            /* pad 64 bit msg len */
            SCE_U64ToU32(&msgBitLen, &calcBuf[14], 1);
#endif
        }
        else
        {
#ifdef HASH_LITTLE
            shaLenEncode(msg_512_high, tempBuf, blockSize - 16);
            shaLenEncode(msg_512_low, tempBuf, blockSize - 8);
#else
            /* pad 128 bit msg len */
            SCE_U64ToU32(&msg_512_high, &calcBuf[28], 1);
            SCE_U64ToU32(&msg_512_low, &calcBuf[30], 1);
#endif
        }

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + count * blockSize, tempBuf, blockSize);
#else
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_LKEY + count * blockSize, calcBuf, blockSize);
#endif

        count++;

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_LKEY, blockSize * count);
#else
        printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_LKEY, (blockSize / 4) * count);
#endif
#endif

        if (firstSign == 1)
        {
            SCE_HASH_OPT2_FIRT_BLOCK();
            firstSign = 0;
        }
        else
        {

            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
        }
        SCE_HASH_OPT1_CNT(count - 1);
        count = 0;
        SCE_HASH_SET_SEG_LKEY(0);
        SCE_HASH_SET_SEG_KEY(0);
#ifdef HASH_LITTLE
        SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_LKEY | SCE_HASH_OPT3_SEG_RESULT);
#else
        SCE_HASH_OPT3_MODE(0);
#endif
        SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_HMAC256_KEYHASH : HF_HMAC512_KEYHASH);

#ifdef HASH_DEBUG
        printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
        printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
        printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
        printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

        SCE_HASH_AR();
        SCE_HASH_MFSM_DONE();

        /* Clear the subsequent values of keyhash */
        memset((uint8_t *)(ADDR_HASH_SEG_KEY) + digestSize, 0, blockSize - digestSize);
    }
    else
    {
        /* Pad the key with zeros to meet SHA256_BLOCK_SIZE */
        memset(tempBuf, 0, sizeof(tempBuf));
        memcpy(tempBuf, key, keylen);
#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_KEY, tempBuf, blockSize);
#else
        SCE_U8ToU32(tempBuf, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_KEY, calcBuf, blockSize);
#endif
    }

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("skey", (uint8_t *)ADDR_HASH_SEG_KEY, blockSize);
#else
    printBufferU32("skey", (uint32_t *)ADDR_HASH_SEG_KEY, blockSize / 4);
#endif
#endif
}

/**
 * @brief HMAC-SHA256/SHA512 message input
 *
 */
uint32_t hmac_sha256_512_MsgInput(const uint8_t *msg, uint32_t msgLen, uint8_t cacl_mode)
{
    uint64_t msgBitLen, msg_512_high, msg_512_low, temp;
    uint32_t i, loop, remainLen, blockSize;
    uint8_t  firstSign;
    firstSign = 1;
    uint8_t  tempBuf[128];
    uint32_t calcBuf[32], maxCount, count, count_bak;

    count_bak = 0;

#ifdef HASH_DEBUG
    uint32_t digestSize;
#endif

    i = 0;
    if (cacl_mode == SHA256_MODE)
    {
        blockSize = SHA256_BLOCK_SIZE;
#ifdef HASH_DEBUG
        digestSize = SHA256_DIGEST_SIZE;
#endif
        loop      = msgLen / SHA256_BLOCK_SIZE;
        remainLen = msgLen % SHA256_BLOCK_SIZE;
        msgBitLen = msgLen * 8 + SHA256_BLOCK_SIZE * 8; /* Add the bit length of one block of key */
    }
    else
    {
        blockSize = SHA512_BLOCK_SIZE;
#ifdef HASH_DEBUG
        digestSize = SHA512_DIGEST_SIZE;
#endif
        loop         = msgLen / SHA512_BLOCK_SIZE;
        remainLen    = msgLen % SHA512_BLOCK_SIZE;
        msg_512_high = 0;
        msg_512_low  = SHA512_BLOCK_SIZE * 8; /* Add the bit length of one block of key */
    }

    maxCount = 512 / blockSize;
    count    = 0;

    for (i = 0; i < loop; i++)
    {
        if (cacl_mode == SHA512_MODE) /* SHA512 needs to calculate the bit length of the MSG message */
        {
            temp = msg_512_low + SHA512_BLOCK_SIZE * 8;
            if (temp < msg_512_low)
            {
                msg_512_high++;
            }
            msg_512_low = temp;
        }

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, msg + i * blockSize, blockSize);
#else
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(msg + i * blockSize, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif

        count++;
        if (count >= maxCount) /* Calculate every 512 bytes */
        {

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
            printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

            if (firstSign == 1)
            {
                SCE_HASH_OPT2_FIRT_BLOCK();
                firstSign = 0;
            }
            else
            {
                SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            }

            SCE_HASH_OPT1_CNT(count - 1);
            count_bak += count;
            count = 0;
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);
#ifdef HASH_LITTLE
            SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
            SCE_HASH_OPT3_MODE(0);
#endif
            SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_HMAC256_PASS1 : HF_HMAC512_PASS1);

#ifdef HASH_DEBUG
            printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
            printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
            printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
            printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
            printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif
        }
    }
    /* pad msg */
    if (cacl_mode == SHA512_MODE) /* SHA512 needs to calculate the bit length of the MSG message */
    {
        temp = msg_512_low + remainLen * 8;
        if (temp < msg_512_low)
        {
            msg_512_high++;
        }
        msg_512_low = temp;
    }

    if (remainLen < ((cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16)))
    {
        memset(tempBuf, 0, sizeof(tempBuf));
        memcpy(tempBuf, msg + i * blockSize, remainLen);
        tempBuf[remainLen] = HASH_PAD;
    }
    else
    {
        memset(tempBuf, 0, sizeof(tempBuf));
        memcpy(tempBuf, msg + i * blockSize, remainLen);
        tempBuf[remainLen] = HASH_PAD;

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, tempBuf, blockSize);
#else
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(tempBuf, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif

        count++;
        if (count >= maxCount) /* Calculate every 512 bytes */
        {
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
            printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

            if (firstSign == 1)
            {
                SCE_HASH_OPT2_FIRT_BLOCK();
                firstSign = 0;
            }
            else
            {
                SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            }

            SCE_HASH_OPT1_CNT(count - 1);
            count_bak += count;
            count = 0;

            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);
#ifdef HASH_LITTLE
            SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
            SCE_HASH_OPT3_MODE(0);
#endif
            SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_HMAC256_PASS1 : HF_HMAC512_PASS1);

#ifdef HASH_DEBUG
            printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
            printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
            printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
            printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
            printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif
        }
        /* tempBuf still holds the extra block data; clear it so the final
         * length-only block starts from all-zeros regardless of whether a
         * batch flush fired above. */
        memset(tempBuf, 0, sizeof(tempBuf));
    }

    /* Convert BYTE array to U32 array, then write to HASH_MSG */
    SCE_U8ToU32(tempBuf, calcBuf, (cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16));

    if (cacl_mode == SHA256_MODE)
    {
#ifdef HASH_LITTLE
        shaLenEncode(msgBitLen, tempBuf, blockSize - 8);
#else
        /* pad 64 bit msg len */
        SCE_U64ToU32(&msgBitLen, &calcBuf[14], 1);
#endif
    }
    else
    {
#ifdef HASH_LITTLE
        shaLenEncode(msg_512_high, tempBuf, blockSize - 16);
        shaLenEncode(msg_512_low, tempBuf, blockSize - 8);
#else
        /* pad 128 bit msg len */
        SCE_U64ToU32(&msg_512_high, &calcBuf[28], 1);
        SCE_U64ToU32(&msg_512_low, &calcBuf[30], 1);
#endif
    }
#ifdef HASH_LITTLE
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, tempBuf, blockSize);
#else
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif

    count++;

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
    printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

    if (firstSign == 1)
    {
        SCE_HASH_OPT2_FIRT_BLOCK();
        firstSign = 0;
    }
    else
    {
        SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    }
    SCE_HASH_OPT1_CNT(count - 1);
    count_bak += count;
    count = 0;
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
#ifdef HASH_LITTLE
    SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
    SCE_HASH_OPT3_MODE(0);
#endif
    SCE_HASH_FUNC((cacl_mode == SHA256_MODE) ? HF_HMAC256_PASS1 : HF_HMAC512_PASS1);

#ifdef HASH_DEBUG
    printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
    printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
    printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
    printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
    printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif
    return count_bak;
}

void hmac_sha256_512_Result(uint8_t *mac, uint32_t msgCount, uint8_t cacl_mode)
{
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
#ifdef HASH_LITTLE
    SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
    SCE_HASH_OPT3_MODE(0);
#endif
    SCE_HASH_OPT2_FIRT_BLOCK();
    SCE_HASH_OPT1_CNT(msgCount - 1);
    SCE_HASH_FUNC(cacl_mode == SHA256_MODE ? HF_HMAC256_PASS2 : HF_HMAC512_PASS2);

#ifdef HASH_DEBUG
    printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
    printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
    printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
    printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

    if (mac == NULL)
    {
        return;
    }

    if (cacl_mode == SHA256_MODE)
    {
#ifdef HASH_LITTLE
        memcpy(mac, (uint8_t *)ADDR_HASH_SEG_HOUT, SHA256_DIGEST_SIZE);
#ifdef HASH_DEBUG
        bnu_print_mem("HMAC-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, SHA256_DIGEST_SIZE);
#endif
#else
        SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, mac, SHA256_DIGEST_SIZE / 4);
#ifdef HASH_DEBUG
        printBufferU32("HMAC-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, SHA256_DIGEST_SIZE / 4);
#endif
#endif
    }
    else
    {

#ifdef HASH_LITTLE
        memcpy(mac, (uint8_t *)ADDR_HASH_SEG_HOUT, SHA512_DIGEST_SIZE);
#ifdef HASH_DEBUG
        bnu_print_mem("HMAC-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, SHA512_DIGEST_SIZE);
#endif
#else
        SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, mac, SHA512_DIGEST_SIZE / 4);
#ifdef HASH_DEBUG
        printBufferU32("HMAC-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, SHA512_DIGEST_SIZE / 4);
#endif
#endif
    }
}

/**
 * @brief HMAC-SHA256/SHA512 key initialization step
 *
 */
void hmac_sha256_512_KeyInit_Step(const uint8_t *key, uint32_t keylen, uint8_t cacl_mode)
{
    hmac_sha256_512_KeyInit(key, keylen, cacl_mode);
    memset(&HMAC_INFO, 0, sizeof(HMAC_INFO));
    HMAC_INFO.sign = 1; /* First block flag */
}

/**
 * @brief HMAC-SHA256/SHA512 message input step
 *
 */
void hmac_sha256_512_MsgInput_Step(const uint8_t *msg, uint32_t msgLen, uint8_t cacl_mode)
{
    uint64_t temp;
    uint32_t blockSize, function, inOffset;
    uint32_t maxCount, count;
    uint8_t  hash_opt2;
    uint32_t digestSize;

    digestSize = (cacl_mode == SHA256_MODE) ? SHA256_DIGEST_SIZE : SHA512_DIGEST_SIZE;
    blockSize  = (cacl_mode == SHA256_MODE) ? SHA256_BLOCK_SIZE : SHA512_BLOCK_SIZE;
    function   = (cacl_mode == SHA256_MODE) ? HF_HMAC256_PASS1 : HF_HMAC512_PASS1;

    maxCount = 512 / blockSize;
    count    = 0;

    inOffset = 0;
    while ((msgLen + HMAC_INFO.dataLen) >= blockSize)
    {
        memcpy(HMAC_INFO.data + HMAC_INFO.dataLen, msg + inOffset, blockSize - HMAC_INFO.dataLen);

        temp = HMAC_INFO.msgBit_L + (blockSize - HMAC_INFO.dataLen) * 8;

        if (temp < HMAC_INFO.msgBit_L)
        {
            HMAC_INFO.msgBit_H++;
        }
        HMAC_INFO.msgBit_L = temp;

#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, HMAC_INFO.data, blockSize);
#else
        SCE_U8ToU32(HMAC_INFO.data, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG + count * blockSize, calcBuf, blockSize);
#endif
        count++;

        inOffset += blockSize - HMAC_INFO.dataLen;
        msgLen -= blockSize - HMAC_INFO.dataLen;
        HMAC_INFO.dataLen = 0;
        memset(HMAC_INFO.data, 0, sizeof(HMAC_INFO.data));

        if (count >= maxCount) // Calculate every 512 bytes
        {

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
            printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

            if (HMAC_INFO.sign == 1)
            {
                hash_opt2      = HASH_IF_START;
                HMAC_INFO.sign = 0;
            }
            else
            {
                hash_opt2 = 0;
            }

            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);

#ifdef HASH_LITTLE
            SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
            SCE_HASH_OPT3_MODE(0);
#endif

            hash_fun_Execute(count - 1, hash_opt2, function);

            HMAC_INFO.msgCount += count;

            count = 0;

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
            bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
            printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif

#ifdef HASH_LITTLE
            SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, HMAC_INFO.h, digestSize / 4);
#else
            SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, HMAC_INFO.h, digestSize / 4);
#endif
        }
    }

    if (count != 0)
    {

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize * count);
#else
        printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, (blockSize / 4) * count);
#endif
#endif

        SCE_HASH_OPT1_CNT(count - 1);
        HMAC_INFO.msgCount += count;
        count = 0;
        if (HMAC_INFO.sign == 1)
        {
            SCE_HASH_OPT2_FIRT_BLOCK();
            HMAC_INFO.sign = 0;
        }
        else
        {
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
        }
        SCE_HASH_SET_MSG(0);
        SCE_HASH_OUTPUT(0);
        SCE_HASH_FUNC(function);
#ifdef HASH_LITTLE
        SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
        SCE_HASH_OPT3_MODE(0);
#endif
#ifdef HASH_DEBUG
        printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
        printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
        printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
        printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif

        SCE_HASH_AR();
        SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
        printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif

#ifdef HASH_LITTLE
        SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, HMAC_INFO.h, digestSize / 4);
#else
        SCE_U32ToU8((uint32_t *)ADDR_HASH_SEG_HOUT, HMAC_INFO.h, digestSize / 4);
#endif
    }

    temp = HMAC_INFO.msgBit_L + msgLen * 8;

    if (temp < HMAC_INFO.msgBit_L)
    {
        HMAC_INFO.msgBit_H++;
    }
    HMAC_INFO.msgBit_L = temp;

    memcpy(HMAC_INFO.data + HMAC_INFO.dataLen, msg + inOffset, msgLen);
    HMAC_INFO.dataLen += msgLen;
}

/**
 * @brief sha256/sha512 result step
 *
 * @param msg input message
 * @param msgLen input message length
 * @param[out] mac output mac
 * @param cacl_mode hash mode SHA256_MODE or SHA512_MODE
 */
void hmac_sha256_512_Result_Step(uint8_t *msg, uint32_t msgLen, uint8_t *mac, uint8_t cacl_mode)
{
    uint64_t temp;
    uint32_t blockSize, function;

#ifdef HASH_DEBUG
    uint32_t digestSize;
    digestSize = (cacl_mode == SHA256_MODE) ? SHA256_DIGEST_SIZE : SHA512_DIGEST_SIZE;
#endif

    if (mac == NULL)
        return;

    if (msg != NULL && msgLen != 0)
    {
        hmac_sha256_512_MsgInput_Step(msg, msgLen, cacl_mode); /* Calculate the last packet of data */
    }

    blockSize = (cacl_mode == SHA256_MODE) ? SHA256_BLOCK_SIZE : SHA512_BLOCK_SIZE;
    function  = (cacl_mode == SHA256_MODE) ? HF_HMAC256_PASS1 : HF_HMAC512_PASS1;

    /* pad msg, length of the key */
    temp = HMAC_INFO.msgBit_L + blockSize * 8;
    if (temp < HMAC_INFO.msgBit_L)
    {
        HMAC_INFO.msgBit_H++;
    }
    HMAC_INFO.msgBit_L = temp;

    if (HMAC_INFO.dataLen < ((cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16)))
    {
        HMAC_INFO.data[HMAC_INFO.dataLen] = HASH_PAD;
    }
    else
    {
        HMAC_INFO.data[HMAC_INFO.dataLen] = HASH_PAD;

        if (HMAC_INFO.sign == 1)
        {
            SCE_HASH_OPT2_FIRT_BLOCK();
            HMAC_INFO.sign = 0;
        }
        else
        {
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
        }
#ifdef HASH_LITTLE
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, HMAC_INFO.data, blockSize);
#else
        /* Convert BYTE array to U32 array, then write to HASH_MSG */
        SCE_U8ToU32(HMAC_INFO.data, calcBuf, blockSize);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, calcBuf, blockSize);
#endif

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize);
#else
        printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, blockSize / 4);
#endif
#endif

        HMAC_INFO.msgCount++;
        SCE_HASH_OPT1_CNT(0);
        SCE_HASH_SET_MSG(0);
        SCE_HASH_OUTPUT(0);
        SCE_HASH_FUNC(function);
#ifdef HASH_LITTLE
        SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
        SCE_HASH_OPT3_MODE(0);
#endif
#ifdef HASH_DEBUG
        printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
        printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
        printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
        printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif
        SCE_HASH_AR();
        SCE_HASH_MFSM_DONE();
#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
        bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
        printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif
        memset(HMAC_INFO.data, 0, sizeof(HMAC_INFO.data));
    }

#ifndef HASH_LITTLE
    /* Convert BYTE array to U32 array, then write to HASH_MSG */
    SCE_U8ToU32(HMAC_INFO.data, calcBuf, (cacl_mode == SHA256_MODE) ? (SHA256_BLOCK_SIZE - 8) : (SHA512_BLOCK_SIZE - 16));
#endif

    if (cacl_mode == SHA256_MODE)
    {
#ifdef HASH_LITTLE
        shaLenEncode(HMAC_INFO.msgBit_L, HMAC_INFO.data, blockSize - 8);
#else
        /* pad 64 bit msg len */
        SCE_U64ToU32(&HMAC_INFO.msgBit_L, &calcBuf[14], 1);
#endif
    }
    else
    {
#ifdef HASH_LITTLE
        shaLenEncode(HMAC_INFO.msgBit_H, HMAC_INFO.data, blockSize - 16);
        shaLenEncode(HMAC_INFO.msgBit_L, HMAC_INFO.data, blockSize - 8);
#else
        /* pad 128 bit msg len */
        SCE_U64ToU32(&HMAC_INFO.msgBit_H, &calcBuf[28], 1);
        SCE_U64ToU32(&HMAC_INFO.msgBit_L, &calcBuf[30], 1);
#endif
    }

    if (HMAC_INFO.sign == 1)
    {
        SCE_HASH_OPT2_FIRT_BLOCK();
        HMAC_INFO.sign = 0;
    }
    else
    {
        SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    }

#ifdef HASH_LITTLE
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, HMAC_INFO.data, blockSize);
#else
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, calcBuf, blockSize);
#endif

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK", (uint8_t *)ADDR_HASH_SEG_MSG, blockSize);
#else
    printBufferU32("BLOCK", (uint32_t *)ADDR_HASH_SEG_MSG, blockSize / 4);
#endif
#endif

    HMAC_INFO.msgCount++;
    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_FUNC(function);
#ifdef HASH_LITTLE
    SCE_HASH_OPT3_MODE(SCE_HASH_OPT3_SEG_MSG | SCE_HASH_OPT3_SEG_HOUT | SCE_HASH_OPT3_SEG_KEY | SCE_HASH_OPT3_SEG_RESULT);
#else
    SCE_HASH_OPT3_MODE(0);
#endif
#ifdef HASH_DEBUG
    printf("REG_SCE_HASH_OPT1 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT1);
    printf("REG_SCE_HASH_OPT2 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT2);
    printf("REG_SCE_HASH_OPT3 = %08" PRIx32 "\r\n", REG_SCE_HASH_OPT3);
    printf("REG_SCE_HASH_CRFUNC = %08" PRIx32 "\r\n", REG_SCE_HASH_CRFUNC);
#endif
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

#ifdef HASH_DEBUG
#ifdef HASH_LITTLE
    bnu_print_mem("BLOCK-OUT", (uint8_t *)ADDR_HASH_SEG_HOUT, digestSize);
#else
    printBufferU32("BLOCK-OUT", (uint32_t *)ADDR_HASH_SEG_HOUT, digestSize / 4);
#endif
#endif

    hmac_sha256_512_Result(mac, HMAC_INFO.msgCount, cacl_mode);
}

/*-------------------------------------------------------------------------------------------------------------------------------------*/
blake2s_state blakeInfo;

static void blake2s_increment_counter(uint32_t inc)
{
    blakeInfo.t[0] += inc;
    blakeInfo.t[1] += (blakeInfo.t[0] < inc);
}

static void blake2s_set_lastblock(void)
{
    blakeInfo.f[0] = (uint32_t)-1;
}

/**
 * @brief BLAKE2s compress (BLAKE algorithm requires little-endian data, so no endian conversion is performed in the BLAKE algorithm)
 *
 */
void blake2s_compress(void)
{
    uint32_t *ptr;
    ptr = (uint32_t *)ADDR_HASH_SEG_HOUT;

    memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, blakeInfo.h, BLAKE2S_OUTBYTES);
    memcpy_u32(ptr + 12, &blakeInfo.t[0], 8);
    memcpy_u32(ptr + 14, &blakeInfo.f[0], 8);

    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, blakeInfo.buf, BLAKE2S_BLOCKBYTES);

    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_OPT3_MODE(0);
    SCE_HASH_FUNC(HF_BLK2S);

#ifdef HASH_DEBUG
    // printf("REG_SCE_HASH_OPT1 = %08x\r\n", REG_SCE_HASH_OPT1);
    // printf("REG_SCE_HASH_OPT2 = %08x\r\n", REG_SCE_HASH_OPT2);
    // printf("REG_SCE_HASH_OPT3 = %08x\r\n", REG_SCE_HASH_OPT3);
    // printf("REG_SCE_HASH_CRFUNC = %08x\r\n", REG_SCE_HASH_CRFUNC);
#endif
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();
    SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)blakeInfo.h, BLAKE2S_OUTBYTES / 4);
}

/**
 * @brief BLAKE2s init
 *
 * @return int
 */
int blake2s_init(void)
{
    memset(&blakeInfo, 0, sizeof(blakeInfo));
    memcpy(blakeInfo.h, BLK2S_EX, sizeof(BLK2S_EX));
    blakeInfo.outlen = BLAKE2S_OUTBYTES;
    return 0;
}

/**
 * @brief BLAKE2s update
 *
 * @param indata input data
 * @param indataLen input data length
 * @return int
 */
int blake2s_update(const uint8_t *indata, uint32_t indataLen)
{
    const uint8_t *in = indata;
    if (indataLen > 0)
    {
        uint32_t left = blakeInfo.buflen;
        uint32_t fill = BLAKE2S_BLOCKBYTES - left;
        if (indataLen > fill)
        {
            blakeInfo.buflen = 0;
            memcpy(blakeInfo.buf + left, in, fill); /* Fill buffer */
            blake2s_increment_counter(BLAKE2S_BLOCKBYTES);
            blake2s_compress(); /* Compress */
            in += fill;
            indataLen -= fill;
            while (indataLen > BLAKE2S_BLOCKBYTES)
            {
                memcpy(blakeInfo.buf, in, BLAKE2S_BLOCKBYTES);
                blake2s_increment_counter(BLAKE2S_BLOCKBYTES);
                blake2s_compress();
                in += BLAKE2S_BLOCKBYTES;
                indataLen -= BLAKE2S_BLOCKBYTES;
            }
        }
        memcpy(blakeInfo.buf + blakeInfo.buflen, in, indataLen);
        blakeInfo.buflen += indataLen;
    }
    return 0;
}

/**
 * @brief BLAKE2s final
 *
 * @param out output data
 * @param outlen output data length
 * @return int
 */
int blake2s_final(uint8_t *out, uint32_t outlen)
{

    blake2s_increment_counter(blakeInfo.buflen);
    blake2s_set_lastblock();
    memset(blakeInfo.buf + blakeInfo.buflen, 0, BLAKE2S_BLOCKBYTES - blakeInfo.buflen); /* Padding */
    blake2s_compress();
    memcpy(out, blakeInfo.h, outlen);
    return 0;
}

/**
 * @brief BLAKE2s one-time calculation
 *
 * @param out output data
 * @param outlen output data length
 * @param in input data
 * @param inlen input data length
 * @return int
 */
int blake2s(uint8_t *out, uint32_t outlen, const uint8_t *in, uint32_t inlen)
{
    blake2s_init();
    blake2s_update(in, inlen);
    blake2s_final(out, outlen);
    return 0;
}

blake2b_state blakeInfo_b;

/**
 * @brief BLAKE2b increment counter
 *
 * @param inc increment value
 */
static void blake2b_increment_counter(uint32_t inc)
{
    blakeInfo_b.t[0] += inc;
    blakeInfo_b.t[1] += (blakeInfo_b.t[0] < inc);
}

/**
 * @brief  blake2b set last block
 *
 */
static void blake2b_set_lastblock(void)
{
    blakeInfo_b.f[0] = 0xFFFFFFFFFFFFFFFF;
}

/**
 * @brief BLAKE2b compress
 *
 */
void blake2b_compress(void)
{
    uint32_t *ptr;
    ptr = (uint32_t *)ADDR_HASH_SEG_HOUT;

    memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, blakeInfo_b.h, BLAKE2B_OUTBYTES);

    memcpy_u32(ptr + 24, &blakeInfo_b.t[0], 8 * 4);
    memset((uint8_t *)ADDR_HASH_SEG_MSG, 0, BLAKE2B_BLOCKBYTES);

    /* 0x0011223344556677 The algorithm core expects U64 data as 0x7766554433221100, so direct copy is sufficient */
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, (uint8_t *)blakeInfo_b.buf, BLAKE2B_BLOCKBYTES);

    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_OPT3_MODE(0);
    SCE_HASH_FUNC(HF_BLK2B);

#ifdef HASH_DEBUG
    // printf("REG_SCE_HASH_OPT1 = %08x\r\n", REG_SCE_HASH_OPT1);
    // printf("REG_SCE_HASH_OPT2 = %08x\r\n", REG_SCE_HASH_OPT2);
    // printf("REG_SCE_HASH_OPT3 = %08x\r\n", REG_SCE_HASH_OPT3);
    // printf("REG_SCE_HASH_CRFUNC = %08x\r\n", REG_SCE_HASH_CRFUNC);
#endif
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

    SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)blakeInfo_b.h, BLAKE2B_OUTBYTES / 4);
}

/**
 * @brief BLAKE2b init
 *
 * @return int
 */
int blake2b_init(void)
{
    memset(&blakeInfo_b, 0, sizeof(blakeInfo_b));
    memcpy(blakeInfo_b.h, BLK2B_EX, sizeof(BLK2B_EX));
    blakeInfo_b.outlen = BLAKE2B_OUTBYTES;
    return 0;
}

/**
 * @brief BLAKE2b update
 *
 * @param indata input data
 * @param indataLen input data length
 * @return int
 */
int blake2b_update(const uint8_t *indata, uint32_t indataLen)
{
    const uint8_t *in = indata;
    if (indataLen > 0)
    {
        uint32_t left = blakeInfo_b.buflen;
        uint32_t fill = BLAKE2B_BLOCKBYTES - left;
        if (indataLen > fill)
        {
            blakeInfo_b.buflen = 0;
            memcpy(blakeInfo_b.buf + left, in, fill); /* Fill buffer */
            blake2b_increment_counter(BLAKE2B_BLOCKBYTES);
            blake2b_compress(); /* Compress */
            in += fill;
            indataLen -= fill;
            while (indataLen > BLAKE2B_BLOCKBYTES)
            {
                memcpy(blakeInfo_b.buf, in, BLAKE2B_BLOCKBYTES);
                blake2b_increment_counter(BLAKE2B_BLOCKBYTES);
                blake2b_compress();
                in += BLAKE2B_BLOCKBYTES;
                indataLen -= BLAKE2B_BLOCKBYTES;
            }
        }
        memcpy(blakeInfo_b.buf + blakeInfo_b.buflen, in, indataLen);
        blakeInfo_b.buflen += indataLen;
    }
    return 0;
}

/**
 * @brief BLAKE2b final
 *
 * @param out output data
 * @param outlen output data length
 * @return int
 */
int blake2b_final(uint8_t *out, uint32_t outlen)
{
    blake2b_increment_counter(blakeInfo_b.buflen);
    blake2b_set_lastblock();
    memset(blakeInfo_b.buf + blakeInfo_b.buflen, 0, BLAKE2B_BLOCKBYTES - blakeInfo_b.buflen); /* Padding */
    blake2b_compress();
    memcpy(out, blakeInfo_b.h, outlen);
    return 0;
}

/**
 * @brief BLAKE2b one-time calculation
 *
 * @param out output data
 * @param outlen output data length
 * @param in input data
 * @param inlen input data length
 * @return int
 */
int blake2b(uint8_t *out, uint32_t outlen, const uint8_t *in, uint32_t inlen)
{
    blake2b_init();
    blake2b_update(in, inlen);
    blake2b_final(out, outlen);
    return 0;
}

blake3_state blakeInfo3;
/**
 * @brief BLAKE3 init
 *
 * @return int
 */
int blake3_init(void)
{
    memset(&blakeInfo3, 0, sizeof(blake3_state));
    memcpy(blakeInfo3.cv_buf, BLK3_H, sizeof(BLK3_H));
    blakeInfo3.cv     = blakeInfo3.cv_buf;
    blakeInfo3.outlen = BLAKE3_OUTBYTES;
    return 1;
}

/**
 * @brief BLAKE3 compress
 *
 * @param len
 */
void blake3_compress(uint32_t len)
{
    uint32_t  flags;
    uint32_t *ptr, *cv;
    uint64_t  t;

    cv  = blakeInfo3.cv;
    ptr = (uint32_t *)ADDR_HASH_SEG_HOUT;

    flags = 0;
    switch (blakeInfo3.block)
    {
        case 0:
            flags |= CHUNK_START;
            break;
        case 15:
            flags |= CHUNK_END;
            break;
    }

    memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, cv, BLAKE3_OUTBYTES);

    *(ptr + 12) = (uint32_t)blakeInfo3.chunk;
    *(ptr + 13) = (uint32_t)(blakeInfo3.chunk >> 32);
    *(ptr + 14) = len;
    *(ptr + 15) = flags;
    memset((uint8_t *)ADDR_HASH_SEG_MSG, 0, BLAKE3_BLOCKBYTES);

    /* 0x0011223344556677 The algorithm core expects U64 data as 0x7766554433221100, so direct copy is sufficient */
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, (uint8_t *)blakeInfo3.buf, BLAKE3_BLOCKBYTES);

    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_OPT3_MODE(0);
    SCE_HASH_FUNC(HF_BLK3);
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

    SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)cv, BLAKE3_OUTBYTES / 4);

    if (++blakeInfo3.block == 16)
    {
        blakeInfo3.block = 0;
        for (t = ++blakeInfo3.chunk; (t & 1) == 0; t >>= 1)
        {
            cv -= 8;
            memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, BLK3_H, sizeof(BLK3_H));

            *(ptr + 14) = 64;
            *(ptr + 15) = PARENT;
            memset((uint8_t *)ADDR_HASH_SEG_MSG, 0, BLAKE3_BLOCKBYTES);
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, cv, BLAKE3_BLOCKBYTES);

            SCE_HASH_OPT1_CNT(0);
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);
            SCE_HASH_OPT3_MODE(0);
            SCE_HASH_FUNC(HF_BLK3);
            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();
            SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)cv, BLAKE3_OUTBYTES / 4);
        }
        cv += 8;
        memcpy(cv, BLK3_H, sizeof(BLK3_H));
    }
    blakeInfo3.cv = cv;
}

/**
 * @brief BLAKE3 update
 *
 * @param indata input data
 * @param indataLen input data length
 */
void blake3_update(const uint8_t *indata, uint32_t indataLen)
{
    const uint8_t *in = indata;

    if (indataLen > 0)
    {
        uint32_t left = blakeInfo3.buflen;
        uint32_t fill = BLAKE3_BLOCKBYTES - left;

        if (indataLen > fill) // Input is greater than the remaining bytes in the buffer, compress once
        {
            blakeInfo3.buflen = 0;
            memcpy(blakeInfo3.buf + left, in, fill); /* Fill buffer */
            blake3_compress(BLAKE3_BLOCKBYTES);
            in += fill;
            indataLen -= fill;

            while (indataLen > BLAKE3_BLOCKBYTES)
            {
                memcpy(blakeInfo3.buf, in, BLAKE3_BLOCKBYTES);
                blake3_compress(BLAKE3_BLOCKBYTES);
                in += BLAKE3_BLOCKBYTES;
                indataLen -= BLAKE3_BLOCKBYTES;
            }
        }
        memcpy(blakeInfo3.buf + blakeInfo3.buflen, in, indataLen);
        blakeInfo3.buflen += indataLen;
    }
}

/**
 * @brief BLAKE3 final
 *
 * @param out output data
 * @param outlen output data length
 * @return int
 */
int blake3_final(uint8_t *out, uint32_t outlen)
{
    uint32_t flags, b, *in, *cv, *ptr;

    cv  = blakeInfo3.cv;
    ptr = (uint32_t *)ADDR_HASH_SEG_HOUT;
    memset(blakeInfo3.buf + blakeInfo3.buflen, 0, BLAKE3_BLOCKBYTES - blakeInfo3.buflen); /* Padding */

    flags = CHUNK_END;
    if (blakeInfo3.block == 0)
    {
        flags |= CHUNK_START;
    }

    if (cv == blakeInfo3.cv_buf)
    {
        b  = blakeInfo3.buflen;
        in = (uint32_t *)blakeInfo3.buf;
    }
    else
    {
        memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, cv, BLAKE3_OUTBYTES);

        *(ptr + 12) = (uint32_t)blakeInfo3.chunk;
        *(ptr + 13) = (uint32_t)(blakeInfo3.chunk >> 32);
        *(ptr + 14) = blakeInfo3.buflen;
        *(ptr + 15) = flags;
        memset((uint8_t *)ADDR_HASH_SEG_MSG, 0, BLAKE3_BLOCKBYTES);

        /* 0x0011223344556677 The algorithm core expects U64 data as 0x7766554433221100, so direct copy is sufficient */
        memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, (uint8_t *)blakeInfo3.buf, BLAKE3_BLOCKBYTES);

        SCE_HASH_OPT1_CNT(0);
        SCE_HASH_OPT2_FIRT_BLOCK_CLR();
        SCE_HASH_SET_MSG(0);
        SCE_HASH_OUTPUT(0);
        SCE_HASH_OPT3_MODE(0);
        SCE_HASH_FUNC(HF_BLK3);
        SCE_HASH_AR();
        SCE_HASH_MFSM_DONE();
        SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)cv, BLAKE3_OUTBYTES / 4);

        flags = PARENT;
        while ((cv -= 8) != blakeInfo3.cv_buf)
        {
            memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, BLK3_H, sizeof(BLK3_H));

            *(ptr + 14) = 64;
            *(ptr + 15) = flags;
            memset((uint8_t *)ADDR_HASH_SEG_MSG, 0, BLAKE3_BLOCKBYTES);
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, cv, BLAKE3_BLOCKBYTES);

            SCE_HASH_OPT1_CNT(0);
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);
            SCE_HASH_OPT3_MODE(0);
            SCE_HASH_FUNC(HF_BLK3);
            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();
            SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)cv, BLAKE3_OUTBYTES / 4);
        }

        b  = 64;
        in = cv;
        cv = (uint32_t *)BLK3_H;
    }
    flags |= ROOT;

    memset((uint8_t *)ADDR_HASH_SEG_HOUT, 0, 256);
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, cv, BLAKE3_OUTBYTES);

    *(ptr + 14) = b;
    *(ptr + 15) = flags;
    memset((uint8_t *)ADDR_HASH_SEG_MSG, 0, BLAKE3_BLOCKBYTES);

    /* 0x0011223344556677 The algorithm core expects U64 data as 0x7766554433221100, so direct copy is sufficient */
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, in, BLAKE3_BLOCKBYTES);

    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_OPT3_MODE(0);
    SCE_HASH_FUNC(HF_BLK3);
    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();
    SCE_U32_COPY((uint32_t *)ADDR_HASH_SEG_HOUT, (uint8_t *)out, BLAKE3_OUTBYTES / 4);
    return 0;
}

/**
 * @brief BLAKE3 one-time calculation
 *
 * @param out output data
 * @param outlen output data length
 * @param in input data
 * @param inlen input data length
 * @return int
 */
int blake3(uint8_t *out, uint32_t outlen, const uint8_t *in, uint32_t inlen)
{
    blake3_init();
    blake3_update(in, inlen);
    blake3_final(out, outlen);
    return 0;
}

/**
 * @brief SHA3 init
 *
 * @param c sha3 context
 * @param mdlen message digest length
 * @return int
 */
int sha3_init(sha3_ctx_t *c, int mdlen)
{
    int i;

    for (i = 0; i < 50; i++)
        c->st.q[i] = 0;
    c->mdlen = mdlen;
    c->rsiz  = 200 - 2 * mdlen;
    c->pt    = 0;

    return 1;
}

/**
 * @brief SHA3 update
 *
 * @param c sha3 context
 * @param data input data
 * @param len input data length
 * @return int
 */
int sha3_update(sha3_ctx_t *c, const void *data, size_t len)
{
    size_t   i;
    int      j, k;
    uint32_t temp;

    j = c->pt;
    for (i = 0; i < len; i++)
    {
        c->st.b[j++] ^= ((const uint8_t *)data)[i];
        if (j >= c->rsiz)
        {
            /* format ADDR_HASH_SEG_MSG  :
             *  msg in byte 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77 require to msg in word 0x77665544,0x33221100
             */
            for (k = 0; k < 200 / 4; k += 2)
            {
                temp           = c->st.q[k];
                c->st.q[k]     = c->st.q[k + 1];
                c->st.q[k + 1] = temp;
            }

            /* Fill data into ADDR_HASH_SEG_MSG */
            memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, c->st.b, 200);

            SCE_HASH_OPT1_CNT(0);
            SCE_HASH_OPT2_FIRT_BLOCK_CLR();
            SCE_HASH_SET_MSG(0);
            SCE_HASH_OUTPUT(0);
            SCE_HASH_OPT3_MODE(0);

            SCE_HASH_FUNC(HF_SHA3);

            SCE_HASH_AR();
            SCE_HASH_MFSM_DONE();

            for (k = 0; k < 200 / 4; k += 2)
            {
                c->st.q[k]     = ((uint32_t *)ADDR_HASH_SEG_HOUT)[k + 1];
                c->st.q[k + 1] = ((uint32_t *)ADDR_HASH_SEG_HOUT)[k];
            }

            j = 0;
        }
    }
    c->pt = j;

    return 1;
}

/**
 * @brief SHA3 final
 *
 * @param md output data
 * @param c sha3 context
 * @return int
 */
int sha3_final(void *md, sha3_ctx_t *c)
{
    int      i, k;
    uint32_t temp;

    c->st.b[c->pt] ^= 0x06;
    c->st.b[c->rsiz - 1] ^= 0x80;

    /* format ADDR_HASH_SEG_MSG  :
     *  msg in byte 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77 require to msg in word 0x77665544,0x33221100
     */
    for (k = 0; k < 200 / 4; k += 2)
    {
        temp           = c->st.q[k];
        c->st.q[k]     = c->st.q[k + 1];
        c->st.q[k + 1] = temp;
    }

    /* Fill data into ADDR_HASH_SEG_MSG */
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, c->st.b, 200);

    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_OPT3_MODE(0);

    SCE_HASH_FUNC(HF_SHA3);

    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();

    for (k = 0; k < 200 / 4; k += 2)
    {
        c->st.q[k]     = ((uint32_t *)ADDR_HASH_SEG_HOUT)[k + 1];
        c->st.q[k + 1] = ((uint32_t *)ADDR_HASH_SEG_HOUT)[k];
    }

    for (i = 0; i < c->mdlen; i++)
    {
        ((uint8_t *)md)[i] = c->st.b[i];
    }

    return 1;
}

/**
 * @brief SHA3 one-time calculation
 *
 * @param in input data
 * @param inlen input data length
 * @param md output data
 * @param mdlen output data length
 */
void sha3(const uint8_t *in, uint32_t inlen, uint8_t *md, uint32_t mdlen)
{
    sha3_ctx_t sha3;

    sha3_init(&sha3, mdlen);
    sha3_update(&sha3, in, inlen);
    sha3_final(md, &sha3);
}

/**
 * @brief RIPEMD compress
 *
 * @param data input data
 */
void ripemd_compress(const uint8_t *data)
{
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_MSG, data, 64);
    SCE_HASH_OPT1_CNT(0);
    SCE_HASH_OPT2_FIRT_BLOCK_CLR();
    SCE_HASH_SET_MSG(0);
    SCE_HASH_OUTPUT(0);
    SCE_HASH_OPT3_MODE(0);

    SCE_HASH_FUNC(HF_RIPMID);

    SCE_HASH_AR();
    SCE_HASH_MFSM_DONE();
}

/**
 * @brief RIPEMD160 one-time calculation
 *
 * @param indata input data
 * @param indataLen input data length
 * @param outData output data
 */
void ripemd160(const uint8_t *indata, uint32_t indataLen, uint8_t *outData)
{
    uint32_t tempBuf[16], i;
    uint32_t digest[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0UL };

    if (outData == NULL)
        return;

    // First packet assignment
    memcpy_u32((uint8_t *)ADDR_HASH_SEG_HOUT, digest, 20);

    // Process packet compression
    for (i = 0; i < (indataLen / 64); i++)
    {
        ripemd_compress(indata);
        indata += 64;
    }
    // Last packet
    memset(tempBuf, 0, sizeof(tempBuf));
    for (i = 0; i < (indataLen & 63); ++i)
    {
        tempBuf[i >> 2] ^= (uint32_t)*indata++ << ((i & 3) << 3);
    }

    tempBuf[(indataLen >> 2) & 15] ^= (uint32_t)1 << (8 * (indataLen & 3) + 7);

    if ((indataLen & 63) > 55)
    {
        ripemd_compress((uint8_t *)tempBuf);
        memset(tempBuf, 0, 64);
    }

    tempBuf[14] = indataLen << 3;
    tempBuf[15] = (indataLen >> 29);

    ripemd_compress((uint8_t *)tempBuf);

    memcpy((uint8_t *)outData, (uint8_t *)ADDR_HASH_SEG_HOUT, 20);
}