/**
  Private data of OcMachoLib.

Copyright (C) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_MACHO_LIB_INTERNAL_H_
#define OC_MACHO_LIB_INTERNAL_H_

#include <IndustryStandard/AppleMachoImage.h>

///
/// Context used to refer to a Mach-O.
///
struct OC_MACHO_CONTEXT_ {
  CONST MACH_HEADER_64        *MachHeader;
  UINTN                       FileSize;
  UINT32                      NumSymbols;
  UINT32                      StringsSize;
  CONST MACH_NLIST_64         *SymbolTable;
  CONST CHAR8                 *StringTable;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  CONST MACH_NLIST_64         *IndirectSymbolTable;
  CONST MACH_RELOCATION_INFO  *LocalRelocations;
  CONST MACH_RELOCATION_INFO  *ExternRelocations;
};

/**
  Retrieves the SYMTAB command.

  @param[in] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
BOOLEAN
InternalRetrieveSymtabs64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Retrieves an extern Relocation by the address it targets.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_RELOCATION_INFO *
InternalGetExternalRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  );

#endif // OC_MACHO_LIB_INTERNAL_H_
