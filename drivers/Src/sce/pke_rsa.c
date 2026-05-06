/**
 ******************************************************************************
 * @file    pke_rsa.c
 * @author  SCE Team
 * @brief   PKE (Public Key Engine) RSA driver implementation.
 *          This file provides firmware functions to manage the PKE
 *          peripheral for RSA and big number arithmetic operations.
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
#include <stdint.h>
#include <string.h>
#include "daric_util.h"
#include "daric.h"
#include "auth.h"
#include "pke_rsa.h"
#include "alu.h"
#include "hash.h"
#include "sdma.h"
#include "pke_curve.h"
#include "bn_util.h"

bool AdvanceConfig = false; /* Modular multiplication and modular exponentiation acceleration configuration */
// bool AdvanceClrRam = false;    /* Modular multiplication and modular exponentiation acceleration configuration */
uint32_t        packLen  = 0;
static uint32_t ThisNlen = 0;

/**
 * @brief set endianness
 *
 * @param endian 0 is little endian; 1 is big endian;
 */
void bn_endian_set(uint32_t endian)
{
    if (endian == 0)
    {
        REG_SCE_PKE_OPTLTX = 0;
    }
    else
    {
        REG_SCE_PKE_OPTLTX |= REG_SCE_PKE_OPTLTX_PCON | REG_SCE_PKE_OPTLTX_PIB0_PSIB0 | REG_SCE_PKE_OPTLTX_PIB1_PSIB1 | REG_SCE_PKE_OPTLTX_PKB | REG_SCE_PKE_OPTLTX_POB_PSOB;
    }
}

/* For bn_add_mod and bn_add_sub, must call this API before bn_init(); */
void bn_addsub_initpro(void)
{
    REG_SCE_PKE_OPTMASK = 1;
}

/**
 * @brief initialize rsa context for modular add, sub, mult operations.
 *
 * @param ctx rsa context structure pointer
 * @param n modulus, must be odd number
 * @param nlen words number of n, must be <= 8191bit
 */
void bn_context_init(bn_context_t *ctx, const uint32_t *n, uint32_t n_len)
{
    ctx->n = n;

#if RSA_LITTLE_ENDIAN
    ctx->endian = littleEnd;
    assert((n[0] & 0x1) != 0); /* N must be odd number */
    ctx->n_msb = bnu_get_msb_le(n, n_len);
#else
#fixme
    ctx->endian = littleEnd;
    if (ctx->endian == littleEnd)
    {
        assert((n[0] & 0x1) != 0); /* N must be odd number */
        ctx->n_msb = bnu_get_msb_le(n, n_len);
    }
    else
    {
        assert((n[n_len - 1] & 0x01000000) != 0); /* N must be odd number */
        ctx->n_msb = bnu_get_msb_be(n, n_len);
    }
#endif

    ctx->n_len    = n_len;
    ctx->n_size   = n_len * 4;
    ctx->pack_len = 0;
}

/**
 * @brief initialize rsa context for modular add, sub, mult operations.
 *
 * @param ctx rsa context structure pointer
 * @param n modulus, must be odd number. The msb range is [65, 8191].
 * @param advance_mode modular multiply and modular exponentiation should enable.
 * @param nlen words number of n
 */
void bn_init(bn_context_t *ctx, const uint32_t *n, uint32_t nlen, bool advance_mode)
{
    uint32_t      i;
    bool     sign = false;
    uint32_t modulo_len;

    ASSERT_4BYTE_ALIGNED(n);
    assert(nlen <= 256);

    bn_context_init(ctx, n, nlen);

    modulo_len = ctx->n_msb + 1;

    assert(modulo_len >= 65 && modulo_len <= 8191); /*The msb range is [65, 8191] */

    if (advance_mode)
    {
        if (modulo_len >= 256 && modulo_len <= 6144)
        {
            bn_mult_mod_EX();
            REG_SCE_PKE_MIMMCR = PKE_MIMMCR_CONFIG; /* 4 modular multipliers */
            sign               = true;
        }
        else
        {
            REG_SCE_PKE_MIMMCR = 0;
            sign               = false;
        }
    }
    else
    {
        REG_SCE_PKE_MIMMCR = 0;
        sign               = false;
    }

    SCE_PKE_OPTNW(modulo_len); /* n length */
    REG_SCE_PKE_PCON = 0;      /* Seg PCON Point = 0 */
    /* n to Seg_PCON */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PCON, n, ctx->n_len); /* n to Seg_PCON */
    /* H to Seg_PCON follow n (little endian) */
    volatile uint32_t *AddrH = ADDR_PKE_SEG_PCON + ((bnu_get_nzw_le(n, nlen) + 1) & ~1);

    for (i = 0; i < (modulo_len / 32); i++)
    {
        *(AddrH + i) = 0;
    }
    *(AddrH + i)     = 1 << (modulo_len % (32));
    *(AddrH + i + 1) = 0;

#ifndef RSA_LITTLE_ENDIAN
    /* H of big endian */
    if (ctx->endian != littleEnd)
    {
        uint8_t *bout = (uint8_t *)AddrH;
        int      hlen = (i + 2) / 2 * 2 * 4;
        uint32_t tmpd;
        for (i = 0; i < hlen / 2; i++)
        {
            *((uint8_t *)&tmpd + i % 4) = *(bout + hlen - 1 - i);
            if (i % 4 == 3)
            {
                *(AddrH + i / 4) = tmpd;
            }
        }
        while (i < hlen)
        {
            *(AddrH + i / 4) = 0;
            i += 4;
        }
    }
#endif

    SCE_PKE_FUNC(PIR_RSAINIT); /* IR id = RSA Init */
    SCE_PKE_AR();              /* trig func start */
    SCE_PKE_DONE();            /* waiting for done */
    ThisNlen = nlen;

    if (advance_mode && sign && (modulo_len % 256) != 0)
    {
        modulo_len += (256 - (modulo_len % 256));
        SCE_PKE_OPTNW(modulo_len); /* n length */
        ctx->pack_len = modulo_len / 8 - ctx->n_size;
        packLen       = modulo_len / 8 - ThisNlen * 4; /* fixme */
    }
}

/**
 * @brief result = x + y mod n
 *
 * @param ctx rsa context structure pointer
 * @param x
 * @param y
 * @param xlen word number of x
 * @param ylen word number of y
 * @param[out] result output buffer
 */
void bn_add_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result)
{
    assert(xlen <= 256);
    assert(ylen <= 256);
    ASSERT_4BYTE_ALIGNED(result);
    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(y);
#if RSA_LITTLE_ENDIAN
    assert(bnu_get_msb_le(x, xlen) <= ctx->n_msb);
    assert(bnu_get_msb_le(y, ylen) <= ctx->n_msb);
#endif

    memset((uint8_t *)ADDR_PKE_SEG_POB, 0, 1024);
    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 2048);
    /* X to Seg_PIB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, xlen);
    REG_SCE_PKE_PIB0 = 0;
    /* Y to Seg_PIB follow X */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + (ctx->n_len + 1) / 2 * 2, y, ylen);
    REG_SCE_PKE_PIB1 = (ctx->n_len + 1) / 2 * 2;
    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_RSAMA); /* IR id = RSA Mod Add */
    SCE_PKE_AR();            /* trig func start */
    SCE_PKE_DONE();          /* waiting for done */

    /* Seg_POB to Result */
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB, ctx->n_len);
}

/**
 * @brief result = x - y mod n
 *
 * @param ctx rsa context structure pointer
 * @param x
 * @param y
 * @param xlen word number of x
 * @param ylen word number of y
 * @param[out] result output buffer
 */
void bn_sub_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result)
{
    assert(xlen <= 256);
    assert(ylen <= 256);
    ASSERT_4BYTE_ALIGNED(result);
    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(y);

#if RSA_LITTLE_ENDIAN
    assert(bnu_get_msb_le(x, xlen) <= ctx->n_msb);
    assert(bnu_get_msb_le(y, ylen) <= ctx->n_msb);
#endif

    memset((uint8_t *)ADDR_PKE_SEG_POB, 0, 1024);
    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 2048);
    /* X to Seg_PIB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, xlen);
    REG_SCE_PKE_PIB0 = 0;
    /* Y to Seg_PIB follow X */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + (ctx->n_len + 1) / 2 * 2, y, ylen);
    REG_SCE_PKE_PIB1 = (ctx->n_len + 1) / 2 * 2;
    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_RSAMS); /* IR id = RSA Mod Sub */
    SCE_PKE_AR();            /* trig func start */
    SCE_PKE_DONE();          /* waiting for done */

    /* Seg_POB to Result */
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB, ctx->n_len);
}

/**
 * @brief result = x * y mod n
 *
 * @param ctx rsa context structure pointer
 * @param x
 * @param y
 * @param xlen word number of x
 * @param ylen word number of y
 * @param[out] result output buffer
 */
void bn_mult_mod(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result)
{
    assert(xlen <= 256);
    assert(ylen <= 256);
    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(y);
    ASSERT_4BYTE_ALIGNED(result);

#if RSA_LITTLE_ENDIAN
    assert(bnu_get_msb_le(x, xlen) <= ctx->n_msb);
    assert(bnu_get_msb_le(y, ylen) <= ctx->n_msb);
#endif

    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 2048);
    memset((uint8_t *)ADDR_PKE_SEG_POB, 0, 1024);

#if RSA_LITTLE_ENDIAN
    /* X to Seg_PIB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, xlen);
    /* Y to Seg_PIB follow X */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + ctx->pack_len / 4 + (ctx->n_len + 1) / 2 * 2, y, ylen);
#else
    /* X to Seg_PIB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + ctx->pack_len, x, xlen);
    // Y to Seg_PIB follow X
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + ctx->pack_len * 2 + (ctx->n_len + 1) / 2 * 2, y, ylen);
#endif

    REG_SCE_PKE_PIB0 = 0;
    REG_SCE_PKE_PIB1 = ctx->pack_len / 4 + (ctx->n_len + 1) / 2 * 2;

    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_RSAMM); /* IR id = RSA Mod Multi */
    SCE_PKE_AR();            /* trig func start */
    SCE_PKE_DONE();          /* waiting for done */

    /* Seg_POB to Result */
#if RSA_LITTLE_ENDIAN
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB, ctx->n_len);
#else
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB + ctx->pack_len / 4, ctx->n_len);
#endif

    bn_mult_mod_EX();
}

/* Used for accelerator bugs, clearing cache */
void bn_mult_mod_EX(void)
{
    SCE_PKE_OPTNW(6144); /* N length */
    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 6144 / 8);

    REG_SCE_PKE_PIB0   = 0;
    REG_SCE_PKE_PIB1   = 6144 / 32;
    REG_SCE_PKE_MIMMCR = 0x100; /* 1 modular multiplier */

    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_RSAMM); /* IR id = RSA Mod Multi */
    SCE_PKE_AR();            /* trig func start */
    SCE_PKE_DONE();          /* waiting for done */
}

/**
 * @brief result = x ^ y mod n
 *
 * @param ctx rsa context structure pointer
 * @param x
 * @param y
 * @param xlen word number of x
 * @param ylen word number of y
 * @param result output buffer
 */
void bn_expo_mod_sn(bn_context_t *ctx, const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result)
{
    uint32_t expo_len = bnu_get_msb_le(y, ylen) + 1;

#if RSA_LITTLE_ENDIAN
    assert(bnu_get_msb_le(x, xlen) <= ctx->n_msb);
    assert(xlen <= 256);                     /* xlen <= 1024 bytes */
    assert(ylen <= 256);                     /* ylen <= 1024 bytes */
    assert(bnu_get_msb_le(y, ylen) <= 8191); /* msb of y must <= 8191 */
#endif

    SCE_PKE_OPTEW(expo_len); /* Expo length */

    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 2048);
    memset((uint8_t *)ADDR_PKE_SEG_PKB, 0, 1024);
    memset((uint8_t *)ADDR_PKE_SEG_POB, 0, 1024);
    /* X to Seg_PIB follow X */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, xlen);
    REG_SCE_PKE_PIB0 = 0;
    /* Y to Seg_PKB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PKB, y, ylen);
    REG_SCE_PKE_PKB = 0;

    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_RSAME); /* IR id = RSA Mod Exponential */
    SCE_PKE_AR();            /* trig func start */
    SCE_PKE_DONE();          /* waiting for done */

    /* Seg_POB to Result */
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB, ctx->n_len);
}

/**
 * @brief Initialize modular inverse calculation.
 *
 * @param p input number
 * @param plen word number of p
 */
static void bn_inv_init(const uint32_t *p, uint32_t plen)
{
    uint32_t modulo_len;

    /* p can be odd and even; little/big endian */
#if RSA_LITTLE_ENDIAN
    modulo_len = bnu_get_msb_le(p, plen) + 1;
#else
    modulo_len = bnu_get_msb_be(p, plen) + 1;
    assert((p[plen - 1] & 0x01000000) != 0);
#endif
    assert(modulo_len >= 65 && modulo_len <= 4096); /* The msb range is [65, 4096] */

    SCE_PKE_OPTNW(modulo_len); /* p length */
    REG_SCE_PKE_PCON = 0;      /* Seg PCON Point = 0 */
    /* N to Seg_PCON */
    memset((uint8_t *)ADDR_PKE_SEG_PCON, 0, plen * 4 + 32); /* clean PIB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PCON, p, plen); /* p to Seg_PCON */

    SCE_PKE_FUNC(PIR_INVINIT); /* IR id = RSA Inv Init */
    SCE_PKE_AR();              /* trig func start */
    SCE_PKE_DONE();            /* waiting for done */
    return;
}

/**
 * @brief result = 1 / x mod p
 *
 * @param x Big number to be inverted.
 * @param p Modulus
 * @param xlen Word number of x, must be <= 128.
 * @param plen Word number of p, must be <= 128.
 * @param[out] result Output buffer.
 * @return int32_t If x and p are not relatively prime, return -1;
 */
int32_t bn_inv_mod(const uint32_t *x, const uint32_t *p, uint32_t xlen, uint32_t plen, uint32_t *result)
{
    int32_t timeout = 200;

    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(p);
    ASSERT_4BYTE_ALIGNED(result);
    assert(xlen <= 128);

#if RSA_LITTLE_ENDIAN
    assert(bnu_get_msb_le(x, xlen) <= bnu_get_msb_le(p, plen));
#endif

    bn_inv_init(p, plen);

    /* X to Seg_PIB */
    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 2048); /* clean PIB */
    memset((uint8_t *)ADDR_PKE_SEG_POB, 0, 1024);
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, xlen);
    REG_SCE_PKE_PIB0 = 0;

    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_MODINV); /* IR id = RSA Mod Inv */
    SCE_PKE_AR();             /* trig func start */
    SCE_PKE_DONE();           /* waiting for done */
    /* Check bit8 of sfr_sr, different with other cmds!! */
    /* If X and P are not relatively prime, bit8 of sfr_sr never be set, it will be Timeout! */
    while (!(REG_SCE_PKE_SRMFSM & 0x100))
    {
        if (timeout-- < 0)
        {
            return -1;
        }
        else
        {
            HAL_Delay(1);
        }
    }
    REG_SCE_PKE_SRMFSM |= 0x100;

    /* Seg_POB to Result */
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB, plen);
    return 0;
}

/**
 * @brief result = gcd(x, y)
 *
 * @param x Big number.
 * @param y Big number.
 * @param xlen Word number of x. Must be <= 128.
 * @param ylen Word number of y. Must be <= 128.
 * @param[out] result Output buffer.
 */
void bn_gcd(const uint32_t *x, const uint32_t *y, uint32_t xlen, uint32_t ylen, uint32_t *result)
{
    uint32_t dlen;

    ASSERT_4BYTE_ALIGNED(x);
    ASSERT_4BYTE_ALIGNED(y);
    ASSERT_4BYTE_ALIGNED(result);
#if RSA_LITTLE_ENDIAN
    assert(bnu_get_msb_le(x, xlen) <= 4096);
    assert(bnu_get_msb_le(y, ylen) <= 4096);
#endif

    dlen = (xlen > ylen) ? xlen : ylen;
    dlen = (dlen + 1) / 2 * 2; /* 64bit align */
    SCE_PKE_OPTNW(dlen * 32);  /* bit length */

    memset((uint8_t *)ADDR_PKE_SEG_PIB, 0, 2048);
    memset((uint8_t *)ADDR_PKE_SEG_POB, 0, 1024);
    /* X to Seg_PIB */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB, x, xlen);
    REG_SCE_PKE_PIB0 = 0;
    /* Y to Seg_PIB follow X */
    bnu_memcpy_u32((uint32_t *)ADDR_PKE_SEG_PIB + dlen, y, ylen);
    REG_SCE_PKE_PIB1 = dlen;
    /* Result in POB */
    REG_SCE_PKE_POB = 0;
    SCE_PKE_FUNC(PIR_GCD); /* IR id = GCD */
    SCE_PKE_AR();          /* trig func start */
    SCE_PKE_DONE();        /* waiting for done */

    /* Seg_POB to Result */
    bnu_memcpy_u32(result, (uint32_t *)ADDR_PKE_SEG_POB, dlen);
}

void sce_reset(void)
{
    // SCE_CLR_RESET_RAM();    /* Reset&Clear RAM */
}

void sce_ram_clean(void)
{
    SCE_CLR_RAM();
    SCE_RAM_CLEAR_DONE();
}