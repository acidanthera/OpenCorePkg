/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>

INT32
FindPattern (
  IN CONST UINT8   *Pattern,
  IN CONST UINT8   *PatternMask OPTIONAL,
  IN CONST UINT32  PatternSize,
  IN CONST UINT8   *Data,
  IN UINT32        DataSize,
  IN INT32         DataOff
  )
{
  BOOLEAN  Matches;
  UINT32   Index;

  ASSERT (DataOff >= 0);

  if (PatternSize == 0 || DataSize == 0 || (DataOff < 0) || (UINT32)DataOff >= DataSize || DataSize - DataOff < PatternSize) {
    return -1;
  }

  while (DataOff + PatternSize < DataSize) {
    Matches = TRUE;
    for (Index = 0; Index < PatternSize; ++Index) {
      if ((PatternMask == NULL && Data[DataOff + Index] != Pattern[Index])
      || (PatternMask != NULL && (Data[DataOff + Index] & PatternMask[Index]) != Pattern[Index])) {
        Matches = FALSE;
        break;
      }
    }

    if (Matches) {
      return DataOff;
    }
    ++DataOff;
  }

  return -1;
}

UINT32
ApplyPatch (
  IN CONST UINT8   *Pattern,
  IN CONST UINT8   *PatternMask OPTIONAL,
  IN CONST UINT32  PatternSize,
  IN CONST UINT8   *Replace,
  IN CONST UINT8   *ReplaceMask OPTIONAL,
  IN UINT8         *Data,
  IN UINT32        DataSize,
  IN UINT32        Count,
  IN UINT32        Skip
  )
{
  UINT32  ReplaceCount;
  INT32   DataOff;

  ReplaceCount = 0;
  DataOff = 0;

  do {
    DataOff = FindPattern (Pattern, PatternMask, PatternSize, Data, DataSize, DataOff);

    if (DataOff >= 0) {
      //
      // Skip this finding if requested.
      //
      if (Skip > 0) {
        --Skip;
        DataOff += PatternSize;
        continue;
      }

      //
      // Perform replacement.
      //
      if (ReplaceMask == NULL) {
        CopyMem (&Data[DataOff], Replace, PatternSize);
      } else {
        for (UINTN Index = 0; Index < PatternSize; ++Index) {
          Data[DataOff + Index] = (Data[DataOff + Index] & ~ReplaceMask[Index]) | (Replace[Index] & ReplaceMask[Index]);
        }
      }
      ++ReplaceCount;
      DataOff += PatternSize;

      //
      // Check replace count if requested.
      //
      if (Count > 0) {
        --Count;
        if (Count == 0) {
          break;
        }
      }
    }

  } while (DataOff >= 0);

  return ReplaceCount;
}
