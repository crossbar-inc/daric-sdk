/**
 *******************************************************************************
 * @file    main.c
 * @author  Daric Team
 * @brief   Source file for main.c module.
 *******************************************************************************
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
#include "daric_util.h"
#include "daric.h"
#include "daric_hal.h"
#include "system_daric.h"
#include "ll_api.h"
#include "bn_util.h"
#include "ecdsa.h"
#include "ecdsa.h"

/* test result */
typedef enum
{
    TEST_SUCCESS = 0,
    TEST_FAILURE = 1
} TestResult;

// sha256("Hello, world!")
uint8_t MSG_HASH[32] = {
    0x31, 0x5F, 0x5B, 0xDB, 0x76, 0xD0, 0x78, 0xC4, 0x3B, 0x8A, 0xC0, 0x06, 0x4E, 0x4A, 0x01, 0x64,
    0x61, 0x2B, 0x1F, 0xCE, 0x77, 0xC8, 0x69, 0x34, 0x5B, 0xFC, 0x94, 0xC7, 0x58, 0x94, 0xED, 0xD3,
};

uint8_t PRIVK[32] = { 0x3f, 0x41, 0x36, 0xd0, 0x8c, 0x5e, 0xd2, 0xbf, 0x3b, 0xa0, 0x48, 0xaf, 0xe6, 0xdc, 0xae, 0xba,
                      0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

uint8_t SECP256K1_PUBKEY_REF[65] = { 0x04, 0x3f, 0x06, 0x1b, 0xca, 0x59, 0x9b, 0x64, 0x6d, 0x8c, 0x00, 0x32, 0x1f, 0xaf, 0x83, 0xd0, 0x21, 0xb4, 0xb8, 0x20, 0x53, 0x67,
                                     0xe0, 0x6b, 0x27, 0xb2, 0x09, 0xe6, 0x7d, 0x63, 0x09, 0x26, 0xf4, 0xd8, 0x51, 0x58, 0xd1, 0x1c, 0xa6, 0xb0, 0x23, 0xbd, 0xf3, 0x22,
                                     0x04, 0x02, 0x5f, 0x2c, 0x95, 0x03, 0x77, 0x75, 0xdd, 0x59, 0xcf, 0x15, 0xc4, 0x91, 0xed, 0xb3, 0xb7, 0xa3, 0x96, 0x39, 0x2d };

uint8_t SECP256K1_SIGNATURE_REF[64] = { 0xf2, 0x03, 0xcc, 0xaf, 0x21, 0xe5, 0xf3, 0xab, 0x0d, 0x8f, 0x06, 0xfb, 0xff, 0x40, 0x28, 0x17, 0x84, 0x1b, 0x0b, 0x1a, 0x4c, 0xe7,
                                        0xb1, 0xb9, 0x07, 0x36, 0xf0, 0x8d, 0x0e, 0xfd, 0x79, 0x2a, 0x34, 0x4b, 0x7f, 0xfb, 0x50, 0xbe, 0xfa, 0x0b, 0x44, 0xb6, 0xb7, 0x81,
                                        0x77, 0x61, 0xcb, 0x08, 0xe5, 0x26, 0x46, 0x13, 0xc3, 0x2f, 0x99, 0xf2, 0x41, 0xbd, 0x73, 0x47, 0x8c, 0x1c, 0x7f, 0x9f };

uint8_t SECP256R1_PUBKEY_REF[65] = { 0x04, 0x52, 0x70, 0xB0, 0xB3, 0x5B, 0xED, 0xD3, 0x29, 0xF8, 0x34, 0xD8, 0xA8, 0x96, 0x66, 0x9C, 0x2B, 0x71, 0x5B, 0x6A, 0xA1, 0xBB,
                                     0x2D, 0xC4, 0x75, 0x03, 0x95, 0x5A, 0xF8, 0x90, 0xEC, 0x40, 0xAC, 0x44, 0x5F, 0xB4, 0x27, 0xA3, 0x5F, 0xF6, 0xBF, 0x5E, 0xB2, 0xC4,
                                     0x2D, 0x6B, 0xA9, 0x93, 0x7D, 0x7D, 0x0B, 0xCD, 0x8C, 0x95, 0x4A, 0x11, 0x01, 0xC5, 0x45, 0xD7, 0x64, 0x01, 0x97, 0xBA, 0xA9 };

uint8_t SECP256R1_SIGNATURE_REF[64] = { 0x75, 0x30, 0xc9, 0x6e, 0x00, 0xc6, 0x74, 0xe2, 0x04, 0xc2, 0x31, 0x14, 0x9f, 0x82, 0x56, 0xca, 0x29, 0x0d, 0xa2, 0xbe, 0xf2, 0x67,
                                        0xfe, 0x61, 0xb7, 0x8e, 0xd6, 0x75, 0x54, 0x19, 0xe1, 0x3c, 0x39, 0x55, 0x32, 0xd7, 0xcd, 0x99, 0xb5, 0x3f, 0x08, 0xe7, 0xc2, 0x96,
                                        0x52, 0xd5, 0xe7, 0x96, 0x5c, 0xf5, 0x75, 0x9a, 0x7b, 0x5c, 0x75, 0x77, 0x37, 0xe4, 0xc5, 0x76, 0x2e, 0x89, 0x1b, 0x09 };

uint8_t K_BE[32] = { 0xf1, 0xbb, 0xbb, 0xe8, 0x23, 0xc2, 0x18, 0xec, 0xf1, 0x2b, 0x62, 0x53, 0x55, 0xaa, 0xf1, 0x91,
                     0x49, 0x45, 0xdd, 0x72, 0xf3, 0x22, 0x25, 0xc5, 0xbe, 0xd3, 0x76, 0x51, 0x13, 0x28, 0x7c, 0x36 };

char *verify_msg    = "D620591D817F8BAD22609B6AAB286FDBDF201AD1C22B99AEF90B1BBC3269A176";
char *verify_sig    = "512D04F1BD6B34AF39E087E9589B521B955B84C90283813FF59712835B1EAFE0C0EAD992A01962EF7DEC6FA579B23048C15F414D41080D6759F55A10EF7017B2";
char *verify_pubkey = "BB29E0C6EB32474F2F47AE19EE9B769E6A783B30986818421B46EAD8C85832EE";

const char *get_curve_name(curve_type curve)
{
    switch (curve)
    {
        case CT_SECP256K1:
            return "SECP256K1";
        case CT_SECP256R1:
            return "SECP256R1";
        case CT_SECP384R1:
            return "SECP384R1";
        default:
            return "UNKNOWN";
    }
}

uint32_t fixed_nonce(uint32_t *nonce)
{
    memcpy(nonce, K_BE, 32);
    return 0;
}

static TestResult keygen_test(curve_type curve)
{
    int32_t  ret = TEST_SUCCESS;
    uint32_t pubkey[17];
    uint32_t private_key[8];

    printf("--- Keygen Test: %s ---\n", get_curve_name(curve));

    memcpy(private_key, PRIVK, 32);
    bnu_print_mem("input PRIVK", (uint8_t *)private_key, 32);
    ecdsa_get_pubkey(curve, private_key, pubkey);
    bnu_print_mem("output pubkey", (uint8_t *)pubkey, 65);
    if (curve == CT_SECP256K1)
    {
        ret = memcmp(pubkey, SECP256K1_PUBKEY_REF, sizeof(SECP256K1_PUBKEY_REF));
    }
    else if (curve == CT_SECP256R1)
    {
        ret = memcmp(pubkey, SECP256R1_PUBKEY_REF, sizeof(SECP256R1_PUBKEY_REF));
    }

    if (ret == 0)
    {
        printf("keygen success\n\n");
    }
    else
    {
        printf("keygen error\n");
    }

    return ret == 0 ? TEST_SUCCESS : TEST_FAILURE;
}

static TestResult sign_test(curve_type curve)
{
    int32_t  ret = TEST_SUCCESS;
    uint32_t sig[16];
    uint32_t msg_hash[8];
    uint32_t privk[8];

    printf("--- Sign Test: %s ---\n", get_curve_name(curve));

    memcpy(msg_hash, MSG_HASH, 32);
    memcpy(privk, PRIVK, 32);

    ecdsa_sign_digest(curve, privk, msg_hash, fixed_nonce, sig);

    bnu_print_mem("signature: ", (uint8_t *)sig, 64);

    if (curve == CT_SECP256K1)
    {
        ret = memcmp(sig, SECP256K1_SIGNATURE_REF, sizeof(SECP256K1_SIGNATURE_REF));
    }
    else if (curve == CT_SECP256R1)
    {
        ret = memcmp(sig, SECP256R1_SIGNATURE_REF, sizeof(SECP256R1_SIGNATURE_REF));
    }

    if (ret == 0)
    {
        printf("sign success\n\n");
    }
    else
    {
        printf("sign error\n");
    }

    return ret == 0 ? TEST_SUCCESS : TEST_FAILURE;
}

static TestResult verify_test(curve_type curve)
{
    int32_t  ret;
    uint32_t pubkey[17];
    uint32_t sig[16];
    uint32_t msg_hash[8];

    printf("--- Verify Test: %s ---\n", get_curve_name(curve));

    memcpy(msg_hash, MSG_HASH, 32);

    if (curve == CT_SECP256K1)
    {
        bnu_hex2buf_be(verify_pubkey, (uint8_t *)pubkey, 32);
        bnu_hex2buf_be(verify_msg, (uint8_t *)msg_hash, 32);
        bnu_hex2buf_be(verify_sig, (uint8_t *)sig, 64);
        ret = ecdsa_verify_ext(curve, pubkey, 32, sig, msg_hash);
    }
    else if (curve == CT_SECP256R1)
    {
        memcpy(pubkey, SECP256R1_PUBKEY_REF, 65);
        memcpy(sig, SECP256R1_SIGNATURE_REF, 64);
        ret = ecdsa_verify(curve, pubkey, sig, msg_hash);
    }

    printf("ecdsa verify result: %ld\n", ret);
    bnu_print_mem("pubkey: ", (uint8_t *)pubkey, 32);
    bnu_print_mem("sig: ", (uint8_t *)sig, 64);
    bnu_print_mem("msg_hash: ", (uint8_t *)msg_hash, 32);

    if (ret == 0)
    {
        printf("verify success\n\n");
    }
    else
    {
        printf("verify error\n");
    }

    return ret == 0 ? TEST_SUCCESS : TEST_FAILURE;
}

void run_ecdsa_test(void)
{
    curve_type curves[]   = { CT_SECP256K1, CT_SECP256R1 };
    int        num_curves = sizeof(curves) / sizeof(curves[0]);

    for (int i = 0; i < num_curves; i++)
    {
        keygen_test(curves[i]);
        sign_test(curves[i]);
        verify_test(curves[i]);
    }
}

int main(void)
{
    printf("\nHello daric ecdsa demo. Build @ %s %s.\n", __DATE__, __TIME__);

    HAL_Init();
    HAL_TickInit();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    printf("\nDelay 2 seconds.\n");
    HAL_Delay(2000);
    printf("Starting Baremetal ECDSA Demo...\n");

    run_ecdsa_test();

    printf("ECDSA Demo Finished.\n");

    while (1)
        ;

    return 0;
}
