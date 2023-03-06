/** @file
  List available partitions on the system.

Copyright (c) 2021-2023, Mikhail Krichanov. All rights reserved.<BR>
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
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Protocol/BlockIo.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  UINTN                      HandleCount;
  EFI_HANDLE                 *Handles;
  EFI_BLOCK_IO_PROTOCOL      *BlkIo;
  CONST EFI_PARTITION_ENTRY  *PartitionEntry;

  //
  // Find HardDrive
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to find any partitions - %r\n", Status));
    return Status;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **)&BlkIo
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Partition #%u is unavailable - %r\n", (UINT32)(Index + 1), Status));
      continue;
    }

    if (!BlkIo->Media->LogicalPartition) {
      DEBUG ((DEBUG_WARN, "Partition #%u is not a logical partition\n", (UINT32)(Index + 1)));
      continue;
    }

    PartitionEntry = OcGetGptPartitionEntry (Handles[Index]);
    if (PartitionEntry == NULL) {
      DEBUG ((DEBUG_WARN, "Partition #%u GPT entry is not accessible\n", (UINT32)(Index + 1)));
      continue;
    }

    if (IsZeroGuid (&PartitionEntry->UniquePartitionGUID)) {
      DEBUG ((DEBUG_WARN, "Partition #%u GPT entry has all-zero GUID\n", (UINT32)(Index + 1)));
      continue;
    }

    DEBUG ((DEBUG_WARN, "Partition #%u details:\n", (UINT32)(Index + 1)));
    DEBUG ((DEBUG_WARN, "  PartitionName      : %-36s\n", PartitionEntry->PartitionName));
    DEBUG ((DEBUG_WARN, "  PartitionTypeGUID  : %g\n", &PartitionEntry->PartitionTypeGUID));
    DEBUG ((DEBUG_WARN, "  UniquePartitionGUID: %g\n", &PartitionEntry->UniquePartitionGUID));
  }

  FreePool (Handles);

  return EFI_SUCCESS;
}
