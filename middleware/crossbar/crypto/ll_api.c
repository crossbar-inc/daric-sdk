/**
 ******************************************************************************
 * @file    ll_api.c
 * @author  SCE Team
 * @brief   Low Level API implementation file.
 *          This file provides firmware functions for cryptographic
 *          operations including hash, hmac, aes, and modular arithmetic.
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
#include "ll_api.h"
#include "bn_util.h"
#include "hash.h"
#include "trng.h"
#include "pke_curve.h"
#include "pke_rsa.h"
#include "sdma.h"
#include "auth.h"
#include "aes.h"
#include "alu.h"
#include "hal_api.h"

#ifdef TEST_SCE_ENABLED
/**
 * @private debugging function
 */
extern void bn_dump(char *tit, uint32_t *data, uint32_t len);
#endif

/**
 * @private debugging function
 */

#if !defined Debug_Print
#define Debug_Print(...)
#endif

// start preserve data layout
// clang-format off

static const uint32_t secp256k1_order[8] = {
    0xffffffff, 0xffffffff, 0xffffffff, 0xfeffffff, 0xe6dcaeba, 0x3ba048af, 0x8c5ed2bf, 0x414136d0};
static const uint32_t secp256k1_order_half[8] = {
    0xffffff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0x736e575d, 0x1d50a457, 0x462fe9df, 0xa0201b68};
static const uint32_t secp256k1_prime[8] = {
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfeffffff, 0x2ffcffff};
static const uint32_t secp256k1_g_x[8] = {
    0x7e66be79, 0xacbbdcf9, 0x9562a055, 0x070b87ce, 0xdbfc9b02, 0xd928ce2d, 0x5b81f259, 0x9817f816};
static const uint32_t secp256k1_g_y[8] = {
    0x77da3a48, 0x65c4a326, 0xfcfba45d, 0xa808110e, 0x48b417fd, 0x195485a6, 0x8fd0479c, 0xb8d410fb};

static const uint32_t secp256r1_order[8] = {
    0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0xadfae6bc, 0x849e17a7, 0xc2cab9f3, 0x512563fc};
static const uint32_t secp256r1_order_half[8] = {
    0xffffff7f, 0x00000080, 0xffffff7f, 0xffffffff, 0x567d73de, 0x42cf8bd3, 0x61e5dc79, 0xa892317e};
static const uint32_t secp256r1_prime[8] = {
    0xffffffff, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff};
static const uint32_t secp256r1_g_x[8] = {
    0xf2d1176b, 0x47422ce1, 0xe5e6bcf8, 0xf240a463, 0x817d0377, 0xa033eb2d, 0x4539a1f4, 0x96c298d8};
static const uint32_t secp256r1_g_y[8] = {
    0xe242e34f, 0x9b7f1afe, 0x4aebe78e, 0x169e0f7c, 0x5733ce2b, 0xce5e316b, 0x6840b6cb, 0xf551bf37};
static const uint32_t secp256r1_b[8] = {
    0xd835c65a, 0xe7933aaa, 0x55bdebb3, 0xbc869876, 0xb0061d65, 0xf6b053cc, 0x3e3cce3b, 0x4b60d227};
static const uint32_t secp256r1_a[8] = {       // -3
    0xffffffff, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xfcffffff};


/**
 * @brief A block of data with all bits set
 */
extern uint32_t ALLF_DATA[4096/32];

// finish preserve data layout
// clang-format on

/**
 * @brief digest = hash(msg)
 *
 * @param msg input message
 * @param msg_len message length
 * @param hash_type supported hash type: HT_SHA256, HT_SHA512, HT_RIPEMD, HT_SHA3_256, HT_SHA3_512, HT_BLAKE2B, HT_BLAKE2S
 * @param[out] digest digest length of black2b is 64bytes; digest length of black2s is 32bytes;
 * @return cr_status
 */
cr_status hash(const uint8_t *msg, uint32_t msg_len, hash_t hash_type, uint8_t *digest)
{
    cr_status ret = ERR_NONE;
    ll_sce_lock();
    sce_Sysinit_Hash();
    hash_init();
    switch (hash_type)
    {
        case HT_SHA256:
            sha256_512_calc(msg, msg_len, SHA256_MODE, digest);
            break;
        case HT_SHA512:
            sha256_512_calc(msg, msg_len, SHA512_MODE, digest);
            break;
        case HT_RIPEMD:
            ripemd160(msg, msg_len, digest);
            break;
        case HT_SHA3_256:
            sha3(msg, msg_len, digest, 32);
            break;
        case HT_SHA3_512:
            sha3(msg, msg_len, digest, 64);
            break;
        case HT_BLAKE2B:
            if (blake2b(digest, BLAKE2B_OUTBYTES, msg, msg_len) != 0)
                ret = ERR_PARAMETERS;
            break;
        case HT_BLAKE2S:
            if (blake2s(digest, BLAKE2S_OUTBYTES, msg, msg_len) != 0)
                ret = ERR_PARAMETERS;
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    ll_sce_unlock();
    return ret;
}

/**
 * @brief Bare-metal no-op implementations of the RTOS mutex hooks.
 * Override by defining ll_sce_lock / ll_sce_unlock in your RTOS port.
 */
__attribute__((weak)) void ll_sce_lock(void)   { /* bare-metal: no-op */ }
__attribute__((weak)) void ll_sce_unlock(void) { /* bare-metal: no-op */ }

/**
 * @brief this variable used to record the hash type of step calculation
 * value = type + 1 in hash_Init(), so 0 means hash_Init() is not called;
 *
 */
static int32_t hash_step_type = 0;

/**
 * @brief calculate hash step by step: hash_Init() - hash_Update()(optional) - hash_Final()
 * msg can be supplied partly, applies to the msg from non-consecutive address
 * support SHA256 and SHA512 now, will support more hash type after verification
 *
 * @param hash_type supported hash type: HT_SHA256, HT_SHA512, HT_BLAKE2B, HT_BLAKE2S
 *                  (HT_RIPEMD and HT_SHA3_* return ERR_NORESULT — use hash() for one-shot SHA3)
 * @return cr_status
 */
cr_status hash_Init(hash_t hash_type)
{
    cr_status ret = ERR_NONE;
    ll_sce_lock();
    sce_Sysinit_Hash();
    hash_init();
    switch (hash_type)
    {
        case HT_SHA256:
            sha256_512_Init();
            break;
        case HT_SHA512:
            sha256_512_Init();
            break;
        case HT_RIPEMD:
            ret = ERR_NORESULT;
            break;
        case HT_SHA3_256:
        case HT_SHA3_512:
            ret = ERR_NORESULT;
            break;
        case HT_BLAKE2B:
            blake2b_init();
            break;
        case HT_BLAKE2S:
            blake2s_init();
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    if (ret == ERR_NONE)
    {
        hash_step_type = hash_type + 1;
    }
    else
    {
        hash_step_type = 0;
        ll_sce_unlock();
    }
    return ret;
}

/**
 * @brief step 2 of hash calculate step by step, optional, hash_Init() - hash_Final() is OK
 *
 * @param msg input message
 * @param msg_len message length
 * @param hash_type supported hash type: HT_SHA256, HT_SHA512, HT_RIPEMD, HT_SHA3_256, HT_SHA3_512, HT_BLAKE2B, HT_BLAKE2S
 * @return cr_status
 */
cr_status hash_Update(const uint8_t *msg, uint32_t msg_len, hash_t hash_type)
{
    cr_status ret = ERR_NONE;
    if (hash_step_type != hash_type + 1)
    {
        if (hash_step_type != 0)
        {
            hash_step_type = 0;
            ll_sce_unlock();
        }
        return ERR_STEP;
    }
    switch (hash_type)
    {
        case HT_SHA256:
            sha256_512_Input(msg, msg_len, SHA256_MODE);
            break;
        case HT_SHA512:
            sha256_512_Input(msg, msg_len, SHA512_MODE);
            break;
        case HT_RIPEMD:
            ret = ERR_NORESULT;
            hash_step_type = 0;
            ll_sce_unlock();
            break;
        case HT_BLAKE2B:
            if (blake2b_update(msg, msg_len) != 0)
            {
                ret = ERR_PARAMETERS;
                hash_step_type = 0;
                ll_sce_unlock();
            }
            break;
        case HT_BLAKE2S:
            if (blake2s_update(msg, msg_len) != 0)
            {
                ret = ERR_PARAMETERS;
                hash_step_type = 0;
                ll_sce_unlock();
            }
            break;
        default:
            ret = ERR_PARAMETERS;
            hash_step_type = 0;
            ll_sce_unlock();
            break;
    }
    return ret;
}

/**
 * @brief last step of the hash calculate step by step
 *
 * @param msg input message
 * @param msg_len message length
 * @param hash_type supported hash type: HT_SHA256, HT_SHA512, HT_RIPEMD, HT_SHA3_256, HT_SHA3_512, HT_BLAKE2B, HT_BLAKE2S
 * @param[out] digest digest length of black2b is 64bytes; digest length of black2s is 32bytes;
 * @return cr_status
 */
cr_status hash_Final(const uint8_t *msg, uint32_t msg_len, hash_t hash_type, uint8_t *digest)
{
    cr_status ret = ERR_NONE;
    if (hash_step_type != hash_type + 1)
    {
        if (hash_step_type != 0)
        {
            hash_step_type = 0;
            ll_sce_unlock();
        }
        return ERR_STEP;
    }
    switch (hash_type)
    {
        case HT_SHA256:
            sha256_512_Result(msg, msg_len, SHA256_MODE, digest);
            break;
        case HT_SHA512:
            sha256_512_Result(msg, msg_len, SHA512_MODE, digest);
            break;
        case HT_RIPEMD:
            ret = ERR_NORESULT;
            break;
        case HT_BLAKE2B:
            if (blake2b_update(msg, msg_len) != 0 ||
                blake2b_final(digest, BLAKE2B_OUTBYTES) != 0)
            {
                ret = ERR_PARAMETERS;
            }
            break;
        case HT_BLAKE2S:
            if (blake2s_update(msg, msg_len) != 0 ||
                blake2s_final(digest, BLAKE2S_OUTBYTES) != 0)
            {
                ret = ERR_PARAMETERS;
            }
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    hash_step_type = 0;
    ll_sce_unlock();
    return ret;
}

/**
 * @brief calculate hmac
 *
 * @param key
 * @param key_len
 * @param msg
 * @param msg_len
 * @param hash_type supported type: HT_SHA256, HT_SHA512
 * @param[out] digest
 * @return cr_status
 */
cr_status hmac(const uint8_t *key, uint32_t key_len, const uint8_t *msg, uint32_t msg_len, hash_t hash_type, uint8_t *digest)
{
    uint32_t  msgCount;
    cr_status ret = ERR_NONE;

    if ((key == NULL) || (msg == NULL) || (digest == NULL))
        return ERR_PARAMETERS;

    ll_sce_lock();
    sce_Sysinit_Hash();
    hash_init();
    switch (hash_type)
    {
        case HT_SHA256:
            hmac_sha256_512_KeyInit(key, key_len, SHA256_MODE);
            msgCount = hmac_sha256_512_MsgInput(msg, msg_len, SHA256_MODE);
            hmac_sha256_512_Result(digest, msgCount, SHA256_MODE);
            break;
        case HT_SHA512:
            hmac_sha256_512_KeyInit(key, key_len, SHA512_MODE);
            msgCount = hmac_sha256_512_MsgInput(msg, msg_len, SHA512_MODE);
            hmac_sha256_512_Result(digest, msgCount, SHA512_MODE);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    ll_sce_unlock();
    return ret;
}

static int32_t hmac_step_type = 0;

/**
 * @brief calculate hmac step by step: hmac_KeyInit() - hmac_Msg_Update() - hmac_Final()
 *
 * @param key
 * @param key_len
 * @param hash_type supported type: HT_SHA256, HT_SHA512
 * @return cr_status
 */
cr_status hmac_KeyInit(const uint8_t *key, uint32_t key_len, hash_t hash_type)
{
    cr_status ret = ERR_NONE;
    ll_sce_lock();
    sce_Sysinit_Hash();
    hash_init();
    switch (hash_type)
    {
        case HT_SHA256:
            hmac_sha256_512_KeyInit_Step(key, key_len, SHA256_MODE);
            break;
        case HT_SHA512:
            hmac_sha256_512_KeyInit_Step(key, key_len, SHA512_MODE);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    if (ret == ERR_NONE)
    {
        hmac_step_type = (hash_type + 1) | 0x40000;
    }
    else
    {
        hmac_step_type = 0;
        ll_sce_unlock();
    }
    return ret;
}

/**
 * @brief hmac update update
 *
 * @param msg
 * @param msg_len
 * @param hash_type supported type: HT_SHA256, HT_SHA512
 * @return cr_status
 */
cr_status hmac_Msg_Update(const uint8_t *msg, uint32_t msg_len, hash_t hash_type)
{
    cr_status ret = ERR_NONE;

    if (hmac_step_type != ((hash_type + 1) | 0x40000))
    {
        if (hmac_step_type != 0)
        {
            hmac_step_type = 0;
            ll_sce_unlock();
        }
        return ERR_STEP;
    }
    switch (hash_type)
    {
        case HT_SHA256:
            hmac_sha256_512_MsgInput_Step(msg, msg_len, SHA256_MODE);
            break;
        case HT_SHA512:
            hmac_sha256_512_MsgInput_Step(msg, msg_len, SHA512_MODE);
            break;
        default:
            ret            = ERR_PARAMETERS;
            hmac_step_type = 0;
            ll_sce_unlock();
            break;
    }
    return ret;
}

/**
 * @brief hmac get digest
 *
 * @param hash_type supported type: HT_SHA256, HT_SHA512
 * @param[out] digest
 * @return cr_status
 */
cr_status hmac_Final(hash_t hash_type, uint8_t *digest)
{
    cr_status ret = ERR_NONE;
    if (hmac_step_type != ((hash_type + 1) | 0x40000))
    {
        if (hmac_step_type != 0)
        {
            hmac_step_type = 0;
            ll_sce_unlock();
        }
        return ERR_STEP;
    }
    switch (hash_type)
    {
        case HT_SHA256:
            hmac_sha256_512_Result_Step(NULL, 0, digest, SHA256_MODE);
            break;
        case HT_SHA512:
            hmac_sha256_512_Result_Step(NULL, 0, digest, SHA512_MODE);
            break;
        default:
            ret            = ERR_PARAMETERS;
            break;
    }
    hmac_step_type = 0;
    ll_sce_unlock();
    return ret;
}

/**
 * @brief AES encrypt
 *
 * @param mode supported mode: AES_CBC, AES_CFB, AES_CTR, AES_ECB, AES_OFB
 * @param key aes key
 * @param key_len AES_KEY_128, AES_KEY_192, AES_KEY_256
 * @param iv iv length must be 16 bytes; or pass NULL if ECB mode
 * @param input input data (plain)
 * @param input_len input data (plain) length must be multiple of 16bytes, or this API will pad to 16bytes with 0, take care of the output length!
 * @param[out] output output data (cipher)
 * @return cr_status
 */
cr_status aes_encrypt_le(AES_MODE_TYPE mode, const uint8_t *key, AES_KEY_LEN key_len, const uint8_t *iv, const uint8_t *input, uint32_t input_len, uint8_t *output)
{
    cr_status ret = ERR_NONE;
    int       result = 0;
    uint8_t   keytype;
    uint8_t   data[16] = { 0 };

    if ((key == NULL) || (input == NULL) || (output == NULL))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }

    if ((mode != AES_ECB) && (iv == NULL))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }

    switch (key_len)
    {
        case AES_KEY_128:
            keytype = AES_128;
            break;
        case AES_KEY_192:
            keytype = AES_192;
            break;
        case AES_KEY_256:
            keytype = AES_256;
            break;
        default:
            ret = ERR_DATA_LENGTH;
            return ret;
    }

    ll_sce_lock();
    sce_Sysinit_Aes();
    if (input_len >= 16)
    {
        result = AESOperation(key, key_len, output, iv, input, input_len / 16 * 16, keytype, mode, AF_ENC);
    }
    /* pad 0 */
    if ((result == (int)(input_len / 16 * 16)) && (input_len % 16 != 0))
    {
        memcpy(data, input + input_len / 16 * 16, input_len % 16);
        /* data[input_len%16 .. 15] already zero from initialisation above */

        /* Each mode needs a different IV for the second AESOperation call because
         * AESOperation re-initialises the hardware (calls AESInit) on every
         * invocation, discarding any internal chaining state.
         *
         * CTR  : advance the counter by the number of full blocks processed.
         * CBC  : the chaining value is the last produced ciphertext block.
         * CFB  : the feedback register is the last produced ciphertext block.
         * OFB  : the intermediate keystream block is NOT stored in the output
         *        buffer and cannot be recovered after AESOperation returns.
         *        Partial-block OFB is therefore unsupported; return ERR_PARAMETERS.
         * ECB  : no IV at all; partial_iv is unused (AESOperation ignores iv for ECB). */
        const uint8_t *partial_iv = iv;
        uint8_t        updated_iv[16];
        uint32_t       n = input_len / 16;

        if (mode == AES_CTR)
        {
            /* Advance the counter field (iv[8..15], big-endian uint64) by n. */
            uint64_t ctr;
            memcpy(updated_iv, iv, 16);
            ctr = ((uint64_t)updated_iv[8]  << 56) |
                  ((uint64_t)updated_iv[9]  << 48) |
                  ((uint64_t)updated_iv[10] << 40) |
                  ((uint64_t)updated_iv[11] << 32) |
                  ((uint64_t)updated_iv[12] << 24) |
                  ((uint64_t)updated_iv[13] << 16) |
                  ((uint64_t)updated_iv[14] << 8)  |
                  ((uint64_t)updated_iv[15]);
            ctr += n;
            updated_iv[8]  = (uint8_t)(ctr >> 56);
            updated_iv[9]  = (uint8_t)(ctr >> 48);
            updated_iv[10] = (uint8_t)(ctr >> 40);
            updated_iv[11] = (uint8_t)(ctr >> 32);
            updated_iv[12] = (uint8_t)(ctr >> 24);
            updated_iv[13] = (uint8_t)(ctr >> 16);
            updated_iv[14] = (uint8_t)(ctr >> 8);
            updated_iv[15] = (uint8_t)(ctr);
            partial_iv = updated_iv;
        }
        else if (mode == AES_CBC || mode == AES_CFB)
        {
            /* The chaining/feedback state after N full blocks is the last
             * ciphertext block written to output. */
            partial_iv = output + (n - 1u) * 16u;
        }
        else if (mode == AES_OFB)
        {
            /* OFB keystream state is internal to the hardware and not
             * recoverable from the output buffer.  Partial-block OFB is
             * unsupported; the caller must supply block-aligned input. */
            ll_sce_unlock();
            return ERR_PARAMETERS;
        }

        result = AESOperation(key, key_len, output + n * 16u, partial_iv, data, 16, keytype, mode, AF_ENC);
    }
    ll_sce_unlock();
    if (result < 0)
    {
        ret = ERR_NORESULT;
    }
    return ret;
}

/**
 * @brief AES decrypt
 *
 * @param mode supported mode: AES_CBC, AES_CFB, AES_CTR, AES_ECB, AES_OFB
 * @param key aes key
 * @param key_len AES_KEY_128, AES_KEY_192, AES_KEY_256
 * @param iv
 * @param input input data (cipher)
 * @param input_len input data (cipher) length must be multiple of 16 bytes!
 * @param[out] output output data (plain)
 * @return cr_status
 */
cr_status aes_decrypt_le(AES_MODE_TYPE mode, const uint8_t *key, AES_KEY_LEN key_len, const uint8_t *iv, const uint8_t *input, uint32_t input_len, uint8_t *output)
{
    cr_status ret = ERR_NONE;
    int       result;
    uint8_t   keytype;

    if ((key == NULL) || (input == NULL) || (output == NULL))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }

    if ((mode != AES_ECB) && (iv == NULL))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }

    if (input_len % 16 != 0)
    {
        ret = ERR_DATA_LENGTH;
        return ret;
    }

    switch (key_len)
    {
        case AES_KEY_128:
            keytype = AES_128;
            break;
        case AES_KEY_192:
            keytype = AES_192;
            break;
        case AES_KEY_256:
            keytype = AES_256;
            break;
        default:
            ret = ERR_PARAMETERS;
            return ret;
    }

    ll_sce_lock();
    sce_Sysinit_Aes();
    result = AESOperation(key, key_len, output, iv, input, input_len, keytype, mode, AF_DEC);
    ll_sce_unlock();

    if (result < 0)
    {
        ret = ERR_NORESULT;
    }
    return ret;
}

/**
 * @brief result = x+y mod n, x<n, y<n; length of result = n_len
 *
 * @param x pointer to x
 * @param y pointer to y
 * @param n pointer to n
 * @param x_len length of x, number of words; x_len < (4096/32)
 * @param y_len length of y, number of words; y_len < (4096/32)
 * @param n_len length of n, number of words; n_len < (4096/32)
 * @param[out] result length of result = n_len
 * @return cr_status
 */
cr_status modulo_add_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result)
{
    bn_context_t ctx;

    if ((x == NULL) || (y == NULL) || (n == NULL) || (result == NULL))
    {
        return ERR_PARAMETERS;
    }

    ll_sce_lock();
    sce_Sysinit_Pke();
    bn_addsub_initpro();
    bn_init(&ctx, n, n_len, false);
    bn_add_mod(&ctx, x, y, x_len, y_len, result);
    ll_sce_unlock();

    return ERR_NONE;
}

/**
 * @brief result = x-y mod n, x<n, y<n; length of result = n_len
 *
 * @param x pointer to x
 * @param y pointer to y
 * @param n pointer to n
 * @param x_len length of x, number of words; x_len < (4096/32)
 * @param y_len length of y, number of words; y_len < (4096/32)
 * @param n_len length of n, number of words; n_len < (4096/32)
 * @param[out] result length of result = n_len
 * @return cr_status
 */
cr_status modulo_sub_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result)
{
    bn_context_t ctx;

    if ((x == NULL) || (y == NULL) || (n == NULL) || (result == NULL))
    {
        return ERR_PARAMETERS;
    }

    ll_sce_lock();
    sce_Sysinit_Pke();
    bn_addsub_initpro();
    bn_init(&ctx, n, n_len, false);
    bn_sub_mod(&ctx, x, y, x_len, y_len, result);
    ll_sce_unlock();

    return ERR_NONE;
}

/**
 * @brief result = x*y mod n, x<n, y<n; length of result = n_len
 *
 * @param x pointer to x
 * @param y pointer to y
 * @param n pointer to n
 * @param x_len length of x, number of words; x_len < (4096/32)
 * @param y_len length of y, number of words; y_len < (4096/32)
 * @param n_len length of n, number of words; n_len < (4096/32)
 * @param[out] result length of result = n_len
 * @return cr_status
 */
cr_status modulo_multiply_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result)
{
    bn_context_t ctx;

    if ((x == NULL) || (y == NULL) || (n == NULL) || (result == NULL))
    {
        return ERR_PARAMETERS;
    }

    ll_sce_lock();
    sce_Sysinit_Pke();
    bn_init(&ctx, n, n_len, true);
    bn_mult_mod(&ctx, x, y, x_len, y_len, result);
    ll_sce_unlock();

    return ERR_NONE;
}

/**
 * @brief result = x^y mod n, x<n, y<n; length of result = n_len
 *
 * @param x pointer to x
 * @param y pointer to y
 * @param n pointer to n
 * @param x_len length of x, number of words; x_len < (4096/32)
 * @param y_len length of y, number of words; y_len < (4096/32)
 * @param n_len length of n, number of words; n_len < (4096/32)
 * @param[out] result length of result = n_len
 * @return cr_status
 */
cr_status modulo_expo_le(const uint32_t *x, const uint32_t *y, const uint32_t *n, uint32_t x_len, uint32_t y_len, uint32_t n_len, uint32_t *result)
{
    bn_context_t ctx;

    if ((x == NULL) || (y == NULL) || (n == NULL) || (result == NULL))
    {
        return ERR_PARAMETERS;
    }

    /* if exponent is 0, result = 1 (no hardware access needed) */
    if (bnu_is_zero(y, y_len))
    {
        memset(result, 0x0, n_len * sizeof(uint32_t));
        result[0] = 1;
        return ERR_NONE;
    }

    ll_sce_lock();
    sce_Sysinit_Pke();
    bn_init(&ctx, n, n_len, true);
    bn_expo_mod_sn(&ctx, x, y, x_len, y_len, result);
    ll_sce_unlock();

    return ERR_NONE;
}

/**
 * @brief result = 1/x mod n, x<n, x and n must be relatively prime
 *
 * @param x pointer to x
 * @param n pointer to n
 * @param x_len length of x, number of words; x_len < (4096/32)
 * @param n_len length of n, number of words; n_len < (4096/32)
 * @param[out] result length of result = n_len
 * @return cr_status
 */
cr_status modulo_inverse_le(const uint32_t *x, const uint32_t *n, uint32_t x_len, uint32_t n_len, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    int32_t   rn;

    if ((x == NULL) || (n == NULL) || (result == NULL))
    {
        return ERR_PARAMETERS;
    }

    ll_sce_lock();
    sce_Sysinit_Pke();
    rn = bn_inv_mod(x, n, x_len, n_len, result);
    ll_sce_unlock();

    if (rn == -1)
    {
        /* x and n are not coprime: hardware INV_READY bit never set */
        ret = ERR_PARAMETERS;
    }
    return ret;
}

/**
 * @brief X/Y = Q...R. X is dividend, Y is divisor, Q is quotient, R is remainder; xlen, ylen, qlen, rlen are word length; little endian.
 * The value of x and y must be 64-bit aligned and be actual value. So if xlen/ylen is odd number user needs to reserve 1 word of space to pad the value.
 *
 * @param x
 * @param xlen length of x (number of words). xlen must 64bit alignment
 * @param y
 * @param ylen length of y (number of words)
 * @param[out] q
 * @param[out] r
 * @param[out] qlen length of q (number of words)
 * @param[out] rlen length of r (number of words)
 * @return cr_status
 */
cr_status bndivision_le(const uint32_t *x, uint32_t xlen, const uint32_t *y, uint32_t ylen, uint32_t *q, uint32_t *qlen, uint32_t *r, uint32_t *rlen)
{
    if ((x == NULL) || (y == NULL))
    {
        return ERR_PARAMETERS;
    }

    xlen = bnu_get_nzw_le(x, xlen); /* xlen and ylen must be the real length */
    ylen = bnu_get_nzw_le(y, ylen);

    int32_t cmp = bnu_cmp_le(x, xlen, y, ylen);

    if ((q == NULL) && (r == NULL))
    {
        return ERR_NORESULT;
    }
    /* check divisor not zero */
    if (bnu_is_zero(y, ylen))
    {
        return ERR_PARAMETERS;
    }
    if (cmp < 0)
    {
        /* X < Y, Q = 0 */
        if (q != NULL)
        {
            q[0] = 0;
        }
        if (qlen != NULL)
        {
            *qlen = 1;
        }
        if (r != NULL)
        {
            memcpy(r, x, xlen * 4);
        }
        if (rlen != NULL)
        {
            *rlen = xlen;
        }
        return ERR_NONE;
    }
    if (cmp == 0)
    {
        /* X = Y, Q = 1 */
        if (q != NULL)
        {
            q[0] = 1;
        }
        if (qlen != NULL)
        {
            *qlen = 1;
        }
        if (r != NULL)
        {
            r[0] = 0;
        }
        if (rlen != NULL)
        {
            *rlen = 1;
        }
        return ERR_NONE;
    }

    ll_sce_lock();
    uint32_t rv = alu_division(x, y, xlen, ylen, q, qlen, r, rlen);
    ll_sce_unlock();
    return (rv != 0u) ? ERR_NORESULT : ERR_NONE;
}

/**
 * @brief result = greatest common divisor of x, y; if x or y is 0, return Error!
 * x_len, y_len are number of words, <(4096/32); length of result = min(x_len, y_len)
 *
 * @param x pointer to x
 * @param y pointer to y
 * @param x_len length of x (number of words)
 * @param y_len length of x (number of words)
 * @param[out] result length of result = min(x_len, y_len)
 * @return cr_status
 */
cr_status gcd_le(const uint32_t *x, const uint32_t *y, uint32_t x_len, uint32_t y_len, uint32_t *result)
{
    if ((x == NULL) || (y == NULL) || (result == NULL))
    {
        return ERR_PARAMETERS;
    }
    /* check Input not zero */
    if (bnu_is_zero(x, x_len) || bnu_is_zero(y, y_len))
    {
        return ERR_PARAMETERS;
    }
    ll_sce_lock();
    sce_Sysinit_Pke();
    bn_gcd(x, y, x_len, y_len, result);
    ll_sce_unlock();
    return ERR_NONE;
}

/**
 * @brief get prime of the curve big-endian
 *
 * @param curve supported curve: CT_SECP256K1, CT_SECP256R1
 * @param[out] prime
 * @return cr_status
 */
cr_status cv_get_prime_be(curve_type curve, uint32_t *prime)
{
    cr_status ret = ERR_NONE;

    if (prime == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memcpy(prime, secp256k1_prime, 32);
            break;
        case CT_SECP256R1:
            memcpy(prime, secp256r1_prime, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get order of the curve big-endian
 *
 * @param curve supported curve: CT_SECP256K1, CT_SECP256R1
 * @param[out] order
 * @return cr_status
 */
cr_status cv_get_order_be(curve_type curve, uint32_t *order)
{
    cr_status ret = ERR_NONE;

    if (order == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memcpy(order, secp256k1_order, 32);
            break;
        case CT_SECP256R1:
            memcpy(order, secp256r1_order, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get order half of the curve
 *
 * @param curve supported curve: CT_SECP256K1, CT_SECP256R1
 * @param[out] order_half
 * @return cr_status
 */
cr_status cv_get_order_half_be(curve_type curve, uint32_t *order_half)
{
    cr_status ret = ERR_NONE;

    if (order_half == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memcpy(order_half, secp256k1_order_half, 32);
            break;
        case CT_SECP256R1:
            memcpy(order_half, secp256r1_order_half, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get the a of the curve. y^2 = x^3 + ax + b; big-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] a 32 bytes buffer to store a
 * @return cr_status
 */
cr_status cv_get_a_be(curve_type curve, uint32_t *a)
{
    cr_status ret = ERR_NONE;

    if (a == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memset(a, 0, 32);
            break;
        case CT_SECP256R1:
            memcpy(a, secp256r1_a, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get the b of the curve. y^2 = x^3 + ax + b; big-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] b 32 bytes buffer to store b
 * @return cr_status
 */
cr_status cv_get_b_be(curve_type curve, uint32_t *b)
{
    uint8_t *ptr_b = (uint8_t *)b;

    cr_status ret = ERR_NONE;

    if (b == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memset(b, 0, 32);
            ptr_b[31] = 7;
            break;
        case CT_SECP256R1:
            memcpy(b, secp256r1_b, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get G.x; big-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] x
 * @return cr_status
 */
cr_status cv_get_g_x_be(curve_type curve, uint32_t *x)
{
    cr_status ret = ERR_NONE;

    if (x == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memcpy(x, secp256k1_g_x, 32);
            break;
        case CT_SECP256R1:
            memcpy(x, secp256r1_g_x, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get G.y
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] y
 * @return cr_status
 */
cr_status cv_get_g_y_be(curve_type curve, uint32_t *y)
{
    cr_status ret = ERR_NONE;

    if (y == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memcpy(y, secp256k1_g_y, 32);
            break;
        case CT_SECP256R1:
            memcpy(y, secp256r1_g_y, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get prime of the curve little-endian
 *
 * @param curve supported curve: CT_SECP256K1, CT_SECP256R1
 * @param[out] prime
 * @return cr_status
 */
cr_status cv_get_prime_le(curve_type curve, uint32_t *prime)
{
    cr_status ret = ERR_NONE;

    if (prime == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            bnu_swap_endian_2(secp256k1_prime, prime, 32);
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_prime, prime, 32);
            break;
        case CT_ED25519:
            memcpy(prime, ED25519_P_D, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get order of the curve little-endian
 *
 * @param curve supported curve: CT_SECP256K1, CT_SECP256R1
 * @param[out] order
 * @return cr_status
 */
cr_status cv_get_order_le(curve_type curve, uint32_t *order)
{
    cr_status ret = ERR_NONE;

    if (order == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            bnu_swap_endian_2(secp256k1_order, order, 32);
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_order, order, 32);
            break;
        case CT_ED25519:
            memcpy(order, ED25519_L, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get order half of the curve; little-endian
 *
 * @param curve supported curve: CT_SECP256K1, CT_SECP256R1
 * @param[out] order_half
 * @return cr_status
 */
cr_status cv_get_order_half_le(curve_type curve, uint32_t *order_half)
{
    cr_status ret = ERR_NONE;

    if (order_half == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            bnu_swap_endian_2(secp256k1_order_half, order_half, 32);
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_order_half, order_half, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get the a of the curve. y^2 = x^3 + ax + b; little-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] a 32 bytes buffer to store a
 * @return cr_status
 */
cr_status cv_get_a_le(curve_type curve, uint32_t *a)
{
    cr_status ret = ERR_NONE;

    if (a == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memset(a, 0, 32);
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_a, a, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get the b of the curve. y^2 = x^3 + ax + b; little-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] b 32 bytes buffer to store b
 * @return cr_status
 */
cr_status cv_get_b_le(curve_type curve, uint32_t *b)
{
    uint8_t *ptr_b = (uint8_t *)b;

    cr_status ret = ERR_NONE;

    if (b == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            memset(b, 0, 32);
            ptr_b[0] = 7;
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_b, b, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get the d of the ed25519. av^2 + w^2 = 1 + dv^2 * w^2; little-endian
 *
 * @param curve curve type: CT_ED25519
 * @param[out] d 32 bytes buffer to store d
 * @return cr_status
 */
cr_status cv_get_d_le(curve_type curve, uint32_t *d)
{
    cr_status ret = ERR_NONE;

    if (d == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_ED25519:
            memcpy(d, ED25519_P_D + 8, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get G.x; little-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] x
 * @return cr_status
 */
cr_status cv_get_g_x_le(curve_type curve, uint32_t *x)
{
    cr_status ret = ERR_NONE;

    if (x == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            bnu_swap_endian_2(secp256k1_g_x, x, 32);
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_g_x, x, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief get G.y; little-endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param[out] y
 * @return cr_status
 */
cr_status cv_get_g_y_le(curve_type curve, uint32_t *y)
{
    cr_status ret = ERR_NONE;

    if (y == NULL)
        return ERR_PARAMETERS;

    switch (curve)
    {
        case CT_SECP256K1:
            bnu_swap_endian_2(secp256k1_g_y, y, 32);
            break;
        case CT_SECP256R1:
            bnu_swap_endian_2(secp256r1_g_y, y, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief init PKE for curve calculation
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param modLen
 * @return cr_status
 */
cr_status cv_init(curve_type curve, uint16_t modLen)
{
    cr_status ret = ERR_NONE;

    switch (curve)
    {
        case CT_SECP256K1:
            secp256k1_init(modLen);
            break;
        case CT_SECP256R1:
            secp256r1_init(modLen);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief curve only! result = x+y mod n, x<n, y<n; little endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param modulo_type modulo type: MT_N, MT_P
 * @param x pointer to x
 * @param y pointer to y
 * @param[out] result
 * @return cr_status
 */
cr_status cv_modulo_add_le(curve_type curve, modulo_t modulo_type, const uint32_t *x, const uint32_t *y, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    uint32_t  modulo[8];

    if ((x == NULL) || (y == NULL) || (result == NULL))
        return ERR_PARAMETERS;

    if (modulo_type == MT_N)
        ret = cv_get_order_le(curve, modulo);
    else if (modulo_type == MT_P)
        ret = cv_get_prime_le(curve, modulo);
    else
        return ERR_PARAMETERS;

    if (ret != ERR_NONE)
        return ret;

    switch (curve)
    {
        case CT_SECP256R1:
        case CT_SECP256K1:
        case CT_ED25519:
            ll_sce_lock();
            sce_Sysinit_Pke();
            curve_modular_add(modulo, x, y, result, CURVE_256);
            ll_sce_unlock();
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    return ret;
}

/**
 * @brief curve only! result = x-y mod n, x<n, y<n; little endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param modulo_type modulo type: MT_N, MT_P
 * @param x pointer to x
 * @param y pointer to y
 * @param[out] result
 * @return cr_status
 */
cr_status cv_modulo_sub_le(curve_type curve, modulo_t modulo_type, const uint32_t *x, const uint32_t *y, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    uint32_t  modulo[8];

    if ((x == NULL) || (y == NULL) || (result == NULL))
        return ERR_PARAMETERS;

    if (modulo_type == MT_N)
        ret = cv_get_order_le(curve, modulo);
    else if (modulo_type == MT_P)
        ret = cv_get_prime_le(curve, modulo);
    else
        return ERR_PARAMETERS;

    if (ret != ERR_NONE)
        return ret;

    switch (curve)
    {
        case CT_SECP256R1:
        case CT_SECP256K1:
        case CT_ED25519:
            ll_sce_lock();
            sce_Sysinit_Pke();
            curve_modular_sub(modulo, x, y, result, CURVE_256);
            ll_sce_unlock();
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    return ret;
}

/**
 * @brief curve only! result = x*y mod n, x<n, y<n; little endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1
 * @param modulo_type modulo type: MT_N, MT_P
 * @param x pointer to x
 * @param y pointer to y
 * @param[out] result
 * @return cr_status
 */
cr_status cv_modulo_mul_le(curve_type curve, modulo_t modulo_type, const uint32_t *x, const uint32_t *y, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    uint32_t  modulo[8];

    if ((x == NULL) || (y == NULL) || (result == NULL))
        return ERR_PARAMETERS;

    if (modulo_type == MT_N)
        ret = cv_get_order_le(curve, modulo);
    else if (modulo_type == MT_P)
        ret = cv_get_prime_le(curve, modulo);
    else
        return ERR_PARAMETERS;

    if (ret != ERR_NONE)
        return ret;

    switch (curve)
    {
        case CT_SECP256R1:
        case CT_SECP256K1:
        case CT_ED25519:
            ll_sce_lock();
            sce_Sysinit_Pke();
            curve_modular_mul(modulo, x, y, result, CURVE_256);
            ll_sce_unlock();
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    return ret;
}

/**
 * @brief point add on curve: result = px + py; little endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1, CT_ED25519
 * @param px pointer to px
 * @param py pointer to py
 * @param[out] result
 * @return cr_status
 */
cr_status point_add_le(curve_type curve, const uint32_t *px, const uint32_t *py, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    pointDef  Res, A, B;

    if ((px == NULL) || (py == NULL) || (result == NULL))
        return ERR_PARAMETERS;

    ll_sce_lock();

    switch (curve)
    {
        case CT_SECP256K1:
        case CT_SECP256R1:
            sce_Sysinit_Pke();
            cv_init(curve, 0);
            if (memcmp(px, py, 64u) == 0)
            {
                /* P1==P2: chord formula is undefined; use tangent formula (point doubling) */
                ecc_PointDouble(px, result, CURVE_256);
            }
            else
            {
                ecc_PointAdd(px, py, result, CURVE_256);
            }
            break;
        case CT_ED25519:
            sce_Sysinit_Pke();
            memcpy(A.x, px, 32);
            memcpy(A.y, px + 8, 32);
            memcpy(B.x, py, 32);
            memcpy(B.y, py + 8, 32);
            memset((void *)&Res, 0, sizeof(Res));
            ed25519_init(0);
            ed25519_PointAdd(&A, &B, &Res);
            memcpy(result, Res.x, 32);
            memcpy(result + 8, Res.y, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    ll_sce_unlock();
    return ret;
}

/**
 * @brief multiply on curve: result = x * py, x is 32bytes; little endian
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1, CT_ED25519
 * @param x pointer to x
 * @param py pointer to py
 * @param[out] result
 * @return cr_status
 */
cr_status point_multiply_le(curve_type curve, const uint32_t *x, const uint32_t *py, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    pointDef  Res, B;
    uint16_t  bitLen;

    if ((x == NULL) || (py == NULL) || (result == NULL))
        return ERR_PARAMETERS;

    ll_sce_lock();

    switch (curve)
    {
        case CT_SECP256R1:
        case CT_SECP256K1:
            sce_Sysinit_Pke();
            cv_init(curve, 0);
            if (count_one_bits((uint8_t *)x, 32, littleEnd) == 1u)
            {
                /* k=1: hardware ECPM cannot handle OPTEW=1; 1*P == P */
                memcpy(result, py, 64u);
            }
            else
            {
                ecc_PointMul((uint8_t *)x, py, result, CURVE_256);
            }
            break;
        case CT_ED25519:
            sce_Sysinit_Pke();
            bitLen = count_one_bits((uint8_t *)x, 32, littleEnd);
            ed25519_init(bitLen);
            memcpy(B.x, py, 32);
            memcpy(B.y, py + 8, 32);
            ed25519_PointMul((uint8_t *)x, &B, &Res);
            memcpy(result, Res.x, 32);
            memcpy(result + 8, Res.y, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    ll_sce_unlock();
    return ret;
}

/**
 * @brief G point multiply on curve: result = x * G, x is 32bytes; little endian
 * G is base point of curve, decided by curve type, no need input;
 *
 * @param curve curve type: CT_SECP256K1, CT_SECP256R1, CT_ED25519
 * @param x pointer to x
 * @param[out] result
 * @return cr_status
 */
cr_status scalar_multiply_le(curve_type curve, const uint32_t *x, uint32_t *result)
{
    cr_status ret = ERR_NONE;
    pointDef  Res, G;
    uint16_t  bitLen;

    if ((x == NULL) || (result == NULL))
        return ERR_PARAMETERS;

    ll_sce_lock();

    switch (curve)
    {
        case CT_SECP256R1:
        case CT_SECP256K1:
            sce_Sysinit_Pke();
            cv_init(curve, 0);
            if (count_one_bits((uint8_t *)x, 32, littleEnd) == 1u)
            {
                /* k=1: hardware ECPM cannot handle OPTEW=1; 1*G == G */
                memcpy(result, ec_basePonitPtr, 64u);
            }
            else
            {
                ecc_PointMul_Base((uint8_t *)x, result, CURVE_256);
            }
            break;
        case CT_ED25519:
            sce_Sysinit_Pke();
            bitLen = count_one_bits((uint8_t *)x, 32, littleEnd);
            ed25519_init(bitLen);
            memcpy(G.x, ED25519_BASE_POINT, 32);
            memcpy(G.y, ED25519_BASE_POINT + 8, 32);
            ed25519_PointMul((uint8_t *)x, &G, &Res);
            memcpy(result, Res.x, 32);
            memcpy(result + 8, Res.y, 32);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    ll_sce_unlock();
    return ret;
}

const uint32_t seg_size[SEG_NUM] = { 256, 256, 256, 256, 512, 256, 256, 0, 512, 1024, 1024, 1024, 1024, 256, 256, 256, 1024, 1024 };

/**
 * @brief sdma move data from segment to segment (sce internal)
 *
 * @param segid_src id of segment data read from
 * @param src_woff id of segment data write to
 * @param segid_des offset in segment, number of words
 * @param des_woff offset in segment, number of words
 * @param wsize number of words to be moved
 * @param opt endian swap; ToDo: not verify yet, keep 0 now!
 * @return cr_status
 */
cr_status data_move_i(uint32_t segid_src, uint32_t src_woff, uint32_t segid_des, uint32_t des_woff, uint32_t wsize, uint32_t opt)
{
    cr_status ret = ERR_NONE;
    if ((segid_src >= SEG_NUM) || (segid_des >= SEG_NUM) || (segid_src == segid_des) || (wsize == 0))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }
    if ((src_woff * 4 >= seg_size[segid_src]) || (des_woff * 4 >= seg_size[segid_des]))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }
    SCE_SDMA_CLK_EN();
    Ich_TranData(opt, segid_src, src_woff, segid_des, des_woff, wsize);
    return ret;
}

/**
 * @brief sdma move data between external memory and segment (external)
 *
 * @param rw 0, from external memory to segment; 1, from segment to external memory;
 * @param segid id of segment;
 * @param seg_woff offset in segment, number of words;
 * @param exaddr address of external memory
 * @param wsize number of words to be moved;
 * @param opt endian swap; ToDo: not verify yet, keep 0 now!
 * @return cr_status
 */
cr_status data_move_e(uint32_t rw, uint32_t segid, uint32_t seg_woff, uint32_t *exaddr, uint32_t wsize, uint32_t opt)
{
    cr_status ret = ERR_NONE;

    if ((segid >= SEG_NUM) || (wsize == 0))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }
    if (seg_woff * 4 >= seg_size[segid])
    {
        ret = ERR_PARAMETERS;
        return ret;
    }
    if ((rw != 0) && (rw != 1))
    {
        ret = ERR_PARAMETERS;
        return ret;
    }
    return ret;
}


/**
 * @brief sdma move data between external memory and segment with security mode (security external)
 *
 * @param rw 0, from external memory to segment; 1, from segment to external memory;
 * @param segid id of segment;
 * @param seg_woff offset in segment, number of words;
 * @param exaddr address of external memory
 * @param wsize number of words to be moved;
 * @param opt endian swap; ToDo: not verify yet, keep 0 now!
 * @return cr_status
 */
cr_status data_move_s(uint32_t rw, uint32_t segid, uint32_t seg_woff, uint32_t *exaddr, uint32_t wsize, uint32_t opt)
{
    cr_status ret = ERR_NONE;
    return ret;
}

/**
 * @brief TRNG init
 *
 * @return cr_status
 */
cr_status rng_init(void)
{
    trng_continuous_contex_init();

    return ERR_NONE;
}

/**
 * @brief get random number
 *
 * @param buffer pointer to buffer
 * @param length ramdom number size. length of words. must < 1024/4
 * @return cr_status
 */
cr_status rng_buffer(uint32_t *buffer, uint32_t length)
{
    bool ok;

    if (buffer == NULL)
        return ERR_PARAMETERS;

    for (uint32_t retry = 0; retry < 10; retry++)
    {
        ll_sce_lock();
        ok = trng_continuous_contex_get_buffer(buffer, length);
        ll_sce_unlock();

        if (!ok)
        {
            printf("trng_continuous_contex_get_buffer: TimeOut\r\n");
            return ERR_TIMEOUT;
        }
        if (buffer[0] != 0 && buffer[length - 1] != 0)
        {
            return ERR_NONE;
        }
    }
    printf("rng_buffer always 0.\r\n");
    return ERR_TIMEOUT;
}

/**
 * @brief get a rand (len words) with limit, rand < limit;
 *
 * @param rand random number buffer
 * @param len length of random number buffer
 * @param lmt limit buffer
 * @param lmtlen length of limit buffer
 */
void bn_random_limit_le(uint32_t *rand, uint32_t len, const uint32_t *lmt, uint32_t lmtlen)
{
    if (rand == NULL)
        return;

    if (lmt == NULL)
    {
        rng_buffer(rand, len);
        return;
    }
    /* get bit length and word length of limit */
    uint32_t lmtbitlen = bnu_get_msb_le(lmt, lmtlen) + 1;
    if (len < (lmtbitlen + 31) / 32)
    {
        rng_buffer(rand, len);
        return;
    }
    memset(rand, 0, len * 4);
    rng_buffer(rand, (lmtbitlen + 31) / 32);
    if (lmtbitlen % 32)
    {
        rand[(lmtbitlen + 31) / 32 - 1] &= (1 << (lmtbitlen % 32)) - 1;
    }
    /* make sure rand < lmt */
    if (bnu_cmp_le(rand, len, lmt, lmtlen) > 0)
    {
        rand[(lmtbitlen + 31) / 32 - 1] &= lmt[(lmtbitlen + 31) / 32 - 1] - 1;
    }
}