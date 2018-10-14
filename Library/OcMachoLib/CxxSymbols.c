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

#define CXX_PREFIX                     "__Z"
#define VTABLE_PREFIX                  CXX_PREFIX "TV"
#define OSOBJ_PREFIX                   CXX_PREFIX "N"
#define RESERVED_TOKEN                 "_RESERVED"
#define METACLASS_TOKEN                "10gMetaClassE"
#define SUPER_METACLASS_POINTER_TOKEN  "10superClassE"
#define METACLASS_VTABLE_PREFIX        VTABLE_PREFIX "N"
#define METACLASS_VTABLE_SUFFIX        "9MetaClassE"
#define CXX_PURE_VIRTUAL               "___cxa_pure_virtual"
#define FINAL_CLASS_TOKEN              "14__OSFinalClassEv"

#define VTABLE_ENTRY_SIZE_64   8U
#define VTABLE_HEADER_LEN_64   2U
#define VTABLE_HEADER_SIZE_64  (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

#define SYM_MAX_NAME_LEN  256U

/**
  Returns whether Name is pure virtual.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoIsNamePureVirtual (
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
MachoIsNamePadslot (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return (AsciiStrStr (Name, RESERVED_TOKEN) != NULL);
}

/**
  Returns whether Symbol defines a Super Metaclass Pointer.

  @param[in] Symbol       The symbol to check.
  @param[in] StringTable  The KEXT's String Table.

**/
BOOLEAN
MachoSymbolIsSmcp64 (
  IN CONST MACH_NLIST_64  *Symbol,
  IN CONST CHAR8          *StringTable
  )
{
  CONST CHAR8 *Name;

  ASSERT (Symbol != NULL);
  ASSERT (StringTable != NULL);

  Name = (StringTable + Symbol->StringIndex);
  return (AsciiStrStr (Name, SUPER_METACLASS_POINTER_TOKEN) != NULL);
}

/**
  Returns whether Symbol defines a Super Metaclass Pointer.

  @param[in] Symbol       The symbol to check.
  @param[in] StringTable  The KEXT's String Table.

**/
BOOLEAN
MachoSymbolIsMetaclassPointer64 (
  IN CONST MACH_NLIST_64  *Symbol,
  IN CONST CHAR8          *StringTable
  )
{
  CONST CHAR8 *Name;

  ASSERT (Symbol != NULL);
  ASSERT (StringTable != NULL);

  Name = (StringTable + Symbol->StringIndex);
  return (AsciiStrStr (Name, METACLASS_TOKEN) != NULL);
}

/**
  Retrieves the class name of a Super Meta Class Pointer.

  @param[in]  StringTable    String Table of the MACH-O.
  @param[in]  SmcpSymbol     SMCP Symbol to get the class name of.
  @param[in]  ClassNameSize  The size of ClassName.
  @param[out] ClassName      The output buffer for the class name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetClassNameFromSuperMetaClassPointer (
  IN  CONST CHAR8          *StringTable,
  IN  CONST MACH_NLIST_64  *SmcpSymbol,
  IN  UINTN                ClassNameSize,
  OUT CHAR8                *ClassName
  )
{
  BOOLEAN     Result;
  CONST CHAR8 *SuperMetaClassName;
  UINTN       StringLength;
  UINTN       PrefixLength;
  UINTN       SuffixLength;
  UINTN       ClassNameLength;

  ASSERT (StringTable != NULL);
  ASSERT (SmcpSymbol != NULL);
  ASSERT (ClassNameSize > 0);
  ASSERT (ClassName != NULL);

  Result = MachoSymbolIsSmcp64 (SmcpSymbol,  StringTable);
  if (!Result) {
    return FALSE;
  }

  SuperMetaClassName = (StringTable + SmcpSymbol->StringIndex);

  PrefixLength = (ARRAY_SIZE (OSOBJ_PREFIX) - 1);
  StringLength = AsciiStrLen (SuperMetaClassName);
  SuffixLength = (ARRAY_SIZE (SUPER_METACLASS_POINTER_TOKEN) - 1);

  ASSERT ((StringLength - PrefixLength - SuffixLength) < ClassNameSize);

  ClassNameLength = MIN (
                      (StringLength - PrefixLength - SuffixLength),
                      (ClassNameSize - 1)
                      );
  CopyMem (ClassName, &SuperMetaClassName[PrefixLength], ClassNameLength);
  ClassName[ClassNameLength] = '\0';

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
  PrefixLength = (ARRAY_SIZE (VTABLE_PREFIX) - 1);
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
  UINTN BodyLength;

  ASSERT (ClassName != NULL);
  ASSERT (FunctionPrefixSize > 0);
  ASSERT (FunctionPrefix != NULL);

  BodyLength = AsciiStrLen (ClassName);

  if (FunctionPrefixSize < (sizeof (OSOBJ_PREFIX) + BodyLength)) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &FunctionPrefix[Index],
    OSOBJ_PREFIX,
    (sizeof (OSOBJ_PREFIX) - sizeof (*OSOBJ_PREFIX))
    );

  Index += (ARRAY_SIZE (OSOBJ_PREFIX) - 1);
  CopyMem (
    &FunctionPrefix[Index],
    ClassName,
    (BodyLength + sizeof (*ClassName))
    );

  return TRUE;
}

/**
  Retrieves the class name of a Meta Class Pointer.

  @param[in]  StringTable         String Table of the MACH-O.
  @param[in]  MetaClassPtrSymbol  MCP Symbol to get the class name of.
  @param[in]  ClassNameSize       The size of ClassName.
  @param[out] ClassName           The output buffer for the class name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetClassNameFromMetaClassPointer (
  IN  CONST CHAR8          *StringTable,
  IN  CONST MACH_NLIST_64  *MetaClassPtrSymbol,
  IN  UINTN                ClassNameSize,
  OUT CHAR8                *ClassName
  )
{
  BOOLEAN     Result;
  CONST CHAR8 *MetaClassName;
  UINTN       StringLength;
  UINTN       PrefixLength;
  UINTN       SuffixLength;
  UINTN       ClassNameLength;

  ASSERT (StringTable != NULL);
  ASSERT (MetaClassPtrSymbol != NULL);
  ASSERT (ClassNameSize > 0);
  ASSERT (ClassName != NULL);

  Result = MachoSymbolIsMetaclassPointer64 (
             MetaClassPtrSymbol,
             StringTable
             );
  if (!Result) {
    return FALSE;
  }

  MetaClassName = (StringTable + MetaClassPtrSymbol->StringIndex);

  PrefixLength = (ARRAY_SIZE (OSOBJ_PREFIX) - 1);
  StringLength = AsciiStrLen (MetaClassName);
  SuffixLength = (ARRAY_SIZE (METACLASS_TOKEN) - 1);

  ASSERT ((StringLength - PrefixLength - SuffixLength) < ClassNameSize);

  ClassNameLength = MIN (
                      (StringLength - PrefixLength - SuffixLength),
                      (ClassNameSize - 1)
                      );
  CopyMem (ClassName, &MetaClassName[PrefixLength], ClassNameLength);
  ClassName[ClassNameLength] = '\0';

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
  UINTN BodyLength;

  ASSERT (ClassName != NULL);
  ASSERT (VtableNameSize > 0);
  ASSERT (VtableName != NULL);

  BodyLength = AsciiStrLen (ClassName);

  if (VtableNameSize < (sizeof (VTABLE_PREFIX) + BodyLength)) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &VtableName[Index],
    VTABLE_PREFIX,
    (sizeof (VTABLE_PREFIX) - sizeof (*VTABLE_PREFIX))
    );

  Index += (ARRAY_SIZE (VTABLE_PREFIX) - 1);
  CopyMem (&VtableName[Index], ClassName, (BodyLength + sizeof (*ClassName)));

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
  UINTN BodyLength;
  UINTN Index;

  ASSERT (ClassName != NULL);
  ASSERT (VtableNameSize > 0);
  ASSERT (VtableName != NULL);

  BodyLength = AsciiStrLen (ClassName);
  NameSize = (
               sizeof (METACLASS_VTABLE_PREFIX)
                 + BodyLength
                 + (sizeof (METACLASS_VTABLE_SUFFIX) - 1)
             );
  if (VtableNameSize < NameSize) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &VtableName[Index],
    METACLASS_VTABLE_PREFIX,
    (sizeof (METACLASS_VTABLE_PREFIX) - sizeof (*METACLASS_VTABLE_PREFIX))
    );

  Index += (ARRAY_SIZE (METACLASS_VTABLE_PREFIX) - 1);
  CopyMem (
    &VtableName[Index],
    ClassName,
    BodyLength
    );

  Index += BodyLength;
  CopyMem (
    &VtableName[Index],
    METACLASS_VTABLE_SUFFIX,
    sizeof (METACLASS_VTABLE_SUFFIX)
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
  UINTN BodyLength;
  UINTN Index;

  ASSERT (ClassName != NULL);
  ASSERT (FinalSymbolNameSize > 0);
  ASSERT (FinalSymbolName != NULL);

  BodyLength = AsciiStrLen (ClassName);
  NameSize = (
               sizeof (OSOBJ_PREFIX)
                 + BodyLength
                 + (sizeof (FINAL_CLASS_TOKEN) - 1)
             );
  if (FinalSymbolNameSize < NameSize) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &FinalSymbolName[Index],
    OSOBJ_PREFIX,
    (sizeof (OSOBJ_PREFIX) - sizeof (*OSOBJ_PREFIX))
    );

  Index += (ARRAY_SIZE (OSOBJ_PREFIX) - 1);
  CopyMem (
    &FinalSymbolName[Index],
    ClassName,
    BodyLength
    );

  Index += BodyLength;
  CopyMem (
    &FinalSymbolName[Index],
    FINAL_CLASS_TOKEN,
    sizeof (FINAL_CLASS_TOKEN)
    );

  return TRUE;
}

/**
  Returns whether Symbol defines a VTable.

  @param[in] Symbol       The symbol to check.
  @param[in] StringTable  The KEXT's String Table.

**/
BOOLEAN
MachoSymbolIsVtable64 (
  IN CONST MACH_NLIST_64  *Symbol,
  IN CONST CHAR8          *StringTable
  )
{
  CONST CHAR8 *Name;

  ASSERT (Symbol != NULL);
  ASSERT (StringTable != NULL);

  Name = (StringTable + Symbol->StringIndex);
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
  return (AsciiStrnCmp (Name, CXX_PREFIX, (ARRAY_SIZE (CXX_PREFIX) - 1)) == 0);
}

/**
  Returns the number of VTable entires in VtableData.

  @param[in] VtableData   The VTable's data.
  @param[in] MachCpuType  CPU Type of the MACH-O.

**/
UINTN
MachoVtableGetNumberOfEntries64 (
  IN CONST UINT64   *VtableData,
  IN MACH_CPU_TYPE  MachCpuType
  )
{
  UINTN Index;
  UINTN NumberOfEntries;

  ASSERT (VtableData != NULL);

  NumberOfEntries = 0;
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData).
  //
  ASSERT ((MachCpuType != MachCpuTypeArm)
       && (MachCpuType != MachCpuTypeArm64));

  for (Index = VTABLE_HEADER_LEN_64; VtableData[Index] != 0; ++Index) {
    ++NumberOfEntries;
  }

  return NumberOfEntries;
}

/**
  Retrieves Metaclass symbol of a SMCP.

  @param[in] MachHeader           Header of the MACH-O.
  @param[in] NumberOfSymbols      Number of symbols in SymbolTable.
  @param[in] SymbolTable          Symbol Table of the MACH-O.
  @param[in] StringTable          String Table of the MACH-O.
  @param[in] NumberOfRelocations  Number of Relocations in Relocations.
  @param[in] Relocations          The Relocations to evaluate.
  @param[in] Smcp                 The SMCP to evaluate.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_NLIST_64 *
MachoGetMetaclassSymbolFromSmcpSymbol64 (
  IN CONST MACH_HEADER_64        *MachHeader,
  IN UINTN                       NumberOfSymbols,
  IN CONST MACH_NLIST_64         *SymbolTable,
  IN CONST CHAR8                 *StringTable,
  IN UINTN                       NumberOfRelocations,
  IN CONST MACH_RELOCATION_INFO  *Relocations,
  IN CONST MACH_NLIST_64         *Smcp
  )
{
  CONST MACH_RELOCATION_INFO *Relocation;

  ASSERT (MachHeader != NULL);
  ASSERT (SymbolTable != NULL);
  ASSERT (StringTable != NULL);
  ASSERT (Relocations != NULL);
  ASSERT (Smcp != NULL);

  Relocation = MachoGetRelocationByOffset (
                 NumberOfRelocations,
                 Relocations,
                 Smcp->Value,
                 MachHeader->MachCpuType
                 );
  if (Relocation == NULL) {
    return NULL;
  }

  return MachoGetCxxSymbolByRelocation64 (
           MachHeader,
           NumberOfSymbols,
           SymbolTable,
           StringTable,
           Relocation
           );
}

/**
  Retrieves VTable and Meta VTable of a SMCP.
  Logically matches XNU's get_vtable_syms_from_smcp.

  @param[in]  SymbolTable  Symbol Table of the MACH-O.
  @param[in]  StringTable  String Table of the MACH-O.
  @param[in]  DySymtab     Dynamic Symbol Table of the MACH-O.
  @param[in]  SmcpSymbol   SMCP Symbol to retrieve the VTables from.
  @param[out] Vtable       Output buffer for the VTable symbol pointer.
  @param[out] MetaVtable   Output buffer for the Meta VTable symbol pointer.

**/
BOOLEAN
MachoGetVtableSymbolsFromSmcp64 (
  IN  CONST MACH_NLIST_64          *SymbolTable,
  IN  CONST CHAR8                  *StringTable,
  IN  CONST MACH_DYSYMTAB_COMMAND  *DySymtab,
  IN  CONST MACH_NLIST_64          *SmcpSymbol,
  OUT CONST MACH_NLIST_64          **Vtable,
  OUT CONST MACH_NLIST_64          **MetaVtable
  )
{
  CHAR8               ClassName[SYM_MAX_NAME_LEN];
  CHAR8               VtableName[SYM_MAX_NAME_LEN];
  CHAR8               MetaVtableName[SYM_MAX_NAME_LEN];
  BOOLEAN             Result;
  CONST MACH_NLIST_64 *VtableSymbol;
  CONST MACH_NLIST_64 *MetaVtableSymbol;

  ASSERT (SymbolTable != NULL);
  ASSERT (SymbolTable != NULL);
  ASSERT (DySymtab != NULL);
  ASSERT (SmcpSymbol != NULL);
  ASSERT (Vtable != NULL);
  ASSERT (MetaVtable != NULL);

  Result = MachoGetClassNameFromSuperMetaClassPointer (
             NULL,
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

  VtableSymbol = MachoGetLocalDefinedSymbolByName (
                   SymbolTable,
                   StringTable,
                   DySymtab,
                   VtableName
                   );
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
                       SymbolTable,
                       StringTable,
                       DySymtab,
                       MetaVtableName
                       );
  if (MetaVtableSymbol == NULL) {
    return FALSE;
  }

  *Vtable     = VtableSymbol;
  *MetaVtable = MetaVtableSymbol;

  return TRUE;
}
