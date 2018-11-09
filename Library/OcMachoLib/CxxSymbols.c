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

#include <Base.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>

#include "OcMachoLibInternal.h"

#define CXX_PREFIX               "__Z"
#define VTABLE_PREFIX            CXX_PREFIX "TV"
#define OSOBJ_PREFIX             CXX_PREFIX "N"
#define RESERVED_TOKEN           "_RESERVED"
#define METACLASS_TOKEN          "10gMetaClassE"
#define SMCP_TOKEN               "10superClassE"
#define METACLASS_VTABLE_PREFIX  VTABLE_PREFIX "N"
#define METACLASS_VTABLE_SUFFIX  "9MetaClassE"
#define CXX_PURE_VIRTUAL         "___cxa_pure_virtual"
#define FINAL_CLASS_TOKEN        "14__OSFinalClassEv"

#define VTABLE_ENTRY_SIZE_64   8U
#define VTABLE_HEADER_LEN_64   2U
#define VTABLE_HEADER_SIZE_64  (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

#define SYM_MAX_NAME_LEN  256U

/**
  Returns whether Name is pure virtual.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoIsSymbolNamePureVirtual (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return (AsciiStrCmp (Name, CXX_PURE_VIRTUAL) == 0);
}

/**
  Returns whether Name is a Padslot.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoIsSymbolNamePadslot (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return (AsciiStrStr (Name, RESERVED_TOKEN) != NULL);
}

/**
  Returns whether Symbol defines a Super Metaclass Pointer.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   The symbol to check.

**/
BOOLEAN
MachoSymbolIsSmcp64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  CONST CHAR8 *Name;
  CONST CHAR8 *Suffix;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (Context->StringTable != NULL);

  Name = MachoGetSymbolName64 (Context, Symbol);
  //
  // Verify the symbol has...
  //   1) The C++ prefix.
  //   2) The SMCP suffix.
  //   3) At least one character between the prefix and the suffix.
  //
  Suffix = AsciiStrStr (Name, SMCP_TOKEN);

  if ((Suffix == NULL)
   || (AsciiStrnCmp (Name, OSOBJ_PREFIX, L_STR_LEN (OSOBJ_PREFIX)) != 0)
   || ((Suffix - Name) <= L_STR_LEN (OSOBJ_PREFIX))) {
    return FALSE;
  }

  return TRUE;
}

/**
  Returns whether Symbol defines a Super Metaclass Pointer.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   The symbol to check.

**/
BOOLEAN
MachoSymbolIsMetaclassPointer64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  CONST CHAR8 *Name;
  CONST CHAR8 *Suffix;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (Context->StringTable != NULL);

  Name = MachoGetSymbolName64 (Context, Symbol);
  //
  // Verify the symbol has...
  //   1) The C++ prefix.
  //   2) The MetaClass suffix.
  //   3) At least one character between the prefix and the suffix.
  //
  Suffix = AsciiStrStr (Name, METACLASS_TOKEN);

  if ((Suffix == NULL)
   || (AsciiStrnCmp (Name, OSOBJ_PREFIX, L_STR_LEN (OSOBJ_PREFIX)) != 0)
   || ((Suffix - Name) <= L_STR_LEN (OSOBJ_PREFIX))) {
    return FALSE;
  }

  return TRUE;
}

/**
  Retrieves the class name of a Super Meta Class Pointer.

  @param[in,out] Context        Context of the Mach-O.
  @param[in]     SmcpSymbol     SMCP Symbol to get the class name of.
  @param[in]     ClassNameSize  The size of ClassName.
  @param[out]    ClassName      The output buffer for the class name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetClassNameFromSuperMetaClassPointer (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *SmcpSymbol,
  IN     UINTN                ClassNameSize,
  OUT    CHAR8                *ClassName
  )
{
  BOOLEAN                Result;
  CONST CHAR8            *SmcpName;
  UINTN                  StringSize;
  UINTN                  PrefixSize;
  UINTN                  SuffixSize;
  UINTN                  OutputSize;

  ASSERT (Context != NULL);
  ASSERT (SmcpSymbol != NULL);
  ASSERT (ClassNameSize > 0);
  ASSERT (ClassName != NULL);

  ASSERT (Context->StringTable != NULL);

  Result = MachoSymbolIsSmcp64 (Context, SmcpSymbol);
  if (!Result) {
    return FALSE;
  }

  SmcpName   = MachoGetSymbolName64 (Context, SmcpSymbol);
  PrefixSize = L_STR_SIZE_NT (OSOBJ_PREFIX);
  StringSize = (AsciiStrLen (SmcpName) * sizeof (*SmcpName));
  SuffixSize = L_STR_SIZE_NT (SMCP_TOKEN);

  ASSERT ((StringSize - PrefixSize - SuffixSize) < ClassNameSize);

  OutputSize = MIN (
                 (StringSize - PrefixSize - SuffixSize),
                 (ClassNameSize - sizeof (*ClassName))
                 );
  CopyMem (ClassName, &SmcpName[PrefixSize], OutputSize);
  ClassName[OutputSize / sizeof (*ClassName)] = '\0';

  return TRUE;
}

/**
  Retrieves the class name of a VTable.

  @param[out] VtableName  The VTable's name.

**/
CONST CHAR8 *
MachoGetClassNameFromVtableName (
  IN  CONST CHAR8  *VtableName
  )
{
  UINTN PrefixLength;

  ASSERT (VtableName != NULL);
  //
  // As there is no suffix, just return a pointer from within VtableName.
  //
  PrefixLength = L_STR_LEN (VTABLE_PREFIX);
  return &VtableName[PrefixLength];
}

/**
  Retrieves the function prefix of a class name.

  @param[in]  ClassName           The class name to evaluate.
  @param[in]  FunctionPrefixSize  The size of FunctionPrefix.
  @param[out] FunctionPrefix      The output buffer for the function prefix.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetFunctionPrefixFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        FunctionPrefixSize,
  OUT CHAR8        *FunctionPrefix
  )
{
  UINTN Index;
  UINTN BodySize;

  ASSERT (ClassName != NULL);
  ASSERT (FunctionPrefixSize > 0);
  ASSERT (FunctionPrefix != NULL);

  BodySize = (AsciiStrLen (ClassName) * sizeof (*ClassName));

  if (FunctionPrefixSize < (L_STR_SIZE (OSOBJ_PREFIX) + BodySize)) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &FunctionPrefix[Index],
    OSOBJ_PREFIX,
    L_STR_SIZE_NT (OSOBJ_PREFIX)
    );

  Index += L_STR_LEN (OSOBJ_PREFIX);
  CopyMem (
    &FunctionPrefix[Index],
    ClassName,
    (BodySize + sizeof (*ClassName))
    );

  return TRUE;
}

/**
  Retrieves the class name of a Meta Class Pointer.

  @param[in,out] Context             Context of the Mach-O.
  @param[in]     MetaClassPtrSymbol  MCP Symbol to get the class name of.
  @param[in]     ClassNameSize       The size of ClassName.
  @param[out]    ClassName           The output buffer for the class name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetClassNameFromMetaClassPointer (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *MetaClassPtrSymbol,
  IN     UINTN                ClassNameSize,
  OUT    CHAR8                *ClassName
  )
{
  BOOLEAN                Result;
  CONST CHAR8            *MetaClassName;
  UINTN                  StringSize;
  UINTN                  PrefixSize;
  UINTN                  SuffixSize;
  UINTN                  ClassNameLength;

  ASSERT (Context != NULL);
  ASSERT (MetaClassPtrSymbol != NULL);
  ASSERT (ClassNameSize > 0);
  ASSERT (ClassName != NULL);

  ASSERT (Context->StringTable != NULL);

  Result = MachoSymbolIsMetaclassPointer64 (Context, MetaClassPtrSymbol);
  if (!Result) {
    return FALSE;
  }

  MetaClassName = MachoGetSymbolName64 (Context, MetaClassPtrSymbol);
  PrefixSize = L_STR_SIZE_NT (OSOBJ_PREFIX);
  StringSize = (AsciiStrLen (MetaClassName) * sizeof (*MetaClassName));
  SuffixSize = L_STR_SIZE_NT (METACLASS_TOKEN);

  ASSERT ((StringSize - PrefixSize - SuffixSize) < ClassNameSize);

  ClassNameLength = MIN (
                      (StringSize - PrefixSize - SuffixSize),
                      (ClassNameSize - sizeof (*ClassName))
                      );
  CopyMem (ClassName, &MetaClassName[PrefixSize], ClassNameLength);
  ClassName[ClassNameLength / sizeof (*ClassName)] = '\0';

  return TRUE;
}

/**
  Retrieves the VTable name of a class name.

  @param[in]  ClassName       Class name to get the VTable name of.
  @param[in]  VtableNameSize  The size of VtableName.
  @param[out] VtableName      The output buffer for the VTable name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetVtableNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        VtableNameSize,
  OUT CHAR8        *VtableName
  )
{
  UINTN Index;
  UINTN BodySize;

  ASSERT (ClassName != NULL);
  ASSERT (VtableNameSize > 0);
  ASSERT (VtableName != NULL);

  BodySize = AsciiStrSize (ClassName);

  if (VtableNameSize < (L_STR_SIZE_NT (VTABLE_PREFIX) + BodySize)) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &VtableName[Index],
    VTABLE_PREFIX,
    L_STR_SIZE_NT (VTABLE_PREFIX)
    );

  Index += L_STR_LEN (VTABLE_PREFIX);
  CopyMem (&VtableName[Index], ClassName, BodySize);

  return TRUE;
}

/**
  Retrieves the Meta VTable name of a class name.

  @param[in]  ClassName       Class name to get the Meta VTable name of.
  @param[in]  VtableNameSize  The size of VtableName.
  @param[out] VtableName      The output buffer for the VTable name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetMetaVtableNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        VtableNameSize,
  OUT CHAR8        *VtableName
  )
{
  UINTN NameSize;
  UINTN BodySize;
  UINTN Index;

  ASSERT (ClassName != NULL);
  ASSERT (VtableNameSize > 0);
  ASSERT (VtableName != NULL);

  BodySize = (AsciiStrLen (ClassName) * sizeof (*ClassName));
  NameSize = (
               L_STR_SIZE (METACLASS_VTABLE_PREFIX)
                 + BodySize
                 + L_STR_SIZE_NT (METACLASS_VTABLE_SUFFIX)
             );
  if (VtableNameSize < NameSize) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &VtableName[Index],
    METACLASS_VTABLE_PREFIX,
    L_STR_SIZE_NT (METACLASS_VTABLE_PREFIX)
    );

  Index += (ARRAY_SIZE (METACLASS_VTABLE_PREFIX) - 1);
  CopyMem (&VtableName[Index], ClassName, BodySize);

  Index += (BodySize / sizeof (*ClassName));
  CopyMem (
    &VtableName[Index],
    METACLASS_VTABLE_SUFFIX,
    L_STR_SIZE (METACLASS_VTABLE_SUFFIX)
    );

  return TRUE;
}

/**
  Retrieves the final symbol name of a class name.

  @param[in]  ClassName            Class name to get the final symbol name of.
  @param[in]  FinalSymbolNameSize  The size of FinalSymbolName.
  @param[out] FinalSymbolName      The output buffer for the final symbol name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetFinalSymbolNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        FinalSymbolNameSize,
  OUT CHAR8        *FinalSymbolName
  )
{
  UINTN NameSize;
  UINTN BodySize;
  UINTN Index;

  ASSERT (ClassName != NULL);
  ASSERT (FinalSymbolNameSize > 0);
  ASSERT (FinalSymbolName != NULL);

  BodySize = (AsciiStrLen (ClassName) * sizeof (*ClassName));
  NameSize = (
               L_STR_SIZE (OSOBJ_PREFIX)
                 + BodySize
                 + L_STR_SIZE_NT (FINAL_CLASS_TOKEN)
             );
  if (FinalSymbolNameSize < NameSize) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &FinalSymbolName[Index],
    OSOBJ_PREFIX,
    L_STR_SIZE_NT (OSOBJ_PREFIX)
    );

  Index += L_STR_LEN (OSOBJ_PREFIX);
  CopyMem (
    &FinalSymbolName[Index],
    ClassName,
    BodySize
    );

  Index += (BodySize / sizeof (*ClassName));
  CopyMem (
    &FinalSymbolName[Index],
    FINAL_CLASS_TOKEN,
    L_STR_SIZE (FINAL_CLASS_TOKEN)
    );

  return TRUE;
}

/**
  Returns whether Symbol defines a VTable.

  @param[in,out] Context  Conteyt of the Mach-O.
  @param[in]     Symbol   The symbol to check.

**/
BOOLEAN
MachoSymbolIsVtable64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  CONST CHAR8            *Name;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (Context->StringTable != NULL);

  Name = MachoGetSymbolName64 (Context, Symbol);
  //
  // Implicitely checks for METACLASS_VTABLE_PREFIX.
  //
  return (AsciiStrCmp (Name, VTABLE_PREFIX) == 0);
}

/**
  Returns whether the symbol name describes a C++ symbol.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoIsSymbolNameCxx (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return (AsciiStrnCmp (Name, CXX_PREFIX, L_STR_LEN (CXX_PREFIX)) == 0);
}

/**
  Returns the number of VTable entires in VtableData.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     VtableData  The VTable's data.

**/
UINTN
MachoVtableGetNumberOfEntries64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST UINT64      *VtableData
  )
{
  UINTN Index;
  UINTN NumberOfEntries;

  ASSERT (Context != NULL);
  ASSERT (VtableData != NULL);

  NumberOfEntries = 0;
  //
  // Assumption: Not ARM.  Currently verified by the Context initialization.
  //
  for (Index = VTABLE_HEADER_LEN_64; VtableData[Index] != 0; ++Index) {
    ++NumberOfEntries;
  }

  return NumberOfEntries;
}

/**
  Retrieves Metaclass symbol of a SMCP.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Smcp     The SMCP to evaluate.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_NLIST_64 *
MachoGetMetaclassSymbolFromSmcpSymbol64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Smcp
  )
{
  BOOLEAN             Result;
  CONST MACH_NLIST_64 *Symbol;

  ASSERT (Context != NULL);
  ASSERT (Smcp != NULL);

  if (!MachoIsSymbolValueSane64 (Context, Smcp)) {
    return NULL;
  }

  Result = MachoGetSymbolByExternRelocationOffset64 (
             Context,
             Smcp->Value,
             &Symbol
             );
  if (!Result) {
    return NULL;
  }

  return Symbol;
}

/**
  Retrieves VTable and Meta VTable of a SMCP.
  Logically matches XNU's get_vtable_syms_from_smcp.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     SmcpSymbol   SMCP Symbol to retrieve the VTables from.
  @param[out]    Vtable       Output buffer for the VTable symbol pointer.
  @param[out]    MetaVtable   Output buffer for the Meta VTable symbol pointer.

**/
BOOLEAN
MachoGetVtableSymbolsFromSmcp64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *SmcpSymbol,
  OUT    CONST MACH_NLIST_64  **Vtable,
  OUT    CONST MACH_NLIST_64  **MetaVtable
  )
{
  CHAR8               ClassName[SYM_MAX_NAME_LEN];
  CHAR8               VtableName[SYM_MAX_NAME_LEN];
  CHAR8               MetaVtableName[SYM_MAX_NAME_LEN];
  BOOLEAN             Result;
  CONST MACH_NLIST_64 *VtableSymbol;
  CONST MACH_NLIST_64 *MetaVtableSymbol;

  ASSERT (Context != NULL);
  ASSERT (SmcpSymbol != NULL);
  ASSERT (Vtable != NULL);
  ASSERT (MetaVtable != NULL);

  Result = MachoGetClassNameFromSuperMetaClassPointer (
             Context,
             SmcpSymbol,
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

  VtableSymbol = MachoGetLocalDefinedSymbolByName (Context, VtableName);
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

  MetaVtableSymbol = MachoGetLocalDefinedSymbolByName (
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
