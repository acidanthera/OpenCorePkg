/** @file
  Copyright (c) 2018, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "../Include/Uefi.h"

#include <Library/BaseMemoryLib.h>
#include <Library/BaseOverflowLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffLib2.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <stdio.h>
#include <string.h>

#include <UserFile.h>
#include <UserMemory.h>

STATIC UINT64  mHashesMask = MAX_UINT64;
STATIC UINTN   mHashIndex  = 0;
STATIC UINTN   mHashDependency;

STATIC
BOOLEAN
HashUpdate (
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  CONST UINT8  *D;

  D = (CONST UINT8 *)Data;

  (VOID)HashContext;

  BOOLEAN  P;

  for (UINTN i = 0; i < DataLength; i++) {
    mHashDependency += D[i];
  }

  if ((mHashesMask & (1ULL << mHashIndex)) != 0) {
    P = TRUE;
  } else {
    P = FALSE;
  }

  ++mHashIndex;
  mHashIndex &= 63U;

  return P;
}

STATIC
EFI_STATUS
PeCoffTestRtReloc (
  IN OUT  PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  )
{
  EFI_STATUS                      Status;
  PE_COFF_LOADER_RUNTIME_CONTEXT  *RtCtx;
  UINT32                          RtCtxSize;

  Status = PeCoffLoaderGetRuntimeContextSize (Context, &RtCtxSize);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  RtCtx = AllocatePool (RtCtxSize);
  if (RtCtx == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = PeCoffRelocateImage (Context, 0x69696969, RtCtx, RtCtxSize);
  if (EFI_ERROR (Status)) {
    FreePool (RtCtx);
    return Status;
  }

  Status = PeCoffRuntimeRelocateImage (Context->ImageBuffer, Context->SizeOfImage, 0x96969696, RtCtx);

  FreePool (RtCtx);

  return Status;
}

STATIC
EFI_STATUS
PeCoffTestLoad (
  IN OUT  PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  OUT  VOID                             *Destination,
  IN      UINT32                        DestinationSize
  )
{
  EFI_STATUS  Status;
  CHAR8       *PdbPath;
  UINT32      PdbPathSize;

  Status = PeCoffLoadImage (Context, Destination, DestinationSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeCoffGetPdbPath (Context, (CONST CHAR8 **)&PdbPath, &PdbPathSize);
  if (!EFI_ERROR (Status)) {
    ZeroMem (PdbPath, PdbPathSize);
  }

  if (!Context->RelocsStripped) {
    if (Context->Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      Status = PeCoffRelocateImage (Context, (UINTN)(Context->ImageBuffer), NULL, 0);
    } else {
      Status = PeCoffTestRtReloc (Context);
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  PeCoffDiscardSections (Context);

  return EFI_SUCCESS;
}

STATIC
VOID
LoadConfig (
  IN  CONST UINT8  *Data,
  IN  UINTN        Size
  )
{
  UINT32  ConfigSize;
  UINT32  LastByte;

  mHashDependency = 0;
  mHashIndex      = 0;

  ConfigSize = 0;

  if (Size - ConfigSize >= sizeof (UINT8)) {
    ConfigSize += sizeof (UINT8);
    LastByte    = Data[Size - ConfigSize];
  } else {
    LastByte = MAX_UINT8;
  }

  PcdGetBool (PcdImageLoaderRtRelocAllowTargetMismatch) = (LastByte & 1U) != 0;
  PcdGetBool (PcdImageLoaderHashProhibitOverlap)        = (LastByte & 2U) != 0;
  PcdGetBool (PcdImageLoaderLoadHeader)                 = (LastByte & 4U) != 0;
  PcdGetBool (PcdImageLoaderDebugSupport)               = (LastByte & 8U) != 0;
  PcdGetBool (PcdImageLoaderAllowMisalignedOffset)      = (LastByte & 16U) != 0;
  PcdGetBool (PcdImageLoaderRemoveXForWX)               = (LastByte & 32U) != 0;

  if (Size - ConfigSize >= sizeof (UINT32)) {
    ConfigSize += sizeof (UINT32);
    CopyMem (&LastByte, &Data[Size - ConfigSize], sizeof (UINT32));
  } else {
    LastByte = MAX_UINT32;
  }

  PcdGet32 (PcdImageLoaderAlignmentPolicy) = LastByte;

  if (Size - ConfigSize >= sizeof (UINT32)) {
    ConfigSize += sizeof (UINT32);
    CopyMem (&LastByte, &Data[Size - ConfigSize], sizeof (UINT32));
  } else {
    LastByte = MAX_UINT32;
  }

  PcdGet32 (PcdImageLoaderRelocTypePolicy) = LastByte;

  ConfigureMemoryAllocations (Data, Size, &ConfigSize);

  if (Size - ConfigSize >= sizeof (UINT64)) {
    ConfigSize += sizeof (UINT64);
    CopyMem (&mHashesMask, &Data[Size - ConfigSize], sizeof (UINT64));
  } else {
    mHashesMask = MAX_UINT64;
  }
}

STATIC
EFI_STATUS
PeCoffTestLoadFull (
  IN  CONST VOID  *FileBuffer,
  IN  UINT32      FileSize
  )
{
  EFI_STATUS                    Status;
  BOOLEAN                       Result;
  PE_COFF_LOADER_IMAGE_CONTEXT  Context;
  VOID                          *Destination;
  UINT32                        ImageSize;
  UINT32                        DestinationSize;
  UINT32                        DestinationPages;
  UINT32                        DestinationAlignment;
  UINT8                         HashContext;

  Status = PeCoffInitializeContext (&Context, FileBuffer, FileSize);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Result = PeCoffHashImageAuthenticode (
             &Context,
             &HashContext,
             HashUpdate
             );
  if (!Result) {
    return EFI_UNSUPPORTED;
  }

  ImageSize            = PeCoffGetSizeOfImage (&Context);
  DestinationPages     = EFI_SIZE_TO_PAGES (ImageSize);
  DestinationSize      = EFI_PAGES_TO_SIZE (DestinationPages);
  DestinationAlignment = PeCoffGetSectionAlignment (&Context);

  if (DestinationSize >= BASE_16MB) {
    return EFI_UNSUPPORTED;
  }

  Destination = AllocateAlignedCodePages (
                  DestinationPages,
                  DestinationAlignment
                  );
  if (Destination == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = PeCoffTestLoad (&Context, Destination, DestinationSize);

  FreeAlignedPages (Destination, DestinationPages);

  return Status;
}

int
LLVMFuzzerTestOneInput (
  const uint8_t  *Data,
  size_t         Size
  )
{
  VOID  *NewData;

  if (Size == 0) {
    return 0;
  }

  PcdGet8 (PcdDebugRaisePropertyMask) = 0;
  // PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_POOL | DEBUG_PAGE;
  // PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_POOL | DEBUG_PAGE;

  LoadConfig (Data, Size);

  NewData = AllocatePool (Size);
  if (NewData != NULL) {
    CopyMem (NewData, Data, Size);
    PeCoffTestLoadFull (NewData, Size);
    FreePool (NewData);
  }

  DEBUG ((
    DEBUG_POOL | DEBUG_PAGE,
    "UMEM: Allocated %u pools %u pages\n",
    (UINT32)mPoolAllocations,
    (UINT32)mPageAllocations
    ));

  return 0;
}

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  UINT8       *Image;
  UINT32      ImageSize;
  EFI_STATUS  Status;

  if (argc < 2) {
    DEBUG ((DEBUG_ERROR, "Please provide a valid PE image path\n"));
    return -1;
  }

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  // PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_POOL | DEBUG_PAGE;
  // PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_POOL | DEBUG_PAGE;

  if ((Image = UserReadFile (argv[1], &ImageSize)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail\n"));
    return 1;
  }

  Status = LLVMFuzzerTestOneInput (Image, ImageSize);
  FreePool (Image);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  return 0;
}
