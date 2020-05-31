/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Guid/SmBios.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "SmbiosInternal.h"
#include "DebugSmbios.h"

EFI_STATUS
OcSmbiosDump (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS                      Status;
  SMBIOS_TABLE_ENTRY_POINT        *OriginalSmbios;
  SMBIOS_TABLE_3_0_ENTRY_POINT    *OriginalSmbios3;

  Status  = EfiGetSystemConfigurationTable (
    &gEfiSmbiosTableGuid,
    (VOID **) &OriginalSmbios
    );

  if (!EFI_ERROR (Status)) {
    Status = SetFileData (Root, L"EntryV1.bin", OriginalSmbios, OriginalSmbios->EntryPointLength);
    DEBUG ((DEBUG_INFO, "OCSMB: Dumped V1 EP (%u bytes) - %r\n", OriginalSmbios->EntryPointLength, Status));
    Status = SetFileData (Root, L"DataV1.bin", (VOID *) (UINTN) OriginalSmbios->TableAddress, OriginalSmbios->TableLength);
    DEBUG ((DEBUG_INFO, "OCSMB: Dumped V1 DATA (%u bytes) - %r\n", OriginalSmbios->TableLength, Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCSMB: No SMBIOS V1 - %r\n", Status));
  }

  Status  = EfiGetSystemConfigurationTable (
    &gEfiSmbios3TableGuid,
    (VOID **) &OriginalSmbios3
    );

  if (!EFI_ERROR (Status)) {
    Status = SetFileData (Root, L"EntryV3.bin", OriginalSmbios3, OriginalSmbios3->EntryPointLength);
    DEBUG ((DEBUG_INFO, "OCSMB: Dumped V3 EP (%u bytes) - %r\n", OriginalSmbios3->EntryPointLength, Status));
    Status = SetFileData (Root, L"DataV3.bin", (VOID *) (UINTN) OriginalSmbios3->TableAddress, OriginalSmbios3->TableMaximumSize);
    DEBUG ((DEBUG_INFO, "OCSMB: Dumped V3 DATA (%u bytes) - %r\n", OriginalSmbios3->TableMaximumSize, Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCSMB: No SMBIOS V3 - %r\n", Status));
  }

  return EFI_SUCCESS;
}
