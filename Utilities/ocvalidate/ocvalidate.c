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

#include <File.h>
#include <sys/time.h>

/*
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

unsigned int check_ACPI(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into ACPI checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_Booter(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into Booter checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_DeviceProperties(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into DeviceProperties checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_Kernel(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into Kernel checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_Misc(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into Misc checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_NVRAM(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into NVRAM checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_PlatformInfo(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into PlatformInfo checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int check_UEFI(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  DEBUG ((DEBUG_INFO, "config loaded into UEFI checker!\n"));

  if (ret != 0)
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ret, ret > 1 ? "errors" : "error"));
  return ret;
}

unsigned int (*checker_funcs[])(OC_GLOBAL_CONFIG *) = {
  check_ACPI,
  check_Booter,
  check_DeviceProperties,
  check_Kernel,
  check_Misc,
  check_NVRAM,
  check_PlatformInfo,
  check_UEFI
};
const size_t checker_funcs_size = sizeof(checker_funcs) / sizeof(checker_funcs[0]);

unsigned int check_config(OC_GLOBAL_CONFIG *Config) {
  unsigned int ret = 0;

  //
  // I don't think this is possible after OcConfigurationInit?
  //
  // if (!Config) {
  //   DEBUG ((DEBUG_ERROR, "Failed to load config!\n"));
  //   return 1;
  // }

  for (size_t i = 0; i < checker_funcs_size; i++) {
    ret |= (*checker_funcs[i])(Config);
  }

  return ret;
}

int main(int argc, char** argv) {
  uint32_t config_file_size;
  uint8_t  *config_file_buffer;
  if ((config_file_buffer = readFile(argc > 1 ? argv[1] : "config.plist", &config_file_size)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to read %a\n", argc > 1 ? argv[1] : "config.plist"));
    return -1;
  }

  long long a = current_timestamp();

  PcdGet8  (PcdDebugPropertyMask)         |= DEBUG_PROPERTY_DEBUG_CODE_ENABLED;
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  OC_GLOBAL_CONFIG   Config;
  EFI_STATUS         Status;
  Status = OcConfigurationInit (&Config, config_file_buffer, config_file_size);

  if (Status != EFI_SUCCESS) {
    printf("Invalid config\n");
    return -1;
  }

  unsigned int ret = check_config(&Config);
  if (ret == 0) {
    DEBUG ((DEBUG_ERROR, "Done checking %a in %llu ms\n", argc > 1 ? argv[1] : "./config.plist", current_timestamp() - a));
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "Done checking %a in %llu ms, but it has %u %a to be fixed\n",
      argc > 1 ? argv[1] : "./config.plist",
      current_timestamp() - a,
      ret,
      ret > 1 ? "errors" : "error"
      ));
  }

  OcConfigurationFree (&Config);
  free(config_file_buffer);

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
