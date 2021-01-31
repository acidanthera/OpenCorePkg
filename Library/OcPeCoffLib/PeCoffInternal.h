/** @file
  Implements APIs to load PE/COFF Debug information.

  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef PE_COFF_INTERNAL_H
#define PE_COFF_INTERNAL_H

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/OcPeCoffLib.h>
#include <Library/OcGuardLib.h>

#define IS_ALIGNED(v, a)  (((v) & ((a) - 1U)) == 0U)
#define IS_POW2(v)        ((v) != 0 && ((v) & ((v) - 1U)) == 0)

#define PTR_TO_ADDR(Ptr, Size) ((UINTN) Ptr)

#define COMPOSE_32(High, Low)  \
    ((UINT32) ((UINT32) (Low) + ((UINT32) (High) * 65536U)))

#define READ_ALIGNED_16(x)     (*(CONST UINT16 *) (CONST VOID *) (x))
#define WRITE_ALIGNED_16(x, y) do { *(UINT16 *) (VOID *) (x) = (y); } while (FALSE)
#define READ_ALIGNED_32(x)     (*(CONST UINT32 *) (CONST VOID *) (x))
#define WRITE_ALIGNED_32(x, y) do { *(UINT32 *) (VOID *) (x) = (y); } while (FALSE)
#define READ_ALIGNED_64(x)     (*(CONST UINT64 *) (CONST VOID *) (x))
#define WRITE_ALIGNED_64(x, y) do { *(UINT64 *) (VOID *) (x) = (y); } while (FALSE)

#define BaseOverflowSubU16 OcOverflowSubU16
#define BaseOverflowAddU32 OcOverflowAddU32
#define BaseOverflowSubU32 OcOverflowSubU32
#define BaseOverflowMulU32 OcOverflowMulU32
#define BaseOverflowAlignUpU32 OcOverflowAlignUpU32

/**
  Returns the type of a Base Relocation.

  @param[in] Relocation  The composite Base Relocation value.
**/
#define IMAGE_RELOC_TYPE(Relocation)    ((Relocation) >> 12U)

/**
  Returns the target offset of a Base Relocation.

  @param[in] Relocation  The composite Base Relocation value.
**/
#define IMAGE_RELOC_OFFSET(Relocation)  ((Relocation) & 0x0FFFU)

/**
  Returns whether the Image targets the UEFI Subsystem.

  @param[in] Subsystem  The Subsystem value from the Image Header.
**/
#define IMAGE_IS_EFI_SUBYSYSTEM(Subsystem) \
  ((Subsystem) >= EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION && \
   (Subsystem) <= EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER)


#define IMAGE_RELOC_TYPE_SUPPORTED(Type) \
  ((Type) == EFI_IMAGE_REL_BASED_ABSOLUTE) || \
  ((Type) == EFI_IMAGE_REL_BASED_HIGHLOW) || \
  ((Type) == EFI_IMAGE_REL_BASED_DIR64) || \
  ((Type) == EFI_IMAGE_REL_BASED_ARM_MOV32T && PcdGetBool (PcdImageLoaderSupportArmThumb))

#define IMAGE_RELOC_SUPPORTED(Reloc) \
  IMAGE_RELOC_TYPE_SUPPORTED (IMAGE_RELOC_TYPE (Reloc))

//
// 4 byte alignment has been replaced with OC_ALIGNOF (EFI_IMAGE_BASE_RELOCATION_BLOCK)
// for proof simplicity. This obviously was the original intention of the
// specification. Assert in case the equality is not given.
//
STATIC_ASSERT (
  sizeof (UINT32) == OC_ALIGNOF (EFI_IMAGE_BASE_RELOCATION_BLOCK),
  "The current model violates the PE specification"
  );

#endif // PE_COFF_INTERNAL_H
