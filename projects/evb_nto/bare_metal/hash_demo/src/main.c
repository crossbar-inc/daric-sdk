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
#include "stdio.h"
#include <string.h>
#include "daric_util.h"
#include "daric.h"
#include "daric_hal.h"
#include "system_daric.h"
#include "ll_api.h"
#include "bn_util.h"
#include "sha2.h"

/* test result */
typedef enum
{
    TEST_SUCCESS = 0,
    TEST_FAILURE = 1
} TestResult;

static uint8_t hmac256_result1[] = { 0xE8, 0x49, 0x9B, 0xE4, 0xF1, 0x98, 0x0D, 0x68, 0xF1, 0x32, 0x22, 0xA4, 0x18, 0xDF, 0x5C, 0xBD,
                                     0x97, 0xD5, 0x3F, 0xDD, 0xF5, 0x90, 0xC2, 0x10, 0x8E, 0x22, 0xD4, 0x00, 0x05, 0xB7, 0x07, 0x13 };
static uint8_t hmac512_result1[] = { 0xB6, 0x04, 0x76, 0x09, 0xE7, 0x1B, 0xFB, 0x28, 0x35, 0x89, 0xD6, 0x6C, 0xC0, 0x82, 0xB4, 0x2D, 0xEE, 0xC8, 0x83, 0xCC, 0x0E, 0xB7,
                                     0x20, 0x99, 0x5F, 0x01, 0xDF, 0x13, 0xB2, 0xFB, 0x8F, 0x36, 0x93, 0xBE, 0x0C, 0xD5, 0xEF, 0x1B, 0x33, 0x20, 0xF0, 0x41, 0x34, 0xB8,
                                     0x8E, 0xC3, 0x7B, 0x06, 0xB8, 0xC1, 0x6E, 0x67, 0xE9, 0xDA, 0x8B, 0xE1, 0x47, 0x4F, 0x00, 0x23, 0xD6, 0x45, 0xFE, 0x23 };

/* -------------------------------------------------------------------------
 * Lock tracking: override the weak symbols from ll_api.c so we can verify
 * that every lock() is paired with an unlock() in both normal and error paths.
 *
 * lock_depth: should be 1 while a stepped sequence is in progress,
 *             and 0 after every completed (or aborted) operation.
 * lock_errors: incremented whenever an unlock() is called with depth already 0
 *              (double-unlock), which would indicate a bug in the ll_api code.
 * -------------------------------------------------------------------------*/
static int lock_depth  = 0;
static int lock_errors = 0;

void ll_sce_lock(void)
{
    lock_depth++;
}

void ll_sce_unlock(void)
{
    if (lock_depth <= 0)
    {
        printf("[LOCK BUG] ll_sce_unlock called when depth is already %d!\r\n", lock_depth);
        lock_errors++;
        return;
    }
    lock_depth--;
}

/* Check lock_depth equals expected value; print error and tally failure if not. */
static uint32_t lock_check(const char *label, int expected, uint32_t *fail_flag)
{
    if (lock_depth != expected)
    {
        printf("[LOCK ERR] %s: depth=%d, expected=%d\r\n", label, lock_depth, expected);
        *fail_flag |= TEST_FAILURE;
        return TEST_FAILURE;
    }
    return TEST_SUCCESS;
}

/* Reset lock state between test groups. */
static void lock_reset(void)
{
    lock_depth  = 0;
    lock_errors = 0;
}

/* -------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------*/
static void print_hex(const char *label, const uint8_t *data, uint32_t len)
{
    printf("%s: ", label);
    for (uint32_t i = 0; i < len; i++)
        printf("%02X", data[i]);
    printf("\r\n");
}

/* =========================================================================
 * Test group 1 (existing): single-shot hash() and hmac() regression
 * =========================================================================*/
static uint32_t test_singleshot_hash(void)
{
    uint32_t i;
    uint8_t  MsgBuf1[100];
    uint8_t  MsgBuf2[100];
    uint8_t  result[64];
    uint8_t  result_ref[64];
    uint32_t ret = TEST_SUCCESS;

    for (i = 0; i < 100; i++) MsgBuf1[i] = 0xa1;
    for (i = 0; i < 100; i++) MsgBuf2[i] = 0xb2;

    /* SHA256 */
    lock_reset();
    hash(MsgBuf1, sizeof(MsgBuf1), HT_SHA256, result);
    sha256_Raw(MsgBuf1, sizeof(MsgBuf1), result_ref);
    lock_check("hash(SHA256) lock balance", 0, &ret);
    if (memcmp(result, result_ref, 32) != 0)
    {
        printf("  hash(SHA256) FAIL\r\n");
        print_hex("  hw ", result, 32);
        print_hex("  ref", result_ref, 32);
        ret |= TEST_FAILURE;
    }
    else
        printf("  hash(SHA256) OK\r\n");

    /* SHA512 */
    lock_reset();
    hash(MsgBuf2, sizeof(MsgBuf2), HT_SHA512, result);
    sha512_Raw(MsgBuf2, sizeof(MsgBuf2), result_ref);
    lock_check("hash(SHA512) lock balance", 0, &ret);
    if (memcmp(result, result_ref, 64) != 0)
    {
        printf("  hash(SHA512) FAIL\r\n");
        print_hex("  hw ", result, 64);
        print_hex("  ref", result_ref, 64);
        ret |= TEST_FAILURE;
    }
    else
        printf("  hash(SHA512) OK\r\n");

    /* HMAC-SHA256 */
    uint8_t key[32], msg[32];
    for (i = 0; i < 32; i++) { key[i] = (uint8_t)i; msg[i] = (uint8_t)i; }
    lock_reset();
    hmac(key, 32, msg, 32, HT_SHA256, result);
    lock_check("hmac(SHA256) lock balance", 0, &ret);
    if (memcmp(hmac256_result1, result, 32))
    {
        printf("  hmac(SHA256) FAIL\r\n");
        ret |= TEST_FAILURE;
    }
    else
        printf("  hmac(SHA256) OK\r\n");

    /* HMAC-SHA512 */
    lock_reset();
    hmac(key, 32, msg, 32, HT_SHA512, result);
    lock_check("hmac(SHA512) lock balance", 0, &ret);
    if (memcmp(hmac512_result1, result, 64))
    {
        printf("  hmac(SHA512) FAIL\r\n");
        ret |= TEST_FAILURE;
    }
    else
        printf("  hmac(SHA512) OK\r\n");

    return ret;
}

/* =========================================================================
 * Test group 2: stepped hash_Init / hash_Update / hash_Final
 *
 * Strategy: split the message into two halves and compute via the stepped
 * API.  Compare the result against the single-shot hash() which was already
 * validated in group 1.  Also verify lock_depth transitions.
 * =========================================================================*/
static uint32_t test_stepped_hash(void)
{
    uint32_t  i;
    uint8_t   msg[100];
    uint8_t   result_step[64];
    uint8_t   result_single[64];
    cr_status cr;
    uint32_t  ret = TEST_SUCCESS;

    for (i = 0; i < 100; i++) msg[i] = (uint8_t)(i + 0x30);

    /* --- SHA256 stepped --- */
    lock_reset();
    hash(msg, sizeof(msg), HT_SHA256, result_single);   /* reference */

    lock_reset();
    cr = hash_Init(HT_SHA256);
    lock_check("SHA256 stepped: after hash_Init (expect held=1)", 1, &ret);
    if (cr != ERR_NONE) { printf("  hash_Init(SHA256) returned %d\r\n", cr); ret |= TEST_FAILURE; }

    cr = hash_Update(msg, 50, HT_SHA256);               /* first half */
    lock_check("SHA256 stepped: after hash_Update (expect held=1)", 1, &ret);
    if (cr != ERR_NONE) { printf("  hash_Update(SHA256) returned %d\r\n", cr); ret |= TEST_FAILURE; }

    cr = hash_Final(msg + 50, 50, HT_SHA256, result_step); /* second half */
    lock_check("SHA256 stepped: after hash_Final (expect released=0)", 0, &ret);
    if (cr != ERR_NONE) { printf("  hash_Final(SHA256) returned %d\r\n", cr); ret |= TEST_FAILURE; }

    if (memcmp(result_step, result_single, 32) != 0)
    {
        printf("  stepped SHA256 result FAIL\r\n");
        print_hex("  step  ", result_step, 32);
        print_hex("  single", result_single, 32);
        ret |= TEST_FAILURE;
    }
    else
        printf("  stepped SHA256 OK\r\n");

    /* --- SHA512 stepped --- */
    lock_reset();
    hash(msg, sizeof(msg), HT_SHA512, result_single);   /* reference */

    lock_reset();
    cr  = hash_Init(HT_SHA512);
    lock_check("SHA512 stepped: after hash_Init (expect held=1)", 1, &ret);
    cr |= hash_Update(msg, 50, HT_SHA512);
    lock_check("SHA512 stepped: after hash_Update (expect held=1)", 1, &ret);
    cr |= hash_Final(msg + 50, 50, HT_SHA512, result_step);
    lock_check("SHA512 stepped: after hash_Final (expect released=0)", 0, &ret);

    if (cr != ERR_NONE || memcmp(result_step, result_single, 64) != 0)
    {
        printf("  stepped SHA512 FAIL (cr=%d)\r\n", cr);
        ret |= TEST_FAILURE;
    }
    else
        printf("  stepped SHA512 OK\r\n");

    /* --- Init only + Final (no Update) --- */
    lock_reset();
    cr  = hash_Init(HT_SHA256);
    lock_check("SHA256 Init-only: after hash_Init (expect held=1)", 1, &ret);
    cr |= hash_Final(msg, sizeof(msg), HT_SHA256, result_step);
    lock_check("SHA256 Init-only: after hash_Final (expect released=0)", 0, &ret);

    lock_reset();
    hash(msg, sizeof(msg), HT_SHA256, result_single);
    if (cr != ERR_NONE || memcmp(result_step, result_single, 32) != 0)
    {
        printf("  SHA256 Init+Final (no Update) FAIL (cr=%d)\r\n", cr);
        ret |= TEST_FAILURE;
    }
    else
        printf("  SHA256 Init+Final (no Update) OK\r\n");

    return ret;
}

/* =========================================================================
 * Test group 3: stepped hmac_KeyInit / hmac_Msg_Update / hmac_Final
 * =========================================================================*/
static uint32_t test_stepped_hmac(void)
{
    uint32_t  i;
    uint8_t   key[32], msg[32];
    uint8_t   result_step[64];
    uint8_t   result_single[64];
    cr_status cr;
    uint32_t  ret = TEST_SUCCESS;

    for (i = 0; i < 32; i++) { key[i] = (uint8_t)i; msg[i] = (uint8_t)i; }

    /* --- HMAC-SHA256 stepped --- */
    lock_reset();
    hmac(key, 32, msg, 32, HT_SHA256, result_single);   /* reference */

    lock_reset();
    cr = hmac_KeyInit(key, 32, HT_SHA256);
    lock_check("HMAC256 stepped: after hmac_KeyInit (expect held=1)", 1, &ret);
    if (cr != ERR_NONE) { printf("  hmac_KeyInit(SHA256) returned %d\r\n", cr); ret |= TEST_FAILURE; }

    cr = hmac_Msg_Update(msg, 32, HT_SHA256);
    lock_check("HMAC256 stepped: after hmac_Msg_Update (expect held=1)", 1, &ret);
    if (cr != ERR_NONE) { printf("  hmac_Msg_Update(SHA256) returned %d\r\n", cr); ret |= TEST_FAILURE; }

    cr = hmac_Final(HT_SHA256, result_step);
    lock_check("HMAC256 stepped: after hmac_Final (expect released=0)", 0, &ret);
    if (cr != ERR_NONE) { printf("  hmac_Final(SHA256) returned %d\r\n", cr); ret |= TEST_FAILURE; }

    if (memcmp(result_step, result_single, 32) != 0)
    {
        printf("  stepped HMAC-SHA256 result FAIL\r\n");
        print_hex("  step  ", result_step, 32);
        print_hex("  single", result_single, 32);
        ret |= TEST_FAILURE;
    }
    else
        printf("  stepped HMAC-SHA256 OK\r\n");

    /* --- HMAC-SHA512 stepped --- */
    lock_reset();
    hmac(key, 32, msg, 32, HT_SHA512, result_single);

    lock_reset();
    cr  = hmac_KeyInit(key, 32, HT_SHA512);
    lock_check("HMAC512 stepped: after hmac_KeyInit (expect held=1)", 1, &ret);
    cr |= hmac_Msg_Update(msg, 32, HT_SHA512);
    lock_check("HMAC512 stepped: after hmac_Msg_Update (expect held=1)", 1, &ret);
    cr |= hmac_Final(HT_SHA512, result_step);
    lock_check("HMAC512 stepped: after hmac_Final (expect released=0)", 0, &ret);

    if (cr != ERR_NONE || memcmp(result_step, result_single, 64) != 0)
    {
        printf("  stepped HMAC-SHA512 FAIL (cr=%d)\r\n", cr);
        ret |= TEST_FAILURE;
    }
    else
        printf("  stepped HMAC-SHA512 OK\r\n");

    /* --- KeyInit + Final (no Msg_Update) --- */
    lock_reset();
    hmac(key, 32, msg, 32, HT_SHA256, result_single);

    lock_reset();
    cr  = hmac_KeyInit(key, 32, HT_SHA256);
    lock_check("HMAC256 KeyInit+Final: after hmac_KeyInit (expect held=1)", 1, &ret);
    cr |= hmac_Final(HT_SHA256, result_step);
    lock_check("HMAC256 KeyInit+Final: after hmac_Final (expect released=0)", 0, &ret);

    /*
     * Note: KeyInit+Final without Msg_Update feeds an empty message body,
     * which differs from hmac(key, msg). We only check lock balance here,
     * not the digest value.
     */
    if (cr != ERR_NONE)
    {
        printf("  HMAC-SHA256 KeyInit+Final FAIL (cr=%d)\r\n", cr);
        ret |= TEST_FAILURE;
    }
    else
        printf("  HMAC-SHA256 KeyInit+Final (lock balance) OK\r\n");

    return ret;
}

/* =========================================================================
 * Test group 4: error paths must release the lock
 *
 * Verifies that when hash_Update or hash_Final is called with a mismatched
 * hash_type (simulating a caller bug, or a cross-call interleave), the
 * ERR_STEP path correctly calls ll_sce_unlock() so that lock_depth returns
 * to 0 and the hardware is not left permanently locked.
 * =========================================================================*/
static uint32_t test_error_paths(void)
{
    uint8_t   dummy[32];
    cr_status cr;
    uint32_t  ret = TEST_SUCCESS;

    /* --- Case A: hash_Init(SHA256) then hash_Final with wrong type --- */
    lock_reset();
    cr = hash_Init(HT_SHA256);
    lock_check("Error path A: after hash_Init(SHA256) (expect held=1)", 1, &ret);
    if (cr != ERR_NONE) { printf("  hash_Init unexpected error %d\r\n", cr); ret |= TEST_FAILURE; }

    /* Simulate a different caller trying to call hash_Final with SHA512 */
    cr = hash_Final(dummy, sizeof(dummy), HT_SHA512, dummy);
    if (cr != ERR_STEP)
    {
        printf("  Error path A: expected ERR_STEP, got %d\r\n", cr);
        ret |= TEST_FAILURE;
    }
    lock_check("Error path A: after hash_Final(wrong type) (expect released=0)", 0, &ret);
    printf("  Error path A (hash_Final type mismatch releases lock): %s\r\n",
           (ret == TEST_SUCCESS) ? "OK" : "FAIL");

    /* --- Case B: hash_Init(SHA256) then hash_Update with wrong type --- */
    lock_reset();
    cr = hash_Init(HT_SHA256);
    lock_check("Error path B: after hash_Init(SHA256) (expect held=1)", 1, &ret);

    cr = hash_Update(dummy, sizeof(dummy), HT_SHA512);
    if (cr != ERR_STEP)
    {
        printf("  Error path B: expected ERR_STEP, got %d\r\n", cr);
        ret |= TEST_FAILURE;
    }
    lock_check("Error path B: after hash_Update(wrong type) (expect released=0)", 0, &ret);
    printf("  Error path B (hash_Update type mismatch releases lock): %s\r\n",
           (ret == TEST_SUCCESS) ? "OK" : "FAIL");

    /* --- Case C: hmac_KeyInit(SHA256) then hmac_Final with wrong type --- */
    uint8_t key[32];
    memset(key, 0xAB, sizeof(key));

    lock_reset();
    cr = hmac_KeyInit(key, 32, HT_SHA256);
    lock_check("Error path C: after hmac_KeyInit(SHA256) (expect held=1)", 1, &ret);

    cr = hmac_Final(HT_SHA512, dummy);
    if (cr != ERR_STEP)
    {
        printf("  Error path C: expected ERR_STEP, got %d\r\n", cr);
        ret |= TEST_FAILURE;
    }
    lock_check("Error path C: after hmac_Final(wrong type) (expect released=0)", 0, &ret);
    printf("  Error path C (hmac_Final type mismatch releases lock): %s\r\n",
           (ret == TEST_SUCCESS) ? "OK" : "FAIL");

    /* --- Case D: hmac_KeyInit(SHA256) then hmac_Msg_Update with wrong type --- */
    lock_reset();
    cr = hmac_KeyInit(key, 32, HT_SHA256);
    lock_check("Error path D: after hmac_KeyInit(SHA256) (expect held=1)", 1, &ret);

    cr = hmac_Msg_Update(dummy, sizeof(dummy), HT_SHA512);
    if (cr != ERR_STEP)
    {
        printf("  Error path D: expected ERR_STEP, got %d\r\n", cr);
        ret |= TEST_FAILURE;
    }
    lock_check("Error path D: after hmac_Msg_Update(wrong type) (expect released=0)", 0, &ret);
    printf("  Error path D (hmac_Msg_Update type mismatch releases lock): %s\r\n",
           (ret == TEST_SUCCESS) ? "OK" : "FAIL");

    return ret;
}

/* =========================================================================
 * Main test runner
 * =========================================================================*/
void run_hash_test(void)
{
    uint32_t total = TEST_SUCCESS;
    uint32_t r;

    printf("\n--- Group 1: single-shot hash/hmac (regression) ---\r\n");
    r = test_singleshot_hash();
    total |= r;
    printf("Group 1: %s\r\n", r == TEST_SUCCESS ? "PASS" : "FAIL");

    printf("\n--- Group 2: stepped hash_Init/Update/Final ---\r\n");
    r = test_stepped_hash();
    total |= r;
    printf("Group 2: %s\r\n", r == TEST_SUCCESS ? "PASS" : "FAIL");

    printf("\n--- Group 3: stepped hmac_KeyInit/Msg_Update/Final ---\r\n");
    r = test_stepped_hmac();
    total |= r;
    printf("Group 3: %s\r\n", r == TEST_SUCCESS ? "PASS" : "FAIL");

    printf("\n--- Group 4: error paths release the lock ---\r\n");
    r = test_error_paths();
    total |= r;
    printf("Group 4: %s\r\n", r == TEST_SUCCESS ? "PASS" : "FAIL");

    if (lock_errors > 0)
    {
        printf("\n[WARNING] %d double-unlock event(s) detected!\r\n", lock_errors);
        total |= TEST_FAILURE;
    }

    printf("\n========================================\r\n");
    printf("Overall: %s\r\n", total == TEST_SUCCESS ? "ALL PASS" : "SOME FAIL");
    printf("========================================\r\n");
}

int main(void)
{
    printf("\nHello daric hash demo. Build @ %s %s.\n", __DATE__, __TIME__);

    HAL_Init();
    HAL_TickInit();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    printf("\nDelay 2 seconds.\n");
    HAL_Delay(2000);
    printf("Starting Baremetal Hash Demo...\n");
    run_hash_test();
    while (1)
        ;

    return 0;
}
