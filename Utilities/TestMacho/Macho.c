/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>

#include <string.h>
#include <sys/time.h>

#include <UserFile.h>

MACH_HEADER_64           mHeader;
MACH_SECTION_64          mSect;
MACH_SEGMENT_COMMAND_64  mSeg;
MACH_UUID_COMMAND        mUuid;

STATIC
int
FeedMacho (
  IN OUT VOID    *File,
  IN     UINT32  Size
  )
{
  OC_MACHO_CONTEXT  Context;

  if (!MachoInitializeContext64 (&Context, File, Size, 0)) {
    return -1;
  }

  int  Code = 0;

  MACH_HEADER_64  *Hdr = MachoGetMachHeader64 (&Context);

  if ((Hdr != NULL) && (MachoGetFileSize (&Context) > 10) && (MachoGetLastAddress (&Context) != 10)) {
    CopyMem (&mHeader, Hdr, sizeof (mHeader));
    ++Code;
  }

  MACH_UUID_COMMAND  *Cmd = MachoGetUuid (&Context);

  if (Cmd != NULL) {
    CopyMem (&mUuid, Cmd, sizeof (mUuid));
    ++Code;
  }

  MACH_SEGMENT_COMMAND_64  *Segment = MachoGetSegmentByName64 (&Context, "__LINKEDIT");
  MACH_SECTION_64          *Section;

  if (Segment != NULL) {
    CopyMem (&mSeg, Segment, sizeof (mSeg));
    Section = MachoGetSectionByName64 (&Context, Segment, "__objc");
    if (Section != NULL) {
      CopyMem (&mSect, Section, sizeof (mSect));
      ++Code;
    }
  }

  UINT32  Index = 0;

  while ((Section = MachoGetSectionByIndex64 (&Context, Index)) != NULL) {
    CopyMem (&mSect, Section, sizeof (mSect));
    ++Index;
  }

  if ((Section = MachoGetSectionByAddress64 (&Context, Index)) != NULL) {
    CopyMem (&mSect, Section, sizeof (mSect));
    ++Code;
  }

  MACH_NLIST_64  *Symbol = NULL;

  for (Index = 0; (Symbol = MachoGetSymbolByIndex64 (&Context, Index)) != NULL; ++Index) {
    CONST CHAR8  *Indirect = MachoGetIndirectSymbolName64 (&Context, Symbol);
    if (  (AsciiStrCmp (MachoGetSymbolName64 (&Context, Symbol), "__hack") == 0)
       || ((Indirect != NULL) && (AsciiStrCmp (Indirect, "__hack") == 0)))
    {
      ++Code;
    }

    if (MachoSymbolIsSection64 (Symbol)) {
      ++Code;
    }

    if (MachoSymbolIsDefined64 (Symbol)) {
      ++Code;
    }

    if (MachoSymbolIsLocalDefined64 (&Context, Symbol)) {
      ++Code;
    }

    if (MachoIsSymbolValueInRange64 (&Context, Symbol)) {
      ++Code;
    }

    UINT32  Offset;
    if (MachoSymbolGetFileOffset64 (&Context, Symbol, &Offset, NULL)) {
      Code += Offset;
    }

    if (MachoSymbolNameIsPureVirtual (MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    if (MachoSymbolNameIsPadslot (MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    if (MachoSymbolNameIsSmcp (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    if (MachoSymbolNameIsMetaclassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    char  Out[64];
    if (  MachoSymbolNameIsSmcp (&Context, MachoGetSymbolName64 (&Context, Symbol))
       && MachoGetClassNameFromSuperMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out))
    {
      ++Code;
    }

    if (MachoSymbolNameIsVtable (MachoGetSymbolName64 (&Context, Symbol))) {
      if (AsciiStrCmp (MachoGetClassNameFromVtableName (MachoGetSymbolName64 (&Context, Symbol)), "sym") != 0) {
        ++Code;
      }
    }

    if (  MachoGetFunctionPrefixFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoSymbolNameIsMetaclassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol))
       && MachoGetClassNameFromMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoGetVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoGetMetaVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoGetFinalSymbolNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (MachoSymbolNameIsCxx (MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    MACH_NLIST_64  *SCMP = MachoGetMetaclassSymbolFromSmcpSymbol64 (&Context, Symbol);
    if (SCMP != NULL) {
      if (AsciiStrCmp (MachoGetSymbolName64 (&Context, SCMP), "__hack") == 0) {
        ++Code;
      }

      CONST MACH_NLIST_64  *Vtable;
      CONST MACH_NLIST_64  *MetaVtable;
      if (MachoGetVtableSymbolsFromSmcp64 (&Context, MachoGetSymbolName64 (&Context, SCMP), &Vtable, &MetaVtable)) {
        if (AsciiStrCmp (MachoGetSymbolName64 (&Context, Vtable), "__hack") == 0) {
          ++Code;
        }

        if (AsciiStrCmp (MachoGetSymbolName64 (&Context, MetaVtable), "__hack") == 0) {
          ++Code;
        }
      }
    }

    MACH_NLIST_64  SSSS = *Symbol;
    MachoRelocateSymbol64 (&Context, 0x100000000, &SSSS);
  }

  Symbol = MachoGetLocalDefinedSymbolByName64 (&Context, "_Assert");
  if (Symbol != NULL) {
    CONST CHAR8  *Indirect = MachoGetIndirectSymbolName64 (&Context, Symbol);
    if (  (AsciiStrCmp (MachoGetSymbolName64 (&Context, Symbol), "__hack") == 0)
       || ((Indirect != NULL) && (AsciiStrCmp (Indirect, "__hack") == 0)))
    {
      ++Code;
    }

    if (MachoSymbolIsSection64 (Symbol)) {
      ++Code;
    }

    if (MachoSymbolIsDefined64 (Symbol)) {
      ++Code;
    }

    if (MachoSymbolIsLocalDefined64 (&Context, Symbol)) {
      ++Code;
    }

    if (MachoIsSymbolValueInRange64 (&Context, Symbol)) {
      ++Code;
    }

    UINT32  Offset;
    if (MachoSymbolGetFileOffset64 (&Context, Symbol, &Offset, NULL)) {
      Code += Offset;
    }

    if (MachoSymbolNameIsPureVirtual (MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    if (MachoSymbolNameIsPadslot (MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    if (MachoSymbolNameIsSmcp (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    if (MachoSymbolNameIsMetaclassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }

    CHAR8  Out[64];
    if (  MachoSymbolNameIsSmcp (&Context, MachoGetSymbolName64 (&Context, Symbol))
       && MachoGetClassNameFromSuperMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (MachoSymbolNameIsVtable (MachoGetSymbolName64 (&Context, Symbol))) {
      if (AsciiStrCmp (MachoGetClassNameFromVtableName (MachoGetSymbolName64 (&Context, Symbol)), "sym") != 0) {
        ++Code;
      }
    }

    if (  MachoGetFunctionPrefixFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoSymbolNameIsMetaclassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol))
       && MachoGetClassNameFromMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoGetVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoGetMetaVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (  MachoGetFinalSymbolNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof (Out), Out)
       && (AsciiStrCmp ("SomeReallyLongStringJustInCaseToCheckIt", Out) == 0))
    {
      ++Code;
    }

    if (MachoSymbolNameIsCxx (MachoGetSymbolName64 (&Context, Symbol))) {
      ++Code;
    }
  }

  for (UINTN i = 0x1000000; i < MAX_UINTN; i += 0x1000000) {
    if (MachoGetSymbolByRelocationOffset64 (&Context, i, &Symbol)) {
      if (AsciiStrCmp (MachoGetSymbolName64 (&Context, Symbol), "__hack") == 0) {
        ++Code;
      }
    }
  }

  return Code != 963;
}

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  UINT32  FileSize;
  UINT8   *Buffer;

  if ((Buffer = UserReadFile ((argc > 1) ? argv[1] : "kernel", &FileSize)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail\n"));
    return -1;
  }

  return FeedMacho (Buffer, FileSize);
}

int
LLVMFuzzerTestOneInput (
  const uint8_t  *Data,
  size_t         Size
  )
{
  VOID  *NewData;

  if (Size > 0) {
    NewData = AllocatePool (Size);
    if (NewData != NULL) {
      CopyMem (NewData, Data, Size);
      FeedMacho (NewData, (UINT32)Size);
      FreePool (NewData);
    }
  }

  return 0;
}
