/** @file
  Mkext support.

  Copyright (c) 2020, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>

//
// Pick a reasonable maximum to fit.
//
#define MKEXT_HEADER_SIZE (EFI_PAGE_SIZE * 2)

EFI_STATUS
ReadAppleMkext (
  IN     EFI_FILE_PROTOCOL  *File,
  IN     MACH_CPU_TYPE      CpuType,
     OUT UINT8              **Mkext,
     OUT UINT32             *MkextSize,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize,
  IN     UINT32             NumReservedKexts
  )
{
  EFI_STATUS        Status;
  UINT32            Offset;
  UINT8             *TmpMkext;
  UINT32            TmpMkextSize;
  UINT32            TmpMkextFileSize;

  ASSERT (File != NULL);
  ASSERT (Mkext != NULL);
  ASSERT (MkextSize != NULL);
  ASSERT (AllocatedSize != NULL);

  //
  // Read enough to get fat binary header if present.
  //
  TmpMkextSize = MKEXT_HEADER_SIZE;
  Status = GetFileSize (File, &TmpMkextFileSize);
  if (TmpMkextSize >= TmpMkextFileSize) {
    return EFI_INVALID_PARAMETER;
  }

  TmpMkext = AllocatePool (TmpMkextSize);
  if (TmpMkext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetFileData (File, 0, TmpMkextSize, TmpMkext);
  if (EFI_ERROR (Status)) {
    FreePool (TmpMkext);
    return Status;
  }
  Status = FatGetArchitectureOffset (TmpMkext, TmpMkextSize, TmpMkextFileSize, CpuType, &Offset, &TmpMkextSize);
  FreePool (TmpMkext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read target slice of mkext.
  //
  TmpMkext = AllocatePool (TmpMkextSize);
  if (TmpMkext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = GetFileData (File, Offset, TmpMkextSize, TmpMkext);
  if (EFI_ERROR (Status)) {
    FreePool (TmpMkext);
    return Status;
  }

  //
  // Verify mkext arch.
  //
  if (!MkextCheckCpuType (TmpMkext, TmpMkextSize, CpuType)) {
    FreePool (TmpMkext);
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_INFO, "OCAK: Got mkext of %u bytes with 0x%X arch\n", TmpMkextSize, CpuType));

  //
  // Calculate size of decompressed mkext.
  //
  *AllocatedSize = 0;
  Status = MkextDecompress (TmpMkext, TmpMkextSize, NumReservedKexts, NULL, 0, AllocatedSize);
  if (EFI_ERROR (Status)) {
    FreePool (TmpMkext);
    return Status;
  }
  if (OcOverflowAddU32 (*AllocatedSize, ReservedSize, AllocatedSize)) {
    FreePool (TmpMkext);
    return EFI_INVALID_PARAMETER;
  }

  *Mkext = AllocatePool (*AllocatedSize);
  if (*Mkext == NULL) {
    FreePool (TmpMkext);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Decompress mkext into final buffer.
  //
  Status = MkextDecompress (TmpMkext, TmpMkextSize, NumReservedKexts, *Mkext, *AllocatedSize, MkextSize);
  FreePool (TmpMkext);

  if (EFI_ERROR (Status)) {
    FreePool (*Mkext);
  }

  return Status;
}
