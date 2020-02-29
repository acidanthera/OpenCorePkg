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

#include <Library/OcSmbiosLib.h>
#include <Library/OcMiscLib.h>
#include <IndustryStandard/AppleSmBios.h>

#include <sys/time.h>

/*
TODO: ...

 clang -g -fsanitize=undefined,address -fshort-wchar -I../Include -I../../Include -I../../../EfiPkg/Include/ -I../../../EfiPkg/Include/X64 -I../../../MdePkg/Include/ -I../../../UefiCpuPkg/Include/ -include ../Include/Base.h Props.c ../../Library/OcDevicePropertyLib/OcDevicePropertyLib.c ../../Library/OcStringLib/OcAsciiLib.c ../../../MdePkg/Library/UefiDevicePathLib/DevicePathUtilities.c ../../../MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.c ../../../MdePkg/Library/UefiDevicePathLib/DevicePathToText.c -o Props

 for fuzzing:
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -fshort-wchar -I../Include -I../../Include -I../../../EfiPkg/Include/ -I../../../EfiPkg/Include/X64 -I../../../MdePkg/Include/ -I../../../UefiCpuPkg/Include/ -include ../Include/Base.h Props.c ../../Library/OcDevicePropertyLib/OcDevicePropertyLib.c ../../Library/OcStringLib/OcAsciiLib.c ../../../MdePkg/Library/UefiDevicePathLib/DevicePathUtilities.c ../../../MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.c ../../../MdePkg/Library/UefiDevicePathLib/DevicePathToText.c -o Props

 rm -rf DICT fuzz*.log ; mkdir DICT ; cp Props.bin DICT ; ./Props -jobs=4 DICT

 rm -rf Props.dSYM DICT fuzz*.log Props
*/

EFI_GUID gAppleVendorVariableGuid;
EFI_GUID gEfiDevicePathPropertyDatabaseProtocolGuid;
EFI_GUID gEfiLegacyRegionProtocolGuid;
EFI_GUID gEfiDebugPortProtocolGuid;
EFI_GUID gEfiDevicePathFromTextProtocolGuid;
EFI_GUID gEfiDevicePathProtocolGuid;
EFI_GUID gEfiDevicePathToTextProtocolGuid;
EFI_GUID gEfiDevicePathUtilitiesProtocolGuid;
EFI_GUID gEfiPcAnsiGuid;
EFI_GUID gEfiPersistentVirtualCdGuid;
EFI_GUID gEfiPersistentVirtualDiskGuid;
EFI_GUID gEfiSasDevicePathGuid;
EFI_GUID gEfiUartDevicePathGuid;
EFI_GUID gEfiVT100Guid;
EFI_GUID gEfiVT100PlusGuid;
EFI_GUID gEfiVTUTF8Guid;
EFI_GUID gEfiVirtualCdGuid;
EFI_GUID gEfiVirtualDiskGuid;

_Thread_local uint32_t externalUsedPages = 0;
_Thread_local uint8_t externalBlob[EFI_PAGE_SIZE*TOTAL_PAGES];

int main(int argc, char** argv) {

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size > 0) {
    VOID *NewData = AllocatePool (Size);
    if (NewData) {
      CopyMem (NewData, Data, Size);

      

      FreePool (NewData);
    }
  }
  return 0;
}
