/** @file
  Copyright (c) 2018, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "../Include/Uefi.h"

#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcAppleRamDiskLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <string.h>

#include <UserFile.h>

int ENTRY_POINT (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("Please provide a valid Disk Image path\n");
    return -1;
  }
  
  if ((argc % 2) != 1) {
    printf ("Please provide a chunklist file for each DMG, enter \'n\' to skip\n");
  }

  for (int i = 1; i < (argc - 1); i+=2) {
    int     DmgContextValid = 0;
    uint8_t  *Dmg = NULL;
    uint32_t DmgSize;

    uint8_t  *Chunklist = NULL;
    uint32_t ChunklistSize;

    uint8_t  *UncompDmg = NULL;
    uint32_t UncompSize;

    if ((Dmg = UserReadFile (argv[i], &DmgSize)) == NULL) {
      printf ("Read fail\n");
      goto ContinueDmgLoop;
    }

    BOOLEAN Result;

    OC_APPLE_DISK_IMAGE_CONTEXT DmgContext;
    APPLE_RAM_DISK_EXTENT_TABLE ExtentTable;

#define NUM_EXTENTS 20

    ExtentTable.Signature   = APPLE_RAM_DISK_EXTENT_SIGNATURE;
    ExtentTable.Version     = APPLE_RAM_DISK_EXTENT_VERSION;
    ExtentTable.Reserved    = 0;
    ExtentTable.Signature2  = APPLE_RAM_DISK_EXTENT_SIGNATURE;

    ExtentTable.ExtentCount = MIN (NUM_EXTENTS, ARRAY_SIZE (ExtentTable.Extents));

    UINT32 Index;
    for (Index = 0; Index < ExtentTable.ExtentCount; ++Index) {
      ExtentTable.Extents[Index].Start = (uintptr_t)Dmg + (Index * (DmgSize / ExtentTable.ExtentCount));
      ExtentTable.Extents[Index].Length = (DmgSize / ExtentTable.ExtentCount);
    }
    if (Index != 0) {
      ExtentTable.Extents[Index - 1].Length += (DmgSize - (Index * (DmgSize / ExtentTable.ExtentCount)));
    }

    Result = OcAppleDiskImageInitializeContext (&DmgContext, &ExtentTable, DmgSize);
    if (!Result) {
      printf ("DMG Context initialization error\n");
      goto ContinueDmgLoop;
    }

    DmgContextValid = 1;

    if (strcmp (argv[i + 1], "n") != 0) {
      if ((Chunklist = UserReadFile (argv[i + 1], &ChunklistSize)) == NULL) {
        printf ("Read fail\n");
        goto ContinueDmgLoop;
      }

      OC_APPLE_CHUNKLIST_CONTEXT ChunklistContext;
      Result = OcAppleChunklistInitializeContext (&ChunklistContext, Chunklist, ChunklistSize);
      if (!Result) {
        printf ("Chunklist Context initialization error\n");
        goto ContinueDmgLoop;
      }

      Result = OcAppleChunklistVerifySignature (&ChunklistContext, PkDataBase[0].PublicKey);
      if (!Result) {
        printf ("Chunklist signature verification error\n");
        goto ContinueDmgLoop;
      }

      Result = OcAppleDiskImageVerifyData (&DmgContext, &ChunklistContext);
      if (!Result) {
        printf ("Chunklist chunk verification error\n");
        goto ContinueDmgLoop;
      }
    }

    UncompSize = (DmgContext.SectorCount * APPLE_DISK_IMAGE_SECTOR_SIZE);
    UncompDmg  = malloc (UncompSize);
    if (UncompDmg == NULL) {
      printf ("DMG data allocation failed\n");
      goto ContinueDmgLoop;
    }

    Result = OcAppleDiskImageRead (&DmgContext, 0, UncompSize, UncompDmg);
    if (!Result) {
      printf ("DMG read error\n");
      goto ContinueDmgLoop;
    }

    printf ("Decompressed the entire DMG...\n");

#if 0
    FILE *Fh = fopen("out.bin", "wb");
    if (Fh != NULL) {
      fwrite (UncompDmg, UncompSize, 1, Fh);
      fclose (Fh);
    } else {
      printf("File error\n");
    }
#endif

#if 0
    unsigned char Digest[CC_MD5_DIGEST_LENGTH];
    CC_MD5_CTX Context;
    CC_MD5_Init (&Context);
    CC_MD5_Update (&Context, UncompDmg, UncompSize);
    CC_MD5_Final (Digest, &Context);
    for (size_t i = 0; i < CC_MD5_DIGEST_LENGTH; ++i)
      printf("%02x", (unsigned int) Digest[i]);
    puts("");
#endif

    printf ("Success...\n");

  ContinueDmgLoop:
    if (DmgContextValid) {
      OcAppleDiskImageFreeContext (&DmgContext);
    }

    free (Dmg);
    free (Chunklist);
    free (UncompDmg);
  }

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  #define MAX_INPUT 256
  #define MAX_OUTPUT 4096

  if (Size > MAX_INPUT) {
    return 0;
  }

  UINT8 *Test = AllocateZeroPool (MAX_OUTPUT);
  if (Test == NULL) {
    return 0;
  }

  for (size_t Index = 0; Index < 4096; ++Index) {
    ASAN_POISON_MEMORY_REGION (Test + Index, MAX_OUTPUT - Index);
    UINT32 CurrentLength = DecompressZLIB (
                           Test,
                           Index,
                           Data,
                           Size
                           );
    ASAN_UNPOISON_MEMORY_REGION (Test + Index, MAX_OUTPUT - Index);
    ASSERT (CurrentLength <= Index);
  }
  return 0;
}
