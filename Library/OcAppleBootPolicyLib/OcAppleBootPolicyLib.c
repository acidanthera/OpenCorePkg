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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define INTERNAL_GUID_STRING_LENGTH 36

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
VOID *
InternalGetFileInfo (
  IN EFI_FILE_PROTOCOL  *Root,
  IN EFI_GUID           *InformationType
  )
{
  VOID       *FileInfoBuffer;

  UINTN      FileInfoSize;
  EFI_STATUS Status;

  FileInfoSize   = 0;
  FileInfoBuffer = NULL;

  Status = Root->GetInfo (
                   Root,
                   InformationType,
                   &FileInfoSize,
                   NULL
                   );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileInfoBuffer = AllocateZeroPool (FileInfoSize);

    if (FileInfoBuffer != NULL) {
      Status = Root->GetInfo (
                       Root,
                       InformationType,
                       &FileInfoSize,
                       FileInfoBuffer
                       );

      if (EFI_ERROR (Status)) {
        FreePool (FileInfoBuffer);

        FileInfoBuffer = NULL;
      }
    }
  }

  return FileInfoBuffer;
}

STATIC
EFI_STATUS
InternalGetBooterFromBlessedSystemFilePath (
  IN  EFI_FILE_PROTOCOL         *Root,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  *FilePath = (EFI_DEVICE_PATH_PROTOCOL *) InternalGetFileInfo (
                Root,
                &gAppleBlessedSystemFileInfoGuid
                );

  if (*FilePath == NULL) {
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
  UINTN                     BooterPathSize;
  CHAR16                    *BooterPath;

  Status = EFI_NOT_FOUND;

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) InternalGetFileInfo (
                  Root,
                  &gAppleBlessedSystemFolderInfoGuid
                  );

  if (DevicePath == NULL) {
    return Status;
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
  IN  EFI_HANDLE                Device,
  IN  EFI_FILE_PROTOCOL         *Root,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  UINTN         Index;
  CONST CHAR16  *PathName;

  for (Index = 0; Index < ARRAY_SIZE (mBootPathNames); ++Index) {
    PathName = mBootPathNames[Index];

    if (InternalFileExists (Root, PathName)) {
      *DevicePath = FileDevicePath (Device, PathName);
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

  ApfsContainerInfo = InternalGetFileInfo (
                        Root,
                        &gAppleApfsContainerInfoGuid
                        );
  if (ApfsContainerInfo == NULL) {
    Root->Close (Root);
    return EFI_NOT_FOUND;
  }

  CopyGuid (
    ContainerGuid,
    &ApfsContainerInfo->Uuid
    );

  FreePool (ApfsContainerInfo);

  ApfsVolumeInfo = InternalGetFileInfo (Root, &gAppleApfsVolumeInfoGuid);
  if (ApfsVolumeInfo == NULL) {
    Root->Close (Root);
    return EFI_NOT_FOUND;
  }

  CopyGuid (
    VolumeGuid,
    &ApfsVolumeInfo->Uuid
    );

  *VolumeRole = ApfsVolumeInfo->Role;

  FreePool (ApfsVolumeInfo);

  Root->Close (Root);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalGetBooterFromApfsPreboot (
  IN  EFI_HANDLE                      Device,
  IN  EFI_FILE_PROTOCOL               *Root,
  IN  CONST GUID                      *ContainerUuid,
  IN  CONST CHAR16                    *VolumeUuid,
  OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath,
  OUT EFI_HANDLE                      *VolumeHandle
  )
{
  EFI_STATUS                      Status;

  UINTN                           NumberOfHandles;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *HandleRoot;
  APPLE_APFS_CONTAINER_INFO       *ContainerInfo;
  BOOLEAN                         Equal;
  APPLE_APFS_VOLUME_INFO          *VolumeInfo;
  CHAR16                          DirPathNameBuffer[38];
  EFI_FILE_PROTOCOL               *NewHandle;
  EFI_FILE_INFO                   *VolumeDirectoryInfo;
  CONST EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  CONST CHAR16                    *FilePathName;
  UINTN                           GuidPathNameSize;
  BOOLEAN                         DirectoryExists;
  UINTN                           BootFileNameSize;
  CHAR16                          *FullPathName;
  UINTN                           DevPathSize;
  EFI_DEVICE_PATH_PROTOCOL        *DevPath;
  EFI_DEVICE_PATH_PROTOCOL        *DevPathWalker;
  INTN                            Result;

  Status =  gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleFileSystemProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );

  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;

    for (Index = 0; Index < NumberOfHandles; ++Index) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **)&FileSystem
                      );

      if (!EFI_ERROR (Status)) {
        Status = FileSystem->OpenVolume (FileSystem, &HandleRoot);

        if (!EFI_ERROR (Status)) {
          ContainerInfo = InternalGetFileInfo (
                            HandleRoot,
                            &gAppleApfsContainerInfoGuid
                            );

          if (ContainerInfo != NULL) {
            Equal = CompareGuid (&ContainerInfo->Uuid, ContainerUuid);

            if (Equal) {
              VolumeInfo = InternalGetFileInfo (
                              HandleRoot,
                              &gAppleApfsVolumeInfoGuid
                              );

              if (VolumeInfo != NULL) {
                UnicodeSPrint (
                  &DirPathNameBuffer[0],
                  sizeof (DirPathNameBuffer),
                  L"%g",
                  &VolumeInfo->Uuid
                  );

                if ((VolumeUuid != NULL)
                 && StrStr (VolumeUuid, &DirPathNameBuffer[0]) != NULL) {
                  *VolumeHandle = HandleBuffer[Index];
                }

                // BUG: NewHandle is not closed.
                Status = Root->Open (
                                 Root,
                                 &NewHandle,
                                 &DirPathNameBuffer[0],
                                 EFI_FILE_MODE_READ,
                                 0
                                 );

                if (!EFI_ERROR (Status)) {
                  VolumeDirectoryInfo = InternalGetFileInfo (
                                          NewHandle,
                                          &gEfiFileInfoGuid
                                          );

                  if (VolumeDirectoryInfo != NULL) {
                    if ((VolumeDirectoryInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
                      Status = EFI_NOT_FOUND;

                      for (Index = 0; Index < ARRAY_SIZE (mBootPathNames); ++Index) {
                        FilePathName = mBootPathNames[Index];

                        DirectoryExists = InternalFileExists (
                                            NewHandle,
                                            (FilePathName[0] == L'\\')
                                              ? &FilePathName[1]
                                              : &FilePathName[0]
                                            );

                        if (DirectoryExists) {
                          GuidPathNameSize = (StrSize (&DirPathNameBuffer[0]) + 1);
                          BootFileNameSize = StrSize (FilePathName);

                          FullPathName = AllocateZeroPool (
                                           GuidPathNameSize + BootFileNameSize
                                           );

                          if (FullPathName != NULL) {
                            //
                            // BUG: FullPathName[0] cannot be \0 for just
                            //      having been allocated.  Likely this is
                            //      supposed to be checking DirPathNameBuffer
                            //      for the case it's not starting with '\'.
                            //
                            if (FullPathName[0] != L'\\') {
                              StrCpyS (FullPathName, GuidPathNameSize + BootFileNameSize, L"\\");
                            }

                            StrCatS (FullPathName, GuidPathNameSize + BootFileNameSize, &DirPathNameBuffer[0]);
                            StrCatS (FullPathName, GuidPathNameSize + BootFileNameSize, FilePathName);

                            FilePath = FileDevicePath (Device, FullPathName);

                            FreePool (FullPathName);

                            if (FilePath != NULL) {
                              Status = EFI_SUCCESS;

                              DevPathSize = GetDevicePathSize (FilePath);
                              DevPath = (EFI_DEVICE_PATH_PROTOCOL *)*DevicePath;

                              do {
                                DevPathWalker = GetNextDevicePathInstance (
                                                  &DevPath,
                                                  &DevPathSize
                                                  );

                                if (DevPathWalker == NULL) {
                                  *DevicePath = AppendDevicePath (
                                                  *DevicePath,
                                                  FilePath
                                                  );

                                  break;
                                }

                                if ((DevPathSize == 0)
                                 || (DevPathWalker == FilePath)) {
                                  FreePool (DevPathWalker);

                                  break;
                                }

                                Result = CompareMem (
                                            (VOID *)DevPathWalker,
                                            (VOID *)FilePath,
                                            DevPathSize
                                            );

                                FreePool (DevPathWalker);
                              } while (Result != 0);
                            }
                          }

                          break;
                        }
                      }
                    }

                    FreePool (VolumeDirectoryInfo);
                  }
                }
              }

              if (VolumeInfo != NULL) {
                FreePool (VolumeInfo);
              }
            }

            FreePool (ContainerInfo);
          }

          HandleRoot->Close (HandleRoot);
        }
      }
    }

    FreePool (HandleBuffer);
  }

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
  UINTN  GuidLength = INTERNAL_GUID_STRING_LENGTH;

  Length = StrLen (String);
  if (Length < GuidLength) {
    return FALSE;
  }

  for (Index = 0; Index < GuidLength; ++Index) {
    if ((Index == 8 || Index == 12 || Index == 16 || Index == 20)
      && String[Index] != '-') {
      return FALSE;
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
  EFI_DEVICE_PATH_PROTOCOL         *BootDevicePath;
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

  ContainerInfo = InternalGetFileInfo (
                    Root,
                    &gAppleApfsContainerInfoGuid
                    );

  if (ContainerInfo != NULL) {
    BootDevicePath = NULL;

    Status = InternalGetBooterFromApfsPreboot (
      DeviceHandle,
      Root,
      &ContainerInfo->Uuid,
      FilePathName,
      &BootDevicePath,
      ApfsVolumeHandle
      );

    if (BootDevicePath != NULL) {
      FreePool (BootDevicePath);
    }

    FreePool (ContainerInfo);
  } else {
    Status = EFI_NOT_FOUND;
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

  VolumeInfo = InternalGetFileInfo (Root, &gAppleApfsVolumeInfoGuid);
  if (VolumeInfo == NULL) {
    Root->Close (Root);
    return BootPolicyGetBootFile (Device, FilePath);
  }

  Status = EFI_NOT_FOUND;

  ContainerInfo = InternalGetFileInfo (Root, &gAppleApfsContainerInfoGuid);
  if (ContainerInfo != NULL) {
    if ((VolumeInfo->Role & APPLE_APFS_VOLUME_ROLE_PREBOOT) != 0) {
      Status = InternalGetBooterFromBlessedSystemFilePath (Root, FilePath);
      if (EFI_ERROR (Status)) {
        Status = InternalGetBooterFromBlessedSystemFolderPath (Device, Root, FilePath);
        if (EFI_ERROR (Status)) {
          Status = InternalGetBooterFromApfsPreboot (
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

    FreePool (ContainerInfo);
  }

  FreePool (VolumeInfo);
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

    FullPathNameSize  = StrSize (PathName);
    FullPathNameSize += INTERNAL_GUID_STRING_LENGTH * sizeof (CHAR16);
    FullPathBuffer    = AllocateZeroPool (FullPathNameSize);

    if (FullPathBuffer == NULL) {
      (*Root)->Close (*Root);
      continue;
    }

    UnicodeSPrint (
      FullPathBuffer,
      FullPathNameSize,
      L"%g%s",
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

    FileInfo = InternalGetFileInfo (
                 NewHandle,
                 &gEfiFileInfoGuid
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
    (*Root)->Close (*Root);

    if (!EFI_ERROR (Result)) {
      break;
    }

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
  GUID                            **ContainerGuids;
  GUID                            UnusedGuid; // BUG: This variable is never used.
  UINTN                           Index;
  EFI_GUID                        ContainerGuid;
  EFI_GUID                        VolumeGuid;
  APPLE_APFS_VOLUME_ROLE          VolumeRole;
  UINTN                           Index2;
  UINTN                           NumberOfContainers;
  UINTN                           NumberOfVolumeInfo;
  APFS_VOLUME_ROOT                **ApfsVolumes;
  UINTN                           Index3;
  UINTN                           Index4;
  BOOLEAN                         GuidPresent;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;
  EFI_FILE_PROTOCOL               *NewHandle;
  CHAR16                          String[40];
  EFI_FILE_INFO                   *FileInfo;
  APFS_VOLUME_ROOT                *ApfsRoot;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );

  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;

    if (NumberOfHandles > 0) {
      VolumeInfo = AllocateZeroPool (NumberOfHandles * sizeof (*VolumeInfo));

      Status = EFI_OUT_OF_RESOURCES;

      if (VolumeInfo != NULL) {
        // BUG: The previous allocation is verified, this isn't.
        ContainerGuids = AllocateZeroPool (
                           NumberOfHandles * sizeof (**ContainerGuids)
                           );

        ZeroMem ((VOID *)&UnusedGuid, sizeof (UnusedGuid));

        NumberOfVolumeInfo = 0;
        NumberOfContainers = 0;

        Status = EFI_NOT_FOUND;

        for (Index = 0; Index < NumberOfHandles; ++Index) {
          Status = InternalGetApfsVolumeInfo (
                     HandleBuffer[Index],
                     &ContainerGuid,
                     &VolumeGuid,
                     &VolumeRole
                     );

          if (!EFI_ERROR (Status)) {
            CopyGuid (
              &VolumeInfo[NumberOfVolumeInfo].ContainerGuid,
              &ContainerGuid
              );

            CopyGuid (
              &VolumeInfo[NumberOfVolumeInfo].VolumeGuid,
              &VolumeGuid
              );

            VolumeInfo[NumberOfVolumeInfo].VolumeRole = VolumeRole;
            VolumeInfo[NumberOfVolumeInfo].Handle     = HandleBuffer[Index];

            GuidPresent = FALSE;

            for (Index2 = 0; Index2 < NumberOfContainers; ++Index2) {
              if (CompareGuid (ContainerGuids[Index2], &ContainerGuid)) {
                GuidPresent = TRUE;
                break;
              }
            }

            if (!GuidPresent || (NumberOfContainers == 0)) {
              CopyGuid (
                ContainerGuids[NumberOfContainers],
                &VolumeInfo[NumberOfVolumeInfo].ContainerGuid
                );

              // BUG: This may overwrite the GUID at [0]?
              if ((Index2 != 0) && (HandleBuffer[Index] == Handle)) {
                CopyGuid (
                  ContainerGuids[0],
                  &VolumeInfo[NumberOfVolumeInfo].ContainerGuid
                  );
              }

              ++NumberOfContainers;
            }

            ++NumberOfVolumeInfo;
          }
        }

        if (NumberOfVolumeInfo != 0) {
          ApfsVolumes = AllocateZeroPool (
                          NumberOfVolumeInfo * sizeof (ApfsVolumes)
                          );

          Status = EFI_OUT_OF_RESOURCES;

          if (ApfsVolumes != NULL) {
            *Volumes         = ApfsVolumes;
            *NumberOfEntries = 0;

            for (Index2 = 0; Index2 < NumberOfContainers; ++Index2) {
              for (Index3 = 0; Index3 < NumberOfVolumeInfo; ++Index3) {
                if (CompareGuid (
                      ContainerGuids[Index2],
                      &VolumeInfo[Index3].ContainerGuid
                      )
                 && ((VolumeInfo[Index3].VolumeRole & APPLE_APFS_VOLUME_ROLE_RECOVERY) != 0)
                 ) {
                  Status = gBS->HandleProtocol (
                                  VolumeInfo[Index3].Handle,
                                  &gEfiSimpleFileSystemProtocolGuid,
                                  (VOID **)&FileSystem
                                  );

                  if (!EFI_ERROR (Status)) {
                    Status = FileSystem->OpenVolume (FileSystem, &Root);

                    if (!EFI_ERROR (Status)) {
                      for (Index4 = 0; Index4 < NumberOfVolumeInfo; ++Index4) {
                        if (
                          CompareGuid (
                            ContainerGuids[Index2],
                            &VolumeInfo[Index4].ContainerGuid
                            )
                          ) {
                          UnicodeSPrint (
                            &String[0],
                            sizeof (String),
                            L"%g",
                            &VolumeInfo[Index4].VolumeGuid
                            );

                          Status = Root->Open (
                                           Root,
                                           &NewHandle,
                                           String,
                                           EFI_FILE_MODE_READ,
                                           0
                                           );

                          if (!EFI_ERROR (Status)) {
                            FileInfo = InternalGetFileInfo (
                                         NewHandle,
                                         &gEfiFileInfoGuid
                                         );

                            if ((FileInfo != NULL)
                             && ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0)
                             ) {
                              ApfsRoot = ApfsVolumes[*NumberOfEntries];

                              ApfsRoot->Handle = VolumeInfo[Index3].Handle;

                              ApfsRoot->VolumeDirName = AllocateCopyPool (
                                                          StrSize (String),
                                                          (VOID *)&String[0]
                                                          );

                              ApfsRoot->Root = Root;

                              ++(*NumberOfEntries);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            FreePool (VolumeInfo);
            FreePool (ContainerGuids);
            FreePool (HandleBuffer);
          }
        }
      }
    }
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
EFIAPI
OcAppleBootPolicyInstallProtocol (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  VOID       *Interface;
  EFI_HANDLE Handle;

  Status = gBS->LocateProtocol (
                  &gAppleBootPolicyProtocolGuid,
                  NULL,
                  &Interface
                  );

  if (EFI_ERROR (Status)) {
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gAppleBootPolicyProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID **)&mAppleBootPolicyProtocol
                    );
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}
