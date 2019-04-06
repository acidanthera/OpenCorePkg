/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* C port of DumpPublicKey.java from the Android Open source project with
 * support for additional RSA key sizes. (platform/system/core,git/libmincrypt
 * /tools/DumpPublicKey.java). Uses the OpenSSL X509 and BIGNUM library.
 */
#include <openssl/pem.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "openssl_compat.h"
/* Command line tool to extract RSA public keys from X.509 certificates
 * and output a pre-processed version of keys for use by RSA verification
 * routines.
 */
int check(RSA* key) {
  const BIGNUM *n, *e;
  int public_exponent, modulus;
  RSA_get0_key(key, &n, &e, NULL);
  public_exponent = BN_get_word(e);
  modulus = BN_num_bits(n);
  if (public_exponent != 3 && public_exponent != 65537) {
    fprintf(stderr,
            "WARNING: Public exponent should be 3 or 65537 (but is %d).\n",
            public_exponent);
  }
  if (modulus != 1024 && modulus != 2048 && modulus != 3072 && modulus != 4096
      && modulus != 8192) {
    fprintf(stderr, "ERROR: Unknown modulus length = %d.\n", modulus);
    return 0;
  }
  return 1;
}
void native_to_big(unsigned char *data, size_t size) {
  size_t i, tmp = 1;
  if (*(unsigned char *)&tmp == 1) {
    fprintf(stderr, "WARNING: Assuming little endian encoding.\n");
    for (i = 0; i < size / 2; ++i) {
      tmp     = data[i];
      data[i] = data[size - i - 1];
      data[size - i - 1] = tmp;
    }
  } else {
    fprintf(stderr, "WARNING: Assuming big endian encoding.\n");
  }
}
void print_data(void *data, size_t size) {
  size_t i;
  static size_t block = 0;
  if (data == NULL) {
    if (block > 0)
      puts("");
  } else {
    for (i = 0; i < size; ++i, ++block) {
      if (block == 12) {
        puts(",");
        block = 0;
      }
      printf("%s0x%02x", block == 0 ? "" : ", ", ((uint8_t *)data)[i]);
    }
  }
}
/* Pre-processes and outputs RSA public key to standard out.
 */
void output(RSA* key) {
  int i, nwords;
  const BIGNUM *key_n;
  BIGNUM *N = NULL;
  BIGNUM *Big1 = NULL, *Big2 = NULL, *Big32 = NULL, *BigMinus1 = NULL;
  BIGNUM *B = NULL;
  BIGNUM *N0inv= NULL, *R = NULL, *RR = NULL, *RRTemp = NULL, *NnumBits = NULL;
  BIGNUM *n = NULL, *rr = NULL;
  BN_CTX *bn_ctx = BN_CTX_new();
  uint32_t n0invout;
  /* Output size of RSA key in 32-bit words */
  nwords = RSA_size(key) / 4;
  print_data(&nwords, sizeof(nwords));
  /* Initialize BIGNUMs */
  RSA_get0_key(key, &key_n, NULL, NULL);
  N = BN_dup(key_n);
  Big1 = BN_new();
  Big2 = BN_new();
  Big32 = BN_new();
  BigMinus1 = BN_new();
  N0inv= BN_new();
  R = BN_new();
  RR = BN_new();
  RRTemp = BN_new();
  NnumBits = BN_new();
  n = BN_new();
  rr = BN_new();
  BN_set_word(Big1, 1L);
  BN_set_word(Big2, 2L);
  BN_set_word(Big32, 32L);
  BN_sub(BigMinus1, Big1, Big2);
  B = BN_new();
  BN_exp(B, Big2, Big32, bn_ctx); /* B = 2^32 */
  /* Calculate and output N0inv = -1 / N[0] mod 2^32 */
  BN_mod_inverse(N0inv, N, B, bn_ctx);
  BN_sub(N0inv, B, N0inv);
  n0invout = BN_get_word(N0inv);
  print_data(&n0invout, sizeof(n0invout));
  /* Calculate R = 2^(# of key bits) */
  BN_set_word(NnumBits, BN_num_bits(N));
  BN_exp(R, Big2, NnumBits, bn_ctx);
  /* Calculate RR = R^2 mod N */
  BN_copy(RR, R);
  BN_mul(RRTemp, RR, R, bn_ctx);
  BN_mod(RR, RRTemp, N, bn_ctx);
  /* Write out modulus as little endian array of integers. */
  for (i = 0; i < nwords; ++i) {
    uint32_t nout;
    BN_mod(n, N, B, bn_ctx); /* n = N mod B */
    nout = BN_get_word(n);
    print_data(&nout, sizeof(nout));
    BN_rshift(N, N, 32); /*  N = N/B */
  }
  /* Write R^2 as little endian array of integers. */
  for (i = 0; i < nwords; ++i) {
    uint32_t rrout;
    BN_mod(rr, RR, B, bn_ctx); /* rr = RR mod B */
    rrout = BN_get_word(rr);
    print_data(&rrout, sizeof(rrout));
    BN_rshift(RR, RR, 32); /* RR = RR/B */
  }
  /* print terminator */
  print_data(NULL, 0);
  /* Free BIGNUMs. */
  BN_free(N);
  BN_free(Big1);
  BN_free(Big2);
  BN_free(Big32);
  BN_free(BigMinus1);
  BN_free(N0inv);
  BN_free(R);
  BN_free(RRTemp);
  BN_free(NnumBits);
  BN_free(n);
  BN_free(rr);
}
enum {
  INVALID_MODE,
  CERT_MODE,
  PEM_MODE,
  RAW_MODE
};
int main(int argc, char* argv[]) {
  int mode = INVALID_MODE;
  FILE* fp;
  X509* cert = NULL;
  BIGNUM *mod = NULL;
  BIGNUM *exp = NULL;
  RSA* pubkey = NULL;
  EVP_PKEY* key;
  char *progname;
  long size;
  if (argc == 3) {
    if (!strcmp(argv[1], "-cert"))
      mode = CERT_MODE;
    else if (!strcmp(argv[1], "-pub"))
      mode = PEM_MODE;
    else if (!strcmp(argv[1], "-raw"))
      mode = RAW_MODE;
  }
  if (argc != 3 || mode == INVALID_MODE) {
    progname = strrchr(argv[0], '/');
    if (progname)
      progname++;
    else
      progname = argv[0];
    fprintf(stderr, "Usage: %s <-cert | -pub | -raw> <file>\n", progname);
    return -1;
  }
  fp = fopen(argv[2], "r");
  if (!fp) {
    fprintf(stderr, "Couldn't open file %s!\n", argv[2]);
    return -1;
  }
  if (mode == CERT_MODE) {
    /* Read the certificate */
    if (!PEM_read_X509(fp, &cert, NULL, NULL)) {
      fprintf(stderr, "Couldn't read certificate.\n");
      goto fail;
    }
    /* Get the public key from the certificate. */
    key = X509_get_pubkey(cert);
    /* Convert to a RSA_style key. */
    if (!(pubkey = EVP_PKEY_get1_RSA(key))) {
      fprintf(stderr, "Couldn't convert to a RSA style key.\n");
      goto fail;
    }
  } else if (mode == PEM_MODE) {
    /* Read the pubkey in .PEM format. */
    if (!(pubkey = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL))) {
      fprintf(stderr, "Couldn't read public key file.\n");
      goto fail;
    }
  } else {
    /* It is unimportant which exponent is used. */
    static unsigned char expraw[4] = {0, 0, 0, 3};
    static unsigned char modraw[1024];
    if (fseek (fp, 0, SEEK_END) != 0
      || (size = ftell(fp)) < 0
      || fseek(fp, 0, SEEK_SET) != 0) {
      fprintf(stderr, "Couldn't read modulus size.\n");
      goto fail;
    }
    if ((size_t)size > sizeof(modraw)) {
      fprintf(stderr, "Unsupported modulus size %ld.\n", size);
      goto fail;
    }
    if (fread(modraw, size, 1, fp) != 1) {
      fprintf(stderr, "Couldn't read modulus number.\n");
      goto fail;
    }
    native_to_big(modraw, (size_t)size);
    if (!(mod = BN_bin2bn(modraw, (int)size, NULL))) {
      fprintf(stderr, "Couldn't create modulus number.\n");
      goto fail;
    }
    if (!(exp = BN_bin2bn(expraw, sizeof(expraw), NULL))) {
      fprintf(stderr, "Couldn't create public exp number.\n");
      goto fail;
    }
    if (!(pubkey = RSA_new()) || !RSA_set0_key(pubkey, mod, exp, NULL)) {
      fprintf(stderr, "Couldn't create rsa public key.\n");
      goto fail;
    }
    /* RSA context owns its numbers. */
    mod = exp = NULL;
  }
  if (check(pubkey)) {
    output(pubkey);
  }
fail:
  X509_free(cert);
  BN_free(mod);
  BN_free(exp);
  RSA_free(pubkey);
  fclose(fp);
  return 0;
}
