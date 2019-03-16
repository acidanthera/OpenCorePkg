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

#include <sys/time.h>

/*
 clang -g -fsanitize=undefined,address -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h Prelinked.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c  -o Prelinked

 for fuzzing:
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h Prelinked.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c  -o Prelinked
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
  uint32_t f;
  uint8_t *b;
  if ((b = readFile(argc > 1 ? argv[1] : "Prelinked.xml", &f)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  XML_DOCUMENT *doc = XmlDocumentParse((char *)b, f);

  if (doc == NULL) {
    printf("Parse fail\n");
    return -2;
  }

  UINT32 s;
  CHAR8 *buf = XmlDocumentExport(doc, &s);

  if (buf != NULL) {
    printf("Exported into %u bytes:\n\n\n", s);
    printf("%s\n", buf);
    FreePool (buf);
  } else {
    printf("Exporting gave NULL\n");
  }

  XmlDocumentFree (doc);

  free(b);

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

