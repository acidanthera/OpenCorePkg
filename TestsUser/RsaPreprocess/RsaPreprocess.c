/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/


/**
clang -g -fsanitize=undefined,address -I../Include -I../../Include  -I../../Library/OcCryptoLib -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h RsaPreprocess.c  ../../Library/OcAppleKeysLib/OcAppleKeysLib.c ../../Library/OcCryptoLib/BigNumMontgomery.c ../../Library/OcCryptoLib/BigNumPrimitives.c ../../Library/OcCryptoLib/X64/BigNumWordMul64.c -o RsaPreprocess
**/

#include <Base.h>

#include <Library/OcCryptoLib.h>
#include <Library/OcAppleKeysLib.h>

#include <BigNumLib.h>

uint8_t *readFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize);
  fread(string, fsize, 1, f);
  fclose(f);

  *size = fsize;

  return string;
}

int verifyRsa (CONST OC_RSA_PUBLIC_KEY *PublicKey, char *Name)
{
  OC_BN_WORD N0Inv;
  UINTN ModulusSize = PublicKey->Hdr.NumQwords * sizeof (UINT64);

  OC_BN_WORD *RSqrMod = malloc(ModulusSize);
  if (RSqrMod == NULL) {
    printf ("memory allocation error!\n");
    return -1;
  }

  N0Inv = BigNumCalculateMontParams (
            RSqrMod,
            ModulusSize / OC_BN_WORD_SIZE,
            (CONST OC_BN_WORD *) PublicKey->Data
            );

  printf (
    "%s: results: %d %d (%llX vs %llX)\n",
    Name,
    memcmp (
      RSqrMod,
      &PublicKey->Data[PublicKey->Hdr.NumQwords],
      ModulusSize
      ),
    N0Inv != PublicKey->Hdr.N0Inv,
    (unsigned long long) N0Inv,
    (unsigned long long) PublicKey->Hdr.N0Inv
    );

  free(RSqrMod);
  return 0;
}

int main(int argc, char *argv[]) {
  unsigned int      Index;
  OC_RSA_PUBLIC_KEY *PublicKey;
  uint32_t          PkSize;

  for (Index = 1; Index < argc; ++Index) {
    PublicKey = (OC_RSA_PUBLIC_KEY *)readFile (argv[Index], &PkSize);
    if (PublicKey == NULL) {
      printf ("read error\n");
      return -1;
    }

    verifyRsa (PublicKey, argv[Index]);
    free (PublicKey);
  }

  for (Index = 0; Index < ARRAY_SIZE (PkDataBase); ++Index) {
    verifyRsa (PkDataBase[Index].PublicKey, "inbuilt");
  }

  return 0;
}
