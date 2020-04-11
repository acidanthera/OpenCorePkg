/**
  Copyright (C) 2019, Download-Fritz. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>

#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Unblocks all partition handles without a File System protocol attached from
  driver connection, if applicable.

**/
VOID
OcUnblockUnmountedPartitions (
  VOID
  )
{
  EFI_STATUS                          Status;

  UINTN                               NumHandles;
  EFI_HANDLE                          *Handles;
  UINTN                               HandleIndex;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL     *FileSystem;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;

  UINTN                               NumBlockIoInfo;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *BlockIoInfos;
  UINTN                               BlockIoInfoIndex;
  //
  // For all Block I/O handles, check whether it is a partition. If it is and
  // does not have a File System protocol attached, ensure it does not have a
  // blocking driver attached which prevents the connection of a FS driver.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCFSQ: Could not locate DiskIo handles\n"));
    return;
  }

  for (HandleIndex = 0; HandleIndex < NumHandles; ++HandleIndex) {
    //
    // Skip the current handle if a File System driver is already attached.
    //
    Status = gBS->HandleProtocol (
                    Handles[HandleIndex],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **) &FileSystem
                    );
    if (!EFI_ERROR (Status)) {
      continue;
    }
    //
    // Ensure the current handle describes a partition.
    //
    Status = gBS->HandleProtocol (
                    Handles[HandleIndex],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo
                    );
    if (EFI_ERROR (Status) || !BlockIo->Media->LogicalPartition) {
      continue;
    }
    //
    // Disconnect any blocking drivers if applicable.
    //
    Status = gBS->OpenProtocolInformation (
                    Handles[HandleIndex],
                    &gEfiBlockIoProtocolGuid,
                    &BlockIoInfos,
                    &NumBlockIoInfo
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCFSQ: Attached drivers could not been retrieved\n"));
      continue;
    }

    for (BlockIoInfoIndex = 0; BlockIoInfoIndex < NumBlockIoInfo; ++BlockIoInfoIndex) {
      if ((BlockIoInfos[BlockIoInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
        Status = gBS->DisconnectController (
                        Handles[HandleIndex],
                        BlockIoInfos[BlockIoInfoIndex].AgentHandle,
                        NULL
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "OCFSQ: Failed to unblock handle %p - %r\n",
            Handles[HandleIndex],
            Status
            ));
        }
      }
    }

    FreePool (BlockIoInfos);
  }

  FreePool (Handles);
}
