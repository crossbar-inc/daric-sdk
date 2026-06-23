/**
 ******************************************************************************
 * @file    rfc6979.c
 * @author  SCE Team
 * @brief   RFC6979 deterministic nonce generation implementation.
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

#include "rfc6979.h"
// #ifdef DRV_SCE_ENABLED
#include <string.h>
#include <stdio.h>
#include "hash.h"

#define RFC6979_DEBUG 0

#if RFC6979_DEBUG
#define RFC6979_DUMP_BUFFER(name, ptr, len) dump_buffer(name, ptr, len)

static void dump_buffer(char *name, void *buf, uint32_t len)
{
    uint32_t i;
    uint8_t *ptr = (uint8_t *)buf;
    printf("%s = ", name);
    for (i = 0; i < len; i++)
    {
        printf("%02X", ptr[i]);
    }
    printf("\r\n");
}

#else
#define RFC6979_DUMP_BUFFER(name, ptr, len)
#endif

void rfc6979_hmac_init(rfc6979_context *ctx, const uint32_t *msg, const uint32_t *key)
{
    static const uint8_t zero[1] = { 0 };
    static const uint8_t one[1]  = { 1 };

    sce_Sysinit_Hash();
    hash_init();

    /* rfc6979 3.2.b */
    memset(ctx->v, 0x01, 32);
    /* rfc6979 3.2.c */
    memset(ctx->k, 0x00, 32);

    /* rfc6979 3.2.d */
    hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
    hmac_sha256_512_MsgInput_Step(ctx->v, 32, SHA256_MODE);
    hmac_sha256_512_MsgInput_Step((uint8_t *)zero, 1, SHA256_MODE);
    hmac_sha256_512_MsgInput_Step((uint8_t *)key, 32, SHA256_MODE);
    hmac_sha256_512_Result_Step((uint8_t *)msg, 32, ctx->k, SHA256_MODE);
    RFC6979_DUMP_BUFFER("ctx->k:", ctx->k, 32);

    /* rfc6979 3.2.e */
    hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
    hmac_sha256_512_Result_Step(ctx->v, 32, ctx->v, SHA256_MODE);
    RFC6979_DUMP_BUFFER("ctx->v:", ctx->v, 32);

    /* rfc6979 3.2.f */
    hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
    hmac_sha256_512_MsgInput_Step(ctx->v, 32, SHA256_MODE);
    hmac_sha256_512_MsgInput_Step((uint8_t *)one, 1, SHA256_MODE);
    hmac_sha256_512_MsgInput_Step((uint8_t *)key, 32, SHA256_MODE);
    hmac_sha256_512_Result_Step((uint8_t *)msg, 32, ctx->k, SHA256_MODE);
    RFC6979_DUMP_BUFFER("ctx->k2:", ctx->k, 32);

    /* rfc6979 3.2.g */
    hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
    hmac_sha256_512_Result_Step(ctx->v, 32, ctx->v, SHA256_MODE);
    RFC6979_DUMP_BUFFER("ctx->v2:", ctx->v, 32);
    ctx->retry = 0;

    return;
}

void rfc6979_hmac_generate(rfc6979_context *ctx, uint32_t *nonce_out)
{
    static const uint8_t zero[1] = { 0x00 };

    /* rfc6979 3.2.h */
    if (ctx->retry)
    {
        hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
        hmac_sha256_512_MsgInput_Step((uint8_t *)ctx->v, 32, SHA256_MODE);
        hmac_sha256_512_Result_Step((uint8_t *)zero, 1, ctx->k, SHA256_MODE);
        RFC6979_DUMP_BUFFER("ctx->k3-2:", ctx->k, 32);

        hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
        hmac_sha256_512_Result_Step(ctx->v, 32, ctx->v, SHA256_MODE);
        RFC6979_DUMP_BUFFER("ctx->v3-2:", ctx->v, 32);
        memcpy(nonce_out, ctx->v, 32);
    }

    hmac_sha256_512_KeyInit_Step(ctx->k, 32, SHA256_MODE);
    hmac_sha256_512_Result_Step(ctx->v, 32, ctx->v, SHA256_MODE);
    RFC6979_DUMP_BUFFER("ctx->v3-2:", ctx->v, 32);
    memcpy(nonce_out, ctx->v, 32);
    ctx->retry = 1;
}

uint32_t rfc6979_nonce_fn(const uint32_t *msg, const uint32_t *key, uint32_t *nonce)
{
    rfc6979_context rfc6979_ctx;

    rfc6979_hmac_init(&rfc6979_ctx, msg, key);
    rfc6979_hmac_generate(&rfc6979_ctx, nonce);

    return 0;
}

// #endif