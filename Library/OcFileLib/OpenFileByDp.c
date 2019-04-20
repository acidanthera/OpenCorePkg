/** @file
  The UEFI Library provides functions and macros that simplify the development of
  UEFI Drivers and UEFI Applications.  These functions and macros help manage EFI
  events, build simple locks utilizing EFI Task Priority Levels (TPLs), install
  EFI Driver Model related protocols, manage Unicode string tables for UEFI Drivers,
  and print messages on the console output and standard error devices.
  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
EFIAPI
OcOpenFileByDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  OUT    EFI_FILE_PROTOCOL         **File,
  IN     UINT64                    OpenMode,
  IN     UINT64                    Attributes
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      FileSystemHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *LastFile;
  FILEPATH_DEVICE_PATH            *FilePathNode;
  CHAR16                          *AlignedPathName;
  CHAR16                          *PathName;
  UINTN                           PathLength;
  EFI_FILE_PROTOCOL               *NextFile;

  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *File = NULL;

  if (FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Look up the filesystem.
  //
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  FilePath,
                  &FileSystemHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->OpenProtocol (
                  FileSystemHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the root directory of the filesystem. After this operation succeeds,
  // we have to release LastFile on error.
  //
  Status = FileSystem->OpenVolume (FileSystem, &LastFile);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Traverse the device path nodes relative to the filesystem.
  //
  while (!IsDevicePathEnd (*FilePath)) {
    if (DevicePathType (*FilePath) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (*FilePath) != MEDIA_FILEPATH_DP) {
      Status = EFI_INVALID_PARAMETER;
      goto CloseLastFile;
    }
    FilePathNode = (FILEPATH_DEVICE_PATH *)*FilePath;

    //
    // FilePathNode->PathName may be unaligned, and the UEFI specification
    // requires pointers that are passed to protocol member functions to be
    // aligned. Create an aligned copy of the pathname to match that
    // and to apply the hack below.
    //
    AlignedPathName = AllocateCopyPool (
                        (DevicePathNodeLength (FilePathNode) -
                         SIZE_OF_FILEPATH_DEVICE_PATH),
                        FilePathNode->PathName
                        );
    if (AlignedPathName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto CloseLastFile;
    }

    //
    // This is a compatibility hack for firmwares not supporting
    // opening filepaths (directories) with a trailing slash in the end.
    // More details in a852f85986c1fe23fc3a429605e3c560ea800c54 OpenCorePkg commit.
    //
    PathLength = StrLen (AlignedPathName);
    if (PathLength > 0 && AlignedPathName[PathLength - 1] == '\\') {
      AlignedPathName[PathLength - 1] = '\0';
    }

    PathName = AlignedPathName;

    //
    // Open or create the file corresponding to the next pathname fragment.
    //
    Status = LastFile->Open (
                         LastFile,
                         &NextFile,
                         PathName,
                         OpenMode,
                         Attributes
                         );

    FreePool (AlignedPathName);

    if (EFI_ERROR (Status)) {
      goto CloseLastFile;
    }

    //
    // Advance to the next device path node.
    //
    LastFile->Close (LastFile);
    LastFile = NextFile;
    *FilePath = NextDevicePathNode (FilePathNode);
  }

  *File = LastFile;
  return EFI_SUCCESS;

CloseLastFile:
  LastFile->Close (LastFile);

  //
  // We are on the error path; we must have set an error Status for returning
  // to the caller.
  //
  ASSERT (EFI_ERROR (Status));
  return Status;
}
