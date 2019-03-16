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

#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcAppleKernelLib.h>

#include <sys/time.h>

/*
 clang -g -fsanitize=undefined,address -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h Prelinked.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcMachoLib/CxxSymbols.c ../../Library/OcMachoLib/Header.c ../../Library/OcMachoLib/Relocations.c ../../Library/OcMachoLib/Symbols.c ../../Library/OcAppleKernelLib/Prelinked.c -o Prelinked

 for fuzzing:
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h Prelinked.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcMachoLib/CxxSymbols.c ../../Library/OcMachoLib/Header.c ../../Library/OcMachoLib/Relocations.c ../../Library/OcMachoLib/Symbols.c ../../Library/OcAppleKernelLib/Prelinked.c -o Prelinked
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp Prelinked.plist DICT ; ./Prelinked -jobs=4 DICT

 rm -rf Prelinked.dSYM DICT fuzz*.log Prelinked
*/

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

uint8_t *readFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = 0;
  *size = fsize;

  return string;
}

int main(int argc, char** argv) {
  UINT32 Size;
  UINT8  *Prelinked;
  PRELINKED_CONTEXT Context;
  if ((Prelinked = readFile(argc > 1 ? argv[1] : "prelinkedkernel.unpack", &Size)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  EFI_STATUS Status = PrelinkedContextInit (&Context, Prelinked, Size, Size);

  if (!EFI_ERROR (Status)) {
    PrelinkedDropPlistInfo (&Context);

    Status = PrelinkedInsertPlistInfo (&Context);

    if (EFI_ERROR (Status)) {
      printf("Plist insert error %zx\n", Status);
    }

    FILE *Fh = fopen("out.bin", "wb");

    if (Fh != NULL) {
      fwrite (Prelinked, Context.PrelinkedSize, 1, Fh);
      fclose(Fh);
    }

    PrelinkedContextFree (&Context);
  } else {
    printf("Context creation error %zx\n", Status);
  }

  free(Prelinked);

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  VOID *NewData = AllocatePool (Size);
  if (NewData) {
    CopyMem (NewData, Data, Size);
    XML_DOCUMENT *doc = XmlDocumentParse((char *)NewData, Size);
    if (doc != NULL) {
      UINT32 s;
      CHAR8 *buf = XmlDocumentExport(doc, &s);
      if (buf != NULL) {
        FreePool (buf);
      }
      XmlDocumentFree (doc);
    }
    FreePool (NewData);
  }
  return 0;
}

