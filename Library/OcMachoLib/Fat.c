/** @file
  Copyright (C) 2020, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>
#include <IndustryStandard/AppleFatBinaryImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

EFI_STATUS
FatGetArchitectureOffset (
  IN  CONST UINT8       *Buffer,
  IN  UINT32            BufferSize,
  IN  UINT32            FullSize,
  IN  MACH_CPU_TYPE     CpuType,
  OUT UINT32            *FatOffset,
  OUT UINT32            *FatSize
  )
{
  BOOLEAN           SwapBytes;
  MACH_FAT_HEADER   *FatHeader;
  UINT32            NumberOfFatArch;
  UINT32            Offset;
  MACH_CPU_TYPE     TmpCpuType;
  UINT32            TmpSize;
  UINT32            Index;
  UINT32            Size;

  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);
  ASSERT (FullSize > 0);
  ASSERT (FatOffset != NULL);
  ASSERT (FatSize != NULL);

  if (BufferSize < sizeof (MACH_FAT_HEADER)
   || !OC_TYPE_ALIGNED (MACH_FAT_HEADER, Buffer)) {
    return EFI_INVALID_PARAMETER;
  }

  FatHeader = (MACH_FAT_HEADER*) Buffer;
  if (FatHeader->Signature != MACH_FAT_BINARY_INVERT_SIGNATURE
   && FatHeader->Signature != MACH_FAT_BINARY_SIGNATURE
   && FatHeader->Signature != EFI_FAT_BINARY_SIGNATURE) {
    //
    // Non-fat binary.
    //
    *FatOffset  = 0;
    *FatSize    = FullSize;
    return EFI_SUCCESS;
  }

  SwapBytes       = FatHeader->Signature == MACH_FAT_BINARY_INVERT_SIGNATURE;
  NumberOfFatArch = FatHeader->NumberOfFatArch;
  if (SwapBytes) {
    NumberOfFatArch = SwapBytes32 (NumberOfFatArch);
  }

  if (OcOverflowMulAddU32 (NumberOfFatArch, sizeof (MACH_FAT_ARCH), sizeof (MACH_FAT_HEADER), &TmpSize)
    || TmpSize > BufferSize) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: extend the interface to support subtypes (i.e. MachCpuSubtypeX8664H) some day.
  // 
  for (Index = 0; Index < NumberOfFatArch; ++Index) {
    TmpCpuType = FatHeader->FatArch[Index].CpuType;
    if (SwapBytes) {
      TmpCpuType = SwapBytes32 (TmpCpuType);
    }
    if (TmpCpuType == CpuType) {
      Offset = FatHeader->FatArch[Index].Offset;
      Size   = FatHeader->FatArch[Index].Size;
      if (SwapBytes) {
        Offset = SwapBytes32 (Offset);
        Size   = SwapBytes32 (Size);
      }

      if (Offset == 0
        || OcOverflowAddU32 (Offset, Size, &TmpSize)
        || TmpSize > FullSize) {
        return EFI_INVALID_PARAMETER;
      }

      *FatOffset  = Offset;
      *FatSize    = Size;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
FatFilterArchitectureByType (
  IN OUT UINT8         **FileData,
  IN OUT UINT32        *FileSize,
  IN     MACH_CPU_TYPE CpuType
  )
{
  EFI_STATUS    Status;
  UINT32        FatOffset;
  UINT32        FatSize;

  ASSERT (FileData != NULL);
  ASSERT (FileSize != NULL);
  ASSERT (*FileSize > 0);

  Status = FatGetArchitectureOffset (*FileData, *FileSize, *FileSize, CpuType, &FatOffset, &FatSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *FileData += FatOffset;
  *FileSize = FatSize;

  return EFI_SUCCESS;
}

EFI_STATUS
FatFilterArchitecture32 (
  IN OUT UINT8         **FileData,
  IN OUT UINT32        *FileSize
  )
{
  return FatFilterArchitectureByType (FileData, FileSize, MachCpuTypeX86);
}

EFI_STATUS
FatFilterArchitecture64 (
  IN OUT UINT8         **FileData,
  IN OUT UINT32        *FileSize
  )
{
  return FatFilterArchitectureByType (FileData, FileSize, MachCpuTypeX8664);
}
