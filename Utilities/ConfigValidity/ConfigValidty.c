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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcConfigurationLib.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

/*
  clang -g \
  -fsanitize=undefined,address \
  -fshort-wchar \
  -D NO_MSABI_VA_FUNCS \
  -I../../Include/Acidanthera \
  -I../../Include/Apple \
  -I../../Include/Generic \
  -I../../User/Include \
  -I../../UDK/MdePkg/Include \
  -I../../UDK/MdePkg/Include/X64 \
  -include ../../User/Include/Pcd.h \
  -include ../../User/Include/EfiVar.h \
  ConfigValidty.c \
  ../../User/Library/BaseMemoryLib.c \
  ../../User/Library/Pcd.c \
  ../../User/Library/EfiVar.c \
  ../../User/Library/DebugBreak.c \
  ../../User/Library/Math.c \
  ../../UDK/MdePkg/Library/UefiDebugLibConOut/DebugLib.c \
  ../../UDK/MdePkg/Library/UefiDebugLibConOut/DebugLibConstructor.c \
  ../../UDK/MdePkg/Library/BaseLib/CpuDeadLoop.c \
  ../../UDK/MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.c \
  ../../UDK/MdePkg/Library/BasePrintLib/PrintLib.c \
  ../../UDK/MdePkg/Library/BasePrintLib/PrintLibInternal.c \
  ../../UDK/MdePkg/Library/BaseLib/SafeString.c \
  ../../UDK/MdePkg/Library/BaseLib/String.c \
  ../../UDK/MdePkg/Library/BaseLib/SwapBytes16.c \
  ../../UDK/MdePkg/Library/BaseLib/SwapBytes32.c \
  ../../Library/OcConfigurationLib/OcConfigurationLib.c \
  ../../Library/OcTemplateLib/OcTemplateLib.c \
  ../../Library/OcSerializeLib/OcSerializeLib.c \
  ../../Library/OcXmlLib/OcXmlLib.c \
  ../../Library/OcStringLib/OcAsciiLib.c \
  -o ConfigValidty

 for fuzzing (TODO):
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h ConfigValidty.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcConfigurationLib/OcConfigurationLib.c -o ConfigValidty
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp ConfigValidty.plist DICT ; ./ConfigValidty -jobs=4 DICT

 rm -rf ConfigValidty.dSYM DICT fuzz*.log ConfigValidty
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
  if ((b = readFile(argc > 1 ? argv[1] : "config.plist", &f)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  long long a = current_timestamp();

  OC_GLOBAL_CONFIG   Config;
  EFI_STATUS Status = OcConfigurationInit (&Config, b, f);

  DEBUG((EFI_D_ERROR, "Done in %llu ms\n", current_timestamp() - a));

  OcConfigurationFree (&Config);

  free(b);

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  VOID *NewData = AllocatePool (Size);
  if (NewData) {
    CopyMem (NewData, Data, Size);
    OC_GLOBAL_CONFIG   Config;
    OcConfigurationInit (&Config, NewData, Size);
    OcConfigurationFree (&Config);
    FreePool (NewData);
  }
  return 0;
}

