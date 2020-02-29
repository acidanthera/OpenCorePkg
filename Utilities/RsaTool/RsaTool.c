/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* C port of DumpPublicKey.java from the Android Open source project with
 * support for additional RSA key sizes. (platform/system/core,git/libmincrypt
 * /tools/DumpPublicKey.java). Uses the OpenSSL X509 and BIGNUM library.
 */
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "openssl_compat.h"
/* Command line tool to extract RSA public keys from X.509 certificates
 * and output a pre-processed version of keys for use by RSA verification
 * routines.
 */
static int check(RSA* key) {
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
static void native_to_big(unsigned char* data, size_t size) {
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
typedef void (*t_data_printer)(void* context, void* data, size_t size);
static void print_data(void* context, void* data, size_t size) {
  size_t i;
  (void)context;
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
static void write_data(void* context, void* data, size_t size) {
  if (size > 0 && fwrite(data, size, 1, (FILE *)context) != 1) {
    abort();
  }
}
/* Pre-processes and outputs RSA public key to standard out.
 */
static void output(RSA* key, t_data_printer printer, void *printer_ctx) {
  uint64_t i, nwords;
  const BIGNUM *key_n;
  BIGNUM *N = NULL;
  BIGNUM *Big1 = NULL, *Big2 = NULL, *Big64 = NULL, *BigMinus1 = NULL;
  BIGNUM *B = NULL;
  BIGNUM *N0inv= NULL, *R = NULL, *RR = NULL, *RRTemp = NULL, *NnumBits = NULL;
  BIGNUM *n = NULL, *rr = NULL;
  BN_CTX *bn_ctx = BN_CTX_new();
  uint64_t n0invout;
  /* Output size of RSA key in 64-bit words */
  nwords = RSA_size(key) / 8;
  if (nwords > UINT16_MAX) return;
  printer(printer_ctx, &nwords, sizeof(nwords));
  /* Initialize BIGNUMs */
  RSA_get0_key(key, &key_n, NULL, NULL);
  N = BN_dup(key_n);
  Big1 = BN_new();
  Big2 = BN_new();
  Big64 = BN_new();
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
  BN_set_word(Big64, 64L);
  BN_sub(BigMinus1, Big1, Big2);
  B = BN_new();
  BN_exp(B, Big2, Big64, bn_ctx); /* B = 2^64 */
  /* Calculate and output N0inv = -1 / N[0] mod 2^64 */
  BN_mod_inverse(N0inv, N, B, bn_ctx);
  BN_sub(N0inv, B, N0inv);
  n0invout = (uint64_t) BN_get_word(N0inv);
  BN_rshift(N0inv, N0inv, 32);
  n0invout |= (uint64_t) BN_get_word(N0inv) << 32ULL;
  printer(printer_ctx, &n0invout, sizeof(n0invout));
  /* Calculate R = 2^(# of key bits) */
  BN_set_word(NnumBits, BN_num_bits(N));
  BN_exp(R, Big2, NnumBits, bn_ctx);
  /* Calculate RR = R^2 mod N */
  BN_copy(RR, R);
  BN_mul(RRTemp, RR, R, bn_ctx);
  BN_mod(RR, RRTemp, N, bn_ctx);
  /* Write out modulus as little endian array of integers. */
  for (i = 0; i < nwords*2; ++i) {
    uint32_t nout;
    BN_mod(n, N, B, bn_ctx); /* n = N mod B */
    nout = BN_get_word(n);
    BN_rshift(N, N, 32); /*  N = N/B */
    printer(printer_ctx, &nout, sizeof(nout));
  }
  /* Write R^2 as little endian array of integers. */
  for (i = 0; i < nwords*2; ++i) {
    uint32_t rrout;
    BN_mod(rr, RR, B, bn_ctx); /* rr = RR mod B */
    rrout = BN_get_word(rr);
    BN_rshift(RR, RR, 32); /* RR = RR/B */
    printer(printer_ctx, &rrout, sizeof(rrout));
  }
  /* print terminator */
  printer(printer_ctx, NULL, 0);
  /* Free BIGNUMs. */
  BN_free(N);
  BN_free(Big1);
  BN_free(Big2);
  BN_free(Big64);
  BN_free(BigMinus1);
  BN_free(N0inv);
  BN_free(R);
  BN_free(RRTemp);
  BN_free(NnumBits);
  BN_free(n);
  BN_free(rr);
}
uint8_t* read_file(FILE *fp, unsigned int *size) {
  long fsize = 0;
  uint8_t *data = NULL;
  if (fseek(fp, 0, SEEK_END)) return NULL;
  if ((fsize = ftell(fp)) <= 0 || fsize > UINT_MAX) return NULL;
  if (fseek(fp, 0, SEEK_SET)) return NULL;
  *size = (unsigned int)fsize;
  if (!(data = malloc(fsize))) return NULL;
  if (fread(data, fsize, 1, fp) == 1) return data;
  free(data);
  return NULL;
}
int sign_file(FILE* fp, const char* sigfile, const char *pubkfile) {
  RSA* rsa = NULL;
  BIGNUM* bn = NULL;
  EVP_PKEY* key = NULL;
  const EVP_MD* md = NULL;
  EVP_MD_CTX* ctx = NULL;
  FILE* sigf = NULL;
  FILE* pubkf = NULL;
  uint8_t* fp_data = NULL;
  unsigned int fp_size = 0;
  uint8_t signature[256];
  unsigned int signature_size;
  int result = -1;

  if (!(fp_data = read_file(fp, &fp_size))) goto done;
  if (!(rsa = RSA_new())) goto done;
  if (!(bn = BN_new())) goto done;
  if (!(md = EVP_sha256())) goto done;
  if (!(key = EVP_PKEY_new())) goto done;
  if (!(ctx = EVP_MD_CTX_create())) goto done;
  if (!BN_set_word(bn, RSA_F4)) goto done;
  if (!RSA_generate_key_ex(rsa, 2048, bn, NULL)) goto done;
  if (!EVP_PKEY_set1_RSA(key, rsa)) goto done;
  if (EVP_PKEY_size(key) != sizeof(signature)) goto done;
  if (!EVP_SignInit(ctx, md)) goto done;
  if (!EVP_SignUpdate(ctx, fp_data, fp_size)) goto done;
  if (!EVP_SignFinal(ctx, &signature[0], &signature_size, key)) goto done;

  sigf = fopen(sigfile, "wb");
  if (!sigf) goto done;
  if (fwrite(signature, signature_size, 1, sigf) != 1) goto done;
  pubkf = fopen(pubkfile, "wb");
  if (!pubkf) goto done;
  output(rsa, write_data, pubkf);
  result = 0;

done:
  RSA_free(rsa);
  BN_free(bn);
  EVP_PKEY_free(key);
  EVP_MD_CTX_cleanup(ctx);
  free(fp_data);
  if (sigf) fclose(sigf);
  if (pubkf) fclose(pubkf);

  return result;
}
enum {
  INVALID_MODE,
  CERT_MODE,
  PEM_MODE,
  RAW_MODE,
  SIGN_MODE
};
int main(int argc, char* argv[]) {
  int mode = INVALID_MODE;
  FILE* fp;
  X509* cert = NULL;
  BIGNUM *mod = NULL;
  BIGNUM *exp = NULL;
  RSA* pubkey = NULL;
  EVP_PKEY* key = NULL;
  char* progname;
  long size;
  if (argc == 3) {
    if (!strcmp(argv[1], "-cert"))
      mode = CERT_MODE;
    else if (!strcmp(argv[1], "-pub"))
      mode = PEM_MODE;
    else if (!strcmp(argv[1], "-raw"))
      mode = RAW_MODE;
  } else if (argc == 5) {
    if (!strcmp(argv[1], "-sign"))
      mode = SIGN_MODE;
  }
  if (mode == INVALID_MODE) {
    progname = strrchr(argv[0], '/');
    if (progname)
      progname++;
    else
      progname = argv[0];
    fprintf(stderr, "Usage: %s <-cert | -pub | -raw> <file>\n", progname);
    fprintf(stderr, "Usage: %s -sign <file> <signature> <pubkey>\n", progname);
    return -1;
  }
  fp = fopen(argv[2], "r");
  if (!fp) {
    fprintf(stderr, "Couldn't open file %s!\n", argv[2]);
    return -1;
  }

  if (mode == SIGN_MODE) {
    int ret = sign_file(fp, argv[3], argv[4]);
    fclose (fp);
    return ret;
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
    output(pubkey, print_data, NULL);
  }
fail:
  X509_free(cert);
  BN_free(mod);
  BN_free(exp);
  RSA_free(pubkey);
  EVP_PKEY_free(key);
  fclose(fp);
  return 0;
}
