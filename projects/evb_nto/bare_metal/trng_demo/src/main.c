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
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "daric_util.h"
#include "daric.h"
#include "daric_hal.h"
#include "system_daric.h"
#include "ll_api.h"
#include "bn_util.h"

/* Per ll_api.h doc: length of words, must < 1024/4 = 256 */
#define RNG_MAX_WORDS 256u

/* Set to 1 to print raw random output during each test, 0 to suppress */
#define TRNG_VERBOSE_PRINT 1

#if TRNG_VERBOSE_PRINT
#define PRINT_BUF(buf, len)                                                   \
    do {                                                                      \
        for (uint32_t _i = 0; _i < (len); _i++) {                            \
            printf("%08" PRIx32 " ", (buf)[_i]);                              \
            if ((_i + 1) % 8 == 0) printf("\r\n");                           \
        }                                                                     \
        if ((len) % 8 != 0) printf("\r\n");                                   \
    } while (0)
#else
#define PRINT_BUF(buf, len) ((void)0)
#endif

static uint32_t g_pass_count = 0;
static uint32_t g_fail_count = 0;

#define CHECK(cond, name)                                    \
    do {                                                     \
        if (cond) {                                          \
            printf("[ PASS ] %s\r\n", name);                \
            g_pass_count++;                                  \
        } else {                                             \
            printf("[ FAIL ] %s\r\n", name);                \
            g_fail_count++;                                  \
        }                                                    \
    } while (0)

static bool buf_all_zero(const uint32_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        if (buf[i] != 0)
            return false;
    return true;
}

static bool buf_equal(const uint32_t *a, const uint32_t *b, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        if (a[i] != b[i])
            return false;
    return true;
}

/* ------------------------------------------------------------------
 * Test 1: rng_buffer without prior rng_init (lazy init path).
 *         Must run FIRST before any rng_init call.
 * ------------------------------------------------------------------ */
static void test_lazy_init(void)
{
    uint32_t buf[8] = { 0 };

    printf("\r\n--- Test 1: rng_buffer without rng_init (lazy init) ---\r\n");
    cr_status ret = rng_buffer(buf, 8);
    CHECK(ret == ERR_NONE, "lazy init: rng_buffer succeeds without rng_init");
    CHECK(!buf_all_zero(buf, 8), "lazy init: output is non-zero");
    PRINT_BUF(buf, 8);
}

/* ------------------------------------------------------------------
 * Test 2: invalid size=0 must return error
 * ------------------------------------------------------------------ */
static void test_invalid_size_zero(void)
{
    uint32_t buf[8];

    printf("\r\n--- Test 2: invalid size=0 ---\r\n");
    memset(buf, 0xAA, sizeof(buf));
    cr_status ret = rng_buffer(buf, 0);
    CHECK(ret != ERR_NONE, "size=0 returns error");

    bool untouched = true;
    for (uint32_t i = 0; i < 8; i++)
        if (buf[i] != 0xAAAAAAAAu)
            untouched = false;
    CHECK(untouched, "size=0 does not modify buffer");
}

/* ------------------------------------------------------------------
 * Test 3: invalid size > RNG_MAX_WORDS must return error
 * ------------------------------------------------------------------ */
static void test_invalid_size_overflow(void)
{
    static uint32_t buf[RNG_MAX_WORDS + 4];

    printf("\r\n--- Test 3: invalid size > %u words ---\r\n", RNG_MAX_WORDS);
    memset(buf, 0xBB, sizeof(buf));
    cr_status ret = rng_buffer(buf, RNG_MAX_WORDS + 1);
    CHECK(ret != ERR_NONE, "size=MAX+1 returns error");
}

/* ------------------------------------------------------------------
 * Test 4: minimum valid size = 1 word
 * ------------------------------------------------------------------ */
static void test_min_size(void)
{
    uint32_t word = 0;

    printf("\r\n--- Test 4: minimum size=1 ---\r\n");
    rng_init();
    cr_status ret = rng_buffer(&word, 1);
    CHECK(ret == ERR_NONE, "size=1 returns ERR_NONE");
    CHECK(word != 0, "size=1 output is non-zero");
    PRINT_BUF(&word, 1);
}

/* ------------------------------------------------------------------
 * Test 5: maximum valid size = RNG_MAX_WORDS (256 words = 1024 bytes)
 * ------------------------------------------------------------------ */
static void test_max_size(void)
{
    static uint32_t buf[RNG_MAX_WORDS];

    printf("\r\n--- Test 5: maximum size=%u words ---\r\n", RNG_MAX_WORDS);
    rng_init();
    memset(buf, 0, sizeof(buf));
    cr_status ret = rng_buffer(buf, RNG_MAX_WORDS);
    CHECK(ret == ERR_NONE, "size=MAX returns ERR_NONE");
    CHECK(!buf_all_zero(buf, RNG_MAX_WORDS), "size=MAX output is non-zero");
    PRINT_BUF(buf, RNG_MAX_WORDS);
}

/* ------------------------------------------------------------------
 * Test 6: FIFO exhaustion — total words requested >> RNG_MAX_WORDS,
 *         forcing multiple hardware restarts transparently.
 *         40 rounds x 16 words = 640 words, crosses 256-word boundary
 *         twice.
 * ------------------------------------------------------------------ */
static void test_fifo_exhaustion(void)
{
#define EXHAUST_CHUNK  16u
#define EXHAUST_ROUNDS 40u
    static uint32_t buf[EXHAUST_CHUNK];
    uint32_t        fail_count = 0;

    printf("\r\n--- Test 6: FIFO exhaustion (%u rounds x %u words = %u total) ---\r\n",
           EXHAUST_ROUNDS, EXHAUST_CHUNK, EXHAUST_ROUNDS * EXHAUST_CHUNK);
    rng_init();

    for (uint32_t i = 0; i < EXHAUST_ROUNDS; i++)
    {
        memset(buf, 0, sizeof(buf));
        cr_status ret = rng_buffer(buf, EXHAUST_CHUNK);
        if (ret != ERR_NONE)
        {
            printf("  Round %" PRIu32 ": rng_buffer error\r\n", i);
            fail_count++;
        }
        else if (buf_all_zero(buf, EXHAUST_CHUNK))
        {
            printf("  Round %" PRIu32 ": all-zero output!\r\n", i);
            fail_count++;
        }
        else
        {
            printf("  Round %" PRIu32 ": ", i);
            PRINT_BUF(buf, EXHAUST_CHUNK);
        }
    }
    CHECK(fail_count == 0, "FIFO exhaustion: all rounds succeeded");
}

/* ------------------------------------------------------------------
 * Test 7: consecutive outputs must not be identical
 * ------------------------------------------------------------------ */
static void test_output_uniqueness(void)
{
#define UNIQUE_WORDS  8u
#define UNIQUE_ROUNDS 10u
    static uint32_t prev[UNIQUE_WORDS];
    static uint32_t curr[UNIQUE_WORDS];
    uint32_t        dup_count = 0;

    printf("\r\n--- Test 7: output uniqueness (%u rounds) ---\r\n", UNIQUE_ROUNDS);
    rng_init();
    rng_buffer(prev, UNIQUE_WORDS);
    printf("  Round 0: "); PRINT_BUF(prev, UNIQUE_WORDS);

    for (uint32_t i = 1; i < UNIQUE_ROUNDS; i++)
    {
        memset(curr, 0, sizeof(curr));
        rng_buffer(curr, UNIQUE_WORDS);
        printf("  Round %" PRIu32 ": ", i); PRINT_BUF(curr, UNIQUE_WORDS);
        if (buf_equal(prev, curr, UNIQUE_WORDS))
        {
            printf("  Round %" PRIu32 ": duplicate output!\r\n", i);
            dup_count++;
        }
        memcpy(prev, curr, sizeof(curr));
    }
    CHECK(dup_count == 0, "uniqueness: no duplicate consecutive outputs");
}

/* ------------------------------------------------------------------
 * Test 8: multi-caller simulation — two logical callers interleave
 *         rng_buffer calls with different sizes, no reinit between.
 *         Validates that removing the context parameter did not break
 *         independent use of the shared driver state.
 * ------------------------------------------------------------------ */
static void test_multi_caller(void)
{
#define CALLER_A_WORDS 8u
#define CALLER_B_WORDS 4u
#define CALLER_ROUNDS  20u
    static uint32_t buf_a[CALLER_A_WORDS];
    static uint32_t buf_b[CALLER_B_WORDS];
    uint32_t        fail_count = 0;

    printf("\r\n--- Test 8: multi-caller interleave (%u rounds) ---\r\n", CALLER_ROUNDS);
    rng_init();

    for (uint32_t i = 0; i < CALLER_ROUNDS; i++)
    {
        memset(buf_a, 0, sizeof(buf_a));
        if (rng_buffer(buf_a, CALLER_A_WORDS) != ERR_NONE || buf_all_zero(buf_a, CALLER_A_WORDS))
        {
            printf("  Caller A round %" PRIu32 " failed\r\n", i);
            fail_count++;
        }
        else
        {
            printf("  A[%" PRIu32 "]: ", i); PRINT_BUF(buf_a, CALLER_A_WORDS);
        }

        memset(buf_b, 0, sizeof(buf_b));
        if (rng_buffer(buf_b, CALLER_B_WORDS) != ERR_NONE || buf_all_zero(buf_b, CALLER_B_WORDS))
        {
            printf("  Caller B round %" PRIu32 " failed\r\n", i);
            fail_count++;
        }
        else
        {
            printf("  B[%" PRIu32 "]: ", i); PRINT_BUF(buf_b, CALLER_B_WORDS);
        }
    }
    CHECK(fail_count == 0, "multi-caller: all interleaved calls succeeded");
}

/* ------------------------------------------------------------------
 * Test 9: rng_buffer never returns all-zero to caller
 *         (rng_buffer has an internal all-zero retry loop)
 * ------------------------------------------------------------------ */
static void test_no_allzero_output(void)
{
#define ALLZERO_ROUNDS 30u
#define ALLZERO_WORDS  8u
    static uint32_t buf[ALLZERO_WORDS];
    uint32_t        allzero_count = 0;
    uint32_t        err_count     = 0;

    printf("\r\n--- Test 9: rng_buffer never returns all-zero (%u rounds) ---\r\n", ALLZERO_ROUNDS);
    rng_init();

    for (uint32_t i = 0; i < ALLZERO_ROUNDS; i++)
    {
        memset(buf, 0, sizeof(buf));
        cr_status ret = rng_buffer(buf, ALLZERO_WORDS);
        if (ret != ERR_NONE)
            err_count++;
        else if (buf_all_zero(buf, ALLZERO_WORDS))
            allzero_count++;
        else
        {
            printf("  [%" PRIu32 "]: ", i); PRINT_BUF(buf, ALLZERO_WORDS);
        }
    }
    CHECK(err_count == 0, "no-allzero: rng_buffer always returns ERR_NONE");
    CHECK(allzero_count == 0, "no-allzero: rng_buffer never returns all-zero buffer");
}

/* ------------------------------------------------------------------
 * Test 10: rng_init idempotent — repeated calls must not break
 *          subsequent rng_buffer
 * ------------------------------------------------------------------ */
static void test_reinit_idempotent(void)
{
    uint32_t buf[8] = { 0 };

    printf("\r\n--- Test 10: rng_init called multiple times ---\r\n");
    rng_init();
    rng_init();
    rng_init();
    cr_status ret = rng_buffer(buf, 8);
    CHECK(ret == ERR_NONE, "reinit: rng_buffer succeeds after triple rng_init");
    CHECK(!buf_all_zero(buf, 8), "reinit: output is non-zero after triple rng_init");
    PRINT_BUF(buf, 8);
}

int main(void)
{
    printf("\nHello daric trng demo. Build @ %s %s.\n", __DATE__, __TIME__);

    HAL_Init();
    HAL_TickInit();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    printf("\nDelay 2 seconds.\n");
    HAL_Delay(2000);
    printf("========================================\r\n");
    printf("  TRNG LL-API Comprehensive Test Suite\r\n");
    printf("========================================\r\n");

    /* test_lazy_init must run before any rng_init call */
    test_lazy_init();
    test_invalid_size_zero();
    test_invalid_size_overflow();
    test_min_size();
    test_max_size();
    test_fifo_exhaustion();
    test_output_uniqueness();
    test_multi_caller();
    test_no_allzero_output();
    test_reinit_idempotent();

    printf("\r\n========================================\r\n");
    printf("  Results: %" PRIu32 " passed, %" PRIu32 " failed\r\n", g_pass_count, g_fail_count);
    printf("  Overall: %s\r\n", g_fail_count == 0 ? "PASS" : "FAIL");
    printf("========================================\r\n");

    while (1)
        ;

    return 0;
}
