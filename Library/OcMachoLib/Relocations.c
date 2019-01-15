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

#include <Base.h>

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

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_RELOCATION_INFO *
InternalGetExternalRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  )
{
  UINT32               Index;
  MACH_RELOCATION_INFO *Relocation;

  ASSERT (Context != NULL);
  //
  // Assumption: 64-bit.
  //
  if (!InternalRetrieveSymtabs64 (Context)) {
    return NULL;
  }

  ASSERT (Context->DySymtab != NULL);
  ASSERT (Context->ExternRelocations != NULL);

  for (Index = 0; Index < Context->DySymtab->NumExternalRelocations; ++Index) {
    Relocation = &Context->ExternRelocations[Index];
    //
    // A section-based relocation entry can be skipped for absolute 
    // symbols.
    // Assumption: Not i386.
    //
    if (((UINT32)Relocation->Address & MACH_RELOC_SCATTERED) != 0) {
      ASSERT (FALSE);
      continue;
    }

    if ((Relocation->Extern == 0)
     && (Relocation->Address == MACH_RELOC_ABSOLUTE)) {
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
    if (MachoRelocationIsPairIntel64 (Relocation->Type)) {
      if (Index == (MAX_UINT32 - 1)) {
        break;
      }
      ++Index;
    }
  }

  return NULL;
}
