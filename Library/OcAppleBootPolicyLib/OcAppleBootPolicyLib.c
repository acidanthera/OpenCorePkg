/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <AppleMacEfi.h>

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/AppleBootPolicy.h>

#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcFileLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct {
  EFI_HANDLE             Handle;
  GUID                   ContainerGuid;
  GUID                   VolumeGuid;
  APPLE_APFS_VOLUME_ROLE VolumeRole;
} APFS_VOLUME_INFO;

typedef struct {
  EFI_HANDLE        Handle;
  CHAR16            *VolumeDirName;
  EFI_FILE_PROTOCOL *Root;
} APFS_VOLUME_ROOT;

///
/// An array of file paths to search for in case no file is blessed.
///
STATIC CONST CHAR16 *mBootPathNames[] = {
  APPLE_BOOTER_DEFAULT_FILE_NAME,
  APPLE_REMOVABLE_MEDIA_FILE_NAME,
  EFI_REMOVABLE_MEDIA_FILE_NAME,
  APPLE_BOOTER_ROOT_FILE_NAME
};

EFI_STATUS
EFIAPI
BootPolicyGetBootFile (
  IN     EFI_HANDLE                Device,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

EFI_STATUS
EFIAPI
BootPolicyGetBootFileEx (
  IN  EFI_HANDLE                Device,
  IN  UINT32                    Unused OPTIONAL,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

EFI_STATUS
EFIAPI
BootPolicyGetBootInfo (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  );

EFI_STATUS
EFIAPI
BootPolicyGetPathNameOnApfsRecovery (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  OUT CHAR16                    **FullPathName,
  OUT VOID                      **Reserved,
  OUT EFI_FILE_PROTOCOL         **Root,
  OUT EFI_HANDLE                *DeviceHandle
  );

EFI_STATUS
EFIAPI
BootPolicyGetApfsRecoveryVolumes (
  IN  EFI_HANDLE  Handle,
  OUT VOID        **Volumes,
  OUT UINTN       *NumberOfEntries
  );

///
/// The APPLE_BOOT_POLICY_PROTOCOL instance to get installed.
///
STATIC APPLE_BOOT_POLICY_PROTOCOL mAppleBootPolicyProtocol = {
  APPLE_BOOT_POLICY_PROTOCOL_REVISION,
  BootPolicyGetBootFile,
  BootPolicyGetBootFileEx,
  BootPolicyGetBootInfo,
  BootPolicyGetPathNameOnApfsRecovery,
  BootPolicyGetApfsRecoveryVolumes
};

/**
  Checks whether the given file exists or not.

  @param[in] Root      The volume's opened root.
  @param[in] FileName  The path of the file to check.

  @return  Returned is whether the specified file exists or not.

**/
STATIC
BOOLEAN
InternalFileExists (
  IN EFI_FILE_HANDLE  Root,
  IN CONST CHAR16     *FileName
  )
{
  EFI_STATUS      Status;
  EFI_FILE_HANDLE FileHandle;

  Status = Root->Open (
                   Root,
                   &FileHandle,
                   (CHAR16 *) FileName,
                   EFI_FILE_MODE_READ,
                   0
                   );

  if (!EFI_ERROR (Status)) {
    FileHandle->Close (FileHandle);
    return TRUE;
  }

  return FALSE;
}

STATIC
EFI_STATUS
InternalGetApfsSpecialFileInfo (
  IN     EFI_FILE_PROTOCOL          *Root,
  IN OUT APPLE_APFS_VOLUME_INFO     **VolumeInfo OPTIONAL,
  IN OUT APPLE_APFS_CONTAINER_INFO  **ContainerInfo OPTIONAL
  )
{
  if (ContainerInfo == NULL && VolumeInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VolumeInfo != NULL) {
    *VolumeInfo = GetFileInfo (
      Root,
      &gAppleApfsVolumeInfoGuid,
      sizeof (*VolumeInfo),
      NULL
      );

    if (*VolumeInfo == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  if (ContainerInfo != NULL) {
    *ContainerInfo = GetFileInfo (
      Root,
      &gAppleApfsContainerInfoGuid,
      sizeof (*ContainerInfo),
      NULL
      );

    if (*ContainerInfo == NULL) {
      if (VolumeInfo != NULL) {
        FreePool (*VolumeInfo);
      }
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalGetBooterFromBlessedSystemFilePath (
  IN  EFI_FILE_PROTOCOL         *Root,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  UINTN  FilePathSize;

  *FilePath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
                Root,
                &gAppleBlessedSystemFileInfoGuid,
                sizeof (EFI_DEVICE_PATH_PROTOCOL),
                &FilePathSize
                );

  if (*FilePath == NULL) {
    return EFI_NOT_FOUND;
  }

  if (!IsDevicePathValid(*FilePath, FilePathSize)) {
    FreePool (*FilePath);
    *FilePath = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalGetBooterFromBlessedSystemFolderPath (
  IN  EFI_HANDLE                Device,
  IN  EFI_FILE_PROTOCOL         *Root,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  EFI_STATUS           Status;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  FILEPATH_DEVICE_PATH      *FolderDevicePath;
  UINTN                     DevicePathSize;
  UINTN                     BooterPathSize;
  CHAR16                    *BooterPath;

  Status = EFI_NOT_FOUND;

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
                  Root,
                  &gAppleBlessedSystemFolderInfoGuid,
                  sizeof (EFI_DEVICE_PATH_PROTOCOL),
                  &DevicePathSize
                  );

  if (DevicePath == NULL) {
    return Status;
  }

  if (!IsDevicePathValid(DevicePath, DevicePathSize)) {
    FreePool (DevicePath);
    return EFI_NOT_FOUND;
  }

  DevicePathWalker = DevicePath;

  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)) {

      FolderDevicePath  = (FILEPATH_DEVICE_PATH *) DevicePathWalker;
      BooterPathSize    = StrSize (&FolderDevicePath->PathName[0])
                          + StrSize (APPLE_BOOTER_ROOT_FILE_NAME) - sizeof (CHAR16);
      BooterPath        = AllocateZeroPool (BooterPathSize);

      if (BooterPath != NULL) {
        StrCpyS (BooterPath, BooterPathSize, &FolderDevicePath->PathName[0]);
        StrCatS (BooterPath, BooterPathSize, APPLE_BOOTER_ROOT_FILE_NAME);
        Status = EFI_SUCCESS;
      } else {
        Status = EFI_OUT_OF_RESOURCES;
      }

      break;
    }

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }

  FreePool (DevicePath);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!InternalFileExists (Root, BooterPath)) {
    return EFI_NOT_FOUND;
  }

  *FilePath = FileDevicePath (Device, BooterPath);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalGetBooterFromPredefinedNameList (
  IN     EFI_HANDLE                Device,
  IN     EFI_FILE_PROTOCOL         *Root,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath  OPTIONAL
  )
{
  UINTN         Index;
  CONST CHAR16  *PathName;

  for (Index = 0; Index < ARRAY_SIZE (mBootPathNames); ++Index) {
    PathName = mBootPathNames[Index];

    if (InternalFileExists (Root, PathName)) {
      if (DevicePath != NULL) {
        *DevicePath = FileDevicePath (Device, PathName);
      }
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
InternalGetApfsVolumeInfo (
  IN  EFI_HANDLE              Device,
  OUT EFI_GUID                *ContainerGuid,
  OUT EFI_GUID                *VolumeGuid,
  OUT APPLE_APFS_VOLUME_ROLE  *VolumeRole
  )
{
  EFI_STATUS                      Status;

  EFI_FILE_PROTOCOL               *Root;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  APPLE_APFS_CONTAINER_INFO       *ApfsContainerInfo;
  APPLE_APFS_VOLUME_INFO          *ApfsVolumeInfo;

  Root = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &FileSystem
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalGetApfsSpecialFileInfo (Root, &ApfsVolumeInfo, &ApfsContainerInfo);

  Root->Close (Root);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  CopyGuid (
    VolumeGuid,
    &ApfsVolumeInfo->Uuid
    );

  *VolumeRole = ApfsVolumeInfo->Role;

  CopyGuid (
    ContainerGuid,
    &ApfsContainerInfo->Uuid
    );

  FreePool (ApfsVolumeInfo);
  FreePool (ApfsContainerInfo);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalGetBooterFromApfsVolumePredefinedNameList (
  IN     EFI_HANDLE                      Device,
  IN     EFI_FILE_PROTOCOL               *PrebootRoot,
  IN     CHAR16                          *VolumeDirectoryName,
  IN OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath  OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_FILE_PROTOCOL         *VolumeDirectoryHandle;
  EFI_FILE_INFO             *VolumeDirectoryInfo;
  EFI_DEVICE_PATH_PROTOCOL  *BooterPath;

  Status = PrebootRoot->Open (
                     PrebootRoot,
                     &VolumeDirectoryHandle,
                     VolumeDirectoryName,
                     EFI_FILE_MODE_READ,
                     0
                     );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  VolumeDirectoryInfo = GetFileInfo (
                          VolumeDirectoryHandle,
                          &gEfiFileInfoGuid,
                          sizeof (*VolumeDirectoryInfo),
                          NULL
                          );

  if (VolumeDirectoryInfo == NULL) {
    VolumeDirectoryHandle->Close (VolumeDirectoryHandle);
    return EFI_NOT_FOUND;
  }

  Status = EFI_NOT_FOUND;

  if ((VolumeDirectoryInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
    Status = InternalGetBooterFromPredefinedNameList (
      Device,
      VolumeDirectoryHandle,
      DevicePath != NULL ? &BooterPath : NULL
      );
  }

  FreePool (VolumeDirectoryInfo);
  VolumeDirectoryHandle->Close (VolumeDirectoryHandle);

  if (EFI_ERROR (Status) || DevicePath == NULL) {
    return Status;
  }

  *DevicePath = AppendDevicePathInstance (*DevicePath, BooterPath);
  FreePool (BooterPath);

  return EFI_SUCCESS;
}

/**
  APFS boot happens from Preboot volume, which normally contains blessed file
  or folder. In case blessed path location fails, we iterate every volume
  within the container, and try to locate a predefined booter path on Preboot
  volume (e.g. Preboot://{VolumeUuid}/System/Library/CoreServices/boot.efi).
**/
STATIC
EFI_STATUS
InternalGetBooterFromApfsPredefinedNameList (
  IN  EFI_HANDLE                      Device,
  IN  EFI_FILE_PROTOCOL               *PrebootRoot,
  IN  CONST GUID                      *ContainerUuid,
  IN  CONST CHAR16                    *VolumeUuid,
  OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath  OPTIONAL,
  OUT EFI_HANDLE                      *VolumeHandle  OPTIONAL
  )
{
  EFI_STATUS                      Status;

  UINTN                           NumberOfHandles;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *HandleRoot;
  APPLE_APFS_CONTAINER_INFO       *ContainerInfo;
  APPLE_APFS_VOLUME_INFO          *VolumeInfo;
  CHAR16                          VolumeDirectoryName[GUID_STRING_LENGTH+1];

  Status =  gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleFileSystemProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < NumberOfHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **) &FileSystem
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = FileSystem->OpenVolume (FileSystem, &HandleRoot);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = InternalGetApfsSpecialFileInfo (HandleRoot, &VolumeInfo, &ContainerInfo);

    HandleRoot->Close (HandleRoot);

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = EFI_NOT_FOUND;

    if (!CompareGuid (&ContainerInfo->Uuid, ContainerUuid)) {
      FreePool (ContainerInfo);
      FreePool (VolumeInfo);
      continue;
    }

    UnicodeSPrint (
      VolumeDirectoryName,
      sizeof (VolumeDirectoryName),
      L"%g",
      &VolumeInfo->Uuid
      );

    FreePool (ContainerInfo);
    FreePool (VolumeInfo);

    if (VolumeUuid != NULL && StrStr (VolumeUuid, VolumeDirectoryName) != NULL) {
      *VolumeHandle = HandleBuffer[Index];
    }

    Status = InternalGetBooterFromApfsVolumePredefinedNameList (
      Device,
      PrebootRoot,
      VolumeDirectoryName,
      DevicePath
      );

    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (HandleBuffer);

  return Status;
}

STATIC
BOOLEAN
HasValidGuidStringPrefix (
  IN CONST CHAR16  *String
  )
{
  UINTN  Length;
  UINTN  Index;
  UINTN  GuidLength = GUID_STRING_LENGTH;

  Length = StrLen (String);
  if (Length < GuidLength) {
    return FALSE;
  }

  for (Index = 0; Index < GuidLength; ++Index) {
    if (Index == 8 || Index == 13 || Index == 18 || Index == 23) {
      if (String[Index] != '-') {
        return FALSE;
      }
    } else if (!(String[Index] >= L'0' && String[Index] <= L'9')
      && !(String[Index] >= L'A' && String[Index] <= L'F')
      && !(String[Index] >= L'a' && String[Index] <= L'f')) {
      return FALSE;
    }
  }

  return TRUE;
}

STATIC
EFI_STATUS
InternalGetBootPathName (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName
  )
{
  UINTN                           Size;
  CHAR16                          *PathName;
  UINTN                           PathNameSize;
  FILEPATH_DEVICE_PATH            *FilePath;
  CHAR16                          *Slash;
  UINTN                           Len;
  CHAR16                          *FilePathName;

  if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH)
   && (DevicePathSubType (DevicePath) == MEDIA_FILEPATH_DP)) {
    FilePath = (FILEPATH_DEVICE_PATH *) DevicePath;

    Size = DevicePathNodeLength (DevicePath);

    PathNameSize = Size - SIZE_OF_FILEPATH_DEVICE_PATH + sizeof (CHAR16);
    PathName = AllocateZeroPool (PathNameSize);

    if (PathName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (
      PathName,
      FilePath->PathName,
      Size - SIZE_OF_FILEPATH_DEVICE_PATH
      );

    Slash = StrStr (FilePath->PathName, L"\\");

    if (Slash != NULL) {
      Len = StrLen (PathName);

      FilePathName = &PathName[Len - 1];

      while (*FilePathName != L'\\') {
        *FilePathName = L'\0';
        --FilePathName;
      }
    } else {
      StrCpyS (PathName, PathNameSize, L"\\");
    }
  } else {
    PathName = AllocateZeroPool (sizeof (L"\\"));

    if (PathName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    StrCpyS (PathName, sizeof (L"\\"), L"\\");
  }

  *BootPathName = PathName;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalGetApfsVolumeHandle (
  IN  EFI_HANDLE                DeviceHandle,
  IN  CHAR16                    *PathName,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  )
{
  EFI_STATUS                       Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *Root;
  APPLE_APFS_CONTAINER_INFO        *ContainerInfo;
  CHAR16                           *FilePathName;

  FilePathName = &PathName[0];

  if (PathName[0] == L'\\') {
    FilePathName = &PathName[1];
  }

  if (!HasValidGuidStringPrefix (FilePathName)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &FileSystem
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalGetApfsSpecialFileInfo (Root, NULL, &ContainerInfo);
  if (!EFI_ERROR (Status)) {
    Status = InternalGetBooterFromApfsPredefinedNameList (
      DeviceHandle,
      Root,
      &ContainerInfo->Uuid,
      FilePathName,
      NULL,
      ApfsVolumeHandle
      );

    FreePool (ContainerInfo);
  }

  Root->Close (Root);

  return Status;
}

/**
  Locates the bootable file of the given volume.  Prefered are the values
  blessed, though if unavailable, hard-coded names are being verified and used
  if existing.

  The blessed paths are to be determined by the HFS Driver via
  EFI_FILE_PROTOCOL.GetInfo().  The related identifier definitions are to be
  found in AppleBless.h.

  @param[in]  Device    The Device's Handle to perform the search on.
  @param[out] FilePath  A pointer to the device path pointer to set to the file
                        path of the boot file.

  @return                       The status of the operation is returned.
  @retval EFI_NOT_FOUND         A bootable file could not be found on the given
                                volume.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_SUCCESS           The operation completed successfully and the
                                PathName Buffer has been filled.
  @retval other                 The status of an operation used to complete
                                this operation is returned.
**/
EFI_STATUS
EFIAPI
BootPolicyGetBootFile (
  IN     EFI_HANDLE                Device,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;

  *FilePath = NULL;
  Root = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &FileSystem
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalGetBooterFromBlessedSystemFilePath (Root, FilePath);
  if (EFI_ERROR (Status)) {
    Status = InternalGetBooterFromBlessedSystemFolderPath (Device, Root, FilePath);
    if (EFI_ERROR (Status)) {
      Status = InternalGetBooterFromPredefinedNameList (Device, Root, FilePath);
    }
  }

  Root->Close (Root);

  return Status;
}

EFI_STATUS
EFIAPI
BootPolicyGetBootFileEx (
  IN  EFI_HANDLE                      Device,
  IN  UINT32                          Mode,
  OUT EFI_DEVICE_PATH_PROTOCOL        **FilePath
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;
  APPLE_APFS_CONTAINER_INFO       *ContainerInfo;
  APPLE_APFS_VOLUME_INFO          *VolumeInfo;

  *FilePath = NULL;
  Root = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &FileSystem
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalGetApfsSpecialFileInfo (Root, &VolumeInfo, &ContainerInfo);
  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    if ((VolumeInfo->Role & APPLE_APFS_VOLUME_ROLE_PREBOOT) != 0) {
      Status = InternalGetBooterFromBlessedSystemFilePath (Root, FilePath);
      if (EFI_ERROR (Status)) {
        Status = InternalGetBooterFromBlessedSystemFolderPath (Device, Root, FilePath);
        if (EFI_ERROR (Status)) {
          Status = InternalGetBooterFromApfsPredefinedNameList (
                     Device,
                     Root,
                     &ContainerInfo->Uuid,
                     NULL,
                     FilePath,
                     NULL
                     );
        }
      }
    }

    FreePool (VolumeInfo);
    FreePool (ContainerInfo);
  } else {
    Status = InternalGetBooterFromBlessedSystemFilePath (Root, FilePath);
    if (EFI_ERROR (Status)) {
      Status = InternalGetBooterFromBlessedSystemFolderPath (Device, Root, FilePath);
      if (EFI_ERROR (Status)) {
        Status = InternalGetBooterFromPredefinedNameList (Device, Root, FilePath);
      }
    }
  }

  Root->Close (Root);

  return Status;
}

EFI_STATUS
EFIAPI
BootPolicyGetBootInfo (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  )
{
  EFI_STATUS                      Status;

  EFI_HANDLE                      DeviceHandle;
  CHAR16                          *PathName;

  *BootPathName = NULL;
  *Device       = NULL;
  *ApfsVolumeHandle = NULL;

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &DevicePath,
                  &DeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalGetBootPathName (DevicePath, &PathName);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Device = DeviceHandle;
  *BootPathName = PathName;

  InternalGetApfsVolumeHandle (DeviceHandle, PathName, ApfsVolumeHandle);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootPolicyGetPathNameOnApfsRecovery (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  OUT CHAR16                    **FullPathName,
  OUT VOID                      **Reserved,
  OUT EFI_FILE_PROTOCOL         **Root,
  OUT EFI_HANDLE                *DeviceHandle
  )
{
  EFI_STATUS                      Result;

  EFI_STATUS                      Status;

  CHAR16                          *BootPathName;
  EFI_HANDLE                      Device;
  EFI_HANDLE                      VolumeHandle;

  GUID                            ContainerGuid;
  GUID                            VolumeGuid;
  APPLE_APFS_VOLUME_ROLE          VolumeRole;

  GUID                            ContainerGuid2;
  GUID                            VolumeGuid2;
  APPLE_APFS_VOLUME_ROLE          VolumeRole2;

  UINTN                           NoHandles;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           Index;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

  CHAR16                          *FullPathBuffer;
  UINTN                           FullPathNameSize;

  EFI_FILE_PROTOCOL               *NewHandle;

  EFI_FILE_INFO                   *FileInfo;

  Result = EFI_INVALID_PARAMETER;

  NewHandle     = NULL;
  *Root         = NULL;
  *FullPathName = NULL;
  *Reserved     = NULL;

  if (PathName == NULL || DevicePath == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = BootPolicyGetBootInfo (
             DevicePath,
             &BootPathName,
             &Device,
             &VolumeHandle
             );

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  FreePool (BootPathName);

  if (VolumeHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = InternalGetApfsVolumeInfo (
             VolumeHandle,
             &ContainerGuid,
             &VolumeGuid,
             &VolumeRole
             );

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiSimpleFileSystemProtocolGuid,
            NULL,
            &NoHandles,
            &HandleBuffer
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Result = EFI_NOT_FOUND;

  for (Index = 0; Index < NoHandles; ++Index) {
    Status = InternalGetApfsVolumeInfo (
               HandleBuffer[Index],
               &ContainerGuid2,
               &VolumeGuid2,
               &VolumeRole2
               );
    if (EFI_ERROR (Status)
      || VolumeRole2 != APPLE_APFS_VOLUME_ROLE_RECOVERY
      || !CompareGuid (&ContainerGuid2, &ContainerGuid)) {
      continue;
    }

    Status = gBS->HandleProtocol (
              HandleBuffer[Index],
              &gEfiSimpleFileSystemProtocolGuid,
              (VOID **) &FileSystem
              );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = FileSystem->OpenVolume (FileSystem, Root);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FullPathNameSize  = sizeof (CHAR16) + StrSize (PathName);
    FullPathNameSize += GUID_STRING_LENGTH * sizeof (CHAR16);
    FullPathBuffer    = AllocateZeroPool (FullPathNameSize);

    if (FullPathBuffer == NULL) {
      (*Root)->Close (*Root);
      continue;
    }

    //
    // Note, this \\ prefixing does not exist in Apple code, and should not be required,
    // but we add it for return path consistency.
    //
    UnicodeSPrint (
      FullPathBuffer,
      FullPathNameSize,
      L"\\%g%s",
      &VolumeGuid,
      PathName
      );

    Status = (*Root)->Open (
                        *Root,
                        &NewHandle,
                        FullPathBuffer,
                        EFI_FILE_MODE_READ,
                        0
                        );

    if (EFI_ERROR (Status)) {
      (*Root)->Close (*Root);
      FreePool (FullPathBuffer);
      continue;
    }

    FileInfo = GetFileInfo (
                 NewHandle,
                 &gEfiFileInfoGuid,
                 sizeof (*FileInfo),
                 NULL
                 );

    if (FileInfo != NULL) {
      if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
        *FullPathName = FullPathBuffer;
        *DeviceHandle = HandleBuffer[Index];
        Result = EFI_SUCCESS;
      }

      FreePool (FileInfo);
    }

    NewHandle->Close (NewHandle);

    if (!EFI_ERROR (Result)) {
      break;
    }

    (*Root)->Close (*Root);
    FreePool (FullPathBuffer);
  }

  FreePool (HandleBuffer);

  return Result;
}

EFI_STATUS
EFIAPI
BootPolicyGetApfsRecoveryVolumes (
  IN  EFI_HANDLE  Handle,
  OUT VOID        **Volumes,
  OUT UINTN       *NumberOfEntries
  )
{
  EFI_STATUS                      Status;

  UINTN                           NumberOfHandles;
  EFI_HANDLE                      *HandleBuffer;
  APFS_VOLUME_INFO                *VolumeInfo;
  GUID                            *ContainerGuids;
  EFI_GUID                        ContainerGuid;
  UINTN                           NumberOfContainers;
  UINTN                           NumberOfVolumeInfos;
  UINTN                           Index;
  UINTN                           Index2;
  UINTN                           Index3;
  APFS_VOLUME_ROOT                **ApfsVolumes;
  BOOLEAN                         GuidPresent;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;
  EFI_FILE_PROTOCOL               *NewHandle;
  CHAR16                          VolumePathName[GUID_STRING_LENGTH + 1];
  EFI_FILE_INFO                   *FileInfo;
  APFS_VOLUME_ROOT                *ApfsRoot;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (NumberOfEntries > 0) {
    VolumeInfo = AllocateZeroPool (NumberOfHandles * sizeof (*VolumeInfo));
    ContainerGuids = AllocateZeroPool (NumberOfHandles * sizeof (*ContainerGuids));

    if (VolumeInfo == NULL || ContainerGuids == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  } else {
    Status = EFI_NOT_FOUND;
    VolumeInfo = NULL;
    ContainerGuids = NULL;
  }

  if (EFI_ERROR (Status)) {
    FreePool (HandleBuffer);

    if (VolumeInfo != NULL) {
      FreePool (VolumeInfo);
    }

    if (ContainerGuids != NULL) {
      FreePool (ContainerGuids);
    }

    return Status;
  }

  NumberOfVolumeInfos = 0;
  NumberOfContainers = 0;

  for (Index = 0; Index < NumberOfHandles; ++Index) {
    Status = InternalGetApfsVolumeInfo (
               HandleBuffer[Index],
               &VolumeInfo[NumberOfVolumeInfos].ContainerGuid,
               &VolumeInfo[NumberOfVolumeInfos].VolumeGuid,
               &VolumeInfo[NumberOfVolumeInfos].VolumeRole
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    VolumeInfo[NumberOfVolumeInfos].Handle     = HandleBuffer[Index];

    GuidPresent = FALSE;
    for (Index2 = 0; Index2 < NumberOfContainers; ++Index2) {
      if (CompareGuid (&ContainerGuids[Index2], &ContainerGuid)) {
        GuidPresent = TRUE;
        break;
      }
    }

    if (!GuidPresent) {
      CopyGuid (
        &ContainerGuids[NumberOfContainers],
        &VolumeInfo[NumberOfVolumeInfos].ContainerGuid
        );

      if (Index2 != 0 && HandleBuffer[Index] == Handle) {
        CopyMem (
          &ContainerGuids[1],
          &ContainerGuids[0],
          NumberOfContainers * sizeof (ContainerGuids[0])
          );
        CopyGuid (
          &ContainerGuids[0],
          &VolumeInfo[NumberOfVolumeInfos].ContainerGuid
          );
      }

      ++NumberOfContainers;
    }

    ++NumberOfVolumeInfos;
  }

  Status = EFI_SUCCESS;

  if (NumberOfVolumeInfos > 0) {
    ApfsVolumes = AllocateZeroPool (
                    NumberOfVolumeInfos * sizeof (*ApfsVolumes)
                    );
    if (ApfsVolumes == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    FreePool (HandleBuffer);
    FreePool (VolumeInfo);
    FreePool (ContainerGuids);
    return Status;
  }

  *Volumes         = ApfsVolumes;
  *NumberOfEntries = 0;

  for (Index = 0; Index < NumberOfContainers; ++Index) {
    for (Index2 = 0; Index2 < NumberOfVolumeInfos; ++Index2) {
      if ((VolumeInfo[Index2].VolumeRole & APPLE_APFS_VOLUME_ROLE_RECOVERY) == 0
        || !CompareGuid (&ContainerGuids[Index], &VolumeInfo[Index2].ContainerGuid)) {
        continue;
      }

      Status = gBS->HandleProtocol (
                      VolumeInfo[Index2].Handle,
                      &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **) &FileSystem
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = FileSystem->OpenVolume (FileSystem, &Root);
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Locate recovery for every volume present.
      //
      for (Index3 = 0; Index3 < NumberOfVolumeInfos; ++Index3) {
        if ((VolumeInfo[Index2].VolumeRole &
          (APPLE_APFS_VOLUME_ROLE_RECOVERY|APPLE_APFS_VOLUME_ROLE_PREBOOT)) != 0
          || !CompareGuid (&ContainerGuids[Index], &VolumeInfo[Index3].ContainerGuid)) {
          continue;
        }

        UnicodeSPrint (
          VolumePathName,
          sizeof (VolumePathName),
          L"%g",
          &VolumeInfo[Index3].VolumeGuid
          );

        Status = Root->Open (
                         Root,
                         &NewHandle,
                         VolumePathName,
                         EFI_FILE_MODE_READ,
                         0
                         );

        if (EFI_ERROR (Status)) {
          continue;
        }

        FileInfo = GetFileInfo (
                     NewHandle,
                     &gEfiFileInfoGuid,
                     sizeof (*FileInfo),
                     NULL
                     );

        NewHandle->Close (NewHandle);

        if (FileInfo != NULL) {
          if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
            ApfsRoot = ApfsVolumes[*NumberOfEntries];
            ApfsRoot->Handle = VolumeInfo[Index2].Handle;
            ApfsRoot->VolumeDirName = AllocateCopyPool (
                                        StrSize (VolumePathName),
                                        VolumePathName
                                        );
            ApfsRoot->Root = Root;

            ++(*NumberOfEntries);
          }
          FreePool (FileInfo);
        }
      }
    }
  }

  FreePool (VolumeInfo);
  FreePool (ContainerGuids);
  FreePool (HandleBuffer);

  if (!EFI_ERROR (Status) && *NumberOfEntries == 0) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/**
  The Entry Point installing the APPLE_BOOT_POLICY_PROTOCOL.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.

**/
EFI_STATUS
OcAppleBootPolicyInstallProtocol (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  VOID        *Interface;
  EFI_HANDLE  Handle;

  Status = gBS->LocateProtocol (
                  &gAppleBootPolicyProtocolGuid,
                  NULL,
                  &Interface
                  );

  if (EFI_ERROR (Status)) {
    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gAppleBootPolicyProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID **) &mAppleBootPolicyProtocol
                    );
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}
