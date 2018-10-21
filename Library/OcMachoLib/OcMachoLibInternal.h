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
typedef struct {
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
} OC_MACHO_CONTEXT;

/**
  Retrieves the SYMTAB command.

  @param[in] MachoContext  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
BOOLEAN
InternalRetrieveSymtabs64 (
  IN OC_MACHO_CONTEXT  *MachoContext
  );

#endif // OC_MACHO_LIB_INTERNAL_H_
