/**
 ******************************************************************************
 * @file    ll_api.h
 * @author  SCE Team
 * @brief   Header file for Low Level API operations.
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

#ifndef __LL_API_H__
#define __LL_API_H__

#include "daric_util.h"

typedef enum
{
    ERR_NONE = 0,
    ERR_NO_PERMISSION,
    ERR_DATA_LENGTH,
    ERR_DATA_FORMAT,
    ERR_PARAMETERS,
    ERR_STEP,
    ERR_VERIFY_FAIL,
    ERR_TIMEOUT,
    ERR_NORESULT,
} cr_status;

typedef enum
{
    HT_RIPEMD = 0,
    HT_SHA256,
    HT_SHA512,
    HT_SHA3_256,
    HT_SHA3_512,
    HT_BLAKE2B,
    HT_BLAKE2S,
    HT_BLAKE3,
} hash_t;

typedef enum
{
    AES_KEY_128 = 16,
    AES_KEY_192 = 24,
    AES_KEY_256 = 32,
} AES_KEY_LEN;

typedef enum
{
    AES_ECB = 0,
    AES_CBC = 0x10,
    AES_CTR = 0x20,
    AES_CFB = 0x40,
    AES_OFB = 0x30,
} AES_MODE_TYPE;

typedef enum
{
    CT_SECP256K1 = 0,
    CT_SECP256R1,
    CT_SECP384R1,
    CT_ED25519,
} curve_type;

typedef enum
{
    MT_N = 0,
    MT_P,
} modulo_t;

typedef enum
{
    SEG_LKEY = 0,
    SEG_KEY,
    SEG_SKEY,
    SEG_SCRT,
    SEG_MSG,
    SEG_HOUT,
    SEG_SOB,
    SEG_PCON,
    SEG_PKB,
    SEG_PIB,
    SEG_PSIB,
    SEG_POB,
    SEG_PSOB,
    SEG_AKEY,
    SEG_AIB,
    SEG_AOB,
    SEG_RNGA,
    SEG_RNGB,
    SEG_NUM
} segment_id;

/**
 * ignore duplicate function/variable definition messages in doxygen
 */

extern uint32_t ALLF_DATA[4096 / 32];

cr_status hash(const uint8_t *msg, uint32_t msg_len, hash_t hash_type, uint8_t *digest);
cr_status hash_Init(hash_t hash_type);
cr_status hash_Update(const uint8_t *msg, uint32_t msg_len, hash_t hash_type);
cr_status hash_Final(const uint8_t *msg, uint32_t msg_len, hash_t hash_type, uint8_t *digest);

cr_status hmac(const uint8_t *key, uint32_t key_len, const uint8_t *msg, uint32_t msg_len, hash_t hash_type, uint8_t *digest);
cr_status hmac_KeyInit(const uint8_t *key, uint32_t key_len, hash_t hash_type);
cr_status hmac_Msg_Update(const uint8_t *msg, uint32_t msg_len, hash_t hash_type);
cr_status hmac_Final(hash_t hash_type, uint8_t *digest);

cr_status aes_encrypt_le(AES_MODE_TYPE mode, const uint8_t *key, AES_KEY_LEN key_len, const uint8_t *iv, const uint8_t *input, uint32_t input_len, uint8_t *output);
cr_status aes_decrypt_le(AES_MODE_TYPE mode, const uint8_t *key, AES_KEY_LEN key_len, const uint8_t *iv, const uint8_t *input, uint32_t input_len, uint8_t *output);

cr_status modulo_add_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result);
cr_status modulo_sub_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result);
cr_status modulo_multiply_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result);
cr_status modulo_expo_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result);
cr_status modulo_inverse_le(const uint32_t *x, const uint32_t *n, uint32_t x_len, uint32_t n_len, uint32_t *result);
cr_status gcd_le(const uint32_t *x, const uint32_t *y, uint32_t x_len, uint32_t y_len, uint32_t *result);
cr_status bndivision_le(const uint32_t *x, uint32_t xlen, const uint32_t *y, uint32_t ylen, uint32_t *q, uint32_t *qlen, uint32_t *r, uint32_t *rlen);

cr_status cv_get_prime_be(curve_type curve, uint32_t *prime);
cr_status cv_get_order_be(curve_type curve, uint32_t *order);
cr_status cv_get_order_half_be(curve_type curve, uint32_t *order_half);
cr_status cv_get_a_be(curve_type curve, uint32_t *a);
cr_status cv_get_b_be(curve_type curve, uint32_t *b);
cr_status cv_get_g_x_be(curve_type curve, uint32_t *x);
cr_status cv_get_g_y_be(curve_type curve, uint32_t *y);

cr_status point_add_le(curve_type curve, const uint32_t *px, const uint32_t *py, uint32_t *result);
cr_status point_multiply_le(curve_type curve, const uint32_t *x, const uint32_t *py, uint32_t *result);
cr_status scalar_multiply_le(curve_type curve, const uint32_t *x, uint32_t *result);
cr_status cv_modulo_add_le(curve_type curve, modulo_t modulo_type, const uint32_t *x, const uint32_t *y, uint32_t *result);
cr_status cv_modulo_sub_le(curve_type curve, modulo_t modulo_type, const uint32_t *x, const uint32_t *y, uint32_t *result);
cr_status cv_modulo_mul_le(curve_type curve, modulo_t modulo_type, const uint32_t *x, const uint32_t *y, uint32_t *result);
cr_status cv_get_prime_le(curve_type curve, uint32_t *prime);
cr_status cv_get_order_le(curve_type curve, uint32_t *order);
cr_status cv_get_order_half_le(curve_type curve, uint32_t *order_half);
cr_status cv_get_a_le(curve_type curve, uint32_t *a);
cr_status cv_get_b_le(curve_type curve, uint32_t *b);
cr_status cv_get_g_x_le(curve_type curve, uint32_t *x);
cr_status cv_get_g_y_le(curve_type curve, uint32_t *y);
cr_status cv_get_d_le(curve_type curve, uint32_t *d);

cr_status rng_init(void);
cr_status rng_buffer(uint32_t *buffer, uint32_t length);

/**
 * @brief Portable mutex hooks for RTOS environments.
 *
 * Default implementations (bare-metal no-ops) are provided as weak symbols
 * in ll_api.c.  To enable mutual exclusion under an RTOS, define these two
 * functions in your application or BSP layer:
 *
 *   void ll_sce_lock(void)   { <acquire your RTOS mutex> }
 *   void ll_sce_unlock(void) { <release your RTOS mutex> }
 *
 * The lock must be held for the entire Init → [Update]* → Final sequence.
 * Use a non-recursive mutex; do not call hash/hmac stepped APIs re-entrantly.
 */
void ll_sce_lock(void);
void ll_sce_unlock(void);

cr_status data_move_i(uint32_t segid_src, uint32_t src_woff, uint32_t segid_des, uint32_t des_woff, uint32_t wsize, uint32_t opt);
cr_status data_move_e(uint32_t rw, uint32_t segid, uint32_t seg_woff, uint32_t *exaddr, uint32_t wsize, uint32_t opt);
cr_status data_move_s(uint32_t rw, uint32_t segid, uint32_t seg_woff, uint32_t *exaddr, uint32_t wsize, uint32_t opt);
void      bn_random_limit_le(uint32_t *rand, uint32_t len, const uint32_t *lmt, uint32_t lmtlen);
#endif
