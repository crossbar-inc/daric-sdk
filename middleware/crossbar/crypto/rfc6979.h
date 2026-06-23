/**
 ******************************************************************************
 * @file    rfc6979.h
 * @author  SCE Team
 * @brief   Header file for RFC6979 operations.
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

#ifndef __DARIC_RFC6979_H__
#define __DARIC_RFC6979_H__

// #include "drv_conf.h"
// #ifdef DRV_SCE_ENABLED
#include <stdint.h>

typedef struct
{
    uint8_t v[32];
    uint8_t k[32];
    int     retry;
} rfc6979_context;

/**
 * @brief Initialise an RFC 6979 HMAC-DRBG context.
 * @param ctx: context to initialise
 * @param msg: 32-byte message hash (big-endian)
 * @param key: 32-byte private key (big-endian)
 */
void rfc6979_hmac_init(rfc6979_context *ctx, const uint32_t *msg, const uint32_t *key);

/**
 * @brief Generate the next deterministic nonce from an RFC 6979 context.
 *        Each successive call produces a different nonce for the same (msg, key).
 * @param ctx:       context previously initialised with rfc6979_hmac_init()
 * @param nonce_out: [out] 32-byte nonce
 */
void rfc6979_hmac_generate(rfc6979_context *ctx, uint32_t *nonce_out);

/**
 * @brief One-shot RFC 6979 nonce generation.
 * @param msg:   32-byte message hash (big-endian)
 * @param key:   32-byte private key (big-endian)
 * @param nonce: [out] 32-byte nonce
 * @return 0 on success
 */
uint32_t rfc6979_nonce_fn(const uint32_t *msg, const uint32_t *key, uint32_t *nonce);
#endif
// #endif