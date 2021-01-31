/** @file
  Partition driver that produces logical BlockIo devices from a physical
  BlockIo device. The logical BlockIo devices are based on the format
  of the raw block devices media. Currently "El Torito CD-ROM", UDF, Legacy
  MBR, and GPT partition schemes are supported.

Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "Partition.h"

//
// Partition Driver Global Variables.
//
EFI_DRIVER_BINDING_PROTOCOL gPartitionDriverBinding = {
  PartitionDriverBindingSupported,
  PartitionDriverBindingStart,
  PartitionDriverBindingStop,
  //
  // Grub4Dos copies the BPB of the first partition to the MBR. If the
  // DriverBindingStart() of the Fat driver gets run before that of Partition
  // driver only the first partition can be recognized.
  // Let the driver binding version of Partition driver be higher than that of
  // Fat driver to make sure the DriverBindingStart() of the Partition driver
  // gets run before that of Fat driver so that all the partitions can be recognized.
  //
  0xb,
  NULL,
  NULL
};

//
// Prioritized function list to detect partition table.
// Refer to UEFI Spec 13.3.2 Partition Discovery, the block device
// should be scanned in below order:
// 1. GPT
// 2. ISO 9660 (El Torito) (or UDF)
// 3. MBR
// 4. no partiton found
// Note: UDF is using a same method as booting from CD-ROM, so put it along
//        with CD-ROM check.
//
PARTITION_DETECT_ROUTINE mPartitionDetectRoutineTable[] = {
  PartitionInstallGptChildHandles,
  PartitionInstallAppleChildHandles,
  PartitionInstallUdfChildHandles,
  PartitionInstallMbrChildHandles,
  NULL
};

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a BlockIo and DiskIo protocol or a BlockIo2 protocol can be
  supported.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
PartitionDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DISK_IO_PROTOCOL      *DiskIo;
  EFI_DEV_PATH              *Node;

  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      Node = (EFI_DEV_PATH *) RemainingDevicePath;
      if (Node->DevPath.Type != MEDIA_DEVICE_PATH ||
        Node->DevPath.SubType != MEDIA_HARDDRIVE_DP ||
        DevicePathNodeLength (&Node->DevPath) != sizeof (HARDDRIVE_DEVICE_PATH)) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDiskIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}

/**
  Start this driver on ControllerHandle by opening a Block IO or a Block IO2
  or both, and Disk IO protocol, reading Device Path, and creating a child
  handle with a Disk IO and device path protocol.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
PartitionDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                OpenStatus;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL    *BlockIo2;
  EFI_DISK_IO_PROTOCOL      *DiskIo;
  EFI_DISK_IO2_PROTOCOL     *DiskIo2;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  PARTITION_DETECT_ROUTINE  *Routine;
  BOOLEAN                   MediaPresent;
  EFI_TPL                   OldTpl;

  BlockIo2 = NULL;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, return EFI_SUCCESS
    //
    if (IsDevicePathEnd (RemainingDevicePath)) {
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  //
  // Try to open BlockIO and BlockIO2. If BlockIO would be opened, continue,
  // otherwise, return error.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIo2ProtocolGuid,
                  (VOID **) &BlockIo2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    BlockIo2 = NULL;
  }

  //
  // Get the Device Path Protocol on ControllerHandle's handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    goto Exit;
  }

  //
  // Get the DiskIo and DiskIo2.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    goto Exit;
  }

  OpenStatus = Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIo2ProtocolGuid,
                  (VOID **) &DiskIo2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    DiskIo2 = NULL;
  }

  //
  // Try to read blocks when there's media or it is removable physical partition.
  //
  Status       = EFI_UNSUPPORTED;
  MediaPresent = BlockIo->Media->MediaPresent;
  if (BlockIo->Media->MediaPresent ||
      (BlockIo->Media->RemovableMedia && !BlockIo->Media->LogicalPartition)) {
    //
    // Try for GPT, then legacy MBR partition types, and then UDF and El Torito.
    // If the media supports a given partition type install child handles to
    // represent the partitions described by the media.
    //
    Routine = &mPartitionDetectRoutineTable[0];
    while (*Routine != NULL) {
      Status = (*Routine) (
                   This,
                   ControllerHandle,
                   DiskIo,
                   DiskIo2,
                   BlockIo,
                   BlockIo2,
                   ParentDevicePath
                   );
      if (!EFI_ERROR (Status) || Status == EFI_MEDIA_CHANGED || Status == EFI_NO_MEDIA) {
        break;
      }
      Routine++;
    }
  }
  //
  // In the case that the driver is already started (OpenStatus == EFI_ALREADY_STARTED),
  // the DevicePathProtocol and the DiskIoProtocol are not actually opened by the
  // driver. So don't try to close them. Otherwise, we will break the dependency
  // between the controller and the driver set up before.
  //
  // In the case that when the media changes on a device it will Reinstall the
  // BlockIo interaface. This will cause a call to our Stop(), and a subsequent
  // reentrant call to our Start() successfully. We should leave the device open
  // when this happen. The "media change" case includes either the status is
  // EFI_MEDIA_CHANGED or it is a "media" to "no media" change.
  //
  if (EFI_ERROR (Status)          &&
      !EFI_ERROR (OpenStatus)     &&
      Status != EFI_MEDIA_CHANGED &&
      !(MediaPresent && Status == EFI_NO_MEDIA)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDiskIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    //
    // Close Parent DiskIo2 if has.
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIo2ProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
PartitionDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL  *BlockIo2;
  BOOLEAN                 AllChildrenStopped;
  PARTITION_PRIVATE_DATA  *Private;
  EFI_DISK_IO_PROTOCOL    *DiskIo;
  EFI_GUID                *TypeGuid;

  BlockIo  = NULL;
  BlockIo2 = NULL;
  Private = NULL;

  if (NumberOfChildren == 0) {
    //
    // In the case of re-entry of the PartitionDriverBindingStop, the
    // NumberOfChildren may not reflect the actual number of children on the
    // bus driver. Hence, additional check is needed here.
    //
    if (HasChildren (ControllerHandle)) {
      DEBUG((EFI_D_ERROR, "PartitionDriverBindingStop: Still has child.\n"));
      return EFI_DEVICE_ERROR;
    }

    //
    // Close the bus driver
    //
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDiskIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    //
    // Close Parent BlockIO2 if has.
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIo2ProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;
  for (Index = 0; Index < NumberOfChildren; Index++) {
    gBS->OpenProtocol (
           ChildHandleBuffer[Index],
           &gEfiBlockIoProtocolGuid,
           (VOID **) &BlockIo,
           This->DriverBindingHandle,
           ControllerHandle,
           EFI_OPEN_PROTOCOL_GET_PROTOCOL
           );
    //
    // Try to locate BlockIo2.
    //
    gBS->OpenProtocol (
           ChildHandleBuffer[Index],
           &gEfiBlockIo2ProtocolGuid,
           (VOID **) &BlockIo2,
           This->DriverBindingHandle,
           ControllerHandle,
           EFI_OPEN_PROTOCOL_GET_PROTOCOL
           );


    Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (BlockIo);
    if (Private->InStop) {
      //
      // If the child handle is going to be stopped again during the re-entry
      // of DriverBindingStop, just do nothing.
      //
      break;
    }
    Private->InStop = TRUE;

    BlockIo->FlushBlocks (BlockIo);

    if (BlockIo2 != NULL) {
      Status = BlockIo2->FlushBlocksEx (BlockIo2, NULL);
      DEBUG((EFI_D_ERROR, "PartitionDriverBindingStop: FlushBlocksEx returned with %r\n", Status));
    } else {
      Status = EFI_SUCCESS;
    }

    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIoProtocolGuid,
           This->DriverBindingHandle,
           ChildHandleBuffer[Index]
           );

    if (IsZeroGuid (&Private->TypeGuid)) {
      TypeGuid = NULL;
    } else {
      TypeGuid = &Private->TypeGuid;
    }

    //
    // All Software protocols have be freed from the handle so remove it.
    // Remove the BlockIo Protocol if has.
    // Remove the BlockIo2 Protocol if has.
    //
    if (BlockIo2 != NULL) {
      //
      // Some device drivers might re-install the BlockIO(2) protocols for a
      // media change condition. Therefore, if the FlushBlocksEx returned with
      // EFI_MEDIA_CHANGED, just let the BindingStop fail to avoid potential
      // reference of already stopped child handle.
      //
      if (Status != EFI_MEDIA_CHANGED) {
        Status = gBS->UninstallMultipleProtocolInterfaces (
                         ChildHandleBuffer[Index],
                         &gEfiDevicePathProtocolGuid,
                         Private->DevicePath,
                         &gEfiBlockIoProtocolGuid,
                         &Private->BlockIo,
                         &gEfiBlockIo2ProtocolGuid,
                         &Private->BlockIo2,
                         &gEfiPartitionInfoProtocolGuid,
                         &Private->PartitionInfo,
                         &gApplePartitionInfoProtocolGuid,
                         &Private->ApplePartitionInfo,
                         TypeGuid,
                         NULL,
                         NULL
                         );
      }
    } else {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                       ChildHandleBuffer[Index],
                       &gEfiDevicePathProtocolGuid,
                       Private->DevicePath,
                       &gEfiBlockIoProtocolGuid,
                       &Private->BlockIo,
                       &gEfiPartitionInfoProtocolGuid,
                       &Private->PartitionInfo,
                       &gApplePartitionInfoProtocolGuid,
                       &Private->ApplePartitionInfo,
                       TypeGuid,
                       NULL,
                       NULL
                       );
    }

    if (EFI_ERROR (Status)) {
      Private->InStop = FALSE;
      gBS->OpenProtocol (
             ControllerHandle,
             &gEfiDiskIoProtocolGuid,
             (VOID **) &DiskIo,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index],
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    } else {
      FreePool (Private->DevicePath);
      FreePool (Private);
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      if (Status == EFI_MEDIA_CHANGED) {
        break;
      }
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Reset the Block Device.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
PartitionReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  return Private->ParentBlockIo->Reset (
                                  Private->ParentBlockIo,
                                  ExtendedVerification
                                  );
}

/**
  Probe the media status and return EFI_NO_MEDIA or EFI_MEDIA_CHANGED
  for no media or media change case. Otherwise DefaultStatus is returned.

  @param DiskIo             Pointer to the DiskIo instance.
  @param MediaId            Id of the media, changes every time the media is replaced.
  @param DefaultStatus      The default status to return when it's not the no media
                            or media change case.

  @retval EFI_NO_MEDIA      There is no media.
  @retval EFI_MEDIA_CHANGED The media was changed.
  @retval others            The default status to return.
**/
EFI_STATUS
ProbeMediaStatus (
  IN EFI_DISK_IO_PROTOCOL    *DiskIo,
  IN UINT32                  MediaId,
  IN EFI_STATUS              DefaultStatus
  )
{
  EFI_STATUS                 Status;
  UINT8                      Buffer[1];

  //
  // Read 1 byte from offset 0 to check if the MediaId is still valid.
  // The reading operation is synchronious thus it is not worth it to
  // allocate a buffer from the pool. The destination buffer for the
  // data is in the stack.
  //
  Status = DiskIo->ReadDisk (DiskIo, MediaId, 0, 1, (VOID*)Buffer);
  if ((Status == EFI_NO_MEDIA) || (Status == EFI_MEDIA_CHANGED)) {
    return Status;
  }
  return DefaultStatus;
}

/**
  Read by using the Disk IO protocol on the parent device. Lba addresses
  must be converted to byte offsets.

  @param  This       Protocol instance pointer.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
PartitionReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
{
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_INVALID_PARAMETER);
  }
  //
  // Because some kinds of partition have different block size from their parent
  // device, we call the Disk IO protocol on the parent device, not the Block IO
  // protocol
  //
  return Private->DiskIo->ReadDisk (Private->DiskIo, MediaId, Offset, BufferSize, Buffer);
}

/**
  Write by using the Disk IO protocol on the parent device. Lba addresses
  must be converted to byte offsets.

  @param[in]  This       Protocol instance pointer.
  @param[in]  MediaId    Id of the media, changes every time the media is replaced.
  @param[in]  Lba        The starting Logical Block Address to read from
  @param[in]  BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]  Buffer     Buffer containing data to be written to device.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains a LBA that is not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
PartitionWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                  *Buffer
  )
{
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_INVALID_PARAMETER);
  }
  //
  // Because some kinds of partition have different block size from their parent
  // device, we call the Disk IO protocol on the parent device, not the Block IO
  // protocol
  //
  return Private->DiskIo->WriteDisk (Private->DiskIo, MediaId, Offset, BufferSize, Buffer);
}


/**
  Flush the parent Block Device.

  @param  This              Protocol instance pointer.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
PartitionFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  return Private->ParentBlockIo->FlushBlocks (Private->ParentBlockIo);
}

/**
  Probe the media status and return EFI_NO_MEDIA or EFI_MEDIA_CHANGED
  for no media or media change case. Otherwise DefaultStatus is returned.

  @param DiskIo2            Pointer to the DiskIo2 instance.
  @param MediaId            Id of the media, changes every time the media is replaced.
  @param DefaultStatus      The default status to return when it's not the no media
                            or media change case.

  @retval EFI_NO_MEDIA      There is no media.
  @retval EFI_MEDIA_CHANGED The media was changed.
  @retval others            The default status to return.
**/
EFI_STATUS
ProbeMediaStatusEx (
  IN EFI_DISK_IO2_PROTOCOL   *DiskIo2,
  IN UINT32                  MediaId,
  IN EFI_STATUS              DefaultStatus
  )
{
  EFI_STATUS                 Status;
  UINT8                      Buffer[1];

  //
  // Read 1 byte from offset 0 to check if the MediaId is still valid.
  // The reading operation is synchronious thus it is not worth it to
  // allocate a buffer from the pool. The destination buffer for the
  // data is in the stack.
  //
  Status = DiskIo2->ReadDiskEx (DiskIo2, MediaId, 0, NULL, 1, (VOID*)Buffer);
  if ((Status == EFI_NO_MEDIA) || (Status == EFI_MEDIA_CHANGED)) {
    return Status;
  }
  return DefaultStatus;
}

/**
  Reset the Block Device throught Block I/O2 protocol.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
PartitionResetEx (
  IN EFI_BLOCK_IO2_PROTOCOL *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  return Private->ParentBlockIo2->Reset (
                                    Private->ParentBlockIo2,
                                    ExtendedVerification
                                    );
}

/**
  The general callback for the DiskIo2 interfaces.
  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which points to the PARTITION_ACCESS_TASK instance.
**/
VOID
EFIAPI
PartitionOnAccessComplete (
  IN EFI_EVENT                 Event,
  IN VOID                      *Context
  )
{
  PARTITION_ACCESS_TASK   *Task;

  Task = (PARTITION_ACCESS_TASK *) Context;

  gBS->CloseEvent (Event);

  Task->BlockIo2Token->TransactionStatus = Task->DiskIo2Token.TransactionStatus;
  gBS->SignalEvent (Task->BlockIo2Token->Event);

  FreePool (Task);
}

/**
  Create a new PARTITION_ACCESS_TASK instance.

  @param  Token  Pointer to the EFI_BLOCK_IO2_TOKEN.

  @return Pointer to the created PARTITION_ACCESS_TASK instance or NULL upon failure.
**/
PARTITION_ACCESS_TASK *
PartitionCreateAccessTask (
  IN EFI_BLOCK_IO2_TOKEN    *Token
  )
{
  EFI_STATUS                Status;
  PARTITION_ACCESS_TASK     *Task;

  Task = AllocatePool (sizeof (*Task));
  if (Task == NULL) {
    return NULL;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PartitionOnAccessComplete,
                  Task,
                  &Task->DiskIo2Token.Event
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Task);
    return NULL;
  }

  Task->BlockIo2Token = Token;

  return Task;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  This function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned.
  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_or EFI_MEDIA_CHANGED is returned and
  non-blocking I/O is being used, the Event associated with this request will
  not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is
                              replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination buffer for the data. The
                              caller is responsible for either having implicit or
                              explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Token->Event is
                                not NULL.The data was read correctly from the
                                device if the Token->Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.
**/
EFI_STATUS
EFIAPI
PartitionReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  OUT    VOID                   *Buffer
  )
{
  EFI_STATUS              Status;
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;
  PARTITION_ACCESS_TASK   *Task;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatusEx (Private->DiskIo2, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatusEx (Private->DiskIo2, MediaId, EFI_INVALID_PARAMETER);
  }

  if ((Token != NULL) && (Token->Event != NULL)) {
    Task = PartitionCreateAccessTask (Token);
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = Private->DiskIo2->ReadDiskEx (Private->DiskIo2, MediaId, Offset, &Task->DiskIo2Token, BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Task->DiskIo2Token.Event);
      FreePool (Task);
    }
  } else {
    Status = Private->DiskIo2->ReadDiskEx (Private->DiskIo2, MediaId, Offset, NULL, BufferSize, Buffer);
  }

  return Status;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  This function writes the requested number of blocks to the device. All blocks
  are written, or an error is returned.If EFI_DEVICE_ERROR, EFI_NO_MEDIA,
  EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED is returned and non-blocking I/O is
  being used, the Event associated with this request will not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The write request was queued if Event is not NULL.
                                The data was written correctly to the device if
                                the Event is NULL.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
EFIAPI
PartitionWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  )
{
  EFI_STATUS              Status;
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;
  PARTITION_ACCESS_TASK   *Task;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatusEx (Private->DiskIo2, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatusEx (Private->DiskIo2, MediaId, EFI_INVALID_PARAMETER);
  }

  if ((Token != NULL) && (Token->Event != NULL)) {
    Task = PartitionCreateAccessTask (Token);
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status =  Private->DiskIo2->WriteDiskEx (Private->DiskIo2, MediaId, Offset, &Task->DiskIo2Token, BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Task->DiskIo2Token.Event);
      FreePool (Task);
    }
  } else {
    Status = Private->DiskIo2->WriteDiskEx (Private->DiskIo2, MediaId, Offset, NULL, BufferSize, Buffer);
  }
  return Status;
}

/**
  Flush the Block Device.

  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED
  is returned and non-blocking I/O is being used, the Event associated with
  this request will not be signaled.

  @param[in]      This     Indicates a pointer to the calling context.
  @param[in, out] Token    A pointer to the token associated with the transaction

  @retval EFI_SUCCESS          The flush request was queued if Event is not NULL.
                               All outstanding data was written correctly to the
                               device if the Event is NULL.
  @retval EFI_DEVICE_ERROR     The device reported an error while writting back
                               the data.
  @retval EFI_WRITE_PROTECTED  The device cannot be written to.
  @retval EFI_NO_MEDIA         There is no media in the device.
  @retval EFI_MEDIA_CHANGED    The MediaId is not for the current media.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack
                               of resources.

**/
EFI_STATUS
EFIAPI
PartitionFlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token
  )
{
  EFI_STATUS              Status;
  PARTITION_PRIVATE_DATA  *Private;
  PARTITION_ACCESS_TASK   *Task;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  if ((Token != NULL) && (Token->Event != NULL)) {
    Task = PartitionCreateAccessTask (Token);
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = Private->DiskIo2->FlushDiskEx (Private->DiskIo2, &Task->DiskIo2Token);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Task->DiskIo2Token.Event);
      FreePool (Task);
    }
  } else {
    Status = Private->DiskIo2->FlushDiskEx (Private->DiskIo2, NULL);
  }
  return Status;
}


/**
  Create a child handle for a logical block device that represents the
  bytes Start to End of the Parent Block IO device.

  @param[in]  This              Protocol instance pointer.
  @param[in]  ParentHandle      Parent Handle for new child.
  @param[in]  ParentDiskIo      Parent DiskIo interface.
  @param[in]  ParentDiskIo2     Parent DiskIo2 interface.
  @param[in]  ParentBlockIo     Parent BlockIo interface.
  @param[in]  ParentBlockIo2    Parent BlockIo2 interface.
  @param[in]  ParentDevicePath  Parent Device Path.
  @param[in]  DevicePathNode    Child Device Path node.
  @param[in]  PartitionInfo     Child Partition Information interface.
  @param[in]  ApplePartitionInfo Child Apple Partition Information interface.
  @param[in]  Start             Start Block.
  @param[in]  End               End Block.
  @param[in]  BlockSize         Child block size.
  @param[in]  TypeGuid          Partition GUID Type.

  @retval EFI_SUCCESS       A child handle was added.
  @retval other             A child handle was not added.

**/
EFI_STATUS
PartitionInstallChildHandle (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ParentHandle,
  IN  EFI_DISK_IO_PROTOCOL         *ParentDiskIo,
  IN  EFI_DISK_IO2_PROTOCOL        *ParentDiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL        *ParentBlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *ParentBlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePathNode,
  IN  EFI_PARTITION_INFO_PROTOCOL  *PartitionInfo,
  IN  APPLE_PARTITION_INFO_PROTOCOL *ApplePartitionInfo,
  IN  EFI_LBA                      Start,
  IN  EFI_LBA                      End,
  IN  UINT32                       BlockSize,
  IN  EFI_GUID                     *TypeGuid
  )
{
  EFI_STATUS              Status;
  PARTITION_PRIVATE_DATA  *Private;

  Status  = EFI_SUCCESS;
  Private = AllocateZeroPool (sizeof (PARTITION_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature        = PARTITION_PRIVATE_DATA_SIGNATURE;

  Private->Start            = MultU64x32 (Start, ParentBlockIo->Media->BlockSize);
  Private->End              = MultU64x32 (End + 1, ParentBlockIo->Media->BlockSize);

  Private->BlockSize        = BlockSize;
  Private->ParentBlockIo    = ParentBlockIo;
  Private->ParentBlockIo2   = ParentBlockIo2;
  Private->DiskIo           = ParentDiskIo;
  Private->DiskIo2          = ParentDiskIo2;

  //
  // Set the BlockIO into Private Data.
  //
  Private->BlockIo.Revision = ParentBlockIo->Revision;

  Private->BlockIo.Media    = &Private->Media;
  CopyMem (Private->BlockIo.Media, ParentBlockIo->Media, sizeof (EFI_BLOCK_IO_MEDIA));

  Private->BlockIo.Reset        = PartitionReset;
  Private->BlockIo.ReadBlocks   = PartitionReadBlocks;
  Private->BlockIo.WriteBlocks  = PartitionWriteBlocks;
  Private->BlockIo.FlushBlocks  = PartitionFlushBlocks;

  //
  // Set the BlockIO2 into Private Data.
  //
  if (Private->DiskIo2 != NULL) {
    ASSERT (Private->ParentBlockIo2 != NULL);
    Private->BlockIo2.Media    = &Private->Media2;
    CopyMem (Private->BlockIo2.Media, ParentBlockIo2->Media, sizeof (EFI_BLOCK_IO_MEDIA));

    Private->BlockIo2.Reset          = PartitionResetEx;
    Private->BlockIo2.ReadBlocksEx   = PartitionReadBlocksEx;
    Private->BlockIo2.WriteBlocksEx  = PartitionWriteBlocksEx;
    Private->BlockIo2.FlushBlocksEx  = PartitionFlushBlocksEx;
  }

  Private->Media.IoAlign   = 0;
  Private->Media.LogicalPartition = TRUE;
  Private->Media.LastBlock = DivU64x32 (
                               MultU64x32 (
                                 End - Start + 1,
                                 ParentBlockIo->Media->BlockSize
                                 ),
                                BlockSize
                               ) - 1;

  Private->Media.BlockSize = (UINT32) BlockSize;

  Private->Media2.IoAlign   = 0;
  Private->Media2.LogicalPartition = TRUE;
  Private->Media2.LastBlock = Private->Media.LastBlock;
  Private->Media2.BlockSize = (UINT32) BlockSize;

  //
  // Per UEFI Spec, LowestAlignedLba, LogicalBlocksPerPhysicalBlock and OptimalTransferLengthGranularity must be 0
  //  for logical partitions.
  //
  if (Private->BlockIo.Revision >= EFI_BLOCK_IO_PROTOCOL_REVISION2) {
    Private->Media.LowestAlignedLba               = 0;
    Private->Media.LogicalBlocksPerPhysicalBlock  = 0;
    Private->Media2.LowestAlignedLba              = 0;
    Private->Media2.LogicalBlocksPerPhysicalBlock = 0;
    if (Private->BlockIo.Revision >= EFI_BLOCK_IO_PROTOCOL_REVISION3) {
      Private->Media.OptimalTransferLengthGranularity  = 0;
      Private->Media2.OptimalTransferLengthGranularity = 0;
    }
  }

  Private->DevicePath     = AppendDevicePathNode (ParentDevicePath, DevicePathNode);

  if (Private->DevicePath == NULL) {
    FreePool (Private);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the PartitionInfo into Private Data.
  //
  CopyMem (&Private->PartitionInfo, PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
  CopyMem (&Private->ApplePartitionInfo, ApplePartitionInfo, sizeof (APPLE_PARTITION_INFO_PROTOCOL));

  if (TypeGuid != NULL) {
    CopyGuid(&(Private->TypeGuid), TypeGuid);
  } else {
    ZeroMem ((VOID *)&(Private->TypeGuid), sizeof (EFI_GUID));
  }

  //
  // Create the new handle.
  //
  Private->Handle = NULL;
  if (Private->DiskIo2 != NULL) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Private->Handle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Private->BlockIo,
                    &gEfiBlockIo2ProtocolGuid,
                    &Private->BlockIo2,
                    &gEfiPartitionInfoProtocolGuid,
                    &Private->PartitionInfo,
                    &gApplePartitionInfoProtocolGuid,
                    &Private->ApplePartitionInfo,
                    TypeGuid,
                    NULL,
                    NULL
                    );
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Private->Handle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Private->BlockIo,
                    &gEfiPartitionInfoProtocolGuid,
                    &Private->PartitionInfo,
                    &gApplePartitionInfoProtocolGuid,
                    &Private->ApplePartitionInfo,
                    TypeGuid,
                    NULL,
                    NULL
                    );
  }

  if (!EFI_ERROR (Status)) {
    //
    // Open the Parent Handle for the child
    //
    Status = gBS->OpenProtocol (
                    ParentHandle,
                    &gEfiDiskIoProtocolGuid,
                    (VOID **) &ParentDiskIo,
                    This->DriverBindingHandle,
                    Private->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  } else {
    FreePool (Private->DevicePath);
    FreePool (Private);

    //
    // if the Status == EFI_ALREADY_STARTED, it means the child handles
    // are already installed. So return EFI_SUCCESS to avoid do the next
    // partition type check.
    //
    if (Status == EFI_ALREADY_STARTED) {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}


/**
  The user Entry Point for module Partition. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePartition (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gPartitionDriverBinding,
             ImageHandle,
             &gPartitionComponentName,
             &gPartitionComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}


/**
  Test to see if there is any child on ControllerHandle.

  @param[in]  ControllerHandle    Handle of device to test.

  @retval TRUE                    There are children on the ControllerHandle.
  @retval FALSE                   No child is on the ControllerHandle.

**/
BOOLEAN
HasChildren (
  IN EFI_HANDLE           ControllerHandle
  )
{
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  EFI_STATUS                           Status;
  UINTN                                Index;

  Status = gBS->OpenProtocolInformation (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      break;
    }
  }
  FreePool (OpenInfoBuffer);

  return (BOOLEAN) (Index < EntryCount);
}
