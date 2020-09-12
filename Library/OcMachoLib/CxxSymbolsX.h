/** @file
  Provides services for C++ symbols.

Copyright (c) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MachoX.h"

MACH_NLIST_X *
MACH_X (MachoGetMetaclassSymbolFromSmcpSymbol) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_X   *Smcp
  )
{
  MACH_NLIST_X  *Symbol;
  BOOLEAN       Result;

  ASSERT (Context != NULL);
  ASSERT (Smcp != NULL);
  MACH_ASSERT_X (Context);

  Result = MACH_X (MachoGetSymbolByRelocationOffset) (
             Context,
             Smcp->Value,
             &Symbol
             );
  if (Result && (Symbol != NULL)) {
    Result = MachoSymbolNameIsMetaclassPointer (
               Context,
               MACH_X (MachoGetSymbolName) (Context, Symbol)
               );
    if (Result) {
      return Symbol;
    }
  }

  return NULL;
}

BOOLEAN
MACH_X (MachoGetVtableSymbolsFromSmcp) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *SmcpName,
  OUT    CONST MACH_NLIST_X   **Vtable,
  OUT    CONST MACH_NLIST_X   **MetaVtable
  )
{
  CHAR8         ClassName[SYM_MAX_NAME_LEN];
  CHAR8         VtableName[SYM_MAX_NAME_LEN];
  CHAR8         MetaVtableName[SYM_MAX_NAME_LEN];
  BOOLEAN       Result;
  MACH_NLIST_X  *VtableSymbol;
  MACH_NLIST_X  *MetaVtableSymbol;

  ASSERT (Context != NULL);
  ASSERT (SmcpName != NULL);
  ASSERT (Vtable != NULL);
  ASSERT (MetaVtable != NULL);
  MACH_ASSERT_X (Context);

  Result = MachoGetClassNameFromSuperMetaClassPointer (
             Context,
             SmcpName,
             sizeof (ClassName),
             ClassName
             );
  if (!Result) {
    return FALSE;
  }

  Result = MachoGetVtableNameFromClassName (
             ClassName,
             sizeof (VtableName),
             VtableName
             );
  if (!Result) {
    return FALSE;
  }

  VtableSymbol = MACH_X (MachoGetLocalDefinedSymbolByName) (Context, VtableName);
  if (VtableSymbol == NULL) {
    return FALSE;
  }

  Result = MachoGetMetaVtableNameFromClassName (
             ClassName,
             sizeof (MetaVtableName),
             MetaVtableName
             );
  if (!Result) {
    return FALSE;
  }

  MetaVtableSymbol = MACH_X (MachoGetLocalDefinedSymbolByName) (
                       Context,
                       MetaVtableName
                       );
  if (MetaVtableSymbol == NULL) {
    return FALSE;
  }

  *Vtable     = VtableSymbol;
  *MetaVtable = MetaVtableSymbol;

  return TRUE;
}
