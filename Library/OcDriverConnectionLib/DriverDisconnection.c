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

#include <IndustryStandard/Pci.h>

#include <Protocol/AudioDecode.h>
#include <Protocol/BlockIo.h>
#include <Protocol/HdaControllerInfo.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/OcHdaDevicesLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
OcDisconnectDriversOnHandle (
  IN EFI_HANDLE  Controller
  )
{
  EFI_STATUS                          Status;
  UINTN                               NumBlockIoInfo;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *BlockIoInfos;
  UINTN                               BlockIoInfoIndex;

  //
  // Disconnect any blocking drivers if applicable.
  //
  Status = gBS->OpenProtocolInformation (
    Controller,
    &gEfiBlockIoProtocolGuid,
    &BlockIoInfos,
    &NumBlockIoInfo
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDC: Attached drivers could not been retrieved\n"));
    return Status;
  }

  for (BlockIoInfoIndex = 0; BlockIoInfoIndex < NumBlockIoInfo; ++BlockIoInfoIndex) {
    if ((BlockIoInfos[BlockIoInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
      Status = gBS->DisconnectController (
        Controller,
        BlockIoInfos[BlockIoInfoIndex].AgentHandle,
        NULL
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_INFO,
          "OCDC: Failed to unblock handle %p - %r\n",
          Controller,
          Status
          ));
      }
    }
  }

  FreePool (BlockIoInfos);
  return EFI_SUCCESS;
}

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
    DEBUG ((DEBUG_INFO, "OCDC: Could not locate DiskIo handles\n"));
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

    OcDisconnectDriversOnHandle (Handles[HandleIndex]);
  }

  FreePool (Handles);
}

VOID
OcDisconnectGraphicsDrivers (
  VOID
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;

  UINTN                   HandleCount;
  EFI_HANDLE              *HandleBuffer;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  PCI_TYPE00              Pci;

  //
  // Locate all currently connected PCI I/O protocols and disconnect graphics drivers.
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDC: Found %u handles with PCI I/O\n", (UINT32) HandleCount));
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiPciIoProtocolGuid,
        (VOID **) &PciIo
        );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = PciIo->Pci.Read (
        PciIo,
        EfiPciIoWidthUint32,
        0,
        sizeof (Pci) / sizeof (UINT32),
        &Pci
        );
      if (!EFI_ERROR (Status)) {
        if (IS_PCI_VGA (&Pci) == TRUE) {
          Status = gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
          DEBUG ((DEBUG_INFO, "OCDC: Disconnected graphics driver handle %u - %p result - %r\n", Index, HandleBuffer[Index], Status));
        }
      }
    }

    FreePool (HandleBuffer);
  }
}

VOID
OcDisconnectHdaControllers (
  VOID
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;

  UINTN                   HandleCount;
  EFI_HANDLE              *HandleBuffer;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  PCI_CLASSCODE           HdaClassReg;

  //
  // Locate all currently connected PCI I/O protocols and disconnect HDA controllers.
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDC: Found %u handles with PCI I/O\n", (UINT32) HandleCount));
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiPciIoProtocolGuid,
        (VOID **) &PciIo
        );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Read class code from PCI.
      //
      Status = PciIo->Pci.Read (
        PciIo,
        EfiPciIoWidthUint8,
        PCI_CLASSCODE_OFFSET,
        sizeof (PCI_CLASSCODE),
        &HdaClassReg
        );

      //
      // Check class code, ignore everything but HDA controllers.
      //
      if (EFI_ERROR (Status)
        || HdaClassReg.BaseCode != PCI_CLASS_MEDIA
        || HdaClassReg.SubClassCode != PCI_CLASS_MEDIA_MIXED_MODE) {
        continue;
      }

      DebugPrintDevicePathForHandle (DEBUG_INFO, "OCDC: Disconnecting audio controller", HandleBuffer[Index]);
      Status = gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
      DEBUG ((DEBUG_INFO, "OCDC: Disconnected - %r\n", Status));
    }

    FreePool (HandleBuffer);
  }
}
