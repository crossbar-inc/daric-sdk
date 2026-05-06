/**
 ******************************************************************************
 * @file    schnorr.c
 * @author  SCE Team
 * @brief   Schnorr signature implementation file.
 *          This file provides firmware functions for Schnorr signature
 *          generation and verification.
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

#include "schnorr.h"
#include <string.h>
#include <stdio.h>
#include "hash.h"
#include "ecdsa.h"
#include "bn_util.h"

#define SCHNORR_DBG 0

#if SCHNORR_DBG
#define SCHNORR_DUMP_BUFFER(name, ptr, len) dump_buffer(name, ptr, len)
#define SCHNORR_DEBUG(format, ...)          printf(format, ##__VA_ARGS__)

static void dump_buffer(const char *name, const void *buf, uint32_t len)
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
#define SCHNORR_DUMP_BUFFER(name, ptr, len)
#define SCHNORR_DEBUG(format, ...)
#endif

static void tagged_sha256_init(uint8_t *tag, uint32_t tag_length)
{
    uint8_t tag_hash[32];

    sce_Sysinit_Hash();
    hash_init();

    sha256_512_calc(tag, tag_length, SHA256_MODE, tag_hash);
    SCHNORR_DEBUG("hash tag: %s %d\n", tag, tag_length);
    SCHNORR_DUMP_BUFFER("tag hash:", tag_hash, 32);
    sha256_512_Init();
    sha256_512_Input(tag_hash, 32, SHA256_MODE);
    sha256_512_Input(tag_hash, 32, SHA256_MODE);
}

static void tagged_sha256_input(uint8_t *data, uint32_t data_length)
{
    sha256_512_Input(data, data_length, SHA256_MODE);
}

static void tagged_sha256_input_finish(uint8_t *data, uint32_t data_length, uint32_t *result)
{
    sha256_512_Result(data, data_length, SHA256_MODE, (uint8_t *)result);
}

static uint8_t nonce_tag[13]     = "BIP0340/nonce";
static uint8_t challenge_tag[17] = "BIP0340/challenge";
/*  t be the byte-wise xor of bytes(d) and hashBIP0340/aux(a) ;
    k = hashBIP0340/nonce(t || bytes(P) || m) */
/**
 * @brief Generate a deterministic nonce k according to BIP-340.
 *
 * @param priv_key      Private key as a big-endian byte array (32 bytes).
 * @param xonly_pubkey  X-only public key as a big-endian byte array (32 bytes).
 * @param hash          Message digest as a big-endian byte array (32 bytes).
 * @param[out] k        Generated nonce k (32 bytes).
 *
 * @note This implementation follows the BIP-340 recommendation for k generation:
 *       k = hash_BIP0340/nonce(bytes(d) || bytes(P) || m)
 */
static void schnorr_generate_k(const uint32_t *priv_key, const uint32_t *xonly_pubkey, const uint32_t *hash, uint32_t *k)
{
    uint8_t *key = (uint8_t *)priv_key;
    int32_t  i;
    uint8_t  masked_key[32];

    sce_Sysinit_Hash();
    hash_init();

    /* Precomputed TaggedHash("BIP0340/aux", 0x0000...00); */
    static const unsigned char ZERO_MASK[32]
        = { 84, 241, 105, 207, 201, 226, 229, 114, 116, 128, 68, 31, 144, 186, 37, 196, 136, 244, 97, 199, 11, 94, 165, 220, 170, 247, 175, 105, 39, 10, 165, 20 };
    for (i = 0; i < 32; i++)
    {
        masked_key[i] = key[i] ^ ZERO_MASK[i];
    }

    SCHNORR_DUMP_BUFFER("priv_key:", (uint8_t *)priv_key, 32);
    SCHNORR_DUMP_BUFFER("masked_key:", masked_key, 32);
    SCHNORR_DUMP_BUFFER("xonly_pubkey:", (uint8_t *)xonly_pubkey, 32);
    SCHNORR_DUMP_BUFFER("hash:", (uint8_t *)hash, 32);

    tagged_sha256_init(nonce_tag, sizeof(nonce_tag));
    tagged_sha256_input(masked_key, 32);
    tagged_sha256_input((uint8_t *)xonly_pubkey, 32);
    tagged_sha256_input_finish((uint8_t *)hash, 32, k);

    return;
}

/**
 * @brief Calculate the Schnorr challenge e = hash(R || P || m) according to BIP-340.
 *
 * @param curve     Elliptic curve (CT_SECP256K1).
 * @param rx        X-coordinate of the commitment point R (32 bytes, big-endian).
 * @param pubkey32  X-only public key P (32 bytes, big-endian).
 * @param digest    Message digest m (32 bytes, big-endian).
 * @param[out] e    Resulting challenge e mod n (32 bytes, little-endian).
 */
static void schnorr_calculate_e(curve_type curve, const uint8_t *rx, const uint8_t *pubkey32, const uint8_t *digest, uint32_t *e)
{
    uint32_t order[8];

    /* hashBIP0340/challenge(bytes(R) || bytes(P) || m) */
    tagged_sha256_init(challenge_tag, 17);
    tagged_sha256_input((uint8_t *)rx, 32);
    tagged_sha256_input((uint8_t *)pubkey32, 32);
    tagged_sha256_input_finish((uint8_t *)digest, 32, e);

    /* e = e mod n*/
    sce_Sysinit_Pke();
    cv_get_order_le(curve, order);
    bnu_swap_endian_1(e, 32);
    curve_rmodL((uint8_t *)e, 32, (uint8_t *)order, 32, (uint8_t *)e);

    SCHNORR_DUMP_BUFFER("e: ", e, 32);
}

/**
 * @brief Sign a message digest using Schnorr signature (BIP-340).
 *
 * @param curve    Elliptic curve (CT_SECP256K1).
 * @param priv_key Private key as a big-endian byte array (32 bytes).
 * @param digest   Message digest as a big-endian byte array (32 bytes).
 * @param[out] sig Signature output: 64 bytes (r || s), both big-endian.
 *
 * @return 0 on success, -1 on failure.
 *
 * @note This function ensures d = n - d if P has an odd y-coordinate,
 *       and k = n - k if R has an odd y-coordinate, as per BIP-340.
 */
int32_t schnorr_sign_digest(curve_type curve, const uint32_t *priv_key, const uint32_t *digest, uint32_t *sig)
{
    // uint32_t pub_key[8];
    pointDef pk, r;
    uint32_t order[8];
    uint32_t k[8];
    uint32_t d[8];
    uint32_t e[8];
    uint32_t pk_x_be[8];
    uint32_t d_be[8];

    cv_get_order_le(curve, order);
    SCHNORR_DUMP_BUFFER("order: ", order, 32);
    /* pk = d * G */
    bnu_swap_endian_2(priv_key, d, 32);
    scalar_multiply_le(curve, d, (uint32_t *)(&pk));

    SCHNORR_DUMP_BUFFER("pk.y: ", pk.y, 32);
    /* let d = d if has_even_y(P), otherwise let d = n - d */
    if (num_256_is_odd((uint32_t *)pk.y))
    {
        SCHNORR_DEBUG("Pubkey not even\r\n");
        // return -1;
        cv_modulo_sub_le(curve, MT_N, order, d, d);
        SCHNORR_DUMP_BUFFER("d = n - d: ", d, 32);
    }

    bnu_swap_endian_2(d, d_be, 32);
    bnu_swap_endian_2(pk.x, pk_x_be, 32);
    schnorr_generate_k(d_be, pk_x_be, digest, k);
    /* k is produced by hashing, so it's in BE. swap to LE for modulo calculation. */
    bnu_swap_endian_1(k, 32);
    SCHNORR_DUMP_BUFFER("k: ", k, 32);

    /* k = k mod n*/
    sce_Sysinit_Pke();
    SCHNORR_DUMP_BUFFER("order: ", order, 32);
    curve_rmodL((uint8_t *)k, 32, (uint8_t *)order, 32, (uint8_t *)k);
    cv_get_order_le(curve, order); // FIXME: curve_rmodL broke the 'order'
    SCHNORR_DUMP_BUFFER("order: ", order, 32);
    SCHNORR_DUMP_BUFFER("k = k mod n: ", k, 32);

    if (num_256_is_zero((uint8_t *)k))
    {
        SCHNORR_DEBUG("k is zero\r\n");
        return -1;
    }

    /* R = k * G */
    scalar_multiply_le(curve, k, (uint32_t *)(&r));
    SCHNORR_DUMP_BUFFER("r.x: ", r.x, 32);
    SCHNORR_DUMP_BUFFER("r.y: ", r.y, 32);

    /* Let k = k if has_even_y(R), otherwise let k = n - k */
    if (num_256_is_odd((uint32_t *)r.y))
    {
        SCHNORR_DEBUG("R not even\r\n");
        cv_modulo_sub_le(curve, MT_N, order, k, k);
        SCHNORR_DUMP_BUFFER("k = n - k: ", k, 32);
    }

    uint32_t rx_be[8];
    bnu_swap_endian_2(r.x, rx_be, 32);
    schnorr_calculate_e(curve, (uint8_t *)rx_be, (uint8_t *)pk_x_be, (uint8_t *)digest, e);
    /* schnorr_calculate_e produced e as LE (due to bnu_swap_endian_1 inside it). */

    cv_modulo_mul_le(curve, MT_N, e, d, e);
    cv_modulo_add_le(curve, MT_N, k, e, e);

    /* Output high-level signature in BE. */
    bnu_swap_endian_2(r.x, sig, 32);
    bnu_swap_endian_2(e, sig + 8, 32);

    return 0;
}

/**
 * @brief Verify a Schnorr signature (BIP-340).
 *
 * @param curve     Elliptic curve (CT_SECP256K1).
 * @param xonly_pub X-only public key as a big-endian byte array (32 bytes).
 * @param digest    Message digest as a big-endian byte array (32 bytes).
 * @param signature Signature to verify: 64 bytes (r || s), both big-endian.
 *
 * @return 0 on success, negative value on failure.
 */
int schnorr_verify_digest(curve_type curve, const uint32_t *xonly_pub, const uint32_t *digest, const uint32_t *signature)
{
    uint32_t prime[8];
    uint32_t order[8];
    uint32_t r[8];
    uint32_t s[8];
    uint32_t e[8];
    uint32_t dig_le[8];
    pointDef pk, sg, R;

    bnu_swap_endian_2(signature, r, 32);
    bnu_swap_endian_2(signature + 8, s, 32);
    bnu_swap_endian_2(digest, dig_le, 32);

    cv_get_order_le(curve, order);
    cv_get_prime_le(curve, prime);

    if (!num_256_is_less((uint8_t *)r, (uint8_t *)prime) || !num_256_is_less((uint8_t *)s, (uint8_t *)order))
    {
        return -1;
    }

    if (!ecdsa_read_pubkey(curve, xonly_pub, 32, &pk))
    {
        return -2;
    }

    SCHNORR_DUMP_BUFFER("digest: ", digest, 32);
    SCHNORR_DUMP_BUFFER("r: ", r, 32);
    SCHNORR_DUMP_BUFFER("pkx: ", pk.x, 32);
    SCHNORR_DUMP_BUFFER("pky: ", pk.y, 32);
    schnorr_calculate_e(curve, (uint8_t *)signature, (uint8_t *)xonly_pub, (uint8_t *)digest, e);

    /* R = sG - eP;     sg = s * G */
    scalar_multiply_le(curve, s, (uint32_t *)&sg);
    SCHNORR_DUMP_BUFFER("sg.x: ", sg.x, 32);
    SCHNORR_DUMP_BUFFER("sg.y: ", sg.y, 32);

    /* e = -e */
    cv_modulo_sub_le(curve, MT_N, order, e, e);
    SCHNORR_DUMP_BUFFER("-e: ", e, 32);

    /* R = pk * e */
    point_multiply_le(curve, e, (uint32_t *)&pk, (uint32_t *)&R);
    SCHNORR_DUMP_BUFFER("pke.x: ", R.x, 32);
    SCHNORR_DUMP_BUFFER("pke.y: ", R.y, 32);

    /* R = sg + R*/
    point_add_le(curve, (uint32_t *)&sg, (uint32_t *)&R, (uint32_t *)&R);
    SCHNORR_DUMP_BUFFER("r.x: ", R.x, 32);
    SCHNORR_DUMP_BUFFER("r.y: ", R.y, 32);

    if (point_is_infinity(&R))
    {
        return -3;
    }

    if (num_256_is_odd((uint32_t *)R.y))
    {
        return -4;
    }

    if (!num_256_is_equal(r, (uint32_t *)R.x))
    {
        return -5;
    }

    return 0;
}
