/** @file
  Copyright (c) 2018, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcAppleRamDiskLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <UserFile.h>

#define  NUM_EXTENTS  20

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  int                          Index;
  BOOLEAN                      DmgContextValid;
  UINT8                        *Dmg;
  UINT32                       DmgSize;
  UINT8                        *Chunklist;
  UINT32                       ChunklistSize;
  UINT8                        *UncompDmg;
  UINT32                       UncompSize;
  BOOLEAN                      Result;
  OC_APPLE_DISK_IMAGE_CONTEXT  DmgContext;
  APPLE_RAM_DISK_EXTENT_TABLE  ExtentTable;
  UINT32                       Index2;
  OC_APPLE_CHUNKLIST_CONTEXT   ChunklistContext;

  if (argc < 2) {
    DEBUG ((DEBUG_ERROR, "Please provide a valid Disk Image path\n"));
    return -1;
  }

  if ((argc % 2) != 1) {
    DEBUG ((DEBUG_ERROR, "Please provide a chunklist file for each DMG, enter \'n\' to skip\n"));
  }

  for (Index = 1; Index < (argc - 1); Index += 2) {
    DmgContextValid = FALSE;
    Dmg             = NULL;
    Chunklist       = NULL;
    UncompDmg       = NULL;

    if ((Dmg = UserReadFile (argv[Index], &DmgSize)) == NULL) {
      DEBUG ((DEBUG_ERROR, "Read fail\n"));
      goto ContinueDmgLoop;
    }

    ExtentTable.Signature   = APPLE_RAM_DISK_EXTENT_SIGNATURE;
    ExtentTable.Version     = APPLE_RAM_DISK_EXTENT_VERSION;
    ExtentTable.Reserved    = 0;
    ExtentTable.Signature2  = APPLE_RAM_DISK_EXTENT_SIGNATURE;
    ExtentTable.ExtentCount = MIN (NUM_EXTENTS, ARRAY_SIZE (ExtentTable.Extents));

    for (Index2 = 0; Index2 < ExtentTable.ExtentCount; ++Index2) {
      ExtentTable.Extents[Index2].Start  = (UINTN)Dmg + (Index2 * (DmgSize / ExtentTable.ExtentCount));
      ExtentTable.Extents[Index2].Length = (DmgSize / ExtentTable.ExtentCount);
    }

    if (Index2 != 0) {
      ExtentTable.Extents[Index2 - 1].Length += (DmgSize - (Index2 * (DmgSize / ExtentTable.ExtentCount)));
    }

    Result = OcAppleDiskImageInitializeContext (&DmgContext, &ExtentTable, DmgSize);
    if (!Result) {
      DEBUG ((DEBUG_ERROR, "DMG Context initialization error\n"));
      goto ContinueDmgLoop;
    }

    DmgContextValid = TRUE;

    if (AsciiStrCmp (argv[Index + 1], "n") != 0) {
      if ((Chunklist = UserReadFile (argv[Index + 1], &ChunklistSize)) == NULL) {
        DEBUG ((DEBUG_ERROR, "Read fail\n"));
        goto ContinueDmgLoop;
      }

      Result = OcAppleChunklistInitializeContext (&ChunklistContext, Chunklist, ChunklistSize);
      if (!Result) {
        DEBUG ((DEBUG_ERROR, "Chunklist Context initialization error\n"));
        goto ContinueDmgLoop;
      }

      Result = OcAppleChunklistVerifySignature (&ChunklistContext, PkDataBase[0].PublicKey);
      if (!Result) {
        DEBUG ((DEBUG_ERROR, "Chunklist signature verification error\n"));
        goto ContinueDmgLoop;
      }

      Result = OcAppleDiskImageVerifyData (&DmgContext, &ChunklistContext);
      if (!Result) {
        DEBUG ((DEBUG_ERROR, "Chunklist chunk verification error\n"));
        goto ContinueDmgLoop;
      }
    }

    UncompSize = (DmgContext.SectorCount * APPLE_DISK_IMAGE_SECTOR_SIZE);
    UncompDmg  = AllocatePool (UncompSize);
    if (UncompDmg == NULL) {
      DEBUG ((DEBUG_ERROR, "DMG data allocation failed\n"));
      goto ContinueDmgLoop;
    }

    Result = OcAppleDiskImageRead (&DmgContext, 0, UncompSize, UncompDmg);
    if (!Result) {
      DEBUG ((DEBUG_ERROR, "DMG read error\n"));
      goto ContinueDmgLoop;
    }

    DEBUG ((DEBUG_ERROR, "Decompressed the entire DMG...\n"));

 #if 0
    UserWriteFile ("out.bin", UncompDmg, UncompSize);
 #endif

 #if 0
    UINT8       Digest[CC_MD5_DIGEST_LENGTH];
    CC_MD5_CTX  Context;
    UINTN       Index3;

    CC_MD5_Init (&Context);
    CC_MD5_Update (&Context, UncompDmg, UncompSize);
    CC_MD5_Final (Digest, &Context);
    for (Index3 = 0; Index3 < CC_MD5_DIGEST_LENGTH; ++Index3) {
      DEBUG ((DEBUG_ERROR, "%02x", (UINT32)Digest[Index3]));
    }

    DEBUG ((DEBUG_ERROR, "\n"));
 #endif

    DEBUG ((DEBUG_ERROR, "Success...\n"));

ContinueDmgLoop:
    if (DmgContextValid) {
      OcAppleDiskImageFreeContext (&DmgContext);
    }

    FreePool (Dmg);
    FreePool (Chunklist);
    FreePool (UncompDmg);
  }

  return 0;
}

int
LLVMFuzzerTestOneInput (
  const uint8_t  *Data,
  size_t         Size
  )
{
  #define  MAX_INPUT   256
  #define  MAX_OUTPUT  4096

  UINT8   *Test;
  UINTN   Index;
  UINT32  CurrentLength;

  if (Size > MAX_INPUT) {
    return 0;
  }

  Test = AllocateZeroPool (MAX_OUTPUT);
  if (Test == NULL) {
    return 0;
  }

  for (Index = 0; Index < 4096; ++Index) {
    ASAN_POISON_MEMORY_REGION (Test + Index, MAX_OUTPUT - Index);
    CurrentLength = DecompressZLIB (
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
