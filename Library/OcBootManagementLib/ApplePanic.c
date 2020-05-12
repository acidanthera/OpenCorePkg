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

#include <Guid/AppleVariable.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcRtcLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
VOID *
PanicUnpack (
  IN  CONST VOID   *Packed,
  IN  UINTN        PackedSize,
  OUT UINTN        *UnpackedSize
  )
{
  VOID          *Unpacked;
  CONST UINT8   *PackedWalker;
  UINT8         *UnpackedWalker;
  UINT64        Sequence;

  if (OcOverflowMulUN (PackedSize, 8, UnpackedSize)) {
    return NULL;
  }

  //
  // Source buffer is required to be 8-byte aligned by Apple,
  // so we can freely truncate here. 
  //
  *UnpackedSize /= 7;
  *UnpackedSize = *UnpackedSize - (*UnpackedSize % 8);
  if (*UnpackedSize == 0 || OcOverflowAddUN (*UnpackedSize, 1, UnpackedSize)) {
    return NULL;
  }

  Unpacked = AllocatePool (*UnpackedSize);
  if (Unpacked == NULL) {
    return NULL;
  }

  Sequence = 0;
  PackedWalker = Packed;
  UnpackedWalker = Unpacked;

  while (PackedSize >= 7) {
    CopyMem (&Sequence, PackedWalker, 7);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 0, 7 * 0 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 1, 7 * 1 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 2, 7 * 2 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 3, 7 * 3 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 4, 7 * 4 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 5, 7 * 5 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 6, 7 * 6 + 6);
    *UnpackedWalker++ = (UINT8) BitFieldRead64 (Sequence, 7 * 7, 7 * 7 + 6);
    PackedWalker += 7;
    PackedSize   -= 7;
  }

  //
  // Ensure null-termination.
  //
  *UnpackedWalker++ = '\0';

  return Unpacked;
}

STATIC
BOOLEAN
PanicExpandPutBuf (
  IN OUT CHAR8        **Buffer,
  IN OUT UINTN        *AllocatedSize,
  IN OUT UINTN        *CurrentSize,
  IN     CONST CHAR8  *NewData,
  IN     UINTN        NewDataSize
  )
{
  CHAR8  *TmpBuffer;
  UINTN  NewSize;

  if (*AllocatedSize - *CurrentSize <= NewDataSize) {
    if (*AllocatedSize > 0) {
      NewSize = *AllocatedSize;
    } else {
      NewSize = NewDataSize;
    }

    NewSize = MAX (NewSize, 8192);

    if (OcOverflowMulUN (NewSize, 2, &NewSize)) {
      if (*Buffer != NULL) {
        FreePool (*Buffer);
      }
      return FALSE;
    }

    TmpBuffer = ReallocatePool (*AllocatedSize, NewSize, *Buffer);
    if (TmpBuffer == NULL) {
      if (*Buffer != NULL) {
        FreePool (*Buffer);
      }
      return FALSE;
    }

    *Buffer = TmpBuffer;
    *AllocatedSize = NewSize;
  }

  CopyMem (*Buffer + *CurrentSize, NewData, NewDataSize);
  *CurrentSize += NewDataSize;
  return TRUE;
}

STATIC
CHAR8 *
PanicExpand (
  IN  CONST CHAR8  *Encoded,
  IN  UINTN        EncodedSize,
  OUT UINTN        *ExpandedSize
  )
{
  CHAR8    *Expanded;
  UINTN    AllocatedSize;
  UINTN    CurrentSize;
  UINTN    TmpSize;
  CHAR8    *EncodedStart;
  BOOLEAN  Success;

  EncodedStart = AsciiStrStr (Encoded, "loaded kext");
  if (EncodedStart == NULL) {
    return NULL; ///< Not encoded.
  }

  Expanded         = NULL;
  AllocatedSize    = 0;
  CurrentSize      = 0;

  TmpSize = EncodedStart - EncodedStart;

  if (!PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, Encoded, TmpSize)) {
    return NULL;
  }

  Encoded     += TmpSize;
  EncodedSize -= TmpSize;

  while (EncodedSize > 0) {
    TmpSize = 1;

    switch (Encoded[0]) {
      case '>':
        Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "com.apple.driver.", L_STR_LEN ("com.apple.driver."));
        break;
      case '|':
        Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "com.apple.iokit.", L_STR_LEN ("com.apple.iokit."));
        break;
      case '$':
        Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "com.apple.security.", L_STR_LEN ("com.apple.security."));
        break;
      case '@':
        Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "com.apple.", L_STR_LEN ("com.apple."));
        break;
      case '!':
        if (EncodedSize >= 2) {
          ++TmpSize;
          switch (Encoded[1]) {
            case 'U':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "AppleUSB", L_STR_LEN ("AppleUSB"));
              break;
            case 'A':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "Apple", L_STR_LEN ("Apple"));
              break;
            case 'F':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "Family", L_STR_LEN ("Family"));
              break;
            case 'S':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "Storage", L_STR_LEN ("Storage"));
              break;
            case 'C':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "Controller", L_STR_LEN ("Controller"));
              break;
            case 'B':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "Bluetooth", L_STR_LEN ("Bluetooth"));
              break;
            case 'I':
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, "Intel", L_STR_LEN ("Intel"));
              break;
            default:
              Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, Encoded, 2);
              break;
          }
          break;
        }
        /* Fallthrough */
      default:
        Success = PanicExpandPutBuf (&Expanded, &AllocatedSize, &CurrentSize, Encoded, 1);
        break;
    }

    if (!Success) {
      return NULL;
    }

    Encoded     += TmpSize;
    EncodedSize -= TmpSize;
  }

  *ExpandedSize = CurrentSize;

  //
  // One extra byte is always guaranteed.
  //
  Expanded[CurrentSize] = '\0';

  return Expanded;
}

VOID *
OcReadApplePanicLog (
  OUT UINT32       *PanicSize
  )
{
  EFI_STATUS   Status;
  UINT32       Index;
  VOID         *Value;
  UINTN        ValueSize;
  CHAR16       VariableName[32];
  VOID         *TmpData;
  UINTN        TmpDataSize;
  VOID         *PanicData;

  TmpData     = NULL;
  TmpDataSize = 0;

  for (Index = 0; Index < 1024; ++Index) {
    UnicodeSPrint (
      VariableName,
      sizeof (VariableName),
      APPLE_PANIC_INFO_NO_VARIABLE_NAME,
      Index
      );
    Status = GetVariable2 (VariableName, &gAppleBootVariableGuid, &Value, &ValueSize);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (ValueSize == 0) {
      FreePool (Value);
      break;
    }

    PanicData = ReallocatePool (TmpDataSize, TmpDataSize + ValueSize, TmpData);
    if (PanicData == NULL) {
      FreePool (Value);
      if (TmpData != NULL) {
        FreePool (TmpData);
      }
      return NULL;
    }
    TmpData = PanicData;

    CopyMem (
      (UINT8 *) TmpData + TmpDataSize,
      Value,
      ValueSize
      );

    FreePool (Value);
    TmpDataSize += ValueSize;
  }

  if (Index == 0) {
    //
    // We have not advanced past the first variable, there is no panic log.
    //
    return NULL;
  }

  //
  // We have a kernel panic now.
  //
  ASSERT (TmpData != NULL);
  ASSERT (TmpDataSize > 0);

  PanicData = PanicUnpack (TmpData, TmpDataSize, &TmpDataSize);
  FreePool (TmpData);

  if (PanicData != NULL) {
    //
    // Truncate trailing zeroes.
    //
    TmpDataSize = AsciiStrLen (PanicData);
    TmpData = PanicExpand (PanicData, TmpDataSize, &TmpDataSize);
    if (TmpData != NULL) {
      FreePool (PanicData);
      PanicData = TmpData;
    }
  }

  *PanicSize = (UINT32) TmpDataSize;

  return TmpData;
}
