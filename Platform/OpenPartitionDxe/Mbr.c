/** @file
  Decode a hard disk partitioned with the legacy MBR found on most PC's

  MBR - Master Boot Record is in the first sector of a partitioned hard disk.
        The MBR supports four partitions per disk. The MBR also contains legacy
        code that is not run on an EFI system. The legacy code reads the
        first sector of the active partition into memory and

  BPB - BIOS Parameter Block is in the first sector of a FAT file system.
        The BPB contains information about the FAT file system. The BPB is
        always on the first sector of a media. The first sector also contains
        the legacy boot strap code.

Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Partition.h"

/**
  Test to see if the Mbr buffer is a valid MBR.

  @param  Mbr         Parent Handle.
  @param  LastLba     Last Lba address on the device.

  @retval TRUE        Mbr is a Valid MBR.
  @retval FALSE       Mbr is not a Valid MBR.

**/
BOOLEAN
PartitionValidMbr (
  IN  MASTER_BOOT_RECORD      *Mbr,
  IN  EFI_LBA                 LastLba
  )
{
  UINT32  StartingLBA;
  UINT32  EndingLBA;
  UINT32  NewEndingLBA;
  INTN    Index1;
  INTN    Index2;
  BOOLEAN MbrValid;

  if (Mbr->Signature != MBR_SIGNATURE) {
    return FALSE;
  }
  //
  // The BPB also has this signature, so it can not be used alone.
  //
  MbrValid = FALSE;
  for (Index1 = 0; Index1 < MAX_MBR_PARTITIONS; Index1++) {
    if (Mbr->Partition[Index1].OSIndicator == 0x00 || UNPACK_UINT32 (Mbr->Partition[Index1].SizeInLBA) == 0) {
      continue;
    }

    MbrValid    = TRUE;
    StartingLBA = UNPACK_UINT32 (Mbr->Partition[Index1].StartingLBA);
    EndingLBA   = StartingLBA + UNPACK_UINT32 (Mbr->Partition[Index1].SizeInLBA) - 1;
    if (EndingLBA > LastLba) {
      //
      // Compatibility Errata:
      //  Some systems try to hide drive space with their INT 13h driver
      //  This does not hide space from the OS driver. This means the MBR
      //  that gets created from DOS is smaller than the MBR created from
      //  a real OS (NT & Win98). This leads to BlockIo->LastBlock being
      //  wrong on some systems FDISKed by the OS.
      //
      // return FALSE since no block devices on a system are implemented
      // with INT 13h
      //

      DEBUG((EFI_D_INFO, "PartitionValidMbr: Bad MBR partition size EndingLBA(%1x) > LastLBA(%1x)\n", EndingLBA, LastLba));

      return FALSE;
    }

    for (Index2 = Index1 + 1; Index2 < MAX_MBR_PARTITIONS; Index2++) {
      if (Mbr->Partition[Index2].OSIndicator == 0x00 || UNPACK_UINT32 (Mbr->Partition[Index2].SizeInLBA) == 0) {
        continue;
      }

      NewEndingLBA = UNPACK_UINT32 (Mbr->Partition[Index2].StartingLBA) + UNPACK_UINT32 (Mbr->Partition[Index2].SizeInLBA) - 1;
      if (NewEndingLBA >= StartingLBA && UNPACK_UINT32 (Mbr->Partition[Index2].StartingLBA) <= EndingLBA) {
        //
        // This region overlaps with the Index1'th region
        //
        return FALSE;
      }
    }
  }
  //
  // None of the regions overlapped so MBR is O.K.
  //
  return MbrValid;
}


/**
  Install child handles if the Handle supports MBR format.

  @param[in]  This              Calling context.
  @param[in]  Handle            Parent Handle.
  @param[in]  DiskIo            Parent DiskIo interface.
  @param[in]  DiskIo2           Parent DiskIo2 interface.
  @param[in]  BlockIo           Parent BlockIo interface.
  @param[in]  BlockIo2          Parent BlockIo2 interface.
  @param[in]  DevicePath        Parent Device Path.

  @retval EFI_SUCCESS       A child handle was added.
  @retval EFI_MEDIA_CHANGED Media change was detected.
  @retval Others            MBR partition was not found.

**/
EFI_STATUS
PartitionInstallMbrChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_DISK_IO2_PROTOCOL        *DiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *BlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_STATUS                   Status;
  MASTER_BOOT_RECORD           *Mbr;
  UINT32                       ExtMbrStartingLba;
  UINT32                       Index;
  HARDDRIVE_DEVICE_PATH        HdDev;
  HARDDRIVE_DEVICE_PATH        ParentHdDev;
  EFI_STATUS                   Found;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL     *LastDevicePathNode;
  UINT32                       BlockSize;
  UINT32                       MediaId;
  EFI_LBA                      LastSector;
  EFI_PARTITION_INFO_PROTOCOL  PartitionInfo;
  APPLE_PARTITION_INFO_PROTOCOL ApplePartitionInfo;

  Found           = EFI_NOT_FOUND;

  BlockSize   = BlockIo->Media->BlockSize;
  MediaId     = BlockIo->Media->MediaId;
  LastSector  = DivU64x32 (
                  MultU64x32 (BlockIo->Media->LastBlock + 1, BlockSize),
                  MBR_SIZE
                  ) - 1;

  //
  // Ensure the block size can hold the MBR
  //
  if (BlockSize < sizeof (MASTER_BOOT_RECORD)) {
    return EFI_NOT_FOUND;
  }

  Mbr = AllocatePool (BlockSize);
  if (Mbr == NULL) {
    return Found;
  }

  Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     0,
                     BlockSize,
                     Mbr
                     );
  if (EFI_ERROR (Status)) {
    Found = Status;
    goto Done;
  }
  if (!PartitionValidMbr (Mbr, LastSector)) {
    goto Done;
  }
  //
  // We have a valid mbr - add each partition
  //
  //
  // Get starting and ending LBA of the parent block device.
  //
  LastDevicePathNode = NULL;
  ZeroMem (&ParentHdDev, sizeof (ParentHdDev));
  DevicePathNode = DevicePath;
  while (!IsDevicePathEnd (DevicePathNode)) {
    LastDevicePathNode  = DevicePathNode;
    DevicePathNode      = NextDevicePathNode (DevicePathNode);
  }

  if (LastDevicePathNode != NULL) {
    if (DevicePathType (LastDevicePathNode) == MEDIA_DEVICE_PATH &&
        DevicePathSubType (LastDevicePathNode) == MEDIA_HARDDRIVE_DP
        ) {
      CopyMem (&ParentHdDev, LastDevicePathNode, sizeof (ParentHdDev));
    } else {
      LastDevicePathNode = NULL;
    }
  }

  ZeroMem (&HdDev, sizeof (HdDev));
  HdDev.Header.Type     = MEDIA_DEVICE_PATH;
  HdDev.Header.SubType  = MEDIA_HARDDRIVE_DP;
  SetDevicePathNodeLength (&HdDev.Header, sizeof (HdDev));
  HdDev.MBRType         = MBR_TYPE_PCAT;
  HdDev.SignatureType   = SIGNATURE_TYPE_MBR;

  if (LastDevicePathNode == NULL) {
    //
    // This is a MBR, add each partition
    //
    for (Index = 0; Index < MAX_MBR_PARTITIONS; Index++) {
      if (Mbr->Partition[Index].OSIndicator == 0x00 || UNPACK_UINT32 (Mbr->Partition[Index].SizeInLBA) == 0) {
        //
        // Don't use null MBR entries
        //
        continue;
      }

      if (Mbr->Partition[Index].OSIndicator == PMBR_GPT_PARTITION) {
        //
        // This is the guard MBR for the GPT. If you ever see a GPT disk with zero partitions you can get here.
        //  We can not produce an MBR BlockIo for this device as the MBR spans the GPT headers. So formating
        //  this BlockIo would corrupt the GPT structures and require a recovery that would corrupt the format
        //  that corrupted the GPT partition.
        //
        continue;
      }

      HdDev.PartitionNumber = Index + 1;
      HdDev.PartitionStart  = UNPACK_UINT32 (Mbr->Partition[Index].StartingLBA);
      HdDev.PartitionSize   = UNPACK_UINT32 (Mbr->Partition[Index].SizeInLBA);
      CopyMem (HdDev.Signature, &(Mbr->UniqueMbrSignature[0]), sizeof (Mbr->UniqueMbrSignature));

      ZeroMem (&PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
      PartitionInfo.Revision = EFI_PARTITION_INFO_PROTOCOL_REVISION;
      PartitionInfo.Type     = PARTITION_TYPE_MBR;
      if (Mbr->Partition[Index].OSIndicator == EFI_PARTITION) {
        PartitionInfo.System = 1;
      }
      CopyMem (&PartitionInfo.Info.Mbr, &Mbr->Partition[Index], sizeof (MBR_PARTITION_RECORD));

      ZeroMem (&ApplePartitionInfo, sizeof (APPLE_PARTITION_INFO_PROTOCOL));
      ApplePartitionInfo.Revision      = APPLE_PARTITION_INFO_REVISION;
      ApplePartitionInfo.MBRType       = HdDev.MBRType;
      ApplePartitionInfo.SignatureType = HdDev.SignatureType;
      ApplePartitionInfo.PartitionNumber = HdDev.PartitionNumber;
      ApplePartitionInfo.PartitionStart  = HdDev.PartitionStart;
      ApplePartitionInfo.PartitionSize   = HdDev.PartitionSize;
      CopyMem (&ApplePartitionInfo.Signature, &(HdDev.Signature[0]), sizeof (UINT32));
      CopyMem (&ApplePartitionInfo.PartitionType, &Mbr->Partition[Index].OSIndicator, sizeof (UINT8));

      Status = PartitionInstallChildHandle (
                This,
                Handle,
                DiskIo,
                DiskIo2,
                BlockIo,
                BlockIo2,
                DevicePath,
                (EFI_DEVICE_PATH_PROTOCOL *) &HdDev,
                &PartitionInfo,
                &ApplePartitionInfo,
                HdDev.PartitionStart,
                HdDev.PartitionStart + HdDev.PartitionSize - 1,
                MBR_SIZE,
                ((Mbr->Partition[Index].OSIndicator == EFI_PARTITION) ? &gEfiPartTypeSystemPartGuid: NULL)
                );

      if (!EFI_ERROR (Status)) {
        Found = EFI_SUCCESS;
      }
    }
  } else {
    //
    // It's an extended partition. Follow the extended partition
    // chain to get all the logical drives
    //
    Index             = 0;
    ExtMbrStartingLba = 0;

    do {

      Status = DiskIo->ReadDisk (
                         DiskIo,
                         MediaId,
                         MultU64x32 (ExtMbrStartingLba, BlockSize),
                         BlockSize,
                         Mbr
                         );
      if (EFI_ERROR (Status)) {
        Found = Status;
        goto Done;
      }

      if (UNPACK_UINT32 (Mbr->Partition[0].SizeInLBA) == 0) {
        break;
      }

      if ((Mbr->Partition[0].OSIndicator == EXTENDED_DOS_PARTITION) ||
          (Mbr->Partition[0].OSIndicator == EXTENDED_WINDOWS_PARTITION)) {
        ExtMbrStartingLba = UNPACK_UINT32 (Mbr->Partition[0].StartingLBA);
        continue;
      }
      HdDev.PartitionNumber = ++Index;
      HdDev.PartitionStart  = UNPACK_UINT32 (Mbr->Partition[0].StartingLBA) + ExtMbrStartingLba + ParentHdDev.PartitionStart;
      HdDev.PartitionSize   = UNPACK_UINT32 (Mbr->Partition[0].SizeInLBA);
      if ((HdDev.PartitionStart + HdDev.PartitionSize - 1 >= ParentHdDev.PartitionStart + ParentHdDev.PartitionSize) ||
          (HdDev.PartitionStart <= ParentHdDev.PartitionStart)) {
        break;
      }

      //
      // The signature in EBR(Extended Boot Record) should always be 0.
      //
      *((UINT32 *) &HdDev.Signature[0]) = 0;

      ZeroMem (&PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
      PartitionInfo.Revision = EFI_PARTITION_INFO_PROTOCOL_REVISION;
      PartitionInfo.Type     = PARTITION_TYPE_MBR;
      if (Mbr->Partition[0].OSIndicator == EFI_PARTITION) {
        PartitionInfo.System = 1;
      }
      CopyMem (&PartitionInfo.Info.Mbr, &Mbr->Partition[0], sizeof (MBR_PARTITION_RECORD));

      ZeroMem (&ApplePartitionInfo, sizeof (APPLE_PARTITION_INFO_PROTOCOL));
      ApplePartitionInfo.Revision      = APPLE_PARTITION_INFO_REVISION;
      ApplePartitionInfo.MBRType       = HdDev.MBRType;
      ApplePartitionInfo.SignatureType = HdDev.SignatureType;
      ApplePartitionInfo.PartitionNumber = HdDev.PartitionNumber;
      ApplePartitionInfo.PartitionStart  = HdDev.PartitionStart;
      ApplePartitionInfo.PartitionSize   = HdDev.PartitionSize;
      CopyMem (&ApplePartitionInfo.Signature, &(HdDev.Signature[0]), sizeof (UINT32));
      CopyMem (&ApplePartitionInfo.PartitionType, &Mbr->Partition[0].OSIndicator, sizeof (UINT8));

      Status = PartitionInstallChildHandle (
                 This,
                 Handle,
                 DiskIo,
                 DiskIo2,
                 BlockIo,
                 BlockIo2,
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *) &HdDev,
                 &PartitionInfo,
                 &ApplePartitionInfo,
                 HdDev.PartitionStart - ParentHdDev.PartitionStart,
                 HdDev.PartitionStart - ParentHdDev.PartitionStart + HdDev.PartitionSize - 1,
                 MBR_SIZE,
                 ((Mbr->Partition[0].OSIndicator == EFI_PARTITION) ? &gEfiPartTypeSystemPartGuid: NULL)
                 );
      if (!EFI_ERROR (Status)) {
        Found = EFI_SUCCESS;
      }

      if ((Mbr->Partition[1].OSIndicator != EXTENDED_DOS_PARTITION) &&
          (Mbr->Partition[1].OSIndicator != EXTENDED_WINDOWS_PARTITION)
          ) {
        break;
      }

      ExtMbrStartingLba = UNPACK_UINT32 (Mbr->Partition[1].StartingLBA);
      //
      // Don't allow partition to be self referencing
      //
      if (ExtMbrStartingLba == 0) {
        break;
      }
    } while (ExtMbrStartingLba  < ParentHdDev.PartitionSize);
  }

Done:
  FreePool (Mbr);

  return Found;
}
