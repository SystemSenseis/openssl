/*
 * Copyright 2018-2019 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright (c) 2018-2019, Oracle and/or its affiliates.  All rights reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* Tests of the EVP_KDF_CTX APIs */

#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include "testutil.h"

static EVP_KDF_CTX *get_kdfbyname(const char *name)
{
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, name, NULL);
    EVP_KDF_CTX *kctx = EVP_KDF_CTX_new(kdf);

    EVP_KDF_free(kdf);
    return kctx;
}

static int test_kdf_tls1_prf(void)
{
    int ret;
    EVP_KDF_CTX *kctx = NULL;
    unsigned char out[16];
    OSSL_PARAM params[4], *p = params;
    static const unsigned char expected[sizeof(out)] = {
        0x8e, 0x4d, 0x93, 0x25, 0x30, 0xd7, 0x65, 0xa0,
        0xaa, 0xe9, 0x74, 0xc3, 0x04, 0x73, 0x5e, 0xcc
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha256", sizeof("sha256"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SECRET,
                                             (unsigned char *)"secret",
                                             (size_t)6);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SEED,
                                             (unsigned char *)"seed",
                                             (size_t)4);
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_TLS1_PRF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_hkdf(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[10];
    OSSL_PARAM params[5], *p = params;
    static const unsigned char expected[sizeof(out)] = {
        0x2a, 0xc4, 0x36, 0x9f, 0x52, 0x59, 0x96, 0xf8, 0xde, 0x13
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha256", sizeof("sha256"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                             (unsigned char *)"salt", 4);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY,
                                             (unsigned char *)"secret", 6);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO,
                                             (unsigned char *)"label", 5);
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_HKDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_pbkdf2(void)
{
    int ret = 0;
    EVP_KDF_CTX *kctx;
    unsigned char out[25];
    size_t len = 0;
    unsigned int iterations = 4096;
    int mode = 0;
    OSSL_PARAM params[6], *p = params;
    const unsigned char expected[sizeof(out)] = {
        0x34, 0x8c, 0x89, 0xdb, 0xcb, 0xd3, 0x2b, 0x2f,
        0x32, 0xd8, 0x14, 0xb8, 0x11, 0x6e, 0x84, 0xcf,
        0x2b, 0x17, 0x34, 0x7e, 0xbc, 0x18, 0x00, 0x18,
        0x1c
    };

    if (sizeof(len) > 32)
        len = SIZE_MAX;

    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                             (unsigned char *)
                                                "passwordPASSWORDpassword", 24);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                             (unsigned char *)
                                                "saltSALTsaltSALTsaltSALTsaltSALTsalt",
                                                36);
    *p++ = OSSL_PARAM_construct_uint(OSSL_KDF_PARAM_ITER, &iterations);
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                             (char *)"sha256", 7);
    *p++ = OSSL_PARAM_construct_int(OSSL_KDF_PARAM_PKCS5, &mode);
    *p = OSSL_PARAM_construct_end();

    if (!TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_PBKDF2))
        || !TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        || !TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        || !TEST_mem_eq(out, sizeof(out), expected, sizeof(expected))
        || !TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        /* A key length that is too small should fail */
        || !TEST_int_eq(EVP_KDF_derive(kctx, out, 112 / 8 - 1), 0)
        /* A key length that is too large should fail */
        || (len != 0 && !TEST_int_eq(EVP_KDF_derive(kctx, out, len), 0)))
        goto err;
#if 0
/* TODO */
          /* Salt length less than 128 bits should fail */
          || TEST_int_eq(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT,
                                      "123456781234567",
                                      (size_t)15), 0)
          /* A small iteration count should fail */
          || TEST_int_eq(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_ITER, 1), 0)
          || TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PBKDF2_PKCS5_MODE,
                                      1), 0)
          /* Small salts will pass if the "pkcs5" mode is enabled */
          || TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT,
                                      "123456781234567",
                                      (size_t)15), 0)
          /* A small iteration count will pass if "pkcs5" mode is enabled */
          || TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_ITER, 1), 0)
          /*
           * If the "pkcs5" mode is disabled then the small salt and iter will
           * fail when the derive gets called.
           */
          || TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PBKDF2_PKCS5_MODE,
                                      0), 0)
          || TEST_int_eq(EVP_KDF_derive(kctx, out, sizeof(out)), 0);
#endif
    ret = 1;
err:
    EVP_KDF_CTX_free(kctx);
    return ret;
}

#ifndef OPENSSL_NO_SCRYPT
static int test_kdf_scrypt(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[7], *p = params;
    unsigned char out[64];
    unsigned int nu = 1024, ru = 8, pu = 16, maxmem = 16;
    static const unsigned char expected[sizeof(out)] = {
        0xfd, 0xba, 0xbe, 0x1c, 0x9d, 0x34, 0x72, 0x00,
        0x78, 0x56, 0xe7, 0x19, 0x0d, 0x01, 0xe9, 0xfe,
        0x7c, 0x6a, 0xd7, 0xcb, 0xc8, 0x23, 0x78, 0x30,
        0xe7, 0x73, 0x76, 0x63, 0x4b, 0x37, 0x31, 0x62,
        0x2e, 0xaf, 0x30, 0xd9, 0x2e, 0x22, 0xa3, 0x88,
        0x6f, 0xf1, 0x09, 0x27, 0x9d, 0x98, 0x30, 0xda,
        0xc7, 0x27, 0xaf, 0xb9, 0x4a, 0x83, 0xee, 0x6d,
        0x83, 0x60, 0xcb, 0xdf, 0xa2, 0xcc, 0x06, 0x40
    };

    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                             (char *)"password", 8);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                             (char *)"NaCl", 4);
    *p++ = OSSL_PARAM_construct_uint(OSSL_KDF_PARAM_SCRYPT_N, &nu);
    *p++ = OSSL_PARAM_construct_uint(OSSL_KDF_PARAM_SCRYPT_R, &ru);
    *p++ = OSSL_PARAM_construct_uint(OSSL_KDF_PARAM_SCRYPT_P, &pu);
    *p++ = OSSL_PARAM_construct_uint(OSSL_KDF_PARAM_SCRYPT_MAXMEM, &maxmem);
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_SCRYPT))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        /* failure test *//*
        && TEST_int_le(EVP_KDF_derive(kctx, out, sizeof(out)), 0)*/
        && TEST_true(OSSL_PARAM_set_uint(p - 1, 10 * 1024 * 1024))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, p - 1))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}
#endif /* OPENSSL_NO_SCRYPT */

static int test_kdf_ss_hash(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[4], *p = params;
    unsigned char out[14];
    static unsigned char z[] = {
        0x6d,0xbd,0xc2,0x3f,0x04,0x54,0x88,0xe4,0x06,0x27,0x57,0xb0,0x6b,0x9e,
        0xba,0xe1,0x83,0xfc,0x5a,0x59,0x46,0xd8,0x0d,0xb9,0x3f,0xec,0x6f,0x62,
        0xec,0x07,0xe3,0x72,0x7f,0x01,0x26,0xae,0xd1,0x2c,0xe4,0xb2,0x62,0xf4,
        0x7d,0x48,0xd5,0x42,0x87,0xf8,0x1d,0x47,0x4c,0x7c,0x3b,0x18,0x50,0xe9
    };
    static unsigned char other[] = {
        0xa1,0xb2,0xc3,0xd4,0xe5,0x43,0x41,0x56,0x53,0x69,0x64,0x3c,0x83,0x2e,
        0x98,0x49,0xdc,0xdb,0xa7,0x1e,0x9a,0x31,0x39,0xe6,0x06,0xe0,0x95,0xde,
        0x3c,0x26,0x4a,0x66,0xe9,0x8a,0x16,0x58,0x54,0xcd,0x07,0x98,0x9b,0x1e,
        0xe0,0xec,0x3f,0x8d,0xbe
    };
    static const unsigned char expected[sizeof(out)] = {
        0xa4,0x62,0xde,0x16,0xa8,0x9d,0xe8,0x46,0x6e,0xf5,0x46,0x0b,0x47,0xb8
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha224", sizeof("sha224"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, z, sizeof(z));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, other,
                                             sizeof(other));
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_SSKDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_x963(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[4], *p = params;
    unsigned char out[1024 / 8];
    /*
     * Test data from https://csrc.nist.gov/CSRC/media/Projects/
     *  Cryptographic-Algorithm-Validation-Program/documents/components/
     *  800-135testvectors/ansx963_2001.zip
     */
    static unsigned char z[] = {
        0x00, 0xaa, 0x5b, 0xb7, 0x9b, 0x33, 0xe3, 0x89, 0xfa, 0x58, 0xce, 0xad,
        0xc0, 0x47, 0x19, 0x7f, 0x14, 0xe7, 0x37, 0x12, 0xf4, 0x52, 0xca, 0xa9,
        0xfc, 0x4c, 0x9a, 0xdb, 0x36, 0x93, 0x48, 0xb8, 0x15, 0x07, 0x39, 0x2f,
        0x1a, 0x86, 0xdd, 0xfd, 0xb7, 0xc4, 0xff, 0x82, 0x31, 0xc4, 0xbd, 0x0f,
        0x44, 0xe4, 0x4a, 0x1b, 0x55, 0xb1, 0x40, 0x47, 0x47, 0xa9, 0xe2, 0xe7,
        0x53, 0xf5, 0x5e, 0xf0, 0x5a, 0x2d
    };
    static unsigned char shared[] = {
        0xe3, 0xb5, 0xb4, 0xc1, 0xb0, 0xd5, 0xcf, 0x1d, 0x2b, 0x3a, 0x2f, 0x99,
        0x37, 0x89, 0x5d, 0x31
    };
    static const unsigned char expected[sizeof(out)] = {
        0x44, 0x63, 0xf8, 0x69, 0xf3, 0xcc, 0x18, 0x76, 0x9b, 0x52, 0x26, 0x4b,
        0x01, 0x12, 0xb5, 0x85, 0x8f, 0x7a, 0xd3, 0x2a, 0x5a, 0x2d, 0x96, 0xd8,
        0xcf, 0xfa, 0xbf, 0x7f, 0xa7, 0x33, 0x63, 0x3d, 0x6e, 0x4d, 0xd2, 0xa5,
        0x99, 0xac, 0xce, 0xb3, 0xea, 0x54, 0xa6, 0x21, 0x7c, 0xe0, 0xb5, 0x0e,
        0xef, 0x4f, 0x6b, 0x40, 0xa5, 0xc3, 0x02, 0x50, 0xa5, 0xa8, 0xee, 0xee,
        0x20, 0x80, 0x02, 0x26, 0x70, 0x89, 0xdb, 0xf3, 0x51, 0xf3, 0xf5, 0x02,
        0x2a, 0xa9, 0x63, 0x8b, 0xf1, 0xee, 0x41, 0x9d, 0xea, 0x9c, 0x4f, 0xf7,
        0x45, 0xa2, 0x5a, 0xc2, 0x7b, 0xda, 0x33, 0xca, 0x08, 0xbd, 0x56, 0xdd,
        0x1a, 0x59, 0xb4, 0x10, 0x6c, 0xf2, 0xdb, 0xbc, 0x0a, 0xb2, 0xaa, 0x8e,
        0x2e, 0xfa, 0x7b, 0x17, 0x90, 0x2d, 0x34, 0x27, 0x69, 0x51, 0xce, 0xcc,
        0xab, 0x87, 0xf9, 0x66, 0x1c, 0x3e, 0x88, 0x16
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha512", sizeof("sha512"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, z, sizeof(z));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, shared,
                                             sizeof(shared));
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_X963KDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

/*
 * KBKDF test vectors from RFC 6803 (Camellia Encryption for Kerberos 5)
 * section 10.
 */
static int test_kdf_kbkdf_6803_128(void)
{
    int ret = 0, i, p;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[7];
    static unsigned char input_key[] = {
        0x57, 0xD0, 0x29, 0x72, 0x98, 0xFF, 0xD9, 0xD3,
        0x5D, 0xE5, 0xA4, 0x7F, 0xB4, 0xBD, 0xE2, 0x4B,
    };
    static unsigned char constants[][5] = {
        { 0x00, 0x00, 0x00, 0x02, 0x99 },
        { 0x00, 0x00, 0x00, 0x02, 0xaa },
        { 0x00, 0x00, 0x00, 0x02, 0x55 },
    };
    static unsigned char outputs[][16] = {
        {0xD1, 0x55, 0x77, 0x5A, 0x20, 0x9D, 0x05, 0xF0,
         0x2B, 0x38, 0xD4, 0x2A, 0x38, 0x9E, 0x5A, 0x56},
        {0x64, 0xDF, 0x83, 0xF8, 0x5A, 0x53, 0x2F, 0x17,
         0x57, 0x7D, 0x8C, 0x37, 0x03, 0x57, 0x96, 0xAB},
        {0x3E, 0x4F, 0xBD, 0xF3, 0x0F, 0xB8, 0x25, 0x9C,
         0x42, 0x5C, 0xB6, 0xC9, 0x6F, 0x1F, 0x46, 0x35}
    };
    static unsigned char iv[16] = { 0 };
    unsigned char result[16] = { 0 };

    for (i = 0; i < 3; i++) {
        p = 0;
        params[p++] = OSSL_PARAM_construct_utf8_string(
            OSSL_KDF_PARAM_CIPHER, "CAMELLIA-128-CBC", 0);
        params[p++] = OSSL_PARAM_construct_utf8_string(
            OSSL_KDF_PARAM_MAC, "CMAC", 0);
        params[p++] = OSSL_PARAM_construct_utf8_string(
            OSSL_KDF_PARAM_MODE, "FEEDBACK", 0);
        params[p++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_KEY, input_key, sizeof(input_key));
        params[p++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SALT, constants[i], sizeof(constants[i]));
        params[p++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SEED, iv, sizeof(iv));
        params[p] = OSSL_PARAM_construct_end();

        kctx = get_kdfbyname("KBKDF");
        ret = TEST_ptr(kctx)
            && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
            && TEST_int_gt(EVP_KDF_derive(kctx, result, sizeof(result)), 0)
            && TEST_mem_eq(result, sizeof(result), outputs[i],
                           sizeof(outputs[i]));
        EVP_KDF_CTX_free(kctx);
        if (ret != 1)
            return ret;
    }

    return ret;
}

static int test_kdf_kbkdf_6803_256(void)
{
    int ret = 0, i, p;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[7];
    static unsigned char input_key[] = {
        0xB9, 0xD6, 0x82, 0x8B, 0x20, 0x56, 0xB7, 0xBE,
        0x65, 0x6D, 0x88, 0xA1, 0x23, 0xB1, 0xFA, 0xC6,
        0x82, 0x14, 0xAC, 0x2B, 0x72, 0x7E, 0xCF, 0x5F,
        0x69, 0xAF, 0xE0, 0xC4, 0xDF, 0x2A, 0x6D, 0x2C,
    };
    static unsigned char constants[][5] = {
        { 0x00, 0x00, 0x00, 0x02, 0x99 },
        { 0x00, 0x00, 0x00, 0x02, 0xaa },
        { 0x00, 0x00, 0x00, 0x02, 0x55 },
    };
    static unsigned char outputs[][32] = {
        {0xE4, 0x67, 0xF9, 0xA9, 0x55, 0x2B, 0xC7, 0xD3,
         0x15, 0x5A, 0x62, 0x20, 0xAF, 0x9C, 0x19, 0x22,
         0x0E, 0xEE, 0xD4, 0xFF, 0x78, 0xB0, 0xD1, 0xE6,
         0xA1, 0x54, 0x49, 0x91, 0x46, 0x1A, 0x9E, 0x50,
        },
        {0x41, 0x2A, 0xEF, 0xC3, 0x62, 0xA7, 0x28, 0x5F,
         0xC3, 0x96, 0x6C, 0x6A, 0x51, 0x81, 0xE7, 0x60,
         0x5A, 0xE6, 0x75, 0x23, 0x5B, 0x6D, 0x54, 0x9F,
         0xBF, 0xC9, 0xAB, 0x66, 0x30, 0xA4, 0xC6, 0x04,
        },
        {0xFA, 0x62, 0x4F, 0xA0, 0xE5, 0x23, 0x99, 0x3F,
         0xA3, 0x88, 0xAE, 0xFD, 0xC6, 0x7E, 0x67, 0xEB,
         0xCD, 0x8C, 0x08, 0xE8, 0xA0, 0x24, 0x6B, 0x1D,
         0x73, 0xB0, 0xD1, 0xDD, 0x9F, 0xC5, 0x82, 0xB0,
        },
    };
    static unsigned char iv[16] = { 0 };
    unsigned char result[32] = { 0 };

    for (i = 0; i < 3; i++) {
        p = 0;
        params[p++] = OSSL_PARAM_construct_utf8_string(
            OSSL_KDF_PARAM_CIPHER, "CAMELLIA-256-CBC", 0);
        params[p++] = OSSL_PARAM_construct_utf8_string(
            OSSL_KDF_PARAM_MAC, "CMAC", 0);
        params[p++] = OSSL_PARAM_construct_utf8_string(
            OSSL_KDF_PARAM_MODE, "FEEDBACK", 0);
        params[p++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_KEY, input_key, sizeof(input_key));
        params[p++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SALT, constants[i], sizeof(constants[i]));
        params[p++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SEED, iv, sizeof(iv));
        params[p] = OSSL_PARAM_construct_end();

        kctx = get_kdfbyname("KBKDF");
        ret = TEST_ptr(kctx)
            && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
            && TEST_int_gt(EVP_KDF_derive(kctx, result, sizeof(result)), 0)
            && TEST_mem_eq(result, sizeof(result), outputs[i],
                           sizeof(outputs[i]));
        EVP_KDF_CTX_free(kctx);
        if (ret != 1)
            return ret;
    }

    return ret;
}

/* Two test vectors from RFC 8009 (AES Encryption with HMAC-SHA2 for Kerberos
 * 5) appendix A. */
static int test_kdf_kbkdf_8009_prf1(void)
{
    int ret, i = 0;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[6];
    char *label = "prf", *digest = "sha256", *prf_input = "test",
        *mac = "HMAC";
    static unsigned char input_key[] = {
        0x37, 0x05, 0xD9, 0x60, 0x80, 0xC1, 0x77, 0x28,
        0xA0, 0xE8, 0x00, 0xEA, 0xB6, 0xE0, 0xD2, 0x3C,
    };
    static unsigned char output[] = {
        0x9D, 0x18, 0x86, 0x16, 0xF6, 0x38, 0x52, 0xFE,
        0x86, 0x91, 0x5B, 0xB8, 0x40, 0xB4, 0xA8, 0x86,
        0xFF, 0x3E, 0x6B, 0xB0, 0xF8, 0x19, 0xB4, 0x9B,
        0x89, 0x33, 0x93, 0xD3, 0x93, 0x85, 0x42, 0x95,
    };
    unsigned char result[sizeof(output)] = { 0 };

    params[i++] = OSSL_PARAM_construct_utf8_string(
        OSSL_KDF_PARAM_DIGEST, digest, strlen(digest) + 1);
    params[i++] = OSSL_PARAM_construct_utf8_string(
        OSSL_KDF_PARAM_MAC, mac, strlen(mac) + 1);
    params[i++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_KEY, input_key, sizeof(input_key));
    params[i++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_SALT, label, strlen(label));
    params[i++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_INFO, prf_input, strlen(prf_input));
    params[i] = OSSL_PARAM_construct_end();

    kctx = get_kdfbyname("KBKDF");
    ret = TEST_ptr(kctx)
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, result, sizeof(result)), 0)
        && TEST_mem_eq(result, sizeof(result), output, sizeof(output));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_kbkdf_8009_prf2(void)
{
    int ret, i = 0;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[6];
    char *label = "prf", *digest = "sha384", *prf_input = "test",
        *mac = "HMAC";
    static unsigned char input_key[] = {
        0x6D, 0x40, 0x4D, 0x37, 0xFA, 0xF7, 0x9F, 0x9D,
        0xF0, 0xD3, 0x35, 0x68, 0xD3, 0x20, 0x66, 0x98,
        0x00, 0xEB, 0x48, 0x36, 0x47, 0x2E, 0xA8, 0xA0,
        0x26, 0xD1, 0x6B, 0x71, 0x82, 0x46, 0x0C, 0x52,
    };
    static unsigned char output[] = {
        0x98, 0x01, 0xF6, 0x9A, 0x36, 0x8C, 0x2B, 0xF6,
        0x75, 0xE5, 0x95, 0x21, 0xE1, 0x77, 0xD9, 0xA0,
        0x7F, 0x67, 0xEF, 0xE1, 0xCF, 0xDE, 0x8D, 0x3C,
        0x8D, 0x6F, 0x6A, 0x02, 0x56, 0xE3, 0xB1, 0x7D,
        0xB3, 0xC1, 0xB6, 0x2A, 0xD1, 0xB8, 0x55, 0x33,
        0x60, 0xD1, 0x73, 0x67, 0xEB, 0x15, 0x14, 0xD2,
    };
    unsigned char result[sizeof(output)] = { 0 };

    params[i++] = OSSL_PARAM_construct_utf8_string(
        OSSL_KDF_PARAM_DIGEST, digest, strlen(digest) + 1);
    params[i++] = OSSL_PARAM_construct_utf8_string(
        OSSL_KDF_PARAM_MAC, mac, strlen(mac) + 1);
    params[i++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_KEY, input_key, sizeof(input_key));
    params[i++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_SALT, label, strlen(label));
    params[i++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_INFO, prf_input, strlen(prf_input));
    params[i] = OSSL_PARAM_construct_end();

    kctx = get_kdfbyname("KBKDF");
    ret = TEST_ptr(kctx)
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, result, sizeof(result)), 0)
        && TEST_mem_eq(result, sizeof(result), output, sizeof(output));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_ss_hmac(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[6], *p = params;
    unsigned char out[16];
    static unsigned char z[] = {
        0xb7,0x4a,0x14,0x9a,0x16,0x15,0x46,0xf8,0xc2,0x0b,0x06,0xac,0x4e,0xd4
    };
    static unsigned char other[] = {
        0x34,0x8a,0x37,0xa2,0x7e,0xf1,0x28,0x2f,0x5f,0x02,0x0d,0xcc
    };
    static unsigned char salt[] = {
        0x36,0x38,0x27,0x1c,0xcd,0x68,0xa2,0x5d,0xc2,0x4e,0xcd,0xdd,0x39,0xef,
        0x3f,0x89
    };
    static const unsigned char expected[sizeof(out)] = {
        0x44,0xf6,0x76,0xe8,0x5c,0x1b,0x1a,0x8b,0xbc,0x3d,0x31,0x92,0x18,0x63,
        0x1c,0xa3
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_MAC,
                                            (char *)OSSL_MAC_NAME_HMAC,
                                            sizeof(OSSL_MAC_NAME_HMAC));
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha256", sizeof("sha256"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, z, sizeof(z));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, other,
                                             sizeof(other));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, salt,
                                             sizeof(salt));
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_SSKDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_ss_kmac(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[6], *p = params;
    unsigned char out[64];
    size_t mac_size = 20;
    static unsigned char z[] = {
        0xb7,0x4a,0x14,0x9a,0x16,0x15,0x46,0xf8,0xc2,0x0b,0x06,0xac,0x4e,0xd4
    };
    static unsigned char other[] = {
        0x34,0x8a,0x37,0xa2,0x7e,0xf1,0x28,0x2f,0x5f,0x02,0x0d,0xcc
    };
    static unsigned char salt[] = {
        0x36,0x38,0x27,0x1c,0xcd,0x68,0xa2,0x5d,0xc2,0x4e,0xcd,0xdd,0x39,0xef,
        0x3f,0x89
    };
    static const unsigned char expected[sizeof(out)] = {
        0xe9,0xc1,0x84,0x53,0xa0,0x62,0xb5,0x3b,0xdb,0xfc,0xbb,0x5a,0x34,0xbd,
        0xb8,0xe5,0xe7,0x07,0xee,0xbb,0x5d,0xd1,0x34,0x42,0x43,0xd8,0xcf,0xc2,
        0xc2,0xe6,0x33,0x2f,0x91,0xbd,0xa5,0x86,0xf3,0x7d,0xe4,0x8a,0x65,0xd4,
        0xc5,0x14,0xfd,0xef,0xaa,0x1e,0x67,0x54,0xf3,0x73,0xd2,0x38,0xe1,0x95,
        0xae,0x15,0x7e,0x1d,0xe8,0x14,0x98,0x03
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_MAC,
                                            (char *)OSSL_MAC_NAME_KMAC128,
                                            sizeof(OSSL_MAC_NAME_KMAC128));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, z, sizeof(z));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, other,
                                             sizeof(other));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, salt,
                                             sizeof(salt));
    *p++ = OSSL_PARAM_construct_size_t(OSSL_KDF_PARAM_MAC_SIZE, &mac_size);
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_SSKDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_sshkdf(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[6], *p = params;
    char kdftype = EVP_KDF_SSHKDF_TYPE_INITIAL_IV_CLI_TO_SRV;
    unsigned char out[8];
    /* Test data from NIST CAVS 14.1 test vectors */
    static unsigned char key[] = {
        0x00, 0x00, 0x00, 0x81, 0x00, 0x87, 0x5c, 0x55, 0x1c, 0xef, 0x52, 0x6a,
        0x4a, 0x8b, 0xe1, 0xa7, 0xdf, 0x27, 0xe9, 0xed, 0x35, 0x4b, 0xac, 0x9a,
        0xfb, 0x71, 0xf5, 0x3d, 0xba, 0xe9, 0x05, 0x67, 0x9d, 0x14, 0xf9, 0xfa,
        0xf2, 0x46, 0x9c, 0x53, 0x45, 0x7c, 0xf8, 0x0a, 0x36, 0x6b, 0xe2, 0x78,
        0x96, 0x5b, 0xa6, 0x25, 0x52, 0x76, 0xca, 0x2d, 0x9f, 0x4a, 0x97, 0xd2,
        0x71, 0xf7, 0x1e, 0x50, 0xd8, 0xa9, 0xec, 0x46, 0x25, 0x3a, 0x6a, 0x90,
        0x6a, 0xc2, 0xc5, 0xe4, 0xf4, 0x8b, 0x27, 0xa6, 0x3c, 0xe0, 0x8d, 0x80,
        0x39, 0x0a, 0x49, 0x2a, 0xa4, 0x3b, 0xad, 0x9d, 0x88, 0x2c, 0xca, 0xc2,
        0x3d, 0xac, 0x88, 0xbc, 0xad, 0xa4, 0xb4, 0xd4, 0x26, 0xa3, 0x62, 0x08,
        0x3d, 0xab, 0x65, 0x69, 0xc5, 0x4c, 0x22, 0x4d, 0xd2, 0xd8, 0x76, 0x43,
        0xaa, 0x22, 0x76, 0x93, 0xe1, 0x41, 0xad, 0x16, 0x30, 0xce, 0x13, 0x14,
        0x4e
    };
    static unsigned char xcghash[] = {
        0x0e, 0x68, 0x3f, 0xc8, 0xa9, 0xed, 0x7c, 0x2f, 0xf0, 0x2d, 0xef, 0x23,
        0xb2, 0x74, 0x5e, 0xbc, 0x99, 0xb2, 0x67, 0xda, 0xa8, 0x6a, 0x4a, 0xa7,
        0x69, 0x72, 0x39, 0x08, 0x82, 0x53, 0xf6, 0x42
    };
    static unsigned char sessid[] = {
        0x0e, 0x68, 0x3f, 0xc8, 0xa9, 0xed, 0x7c, 0x2f, 0xf0, 0x2d, 0xef, 0x23,
        0xb2, 0x74, 0x5e, 0xbc, 0x99, 0xb2, 0x67, 0xda, 0xa8, 0x6a, 0x4a, 0xa7,
        0x69, 0x72, 0x39, 0x08, 0x82, 0x53, 0xf6, 0x42
    };
    static const unsigned char expected[sizeof(out)] = {
        0x41, 0xff, 0x2e, 0xad, 0x16, 0x83, 0xf1, 0xe6
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha256", sizeof("sha256"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, key,
                                             sizeof(key));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SSHKDF_XCGHASH,
                                             xcghash, sizeof(xcghash));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SSHKDF_SESSION_ID,
                                             sessid, sizeof(sessid));
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_SSHKDF_TYPE,
                                            &kdftype, sizeof(kdftype));
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_SSHKDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_get_kdf(void)
{
    EVP_KDF *kdf1 = NULL, *kdf2 = NULL;
    ASN1_OBJECT *obj;
    int ok = 1;

    if (!TEST_ptr(obj = OBJ_nid2obj(NID_id_pbkdf2))
        || !TEST_ptr(kdf1 = EVP_KDF_fetch(NULL, OSSL_KDF_NAME_PBKDF2, NULL))
        || !TEST_ptr(kdf2 = EVP_KDF_fetch(NULL, OBJ_nid2sn(OBJ_obj2nid(obj)),
                                          NULL))
        || !TEST_ptr_eq(kdf1, kdf2))
        ok = 0;
    EVP_KDF_free(kdf1);
    kdf1 = NULL;
    EVP_KDF_free(kdf2);
    kdf2 = NULL;

    if (!TEST_ptr(kdf1 = EVP_KDF_fetch(NULL, SN_tls1_prf, NULL))
        || !TEST_ptr(kdf2 = EVP_KDF_fetch(NULL, LN_tls1_prf, NULL))
        || !TEST_ptr_eq(kdf1, kdf2))
        ok = 0;
    /* kdf1 is re-used below, so don't free it here */
    EVP_KDF_free(kdf2);
    kdf2 = NULL;

    if (!TEST_ptr(kdf2 = EVP_KDF_fetch(NULL, OBJ_nid2sn(NID_tls1_prf), NULL))
        || !TEST_ptr_eq(kdf1, kdf2))
        ok = 0;
    EVP_KDF_free(kdf1);
    kdf1 = NULL;
    EVP_KDF_free(kdf2);
    kdf2 = NULL;

    return ok;
}

#ifndef OPENSSL_NO_CMS
static int test_kdf_x942_asn1(void)
{
    int ret;
    EVP_KDF_CTX *kctx = NULL;
    OSSL_PARAM params[4], *p = params;
    const char *cek_alg = SN_id_smime_alg_CMS3DESwrap;
    unsigned char out[24];
    /* RFC2631 Section 2.1.6 Test data */
    static unsigned char z[] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,
        0x0e,0x0f,0x10,0x11,0x12,0x13
    };
    static const unsigned char expected[sizeof(out)] = {
        0xa0,0x96,0x61,0x39,0x23,0x76,0xf7,0x04,
        0x4d,0x90,0x52,0xa3,0x97,0x88,0x32,0x46,
        0xb6,0x7f,0x5f,0x1e,0xf6,0x3e,0xb5,0xfb
    };

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
                                            (char *)"sha1", sizeof("sha1"));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, z,
                                             sizeof(z));
    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_CEK_ALG,
                                            (char *)cek_alg,
                                            strlen(cek_alg) + 1);
    *p = OSSL_PARAM_construct_end();

    ret =
        TEST_ptr(kctx = get_kdfbyname(OSSL_KDF_NAME_X942KDF))
        && TEST_true(EVP_KDF_CTX_set_params(kctx, params))
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}
#endif /* OPENSSL_NO_CMS */

int setup_tests(void)
{
    ADD_TEST(test_kdf_kbkdf_6803_128);
    ADD_TEST(test_kdf_kbkdf_6803_256);
    ADD_TEST(test_kdf_kbkdf_8009_prf1);
    ADD_TEST(test_kdf_kbkdf_8009_prf2);
    ADD_TEST(test_kdf_get_kdf);
    ADD_TEST(test_kdf_tls1_prf);
    ADD_TEST(test_kdf_hkdf);
    ADD_TEST(test_kdf_pbkdf2);
#ifndef OPENSSL_NO_SCRYPT
    ADD_TEST(test_kdf_scrypt);
#endif
    ADD_TEST(test_kdf_ss_hash);
    ADD_TEST(test_kdf_ss_hmac);
    ADD_TEST(test_kdf_ss_kmac);
    ADD_TEST(test_kdf_sshkdf);
    ADD_TEST(test_kdf_x963);
#ifndef OPENSSL_NO_CMS
    ADD_TEST(test_kdf_x942_asn1);
#endif
    return 1;
}
