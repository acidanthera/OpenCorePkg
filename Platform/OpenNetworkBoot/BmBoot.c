/** @file
  Library functions which relate to booting.

Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
Copyright (c) 2011 - 2021, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2021 Hewlett Packard Enterprise Development LP<BR>
Copyright (C) 2024, Mike Beaton. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NetworkBootInternal.h"

EFI_RAM_DISK_PROTOCOL  *mRamDisk = NULL;

/**
  Expand the media device path which points to a BlockIo or SimpleFileSystem instance
  by appending EFI_REMOVABLE_MEDIA_FILE_NAME.

  @param DevicePath  The media device path pointing to a BlockIo or SimpleFileSystem instance.
  @param FullPath    The full path returned by the routine in last call.
                     Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
STATIC
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
STATIC
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
STATIC
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
STATIC
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
STATIC
EFI_DEVICE_PATH_PROTOCOL *
BmExpandLoadFile (
  IN  EFI_HANDLE                LoadFileHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT VOID                      **Data,
  OUT UINT32                    *DataSize
  )
{
  EFI_STATUS                Status;
  EFI_LOAD_FILE_PROTOCOL    *LoadFile;
  VOID                      *FileBuffer;
  EFI_HANDLE                RamDiskHandle;
  UINTN                     BufferSize;
  EFI_DEVICE_PATH_PROTOCOL  *FullPath;

  ASSERT (Data != NULL);
  ASSERT (DataSize != NULL);

  *Data     = NULL;
  *DataSize = 0;

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

  //
  // In call tree of original BmGetLoadOptionBuffer, handling this case
  // is deferred to subsequent call to GetFileBufferByFilePath.
  //
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Limited to UINT32 by DataSize in OC_CUSTOM_READ, which is limited
    // by Size in OcGetFileSize.
    //
    if (BufferSize > MAX_UINT32) {
      return NULL;
    }

    FileBuffer = AllocatePool (BufferSize);
    if (FileBuffer == NULL) {
      return NULL;
    }

    //
    // Call LoadFile with the correct buffer size.
    //
    FullPath = DevicePathFromHandle (LoadFileHandle);
    Status   = LoadFile->LoadFile (LoadFile, FilePath, TRUE, &BufferSize, FileBuffer);
    if (EFI_ERROR (Status)) {
      FreePool (FileBuffer);
      return NULL;
    }

    *DataSize = (UINT32)BufferSize;
    *Data     = FileBuffer;

    return DuplicateDevicePath (FullPath);
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
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT VOID                      **Data,
  OUT UINT32                    *DataSize,
  IN BOOLEAN                    ValidateHttp
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                *Handles;
  UINTN                     HandleCount;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_EVENT                 NotifyEvent;

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

  if (ValidateHttp) {
    NotifyEvent = MonitorHttpBootCallback (Handle);
    if (NotifyEvent == NULL) {
      return NULL;
    }
  }

  Node = BmExpandLoadFile (Handle, FilePath, Data, DataSize);

  if (ValidateHttp) {
    gBS->CloseEvent (NotifyEvent);

    if ((Node != NULL) && !UriWasValidated ()) {
      Print (L"\n"); ///< Sort out cramped spacing
      DEBUG ((DEBUG_ERROR, "NTBT: LoadFile returned value but URI was never validated\n"));
      FreePool (Node);
      return NULL;
    }
  }

  return Node;
}
