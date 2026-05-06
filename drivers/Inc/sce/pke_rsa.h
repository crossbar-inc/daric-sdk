/**
 ******************************************************************************
 * @file    pke_rsa.h
 * @author  SCE Team
 * @brief   Header file for PKE RSA driver.
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

#ifndef __PKE_RSA_H__
#define __PKE_RSA_H__

#include <stdbool.h>
#include <stdint.h>

#define RSA_LITTLE_ENDIAN 1

// 7936 ok
#define BN_BIT_LEN  4096
#define BN_BYTE_LEN (BN_BIT_LEN / 8)
#define BN_WORD_LEN (BN_BYTE_LEN / 4)

typedef struct
{
    uint32_t val[BN_WORD_LEN];
} tBigNum;

typedef struct
{
    const uint32_t *n;
    uint32_t        n_size;
    uint32_t        n_len; // N length in word
    uint32_t        endian;
    uint32_t        n_msb;
    uint32_t        pack_len;
} bn_context_t;

uint32_t bn_endian_swap(const uint32_t *in, uint32_t *out, uint32_t wordlen);
void     bn_fill_be(const uint32_t *in, uint32_t inlen, uint32_t *out, uint32_t outlen);
void     bn_addsub_initpro(void);

void bn_context_init(bn_context_t *ctx, const uint32_t *n, uint32_t n_len);
void bn_init(bn_context_t *ctx, const uint32_t *n, uint32_t nlen, bool boost);
void bn_add_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result);
void bn_sub_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result);
void bn_mult_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result);
// void bn_expo_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result);
void    bn_expo_mod_sn(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result);
int32_t bn_inv_mod(const uint32_t *X, const uint32_t *P, uint32_t xlen, uint32_t plen, uint32_t *Result);
void    bn_gcd(const uint32_t *X, const uint32_t *Y, uint32_t xlen, uint32_t ylen, uint32_t *Result);

void bn_endian_set(uint32_t endian);

void sce_reset(void);
void sce_ram_clean(void);

extern bool AdvanceConfig;

#define ENABLE_ADVANCE() (AdvanceConfig = true)
#define DISABLE_ADVANCE()           \
    {                               \
        AdvanceConfig      = false; \
        REG_SCE_PKE_MIMMCR = 0;     \
    }

#if 0
typedef struct menuStruct
{
    char *           inputChar;
    char *           jiraLabel;
    char *           cmdDescription;
    char *           paraDescription;
    const struct menuStruct *menuItem;
    void (*func)(uint16_t argc, const uint32_t *argv, const char* label);

} Menu_Type;
extern const Menu_Type g_rsaMenu[];
#endif

uint32_t bn_expo_mod_crt(const uint32_t *X,
                         const uint32_t *Y,
                         const uint32_t *P,
                         const uint32_t *Q,
                         const uint32_t *N,
                         uint32_t        xlen,
                         uint32_t        ylen,
                         uint32_t        plen,
                         uint32_t        qlen,
                         uint32_t        nlen,
                         uint8_t         dataEndian,
                         uint32_t       *Result);
void     bn_mult_mod_EX(void);
#endif