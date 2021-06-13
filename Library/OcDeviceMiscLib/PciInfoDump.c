/** @file
  Copyright (C) 2021, PMheart. All rights reserved.
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/Pci.h>

#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
OcPciInfoDump (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS                     Status;
  UINTN                          HandleCount;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  PCI_TYPE00                     PciDevice;
  EFI_DEVICE_PATH_PROTOCOL       *PciDevicePath;
  CHAR16                         *TextPciDevicePath;

  CHAR8                          *FileBuffer;
  UINTN                          FileBufferSize;
  CHAR16                         TmpFileName[32];

  ASSERT (Root != NULL);

  FileBufferSize = SIZE_1KB;
  FileBuffer     = (CHAR8 *) AllocateZeroPool (FileBufferSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: No PCI devices found for dumping - %r\n", Status));
    return Status;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiPciIoProtocolGuid,
      (VOID **) &PciIo
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Read the whole PCI device in 32-bit.
    //
    Status = PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint32,
      0,
      sizeof (PCI_TYPE00) / sizeof (UINT32),
      &PciDevice
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Dump PCI info.
    //
    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "%u. Vendor ID: 0x%04X, Device ID: 0x%04X, RevisionID: 0x%02X, ClassCode: 0x%02X%02X%02X",
      Index + 1,
      PciDevice.Hdr.VendorId,
      PciDevice.Hdr.DeviceId,
      PciDevice.Hdr.RevisionID,
      PciDevice.Hdr.ClassCode[2],
      PciDevice.Hdr.ClassCode[1],
      PciDevice.Hdr.ClassCode[0]
      );
    //
    // Dump SubsystemVendorID and SubsystemID if the current device is not a PCI bridge.
    //
    if (!IS_PCI_BRIDGE (&PciDevice)) {
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        ", SubsystemVendorID: 0x%04X, SubsystemID: 0x%04X",
        PciDevice.Device.SubsystemVendorID,
        PciDevice.Device.SubsystemID
        );
    }
    //
    // Also dump device path if possible.
    //
    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiDevicePathProtocolGuid,
      (VOID **) &PciDevicePath
      );
    if (!EFI_ERROR (Status)) {
      TextPciDevicePath = ConvertDevicePathToText (PciDevicePath, FALSE, FALSE);
      if (TextPciDevicePath != NULL) {
        OcAsciiPrintBuffer (
          &FileBuffer,
          &FileBufferSize,
          ",\n   DevicePath: %s",
          TextPciDevicePath
          );

        FreePool (TextPciDevicePath);
      }
    }
    //
    // Finally, append a newline.
    //
    OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "\n");
  }

  //
  // Save dumped PCI info to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"PCIInfo.txt");
    Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCDM: Dumped PCI info - %r\n", Status));

    FreePool (FileBuffer);
  }

  return EFI_SUCCESS;
}
