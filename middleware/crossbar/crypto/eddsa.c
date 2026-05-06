/**
 ******************************************************************************
 * @file    eddsa.c
 * @author  SCE Team
 * @brief   EdDSA implementation file.
 *          This file provides firmware functions to manage the EdDSA
 *          cryptographic operations.
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

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "eddsa.h"
#include "bn_util.h"
#include "hash.h"
#include "ll_api.h"
#include "pke_rsa.h"
#include "pke_curve.h"

// #define EDDSA_DBG 1

#if EDDSA_DBG
#define EDDSA_DUMP_BUFFER(name, ptr, len) dump_buffer(name, ptr, len)
#define EDDSA_DEBUG(format, ...)          printf(format, ##__VA_ARGS__)

static void dump_buffer(char *name, const void *buf, uint32_t len)
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
#define EDDSA_DUMP_BUFFER(name, ptr, len)
#define EDDSA_DEBUG(format, ...)
#endif

// h =  H(R,A,M)
static void ed25519_hashram(const uint8_t *sig, const uint8_t *pub, const uint8_t *msg, uint32_t msg_len, uint8_t *h)
{
    hash_Init(HT_SHA512);
    hash_Update(sig, 32, HT_SHA512);
    hash_Update(pub, 32, HT_SHA512);
    hash_Final(msg, msg_len, HT_SHA512, h);
}

static void ed25519_publickey(const uint8_t *private_key, uint8_t *public_key)
{
    uint32_t hash_buf[16];
    pointDef pointRes;
    uint8_t *hash_p = (uint8_t *)hash_buf;

    hash(private_key, 32, HT_SHA512, hash_p);

    hash_p[0] &= 248;  // set bit0-2 to 0
    hash_p[31] &= 127; // set bit255 to 0
    hash_p[31] |= 64;  // set bit254 to 1

    scalar_multiply_le(CT_ED25519, hash_buf, (uint32_t *)&pointRes);

    pointRes.y[31] ^= ((pointRes.x[0] & 1) << 7);
    memcpy(public_key, pointRes.y, 32);
}

// Signature (R,S)
// C = HASH( private_key )
// a = C [0:31]
// A = a * G Public Key
// r = HASH( HASH(private_key)[32:64] | MSG)  mod  L
// R = r * G
// S = ( r + HASH ( R | A | MSG) a ) mod L
static void ed25519_sign(const uint8_t *msg, uint32_t msgLen, const uint8_t *private_key, const uint8_t *public_key, uint8_t *signature)
{
    uint32_t hash_buf[64 / 4], r[64 / 4], hram[64 / 4];
    uint32_t r_mod[8], hram_mod[8];
    pointDef pointRes;
    uint8_t *hash_p = (uint8_t *)hash_buf;

    hash(private_key, 32, HT_SHA512, hash_p);
    hash_p[0] &= 248;  // set bit0-2 to 0
    hash_p[31] &= 127; // set bit255 to 0
    hash_p[31] |= 64;  // set bit254 to 1

    EDDSA_DUMP_BUFFER("hash(priv_key): ", hash_p, 64);

    hash_Init(HT_SHA512);
    hash_Update(hash_p + 32, 32, HT_SHA512);
    hash_Final(msg, msgLen, HT_SHA512, (uint8_t *)r);

    EDDSA_DUMP_BUFFER("HASH( HASH(private_key)[32:64] | MSG): ", r, 64);

    bndivision_le(r, 16, ED25519_L, 8, NULL, NULL, r_mod, NULL); // r_mod = r mod L

    EDDSA_DUMP_BUFFER("r mod L: ", r_mod, 32);

    scalar_multiply_le(CT_ED25519, r_mod, (uint32_t *)&pointRes);
    pointRes.y[31] ^= ((pointRes.x[0] & 1) << 7);
    memcpy(signature, pointRes.y, 32);
    EDDSA_DUMP_BUFFER("sig[:32]: ", signature, 32);

    ed25519_hashram(signature, public_key, msg, msgLen, (uint8_t *)hram);

    EDDSA_DUMP_BUFFER("HASH ( R | A | MSG): ", hram, 64);

    bndivision_le(hram, 16, ED25519_L, 8, NULL, NULL, hram_mod, NULL); // hram_mod = hram mod L

    EDDSA_DUMP_BUFFER("hram mod L: ", hram_mod, 32);

    // s = (r_mod + hram_mod * a) % L
    cv_modulo_mul_le(CT_ED25519, MT_N, hram_mod, hash_buf, hram_mod);

    EDDSA_DUMP_BUFFER("hram * hash_buf: ", hram_mod, 32);

    cv_modulo_add_le(CT_ED25519, MT_N, r_mod, hram_mod, (uint32_t *)(signature + 32));

    EDDSA_DUMP_BUFFER("signature: ", signature, 64);
}

/* p58 = (p-5)/8 */
const uint32_t p58[8] = { 0xfffffffd, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0fffffff };
/* I = 2^((p-1)/4) mod p */
const uint32_t I[8] = { 0x4a0ea0b0, 0xc4ee1b27, 0xad2fe478, 0x2f431806, 0x3dfbd7a7, 0x2b4d0099, 0x4fc1df0b, 0x2b832480 };
/* ax^2 + y^2 = 1 + dx^2y^2 */
static uint8_t ed25519_recover_x(uint8_t *px, uint8_t *py)
{
    uint32_t sign, ty[8], y2[8], u[8], v[8], tmp0[8] = { 0 }, tmp1[8], d[8], p[8];

    sign = (py[31] & (1 << 7)) >> 7; // get the sign of x (higbit of low byte)
    py[31] &= (1 << 7) - 1;          // clear sign of x, get pure y
    memcpy(ty, py, 32);

    cv_get_d_le(CT_ED25519, d);
    cv_get_prime_le(CT_ED25519, p);

    // -x^2 + y^2 = 1 + dx^2y^2
    // x^2 + dx^2y^2 = x^2(dy^2 + 1) = y^2 - 1
    // x^2 = (y^2 - 1) / (dy^2 + 1) mod P

    /* tmp0 = 1 */
    tmp0[0] = 0x01;

    /* ty = y */
    EDDSA_DUMP_BUFFER("ty", ty, 32);

    /* y2 = y^2 */
    cv_modulo_mul_le(CT_ED25519, MT_P, ty, ty, y2);
    EDDSA_DUMP_BUFFER("y2", y2, 32);

    /* u = (y^2 - 1) mod P */
    cv_modulo_sub_le(CT_ED25519, MT_P, y2, tmp0, u);
    EDDSA_DUMP_BUFFER("U", u, 32);

    // v = (dy^2 + 1) mod P
    cv_modulo_mul_le(CT_ED25519, MT_P, y2, d, v);
    cv_modulo_add_le(CT_ED25519, MT_P, v, tmp0, v);

    EDDSA_DUMP_BUFFER("V", v, 32);

    //          (p+3)/8      3       (p-5)/8
    // x = (u/v)        = u v (u v^7)       (mod p)
    // y2 = v^2

    /* y2 = v^2 */
    cv_modulo_mul_le(CT_ED25519, MT_P, v, v, y2);
    EDDSA_DUMP_BUFFER("v^2", y2, 32);

    /* tmp0 = v^3 */
    cv_modulo_mul_le(CT_ED25519, MT_P, v, y2, tmp0);
    EDDSA_DUMP_BUFFER("v^3", tmp0, 32);

    /* tmp1 = y2 * y2 = v^4 */
    cv_modulo_mul_le(CT_ED25519, MT_P, y2, y2, tmp1);
    /* tmp1 = v^4 * v^3 = v^7 */
    cv_modulo_mul_le(CT_ED25519, MT_P, tmp0, tmp1, tmp1);

    EDDSA_DUMP_BUFFER("v^7", tmp1, 32);

    // p58 = (p-5)/8  This value is constant and not recalculated, defined as a constant 0x0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD

    // x in tmp1
    /* tmp1 = y  = u * v^7 */
    cv_modulo_mul_le(CT_ED25519, MT_P, u, tmp1, tmp1);
    EDDSA_DUMP_BUFFER("tmp1 = u * v^7", tmp1, 32);

    /* r = (u * v^7) ^ p58 */

    modulo_expo_le(tmp1, p58, p, 8, 8, 8, tmp1);
    EDDSA_DUMP_BUFFER("tmp1 = (u * v^7) ^ p58", tmp1, 32);

    /* tmp1 = v^3 * r =  v^3 * ((u * v^7) ^ p58) */
    cv_modulo_mul_le(CT_ED25519, MT_P, tmp0, tmp1, tmp1);
    EDDSA_DUMP_BUFFER("v^3 * ((u * v^7) ^ p58)", tmp1, 32);

    /* tmp1 = r = u * v^3 * r = (u * v^3) * ((u * v^7) ^ p58)*/
    cv_modulo_mul_le(CT_ED25519, MT_P, tmp1, u, tmp1);
    EDDSA_DUMP_BUFFER("r = (u * v^3) * ((u * v^7) ^ p58)", tmp1, 32);

    // tmp0 = v*x^2 mod p
    /* tmp0 = r * r */
    cv_modulo_mul_le(CT_ED25519, MT_P, tmp1, tmp1, tmp0);
    EDDSA_DUMP_BUFFER("tmp0", tmp0, 32);

    /* tmp0 = r * r * v */
    cv_modulo_mul_le(CT_ED25519, MT_P, tmp0, v, tmp0);
    EDDSA_DUMP_BUFFER("tmp0", tmp0, 32);

    // If v*x^2 = u (mod p), x is a square root.
    // If v*x^2 = -u (mod p), x * 2^((p-1)/4) is a square root.
    if (memcmp(tmp0, u, 32) != 0)
    {
        cv_modulo_add_le(CT_ED25519, MT_P, tmp0, u, tmp0);
        EDDSA_DUMP_BUFFER("r * r * v + u", tmp0, 32);
        if (bnu_is_zero(tmp0, 8))
        {
            // x = x * I; I = 2^((p-1)/4) mod p
            cv_modulo_mul_le(CT_ED25519, MT_P, tmp1, (uint32_t *)I, tmp1);
            EDDSA_DUMP_BUFFER("r * I ", tmp1, 32);
        }
        else
        {
            return 0;
        }
    }

    if ((tmp1[0] & 1) != sign)
    {
        // p-x
        cv_modulo_sub_le(CT_ED25519, MT_P, p, tmp1, tmp1);
        EDDSA_DUMP_BUFFER("p - x ", tmp1, 32);
    }

    memcpy(px, tmp1, 32);
    return 1;
}

static uint8_t ed25519_verify(const uint8_t *signature, uint8_t sigLen, const uint8_t *msg, uint32_t msgLen, const uint8_t *public_key, uint8_t pubKey_Len)
{
    uint32_t h[64 / 4], h_mod[8];
    pointDef A, R, sG, hA;

    if (sigLen != 64 || pubKey_Len != 32)
    {
        return 1;
    }

    if (signature[63] & 224)
    {
        return 2;
    }

    // decode R(x,y) = r*G
    memcpy(R.y, signature, 32);
    if (ed25519_recover_x(R.x, R.y) != 1)
    {
        return 3;
    }

    EDDSA_DUMP_BUFFER("R.x", R.x, 32);
    EDDSA_DUMP_BUFFER("R.y", R.y, 32);

    // decode A(x,y)
    memcpy(A.y, public_key, 32);
    if (ed25519_recover_x(A.x, A.y) != 1)
    {
        return 4;
    }

    EDDSA_DUMP_BUFFER("A.x", A.x, 32);
    EDDSA_DUMP_BUFFER("A.y", A.y, 32);

    // h =  H(R,A,M)
    ed25519_hashram((uint8_t *)signature, (uint8_t *)public_key, msg, msgLen, (uint8_t *)h);
    bndivision_le(h, 64 / 4, ED25519_L, 8, NULL, NULL, h_mod, NULL);
    EDDSA_DUMP_BUFFER("h mod L: ", h_mod, 32);

    EDDSA_DUMP_BUFFER("S: ", signature + 32, 32);
    /* S*G = S[0:32] * G */
    point_multiply_le(CT_ED25519, (uint32_t *)(signature + 32), ED25519_BASE_POINT, (uint32_t *)&sG);
    EDDSA_DUMP_BUFFER("S*G.x: ", &sG.x, 32);
    EDDSA_DUMP_BUFFER("S*G.y: ", &sG.y, 32);

    /* ha = h * A */
    point_multiply_le(CT_ED25519, h_mod, (uint32_t *)&A, (uint32_t *)&hA);
    EDDSA_DUMP_BUFFER("h*A.x: ", &hA.x, 32);
    EDDSA_DUMP_BUFFER("h*A.y: ", &hA.y, 32);

    /* R = R + h * A */
    point_add_le(CT_ED25519, (uint32_t *)&R, (uint32_t *)&hA, (uint32_t *)&R);
    EDDSA_DUMP_BUFFER("(R + h *A).x: ", &R.x, 32);
    EDDSA_DUMP_BUFFER("(R + h *A).y: ", &R.y, 32);

    if (memcmp(&sG, &R, 64) != 0)
    {
        return 5;
    }

    return 0;
}

/**
 * @brief follow RFC8032
 *
 * @param curve curve type, only support Ed25519 now, will support Ed448 in the future
 * @param privkey private key, length is 32 bytes, not big number, but bytes array, be used to SHA512
 * @param pubkey[out] public key, encoded point, length is 32 bytes, little endian
 */
cr_status eddsa_gen_pubkey(curve_type curve, uint32_t *privkey, uint32_t *pubkey)
{
    cr_status ret = ERR_NONE;
    switch (curve)
    {
        case CT_ED25519:
            ed25519_publickey((uint8_t *)privkey, (uint8_t *)pubkey);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }
    return ret;
}

/**
 * @brief follow RFC8032
 *
 * @param curve curve type, only support Ed25519 now, will support Ed448 in the future
 * @param privkey private key, length is 32 bytes, not big number, but bytes array, be used to SHA512
 * @param pubkey public key, encoded point, length is 32 bytes, little endian
 * @param msg bytes array used to SHA512, length is msgLen bytes
 * @param msgLen bytes number of msg
 * @param sig[out] signature, pair (R, S), length is 64 bytes
 */
cr_status eddsa_sign(curve_type curve, uint32_t *privkey, uint32_t *pubkey, uint32_t *msg, uint32_t msgLen, uint32_t *sig)
{
    cr_status ret = ERR_NONE;
    switch (curve)
    {
        case CT_ED25519:
            ed25519_sign((uint8_t *)msg, msgLen, (uint8_t *)privkey, (uint8_t *)pubkey, (uint8_t *)sig);
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}

/**
 * @brief follow RFC8032
 *
 * @param curve curve type, only support Ed25519 now, will support Ed448 in the future
 * @param pubkey public key, encoded point, length is 32 bytes, little endian
 * @param sig signature, length is 64 bytes
 * @param msg bytes array used to SHA512, length is msgLen bytes
 * @param msgLen bytes number of msg
 * @return ERR_NONE if valid; ERR_VERIFY_FAIL if invalid
 */
cr_status eddsa_verify(curve_type curve, uint32_t *pubkey, uint32_t *sig, uint32_t *msg, uint32_t msgLen)
{
    cr_status ret = ERR_NONE;
    switch (curve)
    {
        case CT_ED25519:
            uint8_t result;
            result = ed25519_verify((uint8_t *)sig, 64, (uint8_t *)msg, msgLen, (uint8_t *)pubkey, 32);
            if (result != 0)
            {
                ret = ERR_VERIFY_FAIL;
            }
            break;
        default:
            ret = ERR_PARAMETERS;
            break;
    }

    return ret;
}
