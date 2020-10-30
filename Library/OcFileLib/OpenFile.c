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
#include <Library/OcFileLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
SafeFileOpen (
  IN  EFI_FILE_PROTOCOL       *Protocol,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CONST CHAR16            *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  )
{
  EFI_STATUS  Status;
  UINTN       Length;

  DEBUG_CODE_BEGIN ();
  ASSERT (FileName != NULL);
  ASSERT (NewHandle != NULL);
  Length = StrLen (FileName);
  if (Length > 0 && FileName[Length - 1] == L'\\') {
    DEBUG ((DEBUG_INFO, "OCFS: Filename %s has trailing slash\n", FileName));
  }
  DEBUG_CODE_END ();

  *NewHandle = NULL;
  Status = Protocol->Open (
    Protocol,
    NewHandle,
    (CHAR16 *) FileName,
    OpenMode,
    Attributes
    );
  //
  // Some boards like ASUS ROG RAMPAGE VI EXTREME may have malfunctioning FS
  // drivers that report write protection violation errors for read-only
  // operations but otherwise function as expected.
  //
  // REF: https://github.com/acidanthera/bugtracker/issues/1242
  //
  if (Status == EFI_WRITE_PROTECTED
   && OpenMode == EFI_FILE_MODE_READ
   && Attributes == 0
   && *NewHandle != NULL) {
    DEBUG ((
      DEBUG_VERBOSE,
      "OCFS: Avoid invalid WP error for Filename %s\n",
      FileName
      ));
    Status = EFI_SUCCESS;
  }

  return Status;
}

EFI_STATUS
EFIAPI
OcOpenFileByRemainingDevicePath (
  IN  EFI_HANDLE                      FileSystemHandle,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath,
  OUT EFI_FILE_PROTOCOL               **File,
  IN  UINT64                          OpenMode,
  IN  UINT64                          Attributes
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *LastFile;
  CONST EFI_DEVICE_PATH_PROTOCOL  *FilePathNode;
  CHAR16                          *AlignedPathName;
  CHAR16                          *PathName;
  UINTN                           PathLength;
  EFI_FILE_PROTOCOL               *NextFile;

  ASSERT (FileSystemHandle != NULL);
  ASSERT (RemainingDevicePath != NULL);
  ASSERT (File != NULL);

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
  FilePathNode = RemainingDevicePath;
  while (!IsDevicePathEnd (FilePathNode)) {
    if (DevicePathType (FilePathNode) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (FilePathNode) != MEDIA_FILEPATH_DP) {
      Status = EFI_INVALID_PARAMETER;
      goto CloseLastFile;
    }

    //
    // FilePathNode->PathName may be unaligned, and the UEFI specification
    // requires pointers that are passed to protocol member functions to be
    // aligned. Create an aligned copy of the pathname to match that
    // and to apply the hack below.
    //
    AlignedPathName = AllocateCopyPool (
                        (DevicePathNodeLength (FilePathNode) -
                         SIZE_OF_FILEPATH_DEVICE_PATH),
                        ((CONST FILEPATH_DEVICE_PATH *) FilePathNode)->PathName
                        );
    if (AlignedPathName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto CloseLastFile;
    }

    //
    // This is a compatibility hack for firmware types that do not support
    // opening filepaths (directories) with a trailing slash.
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
    Status = SafeFileOpen (
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
    FilePathNode = NextDevicePathNode (FilePathNode);
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

EFI_STATUS
EFIAPI
OcOpenFileByDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  OUT    EFI_FILE_PROTOCOL         **File,
  IN     UINT64                    OpenMode,
  IN     UINT64                    Attributes
  )
{
  EFI_STATUS Status;
  EFI_HANDLE FileSystemHandle;

  ASSERT (File != NULL);
  ASSERT (FilePath != NULL);

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

  return OcOpenFileByRemainingDevicePath (
           FileSystemHandle,
           *FilePath,
           File,
           OpenMode,
           Attributes
           );
}
