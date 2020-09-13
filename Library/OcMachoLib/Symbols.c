/** @file
  Provides services for symbols.

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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

MACH_NLIST_ANY *
MachoGetSymbolByIndex (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_NLIST_ANY *) MachoGetSymbolByIndex32 (Context, Index) :
    (MACH_NLIST_ANY *) MachoGetSymbolByIndex64 (Context, Index);  
}

CONST CHAR8 *
MachoGetSymbolName (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_ANY *Symbol
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MachoGetSymbolName32 (Context, &Symbol->Symbol32) :
    MachoGetSymbolName64 (Context, &Symbol->Symbol64);  
}

BOOLEAN
MachoIsSymbolValueInRange (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_ANY *Symbol
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MachoIsSymbolValueInRange32 (Context, &Symbol->Symbol32) :
    MachoIsSymbolValueInRange64 (Context, &Symbol->Symbol64);  
}

BOOLEAN
MachoSymbolGetFileOffset (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     CONST  MACH_NLIST_ANY  *Symbol,
  OUT    UINT32                 *FileOffset,
  OUT    UINT32                 *MaxSize OPTIONAL
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MachoSymbolGetFileOffset32 (Context, &Symbol->Symbol32, FileOffset, MaxSize) :
    MachoSymbolGetFileOffset64 (Context, &Symbol->Symbol64, FileOffset, MaxSize);  
}

BOOLEAN
MachoSymbolGetDirectFileOffset (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     UINT64                 Address,
  OUT    UINT32                 *FileOffset,
  OUT    UINT32                 *MaxSize OPTIONAL
  )
{
  ASSERT (Context != NULL);
  if (Context->Is32Bit) {
    ASSERT (Address < MAX_UINT32);
  }

  return Context->Is32Bit ?
    InternalMachoSymbolGetDirectFileOffset32 (Context, (UINT32) Address, FileOffset, MaxSize) :
    InternalMachoSymbolGetDirectFileOffset64 (Context, Address, FileOffset, MaxSize);
}
