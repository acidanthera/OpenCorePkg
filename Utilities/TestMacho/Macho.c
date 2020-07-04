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
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>

#include <string.h>
#include <sys/time.h>

#include <File.h>

/*
 for fuzzing (TODO):
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h -I../../../EfiPkg/Include/ Macho.c  ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c  ../../Library/OcMachoLib/CxxSymbols.c ../../Library/OcMachoLib/Header.c ../../Library/OcMachoLib/Relocations.c ../../Library/OcMachoLib/Symbols.c -o Macho
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp /System/Library/Kernels/kernel DICT ; ./Macho -rss_limit_mb=4096M -jobs=4 DICT

 rm -rf fuzz*.log ; mkdir -p DICT ; cp /System/Library/Kernels/kernel DICT/kernel ; ./Macho -jobs=4 -rss_limit_mb=4096M DICT

 rm -rf Macho.dSYM DICT fuzz*.log Macho
*/

MACH_HEADER_64 Header;
MACH_SECTION_64 Sect;
MACH_SEGMENT_COMMAND_64 Seg;
MACH_UUID_COMMAND Uuid;

static int FeedMacho(void *file, uint32_t size) {
  OC_MACHO_CONTEXT Context;
  if (!MachoInitializeContext (&Context, file, size, 0)) {
    return -1;
  }

  int code = 0;

  MACH_HEADER_64 *Hdr = MachoGetMachHeader64 (&Context);
  if (Hdr && MachoGetFileSize(&Context) > 10 && MachoGetLastAddress64(&Context) != 10) {
    memcpy(&Header, Hdr, sizeof(Header));
    code++;
  }

  MACH_UUID_COMMAND *Cmd = MachoGetUuid64(&Context);
  if (Cmd) {
    memcpy(&Uuid, Cmd, sizeof(Uuid));
    code++;
  }

  MACH_SEGMENT_COMMAND_64 *Segment = MachoGetSegmentByName64(&Context, "__LINKEDIT");
  MACH_SECTION_64 *Section;
  if (Segment) {
    memcpy(&Seg, Segment, sizeof(Seg));
    Section = MachoGetSectionByName64 (&Context, Segment, "__objc");
    if (Section) {
      memcpy(&Sect, Section, sizeof(Sect));
      code++;
    }
  }

  uint32_t index = 0;
  while ((Section = MachoGetSectionByIndex64 (&Context, index))) {
    memcpy(&Sect, Section, sizeof(Sect));
    index++;
  }

  if ((Section = MachoGetSectionByAddress64 (&Context, index))) {
    memcpy(&Sect, Section, sizeof(Sect));
    code++;
  }

  MACH_NLIST_64 *Symbol = NULL;
  for (index = 0; (Symbol = MachoGetSymbolByIndex64 (&Context, index)) != NULL; index++) {
    CONST CHAR8 *Indirect = MachoGetIndirectSymbolName64 (&Context, Symbol);
    if (!AsciiStrCmp (MachoGetSymbolName64 (&Context, Symbol), "__hack") ||
      (Indirect && !AsciiStrCmp (Indirect, "__hack"))) {
      code++;
    }
    if (MachoSymbolIsSection (Symbol)) {
      code++;
    }
    if (MachoSymbolIsDefined (Symbol)) {
      code++;
    }
    if (MachoSymbolIsLocalDefined (&Context, Symbol)) {
      code++;
    }

    if (MachoIsSymbolValueInRange64 (&Context, Symbol)) {
      code++;
    }

    UINT32 Offset;
    if (MachoSymbolGetFileOffset64 (&Context, Symbol, &Offset, NULL)) {
      code += Offset;
    }

    if (MachoSymbolNameIsPureVirtual (MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    if (MachoSymbolNameIsPadslot (MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    if (MachoSymbolNameIsSmcp64 (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    if (MachoSymbolNameIsMetaclassPointer64 (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    char out[64];
    if (MachoSymbolNameIsSmcp64 (&Context, MachoGetSymbolName64 (&Context, Symbol))
      && MachoGetClassNameFromSuperMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)) {
      code++;
    }

    if (MachoSymbolNameIsVtable64 (MachoGetSymbolName64 (&Context, Symbol))) {
      if (AsciiStrCmp(MachoGetClassNameFromVtableName (MachoGetSymbolName64 (&Context, Symbol)), "sym")) {
        code++;
      }
    }

    if (MachoGetFunctionPrefixFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoSymbolNameIsMetaclassPointer64 (&Context, MachoGetSymbolName64 (&Context, Symbol))
      && MachoGetClassNameFromMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoGetVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoGetMetaVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoGetFinalSymbolNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoSymbolNameIsCxx (MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    MACH_NLIST_64 *SCMP = MachoGetMetaclassSymbolFromSmcpSymbol64 (&Context, Symbol);
    if (SCMP) {
      if (!AsciiStrCmp (MachoGetSymbolName64 (&Context, SCMP), "__hack")) {
        code++;
      }

      CONST MACH_NLIST_64  *Vtable;
      CONST MACH_NLIST_64  *MetaVtable;
      if (MachoGetVtableSymbolsFromSmcp64 (&Context, MachoGetSymbolName64 (&Context, SCMP), &Vtable, &MetaVtable)) {
        if (!AsciiStrCmp (MachoGetSymbolName64 (&Context, Vtable), "__hack")) {
          code++;
        }
        if (!AsciiStrCmp (MachoGetSymbolName64 (&Context, MetaVtable), "__hack")) {
          code++;
        }
      }
    }

    MACH_NLIST_64 SSSS = *Symbol;
    MachoRelocateSymbol64 (&Context, 0x100000000, &SSSS);
  }

  Symbol = MachoGetLocalDefinedSymbolByName (&Context, "_Assert");
  if (Symbol) {
    CONST CHAR8 *Indirect = MachoGetIndirectSymbolName64 (&Context, Symbol);
    if (!AsciiStrCmp (MachoGetSymbolName64 (&Context, Symbol), "__hack") ||
      (Indirect && !AsciiStrCmp (Indirect, "__hack"))) {
      code++;
    }
    if (MachoSymbolIsSection (Symbol)) {
      code++;
    }
    if (MachoSymbolIsDefined (Symbol)) {
      code++;
    }
    if (MachoSymbolIsLocalDefined (&Context, Symbol)) {
      code++;
    }

    if (MachoIsSymbolValueInRange64 (&Context, Symbol)) {
      code++;
    }
    UINT32 Offset;
    if (MachoSymbolGetFileOffset64 (&Context, Symbol, &Offset, NULL)) {
      code += Offset;
    }

    if (MachoSymbolNameIsPureVirtual (MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    if (MachoSymbolNameIsPadslot (MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    if (MachoSymbolNameIsSmcp64 (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    if (MachoSymbolNameIsMetaclassPointer64 (&Context, MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }

    char out[64];
    if (MachoSymbolNameIsSmcp64 (&Context, MachoGetSymbolName64 (&Context, Symbol))
      && MachoGetClassNameFromSuperMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoSymbolNameIsVtable64 (MachoGetSymbolName64 (&Context, Symbol))) {
      if (AsciiStrCmp(MachoGetClassNameFromVtableName (MachoGetSymbolName64 (&Context, Symbol)), "sym")) {
        code++;
      }
    }

    if (MachoGetFunctionPrefixFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoSymbolNameIsMetaclassPointer64 (&Context, MachoGetSymbolName64 (&Context, Symbol))
      && MachoGetClassNameFromMetaClassPointer (&Context, MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoGetVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoGetMetaVtableNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoGetFinalSymbolNameFromClassName (MachoGetSymbolName64 (&Context, Symbol), sizeof(out), out)
      && !AsciiStrCmp("SomeReallyLongStringJustInCaseToCheckIt", out)) {
      code++;
    }

    if (MachoSymbolNameIsCxx (MachoGetSymbolName64 (&Context, Symbol))) {
      code++;
    }
  }

  for (size_t i = 0x1000000; i < MAX_UINTN; i+= 0x1000000) {
    if (MachoGetSymbolByRelocationOffset64 (&Context, i, &Symbol)) {
      if (!AsciiStrCmp (MachoGetSymbolName64 (&Context, Symbol), "__hack")) {
        code++;
      }
    }
  }

  return code != 963;
}

int main(int argc, char** argv) {
  uint32_t f;
  uint8_t *b;
  if ((b = readFile(argc > 1 ? argv[1] : "kernel", &f)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  return FeedMacho (b, f);
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size > 0) {
    VOID *NewData = AllocatePool (Size);
    if (NewData) {
      CopyMem (NewData, Data, Size);
      FeedMacho (NewData, (UINT32) Size);
      FreePool (NewData);
    }
  }
  return 0;
}
