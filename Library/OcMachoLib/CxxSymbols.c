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

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>

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

BOOLEAN
MachoSymbolNameIsPureVirtual (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return (AsciiStrCmp (Name, CXX_PURE_VIRTUAL) == 0);
}

BOOLEAN
MachoSymbolNameIsPadslot (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return (AsciiStrStr (Name, RESERVED_TOKEN) != NULL);
}

BOOLEAN
MachoSymbolNameIsSmcp (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SymbolName
  )
{
  CONST CHAR8 *Suffix;

  ASSERT (Context != NULL);
  ASSERT (SymbolName != NULL);
  //
  // Verify the symbol has...
  //   1) The C++ prefix.
  //   2) The SMCP suffix.
  //   3) At least one character between the prefix and the suffix.
  //
  Suffix = AsciiStrStr (SymbolName, SMCP_TOKEN);

  if ((Suffix == NULL)
   || (AsciiStrnCmp (SymbolName, OSOBJ_PREFIX, L_STR_LEN (OSOBJ_PREFIX)) != 0)
   || ((UINTN)(Suffix - SymbolName) <= L_STR_LEN (OSOBJ_PREFIX))) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
MachoSymbolNameIsMetaclassPointer (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SymbolName
  )
{
  CONST CHAR8 *Suffix;

  ASSERT (Context != NULL);
  ASSERT (SymbolName != NULL);
  //
  // Verify the symbol has...
  //   1) The C++ prefix.
  //   2) The MetaClass suffix.
  //   3) At least one character between the prefix and the suffix.
  //
  Suffix = AsciiStrStr (SymbolName, METACLASS_TOKEN);

  if ((Suffix == NULL)
   || (AsciiStrnCmp (SymbolName, OSOBJ_PREFIX, L_STR_LEN (OSOBJ_PREFIX)) != 0)
   || ((UINTN)(Suffix - SymbolName) <= L_STR_LEN (OSOBJ_PREFIX))) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
MachoGetClassNameFromSuperMetaClassPointer (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *SmcpName,
  IN     UINTN                ClassNameSize,
  OUT    CHAR8                *ClassName
  )
{
  UINTN                  PrefixSize;
  UINTN                  SuffixSize;
  UINTN                  OutputSize;

  ASSERT (Context != NULL);
  ASSERT (SmcpName != NULL);
  ASSERT (ClassNameSize > 0);
  ASSERT (ClassName != NULL);

  ASSERT (Context->StringTable != NULL);

  ASSERT (MachoSymbolNameIsSmcp (Context, SmcpName));

  PrefixSize = L_STR_LEN (OSOBJ_PREFIX);
  SuffixSize = L_STR_LEN (SMCP_TOKEN);

  OutputSize = (AsciiStrLen (SmcpName) - PrefixSize - SuffixSize);

  if ((OutputSize + 1) > ClassNameSize) {
    return FALSE;
  }

  CopyMem (ClassName, &SmcpName[PrefixSize], OutputSize);
  ClassName[OutputSize] = '\0';

  return TRUE;
}

CONST CHAR8 *
MachoGetClassNameFromVtableName (
  IN  CONST CHAR8  *VtableName
  )
{
  ASSERT (VtableName != NULL);
  ASSERT (MachoSymbolNameIsVtable (VtableName));
  //
  // As there is no suffix, just return a pointer from within VtableName.
  //
  return &VtableName[L_STR_LEN (VTABLE_PREFIX)];
}

BOOLEAN
MachoGetFunctionPrefixFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        FunctionPrefixSize,
  OUT CHAR8        *FunctionPrefix
  )
{
  UINTN   Index;
  UINTN   BodySize;
  BOOLEAN Result;
  UINTN   TotalSize;

  ASSERT (ClassName != NULL);
  ASSERT (FunctionPrefixSize > 0);
  ASSERT (FunctionPrefix != NULL);

  BodySize = AsciiStrSize (ClassName);
  Result   = OcOverflowAddUN (L_STR_LEN (OSOBJ_PREFIX), BodySize, &TotalSize);
  if (Result || (FunctionPrefixSize < TotalSize)) {
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &FunctionPrefix[Index],
    OSOBJ_PREFIX,
    L_STR_LEN (OSOBJ_PREFIX)
    );

  Index += L_STR_LEN (OSOBJ_PREFIX);
  CopyMem (
    &FunctionPrefix[Index],
    ClassName,
    BodySize
    );

  return TRUE;
}

BOOLEAN
MachoGetClassNameFromMetaClassPointer (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *MetaClassName,
  IN     UINTN                ClassNameSize,
  OUT    CHAR8                *ClassName
  )
{
  UINTN                  PrefixSize;
  UINTN                  SuffixSize;
  UINTN                  ClassNameLength;

  ASSERT (Context != NULL);
  ASSERT (MetaClassName != NULL);
  ASSERT (ClassNameSize > 0);
  ASSERT (ClassName != NULL);

  ASSERT (Context->StringTable != NULL);

  ASSERT (MachoSymbolNameIsMetaclassPointer (Context, MetaClassName));

  PrefixSize = L_STR_LEN (OSOBJ_PREFIX);
  SuffixSize = L_STR_LEN (METACLASS_TOKEN);

  ClassNameLength = (AsciiStrLen (MetaClassName) - PrefixSize - SuffixSize);
  if ((ClassNameLength + 1) > ClassNameSize) {
    return FALSE;
  }

  CopyMem (ClassName, &MetaClassName[PrefixSize], ClassNameLength);
  ClassName[ClassNameLength] = '\0';

  return TRUE;
}

BOOLEAN
MachoGetVtableNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        VtableNameSize,
  OUT CHAR8        *VtableName
  )
{
  UINTN   Index;
  UINTN   BodySize;
  BOOLEAN Result;
  UINTN   TotalSize;

  ASSERT (ClassName != NULL);
  ASSERT (VtableNameSize > 0);
  ASSERT (VtableName != NULL);

  BodySize = AsciiStrSize (ClassName);

  Result = OcOverflowAddUN (
             L_STR_LEN (VTABLE_PREFIX),
             BodySize,
             &TotalSize
             );
  if (Result || (VtableNameSize < TotalSize)) {
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &VtableName[Index],
    VTABLE_PREFIX,
    L_STR_LEN (VTABLE_PREFIX)
    );

  Index += L_STR_LEN (VTABLE_PREFIX);
  CopyMem (&VtableName[Index], ClassName, BodySize);

  return TRUE;
}

BOOLEAN
MachoGetMetaVtableNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        VtableNameSize,
  OUT CHAR8        *VtableName
  )
{
  UINTN   BodyLength;
  BOOLEAN Result;
  UINTN   TotalSize;
  UINTN   Index;

  ASSERT (ClassName != NULL);
  ASSERT (VtableNameSize > 0);
  ASSERT (VtableName != NULL);

  BodyLength = AsciiStrLen (ClassName);

  Result = OcOverflowTriAddUN (
             L_STR_LEN (METACLASS_VTABLE_PREFIX),
             BodyLength,
             L_STR_SIZE (METACLASS_VTABLE_SUFFIX),
             &TotalSize
             );
  if (Result || (VtableNameSize < TotalSize)) {
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &VtableName[Index],
    METACLASS_VTABLE_PREFIX,
    L_STR_LEN (METACLASS_VTABLE_PREFIX)
    );

  Index += L_STR_LEN (METACLASS_VTABLE_PREFIX);
  CopyMem (&VtableName[Index], ClassName, BodyLength);

  Index += BodyLength;
  CopyMem (
    &VtableName[Index],
    METACLASS_VTABLE_SUFFIX,
    L_STR_SIZE (METACLASS_VTABLE_SUFFIX)
    );

  return TRUE;
}

BOOLEAN
MachoGetFinalSymbolNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        FinalSymbolNameSize,
  OUT CHAR8        *FinalSymbolName
  )
{
  UINTN   BodyLength;
  BOOLEAN Result;
  UINTN   TotalSize;
  UINTN   Index;

  ASSERT (ClassName != NULL);
  ASSERT (FinalSymbolNameSize > 0);
  ASSERT (FinalSymbolName != NULL);

  BodyLength = AsciiStrLen (ClassName);

  Result = OcOverflowTriAddUN (
             L_STR_LEN (OSOBJ_PREFIX),
             BodyLength,
             L_STR_SIZE (FINAL_CLASS_TOKEN),
             &TotalSize
             );
  if (Result || (FinalSymbolNameSize < TotalSize)) {
    return FALSE;
  }

  Index = 0;
  CopyMem (
    &FinalSymbolName[Index],
    OSOBJ_PREFIX,
    L_STR_LEN (OSOBJ_PREFIX)
    );

  Index += L_STR_LEN (OSOBJ_PREFIX);
  CopyMem (
    &FinalSymbolName[Index],
    ClassName,
    BodyLength
    );

  Index += BodyLength;
  CopyMem (
    &FinalSymbolName[Index],
    FINAL_CLASS_TOKEN,
    L_STR_SIZE (FINAL_CLASS_TOKEN)
    );

  return TRUE;
}

BOOLEAN
MachoSymbolNameIsVtable (
  IN     CONST CHAR8       *SymbolName
  )
{
  ASSERT (SymbolName != NULL);
  //
  // Implicitely checks for METACLASS_VTABLE_PREFIX.
  //
  return AsciiStrnCmp (SymbolName, VTABLE_PREFIX, L_STR_LEN (VTABLE_PREFIX)) == 0;
}

BOOLEAN
MachoSymbolNameIsCxx (
  IN CONST CHAR8  *Name
  )
{
  ASSERT (Name != NULL);
  return AsciiStrnCmp (Name, CXX_PREFIX, L_STR_LEN (CXX_PREFIX)) == 0;
}

MACH_NLIST_ANY *
MachoGetMetaclassSymbolFromSmcpSymbol (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_ANY  *Smcp
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_NLIST_ANY *) MachoGetMetaclassSymbolFromSmcpSymbol32 (Context, &Smcp->Symbol32) :
    (MACH_NLIST_ANY *) MachoGetMetaclassSymbolFromSmcpSymbol64 (Context, &Smcp->Symbol64);
}

BOOLEAN
MachoGetVtableSymbolsFromSmcp (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *SmcpName,
  OUT    CONST MACH_NLIST_ANY **Vtable,
  OUT    CONST MACH_NLIST_ANY **MetaVtable
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MachoGetVtableSymbolsFromSmcp32 (Context, SmcpName, (CONST MACH_NLIST **) Vtable, (CONST MACH_NLIST **) MetaVtable) :
    MachoGetVtableSymbolsFromSmcp64 (Context, SmcpName, (CONST MACH_NLIST_64 **) Vtable, (CONST MACH_NLIST_64 **) MetaVtable);
}
