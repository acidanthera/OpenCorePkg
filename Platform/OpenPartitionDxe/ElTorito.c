/** @file
  Decode an El Torito formatted CD-ROM

Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "Partition.h"


/**
  Install child handles if the Handle supports El Torito format.

  @param[in]  This        Calling context.
  @param[in]  Handle      Parent Handle.
  @param[in]  DiskIo      Parent DiskIo interface.
  @param[in]  DiskIo2     Parent DiskIo2 interface.
  @param[in]  BlockIo     Parent BlockIo interface.
  @param[in]  BlockIo2    Parent BlockIo2 interface.
  @param[in]  DevicePath  Parent Device Path


  @retval EFI_SUCCESS         Child handle(s) was added.
  @retval EFI_MEDIA_CHANGED   Media changed Detected.
  @retval other               no child handle was added.

**/
EFI_STATUS
PartitionInstallElToritoChildHandles (
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
  UINT64                       VolDescriptorOffset;
  UINT32                       Lba2KB;
  EFI_BLOCK_IO_MEDIA           *Media;
  CDROM_VOLUME_DESCRIPTOR      *VolDescriptor;
  ELTORITO_CATALOG             *Catalog;
  UINTN                        Check;
  UINTN                        Index;
  UINTN                        BootEntry;
  UINTN                        MaxIndex;
  UINT16                       *CheckBuffer;
  CDROM_DEVICE_PATH            CdDev;
  UINT32                       SubBlockSize;
  UINT32                       SectorCount;
  EFI_STATUS                   Found;
  UINT32                       VolSpaceSize;
  EFI_PARTITION_INFO_PROTOCOL  PartitionInfo;
  APPLE_PARTITION_INFO_PROTOCOL ApplePartitionInfo;

  Found         = EFI_NOT_FOUND;
  Media         = BlockIo->Media;

  VolSpaceSize  = 0;

  //
  // CD_ROM has the fixed block size as 2048 bytes (SIZE_2KB)
  //

  // If the ISO image has been copied onto a different storage media
  // then the block size might be different (eg: USB).
  // Ensure 2048 (SIZE_2KB) is a multiple of block size
  if (((SIZE_2KB % Media->BlockSize) != 0) || (Media->BlockSize > SIZE_2KB)) {
    return EFI_NOT_FOUND;
  }

  VolDescriptor = AllocatePool ((UINTN)SIZE_2KB);

  if (VolDescriptor == NULL) {
    return EFI_NOT_FOUND;
  }

  Catalog = (ELTORITO_CATALOG *) VolDescriptor;

  //
  // Loop: handle one volume descriptor per time
  //       The ISO-9660 volume descriptor starts at 32k on the media
  //
  for (VolDescriptorOffset = SIZE_32KB;
       VolDescriptorOffset <= MultU64x32 (Media->LastBlock, Media->BlockSize);
       VolDescriptorOffset += SIZE_2KB) {
    Status = DiskIo->ReadDisk (
                       DiskIo,
                       Media->MediaId,
                       VolDescriptorOffset,
                       SIZE_2KB,
                       VolDescriptor
                       );
    if (EFI_ERROR (Status)) {
      Found = Status;
      break;
    }
    //
    // Check for valid volume descriptor signature
    //
    if (VolDescriptor->Unknown.Type == CDVOL_TYPE_END ||
        CompareMem (VolDescriptor->Unknown.Id, CDVOL_ID, sizeof (VolDescriptor->Unknown.Id)) != 0
        ) {
      //
      // end of Volume descriptor list
      //
      break;
    }
    //
    // Read the Volume Space Size from Primary Volume Descriptor 81-88 byte,
    // the 32-bit numerical values is stored in Both-byte orders
    //
    if (VolDescriptor->PrimaryVolume.Type == CDVOL_TYPE_CODED) {
      VolSpaceSize = VolDescriptor->PrimaryVolume.VolSpaceSize[0];
    }
    //
    // Is it an El Torito volume descriptor?
    //
    if (CompareMem (VolDescriptor->BootRecordVolume.SystemId, CDVOL_ELTORITO_ID, sizeof (CDVOL_ELTORITO_ID) - 1) != 0) {
      continue;
    }
    //
    // Read in the boot El Torito boot catalog
    // The LBA unit used by El Torito boot catalog is 2KB unit
    //
    Lba2KB = UNPACK_INT32 (VolDescriptor->BootRecordVolume.EltCatalog);
    // Ensure the LBA (in 2KB unit) fits into our media
    if (Lba2KB * (SIZE_2KB / Media->BlockSize) > Media->LastBlock) {
      continue;
    }

    Status = DiskIo->ReadDisk (
                       DiskIo,
                       Media->MediaId,
                       MultU64x32 (Lba2KB, SIZE_2KB),
                       SIZE_2KB,
                       Catalog
                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "EltCheckDevice: error reading catalog %r\n", Status));
      continue;
    }
    //
    // We don't care too much about the Catalog header's contents, but we do want
    // to make sure it looks like a Catalog header
    //
    if (Catalog->Catalog.Indicator != ELTORITO_ID_CATALOG || Catalog->Catalog.Id55AA != 0xAA55) {
      DEBUG ((EFI_D_ERROR, "EltCheckBootCatalog: El Torito boot catalog header IDs not correct\n"));
      continue;
    }

    Check       = 0;
    CheckBuffer = (UINT16 *) Catalog;
    for (Index = 0; Index < sizeof (ELTORITO_CATALOG) / sizeof (UINT16); Index += 1) {
      Check += CheckBuffer[Index];
    }

    if ((Check & 0xFFFF) != 0) {
      DEBUG ((EFI_D_ERROR, "EltCheckBootCatalog: El Torito boot catalog header checksum failed\n"));
      continue;
    }

    MaxIndex = Media->BlockSize / sizeof (ELTORITO_CATALOG);
    for (Index = 1, BootEntry = 1; Index < MaxIndex; Index += 1) {
      //
      // Next entry
      //
      Catalog += 1;

      //
      // Check this entry
      //
      if (Catalog->Boot.Indicator != ELTORITO_ID_SECTION_BOOTABLE || Catalog->Boot.Lba == 0) {
        continue;
      }

      SubBlockSize  = 512;
      SectorCount   = Catalog->Boot.SectorCount;

      switch (Catalog->Boot.MediaType) {

      case ELTORITO_NO_EMULATION:
        SubBlockSize = Media->BlockSize;
        break;

      case ELTORITO_HARD_DISK:
        break;

      case ELTORITO_12_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x0F;
        break;

      case ELTORITO_14_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x12;
        break;

      case ELTORITO_28_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x24;
        break;

      default:
        DEBUG ((EFI_D_INIT, "EltCheckDevice: unsupported El Torito boot media type %x\n", Catalog->Boot.MediaType));
        SectorCount   = 0;
        SubBlockSize  = Media->BlockSize;
        break;
      }
      //
      // Create child device handle
      //
      CdDev.Header.Type     = MEDIA_DEVICE_PATH;
      CdDev.Header.SubType  = MEDIA_CDROM_DP;
      SetDevicePathNodeLength (&CdDev.Header, sizeof (CdDev));

      if (Index == 1) {
        //
        // This is the initial/default entry
        //
        BootEntry = 0;
      }

      CdDev.BootEntry = (UINT32) BootEntry;
      BootEntry++;
      CdDev.PartitionStart = Catalog->Boot.Lba * (SIZE_2KB / Media->BlockSize);
      if (SectorCount < 2) {
        //
        // When the SectorCount < 2, set the Partition as the whole CD.
        //
        if (VolSpaceSize * (SIZE_2KB / Media->BlockSize) > (Media->LastBlock + 1)) {
          CdDev.PartitionSize = (UINT32)(Media->LastBlock - Catalog->Boot.Lba * (SIZE_2KB / Media->BlockSize) + 1);
        } else {
          CdDev.PartitionSize = (UINT32)(VolSpaceSize - Catalog->Boot.Lba) * (SIZE_2KB / Media->BlockSize);
        }
      } else {
        CdDev.PartitionSize = DivU64x32 (
                                MultU64x32 (
                                  SectorCount * (SIZE_2KB / Media->BlockSize),
                                  SubBlockSize
                                  ) + Media->BlockSize - 1,
                                Media->BlockSize
                                );
      }

      ZeroMem (&PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
      PartitionInfo.Revision = EFI_PARTITION_INFO_PROTOCOL_REVISION;
      PartitionInfo.Type     = PARTITION_TYPE_OTHER;

      ZeroMem (&ApplePartitionInfo, sizeof (APPLE_PARTITION_INFO_PROTOCOL));
      ApplePartitionInfo.Revision        = APPLE_PARTITION_INFO_REVISION;
      ApplePartitionInfo.PartitionNumber = CdDev.BootEntry;
      ApplePartitionInfo.PartitionStart  = CdDev.PartitionStart;
      ApplePartitionInfo.PartitionSize   = CdDev.PartitionSize;

      Status = PartitionInstallChildHandle (
                This,
                Handle,
                DiskIo,
                DiskIo2,
                BlockIo,
                BlockIo2,
                DevicePath,
                (EFI_DEVICE_PATH_PROTOCOL *) &CdDev,
                &PartitionInfo,
                &ApplePartitionInfo,
                Catalog->Boot.Lba * (SIZE_2KB / Media->BlockSize),
                Catalog->Boot.Lba * (SIZE_2KB / Media->BlockSize) + CdDev.PartitionSize - 1,
                SubBlockSize,
                NULL
                );
      if (!EFI_ERROR (Status)) {
        Found = EFI_SUCCESS;
      }
    }
  }

  FreePool (VolDescriptor);

  return Found;
}
