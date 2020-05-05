/** @file
  Copyright (C) 2019, Download-Fritz.  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Guid/Gpt.h>

#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC EFI_GUID mInternalDiskPartitionEntriesProtocolGuid = {
  0x1A81704, 0x3442, 0x4A7D, { 0x87, 0x40, 0xF4, 0xEC, 0x5B, 0xBE, 0x59, 0x77 }
};

STATIC EFI_GUID mInternalPartitionEntryProtocolGuid = {
  0x9FC6B19, 0xB8A1, 0x4A01, { 0x8D, 0xB1, 0x87, 0x94, 0xE7, 0x63, 0x4C, 0xA5 }
};

EFI_STATUS
OcDiskInitializeContext (
  OUT OC_DISK_CONTEXT  *Context,
  IN  EFI_HANDLE       DiskHandle,
  IN  BOOLEAN          UseBlockIo2
  )
{
  EFI_STATUS  Status;

  //
  // Retrieve the Block I/O protocol.
  //
  if (UseBlockIo2) {
    Status = gBS->HandleProtocol (
      DiskHandle,
      &gEfiBlockIo2ProtocolGuid,
      (VOID **) &Context->BlockIo2
      );
  } else {
    Context->BlockIo2 = NULL;
    Status = EFI_ABORTED;
  }

  if (EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
      DiskHandle,
      &gEfiBlockIoProtocolGuid,
      (VOID **) &Context->BlockIo
      );
  } else {
    Context->BlockIo = NULL;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCPI: Block I/O (%d/%d) protocols (%d) are not present on %p - %r\n",
      Context->BlockIo != NULL,
      Context->BlockIo2 != NULL,
      UseBlockIo2,
      DiskHandle,
      Status
      ));
    return Status;
  }

  if (Context->BlockIo2 != NULL && Context->BlockIo2->Media != NULL) {
    Context->BlockSize = Context->BlockIo2->Media->BlockSize;
    Context->MediaId   = Context->BlockIo2->Media->MediaId;
  } else if (Context->BlockIo != NULL && Context->BlockIo->Media != NULL) {
    Context->BlockSize = Context->BlockIo->Media->BlockSize;
    Context->MediaId   = Context->BlockIo->Media->MediaId;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Check that BlockSize is POT.
  //
  if (Context->BlockSize == 0 || (Context->BlockSize & (Context->BlockSize - 1)) != 0) {
    DEBUG ((DEBUG_INFO, "OCPI: Block I/O has invalid block size %u\n", Context->BlockSize));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcDiskRead (
  IN  OC_DISK_CONTEXT  *Context,
  IN  UINT64           Lba,
  IN  UINTN            BufferSize,
  OUT VOID             *Buffer
  )
{
  EFI_STATUS  Status;

  ASSERT (Context->BlockIo != NULL || Context->BlockIo2 != NULL);
  ASSERT ((BufferSize & (Context->BlockSize - 1)) == 0);

  if (Context->BlockIo2 != NULL) {
    Status = Context->BlockIo2->ReadBlocksEx (
      Context->BlockIo2,
      Context->MediaId,
      Lba,
      NULL,
      BufferSize,
      Buffer
      );
  } else {
    Status = Context->BlockIo->ReadBlocks (
      Context->BlockIo,
      Context->MediaId,
      Lba,
      BufferSize,
      Buffer
      );
  }

  return Status;
}

STATIC
VOID
InternalDebugPrintPartitionEntry (
  IN UINTN                      ErrorLevel,
  IN CONST CHAR8                *Message,
  IN CONST EFI_PARTITION_ENTRY  *PartitionEntry
  )
{
  ASSERT (PartitionEntry != NULL);

  DEBUG ((
    ErrorLevel,
    "%a:\n"
    "- PartitionTypeGUID: %g\n"
    "- UniquePartitionGUID: %g\n"
    "- StartingLBA: %lx\n"
    "- EndingLBA: %lx\n"
    "- Attributes: %lx\n"
    "- PartitionName: %s\n",
    Message,
    PartitionEntry->PartitionTypeGUID,
    PartitionEntry->UniquePartitionGUID,
    PartitionEntry->StartingLBA,
    PartitionEntry->EndingLBA,
    PartitionEntry->Attributes,
    PartitionEntry->PartitionName
    ));
}

STATIC
EFI_HANDLE
InternalPartitionGetDiskHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath,
  IN  UINTN                     HdNodeOffset,
  OUT BOOLEAN                   *HasBlockIo2
  )
{
  EFI_HANDLE               DiskHandle;

  EFI_STATUS               Status;

  EFI_DEVICE_PATH_PROTOCOL *PrefixPath;
  EFI_DEVICE_PATH_PROTOCOL *TempPath;

  ASSERT (HdDevicePath != NULL);
  ASSERT (HdNodeOffset < GetDevicePathSize (HdDevicePath));
  ASSERT (HasBlockIo2 != NULL);

  PrefixPath = DuplicateDevicePath (HdDevicePath);
  if (PrefixPath == NULL) {
    DEBUG ((DEBUG_INFO, "OCPI: DP allocation error\n"));
    return NULL;
  }
  //
  // Strip the HD node in order to retrieve the last node supporting Block I/O
  // before it, which is going to be its disk.
  //
  TempPath = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)PrefixPath + HdNodeOffset);
  SetDevicePathEndNode (TempPath);
  
  TempPath = PrefixPath;
  Status = gBS->LocateDevicePath (
                  &gEfiBlockIo2ProtocolGuid,
                  &TempPath,
                  &DiskHandle
                  );
  *HasBlockIo2 = !EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    TempPath = PrefixPath;
    Status = gBS->LocateDevicePath (
                    &gEfiBlockIoProtocolGuid,
                    &TempPath,
                    &DiskHandle
                    );
  }

  if (EFI_ERROR (Status)) {
    DebugPrintDevicePath (
      DEBUG_INFO,
      "OCPI: Failed to locate disk",
      PrefixPath
      );

    DiskHandle = NULL;
  }

  FreePool (PrefixPath);

  return DiskHandle;
}

/**
  Retrieve the disk's device handle from a partition's Device Path.

  @param[in] HdDevicePath  The Device Path of the partition.

**/
EFI_HANDLE
OcPartitionGetDiskHandle (
  IN EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
  )
{
  CONST HARDDRIVE_DEVICE_PATH *HdNode;
  BOOLEAN                     Dummy;

  ASSERT (HdDevicePath != NULL);

  HdNode = (HARDDRIVE_DEVICE_PATH *)(
             FindDevicePathNodeWithType (
               HdDevicePath,
               MEDIA_DEVICE_PATH,
               MEDIA_HARDDRIVE_DP
               )
             );
  if (HdNode == NULL) {
    return NULL;
  }

  return InternalPartitionGetDiskHandle (
           HdDevicePath,
           (UINTN)HdNode - (UINTN)HdDevicePath,
           &Dummy
           );
}

EFI_DEVICE_PATH_PROTOCOL *
OcDiskFindSystemPartitionPath (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *DiskDevicePath,
  OUT UINTN                           *EspDevicePathSize,
  OUT EFI_HANDLE                      *EspDeviceHandle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *EspDevicePath;

  EFI_STATUS                Status;
  BOOLEAN                   Result;
  INTN                      CmpResult;

  UINTN                     Index;
  UINTN                     NumHandles;
  EFI_HANDLE                *Handles;
  EFI_HANDLE                Handle;

  UINTN                     DiskDpSize;
  UINTN                     DiskDpCmpSize;
  EFI_DEVICE_PATH_PROTOCOL *HdDevicePath;
  UINTN                     HdDpSize;

  CONST EFI_PARTITION_ENTRY *PartEntry;

  ASSERT (DiskDevicePath != NULL);
  ASSERT (EspDevicePathSize != NULL);
  ASSERT (EspDeviceHandle != NULL);

  DebugPrintDevicePath (
    DEBUG_INFO,
    "OCPI: Locating disk's ESP",
    (EFI_DEVICE_PATH_PROTOCOL *)DiskDevicePath
    );

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPI: Failed to locate FS handles\n"));
    return NULL;
  }

  EspDevicePath = NULL;

  DiskDpSize = GetDevicePathSize (DiskDevicePath);
  //
  // The partition's Device Path must be at least as big as the disk's (prefix)
  // plus an additional HardDrive node.
  //
  Result = OcOverflowAddUN (
             DiskDpSize,
             sizeof (HARDDRIVE_DEVICE_PATH),
             &DiskDpCmpSize
             );
  if (Result) {
    DEBUG ((DEBUG_INFO, "OCPI: HD node would overflow DP\n"));
    return NULL;
  }

  for (Index = 0; Index < NumHandles; ++Index) {
    Handle = Handles[Index];

    HdDevicePath = DevicePathFromHandle (Handle);
    if (HdDevicePath == NULL) {
      continue;
    }

    HdDpSize = GetDevicePathSize (HdDevicePath);
    if (HdDpSize < DiskDpCmpSize) {
      continue;
    }
    //
    // Verify the partition's Device Path has the disk's prefixed.
    //
    CmpResult = CompareMem (
                  HdDevicePath,
                  DiskDevicePath,
                  DiskDpSize - END_DEVICE_PATH_LENGTH
                  );
    if (CmpResult != 0) {
      continue;
    }

    DebugPrintDevicePath (DEBUG_INFO, "OCPI: Discovered HD DP", HdDevicePath);

    PartEntry = OcGetGptPartitionEntry (Handle);
    if (PartEntry == NULL) {
      continue;
    }

    InternalDebugPrintPartitionEntry (
      DEBUG_INFO,
      "OCPI: Discovered PartEntry",
      PartEntry
      );

    if (CompareGuid (&PartEntry->PartitionTypeGUID, &gEfiPartTypeSystemPartGuid)) {
      EspDevicePath = HdDevicePath;
      *EspDevicePathSize = HdDpSize;
      *EspDeviceHandle = Handle;
      break;
    }
  }

  FreePool (Handles);

  return EspDevicePath;
}

CONST OC_PARTITION_ENTRIES *
OcGetDiskPartitions (
  IN EFI_HANDLE  DiskHandle,
  IN BOOLEAN     UseBlockIo2
  )
{
  OC_PARTITION_ENTRIES       *PartEntries;

  EFI_STATUS                 Status;
  BOOLEAN                    Result;

  OC_DISK_CONTEXT            DiskContext;

  EFI_LBA                    PartEntryLBA;
  UINT32                     NumPartitions;
  UINT32                     PartEntrySize;
  UINTN                      PartEntriesSize;
  UINTN                      PartEntriesStructSize;
  UINTN                      BufferSize;
  EFI_PARTITION_TABLE_HEADER *GptHeader;

  ASSERT (DiskHandle != NULL);

  Status = gBS->HandleProtocol (
    DiskHandle,
    &mInternalDiskPartitionEntriesProtocolGuid,
    (VOID **) &PartEntries
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPI: Located cached partition entries\n"));
    return PartEntries;
  }

  Status = OcDiskInitializeContext (&DiskContext, DiskHandle, UseBlockIo2);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Retrieve the GPT header.
  //
  BufferSize = ALIGN_VALUE (sizeof (*GptHeader), DiskContext.BlockSize);
  GptHeader = AllocatePool (BufferSize);
  if (GptHeader == NULL) {
    DEBUG ((DEBUG_INFO, "OCPI: GPT header allocation error\n"));
    return NULL;
  }

  Status = OcDiskRead (
    &DiskContext,
    PRIMARY_PART_HEADER_LBA,
    BufferSize,
    GptHeader
    );
  if (EFI_ERROR (Status)) {
    FreePool (GptHeader);
    DEBUG ((
      DEBUG_INFO,
      "OCPI: ReadDisk1 (block: %u, io1: %d, io2: %d, size: %u) %r\n",
      DiskContext.BlockSize,
      DiskContext.BlockIo != NULL,
      DiskContext.BlockIo2 != NULL,
      (UINT32) BufferSize,
      Status
      ));
    return NULL;
  }

  if (GptHeader->Header.Signature != EFI_PTAB_HEADER_ID) {
    FreePool (GptHeader);
    DEBUG ((DEBUG_INFO, "OCPI: Partition table not supported\n"));
    return NULL;
  }

  PartEntrySize = GptHeader->SizeOfPartitionEntry;
  if (PartEntrySize < sizeof (EFI_PARTITION_ENTRY)) {
    FreePool (GptHeader);
    DEBUG ((DEBUG_INFO, "OCPI: GPT header is malformed\n"));
    return NULL;
  }

  NumPartitions = GptHeader->NumberOfPartitionEntries;
  PartEntryLBA  = GptHeader->PartitionEntryLBA;

  FreePool (GptHeader);

  Result = OcOverflowMulUN (NumPartitions, PartEntrySize, &PartEntriesSize);
  if (Result || MAX_UINTN - DiskContext.BlockSize < PartEntriesSize) {
    DEBUG ((DEBUG_INFO, "OCPI: Partition entries size overflows\n"));
    return NULL;
  }

  PartEntriesSize = ALIGN_VALUE (PartEntriesSize, DiskContext.BlockSize);

  Result = OcOverflowAddUN (
    sizeof (PartEntries),
    PartEntriesSize,
    &PartEntriesStructSize
    );
  if (Result) {
    DEBUG ((DEBUG_INFO, "OCPI: Partition entries struct size overflows\n"));
    return NULL;
  }
  //
  // Retrieve the GPT partition entries.
  //
  PartEntries = AllocatePool (PartEntriesStructSize);
  if (PartEntries == NULL) {
    DEBUG ((DEBUG_INFO, "OCPI: Partition entries allocation error\n"));
    return NULL;
  }

  Status = OcDiskRead (
    &DiskContext,
    PartEntryLBA,
    PartEntriesSize,
    PartEntries->FirstEntry
    );
  if (EFI_ERROR (Status)) {
    FreePool (PartEntries);
    DEBUG ((
      DEBUG_INFO,
      "OCPI: ReadDisk2 (block: %u, io1: %d, io2: %d, size: %u) %r\n",
      DiskContext.BlockSize,
      DiskContext.BlockIo != NULL,
      DiskContext.BlockIo2 != NULL,
      (UINT32) PartEntriesSize,
      Status
      ));
    return NULL;
  }

  PartEntries->NumPartitions      = NumPartitions;
  PartEntries->PartitionEntrySize = PartEntrySize;
  //
  // FIXME: This causes the handle to be dangling if the device is detached.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
    &DiskHandle,
    &mInternalDiskPartitionEntriesProtocolGuid,
    PartEntries,
    NULL
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPI: Failed to cache partition entries\n"));
    FreePool (PartEntries);
    return NULL;
  }

  return PartEntries;
}

/**
  Retrieve the partition's GPT information, if applicable

  @param[in] FsHandle  The device handle of the partition to retrieve info of.

**/
CONST EFI_PARTITION_ENTRY *
OcGetGptPartitionEntry (
  IN EFI_HANDLE  FsHandle
  )
{
  CONST EFI_PARTITION_ENTRY        *PartEntry;
  CONST OC_PARTITION_ENTRIES       *Partitions;

  EFI_STATUS                       Status;
  EFI_DEVICE_PATH_PROTOCOL         *FsDevicePath;
  CONST HARDDRIVE_DEVICE_PATH      *HdNode;
  EFI_HANDLE                       DiskHandle;
  BOOLEAN                          HasBlockIo2;
  UINTN                            Offset;

  ASSERT (FsHandle != NULL);

  Status = gBS->HandleProtocol (
                  FsHandle,
                  &mInternalPartitionEntryProtocolGuid,
                  (VOID **)&PartEntry
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPI: Located cached partition entry\n"));
    return PartEntry;
  }
  //
  // Retrieve the partition Device Path information.
  //
  FsDevicePath = DevicePathFromHandle (FsHandle);
  if (FsDevicePath == NULL) {
    DEBUG ((DEBUG_INFO, "OCPI: Failed to retrieve Device Path\n"));
    return NULL;
  }

  HdNode = (HARDDRIVE_DEVICE_PATH *)(
             FindDevicePathNodeWithType (
               FsDevicePath,
               MEDIA_DEVICE_PATH,
               MEDIA_HARDDRIVE_DP
               )
             );
  if (HdNode == NULL) {
    DEBUG ((DEBUG_INFO, "OCPI: Device Path does not describe a partition\n"));
    return NULL;
  }

  DiskHandle = InternalPartitionGetDiskHandle (
                 FsDevicePath,
                 (UINTN)HdNode - (UINTN)FsDevicePath,
                 &HasBlockIo2
                 );
  if (DiskHandle == NULL) {
    DebugPrintDevicePath (
      DEBUG_INFO,
      "OCPI: Could not locate partition's disk",
      FsDevicePath
      );
    return NULL;
  }
  //
  // Get the disk's GPT partition entries.
  //
  Partitions = OcGetDiskPartitions (DiskHandle, HasBlockIo2);
  if (Partitions == NULL) {
    DEBUG ((DEBUG_INFO, "OCPI: Failed to retrieve disk info\n"));
    return NULL;
  }

  if (HdNode->PartitionNumber > Partitions->NumPartitions) {
    DEBUG ((DEBUG_INFO, "OCPI: Partition is OOB\n"));
    return NULL;
  }

  ASSERT (HdNode->PartitionNumber > 0);
  Offset = ((UINTN)(HdNode->PartitionNumber - 1) * Partitions->PartitionEntrySize);
  PartEntry = (EFI_PARTITION_ENTRY *)((UINTN)Partitions->FirstEntry + Offset);
  //
  // FIXME: This causes the handle to be dangling if the device is detached.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &FsHandle,
                  &mInternalPartitionEntryProtocolGuid,
                  PartEntry,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPI: Failed to cache partition entry\n"));
    return NULL;
  }

  return PartEntry;
}
