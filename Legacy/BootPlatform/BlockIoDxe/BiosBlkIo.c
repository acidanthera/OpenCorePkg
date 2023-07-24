/** @file
  EFI glue for BIOS INT 13h block devices.

  This file is coded to EDD 3.0 as defined by T13 D1386 Revision 4
  Availible on http://www.t13.org/#Project drafts
  Currently at ftp://fission.dt.wdc.com/pub/standards/x3t13/project/d1386r4.pdf

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BiosBlkIo.h"

//
// Global data declaration
//
//
// EFI Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gBiosBlockIoDriverBinding = {
  BiosBlockIoDriverBindingSupported,
  BiosBlockIoDriverBindingStart,
  BiosBlockIoDriverBindingStop,
  0x3,
  NULL,
  NULL
};

//
// Semaphore to control access to global variables mActiveInstances and mBufferUnder1Mb
//
EFI_LOCK  mGlobalDataLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_APPLICATION);

//
// Number of active instances of this protocol.  This is used to allocate/free
// the shared buffer.  You must acquire the semaphore to modify.
//
UINTN  mActiveInstances = 0;

//
// Pointer to the beginning of the buffer used for real mode thunk
// You must acquire the semaphore to modify.
//
EFI_PHYSICAL_ADDRESS  mBufferUnder1Mb = 0;

//
// Address packet is a buffer under 1 MB for all version EDD calls
//
EDD_DEVICE_ADDRESS_PACKET  *mEddBufferUnder1Mb;

//
// This is a buffer for INT 13h func 48 information
//
BIOS_LEGACY_DRIVE  *mLegacyDriverUnder1Mb;

//
// Buffer of 0xFE00 bytes for EDD 1.1 transfer must be under 1 MB
//  0xFE00 bytes is the max transfer size supported.
//
VOID  *mEdd11Buffer;

THUNK_CONTEXT  mThunkContext;

/**
  Driver entry point.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Entrypoint successfully executed.
  @retval Others       Fail to execute entrypoint.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install protocols
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gBiosBlockIoDriverBinding,
             ImageHandle,
             &gBiosBlockIoComponentName,
             &gBiosBlockIoComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Legacy BIOS GUID to mark this driver as a BIOS Thunk Driver
  //
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gEfiLegacyBiosGuid,
                NULL,
                NULL
                );
}

/**
  Check whether the driver supports this device.

  @param  This                   The Udriver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_8259_PROTOCOL  *Legacy8259;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  PCI_TYPE00                Pci;

  //
  // See if the Legacy 8259 Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **)&Legacy8259);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // See if this is a PCI Mass Storage controller by looking at the Command register and
  // Class Code Register
  //
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof (Pci) / sizeof (UINT32), &Pci);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_MASS_STORAGE) ||
      ((Pci.Hdr.ClassCode[2] == PCI_BASE_CLASS_INTELLIGENT) && (Pci.Hdr.ClassCode[1] == PCI_SUB_CLASS_INTELLIGENT))
      )
  {
    Status = EFI_SUCCESS;
  }

Done:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Starts the device with this driver.

  @param  This                   The driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the driver.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_8259_PROTOCOL  *Legacy8259;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT8                     DiskStart;
  UINT8                     DiskEnd;
  BIOS_BLOCK_IO_DEV         *BiosBlockIoPrivate;
  EFI_DEVICE_PATH_PROTOCOL  *PciDevPath;
  UINTN                     Index;
  // UINTN                     Flags;
  UINTN    TmpAddress;
  BOOLEAN  DeviceEnable;

  //
  // Initialize variables
  //
  PciIo      = NULL;
  PciDevPath = NULL;

  DeviceEnable = FALSE;

  DiskStart = 0x80; // HDDs/USBs normally start at 0x80.
  DiskEnd   = 0xE0; // Some systems halt if the CD/DVD drive is probed, these typically have higher drive numbers.

  //
  // See if the Legacy 8259 Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **)&Legacy8259);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  InitializeBiosIntCaller (&mThunkContext);
  InitializeInterruptRedirection (Legacy8259);

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&PciDevPath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Enable the device and make sure VGA cycles are being forwarded to this VGA device
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  DeviceEnable = TRUE;

  /*//
  // Check to see if there is a legacy option ROM image associated with this PCI device
  //
  Status = LegacyBios->CheckPciRom (
                        LegacyBios,
                        Controller,
                        NULL,
                        NULL,
                        &Flags
                        );
  if (EFI_ERROR (Status)) {
    goto Error;
  }*/
  //
  // Post the legacy option ROM if it is available.
  //

  /* Status = LegacyBios->InstallPciRom (
                         LegacyBios,
                         Controller,
                         NULL,
                         &Flags,
                         &DiskStart,
                         &DiskEnd,
                         NULL,
                         NULL
                         );
   if (EFI_ERROR (Status)) {
     goto Error;
   }*/
  //
  // All instances share a buffer under 1MB to put real mode thunk code in
  // If it has not been allocated, then we allocate it.
  //
  if (mBufferUnder1Mb == 0) {
    //
    // Should only be here if there are no active instances
    //
    ASSERT (mActiveInstances == 0);

    //
    // Acquire the lock
    //
    EfiAcquireLock (&mGlobalDataLock);

    //
    // Allocate below 1MB
    //
    mBufferUnder1Mb = 0x00000000000FFFFF;
    Status          = gBS->AllocatePages (AllocateMaxAddress, EfiBootServicesData, BLOCK_IO_BUFFER_PAGE_SIZE, &mBufferUnder1Mb);

    //
    // Release the lock
    //
    EfiReleaseLock (&mGlobalDataLock);

    //
    // Check memory allocation success
    //
    if (EFI_ERROR (Status)) {
      //
      // In checked builds we want to assert if the allocate failed.
      //
      ASSERT_EFI_ERROR (Status);
      Status          = EFI_OUT_OF_RESOURCES;
      mBufferUnder1Mb = 0;
      goto Error;
    }

    TmpAddress = (UINTN)mBufferUnder1Mb;
    //
    // Adjusting the value to be on proper boundary
    //
    mEdd11Buffer = (VOID *)ALIGN_VARIABLE (TmpAddress);

    TmpAddress = (UINTN)mEdd11Buffer + MAX_EDD11_XFER;
    //
    // Adjusting the value to be on proper boundary
    //
    mLegacyDriverUnder1Mb = (BIOS_LEGACY_DRIVE *)ALIGN_VARIABLE (TmpAddress);

    TmpAddress = (UINTN)mLegacyDriverUnder1Mb + sizeof (BIOS_LEGACY_DRIVE);
    //
    // Adjusting the value to be on proper boundary
    //
    mEddBufferUnder1Mb = (EDD_DEVICE_ADDRESS_PACKET *)ALIGN_VARIABLE (TmpAddress);
  }

  //
  // Allocate the private device structure for each disk
  //
  for (Index = DiskStart; Index < DiskEnd; Index++) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (BIOS_BLOCK_IO_DEV),
                    (VOID **)&BiosBlockIoPrivate
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    //
    // Zero the private device structure
    //
    ZeroMem (BiosBlockIoPrivate, sizeof (BIOS_BLOCK_IO_DEV));

    //
    // Initialize the private device structure
    //
    BiosBlockIoPrivate->Signature        = BIOS_CONSOLE_BLOCK_IO_DEV_SIGNATURE;
    BiosBlockIoPrivate->ControllerHandle = Controller;
    BiosBlockIoPrivate->Legacy8259       = Legacy8259;
    BiosBlockIoPrivate->PciIo            = PciIo;

    BiosBlockIoPrivate->Bios.Floppy               = FALSE;
    BiosBlockIoPrivate->Bios.Number               = (UINT8)Index;
    BiosBlockIoPrivate->Bios.Letter               = (UINT8)(Index - 0x80 + 'C');
    BiosBlockIoPrivate->BlockMedia.RemovableMedia = FALSE;

    BiosBlockIoPrivate->ThunkContext = &mThunkContext;

    if (BiosInitBlockIo (BiosBlockIoPrivate)) {
      SetBiosInitBlockIoDevicePath (PciDevPath, &BiosBlockIoPrivate->Bios, &BiosBlockIoPrivate->DevicePath);

      //
      // Install the Block Io Protocol onto a new child handle
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &BiosBlockIoPrivate->Handle,
                      &gEfiBlockIoProtocolGuid,
                      &BiosBlockIoPrivate->BlockIo,
                      &gEfiDevicePathProtocolGuid,
                      BiosBlockIoPrivate->DevicePath,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        gBS->FreePool (BiosBlockIoPrivate);
      }

      //
      // Open For Child Device
      //
      Status = gBS->OpenProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&BiosBlockIoPrivate->PciIo,
                      This->DriverBindingHandle,
                      BiosBlockIoPrivate->Handle,
                      EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                      );
    } else {
      gBS->FreePool (BiosBlockIoPrivate);
    }
  }

Error:
  if (EFI_ERROR (Status)) {
    if (PciIo != NULL) {
      if (DeviceEnable) {
        PciIo->Attributes (
                 PciIo,
                 EfiPciIoAttributeOperationDisable,
                 EFI_PCI_DEVICE_ENABLE,
                 NULL
                 );
      }

      gBS->CloseProtocol (
             Controller,
             &gEfiPciIoProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      if (PciDevPath != NULL) {
        gBS->CloseProtocol (
               Controller,
               &gEfiDevicePathProtocolGuid,
               This->DriverBindingHandle,
               Controller
               );
      }

      if ((mBufferUnder1Mb != 0) && (mActiveInstances == 0)) {
        gBS->FreePages (mBufferUnder1Mb, BLOCK_IO_BUFFER_PAGE_SIZE);

        //
        // Clear the buffer back to 0
        //
        EfiAcquireLock (&mGlobalDataLock);
        mBufferUnder1Mb = 0;
        EfiReleaseLock (&mGlobalDataLock);
      }
    }
  } else {
    //
    // Successfully installed, so increment the number of active instances
    //
    EfiAcquireLock (&mGlobalDataLock);
    mActiveInstances++;
    EfiReleaseLock (&mGlobalDataLock);
  }

  return Status;
}

/**
  Stop the device handled by this driver.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS             Status;
  BOOLEAN                AllChildrenStopped;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  BIOS_BLOCK_IO_DEV      *BiosBlockIoPrivate;
  UINTN                  Index;

  //
  // Decrement the number of active instances
  //
  if (mActiveInstances != 0) {
    //
    // Add a check since the stop function will be called 2 times for each handle
    //
    EfiAcquireLock (&mGlobalDataLock);
    mActiveInstances--;
    EfiReleaseLock (&mGlobalDataLock);
  }

  if ((mActiveInstances == 0) && (mBufferUnder1Mb != 0)) {
    //
    // Free our global buffer
    //
    Status = gBS->FreePages (mBufferUnder1Mb, BLOCK_IO_BUFFER_PAGE_SIZE);
    ASSERT_EFI_ERROR (Status);

    EfiAcquireLock (&mGlobalDataLock);
    mBufferUnder1Mb = 0;
    EfiReleaseLock (&mGlobalDataLock);
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **)&BlockIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BiosBlockIoPrivate = BIOS_BLOCK_IO_FROM_THIS (BlockIo);

    //
    // Release PCI I/O and Block IO Protocols on the clild handle.
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    &BiosBlockIoPrivate->BlockIo,
                    &gEfiDevicePathProtocolGuid,
                    BiosBlockIoPrivate->DevicePath,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }

    //
    // Shutdown the hardware
    //
    BiosBlockIoPrivate->PciIo->Attributes (
                                 BiosBlockIoPrivate->PciIo,
                                 EfiPciIoAttributeOperationDisable,
                                 EFI_PCI_DEVICE_ENABLE,
                                 NULL
                                 );

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           ChildHandleBuffer[Index]
           );

    gBS->FreePool (BiosBlockIoPrivate);
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );

  return EFI_SUCCESS;
}

/**
  Build device path for device.

  @param  BaseDevicePath         Base device path.
  @param  Drive                  Legacy drive.
  @param  DevicePath             Device path for output.

**/
VOID
SetBiosInitBlockIoDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BaseDevicePath,
  IN  BIOS_LEGACY_DRIVE         *Drive,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  EFI_STATUS                  Status;
  BLOCKIO_VENDOR_DEVICE_PATH  VendorNode;

  Status = EFI_UNSUPPORTED;

  //
  // BugBug: Check for memory leaks!
  //
  if (Drive->EddVersion == EDD_VERSION_30) {
    //
    // EDD 3.0 case.
    //
    Status = BuildEdd30DevicePath (BaseDevicePath, Drive, DevicePath);
  }

  if (EFI_ERROR (Status)) {
    //
    // EDD 1.1 device case or it is unrecognized EDD 3.0 device
    //
    ZeroMem (&VendorNode, sizeof (VendorNode));
    VendorNode.DevicePath.Header.Type    = HARDWARE_DEVICE_PATH;
    VendorNode.DevicePath.Header.SubType = HW_VENDOR_DP;
    SetDevicePathNodeLength (&VendorNode.DevicePath.Header, sizeof (VendorNode));
    CopyMem (&VendorNode.DevicePath.Guid, &gBlockIoVendorGuid, sizeof (EFI_GUID));
    VendorNode.LegacyDriveLetter = Drive->Number;
    *DevicePath                  = AppendDevicePathNode (BaseDevicePath, &VendorNode.DevicePath.Header);
  }
}

/**
  Build device path for EDD 3.0.

  @param  BaseDevicePath         Base device path.
  @param  Drive                  Legacy drive.
  @param  DevicePath             Device path for output.

  @retval EFI_SUCCESS            The device path is built successfully.
  @retval EFI_UNSUPPORTED        It is failed to built device path.

**/
EFI_STATUS
BuildEdd30DevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BaseDevicePath,
  IN  BIOS_LEGACY_DRIVE         *Drive,
  IN  EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  //
  // AVL    UINT64                  Address;
  // AVL    EFI_HANDLE              Handle;
  //
  EFI_DEV_PATH  Node;
  UINT32        Controller;

  Controller = (UINT32)Drive->Parameters.InterfacePath.Pci.Controller;

  ZeroMem (&Node, sizeof (Node));
  if ((AsciiStrnCmp ("ATAPI", Drive->Parameters.InterfaceType, 5) == 0) ||
      (AsciiStrnCmp ("ATA", Drive->Parameters.InterfaceType, 3) == 0)
      )
  {
    //
    // ATA or ATAPI drive found
    //
    Node.Atapi.Header.Type    = MESSAGING_DEVICE_PATH;
    Node.Atapi.Header.SubType = MSG_ATAPI_DP;
    SetDevicePathNodeLength (&Node.Atapi.Header, sizeof (ATAPI_DEVICE_PATH));
    Node.Atapi.SlaveMaster      = Drive->Parameters.DevicePath.Atapi.Master;
    Node.Atapi.Lun              = Drive->Parameters.DevicePath.Atapi.Lun;
    Node.Atapi.PrimarySecondary = (UINT8)Controller;
  } else {
    //
    // Not an ATA/ATAPI drive
    //
    if (Controller != 0) {
      ZeroMem (&Node, sizeof (Node));
      Node.Controller.Header.Type    = HARDWARE_DEVICE_PATH;
      Node.Controller.Header.SubType = HW_CONTROLLER_DP;
      SetDevicePathNodeLength (&Node.Controller.Header, sizeof (CONTROLLER_DEVICE_PATH));
      Node.Controller.ControllerNumber = Controller;
      *DevicePath                      = AppendDevicePathNode (*DevicePath, &Node.DevPath);
    }

    ZeroMem (&Node, sizeof (Node));

    if (AsciiStrnCmp ("SCSI", Drive->Parameters.InterfaceType, 4) == 0) {
      //
      // SCSI drive
      //
      Node.Scsi.Header.Type    = MESSAGING_DEVICE_PATH;
      Node.Scsi.Header.SubType = MSG_SCSI_DP;
      SetDevicePathNodeLength (&Node.Scsi.Header, sizeof (SCSI_DEVICE_PATH));

      //
      // Lun is miss aligned in both EDD and Device Path data structures.
      //  thus we do a byte copy, to prevent alignment traps on IA-64.
      //
      CopyMem (&Node.Scsi.Lun, &Drive->Parameters.DevicePath.Scsi.Lun, sizeof (UINT16));
      Node.Scsi.Pun = Drive->Parameters.DevicePath.Scsi.Pun;
    } else if (AsciiStrnCmp ("USB", Drive->Parameters.InterfaceType, 3) == 0) {
      //
      // USB drive
      //
      Node.Usb.Header.Type    = MESSAGING_DEVICE_PATH;
      Node.Usb.Header.SubType = MSG_USB_DP;
      SetDevicePathNodeLength (&Node.Usb.Header, sizeof (USB_DEVICE_PATH));
      Node.Usb.ParentPortNumber = (UINT8)Drive->Parameters.DevicePath.Usb.Reserved;
    } else if (AsciiStrnCmp ("1394", Drive->Parameters.InterfaceType, 4) == 0) {
      //
      // 1394 drive
      //
      Node.F1394.Header.Type    = MESSAGING_DEVICE_PATH;
      Node.F1394.Header.SubType = MSG_1394_DP;
      SetDevicePathNodeLength (&Node.F1394.Header, sizeof (F1394_DEVICE_PATH));
      Node.F1394.Guid = Drive->Parameters.DevicePath.FireWire.Guid;
    } else if (AsciiStrnCmp ("FIBRE", Drive->Parameters.InterfaceType, 5) == 0) {
      //
      // Fibre drive
      //
      Node.FibreChannel.Header.Type    = MESSAGING_DEVICE_PATH;
      Node.FibreChannel.Header.SubType = MSG_FIBRECHANNEL_DP;
      SetDevicePathNodeLength (&Node.FibreChannel.Header, sizeof (FIBRECHANNEL_DEVICE_PATH));
      Node.FibreChannel.WWN = Drive->Parameters.DevicePath.FibreChannel.Wwn;
      Node.FibreChannel.Lun = Drive->Parameters.DevicePath.FibreChannel.Lun;
    } else {
      DEBUG (
        (
         DEBUG_BLKIO, "It is unrecognized EDD 3.0 device, Drive Number = %x, InterfaceType = %s\n",
         Drive->Number,
         Drive->Parameters.InterfaceType
        )
        );
    }
  }

  if (Node.DevPath.Type == 0) {
    return EFI_UNSUPPORTED;
  }

  *DevicePath = AppendDevicePathNode (BaseDevicePath, &Node.DevPath);
  return EFI_SUCCESS;
}
