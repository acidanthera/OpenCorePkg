/** @file
  Dump memory map and memory attributes.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Protocol/OcFirmwareRuntime.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime;
  EFI_PHYSICAL_ADDRESS          Address;
  UINTN                         Pages;
  UINTN                         MemoryMapSize;
  UINTN                         OriginalSize;
  UINTN                         DescriptorSize;
  EFI_MEMORY_DESCRIPTOR         *MemoryMap;

  Address = 0;
  Status = gBS->LocateProtocol (&gOcFirmwareRuntimeProtocolGuid, NULL, (VOID **) &FwRuntime);
  if (!EFI_ERROR (Status) && FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION) {
    Status = FwRuntime->GetExecArea (&Address, &Pages);
    DEBUG ((
      DEBUG_WARN,
      "MMDD: OpenRuntime r%u resides at %X - %r\n",
      (UINT32) FwRuntime->Revision,
      (UINT32) Address,
      Status
      ));
  } else if (!EFI_ERROR (Status) && FwRuntime->Revision != OC_FIRMWARE_RUNTIME_REVISION) {
    DEBUG ((
      DEBUG_WARN,
      "MMDD: OpenRuntime has unexpected revision r%u instead of r%u\n",
      (UINT32) FwRuntime->Revision,
      (UINT32) OC_FIRMWARE_RUNTIME_REVISION
      ));
  } else {
    DEBUG ((
      DEBUG_WARN,
      "MMDD: OpenRuntime is missing - %r\n",
      Status
      ));
  }

  DEBUG ((
    DEBUG_WARN,
    "MMDD: Note, that DEBUG version of the tool prints more\n"
    ));

  MemoryMap = OcGetCurrentMemoryMap (
    &MemoryMapSize,
    &DescriptorSize,
    NULL,
    NULL,
    &OriginalSize,
    TRUE
    );

  if (MemoryMap != NULL) {
    OcPrintMemoryAttributesTable ();
    OcSortMemoryMap (MemoryMapSize, MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_INFO, "MMDD: Dumping the original memory map\n"));
    OcPrintMemoryMap (MemoryMapSize, MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_INFO, "MMDD: Dumping patched attributes\n"));
    OcRebuildAttributes (Address, NULL);
    Status = OcSplitMemoryMapByAttributes (OriginalSize, &MemoryMapSize, MemoryMap, DescriptorSize);
    if (!EFI_ERROR (Status)) {
      OcPrintMemoryAttributesTable ();
      DEBUG ((DEBUG_INFO, "MMDD: Dumping patched memory map\n"));
      OcPrintMemoryMap (MemoryMapSize, MemoryMap, DescriptorSize);
    } else {
      DEBUG ((DEBUG_INFO, "MMDD: Cannot patch memory map - %r\n", Status));
    }
    DEBUG ((DEBUG_INFO, "MMDD: Dumping shrinked memory map\n"));
    OcShrinkMemoryMap (&MemoryMapSize, MemoryMap, DescriptorSize);
    OcPrintMemoryMap (MemoryMapSize, MemoryMap, DescriptorSize);
    FreePool (MemoryMap);
  } else {
    DEBUG ((DEBUG_INFO, "MMDD: Unable to obtain memory map\n"));
  }

  gBS->Stall (SECONDS_TO_MICROSECONDS (3));

  return EFI_SUCCESS;
}
