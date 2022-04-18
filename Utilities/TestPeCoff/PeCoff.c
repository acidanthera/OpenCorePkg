/** @file
  Copyright (c) 2018, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "../Include/Uefi.h"

#include <Library/OcPeCoffLib.h>
#include <Library/OcGuardLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <stdio.h>
#include <string.h>

#include <UserFile.h>
#include <UserMemory.h>

STATIC UINT64 mHashesMask = MAX_UINT64;
STATIC UINTN mHashIndex = 0;
STATIC UINTN mHashDependency;

BOOLEAN
HashUpdate (
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  CONST UINT8 *D = (CONST UINT8 *)Data;

  (VOID) HashContext;

  BOOLEAN p;

  for (UINTN i = 0; i < DataLength; i++)
    mHashDependency += D[i];

  if ((mHashesMask & (1ULL << mHashIndex)) != 0) {
    p = TRUE;
  } else {
    p = FALSE;
  }

  ++mHashIndex;
  mHashIndex &= 63U;

  return p;
}

STATIC
RETURN_STATUS
PeCoffTestRtReloc (
  PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  RETURN_STATUS                   Status;
  PE_COFF_RUNTIME_CONTEXT *RtCtx;
  UINT32                         RtCtxSize;

  Status = PeCoffRelocationDataSize (Context, &RtCtxSize);

  if (Status != RETURN_SUCCESS) {
    return RETURN_UNSUPPORTED;
  }

  RtCtx = AllocatePool (RtCtxSize);

  if (RtCtx == NULL) {
    return RETURN_UNSUPPORTED;
  }

  Status = PeCoffRelocateImage (Context, 0x69696969, RtCtx, RtCtxSize);

  if (Status != RETURN_SUCCESS) {
    FreePool (RtCtx);
    return Status;
  }

  Status = PeCoffRelocateImageForRuntime (Context->ImageBuffer, Context->SizeOfImage, 0x96969696, RtCtx);
  
  FreePool (RtCtx);

  return Status;
}

STATIC
RETURN_STATUS
PeCoffTestLoad (
  PE_COFF_IMAGE_CONTEXT *Context,
  VOID                  *Destination,
  UINT32                DestinationSize
  )
{
  RETURN_STATUS  Status;
  CHAR8         *PdbPath;
  UINT32        PdbPathSize;

  (VOID) PeCoffLoadImage (Context, Destination, DestinationSize);

  Status = PeCoffGetPdbPath (Context, &PdbPath, &PdbPathSize);

  if (Status == RETURN_SUCCESS) {
    ZeroMem (PdbPath, PdbPathSize);
  }

  if (!Context->RelocsStripped) {
    if (Context->Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      Status = PeCoffRelocateImage (Context, (UINTN) (Context->ImageBuffer), NULL, 0);
    } else {
      Status = PeCoffTestRtReloc (Context);
    }
  }

  if (Status != RETURN_SUCCESS) {
    return Status;
  }

  PeCoffDiscardSections (Context);

  return RETURN_SUCCESS;
}

static void loadConfig(const uint8_t *data, size_t size) {
  mHashDependency = 0;
  mHashIndex = 0;
  UINT32 Off = sizeof(UINT8);
  UINT32 LastByte = data[size - Off];
  PcdGetBool(PcdImageLoaderRtRelocAllowTargetMismatch) = (LastByte & 1U) != 0;
  PcdGetBool(PcdImageLoaderHashProhibitOverlap) = (LastByte & 2U) != 0;
  PcdGetBool(PcdImageLoaderLoadHeader) = (LastByte & 4U) != 0;
  PcdGetBool(PcdImageLoaderSupportArmThumb) = (LastByte & 8U) != 0;
  PcdGetBool(PcdImageLoaderForceLoadDebug) = (LastByte & 16U) != 0;
  PcdGetBool(PcdImageLoaderTolerantLoad) = (LastByte & 32U) != 0;
  PcdGetBool(PcdImageLoaderSupportDebug) = (LastByte & 64U) != 0;
  Off += sizeof(UINT64);
  if (size >= Off)
    memcpy(&mPoolAllocationMask, &data[size - Off], sizeof(UINT64));
  else
    mPoolAllocationMask = MAX_UINT64;
  Off += sizeof(UINT64);
  if (size >= Off)
    memcpy(&mPageAllocationMask, &data[size - Off], sizeof(UINT64));
  else
    mPageAllocationMask = MAX_UINT64;
  Off += sizeof(UINT64);
  if (size >= Off)
    memcpy(&mHashesMask, &data[size - Off], sizeof(UINT64));
  else
    mHashesMask = MAX_UINT64;
}

RETURN_STATUS
PeCoffTestLoadFull (
  IN VOID    *FileBuffer,
  IN UINT32  FileSize
  )
{
  RETURN_STATUS         Status;
  BOOLEAN               Result;
  PE_COFF_IMAGE_CONTEXT Context;
  VOID                  *Destination;
  UINT32                DestinationSize;

  Status = PeCoffInitializeContext (&Context, FileBuffer, FileSize);

  if (Status != RETURN_SUCCESS) {
    return RETURN_UNSUPPORTED;
  }

  UINT8 HashContext;
  Result = PeCoffHashImage (
             &Context,
             HashUpdate,
             &HashContext
             );

  if (!Result) {
    return RETURN_UNSUPPORTED;
  }

  DestinationSize = Context.SizeOfImage + Context.SizeOfImageDebugAdd;

  if (OcOverflowAddU32 (DestinationSize, Context.SectionAlignment, &DestinationSize)) {
    return RETURN_UNSUPPORTED;
  }

  if (DestinationSize >= BASE_16MB) {
    return RETURN_UNSUPPORTED;
  }

  Destination = AllocatePages (EFI_SIZE_TO_PAGES (DestinationSize));
  if (Destination == NULL) {
    return RETURN_UNSUPPORTED;
  }

  Status = PeCoffTestLoad (&Context, Destination, DestinationSize);

  FreePages (Destination, EFI_SIZE_TO_PAGES (DestinationSize));

  return Status;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size == 0)
    return 0;

  //PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_POOL | DEBUG_PAGE;
  //PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_POOL | DEBUG_PAGE;

  loadConfig(Data, Size);

  void *p = AllocatePool(Size);
  if (p != NULL) {
    memcpy(p, Data, Size);
    PeCoffTestLoadFull(p, Size);
    FreePool(p);
  }

  DEBUG ((
    DEBUG_POOL | DEBUG_PAGE,
    "UMEM: Allocated %u pools %u pages\n",
    (UINT32) mPoolAllocations,
    (UINT32) mPageAllocations
    ));

  return 0;
}

int ENTRY_POINT (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("Please provide a valid PE image path\n");
    return -1;
  }

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  //PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_POOL | DEBUG_PAGE;
  //PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_POOL | DEBUG_PAGE;

  uint8_t *Image;
  uint32_t ImageSize;

  if ((Image = UserReadFile (argv[1], &ImageSize)) == NULL) {
    printf ("Read fail\n");
    return 1;
  }

  EFI_STATUS Status = LLVMFuzzerTestOneInput (Image, ImageSize);
  FreePool(Image);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  return 0;
}
