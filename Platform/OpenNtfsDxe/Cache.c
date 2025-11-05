/** @file
  Copyright (c) 2025, Pavel Naberezhnev. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "NTFS.h"
#include "Helper.h"

#define FNVHASH_PRIME   (0x100000001B3ULL)
#define FNVHASH_OFFSET  (0xCBF29CE484222325ULL)

STATIC
UINT64
FnvHash (
  IN CONST CHAR16  *Str
  )
{
  UINT64  Hash;
  UINT64  Char;

  ASSERT (Str != NULL);

  Hash = FNVHASH_OFFSET;
  while ((Char = (UINT64)*Str) != 0) {
    Hash = Hash ^ (Char & 0x0FF);
    Hash = Hash * FNVHASH_PRIME;

    Char = Char >> 8;
    Hash = Hash ^ (Char & 0x0FF);
    Hash = Hash * FNVHASH_PRIME;

    ++Str;
  }

  return Hash;
}

EFI_STATUS
NtfsCfiInit (
  IN EFI_FS  *Fs
  )
{
  ASSERT (Fs != NULL);

  Fs->CFIData = AllocateZeroPool (MINIMUM_INFO_LENGTH);
  if (Fs->CFIData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set init state
  //
  Fs->CFIDataSize = 0;

  return EFI_SUCCESS;
}

VOID
NtfsCfiFree (
  IN EFI_FS  *Fs
  )
{
  ASSERT (Fs != NULL);

  if (Fs->CFIData != NULL) {
    FreePool (Fs->CFIData);
    Fs->CFIData = NULL;
  }
}

VOID
NtfsCfiPush (
  IN EFI_FS        *Fs,
  IN CONST CHAR16  *Path,
  IN UINTN         Size,
  IN INT64         DirIndex
  )
{
  ASSERT (Fs != NULL);
  ASSERT (Path != NULL);

  Fs->CFIHash     = FnvHash (Path);
  Fs->CFIDataSize = Size;
  Fs->CFIDirIndex = DirIndex;
}

INTN
NtfsCfiPop (
  IN EFI_FS        *Fs,
  IN CONST CHAR16  *Path,
  IN UINTN         Size,
  IN INT64         DirIndex,
  OUT VOID         *Buffer
  )
{
  UINT64  Hash;

  ASSERT (Fs != NULL);
  ASSERT (Path != NULL);

  if (Fs->CFIDataSize == 0) {
    return 0;
  }

  Hash = FnvHash (Path);
  if ((Hash == Fs->CFIHash) && (DirIndex == Fs->CFIDirIndex)) {
    if (Size < Fs->CFIDataSize) {
      return -1;
    }

    CopyMem (Buffer, Fs->CFIData, Fs->CFIDataSize);
    ZeroMem (Fs->CFIData, Fs->CFIDataSize);
    Fs->CFIDataSize = 0;
    return 1;
  }

  ZeroMem (Fs->CFIData, Fs->CFIDataSize);
  Fs->CFIDataSize = 0;
  return 0;
}
