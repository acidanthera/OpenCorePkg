/** @file
  Library functions which relate with boot option description.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBm.h"

#define VENDOR_IDENTIFICATION_OFFSET   3
#define VENDOR_IDENTIFICATION_LENGTH   8
#define PRODUCT_IDENTIFICATION_OFFSET  11
#define PRODUCT_IDENTIFICATION_LENGTH  16

CONST UINT16  mBmUsbLangId    = 0x0409; // English
CHAR16        mBmUefiPrefix[] = L"UEFI ";

LIST_ENTRY  mPlatformBootDescriptionHandlers = INITIALIZE_LIST_HEAD_VARIABLE (mPlatformBootDescriptionHandlers);

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath        The bootable device Path to check

  @retval AcpiFloppyBoot    If given device path contains ACPI_DEVICE_PATH type device path node
                            which HID is floppy device.
  @retval MessageAtapiBoot  If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_ATAPI_DP.
  @retval MessageSataBoot   If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_SATA_DP.
  @retval MessageScsiBoot   If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_SCSI_DP.
  @retval MessageUsbBoot    If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_USB_DP.
  @retval BmMiscBoot        If tiven device path doesn't match the above condition.

**/
BM_BOOT_TYPE
BmDevicePathType (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;

  ASSERT (DevicePath != NULL);

  for (Node = DevicePath; !IsDevicePathEndType (Node); Node = NextDevicePathNode (Node)) {
    switch (DevicePathType (Node)) {
      case ACPI_DEVICE_PATH:
        if (EISA_ID_TO_NUM (((ACPI_HID_DEVICE_PATH *)Node)->HID) == 0x0604) {
          return BmAcpiFloppyBoot;
        }

        break;

      case HARDWARE_DEVICE_PATH:
        if (DevicePathSubType (Node) == HW_CONTROLLER_DP) {
          return BmHardwareDeviceBoot;
        }

        break;

      case MESSAGING_DEVICE_PATH:
        //
        // Skip LUN device node
        //
        NextNode = Node;
        do {
          NextNode = NextDevicePathNode (NextNode);
        } while (
                 (DevicePathType (NextNode) == MESSAGING_DEVICE_PATH) &&
                 (DevicePathSubType (NextNode) == MSG_DEVICE_LOGICAL_UNIT_DP)
                 );

        //
        // If the device path not only point to driver device, it is not a messaging device path,
        //
        if (!IsDevicePathEndType (NextNode)) {
          continue;
        }

        switch (DevicePathSubType (Node)) {
          case MSG_ATAPI_DP:
            return BmMessageAtapiBoot;
            break;

          case MSG_SATA_DP:
            return BmMessageSataBoot;
            break;

          case MSG_USB_DP:
            return BmMessageUsbBoot;
            break;

          case MSG_SCSI_DP:
            return BmMessageScsiBoot;
            break;
        }
    }
  }

  return BmMiscBoot;
}

/**
  Eliminate the extra spaces in the Str to one space.

  @param    Str     Input string info.
**/
VOID
BmEliminateExtraSpaces (
  IN CHAR16  *Str
  )
{
  UINTN  Index;
  UINTN  ActualIndex;

  for (Index = 0, ActualIndex = 0; Str[Index] != L'\0'; Index++) {
    if ((Str[Index] != L' ') || ((ActualIndex > 0) && (Str[ActualIndex - 1] != L' '))) {
      Str[ActualIndex++] = Str[Index];
    }
  }

  Str[ActualIndex] = L'\0';
}

/**
  Try to get the controller's ATA/ATAPI description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetDescriptionFromDiskInfo (
  IN EFI_HANDLE  Handle
  )
{
  UINTN                     Index;
  EFI_STATUS                Status;
  EFI_DISK_INFO_PROTOCOL    *DiskInfo;
  UINT32                    BufferSize;
  EFI_ATAPI_IDENTIFY_DATA   IdentifyData;
  EFI_SCSI_INQUIRY_DATA     InquiryData;
  CHAR16                    *Description;
  UINTN                     Length;
  CONST UINTN               ModelNameLength    = 40;
  CONST UINTN               SerialNumberLength = 20;
  CHAR8                     *StrPtr;
  UINT8                     Temp;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  Description = NULL;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID **)&DiskInfo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid) ||
      CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid))
  {
    BufferSize = sizeof (EFI_ATAPI_IDENTIFY_DATA);
    Status     = DiskInfo->Identify (
                             DiskInfo,
                             &IdentifyData,
                             &BufferSize
                             );
    if (!EFI_ERROR (Status)) {
      Description = AllocateZeroPool ((ModelNameLength + SerialNumberLength + 2) * sizeof (CHAR16));
      ASSERT (Description != NULL);
      for (Index = 0; Index + 1 < ModelNameLength; Index += 2) {
        Description[Index]     = (CHAR16)IdentifyData.ModelName[Index + 1];
        Description[Index + 1] = (CHAR16)IdentifyData.ModelName[Index];
      }

      Length                = Index;
      Description[Length++] = L' ';

      for (Index = 0; Index + 1 < SerialNumberLength; Index += 2) {
        Description[Length + Index]     = (CHAR16)IdentifyData.SerialNo[Index + 1];
        Description[Length + Index + 1] = (CHAR16)IdentifyData.SerialNo[Index];
      }

      Length               += Index;
      Description[Length++] = L'\0';
      ASSERT (Length == ModelNameLength + SerialNumberLength + 2);

      BmEliminateExtraSpaces (Description);
    }
  } else if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoScsiInterfaceGuid) ||
             CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoUfsInterfaceGuid))
  {
    BufferSize = sizeof (EFI_SCSI_INQUIRY_DATA);
    Status     = DiskInfo->Inquiry (
                             DiskInfo,
                             &InquiryData,
                             &BufferSize
                             );
    if (!EFI_ERROR (Status)) {
      Description = AllocateZeroPool ((VENDOR_IDENTIFICATION_LENGTH + PRODUCT_IDENTIFICATION_LENGTH + 2) * sizeof (CHAR16));
      ASSERT (Description != NULL);

      //
      // Per SCSI spec, EFI_SCSI_INQUIRY_DATA.Reserved_5_95[3 - 10] save the Verdor identification
      // EFI_SCSI_INQUIRY_DATA.Reserved_5_95[11 - 26] save the product identification,
      // Here combine the vendor identification and product identification to the description.
      //
      StrPtr                               = (CHAR8 *)(&InquiryData.Reserved_5_95[VENDOR_IDENTIFICATION_OFFSET]);
      Temp                                 = StrPtr[VENDOR_IDENTIFICATION_LENGTH];
      StrPtr[VENDOR_IDENTIFICATION_LENGTH] = '\0';
      AsciiStrToUnicodeStrS (StrPtr, Description, VENDOR_IDENTIFICATION_LENGTH + 1);
      StrPtr[VENDOR_IDENTIFICATION_LENGTH] = Temp;

      //
      // Add one space at the middle of vendor information and product information.
      //
      Description[VENDOR_IDENTIFICATION_LENGTH] = L' ';

      StrPtr                                = (CHAR8 *)(&InquiryData.Reserved_5_95[PRODUCT_IDENTIFICATION_OFFSET]);
      StrPtr[PRODUCT_IDENTIFICATION_LENGTH] = '\0';
      AsciiStrToUnicodeStrS (StrPtr, Description + VENDOR_IDENTIFICATION_LENGTH + 1, PRODUCT_IDENTIFICATION_LENGTH + 1);

      BmEliminateExtraSpaces (Description);
    }
  } else if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoSdMmcInterfaceGuid)) {
    DevicePath = DevicePathFromHandle (Handle);
    if (DevicePath == NULL) {
      return NULL;
    }

    while (!IsDevicePathEnd (DevicePath) && (DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH)) {
      DevicePath = NextDevicePathNode (DevicePath);
    }

    if (IsDevicePathEnd (DevicePath)) {
      return NULL;
    }

    if (DevicePathSubType (DevicePath) == MSG_SD_DP) {
      Description = L"SD Device";
    } else if (DevicePathSubType (DevicePath) == MSG_EMMC_DP) {
      Description = L"eMMC Device";
    } else {
      return NULL;
    }

    Description = AllocateCopyPool (StrSize (Description), Description);
  }

  return Description;
}

/**
  Try to get the controller's USB description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetUsbDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                 Status;
  EFI_USB_IO_PROTOCOL        *UsbIo;
  CHAR16                     NullChar;
  CHAR16                     *Manufacturer;
  CHAR16                     *Product;
  CHAR16                     *SerialNumber;
  CHAR16                     *Description;
  EFI_USB_DEVICE_DESCRIPTOR  DevDesc;
  UINTN                      DescMaxSize;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  NullChar = L'\0';

  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    mBmUsbLangId,
                    DevDesc.StrManufacturer,
                    &Manufacturer
                    );
  if (EFI_ERROR (Status)) {
    Manufacturer = &NullChar;
  }

  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    mBmUsbLangId,
                    DevDesc.StrProduct,
                    &Product
                    );
  if (EFI_ERROR (Status)) {
    Product = &NullChar;
  }

  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    mBmUsbLangId,
                    DevDesc.StrSerialNumber,
                    &SerialNumber
                    );
  if (EFI_ERROR (Status)) {
    SerialNumber = &NullChar;
  }

  if ((Manufacturer == &NullChar) &&
      (Product == &NullChar) &&
      (SerialNumber == &NullChar)
      )
  {
    return NULL;
  }

  DescMaxSize = StrSize (Manufacturer) + StrSize (Product) + StrSize (SerialNumber);
  Description = AllocateZeroPool (DescMaxSize);
  ASSERT (Description != NULL);
  StrCatS (Description, DescMaxSize/sizeof (CHAR16), Manufacturer);
  StrCatS (Description, DescMaxSize/sizeof (CHAR16), L" ");

  StrCatS (Description, DescMaxSize/sizeof (CHAR16), Product);
  StrCatS (Description, DescMaxSize/sizeof (CHAR16), L" ");

  StrCatS (Description, DescMaxSize/sizeof (CHAR16), SerialNumber);

  if (Manufacturer != &NullChar) {
    FreePool (Manufacturer);
  }

  if (Product != &NullChar) {
    FreePool (Product);
  }

  if (SerialNumber != &NullChar) {
    FreePool (SerialNumber);
  }

  BmEliminateExtraSpaces (Description);

  return Description;
}

/**
  Return the description for network boot device.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetNetworkDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  MAC_ADDR_DEVICE_PATH      *Mac;
  VLAN_DEVICE_PATH          *Vlan;
  EFI_DEVICE_PATH_PROTOCOL  *Ip;
  EFI_DEVICE_PATH_PROTOCOL  *Uri;
  CHAR16                    *Description;
  UINTN                     DescriptionSize;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiLoadFileProtocolGuid,
                  NULL,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status) || (DevicePath == NULL)) {
    return NULL;
  }

  //
  // The PXE device path is like:
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv4(...)
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)
  //
  // The HTTP device path is like:
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv4(...)[/Dns(...)]/Uri(...)
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)[/Dns(...)]/Uri(...)
  //
  while (!IsDevicePathEnd (DevicePath) &&
         ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
          (DevicePathSubType (DevicePath) != MSG_MAC_ADDR_DP))
         )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (IsDevicePathEnd (DevicePath)) {
    return NULL;
  }

  Mac        = (MAC_ADDR_DEVICE_PATH *)DevicePath;
  DevicePath = NextDevicePathNode (DevicePath);

  //
  // Locate the optional Vlan node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_VLAN_DP)
      )
  {
    Vlan       = (VLAN_DEVICE_PATH *)DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Vlan = NULL;
  }

  //
  // Skip the optional Wi-Fi node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_WIFI_DP)
      )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Locate the IP node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      ((DevicePathSubType (DevicePath) == MSG_IPv4_DP) ||
       (DevicePathSubType (DevicePath) == MSG_IPv6_DP))
      )
  {
    Ip         = DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Ip = NULL;
  }

  //
  // Skip the optional DNS node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_DNS_DP)
      )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Locate the URI node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_URI_DP)
      )
  {
    Uri        = DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Uri = NULL;
  }

  //
  // Build description like below:
  //   "PXEv6 (MAC:112233445566 VLAN1)"
  //   "HTTPv4 (MAC:112233445566)"
  //
  DescriptionSize = sizeof (L"HTTPv6 (MAC:112233445566 VLAN65535)");
  Description     = AllocatePool (DescriptionSize);
  ASSERT (Description != NULL);
  UnicodeSPrint (
    Description,
    DescriptionSize,
    (Vlan == NULL) ?
    L"%sv%d (MAC:%02x%02x%02x%02x%02x%02x)" :
    L"%sv%d (MAC:%02x%02x%02x%02x%02x%02x VLAN%d)",
    (Uri == NULL) ? L"PXE" : L"HTTP",
    ((Ip == NULL) || (DevicePathSubType (Ip) == MSG_IPv4_DP)) ? 4 : 6,
    Mac->MacAddress.Addr[0],
    Mac->MacAddress.Addr[1],
    Mac->MacAddress.Addr[2],
    Mac->MacAddress.Addr[3],
    Mac->MacAddress.Addr[4],
    Mac->MacAddress.Addr[5],
    (Vlan == NULL) ? 0 : Vlan->VlanId
    );
  return Description;
}

/**
  Return the boot description for LoadFile

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetLoadFileDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;
  CHAR16                    *Description;
  EFI_LOAD_FILE_PROTOCOL    *LoadFile;

  Status = gBS->HandleProtocol (Handle, &gEfiLoadFileProtocolGuid, (VOID **)&LoadFile);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get the file name
  //
  Description = NULL;
  Status      = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&FilePath);
  if (!EFI_ERROR (Status)) {
    DevicePathNode = FilePath;
    while (!IsDevicePathEnd (DevicePathNode)) {
      if ((DevicePathNode->Type == MEDIA_DEVICE_PATH) && (DevicePathNode->SubType == MEDIA_FILEPATH_DP)) {
        Description = (CHAR16 *)(DevicePathNode + 1);
        break;
      }

      DevicePathNode = NextDevicePathNode (DevicePathNode);
    }
  }

  if (Description != NULL) {
    return AllocateCopyPool (StrSize (Description), Description);
  }

  return NULL;
}

/**
  Return the boot description for NVME boot device.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetNvmeDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                                Status;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL        *NvmePassthru;
  EFI_DEV_PATH_PTR                          DevicePath;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  NVME_ADMIN_CONTROLLER_DATA                ControllerData;
  CHAR16                                    *Description;
  CHAR16                                    *Char;
  UINTN                                     Index;

  Status = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath.DevPath);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->LocateDevicePath (&gEfiNvmExpressPassThruProtocolGuid, &DevicePath.DevPath, &Handle);
  if (EFI_ERROR (Status) ||
      (DevicePathType (DevicePath.DevPath) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (DevicePath.DevPath) != MSG_NVME_NAMESPACE_DP))
  {
    //
    // Do not return description when the Handle is not a child of NVME controller.
    //
    return NULL;
  }

  //
  // Send ADMIN_IDENTIFY command to NVME controller to get the model and serial number.
  //
  Status = gBS->HandleProtocol (Handle, &gEfiNvmExpressPassThruProtocolGuid, (VOID **)&NvmePassthru);
  ASSERT_EFI_ERROR (Status);

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace data structure.
  //
  Command.Nsid                 = 0;
  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = &ControllerData;
  CommandPacket.TransferLength = sizeof (ControllerData);
  CommandPacket.CommandTimeout = EFI_TIMER_PERIOD_SECONDS (5);
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a controller
  //
  Command.Cdw10 = 1;
  Command.Flags = CDW10_VALID;

  Status = NvmePassthru->PassThru (
                           NvmePassthru,
                           0,
                           &CommandPacket,
                           NULL
                           );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Description = AllocateZeroPool (
                  (ARRAY_SIZE (ControllerData.Mn) + 1
                   + ARRAY_SIZE (ControllerData.Sn) + 1
                   + MAXIMUM_VALUE_CHARACTERS + 1
                  ) * sizeof (CHAR16)
                  );
  if (Description != NULL) {
    Char = Description;
    for (Index = 0; Index < ARRAY_SIZE (ControllerData.Mn); Index++) {
      *(Char++) = (CHAR16)ControllerData.Mn[Index];
    }

    *(Char++) = L' ';
    for (Index = 0; Index < ARRAY_SIZE (ControllerData.Sn); Index++) {
      *(Char++) = (CHAR16)ControllerData.Sn[Index];
    }

    *(Char++) = L' ';
    UnicodeValueToStringS (
      Char,
      sizeof (CHAR16) * (MAXIMUM_VALUE_CHARACTERS + 1),
      0,
      DevicePath.NvmeNamespace->NamespaceId,
      0
      );
    BmEliminateExtraSpaces (Description);
  }

  return Description;
}

/**
  Return the boot description for the controller based on the type.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetMiscDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *Description;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;

  switch (BmDevicePathType (DevicePathFromHandle (Handle))) {
    case BmAcpiFloppyBoot:
      Description = L"Floppy";
      break;

    case BmMessageAtapiBoot:
    case BmMessageSataBoot:
      Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
      ASSERT_EFI_ERROR (Status);
      //
      // Assume a removable SATA device should be the DVD/CD device
      //
      Description = BlockIo->Media->RemovableMedia ? L"DVD/CDROM" : L"Hard Drive";
      break;

    case BmMessageUsbBoot:
      Description = L"USB Device";
      break;

    case BmMessageScsiBoot:
      Description = L"SCSI Device";
      break;

    case BmHardwareDeviceBoot:
      Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
      if (!EFI_ERROR (Status)) {
        Description = BlockIo->Media->RemovableMedia ? L"Removable Disk" : L"Hard Drive";
      } else {
        Description = L"Misc Device";
      }

      break;

    default:
      Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
      if (!EFI_ERROR (Status)) {
        Description = L"Non-Block Boot Device";
      } else {
        Description = L"Misc Device";
      }

      break;
  }

  return AllocateCopyPool (StrSize (Description), Description);
}

/**
  Register the platform provided boot description handler.

  @param Handler  The platform provided boot description handler

  @retval EFI_SUCCESS          The handler was registered successfully.
  @retval EFI_ALREADY_STARTED  The handler was already registered.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource to perform the registration.
**/
EFI_STATUS
EFIAPI
EfiBootManagerRegisterBootDescriptionHandler (
  IN EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER  Handler
  )
{
  LIST_ENTRY                 *Link;
  BM_BOOT_DESCRIPTION_ENTRY  *Entry;

  for ( Link = GetFirstNode (&mPlatformBootDescriptionHandlers)
        ; !IsNull (&mPlatformBootDescriptionHandlers, Link)
        ; Link = GetNextNode (&mPlatformBootDescriptionHandlers, Link)
        )
  {
    Entry = CR (Link, BM_BOOT_DESCRIPTION_ENTRY, Link, BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE);
    if (Entry->Handler == Handler) {
      return EFI_ALREADY_STARTED;
    }
  }

  Entry = AllocatePool (sizeof (BM_BOOT_DESCRIPTION_ENTRY));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Signature = BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE;
  Entry->Handler   = Handler;
  InsertTailList (&mPlatformBootDescriptionHandlers, &Entry->Link);
  return EFI_SUCCESS;
}

BM_GET_BOOT_DESCRIPTION  mBmBootDescriptionHandlers[] = {
  BmGetUsbDescription,
  BmGetDescriptionFromDiskInfo,
  BmGetNetworkDescription,
  BmGetLoadFileDescription,
  BmGetNvmeDescription,
  BmGetMiscDescription
};

/**
  Return the boot description for the controller.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetBootDescription (
  IN EFI_HANDLE  Handle
  )
{
  LIST_ENTRY                 *Link;
  BM_BOOT_DESCRIPTION_ENTRY  *Entry;
  CHAR16                     *Description;
  CHAR16                     *DefaultDescription;
  CHAR16                     *Temp;
  UINTN                      Index;

  //
  // Firstly get the default boot description
  //
  DefaultDescription = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mBmBootDescriptionHandlers); Index++) {
    DefaultDescription = mBmBootDescriptionHandlers[Index](Handle);
    if (DefaultDescription != NULL) {
      //
      // Avoid description confusion between UEFI & Legacy boot option by adding "UEFI " prefix
      // ONLY for core provided boot description handler.
      //
      Temp = AllocatePool (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix));
      ASSERT (Temp != NULL);
      StrCpyS (Temp, (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix)) / sizeof (CHAR16), mBmUefiPrefix);
      StrCatS (Temp, (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix)) / sizeof (CHAR16), DefaultDescription);
      FreePool (DefaultDescription);
      DefaultDescription = Temp;
      break;
    }
  }

  ASSERT (DefaultDescription != NULL);

  //
  // Secondly query platform for the better boot description
  //
  for ( Link = GetFirstNode (&mPlatformBootDescriptionHandlers)
        ; !IsNull (&mPlatformBootDescriptionHandlers, Link)
        ; Link = GetNextNode (&mPlatformBootDescriptionHandlers, Link)
        )
  {
    Entry       = CR (Link, BM_BOOT_DESCRIPTION_ENTRY, Link, BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE);
    Description = Entry->Handler (Handle, DefaultDescription);
    if (Description != NULL) {
      FreePool (DefaultDescription);
      return Description;
    }
  }

  return DefaultDescription;
}

/**
  Enumerate all boot option descriptions and append " 2"/" 3"/... to make
  unique description.

  @param BootOptions            Array of boot options.
  @param BootOptionCount        Count of boot options.
**/
VOID
BmMakeBootOptionDescriptionUnique (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  UINTN                         BootOptionCount
  )
{
  UINTN    Base;
  UINTN    Index;
  UINTN    DescriptionSize;
  UINTN    MaxSuffixSize;
  BOOLEAN  *Visited;
  UINTN    MatchCount;

  if (BootOptionCount == 0) {
    return;
  }

  //
  // Calculate the maximum buffer size for the number suffix.
  // The initial sizeof (CHAR16) is for the blank space before the number.
  //
  MaxSuffixSize = sizeof (CHAR16);
  for (Index = BootOptionCount; Index != 0; Index = Index / 10) {
    MaxSuffixSize += sizeof (CHAR16);
  }

  Visited = AllocateZeroPool (sizeof (BOOLEAN) * BootOptionCount);
  ASSERT (Visited != NULL);

  for (Base = 0; Base < BootOptionCount; Base++) {
    if (!Visited[Base]) {
      MatchCount      = 1;
      Visited[Base]   = TRUE;
      DescriptionSize = StrSize (BootOptions[Base].Description);
      for (Index = Base + 1; Index < BootOptionCount; Index++) {
        if (!Visited[Index] && (StrCmp (BootOptions[Base].Description, BootOptions[Index].Description) == 0)) {
          Visited[Index] = TRUE;
          MatchCount++;
          FreePool (BootOptions[Index].Description);
          BootOptions[Index].Description = AllocatePool (DescriptionSize + MaxSuffixSize);
          UnicodeSPrint (
            BootOptions[Index].Description,
            DescriptionSize + MaxSuffixSize,
            L"%s %d",
            BootOptions[Base].Description,
            MatchCount
            );
        }
      }
    }
  }

  FreePool (Visited);
}
