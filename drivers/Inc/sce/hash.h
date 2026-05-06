/**
 ******************************************************************************
 * @file    hash.h
 * @author  SCE Team
 * @brief   Header file for HASH driver.
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

#ifndef __HASH_H__
#define __HASH_H__

#define HASH_LITTLE // Hardware endianness indicator

// #define HASH_DEBUG

#define HASH_PAD 0x80

#define SHA256_MODE 0
#define SHA512_MODE 1

#define SHA256_BLOCK_SIZE  64
#define SHA256_DIGEST_SIZE 32 // SHA256 outputs a 32 byte digest

#define SHA512_BLOCK_SIZE  128
#define SHA512_DIGEST_SIZE 64 // SHA512 outputs a 64 byte digest

uint32_t SCE_U8ToU32_le(const uint8_t *input, uint32_t *output, uint32_t num);
uint32_t SCE_U8ToU32(const uint8_t *input, uint32_t *output, uint32_t num);
uint32_t SCE_U64ToU32(const uint64_t *input, uint32_t *output, uint32_t num);
uint32_t SCE_U32ToU8(const uint32_t *input, uint8_t *output, uint32_t num);
uint32_t SCE_U32_COPY(const uint32_t *input, uint8_t *output, uint32_t num);

void sce_Sysinit_Hash(void);
void hash_init(void);

// Segmented input
typedef struct
{
    uint8_t  data[128]; //
    uint32_t dataLen;
    uint64_t msgBit_H; //
    uint64_t msgBit_L; //
    uint32_t sign;     //
    uint8_t  h[64];    // hash state
    uint32_t msgCount;
} SHA_CTX;
extern SHA_CTX SHA_INFO;
void           sha256_512_Init(void);
void           sha256_512_Input(const uint8_t *indata, uint32_t indataLen, uint8_t cacl_mode);
void           sha256_512_Result(const uint8_t *indata, uint32_t indataLen, uint8_t cacl_mode, uint8_t *hash);

// One-time calculation
void sha256_512_calc(const uint8_t *indata, uint32_t indataLen, uint8_t cacl_mode, uint8_t *hash);

extern SHA_CTX HMAC_INFO;
// hmac One-time calculation
void     hmac_sha256_512_KeyInit(const uint8_t *key, uint32_t keylen, uint8_t cacl_mode);
uint32_t hmac_sha256_512_MsgInput(const uint8_t *msg, uint32_t msgLen, uint8_t cacl_mode);
void     hmac_sha256_512_Result(uint8_t *mac, uint32_t msgCount, uint8_t cacl_mode);

// hmac Segmented input
void hmac_sha256_512_KeyInit_Step(const uint8_t *key, uint32_t keylen, uint8_t cacl_mode);
void hmac_sha256_512_MsgInput_Step(const uint8_t *msg, uint32_t msgLen, uint8_t cacl_mode);
void hmac_sha256_512_Result_Step(uint8_t *msg, uint32_t msgLen, uint8_t *mac, uint8_t cacl_mode);

#define BLAKE2S_BLOCKBYTES 64
#define BLAKE2S_OUTBYTES   32
typedef struct
{
    uint32_t h[8];
    uint32_t t[2];
    uint32_t f[2];
    uint8_t  buf[BLAKE2S_BLOCKBYTES];
    uint32_t buflen;
    uint32_t outlen;
} blake2s_state;

// blake2
int blake2s_init(void);
int blake2s_update(const uint8_t *indata, uint32_t indataLen);
int blake2s_final(uint8_t *out, uint32_t outlen);
int blake2s(uint8_t *out, uint32_t outlen, const uint8_t *in, uint32_t inlen);

#define BLAKE2B_BLOCKBYTES 128
#define BLAKE2B_OUTBYTES   64
typedef struct
{
    uint32_t h[16];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t  buf[BLAKE2B_BLOCKBYTES];
    uint32_t buflen;
    uint32_t outlen;
} blake2b_state;

int blake2b_init(void);
int blake2b_update(const uint8_t *indata, uint32_t indataLen);
int blake2b_final(uint8_t *out, uint32_t outlen);
int blake2b(uint8_t *out, uint32_t outlen, const uint8_t *in, uint32_t inlen);

#define BLAKE3_OUTBYTES   32
#define BLAKE3_BLOCKBYTES 64

#define CHUNK_START (1u << 0)
#define CHUNK_END   (1u << 1)
#define PARENT      (1u << 2)
#define ROOT        (1u << 3)

typedef struct
{
    uint8_t   buf[BLAKE3_BLOCKBYTES]; /* current input bytes */
    uint32_t  buflen;                 /* bytes in current input block */
    uint32_t  block;                  /* block index in chunk */
    uint64_t  chunk;                  /* chunk index */
    uint32_t *cv, cv_buf[54 * 8];     /* chain value stack */
    uint32_t  outlen;
} blake3_state;

int  blake3_init(void);
void blake3_update(const uint8_t *indata, uint32_t indataLen);
int  blake3_final(uint8_t *out, uint32_t outlen);
int  blake3(uint8_t *out, uint32_t outlen, const uint8_t *in, uint32_t inlen);

// sha3
typedef struct
{
    union
    {                    // state:
        uint8_t  b[200]; // 8-bit bytes
        uint32_t q[50];  // 64-bit words
    } st;
    int pt, rsiz, mdlen; // these don't overflow
} sha3_ctx_t;

void sha3(const uint8_t *in, uint32_t inlen, uint8_t *md, uint32_t mdlen);
int  sha3_init(sha3_ctx_t *c, int mdlen);
int  sha3_update(sha3_ctx_t *c, const void *data, size_t len);
int  sha3_final(void *md, sha3_ctx_t *c);
void ripemd160(const uint8_t *indata, uint32_t indataLen, uint8_t *outData);
void printBuffer(char *name, uint8_t *ptr, uint32_t len);
void printBufferU32(char *name, uint32_t *ptr, uint32_t len);

#endif
