/** @file
  Library functions which relates with booting.

Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
Copyright (c) 2011 - 2021, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2021 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBm.h"

EFI_RAM_DISK_PROTOCOL  *mRamDisk = NULL;

EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION  mBmRefreshLegacyBootOption = NULL;
EFI_BOOT_MANAGER_LEGACY_BOOT                 mBmLegacyBoot              = NULL;

///
/// This GUID is used for an EFI Variable that stores the front device pathes
/// for a partial device path that starts with the HD node.
///
EFI_GUID  mBmHardDriveBootVariableGuid = {
  0xfab7e9e1, 0x39dd, 0x4f2b, { 0x84, 0x08, 0xe2, 0x0e, 0x90, 0x6c, 0xb6, 0xde }
};
EFI_GUID  mBmAutoCreateBootOptionGuid = {
  0x8108ac4e, 0x9f11, 0x4d59, { 0x85, 0x0e, 0xe2, 0x1a, 0x52, 0x2c, 0x59, 0xb2 }
};

/**

  End Perf entry of BDS

  @param  Event                 The triggered event.
  @param  Context               Context for this event.

**/
VOID
EFIAPI
BmEndOfBdsPerfCode (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Record the performance data for End of BDS
  //
  PERF_CROSSMODULE_END ("BDS");

  return;
}

/**
  The function registers the legacy boot support capabilities.

  @param RefreshLegacyBootOption The function pointer to create all the legacy boot options.
  @param LegacyBoot              The function pointer to boot the legacy boot option.
**/
VOID
EFIAPI
EfiBootManagerRegisterLegacyBootSupport (
  EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION  RefreshLegacyBootOption,
  EFI_BOOT_MANAGER_LEGACY_BOOT                 LegacyBoot
  )
{
  mBmRefreshLegacyBootOption = RefreshLegacyBootOption;
  mBmLegacyBoot              = LegacyBoot;
}

/**
  Return TRUE when the boot option is auto-created instead of manually added.

  @param BootOption Pointer to the boot option to check.

  @retval TRUE  The boot option is auto-created.
  @retval FALSE The boot option is manually added.
**/
BOOLEAN
BmIsAutoCreateBootOption (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  if ((BootOption->OptionalDataSize == sizeof (EFI_GUID)) &&
      CompareGuid ((EFI_GUID *)BootOption->OptionalData, &mBmAutoCreateBootOptionGuid)
      )
  {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Find the boot option in the NV storage and return the option number.

  @param OptionToFind  Boot option to be checked.

  @return   The option number of the found boot option.

**/
UINTN
BmFindBootOptionInVariable (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *OptionToFind
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;
  UINTN                         OptionNumber;
  CHAR16                        OptionName[BM_OPTION_NAME_LEN];
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;
  UINTN                         Index;

  OptionNumber = LoadOptionNumberUnassigned;

  //
  // Try to match the variable exactly if the option number is assigned
  //
  if (OptionToFind->OptionNumber != LoadOptionNumberUnassigned) {
    UnicodeSPrint (
      OptionName,
      sizeof (OptionName),
      L"%s%04x",
      mBmLoadOptionName[OptionToFind->OptionType],
      OptionToFind->OptionNumber
      );
    Status = EfiBootManagerVariableToLoadOption (OptionName, &BootOption);

    if (!EFI_ERROR (Status)) {
      ASSERT (OptionToFind->OptionNumber == BootOption.OptionNumber);
      if ((OptionToFind->Attributes == BootOption.Attributes) &&
          (StrCmp (OptionToFind->Description, BootOption.Description) == 0) &&
          (CompareMem (OptionToFind->FilePath, BootOption.FilePath, GetDevicePathSize (OptionToFind->FilePath)) == 0) &&
          (OptionToFind->OptionalDataSize == BootOption.OptionalDataSize) &&
          (CompareMem (OptionToFind->OptionalData, BootOption.OptionalData, OptionToFind->OptionalDataSize) == 0)
          )
      {
        OptionNumber = OptionToFind->OptionNumber;
      }

      EfiBootManagerFreeLoadOption (&BootOption);
    }
  }

  //
  // The option number assigned is either incorrect or unassigned.
  //
  if (OptionNumber == LoadOptionNumberUnassigned) {
    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

    Index = EfiBootManagerFindLoadOption (OptionToFind, BootOptions, BootOptionCount);
    if (Index != -1) {
      OptionNumber = BootOptions[Index].OptionNumber;
    }

    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  }

  return OptionNumber;
}

/**
  Return the correct FV file path.
  FV address may change across reboot. This routine promises the FV file device path is right.

  @param  FilePath     The Memory Mapped Device Path to get the file buffer.

  @return  The updated FV Device Path pointint to the file.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmAdjustFvFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  EFI_DEVICE_PATH_PROTOCOL   *FvFileNode;
  EFI_HANDLE                 FvHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  UINTN                      FvHandleCount;
  EFI_HANDLE                 *FvHandles;
  EFI_DEVICE_PATH_PROTOCOL   *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *FullPath;

  //
  // Get the file buffer by using the exactly FilePath.
  //
  FvFileNode = FilePath;
  Status     = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &FvFileNode, &FvHandle);
  if (!EFI_ERROR (Status)) {
    return DuplicateDevicePath (FilePath);
  }

  //
  // Only wide match other FVs if it's a memory mapped FV file path.
  //
  if ((DevicePathType (FilePath) != HARDWARE_DEVICE_PATH) || (DevicePathSubType (FilePath) != HW_MEMMAP_DP)) {
    return NULL;
  }

  FvFileNode = NextDevicePathNode (FilePath);

  //
  // Firstly find the FV file in current FV
  //
  gBS->HandleProtocol (
         gImageHandle,
         &gEfiLoadedImageProtocolGuid,
         (VOID **)&LoadedImage
         );
  NewDevicePath = AppendDevicePathNode (DevicePathFromHandle (LoadedImage->DeviceHandle), FvFileNode);
  FullPath      = BmAdjustFvFilePath (NewDevicePath);
  FreePool (NewDevicePath);
  if (FullPath != NULL) {
    return FullPath;
  }

  //
  // Secondly find the FV file in all other FVs
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiFirmwareVolume2ProtocolGuid,
         NULL,
         &FvHandleCount,
         &FvHandles
         );
  for (Index = 0; Index < FvHandleCount; Index++) {
    if (FvHandles[Index] == LoadedImage->DeviceHandle) {
      //
      // Skip current FV, it was handed in first step.
      //
      continue;
    }

    NewDevicePath = AppendDevicePathNode (DevicePathFromHandle (FvHandles[Index]), FvFileNode);
    FullPath      = BmAdjustFvFilePath (NewDevicePath);
    FreePool (NewDevicePath);
    if (FullPath != NULL) {
      break;
    }
  }

  if (FvHandles != NULL) {
    FreePool (FvHandles);
  }

  return FullPath;
}

/**
  Check if it's a Device Path pointing to FV file.

  The function doesn't garentee the device path points to existing FV file.

  @param  DevicePath     Input device path.

  @retval TRUE   The device path is a FV File Device Path.
  @retval FALSE  The device path is NOT a FV File Device Path.
**/
BOOLEAN
BmIsFvFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  Node   = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &Node, &Handle);
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  if ((DevicePathType (DevicePath) == HARDWARE_DEVICE_PATH) && (DevicePathSubType (DevicePath) == HW_MEMMAP_DP)) {
    DevicePath = NextDevicePathNode (DevicePath);
    if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) && (DevicePathSubType (DevicePath) == MEDIA_PIWG_FW_FILE_DP)) {
      return IsDevicePathEnd (NextDevicePathNode (DevicePath));
    }
  }

  return FALSE;
}

/**
  Check whether a USB device match the specified USB Class device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbClass    The USB Class device path to match.

  @retval TRUE       The USB device match the USB Class device path.
  @retval FALSE      The USB device does not match the USB Class device path.

**/
BOOLEAN
BmMatchUsbClass (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN USB_CLASS_DEVICE_PATH  *UsbClass
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT8                         DeviceClass;
  UINT8                         DeviceSubClass;
  UINT8                         DeviceProtocol;

  if ((DevicePathType (UsbClass) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbClass) != MSG_USB_CLASS_DP))
  {
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
      (UsbClass->VendorId != DevDesc.IdVendor))
  {
    return FALSE;
  }

  if ((UsbClass->ProductId != 0xffff) &&
      (UsbClass->ProductId != DevDesc.IdProduct))
  {
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
      (UsbClass->DeviceClass != DeviceClass))
  {
    return FALSE;
  }

  if ((UsbClass->DeviceSubClass != 0xff) &&
      (UsbClass->DeviceSubClass != DeviceSubClass))
  {
    return FALSE;
  }

  if ((UsbClass->DeviceProtocol != 0xff) &&
      (UsbClass->DeviceProtocol != DeviceProtocol))
  {
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
BOOLEAN
BmMatchUsbWwid (
  IN EFI_USB_IO_PROTOCOL   *UsbIo,
  IN USB_WWID_DEVICE_PATH  *UsbWwid
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT16                        *LangIdTable;
  UINT16                        TableSize;
  UINT16                        Index;
  CHAR16                        *CompareStr;
  UINTN                         CompareLen;
  CHAR16                        *SerialNumberStr;
  UINTN                         Length;

  if ((DevicePathType (UsbWwid) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbWwid) != MSG_USB_WWID_DP))
  {
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
      (DevDesc.IdProduct != UsbWwid->ProductId))
  {
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
  TableSize   = 0;
  LangIdTable = NULL;
  Status      = UsbIo->UsbGetSupportedLanguages (UsbIo, &LangIdTable, &TableSize);
  if (EFI_ERROR (Status) || (TableSize == 0) || (LangIdTable == NULL)) {
    return FALSE;
  }

  //
  // Serial number in USB WWID device path is the last 64-or-less UTF-16 characters.
  //
  CompareStr = (CHAR16 *)(UINTN)(UsbWwid + 1);
  CompareLen = (DevicePathNodeLength (UsbWwid) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16);
  if (CompareStr[CompareLen - 1] == L'\0') {
    CompareLen--;
  }

  //
  // Compare serial number in each supported language.
  //
  for (Index = 0; Index < TableSize / sizeof (UINT16); Index++) {
    SerialNumberStr = NULL;
    Status          = UsbIo->UsbGetStringDescriptor (
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
        (CompareMem (SerialNumberStr + Length - CompareLen, CompareStr, CompareLen * sizeof (CHAR16)) == 0))
    {
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
    Status = gBS->HandleProtocol (
                    UsbIoHandles[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    UsbIoDevicePath = DevicePathFromHandle (UsbIoHandles[Index]);
    Matched         = FALSE;
    if (!EFI_ERROR (Status) && (UsbIoDevicePath != NULL)) {
      //
      // Compare starting part of UsbIoHandle's device path with ParentDevicePath.
      //
      if (CompareMem (UsbIoDevicePath, DevicePath, ParentDevicePathSize) == 0) {
        if (BmMatchUsbClass (UsbIo, (USB_CLASS_DEVICE_PATH *)((UINTN)DevicePath + ParentDevicePathSize)) ||
            BmMatchUsbWwid (UsbIo, (USB_WWID_DEVICE_PATH *)((UINTN)DevicePath + ParentDevicePathSize)))
        {
          Matched = TRUE;
        }
      }
    }

    if (!Matched) {
      (*UsbIoHandleCount)--;
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
EFI_DEVICE_PATH_PROTOCOL *
BmExpandUsbDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ShortformNode
  )
{
  UINTN                     ParentDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *NextFullPath;
  EFI_HANDLE                *Handles;
  UINTN                     HandleCount;
  UINTN                     Index;
  BOOLEAN                   GetNext;

  NextFullPath         = NULL;
  GetNext              = (BOOLEAN)(FullPath == NULL);
  ParentDevicePathSize = (UINTN)ShortformNode - (UINTN)FilePath;
  RemainingDevicePath  = NextDevicePathNode (ShortformNode);
  Handles              = BmFindUsbDevice (FilePath, ParentDevicePathSize, &HandleCount);

  for (Index = 0; Index < HandleCount; Index++) {
    FilePath = AppendDevicePath (DevicePathFromHandle (Handles[Index]), RemainingDevicePath);
    if (FilePath == NULL) {
      //
      // Out of memory.
      //
      continue;
    }

    NextFullPath = BmGetNextLoadOptionDevicePath (FilePath, NULL);
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
  Expand File-path device path node to be full device path in platform.

  @param FilePath      The device path pointing to a load option.
                       It could be a short-form device path.
  @param FullPath      The full path returned by the routine in last call.
                       Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandFileDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     HandleCount;
  EFI_HANDLE                *Handles;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  UINTN                     MediaType;
  EFI_DEVICE_PATH_PROTOCOL  *NextFullPath;
  BOOLEAN                   GetNext;

  EfiBootManagerConnectAll ();
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles);
  if (EFI_ERROR (Status)) {
    HandleCount = 0;
    Handles     = NULL;
  }

  GetNext      = (BOOLEAN)(FullPath == NULL);
  NextFullPath = NULL;
  //
  // Enumerate all removable media devices followed by all fixed media devices,
  //   followed by media devices which don't layer on block io.
  //
  for (MediaType = 0; MediaType < 3; MediaType++) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (Handles[Index], &gEfiBlockIoProtocolGuid, (VOID *)&BlockIo);
      if (EFI_ERROR (Status)) {
        BlockIo = NULL;
      }

      if (((MediaType == 0) && (BlockIo != NULL) && BlockIo->Media->RemovableMedia) ||
          ((MediaType == 1) && (BlockIo != NULL) && !BlockIo->Media->RemovableMedia) ||
          ((MediaType == 2) && (BlockIo == NULL))
          )
      {
        NextFullPath = AppendDevicePath (DevicePathFromHandle (Handles[Index]), FilePath);
        if (GetNext) {
          break;
        } else {
          GetNext = (BOOLEAN)(CompareMem (NextFullPath, FullPath, GetDevicePathSize (NextFullPath)) == 0);
          FreePool (NextFullPath);
          NextFullPath = NULL;
        }
      }
    }

    if (NextFullPath != NULL) {
      break;
    }
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  return NextFullPath;
}

/**
  Expand URI device path node to be full device path in platform.

  @param FilePath      The device path pointing to a load option.
                       It could be a short-form device path.
  @param FullPath      The full path returned by the routine in last call.
                       Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandUriDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     HandleCount;
  EFI_HANDLE                *Handles;
  EFI_DEVICE_PATH_PROTOCOL  *NextFullPath;
  EFI_DEVICE_PATH_PROTOCOL  *RamDiskDevicePath;
  BOOLEAN                   GetNext;

  EfiBootManagerConnectAll ();
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiLoadFileProtocolGuid, NULL, &HandleCount, &Handles);
  if (EFI_ERROR (Status)) {
    HandleCount = 0;
    Handles     = NULL;
  }

  NextFullPath = NULL;
  GetNext      = (BOOLEAN)(FullPath == NULL);
  for (Index = 0; Index < HandleCount; Index++) {
    NextFullPath = BmExpandLoadFile (Handles[Index], FilePath);

    if (NextFullPath == NULL) {
      continue;
    }

    if (GetNext) {
      break;
    } else {
      GetNext = (BOOLEAN)(CompareMem (NextFullPath, FullPath, GetDevicePathSize (NextFullPath)) == 0);
      //
      // Free the resource occupied by the RAM disk.
      //
      RamDiskDevicePath = BmGetRamDiskDevicePath (NextFullPath);
      if (RamDiskDevicePath != NULL) {
        BmDestroyRamDisk (RamDiskDevicePath);
        FreePool (RamDiskDevicePath);
      }

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
  Save the partition DevicePath to the CachedDevicePath as the first instance.

  @param CachedDevicePath  The device path cache.
  @param DevicePath        The partition device path to be cached.
**/
VOID
BmCachePartitionDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **CachedDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  UINTN                     Count;

  if (BmMatchDevicePaths (*CachedDevicePath, DevicePath)) {
    TempDevicePath    = *CachedDevicePath;
    *CachedDevicePath = BmDelPartMatchInstance (*CachedDevicePath, DevicePath);
    FreePool (TempDevicePath);
  }

  if (*CachedDevicePath == NULL) {
    *CachedDevicePath = DuplicateDevicePath (DevicePath);
    return;
  }

  TempDevicePath    = *CachedDevicePath;
  *CachedDevicePath = AppendDevicePathInstance (DevicePath, *CachedDevicePath);
  if (TempDevicePath != NULL) {
    FreePool (TempDevicePath);
  }

  //
  // Here limit the device path instance number to 12, which is max number for a system support 3 IDE controller
  // If the user try to boot many OS in different HDs or partitions, in theory, the 'HDDP' variable maybe become larger and larger.
  //
  Count          = 0;
  TempDevicePath = *CachedDevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
    TempDevicePath = NextDevicePathNode (TempDevicePath);
    //
    // Parse one instance
    //
    while (!IsDevicePathEndType (TempDevicePath)) {
      TempDevicePath = NextDevicePathNode (TempDevicePath);
    }

    Count++;
    //
    // If the CachedDevicePath variable contain too much instance, only remain 12 instances.
    //
    if (Count == 12) {
      SetDevicePathEndNode (TempDevicePath);
      break;
    }
  }
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
  EFI_DEVICE_PATH_PROTOCOL  *CachedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FullPath;
  UINTN                     CachedDevicePathSize;
  BOOLEAN                   NeedAdjust;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  UINTN                     Size;
  BOOLEAN                   MatchFound;
  BOOLEAN                   ConnectAllAttempted;

  //
  // Check if there is prestore 'HDDP' variable.
  // If exist, search the front path which point to partition node in the variable instants.
  // If fail to find or 'HDDP' not exist, reconnect all and search in all system
  //
  GetVariable2 (L"HDDP", &mBmHardDriveBootVariableGuid, (VOID **)&CachedDevicePath, &CachedDevicePathSize);

  //
  // Delete the invalid 'HDDP' variable.
  //
  if ((CachedDevicePath != NULL) && !IsDevicePathValid (CachedDevicePath, CachedDevicePathSize)) {
    FreePool (CachedDevicePath);
    CachedDevicePath = NULL;
    Status           = gRT->SetVariable (
                              L"HDDP",
                              &mBmHardDriveBootVariableGuid,
                              0,
                              0,
                              NULL
                              );
    ASSERT_EFI_ERROR (Status);
  }

  FullPath = NULL;
  if (CachedDevicePath != NULL) {
    TempNewDevicePath = CachedDevicePath;
    NeedAdjust        = FALSE;
    do {
      //
      // Check every instance of the variable
      // First, check whether the instance contain the partition node, which is needed for distinguishing  multi
      // partial partition boot option. Second, check whether the instance could be connected.
      //
      Instance = GetNextDevicePathInstance (&TempNewDevicePath, &Size);
      if (BmMatchPartitionDevicePathNode (Instance, (HARDDRIVE_DEVICE_PATH *)FilePath)) {
        //
        // Connect the device path instance, the device path point to hard drive media device path node
        // e.g. ACPI() /PCI()/ATA()/Partition()
        //
        Status = EfiBootManagerConnectDevicePath (Instance, NULL);
        if (!EFI_ERROR (Status)) {
          TempDevicePath = AppendDevicePath (Instance, NextDevicePathNode (FilePath));
          //
          // TempDevicePath = ACPI()/PCI()/ATA()/Partition()
          // or             = ACPI()/PCI()/ATA()/Partition()/.../A.EFI
          //
          // When TempDevicePath = ACPI()/PCI()/ATA()/Partition(),
          // it may expand to two potienal full paths (nested partition, rarely happen):
          //   1. ACPI()/PCI()/ATA()/Partition()/Partition(A1)/EFI/BootX64.EFI
          //   2. ACPI()/PCI()/ATA()/Partition()/Partition(A2)/EFI/BootX64.EFI
          // For simplicity, only #1 is returned.
          //
          FullPath = BmGetNextLoadOptionDevicePath (TempDevicePath, NULL);
          FreePool (TempDevicePath);

          if (FullPath != NULL) {
            //
            // Adjust the 'HDDP' instances sequence if the matched one is not first one.
            //
            if (NeedAdjust) {
              BmCachePartitionDevicePath (&CachedDevicePath, Instance);
              //
              // Save the matching Device Path so we don't need to do a connect all next time
              // Failing to save only impacts performance next time expanding the short-form device path
              //
              Status = gRT->SetVariable (
                              L"HDDP",
                              &mBmHardDriveBootVariableGuid,
                              EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                              GetDevicePathSize (CachedDevicePath),
                              CachedDevicePath
                              );
            }

            FreePool (Instance);
            FreePool (CachedDevicePath);
            return FullPath;
          }
        }
      }

      //
      // Come here means the first instance is not matched
      //
      NeedAdjust = TRUE;
      FreePool (Instance);
    } while (TempNewDevicePath != NULL);
  }

  //
  // If we get here we fail to find or 'HDDP' not exist, and now we need
  // to search all devices in the system for a matched partition
  //
  BlockIoBuffer       = NULL;
  MatchFound          = FALSE;
  ConnectAllAttempted = FALSE;
  do {
    if (BlockIoBuffer != NULL) {
      FreePool (BlockIoBuffer);
    }

    Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
    if (EFI_ERROR (Status)) {
      BlockIoHandleCount = 0;
      BlockIoBuffer      = NULL;
    }

    //
    // Loop through all the device handles that support the BLOCK_IO Protocol
    //
    for (Index = 0; Index < BlockIoHandleCount; Index++) {
      BlockIoDevicePath = DevicePathFromHandle (BlockIoBuffer[Index]);
      if (BlockIoDevicePath == NULL) {
        continue;
      }

      if (BmMatchPartitionDevicePathNode (BlockIoDevicePath, (HARDDRIVE_DEVICE_PATH *)FilePath)) {
        //
        // Find the matched partition device path
        //
        TempDevicePath = AppendDevicePath (BlockIoDevicePath, NextDevicePathNode (FilePath));
        FullPath       = BmGetNextLoadOptionDevicePath (TempDevicePath, NULL);
        FreePool (TempDevicePath);

        if (FullPath != NULL) {
          BmCachePartitionDevicePath (&CachedDevicePath, BlockIoDevicePath);

          //
          // Save the matching Device Path so we don't need to do a connect all next time
          // Failing to save only impacts performance next time expanding the short-form device path
          //
          Status = gRT->SetVariable (
                          L"HDDP",
                          &mBmHardDriveBootVariableGuid,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS |
                          EFI_VARIABLE_NON_VOLATILE,
                          GetDevicePathSize (CachedDevicePath),
                          CachedDevicePath
                          );
          MatchFound = TRUE;
          break;
        }
      }
    }

    //
    // If we found a matching BLOCK_IO handle or we've already
    // tried a ConnectAll, we are done searching.
    //
    if (MatchFound || ConnectAllAttempted) {
      break;
    }

    EfiBootManagerConnectAll ();
    ConnectAllAttempted = TRUE;
  } while (1);

  if (CachedDevicePath != NULL) {
    FreePool (CachedDevicePath);
  }

  if (BlockIoBuffer != NULL) {
    FreePool (BlockIoBuffer);
  }

  return FullPath;
}

/**
  Expand the media device path which points to a BlockIo or SimpleFileSystem instance
  by appending EFI_REMOVABLE_MEDIA_FILE_NAME.

  @param DevicePath  The media device path pointing to a BlockIo or SimpleFileSystem instance.
  @param FullPath    The full path returned by the routine in last call.
                     Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandMediaDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  VOID                      *Buffer;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *NextFullPath;
  UINTN                     Size;
  UINTN                     TempSize;
  EFI_HANDLE                *SimpleFileSystemHandles;
  UINTN                     NumberSimpleFileSystemHandles;
  UINTN                     Index;
  BOOLEAN                   GetNext;

  GetNext = (BOOLEAN)(FullPath == NULL);
  //
  // Check whether the device is connected
  //
  TempDevicePath = DevicePath;
  Status         = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &TempDevicePath, &Handle);
  if (!EFI_ERROR (Status)) {
    ASSERT (IsDevicePathEnd (TempDevicePath));

    NextFullPath = FileDevicePath (Handle, EFI_REMOVABLE_MEDIA_FILE_NAME);
    //
    // For device path pointing to simple file system, it only expands to one full path.
    //
    if (GetNext) {
      return NextFullPath;
    } else {
      FreePool (NextFullPath);
      return NULL;
    }
  }

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &TempDevicePath, &Handle);
  ASSERT_EFI_ERROR (Status);

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
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  ASSERT_EFI_ERROR (Status);
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
  Size         = GetDevicePathSize (DevicePath) - END_DEVICE_PATH_LENGTH;
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
    TempSize       = GetDevicePathSize (TempDevicePath) - END_DEVICE_PATH_LENGTH;
    //
    // Check whether the device path of boot option is part of the SimpleFileSystem handle's device path
    //
    if ((Size <= TempSize) && (CompareMem (TempDevicePath, DevicePath, Size) == 0)) {
      NextFullPath = FileDevicePath (SimpleFileSystemHandles[Index], EFI_REMOVABLE_MEDIA_FILE_NAME);
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
  Check whether Left and Right are the same without matching the specific
  device path data in IP device path and URI device path node.

  @retval TRUE  Left and Right are the same.
  @retval FALSE Left and Right are the different.
**/
BOOLEAN
BmMatchHttpBootDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Left,
  IN EFI_DEVICE_PATH_PROTOCOL  *Right
  )
{
  for ( ; !IsDevicePathEnd (Left) && !IsDevicePathEnd (Right)
        ; Left = NextDevicePathNode (Left), Right = NextDevicePathNode (Right)
        )
  {
    if (CompareMem (Left, Right, DevicePathNodeLength (Left)) != 0) {
      if ((DevicePathType (Left) != MESSAGING_DEVICE_PATH) || (DevicePathType (Right) != MESSAGING_DEVICE_PATH)) {
        return FALSE;
      }

      if (DevicePathSubType (Left) == MSG_DNS_DP) {
        Left = NextDevicePathNode (Left);
      }

      if (DevicePathSubType (Right) == MSG_DNS_DP) {
        Right = NextDevicePathNode (Right);
      }

      if (((DevicePathSubType (Left) != MSG_IPv4_DP) || (DevicePathSubType (Right) != MSG_IPv4_DP)) &&
          ((DevicePathSubType (Left) != MSG_IPv6_DP) || (DevicePathSubType (Right) != MSG_IPv6_DP)) &&
          ((DevicePathSubType (Left) != MSG_URI_DP)  || (DevicePathSubType (Right) != MSG_URI_DP))
          )
      {
        return FALSE;
      }
    }
  }

  return (BOOLEAN)(IsDevicePathEnd (Left) && IsDevicePathEnd (Right));
}

/**
  Get the file buffer from the file system produced by Load File instance.

  @param LoadFileHandle The handle of LoadFile instance.
  @param RamDiskHandle  Return the RAM Disk handle.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandNetworkFileSystem (
  IN  EFI_HANDLE  LoadFileHandle,
  OUT EFI_HANDLE  *RamDiskHandle
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                *Handles;
  UINTN                     HandleCount;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    Handles     = NULL;
    HandleCount = 0;
  }

  Handle = NULL;
  for (Index = 0; Index < HandleCount; Index++) {
    Node   = DevicePathFromHandle (Handles[Index]);
    Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &Node, &Handle);
    if (!EFI_ERROR (Status) &&
        (Handle == LoadFileHandle) &&
        (DevicePathType (Node) == MEDIA_DEVICE_PATH) && (DevicePathSubType (Node) == MEDIA_RAM_DISK_DP))
    {
      //
      // Find the BlockIo instance populated from the LoadFile.
      //
      Handle = Handles[Index];
      break;
    }
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  if (Index == HandleCount) {
    Handle = NULL;
  }

  *RamDiskHandle = Handle;

  if (Handle != NULL) {
    //
    // Re-use BmExpandMediaDevicePath() to get the full device path of load option.
    // But assume only one SimpleFileSystem can be found under the BlockIo.
    //
    return BmExpandMediaDevicePath (DevicePathFromHandle (Handle), NULL);
  } else {
    return NULL;
  }
}

/**
  Return the RAM Disk device path created by LoadFile.

  @param FilePath  The source file path.

  @return Callee-to-free RAM Disk device path
**/
EFI_DEVICE_PATH_PROTOCOL *
BmGetRamDiskDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *RamDiskDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_HANDLE                Handle;

  Node   = FilePath;
  Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &Node, &Handle);
  if (!EFI_ERROR (Status) &&
      (DevicePathType (Node) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (Node) == MEDIA_RAM_DISK_DP)
      )
  {
    //
    // Construct the device path pointing to RAM Disk
    //
    Node              = NextDevicePathNode (Node);
    RamDiskDevicePath = DuplicateDevicePath (FilePath);
    ASSERT (RamDiskDevicePath != NULL);
    SetDevicePathEndNode ((VOID *)((UINTN)RamDiskDevicePath + ((UINTN)Node - (UINTN)FilePath)));
    return RamDiskDevicePath;
  }

  return NULL;
}

/**
  Return the buffer and buffer size occupied by the RAM Disk.

  @param RamDiskDevicePath  RAM Disk device path.
  @param RamDiskSizeInPages Return RAM Disk size in pages.

  @retval RAM Disk buffer.
**/
VOID *
BmGetRamDiskMemoryInfo (
  IN EFI_DEVICE_PATH_PROTOCOL  *RamDiskDevicePath,
  OUT UINTN                    *RamDiskSizeInPages
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  UINT64      StartingAddr;
  UINT64      EndingAddr;

  ASSERT (RamDiskDevicePath != NULL);

  *RamDiskSizeInPages = 0;

  //
  // Get the buffer occupied by RAM Disk.
  //
  Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &RamDiskDevicePath, &Handle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (
    (DevicePathType (RamDiskDevicePath) == MEDIA_DEVICE_PATH) &&
    (DevicePathSubType (RamDiskDevicePath) == MEDIA_RAM_DISK_DP)
    );
  StartingAddr        = ReadUnaligned64 ((UINT64 *)((MEDIA_RAM_DISK_DEVICE_PATH *)RamDiskDevicePath)->StartingAddr);
  EndingAddr          = ReadUnaligned64 ((UINT64 *)((MEDIA_RAM_DISK_DEVICE_PATH *)RamDiskDevicePath)->EndingAddr);
  *RamDiskSizeInPages = EFI_SIZE_TO_PAGES ((UINTN)(EndingAddr - StartingAddr + 1));
  return (VOID *)(UINTN)StartingAddr;
}

/**
  Destroy the RAM Disk.

  The destroy operation includes to call RamDisk.Unregister to
  unregister the RAM DISK from RAM DISK driver, free the memory
  allocated for the RAM Disk.

  @param RamDiskDevicePath    RAM Disk device path.
**/
VOID
BmDestroyRamDisk (
  IN EFI_DEVICE_PATH_PROTOCOL  *RamDiskDevicePath
  )
{
  EFI_STATUS  Status;
  VOID        *RamDiskBuffer;
  UINTN       RamDiskSizeInPages;

  ASSERT (RamDiskDevicePath != NULL);

  RamDiskBuffer = BmGetRamDiskMemoryInfo (RamDiskDevicePath, &RamDiskSizeInPages);

  //
  // Destroy RAM Disk.
  //
  if (mRamDisk == NULL) {
    Status = gBS->LocateProtocol (&gEfiRamDiskProtocolGuid, NULL, (VOID *)&mRamDisk);
    ASSERT_EFI_ERROR (Status);
  }

  Status = mRamDisk->Unregister (RamDiskDevicePath);
  ASSERT_EFI_ERROR (Status);
  FreePages (RamDiskBuffer, RamDiskSizeInPages);
}

/**
  Get the file buffer from the specified Load File instance.

  @param LoadFileHandle The specified Load File instance.
  @param FilePath       The file path which will pass to LoadFile().

  @return  The full device path pointing to the load option buffer.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandLoadFile (
  IN  EFI_HANDLE                LoadFileHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                Status;
  EFI_LOAD_FILE_PROTOCOL    *LoadFile;
  VOID                      *FileBuffer;
  EFI_HANDLE                RamDiskHandle;
  UINTN                     BufferSize;
  EFI_DEVICE_PATH_PROTOCOL  *FullPath;

  Status = gBS->OpenProtocol (
                  LoadFileHandle,
                  &gEfiLoadFileProtocolGuid,
                  (VOID **)&LoadFile,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);

  FileBuffer = NULL;
  BufferSize = 0;
  Status     = LoadFile->LoadFile (LoadFile, FilePath, TRUE, &BufferSize, FileBuffer);
  if ((Status != EFI_WARN_FILE_SYSTEM) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return NULL;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // The load option buffer is directly returned by LoadFile.
    //
    return DuplicateDevicePath (DevicePathFromHandle (LoadFileHandle));
  }

  //
  // The load option resides in a RAM disk.
  //
  FileBuffer = AllocateReservedPages (EFI_SIZE_TO_PAGES (BufferSize));
  if (FileBuffer == NULL) {
    DEBUG_CODE_BEGIN ();
    EFI_DEVICE_PATH  *LoadFilePath;
    CHAR16           *LoadFileText;
    CHAR16           *FileText;

    LoadFilePath = DevicePathFromHandle (LoadFileHandle);
    if (LoadFilePath == NULL) {
      LoadFileText = NULL;
    } else {
      LoadFileText = ConvertDevicePathToText (LoadFilePath, FALSE, FALSE);
    }

    FileText = ConvertDevicePathToText (FilePath, FALSE, FALSE);

    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to allocate reserved pages: "
      "BufferSize=%Lu LoadFile=\"%s\" FilePath=\"%s\"\n",
      gEfiCallerBaseName,
      __func__,
      (UINT64)BufferSize,
      LoadFileText,
      FileText
      ));

    if (FileText != NULL) {
      FreePool (FileText);
    }

    if (LoadFileText != NULL) {
      FreePool (LoadFileText);
    }

    DEBUG_CODE_END ();
    return NULL;
  }

  Status = LoadFile->LoadFile (LoadFile, FilePath, TRUE, &BufferSize, FileBuffer);
  if (EFI_ERROR (Status)) {
    FreePages (FileBuffer, EFI_SIZE_TO_PAGES (BufferSize));
    return NULL;
  }

  FullPath = BmExpandNetworkFileSystem (LoadFileHandle, &RamDiskHandle);
  if (FullPath == NULL) {
    //
    // Free the memory occupied by the RAM disk if there is no BlockIo or SimpleFileSystem instance.
    //
    BmDestroyRamDisk (DevicePathFromHandle (RamDiskHandle));
  }

  return FullPath;
}

/**
  Return the full device path pointing to the load option.

  FilePath may:
  1. Exactly matches to a LoadFile instance.
  2. Cannot match to any LoadFile instance. Wide match is required.
  In either case, the routine may return:
  1. A copy of FilePath when FilePath matches to a LoadFile instance and
     the LoadFile returns a load option buffer.
  2. A new device path with IP and URI information updated when wide match
     happens.
  3. A new device path pointing to a load option in RAM disk.
  In either case, only one full device path is returned for a specified
  FilePath.

  @param FilePath    The media device path pointing to a LoadFile instance.

  @return  The load option buffer.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandLoadFiles (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                *Handles;
  UINTN                     HandleCount;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  //
  // Get file buffer from load file instance.
  //
  Node   = FilePath;
  Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &Node, &Handle);
  if (!EFI_ERROR (Status) && IsDevicePathEnd (Node)) {
    //
    // When wide match happens, pass full device path to LoadFile (),
    // otherwise, pass remaining device path to LoadFile ().
    //
    FilePath = Node;
  } else {
    Handle = NULL;
    //
    // Use wide match algorithm to find one when
    //  cannot find a LoadFile instance to exactly match the FilePath
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiLoadFileProtocolGuid,
                    NULL,
                    &HandleCount,
                    &Handles
                    );
    if (EFI_ERROR (Status)) {
      Handles     = NULL;
      HandleCount = 0;
    }

    for (Index = 0; Index < HandleCount; Index++) {
      if (BmMatchHttpBootDevicePath (DevicePathFromHandle (Handles[Index]), FilePath)) {
        Handle = Handles[Index];
        break;
      }
    }

    if (Handles != NULL) {
      FreePool (Handles);
    }
  }

  if (Handle == NULL) {
    return NULL;
  }

  return BmExpandLoadFile (Handle, FilePath);
}

/**
  Get the load option by its device path.

  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  Return the full device path of the load option after
                   short-form device path expanding.
                   Caller is responsible to free it.
  @param FileSize  Return the load option size.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
EFIAPI
EfiBootManagerGetLoadOptionBuffer (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FullPath,
  OUT UINTN                     *FileSize
  )
{
  *FullPath = NULL;

  EfiBootManagerConnectDevicePath (FilePath, NULL);
  return BmGetNextLoadOptionBuffer (LoadOptionTypeMax, FilePath, FullPath, FileSize);
}

/**
  Get the next possible full path pointing to the load option.
  The routine doesn't guarantee the returned full path points to an existing
  file, and it also doesn't guarantee the existing file is a valid load option.
  BmGetNextLoadOptionBuffer() guarantees.

  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  The full path returned by the routine in last call.
                   Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmGetNextLoadOptionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  )
{
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_STATUS                Status;

  ASSERT (FilePath != NULL);

  //
  // Boot from media device by adding a default file name \EFI\BOOT\BOOT{machine type short-name}.EFI
  //
  Node   = FilePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &Node, &Handle);
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &Node, &Handle);
  }

  if (!EFI_ERROR (Status) && IsDevicePathEnd (Node)) {
    return BmExpandMediaDevicePath (FilePath, FullPath);
  }

  //
  // Expand the short-form device path to full device path
  //
  if ((DevicePathType (FilePath) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (FilePath) == MEDIA_HARDDRIVE_DP))
  {
    //
    // Expand the Harddrive device path
    //
    if (FullPath == NULL) {
      return BmExpandPartitionDevicePath (FilePath);
    } else {
      return NULL;
    }
  } else if ((DevicePathType (FilePath) == MEDIA_DEVICE_PATH) &&
             (DevicePathSubType (FilePath) == MEDIA_FILEPATH_DP))
  {
    //
    // Expand the File-path device path
    //
    return BmExpandFileDevicePath (FilePath, FullPath);
  } else if ((DevicePathType (FilePath) == MESSAGING_DEVICE_PATH) &&
             (DevicePathSubType (FilePath) == MSG_URI_DP))
  {
    //
    // Expand the URI device path
    //
    return BmExpandUriDevicePath (FilePath, FullPath);
  } else {
    Node   = FilePath;
    Status = gBS->LocateDevicePath (&gEfiUsbIoProtocolGuid, &Node, &Handle);
    if (EFI_ERROR (Status)) {
      //
      // Only expand the USB WWID/Class device path
      // when FilePath doesn't point to a physical UsbIo controller.
      // Otherwise, infinite recursion will happen.
      //
      for (Node = FilePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
        if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
            ((DevicePathSubType (Node) == MSG_USB_CLASS_DP) || (DevicePathSubType (Node) == MSG_USB_WWID_DP)))
        {
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

  //
  // Load option resides in FV.
  //
  if (BmIsFvFilePath (FilePath)) {
    return BmAdjustFvFilePath (FilePath);
  }

  //
  // Load option resides in Simple File System.
  //
  Node   = FilePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &Node, &Handle);
  if (!EFI_ERROR (Status)) {
    return DuplicateDevicePath (FilePath);
  }

  //
  // Last chance to try: Load option may be loaded through LoadFile.
  //
  return BmExpandLoadFiles (FilePath);
}

/**
  Check if it's a Device Path pointing to BootManagerMenu.

  @param  DevicePath     Input device path.

  @retval TRUE   The device path is BootManagerMenu File Device Path.
  @retval FALSE  The device path is NOT BootManagerMenu File Device Path.
**/
BOOLEAN
BmIsBootManagerMenuFilePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_HANDLE  FvHandle;
  VOID        *NameGuid;
  EFI_STATUS  Status;

  Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &DevicePath, &FvHandle);
  if (!EFI_ERROR (Status)) {
    NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)DevicePath);
    if (NameGuid != NULL) {
      return CompareGuid (NameGuid, PcdGetPtr (PcdBootManagerMenuFile));
    }
  }

  return FALSE;
}

/**
  Report status code with EFI_RETURN_STATUS_EXTENDED_DATA about LoadImage() or
  StartImage() failure.

  @param[in] ErrorCode      An Error Code in the Software Class, DXE Boot
                            Service Driver Subclass. ErrorCode will be used to
                            compose the Value parameter for status code
                            reporting. Must be one of
                            EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR and
                            EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED.

  @param[in] FailureStatus  The failure status returned by the boot service
                            that should be reported.
**/
VOID
BmReportLoadFailure (
  IN UINT32      ErrorCode,
  IN EFI_STATUS  FailureStatus
  )
{
  EFI_RETURN_STATUS_EXTENDED_DATA  ExtendedData;

  if (!ReportErrorCodeEnabled ()) {
    return;
  }

  ASSERT (
    (ErrorCode == EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR) ||
    (ErrorCode == EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED)
    );

  ZeroMem (&ExtendedData, sizeof (ExtendedData));
  ExtendedData.ReturnStatus = FailureStatus;

  REPORT_STATUS_CODE_EX (
    (EFI_ERROR_CODE | EFI_ERROR_MINOR),
    (EFI_SOFTWARE_DXE_BS_DRIVER | ErrorCode),
    0,
    NULL,
    NULL,
    &ExtendedData.DataHeader + 1,
    sizeof (ExtendedData) - sizeof (ExtendedData.DataHeader)
    );
}

/**
  Attempt to boot the EFI boot option. This routine sets L"BootCurent" and
  also signals the EFI ready to boot event. If the device path for the option
  starts with a BBS device path a legacy boot is attempted via the registered
  gLegacyBoot function. Short form device paths are also supported via this
  rountine. A device path starting with MEDIA_HARDDRIVE_DP, MSG_USB_WWID_DP,
  MSG_USB_CLASS_DP gets expaned out to find the first device that matches.
  If the BootOption Device Path fails the removable media boot algorithm
  is attempted (\EFI\BOOTIA32.EFI, \EFI\BOOTX64.EFI,... only one file type
  is tried per processor type)

  @param  BootOption    Boot Option to try and boot.
                        On return, BootOption->Status contains the boot status.
                        EFI_SUCCESS     BootOption was booted
                        EFI_UNSUPPORTED A BBS device path was found with no valid callback
                                        registered via EfiBootManagerInitialize().
                        EFI_NOT_FOUND   The BootOption was not found on the system
                        !EFI_SUCCESS    BootOption failed with this error status

**/
VOID
EFIAPI
EfiBootManagerBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *ImageInfo;
  UINT16                     Uint16;
  UINTN                      OptionNumber;
  UINTN                      OriginalOptionNumber;
  EFI_DEVICE_PATH_PROTOCOL   *FilePath;
  EFI_DEVICE_PATH_PROTOCOL   *RamDiskDevicePath;
  VOID                       *FileBuffer;
  UINTN                      FileSize;
  EFI_BOOT_LOGO_PROTOCOL     *BootLogo;
  EFI_EVENT                  LegacyBootEvent;

  if (BootOption == NULL) {
    return;
  }

  if ((BootOption->FilePath == NULL) || (BootOption->OptionType != LoadOptionTypeBoot)) {
    BootOption->Status = EFI_INVALID_PARAMETER;
    return;
  }

  //
  // 1. Create Boot#### for a temporary boot if there is no match Boot#### (i.e. a boot by selected a EFI Shell using "Boot From File")
  //
  OptionNumber = BmFindBootOptionInVariable (BootOption);
  if (OptionNumber == LoadOptionNumberUnassigned) {
    Status = BmGetFreeOptionNumber (LoadOptionTypeBoot, &Uint16);
    if (!EFI_ERROR (Status)) {
      //
      // Save the BootOption->OptionNumber to restore later
      //
      OptionNumber             = Uint16;
      OriginalOptionNumber     = BootOption->OptionNumber;
      BootOption->OptionNumber = OptionNumber;
      Status                   = EfiBootManagerLoadOptionToVariable (BootOption);
      BootOption->OptionNumber = OriginalOptionNumber;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[Bds] Failed to create Boot#### for a temporary boot - %r!\n", Status));
      BootOption->Status = Status;
      return;
    }
  }

  //
  // 2. Set BootCurrent
  //
  Uint16 = (UINT16)OptionNumber;
  BmSetVariableAndReportStatusCodeOnError (
    L"BootCurrent",
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof (UINT16),
    &Uint16
    );

  //
  // 3. Signal the EVT_SIGNAL_READY_TO_BOOT event when we are about to load and execute
  //    the boot option.
  //
  if (BmIsBootManagerMenuFilePath (BootOption->FilePath)) {
    DEBUG ((DEBUG_INFO, "[Bds] Booting Boot Manager Menu.\n"));
    BmStopHotkeyService (NULL, NULL);
  } else {
    EfiSignalEventReadyToBoot ();
    //
    // Report Status Code to indicate ReadyToBoot was signalled
    //
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT));
    //
    // 4. Repair system through DriverHealth protocol
    //
    BmRepairAllControllers (0);
  }

  PERF_START_EX (gImageHandle, "BdsAttempt", NULL, 0, (UINT32)OptionNumber);

  //
  // 5. Adjust the different type memory page number just before booting
  //    and save the updated info into the variable for next boot to use
  //
  BmSetMemoryTypeInformationVariable (
    (BOOLEAN)((BootOption->Attributes & LOAD_OPTION_CATEGORY) == LOAD_OPTION_CATEGORY_BOOT)
    );

  //
  // 6. Load EFI boot option to ImageHandle
  //
  DEBUG_CODE_BEGIN ();
  if (BootOption->Description == NULL) {
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "[Bds]Booting from unknown device path\n"));
  } else {
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "[Bds]Booting %s\n", BootOption->Description));
  }

  DEBUG_CODE_END ();

  ImageHandle       = NULL;
  RamDiskDevicePath = NULL;
  if (DevicePathType (BootOption->FilePath) != BBS_DEVICE_PATH) {
    Status   = EFI_NOT_FOUND;
    FilePath = NULL;
    EfiBootManagerConnectDevicePath (BootOption->FilePath, NULL);
    FileBuffer = BmGetNextLoadOptionBuffer (LoadOptionTypeBoot, BootOption->FilePath, &FilePath, &FileSize);
    if (FileBuffer != NULL) {
      RamDiskDevicePath = BmGetRamDiskDevicePath (FilePath);

      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeOsLoaderLoad));
      Status = gBS->LoadImage (
                      TRUE,
                      gImageHandle,
                      FilePath,
                      FileBuffer,
                      FileSize,
                      &ImageHandle
                      );
    }

    if (FileBuffer != NULL) {
      FreePool (FileBuffer);
    }

    if (FilePath != NULL) {
      FreePool (FilePath);
    }

    if (EFI_ERROR (Status)) {
      //
      // With EFI_SECURITY_VIOLATION retval, the Image was loaded and an ImageHandle was created
      // with a valid EFI_LOADED_IMAGE_PROTOCOL, but the image can not be started right now.
      // If the caller doesn't have the option to defer the execution of an image, we should
      // unload image for the EFI_SECURITY_VIOLATION to avoid resource leak.
      //
      if (Status == EFI_SECURITY_VIOLATION) {
        gBS->UnloadImage (ImageHandle);
      }

      //
      // Destroy the RAM disk
      //
      if (RamDiskDevicePath != NULL) {
        BmDestroyRamDisk (RamDiskDevicePath);
        FreePool (RamDiskDevicePath);
      }

      //
      // Report Status Code with the failure status to indicate that the failure to load boot option
      //
      BmReportLoadFailure (EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR, Status);
      BootOption->Status = Status;
      return;
    }
  }

  //
  // Check to see if we should legacy BOOT. If yes then do the legacy boot
  // Write boot to OS performance data for Legacy boot
  //
  if ((DevicePathType (BootOption->FilePath) == BBS_DEVICE_PATH) && (DevicePathSubType (BootOption->FilePath) == BBS_BBS_DP)) {
    if (mBmLegacyBoot != NULL) {
      //
      // Write boot to OS performance data for legacy boot.
      //
      PERF_CODE (
        //
        // Create an event to be signalled when Legacy Boot occurs to write performance data.
        //
        Status = EfiCreateEventLegacyBootEx (
                   TPL_NOTIFY,
                   BmEndOfBdsPerfCode,
                   NULL,
                   &LegacyBootEvent
                   );
        ASSERT_EFI_ERROR (Status);
        );

      mBmLegacyBoot (BootOption);
    } else {
      BootOption->Status = EFI_UNSUPPORTED;
    }

    PERF_END_EX (gImageHandle, "BdsAttempt", NULL, 0, (UINT32)OptionNumber);
    return;
  }

  //
  // Provide the image with its load options
  //
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
  ASSERT_EFI_ERROR (Status);

  if (!BmIsAutoCreateBootOption (BootOption)) {
    ImageInfo->LoadOptionsSize = BootOption->OptionalDataSize;
    ImageInfo->LoadOptions     = BootOption->OptionalData;
  }

  //
  // Clean to NULL because the image is loaded directly from the firmwares boot manager.
  //
  ImageInfo->ParentHandle = NULL;

  //
  // Before calling the image, enable the Watchdog Timer for 5 minutes period
  //
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

  //
  // Write boot to OS performance data for UEFI boot
  //
  PERF_CODE (
    BmEndOfBdsPerfCode (NULL, NULL);
    );

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeOsLoaderStart));

  Status = gBS->StartImage (ImageHandle, &BootOption->ExitDataSize, &BootOption->ExitData);
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Image Return Status = %r\n", Status));
  BootOption->Status = Status;

  //
  // Destroy the RAM disk
  //
  if (RamDiskDevicePath != NULL) {
    BmDestroyRamDisk (RamDiskDevicePath);
    FreePool (RamDiskDevicePath);
  }

  if (EFI_ERROR (Status)) {
    //
    // Report Status Code with the failure status to indicate that boot failure
    //
    BmReportLoadFailure (EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED, Status);
  }

  PERF_END_EX (gImageHandle, "BdsAttempt", NULL, 0, (UINT32)OptionNumber);

  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  //
  // Set Logo status invalid after trying one boot option
  //
  BootLogo = NULL;
  Status   = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **)&BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    Status = BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Clear Boot Current
  //
  Status = gRT->SetVariable (
                  L"BootCurrent",
                  &gEfiGlobalVariableGuid,
                  0,
                  0,
                  NULL
                  );
  //
  // Deleting variable with current variable implementation shouldn't fail.
  // When BootXXXX (e.g.: BootManagerMenu) boots BootYYYY, exiting BootYYYY causes BootCurrent deleted,
  // exiting BootXXXX causes deleting BootCurrent returns EFI_NOT_FOUND.
  //
  ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);
}

/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
BOOLEAN
BmMatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH     *HardDriveDevicePath
  )
{
  HARDDRIVE_DEVICE_PATH  *Node;

  if ((BlockIoDevicePath == NULL) || (HardDriveDevicePath == NULL)) {
    return FALSE;
  }

  //
  // Match all the partition device path nodes including the nested partition nodes
  //
  while (!IsDevicePathEnd (BlockIoDevicePath)) {
    if ((DevicePathType (BlockIoDevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (BlockIoDevicePath) == MEDIA_HARDDRIVE_DP)
        )
    {
      //
      // See if the harddrive device path in blockio matches the orig Hard Drive Node
      //
      Node = (HARDDRIVE_DEVICE_PATH *)BlockIoDevicePath;

      //
      // Match Signature and PartitionNumber.
      // Unused bytes in Signature are initiaized with zeros.
      //
      if ((Node->PartitionNumber == HardDriveDevicePath->PartitionNumber) &&
          (Node->MBRType == HardDriveDevicePath->MBRType) &&
          (Node->SignatureType == HardDriveDevicePath->SignatureType) &&
          (CompareMem (Node->Signature, HardDriveDevicePath->Signature, sizeof (Node->Signature)) == 0))
      {
        return TRUE;
      }
    }

    BlockIoDevicePath = NextDevicePathNode (BlockIoDevicePath);
  }

  return FALSE;
}

/**
  Emuerate all possible bootable medias in the following order:
  1. Removable BlockIo            - The boot option only points to the removable media
                                    device, like USB key, DVD, Floppy etc.
  2. Fixed BlockIo                - The boot option only points to a Fixed blockIo device,
                                    like HardDisk.
  3. Non-BlockIo SimpleFileSystem - The boot option points to a device supporting
                                    SimpleFileSystem Protocol, but not supporting BlockIo
                                    protocol.
  4. LoadFile                     - The boot option points to the media supporting
                                    LoadFile protocol.
  Reference: UEFI Spec chapter 3.3 Boot Option Variables Default Boot Behavior

  @param BootOptionCount   Return the boot option count which has been found.

  @retval   Pointer to the boot option array.
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
BmEnumerateBootOptions (
  UINTN  *BootOptionCount
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         HandleCount;
  EFI_HANDLE                    *Handles;
  EFI_BLOCK_IO_PROTOCOL         *BlkIo;
  UINTN                         Removable;
  UINTN                         Index;
  CHAR16                        *Description;

  ASSERT (BootOptionCount != NULL);

  *BootOptionCount = 0;
  BootOptions      = NULL;

  //
  // Parse removable block io followed by fixed block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiBlockIoProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );

  for (Removable = 0; Removable < 2; Removable++) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Handles[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **)&BlkIo
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Skip the logical partitions
      //
      if (BlkIo->Media->LogicalPartition) {
        continue;
      }

      //
      // Skip the fixed block io then the removable block io
      //
      if (BlkIo->Media->RemovableMedia == ((Removable == 0) ? FALSE : TRUE)) {
        continue;
      }

      //
      // Skip removable media if not present
      //
      if ((BlkIo->Media->RemovableMedia == TRUE) &&
          (BlkIo->Media->MediaPresent == FALSE))
      {
        continue;
      }

      Description = BmGetBootDescription (Handles[Index]);
      BootOptions = ReallocatePool (
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                      BootOptions
                      );
      ASSERT (BootOptions != NULL);

      Status = EfiBootManagerInitializeLoadOption (
                 &BootOptions[(*BootOptionCount)++],
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 LOAD_OPTION_ACTIVE,
                 Description,
                 DevicePathFromHandle (Handles[Index]),
                 NULL,
                 0
                 );
      ASSERT_EFI_ERROR (Status);

      FreePool (Description);
    }
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse simple file system not based on block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **)&BlkIo
                    );
    if (!EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle supports a BlkIo protocol, which we've handled in above
      //
      continue;
    }

    Description = BmGetBootDescription (Handles[Index]);
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (Description);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse load file protocol
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiLoadFileProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {
    //
    // Ignore BootManagerMenu. its boot option will be created by EfiBootManagerGetBootManagerMenu().
    //
    if (BmIsBootManagerMenuFilePath (DevicePathFromHandle (Handles[Index]))) {
      continue;
    }

    Description = BmGetBootDescription (Handles[Index]);
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (Description);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  BmMakeBootOptionDescriptionUnique (BootOptions, *BootOptionCount);
  return BootOptions;
}

/**
  The function enumerates all boot options, creates them and registers them in the BootOrder variable.
**/
VOID
EFIAPI
EfiBootManagerRefreshAllBootOption (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MANAGER_LOAD_OPTION          *NvBootOptions;
  UINTN                                 NvBootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION          *BootOptions;
  UINTN                                 BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION          *UpdatedBootOptions;
  UINTN                                 UpdatedBootOptionCount;
  UINTN                                 Index;
  EDKII_PLATFORM_BOOT_MANAGER_PROTOCOL  *PlatformBootManager;

  //
  // Optionally refresh the legacy boot option
  //
  if (mBmRefreshLegacyBootOption != NULL) {
    mBmRefreshLegacyBootOption ();
  }

  BootOptions = BmEnumerateBootOptions (&BootOptionCount);

  //
  // Mark the boot option as added by BDS by setting OptionalData to a special GUID
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    BootOptions[Index].OptionalData     = AllocateCopyPool (sizeof (EFI_GUID), &mBmAutoCreateBootOptionGuid);
    BootOptions[Index].OptionalDataSize = sizeof (EFI_GUID);
  }

  //
  // Locate Platform Boot Options Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEdkiiPlatformBootManagerProtocolGuid,
                  NULL,
                  (VOID **)&PlatformBootManager
                  );
  if (!EFI_ERROR (Status)) {
    //
    // If found, call platform specific refresh to all auto enumerated and NV
    // boot options.
    //
    Status = PlatformBootManager->RefreshAllBootOptions (
                                    (CONST EFI_BOOT_MANAGER_LOAD_OPTION *)BootOptions,
                                    (CONST UINTN)BootOptionCount,
                                    &UpdatedBootOptions,
                                    &UpdatedBootOptionCount
                                    );
    if (!EFI_ERROR (Status)) {
      EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
      BootOptions     = UpdatedBootOptions;
      BootOptionCount = UpdatedBootOptionCount;
    }
  }

  NvBootOptions = EfiBootManagerGetLoadOptions (&NvBootOptionCount, LoadOptionTypeBoot);

  //
  // Remove invalid EFI boot options from NV
  //
  for (Index = 0; Index < NvBootOptionCount; Index++) {
    if (((DevicePathType (NvBootOptions[Index].FilePath) != BBS_DEVICE_PATH) ||
         (DevicePathSubType (NvBootOptions[Index].FilePath) != BBS_BBS_DP)
         ) && BmIsAutoCreateBootOption (&NvBootOptions[Index])
        )
    {
      //
      // Only check those added by BDS
      // so that the boot options added by end-user or OS installer won't be deleted
      //
      if (EfiBootManagerFindLoadOption (&NvBootOptions[Index], BootOptions, BootOptionCount) == -1) {
        Status = EfiBootManagerDeleteLoadOptionVariable (NvBootOptions[Index].OptionNumber, LoadOptionTypeBoot);
        //
        // Deleting variable with current variable implementation shouldn't fail.
        //
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  //
  // Add new EFI boot options to NV
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    if (EfiBootManagerFindLoadOption (&BootOptions[Index], NvBootOptions, NvBootOptionCount) == -1) {
      EfiBootManagerAddLoadOptionVariable (&BootOptions[Index], (UINTN)-1);
      //
      // Try best to add the boot options so continue upon failure.
      //
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  EfiBootManagerFreeLoadOptions (NvBootOptions, NvBootOptionCount);
}

/**
  This function is called to get or create the boot option for the Boot Manager Menu.

  The Boot Manager Menu is shown after successfully booting a boot option.
  This function will first try to search the BootManagerMenuFile is in the same FV as
  the module links to this library. If fails, it will search in all FVs.

  @param  BootOption    Return the boot option of the Boot Manager Menu

  @retval EFI_SUCCESS   Successfully register the Boot Manager Menu.
  @retval EFI_NOT_FOUND The Boot Manager Menu cannot be found.
  @retval others        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS
                        and EFI_NOT_FOUND.
**/
EFI_STATUS
BmRegisterBootManagerMenu (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                Status;
  CHAR16                    *Description;
  UINTN                     DescriptionLength;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     HandleCount;
  EFI_HANDLE                *Handles;
  UINTN                     Index;

  DevicePath  = NULL;
  Description = NULL;
  //
  // Try to find BootManagerMenu from LoadFile protocol
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiLoadFileProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {
    if (BmIsBootManagerMenuFilePath (DevicePathFromHandle (Handles[Index]))) {
      DevicePath  = DuplicateDevicePath (DevicePathFromHandle (Handles[Index]));
      Description = BmGetBootDescription (Handles[Index]);
      break;
    }
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  if (DevicePath == NULL) {
    Status = GetFileDevicePathFromAnyFv (
               PcdGetPtr (PcdBootManagerMenuFile),
               EFI_SECTION_PE32,
               0,
               &DevicePath
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[Bds]BootManagerMenu FFS section can not be found, skip its boot option registration\n"));
      return EFI_NOT_FOUND;
    }

    ASSERT (DevicePath != NULL);
    //
    // Get BootManagerMenu application's description from EFI User Interface Section.
    //
    Status = GetSectionFromAnyFv (
               PcdGetPtr (PcdBootManagerMenuFile),
               EFI_SECTION_USER_INTERFACE,
               0,
               (VOID **)&Description,
               &DescriptionLength
               );
    if (EFI_ERROR (Status)) {
      Description = NULL;
    }
  }

  Status = EfiBootManagerInitializeLoadOption (
             BootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN,
             (Description != NULL) ? Description : L"Boot Manager Menu",
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);
  if (Description != NULL) {
    FreePool (Description);
  }

  DEBUG_CODE (
    EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions;
    UINTN                           BootOptionCount;

    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
    ASSERT (EfiBootManagerFindLoadOption (BootOption, BootOptions, BootOptionCount) == -1);
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
    );

  return EfiBootManagerAddLoadOptionVariable (BootOption, (UINTN)-1);
}

/**
  Return the boot option corresponding to the Boot Manager Menu.
  It may automatically create one if the boot option hasn't been created yet.

  @param BootOption    Return the Boot Manager Menu.

  @retval EFI_SUCCESS   The Boot Manager Menu is successfully returned.
  @retval EFI_NOT_FOUND The Boot Manager Menu cannot be found.
  @retval others        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS
                        and EFI_NOT_FOUND.
**/
EFI_STATUS
EFIAPI
EfiBootManagerGetBootManagerMenu (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                    Status;
  UINTN                         BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         Index;

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
    if (BmIsBootManagerMenuFilePath (BootOptions[Index].FilePath)) {
      Status = EfiBootManagerInitializeLoadOption (
                 BootOption,
                 BootOptions[Index].OptionNumber,
                 BootOptions[Index].OptionType,
                 BootOptions[Index].Attributes,
                 BootOptions[Index].Description,
                 BootOptions[Index].FilePath,
                 BootOptions[Index].OptionalData,
                 BootOptions[Index].OptionalDataSize
                 );
      ASSERT_EFI_ERROR (Status);
      break;
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);

  //
  // Automatically create the Boot#### for Boot Manager Menu when not found.
  //
  if (Index == BootOptionCount) {
    return BmRegisterBootManagerMenu (BootOption);
  } else {
    return EFI_SUCCESS;
  }
}

/**
  Get the next possible full path pointing to the load option.
  The routine doesn't guarantee the returned full path points to an existing
  file, and it also doesn't guarantee the existing file is a valid load option.
  BmGetNextLoadOptionBuffer() guarantees.

  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  The full path returned by the routine in last call.
                   Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerGetNextLoadOptionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  )
{
  return BmGetNextLoadOptionDevicePath (FilePath, FullPath);
}
