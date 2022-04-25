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

#include <UserFile.h>

#include <Library/OcCryptoLib.h>
#include <Library/OcAppleKeysLib.h>

#include <BigNumLib.h>

VOID
VerifyRsa (
  IN CONST OC_RSA_PUBLIC_KEY  *PublicKey,
  IN CONST CHAR8              *Name
  )
{
  OC_BN_WORD  N0Inv;
  UINTN       ModulusSize;
  OC_BN_WORD  *RSqrMod;
  OC_BN_WORD  *Scratch;

  ModulusSize = PublicKey->Hdr.NumQwords * sizeof (UINT64);
  RSqrMod     = AllocatePool (ModulusSize);
  Scratch     = AllocatePool (BIG_NUM_MONT_PARAMS_SCRATCH_SIZE (ModulusSize / OC_BN_WORD_SIZE));

  if ((RSqrMod == NULL) || (Scratch == NULL)) {
    DEBUG ((DEBUG_ERROR, "memory allocation error!\n"));
    FreePool (RSqrMod);
    FreePool (Scratch);
  }

  N0Inv = BigNumCalculateMontParams (
            RSqrMod,
            ModulusSize / OC_BN_WORD_SIZE,
            (CONST OC_BN_WORD *)PublicKey->Data,
            Scratch
            );

  DEBUG ((
    DEBUG_ERROR,
    "%a: results: %d %d (%LX vs %LX)\n",
    Name,
    CompareMem (
      RSqrMod,
      &PublicKey->Data[PublicKey->Hdr.NumQwords],
      ModulusSize
      ),
    N0Inv != PublicKey->Hdr.N0Inv,
    (UINT64)N0Inv,
    (UINT64)PublicKey->Hdr.N0Inv
    ));

  FreePool (Scratch);
  FreePool (RSqrMod);
}

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  int                Index;
  OC_RSA_PUBLIC_KEY  *PublicKey;
  UINT32             PkSize;

  for (Index = 1; Index < argc; ++Index) {
    PublicKey = (OC_RSA_PUBLIC_KEY *)UserReadFile (argv[Index], &PkSize);
    if (PublicKey == NULL) {
      DEBUG ((DEBUG_ERROR, "read error\n"));
      return -1;
    }

    VerifyRsa (PublicKey, argv[Index]);
    FreePool (PublicKey);
  }

  for (Index = 0; (UINTN)Index < ARRAY_SIZE (PkDataBase); ++Index) {
    VerifyRsa (PkDataBase[Index].PublicKey, "inbuilt");
  }

  return 0;
}
