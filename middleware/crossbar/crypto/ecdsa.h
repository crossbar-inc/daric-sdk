/**
 ******************************************************************************
 * @file    ecdsa.h
 * @author  SCE Team
 * @brief   Header file for ECDSA operations.
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

#ifndef __DARIC_ECDSA_H__
#define __DARIC_ECDSA_H__

// #include "drv_conf.h"
// #ifdef DRV_SCE_ENABLED
#include "pke_curve.h"
#include "ll_api.h"
#include "hal_api.h"

/**
 * @brief A pointer to a function to generate a nonce for ECDSA signing.
 * @param msg:   32-byte message hash (big-endian)
 * @param key:   32-byte private key (big-endian)
 * @param nonce: [out] 32-byte nonce filled by the function
 * @return 0 on success, other on failure
 */
typedef uint32_t (*nonce_function)(uint32_t *nonce);
void    ecdsa_get_pubkey(curve_type curve, uint32_t *seckey, uint32_t *pubkey);
void    ecdsa_get_bip340_pubkey(curve_type curve, uint32_t *seckey, uint32_t *pubkey);
int32_t ecdsa_sign_digest(curve_type curve, const uint32_t *priv_key, const uint32_t *digest, nonce_function nonce_fn, uint32_t *sig);
int32_t ecdsa_verify(curve_type curve, const uint32_t *pub_key, const uint32_t *sig, const uint32_t *digest);
bool    ecdsa_validate_pubkey(curve_type curve, pointDef *pub);
bool    ecdsa_read_pubkey(curve_type curve, const uint32_t *pub_key, uint32_t length, pointDef *pub);
bool    num_256_is_less(const uint8_t *x, const uint8_t *y);
bool    num_256_is_equal(const uint32_t *x, const uint32_t *y);
bool    num_256_is_zero(const uint8_t *x);
bool    num_256_is_odd(const uint32_t *x);
void    num_256_rshift(uint8_t *num);
bool    point_is_infinity(const pointDef *p);
int32_t ecdh_multiply(curve_type curve, const uint32_t *priv_key, const uint32_t *pub_key, uint32_t pub_key_length, uint32_t *shared_secret);
int32_t ecdsa_verify_ext(curve_type curve, const uint32_t *pub_key, uint32_t pub_key_length, const uint32_t *sig, const uint32_t *digest);
#endif
// #endif