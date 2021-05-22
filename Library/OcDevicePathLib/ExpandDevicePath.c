/** @file
  Library functions which relates with booting.

Copyright (c) 2011 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <IndustryStandard/Pci.h>

#include <Guid/Gpt.h>

#include <Protocol/BlockIo.h>
#include <Protocol/LoadFile.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UsbIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define ACPI_VMD0001_HID 0x000159A4 // EisaId ("VMD0001")
#define ACPI_VBS0001_HID 0x00015853 // EisaId ("VBS0001")

// CHANGE: Track InternalConnectAll() execution status.
STATIC BOOLEAN mConnectAllExecuted = FALSE;

// CHANGE: Added from UefiBootManagerLib
/**
  Connect all the drivers to all the controllers.

  This function makes sure all the current system drivers manage the correspoinding
  controllers if have. And at the same time, makes sure all the system controllers
  have driver to manage it if have.
**/
VOID
InternalConnectAll (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  // CHANGE: Report execution status.
  mConnectAllExecuted = TRUE;

  //
  // Connect All EFI 1.10 drivers following EFI 1.10 algorithm
  // CHANGE: Only connect device handles.  Do not call gDS->Dispatch().
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    FreePool (HandleBuffer);
  }
}

/**
  Check whether a USB device match the specified USB Class device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbClass    The USB Class device path to match.

  @retval TRUE       The USB device match the USB Class device path.
  @retval FALSE      The USB device does not match the USB Class device path.

**/
STATIC
BOOLEAN
BmMatchUsbClass (
  IN EFI_USB_IO_PROTOCOL        *UsbIo,
  IN USB_CLASS_DEVICE_PATH      *UsbClass
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT8                         DeviceClass;
  UINT8                         DeviceSubClass;
  UINT8                         DeviceProtocol;

  if ((DevicePathType (UsbClass) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbClass) != MSG_USB_CLASS_DP)){
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((UsbClass->VendorId != 0xffff) &&
      (UsbClass->VendorId != DevDesc.IdVendor)) {
    return FALSE;
  }

  if ((UsbClass->ProductId != 0xffff) &&
      (UsbClass->ProductId != DevDesc.IdProduct)) {
    return FALSE;
  }

  DeviceClass    = DevDesc.DeviceClass;
  DeviceSubClass = DevDesc.DeviceSubClass;
  DeviceProtocol = DevDesc.DeviceProtocol;
  if (DeviceClass == 0) {
    //
    // If Class in Device Descriptor is set to 0, use the Class, SubClass and
    // Protocol in Interface Descriptor instead.
    //
    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    DeviceClass    = IfDesc.InterfaceClass;
    DeviceSubClass = IfDesc.InterfaceSubClass;
    DeviceProtocol = IfDesc.InterfaceProtocol;
  }

  //
  // Check Class, SubClass and Protocol.
  //
  if ((UsbClass->DeviceClass != 0xff) &&
      (UsbClass->DeviceClass != DeviceClass)) {
    return FALSE;
  }

  if ((UsbClass->DeviceSubClass != 0xff) &&
      (UsbClass->DeviceSubClass != DeviceSubClass)) {
    return FALSE;
  }

  if ((UsbClass->DeviceProtocol != 0xff) &&
      (UsbClass->DeviceProtocol != DeviceProtocol)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check whether a USB device match the specified USB WWID device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbWwid     The USB WWID device path to match.

  @retval TRUE       The USB device match the USB WWID device path.
  @retval FALSE      The USB device does not match the USB WWID device path.

**/
STATIC
BOOLEAN
BmMatchUsbWwid (
  IN EFI_USB_IO_PROTOCOL        *UsbIo,
  IN USB_WWID_DEVICE_PATH       *UsbWwid
  )
{
  EFI_STATUS                   Status;
  EFI_USB_DEVICE_DESCRIPTOR    DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR IfDesc;
  UINT16                       *LangIdTable;
  UINT16                       TableSize;
  UINT16                       Index;
  CHAR16                       *CompareStr;
  UINTN                        CompareLen;
  CHAR16                       *SerialNumberStr;
  UINTN                        Length;

  if ((DevicePathType (UsbWwid) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbWwid) != MSG_USB_WWID_DP)) {
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if ((DevDesc.IdVendor != UsbWwid->VendorId) ||
      (DevDesc.IdProduct != UsbWwid->ProductId)) {
    return FALSE;
  }

  //
  // Check Interface Number.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if (IfDesc.InterfaceNumber != UsbWwid->InterfaceNumber) {
    return FALSE;
  }

  //
  // Check Serial Number.
  //
  if (DevDesc.StrSerialNumber == 0) {
    return FALSE;
  }

  //
  // Get all supported languages.
  //
  TableSize = 0;
  LangIdTable = NULL;
  Status = UsbIo->UsbGetSupportedLanguages (UsbIo, &LangIdTable, &TableSize);
  if (EFI_ERROR (Status) || (TableSize == 0) || (LangIdTable == NULL)) {
    return FALSE;
  }

  //
  // Serial number in USB WWID device path is the last 64-or-less UTF-16 characters.
  //
  CompareStr = (CHAR16 *) (UINTN) (UsbWwid + 1);
  CompareLen = (DevicePathNodeLength (UsbWwid) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16);
  if (CompareStr[CompareLen - 1] == L'\0') {
    CompareLen--;
  }

  //
  // Compare serial number in each supported language.
  //
  for (Index = 0; Index < TableSize / sizeof (UINT16); Index++) {
    SerialNumberStr = NULL;
    Status = UsbIo->UsbGetStringDescriptor (
                      UsbIo,
                      LangIdTable[Index],
                      DevDesc.StrSerialNumber,
                      &SerialNumberStr
                      );
    if (EFI_ERROR (Status) || (SerialNumberStr == NULL)) {
      continue;
    }

    Length = StrLen (SerialNumberStr);
    if ((Length >= CompareLen) &&
        (CompareMem (SerialNumberStr + Length - CompareLen, CompareStr, CompareLen * sizeof (CHAR16)) == 0)) {
      FreePool (SerialNumberStr);
      return TRUE;
    }

    FreePool (SerialNumberStr);
  }

  return FALSE;
}

/**
  Find a USB device which match the specified short-form device path start with
  USB Class or USB WWID device path. If ParentDevicePath is NULL, this function
  will search in all USB devices of the platform. If ParentDevicePath is not NULL,
  this function will only search in its child devices.

  @param DevicePath           The device path that contains USB Class or USB WWID device path.
  @param ParentDevicePathSize The length of the device path before the USB Class or
                              USB WWID device path.
  @param UsbIoHandleCount     A pointer to the count of the returned USB IO handles.

  @retval NULL       The matched USB IO handles cannot be found.
  @retval other      The matched USB IO handles.

**/
STATIC
EFI_HANDLE *
BmFindUsbDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  UINTN                     ParentDevicePathSize,
  OUT UINTN                     *UsbIoHandleCount
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *UsbIoHandles;
  EFI_DEVICE_PATH_PROTOCOL  *UsbIoDevicePath;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINTN                     Index;
  BOOLEAN                   Matched;

  ASSERT (UsbIoHandleCount != NULL);

  //
  // Get all UsbIo Handles.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  UsbIoHandleCount,
                  &UsbIoHandles
                  );
  if (EFI_ERROR (Status)) {
    *UsbIoHandleCount = 0;
    UsbIoHandles      = NULL;
  }

  for (Index = 0; Index < *UsbIoHandleCount; ) {
    //
    // Get the Usb IO interface.
    //
    Status = gBS->HandleProtocol(
                    UsbIoHandles[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &UsbIo
                    );
    UsbIoDevicePath = DevicePathFromHandle (UsbIoHandles[Index]);
    Matched         = FALSE;
    if (!EFI_ERROR (Status) && (UsbIoDevicePath != NULL)) {

      //
      // Compare starting part of UsbIoHandle's device path with ParentDevicePath.
      //
      if (CompareMem (UsbIoDevicePath, DevicePath, ParentDevicePathSize) == 0) {
        if (BmMatchUsbClass (UsbIo, (USB_CLASS_DEVICE_PATH *) ((UINTN) DevicePath + ParentDevicePathSize)) ||
            BmMatchUsbWwid (UsbIo, (USB_WWID_DEVICE_PATH *) ((UINTN) DevicePath + ParentDevicePathSize))) {
          Matched = TRUE;
        }
      }
    }

    if (!Matched) {
      (*UsbIoHandleCount) --;
      CopyMem (&UsbIoHandles[Index], &UsbIoHandles[Index + 1], (*UsbIoHandleCount - Index) * sizeof (EFI_HANDLE));
    } else {
      Index++;
    }
  }

  return UsbIoHandles;
}

/**
  Expand USB Class or USB WWID device path node to be full device path of a USB
  device in platform.

  This function support following 4 cases:
  1) Boot Option device path starts with a USB Class or USB WWID device path,
     and there is no Media FilePath device path in the end.
     In this case, it will follow Removable Media Boot Behavior.
  2) Boot Option device path starts with a USB Class or USB WWID device path,
     and ended with Media FilePath device path.
  3) Boot Option device path starts with a full device path to a USB Host Controller,
     contains a USB Class or USB WWID device path node, while not ended with Media
     FilePath device path. In this case, it will follow Removable Media Boot Behavior.
  4) Boot Option device path starts with a full device path to a USB Host Controller,
     contains a USB Class or USB WWID device path node, and ended with Media
     FilePath device path.

  @param FilePath      The device path pointing to a load option.
                       It could be a short-form device path.
  @param FullPath      The full path returned by the routine in last call.
                       Set to NULL in first call.
  @param ShortformNode Pointer to the USB short-form device path node in the FilePath buffer.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
STATIC
EFI_DEVICE_PATH_PROTOCOL *
BmExpandUsbDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ShortformNode
  )
{
  UINTN                             ParentDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          *NextFullPath;
  EFI_HANDLE                        *Handles;
  UINTN                             HandleCount;
  UINTN                             Index;
  BOOLEAN                           GetNext;

  NextFullPath = NULL;
  GetNext = (BOOLEAN)(FullPath == NULL);
  ParentDevicePathSize = (UINTN) ShortformNode - (UINTN) FilePath;
  RemainingDevicePath = NextDevicePathNode (ShortformNode);
  Handles = BmFindUsbDevice (FilePath, ParentDevicePathSize, &HandleCount);

  for (Index = 0; Index < HandleCount; Index++) {
    FilePath = AppendDevicePath (DevicePathFromHandle (Handles[Index]), RemainingDevicePath);
    if (FilePath == NULL) {
      //
      // Out of memory.
      //
      continue;
    }
    NextFullPath = OcGetNextLoadOptionDevicePath (FilePath, NULL);
    FreePool (FilePath);
    if (NextFullPath == NULL) {
      //
      // No BlockIo or SimpleFileSystem under FilePath.
      //
      continue;
    }
    if (GetNext) {
      break;
    } else {
      GetNext = (BOOLEAN)(CompareMem (NextFullPath, FullPath, GetDevicePathSize (NextFullPath)) == 0);
      FreePool (NextFullPath);
      NextFullPath = NULL;
    }
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  return NextFullPath;
}

/**
  Expand the media device path which points to a BlockIo or SimpleFileSystem instance.

  @param DevicePath  The media device path pointing to a BlockIo or SimpleFileSystem instance.
  @param FullPath    The full path returned by the routine in last call.
                     Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
STATIC
EFI_DEVICE_PATH_PROTOCOL *
BmExpandMediaDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL        *FullPath,
  IN  EFI_HANDLE                      Handle
  )
{
  EFI_STATUS                          Status;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;
  VOID                                *Buffer;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *NextFullPath;
  UINTN                               Size;
  UINTN                               TempSize;
  EFI_HANDLE                          *SimpleFileSystemHandles;
  UINTN                               NumberSimpleFileSystemHandles;
  UINTN                               Index;
  BOOLEAN                             GetNext;

  GetNext = (BOOLEAN)(FullPath == NULL);
  //
  // Check whether the device is connected
  // CHANGE: This branch has been removed as it has been covered by a caller change
  //

  // CHANGE: Get Handle via parameter as the function is called by the caller too

  //
  // For device boot option only pointing to the removable device handle,
  // should make sure all its children handles (its child partion or media handles)
  // are created and connected.
  //
  gBS->ConnectController (Handle, NULL, NULL, TRUE);

  //
  // Issue a dummy read to the device to check for media change.
  // When the removable media is changed, any Block IO read/write will
  // cause the BlockIo protocol be reinstalled and EFI_MEDIA_CHANGED is
  // returned. After the Block IO protocol is reinstalled, subsequent
  // Block IO read/write will success.
  //
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
  // CHANGE: Do not ASSERT.
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  Buffer = AllocatePool (BlockIo->Media->BlockSize);
  if (Buffer != NULL) {
    BlockIo->ReadBlocks (
      BlockIo,
      BlockIo->Media->MediaId,
      0,
      BlockIo->Media->BlockSize,
      Buffer
      );
    FreePool (Buffer);
  }

  //
  // Detect the the default boot file from removable Media
  //
  NextFullPath = NULL;
  Size = GetDevicePathSize (DevicePath) - END_DEVICE_PATH_LENGTH;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberSimpleFileSystemHandles,
         &SimpleFileSystemHandles
         );
  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    //
    // Get the device path size of SimpleFileSystem handle
    //
    TempDevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    TempSize = GetDevicePathSize (TempDevicePath) - END_DEVICE_PATH_LENGTH;
    //
    // Check whether the device path of boot option is part of the SimpleFileSystem handle's device path
    //
    if ((Size <= TempSize) && (CompareMem (TempDevicePath, DevicePath, Size) == 0)) {
      // CHANGE: Do not append EFI boot file.
      NextFullPath = DuplicateDevicePath (TempDevicePath);
      if (GetNext) {
        break;
      } else {
        GetNext = (BOOLEAN)(CompareMem (NextFullPath, FullPath, GetDevicePathSize (NextFullPath)) == 0);
        FreePool (NextFullPath);
        NextFullPath = NULL;
      }
    }
  }

  if (SimpleFileSystemHandles != NULL) {
    FreePool (SimpleFileSystemHandles);
  }

  return NextFullPath;
}

/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  Handle                 The partition's device handle
  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.
  @param  LocateEsp              Use ESP- instead of PartitionNumber-matching.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
STATIC
BOOLEAN
BmMatchPartitionDevicePathNode (
  IN  EFI_HANDLE                 Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath,
  IN  BOOLEAN                    LocateEsp
  )
{
  HARDDRIVE_DEVICE_PATH     *Node;
  CONST EFI_PARTITION_ENTRY *PartEntry;

  if ((BlockIoDevicePath == NULL) || (HardDriveDevicePath == NULL)) {
    return FALSE;
  }

  // CHANGE: Abort early if partition cannot be an ESP while it was requested.
  if (LocateEsp
   && (HardDriveDevicePath->MBRType != MBR_TYPE_EFI_PARTITION_TABLE_HEADER
    || HardDriveDevicePath->SignatureType != SIGNATURE_TYPE_GUID)) {
    return FALSE;
  }

  //
  // Match all the partition device path nodes including the nested partition nodes
  //
  while (!IsDevicePathEnd (BlockIoDevicePath)) {
    if ((DevicePathType (BlockIoDevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (BlockIoDevicePath) == MEDIA_HARDDRIVE_DP)
        ) {
      //
      // See if the harddrive device path in blockio matches the orig Hard Drive Node
      //
      Node = (HARDDRIVE_DEVICE_PATH *) BlockIoDevicePath;

      //
      // Match Signature and PartitionNumber.
      // Unused bytes in Signature are initiaized with zeros.
      //
      if ((Node->MBRType == HardDriveDevicePath->MBRType) &&
          (Node->SignatureType == HardDriveDevicePath->SignatureType) &&
          (CompareMem (Node->Signature, HardDriveDevicePath->Signature, sizeof (Node->Signature)) == 0)) {
        // CHANGE: Allow ESP location when PartitionNumber mismatches.
        if (!LocateEsp) {
          if (Node->PartitionNumber == HardDriveDevicePath->PartitionNumber) {
            return TRUE;
          }
        } else {
          PartEntry = OcGetGptPartitionEntry (Handle);
          if (PartEntry != NULL
            && CompareGuid (&PartEntry->PartitionTypeGUID, &gEfiPartTypeSystemPartGuid)) {
            return TRUE;
          }
        }
      }
    }

    BlockIoDevicePath = NextDevicePathNode (BlockIoDevicePath);
  }

  return FALSE;
}

/**
  Expand a device path that starts with a hard drive media device path node to be a
  full device path that includes the full hardware path to the device. We need
  to do this so it can be booted. As an optimization the front match (the part point
  to the partition node. E.g. ACPI() /PCI()/ATA()/Partition() ) is saved in a variable
  so a connect all is not required on every boot. All successful history device path
  which point to partition node (the front part) will be saved.

  @param FilePath      The device path pointing to a load option.
                       It could be a short-form device path.

  @return The full device path pointing to the load option.
**/
STATIC
EFI_DEVICE_PATH_PROTOCOL *
BmExpandPartitionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                Status;
  UINTN                     BlockIoHandleCount;
  EFI_HANDLE                *BlockIoBuffer;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FullPath;
  BOOLEAN                   LocateEsp;

  // CHANGE: Remove HDDP variable code.

  FullPath = NULL;
  //
  // If we get here we fail to find or 'HDDP' not exist, and now we need
  // to search all devices in the system for a matched partition
  //
  // CHANGE: Only connect all on failure.
  //
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
  if (EFI_ERROR (Status)) {
    BlockIoHandleCount = 0;
    BlockIoBuffer      = NULL;
  }
  //
  // Loop through all the device handles that support the BLOCK_IO Protocol
  // CHANGE: Locate ESP when failing to find an exact match
  //
  LocateEsp = FALSE;
  do {
    for (Index = 0; Index < BlockIoHandleCount; Index++) {
      BlockIoDevicePath = DevicePathFromHandle (BlockIoBuffer[Index]);
      if (BlockIoDevicePath == NULL) {
        continue;
      }

      if (BmMatchPartitionDevicePathNode (BlockIoBuffer[Index], BlockIoDevicePath, (HARDDRIVE_DEVICE_PATH *) FilePath, LocateEsp)) {
        //
        // Find the matched partition device path
        //
        TempDevicePath = AppendDevicePath (BlockIoDevicePath, NextDevicePathNode (FilePath));
        FullPath = OcGetNextLoadOptionDevicePath (TempDevicePath, NULL);
        FreePool (TempDevicePath);

        if (FullPath != NULL) {
          break;
        }
      }
    }

    if (FullPath != NULL) {
      break;
    }

    if (!mConnectAllExecuted) {
      InternalConnectAll ();
      FullPath = BmExpandPartitionDevicePath (FilePath);
      break;
    }

    if (LocateEsp) {
      break;
    }
    LocateEsp = TRUE;
  } while (TRUE);

  if (BlockIoBuffer != NULL) {
    FreePool (BlockIoBuffer);
  }

  return FullPath;
}

/**
  Connect the specific Usb device which match the short form device path,
  and whose bus is determined by Host Controller (Uhci or Ehci).

  @param  DevicePath             A short-form device path that starts with the first
                                 element being a USB WWID or a USB Class device
                                 path

  @return EFI_INVALID_PARAMETER  DevicePath is NULL pointer.
                                 DevicePath is not a USB device path.

  @return EFI_SUCCESS            Success to connect USB device
  @return EFI_NOT_FOUND          Fail to find handle for USB controller to connect.

**/
STATIC
EFI_STATUS
BmConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *Handles;
  UINTN                                 HandleCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  BOOLEAN                               AtLeastOneConnected;

  //
  // Check the passed in parameters
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
      ((DevicePathSubType (DevicePath) != MSG_USB_CLASS_DP) && (DevicePathSubType (DevicePath) != MSG_USB_WWID_DP))
     ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the usb host controller firstly, then connect with the remaining device path
  //
  AtLeastOneConnected = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Handles[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Check whether the Pci device is the wanted usb host controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status) &&
            ((PCI_CLASS_SERIAL == Class[2]) && (PCI_CLASS_SERIAL_USB == Class[1]))
           ) {
          Status = gBS->ConnectController (
                          Handles[Index],
                          NULL,
                          DevicePath,
                          FALSE
                          );
          if (!EFI_ERROR(Status)) {
            AtLeastOneConnected = TRUE;
          }
        }
      }
    }

    if (Handles != NULL) {
      FreePool (Handles);
    }
  }

  return AtLeastOneConnected ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Match macOS-made Hyper-V device path onto Hyper-V UEFI firmware device path.
  macOS is unaware of Hyper-V and therefore produces paths like:
  Acpi(0x000159A4,0x0)
    /Acpi(0x00015853,0x0)
    /Scsi(0x0,0x0)
    /HD(2,GPT,63882141-5773-4630-B8FD-2C6E4A491C78,0x64028,0x3A66090)
  We need to match them on real Hyper-V device paths like:
  AcpiEx(@@@0000,@@@0000,0x0,VMBus,,)
    /VenHw(9B17E5A2-0891-42DD-B653-80B5C22809BA,D96361BAA104294DB60572E2FFB1DC7F19539477F36F4B429750F8B5593AAA27)
    /Scsi(0x0,0x0)
    /HD(2,GPT,63882141-5773-4630-B8FD-2C6E4A491C78,0x64028,0x3A66090)

  @param[in] FilePath macOS-made device path with ACPI parts trimmed (i.e. Scsi(0x0, 0x0)/.../File).

  @return  real Hyper-V device path or NULL.
**/
STATIC
EFI_DEVICE_PATH_PROTOCOL *
BmExpandHyperVDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath
  )
{
  EFI_STATUS                 Status;
  UINTN                      HandleCount;
  UINTN                      Index;
  EFI_HANDLE                 *HandleBuffer;
  EFI_DEVICE_PATH_PROTOCOL   *HvDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *Node;
  EFI_DEVICE_PATH_PROTOCOL   *NewDevicePath;
  UINTN                      HvSuffixSize;

  DebugPrintDevicePath (DEBUG_INFO, "OCDP: Expanding Hyper-V DP", FilePath);

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiDevicePathProtocolGuid,
      (VOID **) &HvDevicePath
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DebugPrintDevicePath (DEBUG_INFO, "OCDP: Matching Hyper-V DP", HvDevicePath);

    for (Node = HvDevicePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
      //
      // Skip till we find the matching node in the middle of macOS-made DP.
      //
      if (DevicePathType (Node) == DevicePathType (FilePath)
        && DevicePathSubType (Node) == DevicePathSubType (FilePath)) {
        //
        // Match the macOS-made DP till the filename.
        //
        HvSuffixSize = GetDevicePathSize (Node) - END_DEVICE_PATH_LENGTH;
        if (CompareMem (Node, FilePath, HvSuffixSize) == 0) {
          NewDevicePath = AppendDevicePath (
            HvDevicePath,
            (VOID *) ((UINTN) FilePath + HvSuffixSize)
            );
          if (NewDevicePath != NULL) {
            DebugPrintDevicePath (DEBUG_INFO, "OCDP: Matched Hyper-V DP", NewDevicePath);
          }
          FreePool (HandleBuffer);
          return NewDevicePath;
        }
      }
    }
  }

  FreePool (HandleBuffer);
  return NULL;
}

EFI_DEVICE_PATH_PROTOCOL *
OcGetNextLoadOptionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL          *FullPath
  )
{
  EFI_HANDLE                      Handle;
  EFI_DEVICE_PATH_PROTOCOL        *Node;
  ACPI_HID_DEVICE_PATH            *AcpiNode;
  EFI_STATUS                      Status;

  ASSERT (FilePath != NULL);

  //
  // CHANGE: If SimpleFileSystem is located, the device path is already expanded.
  //
  Node = FilePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &Node, &Handle);
  if (!EFI_ERROR (Status)) {
    if (FullPath != NULL) {
      return NULL;
    }

    return DuplicateDevicePath (FilePath);
  }

  // CHANGE: Hyper-V support start.

  //
  // Match ACPI_VMD0001_HID.
  //
  if (FullPath == NULL ///< First and only call.
    && DevicePathType (Node) == ACPI_DEVICE_PATH
    && DevicePathSubType (Node) == ACPI_DP
    && DevicePathNodeLength (Node) == sizeof (ACPI_HID_DEVICE_PATH)) {
    AcpiNode = (ACPI_HID_DEVICE_PATH *) Node;
    if (AcpiNode->HID == ACPI_VMD0001_HID && AcpiNode->UID == 0) {
      //
      // Match ACPI_VBS0001_HID.
      //
      Node = NextDevicePathNode (Node);
      if (DevicePathType (Node) == ACPI_DEVICE_PATH
        && DevicePathSubType (Node) == ACPI_DP
        && DevicePathNodeLength (Node) == sizeof (ACPI_HID_DEVICE_PATH)) {
        AcpiNode = (ACPI_HID_DEVICE_PATH *) Node;
        if (AcpiNode->HID == ACPI_VBS0001_HID && AcpiNode->UID == 0) {
          Node = NextDevicePathNode (Node);
          return BmExpandHyperVDevicePath (Node);
        }
      }
    }
  }

  // CHANGE: Hyper-V support end.

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &Node, &Handle);
  if (!EFI_ERROR (Status) && IsDevicePathEnd (Node)) {
    return BmExpandMediaDevicePath (FilePath, FullPath, Handle);
  }

  // CHANGE: File Path node support removed.

  //
  // Expand the short-form device path to full device path
  //
  if ((DevicePathType (FilePath) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (FilePath) == MEDIA_HARDDRIVE_DP)) {
    //
    // Expand the Harddrive device path
    //
    if (FullPath == NULL) {
      return BmExpandPartitionDevicePath (FilePath);
    } else {
      return NULL;
    }
  } else if ((DevicePathType (FilePath) == MESSAGING_DEVICE_PATH) &&
             (DevicePathSubType (FilePath) == MSG_URI_DP)) {
    //
    // CHANGE: Removed expansion of the URI device path
    //
    return NULL;
  } else {
    Node = FilePath;
    Status = gBS->LocateDevicePath (&gEfiUsbIoProtocolGuid, &Node, &Handle);
    if (EFI_ERROR (Status)) {
      //
      // Only expand the USB WWID/Class device path
      // when FilePath doesn't point to a physical UsbIo controller.
      // Otherwise, infinite recursion will happen.
      //
      for (Node = FilePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
        if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
            ((DevicePathSubType (Node) == MSG_USB_CLASS_DP) || (DevicePathSubType (Node) == MSG_USB_WWID_DP))) {
          break;
        }
      }

      //
      // Expand the USB WWID/Class device path
      //
      if (!IsDevicePathEnd (Node)) {
        if (FilePath == Node) {
          //
          // Boot Option device path starts with USB Class or USB WWID device path.
          // For Boot Option device path which doesn't begin with the USB Class or
          // USB WWID device path, it's not needed to connect again here.
          //
          BmConnectUsbShortFormDevicePath (FilePath);
        }
        return BmExpandUsbDevicePath (FilePath, FullPath, Node);
      }
    }
  }

  //
  // For the below cases, FilePath only expands to one Full path.
  // So just handle the case when FullPath == NULL.
  //
  if (FullPath != NULL) {
    return NULL;
  }

  // CHANGE: SimleFileSystem case has been covered at the beginning of the function

  // CHANGE: Removed FV support.

  //
  // CHANGE: Remove LoadFile support (e.g. PXE network boot).
  //
  return NULL;
}
