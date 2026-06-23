/**
 ******************************************************************************
 * @file    ecdsa.c
 * @author  SCE Team
 * @brief   ECDSA implementation file.
 *          This file provides firmware functions to manage the ECDSA
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

#include "ecdsa.h"
// #ifdef DRV_SCE_ENABLED
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "bn_util.h"
#include "rfc6979.h"

// #define ECDSA_DBG 1

#if ECDSA_DBG
#define ECDSA_DUMP_BUFFER(name, ptr, len) dump_buffer(name, ptr, len)
#define ECDSA_DEBUG(format, ...)          printf(format, ##__VA_ARGS__)

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
#define ECDSA_DUMP_BUFFER(name, ptr, len)
#define ECDSA_DEBUG(format, ...)
#endif

// Returns x < y. not time consistent
bool num_256_is_less(const uint8_t *x, const uint8_t *y)
{
    return bnu_cmp_le((uint32_t *)x, 8, (uint32_t *)y, 8) < 0;
}

// Returns x == y
bool num_256_is_equal(const uint32_t *x, const uint32_t *y)
{
    return bnu_cmp_le((uint32_t *)x, 8, (uint32_t *)y, 8) == 0;
}

// Returns x == 0
bool num_256_is_zero(const uint8_t *x)
{
    return bnu_is_zero((uint32_t *)x, 8);
}

bool num_256_is_odd(const uint32_t *x)
{
    uint8_t *ptr = (uint8_t *)x;
    return ptr[0] & 1;
}

/**
 * @brief num = num + 1
 *
 * @param num 256bit little endian number
 */
void num_256_add_one(uint8_t *num)
{
    for (int i = 0; i < 32; i++)
    {
        num[i]++;
        if (num[i] != 0)
            break;
    }
}

bool point_is_infinity(const pointDef *p)
{
    return num_256_is_zero((uint8_t *)&(p->x)) && num_256_is_zero((uint8_t *)&(p->y));
}

/**
 * @brief fail if seckey == 0 or seckey >= order
 *
 * @param curve curve type
 * @param seckey private key
 * @return true
 * @return false
 */
bool ecdsa_seckey_verify(curve_type curve, const uint32_t *seckey)
{
    uint32_t order[8];

    cv_get_order_be(curve, order);
    if (num_256_is_zero((uint8_t *)seckey) || !num_256_is_less((uint8_t *)seckey, (uint8_t *)order))
    {
        return false;
    }
    return true;
}

/**
 * @brief
 *
 * @param curve curve type
 * @param seckey private key
 * @param pubkey[out] 65 bytes format: 0x04 + x(32 bytes) + y(32 bytes)
 */
void ecdsa_get_pubkey(curve_type curve, uint32_t *seckey, uint32_t *pubkey)
{
    uint8_t *pubkey_ptr = (uint8_t *)pubkey;
    uint32_t priv_key[8];
    uint32_t ec_p[16];

    pubkey_ptr[0] = 0x04;
    // big endian to little endian
    bnu_swap_endian_2(seckey, priv_key, 32);
    scalar_multiply_le(curve, priv_key, ec_p);
    bnu_swap_endian_2(ec_p, pubkey_ptr + 1, 32);
    bnu_swap_endian_2(ec_p + 8, pubkey_ptr + 33, 32);
}

/**
 * @brief x-only public key
 *
 * @param curve curve type
 * @param seckey private key
 * @param pubkey[out] 32 bytes public key
 */
void ecdsa_get_bip340_pubkey(curve_type curve, uint32_t *seckey, uint32_t *pubkey)
{
    uint32_t pub[16];
    uint32_t priv_key[8];

    bnu_swap_endian_2(seckey, priv_key, 32);
    scalar_multiply_le(curve, priv_key, pub);
    bnu_swap_endian_2(pub, pubkey, 32);
}

/**
 * @brief Sign a message digest using ECDSA.
 *
 * @param curve    Elliptic curve to use (CT_SECP256K1, CT_SECP256R1, ...).
 * @param priv_key Private key as a big-endian byte array (32 bytes for
 *                 256-bit curves).
 * @param digest   Message hash as a big-endian byte array (32 bytes).
 * @param nonce_fn Nonce (k) generation callback, called as @c nonce_fn(k).
 *                 Pass @c NULL to use the built-in RFC 6979 deterministic
 *                 nonce generator.
 *                 The callback may return any 32-byte value; nonces that are
 *                 zero or >= the curve order are silently skipped and the
 *                 callback is invoked again.
 * @param sig      [out] Signature output: r || s, each 32 bytes, big-endian.
 * @return 0 on success, -1 if the nonce function returns an error.
 *
 * @note **Low-s normalisation**: if the computed @p s value satisfies
 *       @c s > n/2 (where @p n is the curve order), the function
 *       substitutes @c s = n - s before writing the output.  This produces
 *       the canonical "low-s" form required by BIP-66 and the SEC standard.
 *       Verifiers that accept only low-s signatures (e.g. Bitcoin) will
 *       accept the output directly; verifiers that accept both forms will
 *       also accept it.  When comparing signatures byte-for-byte against an
 *       external reference, ensure the reference is also normalised to low-s.
 */
int32_t ecdsa_sign_digest(curve_type curve, const uint32_t *priv_key, const uint32_t *digest, nonce_function nonce_fn, uint32_t *sig)
{
    // uint8_t r[64];
    pointDef r;
    uint32_t s[8];
    uint32_t k_inv[8];
    uint32_t order[8];
    uint32_t order_half[8];
    uint32_t nonce[8];
    uint32_t nonce_le[8];
    uint32_t priv[8], dig[8];
    rfc6979_context rfc6979_ctx;

    cv_get_order_le(curve, order);
    cv_get_order_half_le(curve, order_half);
    bnu_swap_endian_2(priv_key, priv, 32);
    bnu_swap_endian_2(digest, dig, 32);

    if (nonce_fn == NULL)
    {
        rfc6979_hmac_init(&rfc6979_ctx, digest, priv_key);
    }

    for (int i = 0; i < 10000; i++)
    {
        if (nonce_fn != NULL)
        {
            if (0 != nonce_fn(nonce))
            {
                return -1;
            }
        }
        else
        {
            rfc6979_hmac_generate(&rfc6979_ctx, nonce);
        }
        bnu_swap_endian_1(nonce, 32);
        memcpy(nonce_le, nonce, 32);

        if (num_256_is_zero((uint8_t *)nonce_le) || !num_256_is_less((uint8_t *)nonce_le, (uint8_t *)order))
        {
            continue;
        }

        /* R */
        scalar_multiply_le(curve, (uint32_t *)nonce, (uint32_t *)&r);
        ECDSA_DUMP_BUFFER("r.x: ", &r.x, 32);
        ECDSA_DUMP_BUFFER("r.y: ", &r.y, 32);

        // TODO: need do modulo(r.x) here?
        if (num_256_is_zero((uint8_t *)&(r.x)))
        {
            continue;
        }

        ECDSA_DUMP_BUFFER("priv_key: ", (uint32_t *)priv, 32);
        /* r.x * priv */
        cv_modulo_mul_le(curve, MT_N, (uint32_t *)&r.x, (uint32_t *)priv, s);
        ECDSA_DUMP_BUFFER("r.x * priv: ", s, 32);
        /* r.x * priv + message */
        ECDSA_DUMP_BUFFER("message digest: ", (uint8_t *)dig, 32);
        cv_modulo_add_le(curve, MT_N, s, (uint32_t *)dig, s);
        ECDSA_DUMP_BUFFER("r.x * priv + message: ", s, 32);

        // bnu_swap_endian_1((uint8_t *)nonce_le, 32);
        // bnu_swap_endian_1((uint8_t *)order, 32);
        /* 1/k */
        modulo_inverse_le(nonce_le, order, 8, 8, k_inv);
        // bnu_swap_endian_1((uint8_t *)k_inv, 32);
        ECDSA_DUMP_BUFFER("1/k: ", k_inv, 32);

        /* (r.x * priv + message) * 1/k */
        cv_modulo_mul_le(curve, MT_N, s, k_inv, s);
        ECDSA_DUMP_BUFFER("s: ", s, 32);

        if (num_256_is_zero((uint8_t *)s))
        {
            continue;
        }

        if (num_256_is_less((uint8_t *)order_half, (uint8_t *)s))
        {
            /* s = order - s */
            cv_get_order_le(curve, order);
            cv_modulo_sub_le(curve, MT_N, order, s, s);
            ECDSA_DUMP_BUFFER("s = -s: ", s, 32);
        }

        bnu_swap_endian_2(&r.x, sig, 32);
        bnu_swap_endian_2(s, sig + 8, 32);
        // memcpy(sig, &r.x, 32);
        // memcpy(sig + 8, s, 32);

        return 0;
    }
    return -1;
}

bool ecdsa_validate_pubkey(curve_type curve, pointDef *pub)
{
    uint32_t prime[8];
    uint32_t y2[8];
    uint32_t x3_ax_b[8];
    uint32_t a[8] = { 0 };
    uint32_t b[8] = { 0 };

    if (point_is_infinity(pub))
    {
        return false;
    }

    cv_get_prime_le(curve, prime);
    cv_get_a_le(curve, a);
    cv_get_b_le(curve, b);

    if (!num_256_is_less((uint8_t *)&(pub->x), (uint8_t *)prime) || !num_256_is_less((uint8_t *)&(pub->y), (uint8_t *)prime))
    {
        return false;
    }

    memcpy(y2, (uint8_t *)&(pub->y), 32);
    memcpy(x3_ax_b, (uint8_t *)&(pub->x), 32);

    /* y^2 */
    cv_modulo_mul_le(curve, MT_P, y2, y2, y2);
    ECDSA_DUMP_BUFFER("y^2: ", y2, 32);

    /* x^3 + ax + b */
    cv_modulo_mul_le(curve, MT_P, x3_ax_b, x3_ax_b, x3_ax_b); // x^2
    ECDSA_DUMP_BUFFER("x^2: ", x3_ax_b, 32);

    cv_modulo_add_le(curve, MT_P, x3_ax_b, a, x3_ax_b); // x^2 + a
    ECDSA_DUMP_BUFFER("x^2 + a: ", x3_ax_b, 32);

    cv_modulo_mul_le(curve, MT_P, x3_ax_b, (uint32_t *)&(pub->x), x3_ax_b); // x^3 + ax
    ECDSA_DUMP_BUFFER("x^3 + ax: ", x3_ax_b, 32);

    cv_modulo_add_le(curve, MT_P, x3_ax_b, b, x3_ax_b); // x^3 + ax + b
    ECDSA_DUMP_BUFFER("x^3 + ax + b: ", x3_ax_b, 32);

    if (num_256_is_equal(y2, x3_ax_b))
    {
        return true;
    }

    return false;
}

/**
 * @brief calculate p.y from p.x. y^2 = x^3 + ax + b
 *
 * @param curve curve type
 * @param p ec point
 */
static void ec_point_calculate_y(curve_type curve, pointDef *p)
{
    uint32_t  prime[8];
    uint32_t  e[8];
    uint32_t  a[8];
    uint32_t  b[8];
    uint32_t *y = (uint32_t *)p->y;

    cv_get_prime_le(curve, prime);
    cv_get_a_le(curve, a);
    cv_get_b_le(curve, b);

    memcpy(y, (uint8_t *)&(p->x), 32); // x
    ECDSA_DUMP_BUFFER("x: ", y, 32);

    /* x^3 + ax + b */
    cv_modulo_mul_le(curve, MT_P, y, y, y); // x^2
    ECDSA_DUMP_BUFFER("x^2: ", y, 32);

    cv_modulo_add_le(curve, MT_P, y, a, y); // x^2 + a
    ECDSA_DUMP_BUFFER("x^2 + a: ", y, 32);

    cv_modulo_mul_le(curve, MT_P, y, (uint32_t *)&(p->x), y); // x^3 + ax
    ECDSA_DUMP_BUFFER("x^3 + ax: ", y, 32);

    cv_modulo_add_le(curve, MT_P, y, b, y); // x^3 + ax + b
    ECDSA_DUMP_BUFFER("x^3 + ax + b: ", y, 32);

    /* y = sqrt(y) % prime. if prime % 4 == 3, y = y**((prime+1)/4) % prime
    see
    http://en.wikipedia.org/wiki/Quadratic_residue#Prime_or_prime_power_modulus
    */
    memcpy(e, prime, 32);          // e = prime
    num_256_add_one((uint8_t *)e); // e = prime + 1
    ECDSA_DUMP_BUFFER("prime + 1: ", e, 32);

    /* e = (prime + 1) / 4 */
    memcpy(a, e, 32);
    bnu_rightshift_le(a, 8, 2, e);
    ECDSA_DUMP_BUFFER("(prime + 1) / 4: ", e, 32);

    modulo_expo_le(y, e, prime, 8, 8, 8, y);
    ECDSA_DUMP_BUFFER("y: ", y, 32);
}

/**
 * @brief Get point coordinates from public key. Support 65-bytes, 33 bytes, 32
 * bytes public keys. Output little endian ec point.
 *
 * @param curve curve type
 * @param pub_key public key. big endian
 * @param length key length
 * @param pub[out] point output. little endian
 * @return true
 * @return false
 */
bool ecdsa_read_pubkey(curve_type curve, const uint32_t *pub_key, uint32_t length, pointDef *pub)
{
    uint8_t *pubkey_ptr = (uint8_t *)pub_key;
    uint32_t prime[8];

    if (length == 65)
    {
        memcpy(&(pub->x), (uint8_t *)pubkey_ptr + 1, 32);
        bnu_swap_endian_1(&(pub->x), 32);
        memcpy(&(pub->y), (uint8_t *)pubkey_ptr + 33, 32);
        bnu_swap_endian_1(&(pub->y), 32);
        return ecdsa_validate_pubkey(curve, pub);
    }
    else if (length == 33)
    {
        memcpy(&(pub->x), (uint8_t *)pubkey_ptr + 1, 32);
        bnu_swap_endian_1(&(pub->x), 32);
        ec_point_calculate_y(curve, pub);
        if ((pubkey_ptr[0] & 0x1) != (pub->y[0] & 0x1))
        {
            cv_get_prime_le(curve, prime);
            cv_modulo_sub_le(curve, MT_P, prime, (uint32_t *)&pub->y, (uint32_t *)&pub->y);
            ECDSA_DUMP_BUFFER("y = p - y: ", &pub->y, 32);
        }
        return ecdsa_validate_pubkey(curve, pub);
    }
    else if (length == 32)
    {
        memcpy(&(pub->x), (uint8_t *)pubkey_ptr, 32);
        bnu_swap_endian_1(&(pub->x), 32);
        ec_point_calculate_y(curve, pub);
        if (num_256_is_odd((uint32_t *)&pub->y))
        {
            ECDSA_DEBUG("Y not even\r\n");
            cv_get_prime_le(curve, prime);
            cv_modulo_sub_le(curve, MT_P, prime, (uint32_t *)&pub->y, (uint32_t *)&pub->y);
            ECDSA_DUMP_BUFFER("y = prime - y: ", &pub->y, 32);
        }
        return ecdsa_validate_pubkey(curve, pub);
    }

    return false;
}

/**
 * @brief
 *
 * @param curve curve type
 * @param pub_key 65 bytes pubkey, format: 0x04 + x(32 bytes) + y(32 bytes)
 * @param sig signature
 * @param digest hash of message
 * @return 0 if valid, otherwise invalid
 */
int32_t ecdsa_verify(curve_type curve, const uint32_t *pub_key, const uint32_t *sig, const uint32_t *digest)
{
    return ecdsa_verify_ext(curve, pub_key, 65, sig, digest);
}

/**
 * @brief ecdsa verify. Support 65-bytes, 33 bytes, 32 bytes public keys.
 *
 * @param curve curve type
 * @param pub_key public key
 * @param sig signature
 * @param pub_key_length Support 65-bytes, 33 bytes, 32 bytes public keys.
 * @param digest
 * @return int32_t
 */
int32_t ecdsa_verify_ext(curve_type curve, const uint32_t *pub_key, uint32_t pub_key_length, const uint32_t *sig, const uint32_t *digest)
{

    pointDef pub = { 0 }, res = { 0 };
    uint32_t r[8]  = { 0 };
    uint32_t s[8]  = { 0 };
    uint32_t z[8]  = { 0 };
    uint32_t u1[8] = { 0 };
    uint32_t u2[8] = { 0 };
    uint32_t order[8];

    if (!ecdsa_read_pubkey(curve, pub_key, pub_key_length, &pub))
    {
        return -1;
    }

    cv_get_order_le(curve, order);

    memcpy(r, sig, 32);
    memcpy(s, sig + 8, 32);
    memcpy(z, digest, 32);

    bnu_swap_endian_1(r, 32);
    bnu_swap_endian_1(s, 32);
    bnu_swap_endian_1(z, 32);
    if (num_256_is_zero((uint8_t *)s) || num_256_is_zero((uint8_t *)r) || num_256_is_zero((uint8_t *)z) || !num_256_is_less((uint8_t *)r, (uint8_t *)order)
        || !num_256_is_less((uint8_t *)s, (uint8_t *)order))
    {
        return -2;
    }

    /* s = s^-1 */
    modulo_inverse_le(s, order, 8, 8, s);
    ECDSA_DUMP_BUFFER("1/s: ", s, 32);

    /* u1 = z * s^-1 mod n */
    cv_modulo_mul_le(curve, MT_N, z, s, u1);
    ECDSA_DUMP_BUFFER("u1: ", u1, 32);

    /* u2 = r * s^-1 mod n */
    cv_modulo_mul_le(curve, MT_N, r, s, u2);
    ECDSA_DUMP_BUFFER("u2: ", u2, 32);

    /* res = u1 * G */
    scalar_multiply_le(curve, u1, (uint32_t *)&res);
    ECDSA_DUMP_BUFFER("res.x: ", &res.x, 32);
    ECDSA_DUMP_BUFFER("res.y: ", &res.y, 32);

    /* pub = s * pub [ = u2 * Q]*/
    point_multiply_le(curve, u2, (uint32_t *)&pub, (uint32_t *)&pub);
    ECDSA_DUMP_BUFFER("pub.x: ", &pub.x, 32);
    ECDSA_DUMP_BUFFER("pub.y: ", &pub.y, 32);

    point_add_le(curve, (uint32_t *)&pub, (uint32_t *)&res, (uint32_t *)&res);
    ECDSA_DUMP_BUFFER("res.x: ", &res.x, 32);
    ECDSA_DUMP_BUFFER("res.y: ", &res.y, 32);

    if (point_is_infinity(&res))
    {
        return -4;
    }

    // TODO: need do modulo(res.x) here?

    if (0 != bnu_cmp_le((uint32_t *)&res.x, 8, r, 8))
    {
        return -5;
    }

    return 0;
}

/**
 * @brief calculate shared secret. shared_secret = priv_key * pub_key
 *
 * @param curve curve type
 * @param priv_key private key 1
 * @param pub_key public key 2
 * @param pub_key_length length of pub_key. Support 65-bytes, 33 bytes, 32 bytes public keys.
 * @param shared_secret[out] 65 bytes shared secret. format: 0x04 + x(32 bytes) + y(32 bytes)
 * @return int32_t
 */
int32_t ecdh_multiply(curve_type curve, const uint32_t *priv_key, const uint32_t *pub_key, uint32_t pub_key_length, uint32_t *shared_secret)
{
    pointDef pub    = { 0 };
    uint8_t *sc_ptr = (uint8_t *)shared_secret;
    sc_ptr[0]       = 0x04;
    uint32_t priv[8], ec_p[16];

    if (!ecdsa_read_pubkey(curve, pub_key, pub_key_length, &pub))
    {
        return 1;
    }

    bnu_swap_endian_2(priv_key, priv, 32);
    point_multiply_le(curve, priv, (uint32_t *)&pub, ec_p);
    bnu_swap_endian_2(ec_p, sc_ptr + 1, 32);
    bnu_swap_endian_2(ec_p + 8, sc_ptr + 33, 32);

    return 0;
}
// #endif
