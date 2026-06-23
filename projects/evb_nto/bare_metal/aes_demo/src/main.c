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

/* test result */
typedef enum
{
    TEST_SUCCESS = 0,
    TEST_FAILURE = 1
} TestResult;

static char *aeskey128[] = {
    "B53FD32319D3F6B80C2FEAF429EF9BBD", "1443140684A3320C74D5E7F313F6A8AA", "35B6D116CF108EA7F2C7608CF75888F5", "0348F0D3A8077AFBBC79E75E5EFD7B31",
    "78F30BB1D4D023057537433E715EBD8D", "99936C57C59E4EDA4F66E980F9EDD7FC", "9CB86C2448D171F3FEF5945995A257FC", "C95A1BCE92E1C4CBB18062F18A28B0A2",
};
static char *aeskey192[] = {
    "A595459D14A5D3E46B96924F5C3F60AE75A918EC5D04D5A1", "4CACD131702ABB9FAC8763C1A541E0A42968BC90EA62C757", "FA500C43E4E5672C7C23B6794F4F0892411A7E3F54EF1D6A",
    "2706D1FC3702E2D627E24F9FF9846432B9EA5857F30451DA", "7AB04301E01C97F1007042C76159565476BE63437D3CAF90", "834E0BC40978E53A03B122261FB9BC464F151816EC660EEB",
    "90E1AEF871B9212D627659026EBA61AB844C31D989E5C52E", "AB80A8995EBA6A6F412D6FBCD2EAFA07481F6B25E8253E2B",
};
static char *aeskey256[] = {
    "1C53D9450EFC89957FD2813B538409BEC4DE0BEE664C7B85C7FE3DCFE9392F62", "BF2374BB13FE233F7996F7427DC31B3C9FE9C3B061332E66568E95CC977025AA",
    "DD94CE365442BEB49CC8B1E6DF20FA84FA2A5B1547807323925DD0343D4AFD65", "F88F2BF005F1D1C06104ED8DEB02AE10FBDCE68FD564CDAE2C81263DCAA6A89A",
    "7B631FE2A63A82189D15111B948237079CB5981E156B64A6AC0564F450460428", "79B158C3EA4A85B89F689E1F0ADA1DD724AB195A544A73D919FA2EA2A86E72CD",
    "FFB4D9DD1D6E1D319EF9C83FCD605DF84EB6E3795CA3F5ADCE1A7F0501FC155B", "54025671EF3B9485F7DF54B21E55A86338602E6BB205E89B9A1EF2B5AAF91F4C",
};
static char *aesiv = "e5a481e21f70c77b48a477f1c1ed394b";
/*aes test plain text 128bytes*/
static char *aesplaintext
    = "869E92C0AC7F8350436C3890791A3B9EC131ED6B32660D2AB6CED19CC4912147E99BE3F788974873B4D9449AD1514E448AC08B87C66D5B20DFC57C2B2A888D95BD8A716E7DA32DF9D2E07FF6616D976E80D1DE8F8A67"
      "29AE2E5AF65F96446DF0916897A15FA1730A4013F2148EAFD8C20C44D656E134DCACDF597994B4A856CF";

static void run_aes_api_test(void)
{
    uint32_t ret = TEST_SUCCESS;

    uint32_t  aeskey[8]  = { 0 };
    uint32_t  iv[4]      = { 0 };
    uint32_t  plain[32]  = { 0 };
    uint32_t  cipher[32] = { 0 };
    uint32_t  tmpbuf[32] = { 0 };
    uint32_t  testkeycnt, i;
    cr_status result;

    printf("==== Starting AES API Test ====\r\n");

    bnu_hex2buf_be(aesplaintext, (uint8_t *)plain, sizeof(plain));
    bnu_hex2buf_be(aesiv, (uint8_t *)iv, sizeof(iv));

    /*test case 1: AES ECB*/
    testkeycnt = sizeof(aeskey128) / sizeof(aeskey128[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey128[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_ECB, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES ECB encrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_ECB, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES ECB decrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES ECB test key128[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES ECB test key128[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey192) / sizeof(aeskey192[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey192[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_ECB, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES ECB encrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_ECB, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES ECB decrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES ECB test key192[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES ECB test key192[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey256) / sizeof(aeskey256[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey256[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_ECB, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES ECB encrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_ECB, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES ECB decrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES ECB test key256[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES ECB test key256[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }

    /*test case 2: AES CBC*/
    testkeycnt = sizeof(aeskey128) / sizeof(aeskey128[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey128[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CBC, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CBC encrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CBC, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CBC decrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CBC test key128[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CBC test key128[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey192) / sizeof(aeskey192[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey192[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CBC, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CBC encrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CBC, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CBC decrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CBC test key192[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CBC test key192[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey256) / sizeof(aeskey256[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey256[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CBC, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CBC encrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CBC, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CBC decrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CBC test key256[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CBC test key256[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }

    /*test case 3: AES CTR*/
    testkeycnt = sizeof(aeskey128) / sizeof(aeskey128[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey128[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CTR, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CTR encrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CTR, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CTR decrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CTR test key128[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CTR test key128[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey192) / sizeof(aeskey192[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey192[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CTR, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CTR encrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CTR, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CTR decrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CTR test key192[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CTR test key192[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey256) / sizeof(aeskey256[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey256[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CTR, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CTR encrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CTR, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CTR decrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CTR test key256[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CTR test key256[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }

    /*test case 4: AES CFB*/
    testkeycnt = sizeof(aeskey128) / sizeof(aeskey128[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey128[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CFB, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CFB encrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CFB, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CFB decrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CFB test key128[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CFB test key128[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey192) / sizeof(aeskey192[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey192[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CFB, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CFB encrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CFB, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CFB decrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CFB test key192[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CFB test key192[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey256) / sizeof(aeskey256[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey256[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_CFB, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CFB encrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_CFB, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES CFB decrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES CFB test key256[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES CFB test key256[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }

    /*test case 5: AES OFB*/
    testkeycnt = sizeof(aeskey128) / sizeof(aeskey128[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey128[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_OFB, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES OFB encrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_OFB, (uint8_t *)aeskey, AES_KEY_128, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES OFB decrypt key128[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES OFB test key128[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES OFB test key128[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey192) / sizeof(aeskey192[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey192[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_OFB, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES OFB encrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_OFB, (uint8_t *)aeskey, AES_KEY_192, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES OFB decrypt key192[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES OFB test key192[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES OFB test key192[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }
    testkeycnt = sizeof(aeskey256) / sizeof(aeskey256[0]);
    for (i = 0; i < testkeycnt; i++)
    {
        bnu_hex2buf_be(aeskey256[i], (uint8_t *)aeskey, sizeof(aeskey));
        result = aes_encrypt_le(AES_OFB, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)plain, sizeof(plain), (uint8_t *)cipher);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES OFB encrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        result = aes_decrypt_le(AES_OFB, (uint8_t *)aeskey, AES_KEY_256, (uint8_t *)iv, (uint8_t *)cipher, sizeof(cipher), (uint8_t *)tmpbuf);
        if (result != ERR_NONE)
        {
            ret |= TEST_FAILURE;
            printf("AES OFB decrypt key256[%ld] error: %d\r\n", i, result);
            continue;
        }
        if (memcmp(plain, tmpbuf, sizeof(plain)) == 0)
        {
            printf("AES OFB test key256[%ld] Success!\r\n", i);
        }
        else
        {
            ret |= TEST_FAILURE;
            printf("AES OFB test key256[%ld] error!\r\n", i);
            bnu_print_mem_duart("plain", (uint8_t *)plain, sizeof(plain));
            bnu_print_mem_duart("cipher", (uint8_t *)cipher, sizeof(cipher));
            bnu_print_mem_duart("result", (uint8_t *)tmpbuf, sizeof(tmpbuf));
        }
    }

    if (ret == TEST_SUCCESS)
    {
        printf("==== All AES API Tests Passed! ====\r\n");
    }
    else
    {
        printf("==== Some AES API Tests Failed! ====\r\n");
    }
}

int main(void)
{
    printf("\nHello daric aes demo. Build @ %s %s.\n", __DATE__, __TIME__);

    HAL_Init();
    HAL_TickInit();
    HAL_ClockConfig(HAL_CPU_FREQSEL_700MHZ);
    clkAnalysis();

    printf("\nDelay 2 seconds.\n");
    HAL_Delay(2000);
    printf("Starting Baremetal AES Demo...\n");

    run_aes_api_test();

    while (1)
        ;

    return 0;
}
