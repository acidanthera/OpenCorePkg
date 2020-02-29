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
 clang -g -fshort-wchar -DCONFIG_TABLE_INSTALLER=NilInstallConfigurationTableCustom -fsanitize=undefined,address -I../Include -I../../Include -I../../../EfiPkg/Include/ -I../../../MdePkg/Include/ -I../../../UefiCpuPkg/Include/ -include ../Include/Base.h Smbios.c ../../Library/OcSmbiosLib/DebugSmbios.c ../../Library/OcSmbiosLib/SmbiosInternal.c ../../Library/OcSmbiosLib/SmbiosPatch.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcMiscLib/LegacyRegionLock.c ../../Library/OcMiscLib/LegacyRegionUnlock.c ../../Library/OcCpuLib/OcCpuLib.c -o Smbios

 for fuzzing:
 clang-mp-7.0 -fshort-wchar -DCONFIG_TABLE_INSTALLER=NilInstallConfigurationTableCustom -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../EfiPkg/Include/ -I../../../MdePkg/Include/ -I../../../UefiCpuPkg/Include/ -include ../Include/Base.h Smbios.c ../../Library/OcSmbiosLib/DebugSmbios.c ../../Library/OcSmbiosLib/SmbiosInternal.c ../../Library/OcSmbiosLib/SmbiosPatch.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcMiscLib/LegacyRegionLock.c ../../Library/OcMiscLib/LegacyRegionUnlock.c ../../Library/OcCpuLib/OcCpuLib.c -o Smbios

 rm -rf DICT fuzz*.log ; mkdir DICT ; cp Smbios.bin DICT ; ./Smbios -jobs=4 DICT

 rm -rf Smbios.dSYM DICT fuzz*.log Smbios
*/

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

EFI_GUID gEfiLegacyRegionProtocolGuid;
EFI_GUID gEfiPciRootBridgeIoProtocolGuid;
EFI_GUID gEfiSmbios3TableGuid;
EFI_GUID gEfiSmbiosTableGuid;
EFI_GUID gOcCustomSmbiosTableGuid;

STATIC GUID SystemUUID = {0x5BC82C38, 0x4DB6, 0x4883, {0x85, 0x2E, 0xE7, 0x8D, 0x78, 0x0A, 0x6F, 0xE6}};
STATIC UINT8 BoardType = 0xA; // Motherboard (BaseBoardTypeMotherBoard)
STATIC UINT8 MemoryFormFactor = 0xD; // SODIMM, 0x9 for DIMM (MemoryFormFactorSodimm)
STATIC UINT8 ChassisType = 0xD; // All in one (MiscChassisTypeAllInOne)
STATIC OC_SMBIOS_DATA SmbiosData = {
  .BIOSVendor = NULL, // Do not change BIOS Vendor
  .BIOSVersion = "134.0.0.0.0",
  .BIOSReleaseDate = "12/08/2017",
  .SystemManufacturer = NULL, // Do not change System Manufacturer
  .SystemProductName = "iMac14,2",
  .SystemVersion = "1.0",
  .SystemSerialNumber = "SU77OPENCORE",
  .SystemUUID = &SystemUUID,
  .SystemSKUNumber = "Mac-27ADBB7B4CEE8E61",
  .SystemFamily = "iMac",
  .BoardManufacturer = NULL, // Do not change Board Manufacturer
  .BoardProduct = "Mac-27ADBB7B4CEE8E61",
  .BoardVersion = "iMac14,2",
  .BoardSerialNumber = "SU77PEPELATZWAFFE",
  .BoardAssetTag = "",
  .BoardLocationInChassis = "Part Component",
  .BoardType = &BoardType,
  .MemoryFormFactor = &MemoryFormFactor,
  .ChassisType = &ChassisType,
  .ChassisManufacturer = NULL, // Do not change Chassis Manufacturer
  .ChassisVersion = "Mac-27ADBB7B4CEE8E61",
  .ChassisSerialNumber = "SU77OPENCORE",
  .ChassisAssetTag = "iMac-Aluminum",
  .FirmwareFeatures = 0xE00FE137,
  .FirmwareFeaturesMask = 0xFF1FFF3F,
  .ProcessorType = NULL, // Will be calculated automatically
  .PlatformFeature = 1
};


bool doDump = false;
_Thread_local uint32_t externalUsedPages = 0;
_Thread_local uint8_t externalBlob[EFI_PAGE_SIZE*TOTAL_PAGES];

_Thread_local SMBIOS_TABLE_ENTRY_POINT        gSmbios;
_Thread_local SMBIOS_TABLE_3_0_ENTRY_POINT    gSmbios3;

EFI_STATUS EfiGetSystemConfigurationTable (EFI_GUID *TableGuid, OUT VOID **Table) {
  /*if (Table && TableGuid == &gEfiSmbiosTableGuid) {
    *Table = &gSmbios;
    return EFI_SUCCESS;
  } else*/ if (Table && TableGuid == &gEfiSmbios3TableGuid) {
    *Table = &gSmbios3;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS NilInstallConfigurationTableCustom(EFI_GUID *Guid, VOID *Table) {
  printf("Set table %p, looking for %p\n", Guid, &gEfiSmbios3TableGuid);
  if (Guid == &gEfiSmbios3TableGuid && doDump) {
    SMBIOS_TABLE_3_0_ENTRY_POINT  *Ep = (SMBIOS_TABLE_3_0_ENTRY_POINT *) Table;
    (void)remove("out.bin");
    FILE *fh = fopen("out.bin", "wb");
    if (fh != NULL) {
      fwrite((void *)Ep->TableAddress, Ep->TableMaximumSize, 1, fh);
      fclose(fh);
    }
  }
  return EFI_SUCCESS;
}

int main(int argc, char** argv) {
  uint32_t f;
  uint8_t *b;
  if ((b = readFile(argc > 1 ? argv[1] : "Smbios.bin", &f)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  doDump = true;
  gSmbios3.TableMaximumSize = f;
  gSmbios3.TableAddress = (uintptr_t)b;
  gSmbios3.EntryPointLength = sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT);

  OC_CPU_INFO  CpuInfo;
  OcCpuScanProcessor (&CpuInfo);

  CreateSmbios (
    &SmbiosData,
    1,
    &CpuInfo
    );

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size > 0) {
    VOID *NewData = AllocatePool (Size);
    if (NewData) {
      CopyMem (NewData, Data, Size);

      externalUsedPages = 0;
      gSmbios3.TableMaximumSize = Size;
      gSmbios3.TableAddress = (uintptr_t)NewData;
      gSmbios3.EntryPointLength = sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT);

      OC_CPU_INFO  CpuInfo;
      OcCpuScanProcessor (&CpuInfo);

      CreateSmbios (
        &SmbiosData,
        0,
        &CpuInfo
        );

      FreePool (NewData);
    }
  }
  return 0;
}
