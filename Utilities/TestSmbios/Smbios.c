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

#include <File.h>
#include <GlobalVar.h>
#include <BootServices.h>
#include <Pcd.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcMiscLib.h>
#include <IndustryStandard/AppleSmBios.h>

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 for fuzzing (TODO):
 clang-mp-7.0 -fshort-wchar -DCONFIG_TABLE_INSTALLER=NilInstallConfigurationTableCustom -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../EfiPkg/Include/ -I../../../MdePkg/Include/ -I../../../UefiCpuPkg/Include/ -include ../Include/Base.h Smbios.c ../../Library/OcSmbiosLib/DebugSmbios.c ../../Library/OcSmbiosLib/SmbiosInternal.c ../../Library/OcSmbiosLib/SmbiosPatch.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcMiscLib/LegacyRegionLock.c ../../Library/OcMiscLib/LegacyRegionUnlock.c ../../Library/OcCpuLib/OcCpuLib.c -o Smbios

 rm -rf DICT fuzz*.log ; mkdir DICT ; cp Smbios.bin DICT ; ./Smbios -jobs=4 DICT

 rm -rf Smbios.dSYM DICT fuzz*.log Smbios
*/

STATIC GUID SystemUUID = {0x5BC82C38, 0x4DB6, 0x4883, {0x85, 0x2E, 0xE7, 0x8D, 0x78, 0x0A, 0x6F, 0xE6}};
STATIC UINT8 BoardType = 0xA; // Motherboard (BaseBoardTypeMotherBoard)
STATIC UINT8 MemoryFormFactor = 0xD; // SODIMM, 0x9 for DIMM (MemoryFormFactorSodimm)
STATIC UINT8 ChassisType = 0xD; // All in one (MiscChassisTypeAllInOne)
STATIC UINT32 PlatformFeature = 1;
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
  .PlatformFeature = &PlatformFeature
};

bool doDump = false;

SMBIOS_TABLE_ENTRY_POINT        gSmbios;
SMBIOS_TABLE_3_0_ENTRY_POINT    gSmbios3;

int main(int argc, char** argv) {
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

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
  EFI_STATUS Status;
  Status = gBS->InstallConfigurationTable (&gEfiSmbios3TableGuid, &gSmbios3);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install gSmbios3 - %r", Status));
  }

  OC_CPU_INFO  CpuInfo;
  OcCpuScanProcessor (&CpuInfo);

  OC_SMBIOS_TABLE  SmbiosTable;
  Status = OcSmbiosTablePrepare (&SmbiosTable);
  if (!EFI_ERROR (Status)) {
    Status = OcSmbiosCreate (&SmbiosTable, &SmbiosData, OcSmbiosUpdateCreate, &CpuInfo);
    if (!EFI_ERROR (Status)) {
      SMBIOS_TABLE_3_0_ENTRY_POINT *patchedTablePtr = NULL;
      Status = EfiGetSystemConfigurationTable (&gEfiSmbios3TableGuid, (VOID **) &patchedTablePtr);
      if (doDump && !EFI_ERROR (Status)) {
        (void)remove("out.bin");
        FILE *fh = fopen("out.bin", "wb");
        if (fh != NULL) {
          fwrite((const void *) (uintptr_t) (patchedTablePtr->TableAddress), (size_t) (patchedTablePtr->TableMaximumSize), 1, fh);
          fclose(fh);
        } else {
          DEBUG ((DEBUG_ERROR, "Failed to produce out.bin - %r", Status));
        }
      } else {
        DEBUG ((DEBUG_ERROR, "EfiGetSystemConfigurationTable returns error - %r", Status));
      }
    } else {
      DEBUG ((DEBUG_ERROR, "OcSmbiosCreate returns error - %r", Status));
    }

    OcSmbiosTableFree (&SmbiosTable);
  } else {
    DEBUG ((DEBUG_ERROR, "Failed to prepare smbios table - %r", Status));
  }

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size > 0) {
    VOID *NewData = AllocatePool (Size);
    if (NewData) {
      CopyMem (NewData, Data, Size);

      gSmbios3.TableMaximumSize = Size;
      gSmbios3.TableAddress = (uintptr_t)NewData;
      gSmbios3.EntryPointLength = sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT);

      OC_CPU_INFO  CpuInfo;
      OcCpuScanProcessor (&CpuInfo);

      EFI_STATUS Status;
      OC_SMBIOS_TABLE  SmbiosTable;
      Status = OcSmbiosTablePrepare (&SmbiosTable);
      if (!EFI_ERROR (Status)) {
        OcSmbiosCreate (&SmbiosTable, &SmbiosData, OcSmbiosUpdateCreate, &CpuInfo);
        OcSmbiosTableFree (&SmbiosTable);
      }

      FreePool (NewData);
    }
  }
  return 0;
}
