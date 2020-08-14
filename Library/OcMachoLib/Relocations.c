/** @file
  Provides services for Relocations.

Copyright (c) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/DebugLib.h>
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

/**
  Returns whether the Relocation's type indicates a Pair for the Intel 64
  platform.
  
  @param[in] Type  The Relocation's type to verify.

**/
BOOLEAN
MachoRelocationIsPairIntel64 (
  IN UINT8  Type
  )
{
  return (Type == MachX8664RelocSubtractor);
}

/**
  Returns whether the Relocation's type matches a Pair's for the Intel 64
  platform.
  
  @param[in] Type  The Relocation's type to verify.

**/
BOOLEAN
MachoIsRelocationPairTypeIntel64 (
  IN UINT8  Type
  )
{
  return (Type == MachX8664RelocUnsigned);
}

/**
  Returns whether the Relocation shall be preserved for the Intel 64 platform.
  
  @param[in] Type  The Relocation's type to verify.

**/
BOOLEAN
MachoPreserveRelocationIntel64 (
  IN UINT8  Type
  )
{
  return (Type == MachX8664RelocUnsigned);
}

/**
  Retrieves an extern Relocation by the address it targets.

  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_RELOCATION_INFO *
InternalLookupRelocationByOffset (
  IN     UINT64                Address,
  IN     UINT32                NumRelocs,
  IN     MACH_RELOCATION_INFO  *Relocs
  )
{
  UINT32               Index;
  MACH_RELOCATION_INFO *Relocation;

  for (Index = 0; Index < NumRelocs; ++Index) {
    Relocation = &Relocs[Index];
    //
    // A section-based relocation entry can be skipped for absolute symbols.
    //
    if ((Relocation->Extern == 0)
     && (Relocation->SymbolNumber == MACH_RELOC_ABSOLUTE)) {
      continue;
    }

    if ((UINT64)Relocation->Address == Address) {
      return Relocation;
    }
    //
    // Relocation Pairs can be skipped.
    // Assumption: Intel X64.  Currently verified by the Context
    //             initialization.
    //
    if (MachoRelocationIsPairIntel64 ((UINT8)Relocation->Type)) {
      if (Index == (MAX_UINT32 - 1)) {
        break;
      }
      ++Index;
    }
  }

  return NULL;
}

/**
  Retrieves an extern Relocation by the address it targets.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_RELOCATION_INFO *
InternalGetExternRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  )
{
  return InternalLookupRelocationByOffset (
           Address,
           Context->DySymtab->NumExternalRelocations,
           Context->ExternRelocations
           );
}

/**
  Retrieves an extern Relocation by the address it targets.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_RELOCATION_INFO *
InternalGetLocalRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  )
{
  return InternalLookupRelocationByOffset (
           Address,
           Context->DySymtab->NumOfLocalRelocations,
           Context->LocalRelocations
           );
}
