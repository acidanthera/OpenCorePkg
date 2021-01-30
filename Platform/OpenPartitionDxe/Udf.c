/** @file
  Scan for an UDF file system on a formatted media.

  Caution: This file requires additional review when modified.
  This driver will have external input - CD/DVD media.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  FindUdfFileSystem() routine will consume the media properties and do basic
  validation.

  Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
  Copyright (C) 2014-2017 Paulo Alcantara <pcacjr@zytor.com>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Partition.h"

#define MAX_CORRECTION_BLOCKS_NUM 512u

//
// C5BD4D42-1A76-4996-8956-73CDA326CD0A
//
#define EFI_UDF_DEVICE_PATH_GUID                        \
  { 0xC5BD4D42, 0x1A76, 0x4996,                         \
    { 0x89, 0x56, 0x73, 0xCD, 0xA3, 0x26, 0xCD, 0x0A }  \
  }

typedef struct {
  VENDOR_DEVICE_PATH        DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  End;
} UDF_DEVICE_PATH;

//
// Vendor-Defined Device Path GUID for UDF file system
//
EFI_GUID gUdfDevPathGuid = EFI_UDF_DEVICE_PATH_GUID;

//
// Vendor-Defined Media Device Path for UDF file system
//
UDF_DEVICE_PATH gUdfDevicePath = {
  { { MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH), 0 } },
    EFI_UDF_DEVICE_PATH_GUID
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

/**
  Find the anchor volume descriptor pointer.

  @param[in]  BlockIo               BlockIo interface.
  @param[in]  DiskIo                DiskIo interface.
  @param[out] AnchorPoint           Anchor volume descriptor pointer.
  @param[out] LastRecordedBlock     Last recorded block.

  @retval EFI_SUCCESS               Anchor volume descriptor pointer found.
  @retval EFI_VOLUME_CORRUPTED      The file system structures are corrupted.
  @retval other                     Anchor volume descriptor pointer not found.

**/
EFI_STATUS
FindAnchorVolumeDescriptorPointer (
  IN   EFI_BLOCK_IO_PROTOCOL                 *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL                  *DiskIo,
  OUT  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoint,
  OUT  EFI_LBA                               *LastRecordedBlock
  )
{
  EFI_STATUS                            Status;
  UINT32                                BlockSize;
  EFI_LBA                               EndLBA;
  UDF_DESCRIPTOR_TAG                    *DescriptorTag;
  UINTN                                 AvdpsCount;
  UINTN                                 Size;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoints;
  INTN                                  Index;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPointPtr;
  EFI_LBA                               LastAvdpBlockNum;

  //
  // UDF 2.60, 2.2.3 Anchor Volume Descriptor Pointer
  //
  // An Anchor Volume Descriptor Pointer structure shall be recorded in at
  // least 2 of the following 3 locations on the media: Logical Sector 256,
  // N - 256 or N, where N is the last *addressable* sector of a volume.
  //
  // To figure out what logical sector N is, the SCSI commands READ CAPACITY and
  // READ TRACK INFORMATION are used, however many drives or medias report their
  // "last recorded block" wrongly. Although, READ CAPACITY returns the last
  // readable data block but there might be unwritten blocks, which are located
  // outside any track and therefore AVDP will not be found at block N.
  //
  // That said, we define a magic number of 512 blocks to be used as correction
  // when attempting to find AVDP and define last block number.
  //
  BlockSize = BlockIo->Media->BlockSize;
  EndLBA = BlockIo->Media->LastBlock;
  *LastRecordedBlock = EndLBA;
  AvdpsCount = 0;

  //
  // Check if the block size of the underlying media can hold the data of an
  // Anchor Volume Descriptor Pointer
  //
  if (BlockSize < sizeof (UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Media block size 0x%x unable to hold an AVDP.\n",
      __FUNCTION__,
      BlockSize
      ));
    return EFI_UNSUPPORTED;
  }

  //
  // Find AVDP at block 256
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (256, BlockSize),
    sizeof (*AnchorPoint),
    AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DescriptorTag = &AnchorPoint->DescriptorTag;

  //
  // Check if read block is a valid AVDP descriptor
  //
  if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
    DEBUG ((DEBUG_INFO, "%a: found AVDP at block %d\n", __FUNCTION__, 256));
    AvdpsCount++;
  }

  //
  // Find AVDP at block N - 256
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 ((UINT64)EndLBA - 256, BlockSize),
    sizeof (*AnchorPoint),
    AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if read block is a valid AVDP descriptor
  //
  if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer &&
      ++AvdpsCount == 2) {
    DEBUG ((DEBUG_INFO, "%a: found AVDP at block %Ld\n", __FUNCTION__,
            EndLBA - 256));
    return EFI_SUCCESS;
  }

  //
  // Check if at least one AVDP was found in previous locations
  //
  if (AvdpsCount == 0) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Find AVDP at block N
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 ((UINT64)EndLBA, BlockSize),
    sizeof (*AnchorPoint),
    AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if read block is a valid AVDP descriptor
  //
  if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
    return EFI_SUCCESS;
  }

  //
  // No AVDP found at block N. Possibly drive/media returned bad last recorded
  // block, or it is part of unwritten data blocks and outside any track.
  //
  // Search backwards for an AVDP from block N-1 through
  // N-MAX_CORRECTION_BLOCKS_NUM. If any AVDP is found, then correct last block
  // number for the new UDF partition child handle.
  //
  Size = MAX_CORRECTION_BLOCKS_NUM * BlockSize;

  AnchorPoints = AllocateZeroPool (Size);
  if (AnchorPoints == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read consecutive MAX_CORRECTION_BLOCKS_NUM disk blocks
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 ((UINT64)EndLBA - MAX_CORRECTION_BLOCKS_NUM, BlockSize),
    Size,
    AnchorPoints
    );
  if (EFI_ERROR (Status)) {
    goto Out_Free;
  }

  Status = EFI_VOLUME_CORRUPTED;

  //
  // Search for AVDP from blocks N-1 through N-MAX_CORRECTION_BLOCKS_NUM
  //
  for (Index = MAX_CORRECTION_BLOCKS_NUM - 2; Index >= 0; Index--) {
    AnchorPointPtr = (VOID *)((UINTN)AnchorPoints + Index * BlockSize);

    DescriptorTag = &AnchorPointPtr->DescriptorTag;

    //
    // Check if read block is a valid AVDP descriptor
    //
    if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
      //
      // Calculate last recorded block number
      //
      LastAvdpBlockNum = EndLBA - (MAX_CORRECTION_BLOCKS_NUM - Index);
      DEBUG ((DEBUG_WARN, "%a: found AVDP at block %Ld\n", __FUNCTION__,
              LastAvdpBlockNum));
      DEBUG ((DEBUG_WARN, "%a: correcting last block from %Ld to %Ld\n",
              __FUNCTION__, EndLBA, LastAvdpBlockNum));
      //
      // Save read AVDP from last block
      //
      CopyMem (AnchorPoint, AnchorPointPtr, sizeof (*AnchorPointPtr));
      //
      // Set last recorded block number
      //
      *LastRecordedBlock = LastAvdpBlockNum;
      Status = EFI_SUCCESS;
      break;
    }
  }

Out_Free:
  FreePool (AnchorPoints);
  return Status;
}

/**
  Find UDF volume identifiers in a Volume Recognition Sequence.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.

  @retval EFI_SUCCESS             UDF volume identifiers were found.
  @retval EFI_NOT_FOUND           UDF volume identifiers were not found.
  @retval other                   Failed to perform disk I/O.

**/
EFI_STATUS
FindUdfVolumeIdentifiers (
  IN EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN EFI_DISK_IO_PROTOCOL   *DiskIo
  )
{
  EFI_STATUS                            Status;
  UINT64                                Offset;
  UINT64                                EndDiskOffset;
  CDROM_VOLUME_DESCRIPTOR               VolDescriptor;
  CDROM_VOLUME_DESCRIPTOR               TerminatingVolDescriptor;

  ZeroMem ((VOID *)&TerminatingVolDescriptor, sizeof (CDROM_VOLUME_DESCRIPTOR));

  //
  // Start Volume Recognition Sequence
  //
  EndDiskOffset = MultU64x32 (BlockIo->Media->LastBlock,
                              BlockIo->Media->BlockSize);

  for (Offset = UDF_VRS_START_OFFSET; Offset < EndDiskOffset;
       Offset += UDF_LOGICAL_SECTOR_SIZE) {
    //
    // Check if block device has a Volume Structure Descriptor and an Extended
    // Area.
    //
    Status = DiskIo->ReadDisk (
      DiskIo,
      BlockIo->Media->MediaId,
      Offset,
      sizeof (CDROM_VOLUME_DESCRIPTOR),
      (VOID *)&VolDescriptor
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                    (VOID *)UDF_BEA_IDENTIFIER,
                    sizeof (VolDescriptor.Unknown.Id)) == 0) {
      break;
    }

    if ((CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                     (VOID *)CDVOL_ID,
                     sizeof (VolDescriptor.Unknown.Id)) != 0) ||
        (CompareMem ((VOID *)&VolDescriptor,
                     (VOID *)&TerminatingVolDescriptor,
                     sizeof (CDROM_VOLUME_DESCRIPTOR)) == 0)) {
      return EFI_NOT_FOUND;
    }
  }

  //
  // Look for "NSR0{2,3}" identifiers in the Extended Area.
  //
  Offset += UDF_LOGICAL_SECTOR_SIZE;
  if (Offset >= EndDiskOffset) {
    return EFI_NOT_FOUND;
  }

  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    Offset,
    sizeof (CDROM_VOLUME_DESCRIPTOR),
    (VOID *)&VolDescriptor
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                   (VOID *)UDF_NSR2_IDENTIFIER,
                   sizeof (VolDescriptor.Unknown.Id)) != 0) &&
      (CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                   (VOID *)UDF_NSR3_IDENTIFIER,
                   sizeof (VolDescriptor.Unknown.Id)) != 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // Look for "TEA01" identifier in the Extended Area
  //
  Offset += UDF_LOGICAL_SECTOR_SIZE;
  if (Offset >= EndDiskOffset) {
    return EFI_NOT_FOUND;
  }

  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    Offset,
    sizeof (CDROM_VOLUME_DESCRIPTOR),
    (VOID *)&VolDescriptor
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                  (VOID *)UDF_TEA_IDENTIFIER,
                  sizeof (VolDescriptor.Unknown.Id)) != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Check if Logical Volume Descriptor is supported by current EDK2 UDF file
  system implementation.

  @param[in]  LogicalVolDesc  Logical Volume Descriptor pointer.

  @retval TRUE                Logical Volume Descriptor is supported.
  @retval FALSE               Logical Volume Descriptor is not supported.

**/
BOOLEAN
IsLogicalVolumeDescriptorSupported (
  UDF_LOGICAL_VOLUME_DESCRIPTOR *LogicalVolDesc
  )
{
  //
  // Check for a valid UDF revision range
  //
  switch (LogicalVolDesc->DomainIdentifier.Suffix.Domain.UdfRevision) {
  case 0x0102:
  case 0x0150:
  case 0x0200:
  case 0x0201:
  case 0x0250:
  case 0x0260:
    break;
  default:
    return FALSE;
  }

  //
  // Check for a single Partition Map
  //
  if (LogicalVolDesc->NumberOfPartitionMaps > 1) {
    return FALSE;
  }
  //
  // UDF 1.02 revision supports only Type 1 (Physical) partitions, but
  // let's check it any way.
  //
  // PartitionMap[0] -> type
  // PartitionMap[1] -> length (in bytes)
  //
  if (LogicalVolDesc->PartitionMaps[0] != 1 ||
      LogicalVolDesc->PartitionMaps[1] != 6) {
    return FALSE;
  }

  return TRUE;
}

/**
  Find UDF logical volume location and whether it is supported by current EDK2
  UDF file system implementation.

  @param[in]  BlockIo               BlockIo interface.
  @param[in]  DiskIo                DiskIo interface.
  @param[in]  AnchorPoint           Anchor volume descriptor pointer.
  @param[in]  LastRecordedBlock     Last recorded block in media.
  @param[out] MainVdsStartBlock     Main VDS starting block number.
  @param[out] MainVdsEndBlock       Main VDS ending block number.

  @retval EFI_SUCCESS               UDF logical volume was found.
  @retval EFI_VOLUME_CORRUPTED      UDF file system structures are corrupted.
  @retval EFI_UNSUPPORTED           UDF logical volume is not supported.
  @retval other                     Failed to perform disk I/O.

**/
EFI_STATUS
FindLogicalVolumeLocation (
  IN   EFI_BLOCK_IO_PROTOCOL                 *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL                  *DiskIo,
  IN   UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoint,
  IN   EFI_LBA                               LastRecordedBlock,
  OUT  UINT64                                *MainVdsStartBlock,
  OUT  UINT64                                *MainVdsEndBlock
  )
{
  EFI_STATUS                     Status;
  UINT32                         BlockSize;
  UDF_EXTENT_AD                  *ExtentAd;
  UINT64                         SeqBlocksNum;
  UINT64                         SeqStartBlock;
  UINT64                         GuardMainVdsStartBlock;
  VOID                           *Buffer;
  UINT64                         SeqEndBlock;
  BOOLEAN                        StopSequence;
  UINTN                          LvdsCount;
  UDF_LOGICAL_VOLUME_DESCRIPTOR  *LogicalVolDesc;
  UDF_DESCRIPTOR_TAG             *DescriptorTag;

  BlockSize = BlockIo->Media->BlockSize;
  ExtentAd = &AnchorPoint->MainVolumeDescriptorSequenceExtent;

  //
  // UDF 2.60, 2.2.3.1 struct MainVolumeDescriptorSequenceExtent
  //
  // The Main Volume Descriptor Sequence Extent shall have a minimum length of
  // 16 logical sectors.
  //
  // Also make sure it does not exceed maximum number of blocks in the disk.
  //
  SeqBlocksNum = DivU64x32 ((UINT64)ExtentAd->ExtentLength, BlockSize);
  if (SeqBlocksNum < 16 || (EFI_LBA)SeqBlocksNum > LastRecordedBlock + 1) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Check for valid Volume Descriptor Sequence starting block number
  //
  SeqStartBlock = (UINT64)ExtentAd->ExtentLocation;
  if (SeqStartBlock > LastRecordedBlock ||
      SeqStartBlock + SeqBlocksNum - 1 > LastRecordedBlock) {
    return EFI_VOLUME_CORRUPTED;
  }

  GuardMainVdsStartBlock = SeqStartBlock;

  //
  // Allocate buffer for reading disk blocks
  //
  Buffer = AllocateZeroPool ((UINTN)BlockSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SeqEndBlock = SeqStartBlock + SeqBlocksNum;
  StopSequence = FALSE;
  LvdsCount = 0;
  Status = EFI_VOLUME_CORRUPTED;
  //
  // Start Main Volume Descriptor Sequence
  //
  for (; SeqStartBlock < SeqEndBlock && !StopSequence; SeqStartBlock++) {
    //
    // Read disk block
    //
    Status = BlockIo->ReadBlocks (
      BlockIo,
      BlockIo->Media->MediaId,
      SeqStartBlock,
      BlockSize,
      Buffer
      );
    if (EFI_ERROR (Status)) {
      goto Out_Free;
    }

    DescriptorTag = Buffer;

    //
    // ECMA 167, 8.4.1 Contents of a Volume Descriptor Sequence
    //
    // - A Volume Descriptor Sequence shall contain one or more Primary Volume
    //   Descriptors.
    // - A Volume Descriptor Sequence shall contain zero or more Implementation
    //   Use Volume Descriptors.
    // - A Volume Descriptor Sequence shall contain zero or more Partition
    //   Descriptors.
    // - A Volume Descriptor Sequence shall contain zero or more Logical Volume
    //   Descriptors.
    // - A Volume Descriptor Sequence shall contain zero or more Unallocated
    //   Space Descriptors.
    //
    switch (DescriptorTag->TagIdentifier) {
    case UdfPrimaryVolumeDescriptor:
    case UdfImplemenationUseVolumeDescriptor:
    case UdfPartitionDescriptor:
    case UdfUnallocatedSpaceDescriptor:
      break;

    case UdfLogicalVolumeDescriptor:
      LogicalVolDesc = Buffer;

      //
      // Check for existence of a single LVD and whether it is supported by
      // current EDK2 UDF file system implementation.
      //
      if (++LvdsCount > 1 ||
          !IsLogicalVolumeDescriptorSupported (LogicalVolDesc)) {
        Status = EFI_UNSUPPORTED;
        StopSequence = TRUE;
      }

      break;

    case UdfTerminatingDescriptor:
      //
      // Stop the sequence when we find a Terminating Descriptor
      // (aka Unallocated Sector), se we don't have to walk all the unallocated
      // area unnecessarily.
      //
      StopSequence = TRUE;
      break;

    default:
      //
      // An invalid Volume Descriptor has been found in the sequece. Volume is
      // corrupted.
      //
      Status = EFI_VOLUME_CORRUPTED;
      goto Out_Free;
    }
  }

  //
  // Check if LVD was found
  //
  if (!EFI_ERROR (Status) && LvdsCount == 1) {
    *MainVdsStartBlock = GuardMainVdsStartBlock;
    //
    // We do not need to read either LVD or PD descriptors to know the last
    // valid block in the found UDF file system. It's already
    // LastRecordedBlock.
    //
    *MainVdsEndBlock = LastRecordedBlock;

    Status = EFI_SUCCESS;
  }

Out_Free:
  //
  // Free block read buffer
  //
  FreePool (Buffer);

  return Status;
}

/**
  Find a supported UDF file system in block device.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from Partition.

  The CD/DVD media is the external input, so this routine will do basic
  validation for the media.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[out] StartingLBA         UDF file system starting LBA.
  @param[out] EndingLBA           UDF file system starting LBA.

  @retval EFI_SUCCESS             UDF file system was found.
  @retval other                   UDF file system was not found.

**/
EFI_STATUS
FindUdfFileSystem (
  IN EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN EFI_DISK_IO_PROTOCOL   *DiskIo,
  OUT EFI_LBA               *StartingLBA,
  OUT EFI_LBA               *EndingLBA
  )
{
  EFI_STATUS                            Status;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  AnchorPoint;
  EFI_LBA                               LastRecordedBlock;

  //
  // Find UDF volume identifiers
  //
  Status = FindUdfVolumeIdentifiers (BlockIo, DiskIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find Anchor Volume Descriptor Pointer
  //
  Status = FindAnchorVolumeDescriptorPointer (
    BlockIo,
    DiskIo,
    &AnchorPoint,
    &LastRecordedBlock
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find Logical Volume location
  //
  Status = FindLogicalVolumeLocation (
    BlockIo,
    DiskIo,
    &AnchorPoint,
    LastRecordedBlock,
    (UINT64 *)StartingLBA,
    (UINT64 *)EndingLBA
    );

  return Status;
}

/**
  Install child handles if the Handle supports UDF/ECMA-167 volume format.

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
PartitionInstallUdfChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_DISK_IO2_PROTOCOL        *DiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *BlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  UINT32                       RemainderByMediaBlockSize;
  EFI_STATUS                   Status;
  EFI_BLOCK_IO_MEDIA           *Media;
  EFI_PARTITION_INFO_PROTOCOL  PartitionInfo;
  APPLE_PARTITION_INFO_PROTOCOL ApplePartitionInfo;
  EFI_LBA                      StartingLBA;
  EFI_LBA                      EndingLBA;
  BOOLEAN                      ChildCreated;

  Media = BlockIo->Media;
  ChildCreated = FALSE;

  //
  // Check if UDF logical block size is multiple of underlying device block size
  //
  DivU64x32Remainder (
    UDF_LOGICAL_SECTOR_SIZE,   // Dividend
    Media->BlockSize,          // Divisor
    &RemainderByMediaBlockSize // Remainder
    );
  if (RemainderByMediaBlockSize != 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Detect El Torito feature first.
  // And always continue to search for UDF.
  //
  Status = PartitionInstallElToritoChildHandles (
             This,
             Handle,
             DiskIo,
             DiskIo2,
             BlockIo,
             BlockIo2,
             DevicePath
             );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "PartitionDxe: El Torito standard found on handle 0x%p.\n", Handle));
    ChildCreated = TRUE;
  }

  //
  // Search for an UDF file system on block device
  //
  Status = FindUdfFileSystem (BlockIo, DiskIo, &StartingLBA, &EndingLBA);
  if (EFI_ERROR (Status)) {
    return (ChildCreated ? EFI_SUCCESS : EFI_NOT_FOUND);
  }

  //
  // Create Partition Info protocol for UDF file system
  //
  ZeroMem (&PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
  PartitionInfo.Revision = EFI_PARTITION_INFO_PROTOCOL_REVISION;
  PartitionInfo.Type = PARTITION_TYPE_OTHER;

  ZeroMem (&ApplePartitionInfo, sizeof (APPLE_PARTITION_INFO_PROTOCOL));
  ApplePartitionInfo.Revision = APPLE_PARTITION_INFO_REVISION;
  ApplePartitionInfo.PartitionNumber = 1;
  ApplePartitionInfo.PartitionStart  = StartingLBA;
  ApplePartitionInfo.PartitionSize   = EndingLBA - StartingLBA + 1;

  //
  // Install partition child handle for UDF file system
  //
  Status = PartitionInstallChildHandle (
    This,
    Handle,
    DiskIo,
    DiskIo2,
    BlockIo,
    BlockIo2,
    DevicePath,
    (EFI_DEVICE_PATH_PROTOCOL *)&gUdfDevicePath,
    &PartitionInfo,
    &ApplePartitionInfo,
    StartingLBA,
    EndingLBA,
    Media->BlockSize,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return (ChildCreated ? EFI_SUCCESS : Status);
  }

  return EFI_SUCCESS;
}
