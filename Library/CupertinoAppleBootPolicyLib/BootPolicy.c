#include <Uefi.h>

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CupertinoBlessLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MiscFileLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

///
/// An array of file paths to search for in case no file is blessed.
///
STATIC CONST CHAR16 *mBootPathNames[] = {
  APPLE_BOOTER_DEFAULT_FILE_NAME,
  APPLE_REMOVABLE_MEDIA_FILE_NAME,
  APPLE_BOOTER_ROOT_FILE_NAME
};

STATIC
CONST CHAR16 *
InternalGetBlessedSystemFolderPathName (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  CONST CHAR16               *PathName;

  VOID                       *DevicePath;
  CONST VOID                 *DevicePathWalker;
  CONST FILEPATH_DEVICE_PATH *FilePath;

  PathName = NULL;

  DevicePath = CupertinoGetBlessedSystemFolder (Root);

  if (DevicePath != NULL) {
    DevicePathWalker = DevicePath;

    while (!IsDevicePathEnd (DevicePathWalker)) {
      if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
       && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)) {
        FilePath = (CONST FILEPATH_DEVICE_PATH *)DevicePathWalker;
        PathName = AllocateCopyPool (
                     StrSize (&FilePath->PathName[0]),
                     &FilePath->PathName[0]
                     );

        break;
      }

      DevicePathWalker = NextDevicePathNode (DevicePathWalker);
    }

    FreePool (DevicePath);
  }

  return PathName;
}

STATIC
CONST CHAR16 *
InternalGetProbedBootPathName (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  CONST CHAR16 *BootPathName;

  UINTN        Index;
  CONST CHAR16 *PathName;

  BootPathName = NULL;

  for (Index = 0; Index < ARRAY_SIZE (mBootPathNames); ++Index) {
    PathName = mBootPathNames[Index];

    if (FileExists (Root, PathName)) {
      BootPathName = PathName;

      break;
  }
  }

  return BootPathName;
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
  CONST APPLE_APFS_CONTAINER_INFO *ApfsContainerInfo;
  CONST APPLE_APFS_VOLUME_INFO    *ApfsVolumeInfo;

  Root = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );

  if (!EFI_ERROR (Status)) {
    Status = FileSystem->OpenVolume (FileSystem, &Root);

    if (!EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;

      ApfsContainerInfo = MiscGetFileInformation (
                            Root,
                            &gAppleApfsContainerInfoGuid
                            );

      if (ApfsContainerInfo != NULL) {
        CopyGuid (
          ContainerGuid,
          &ApfsContainerInfo->Uuid
          );

        FreePool ((VOID *)ApfsContainerInfo);
        
        Status = EFI_SUCCESS;
      }

      ApfsVolumeInfo = MiscGetFileInformation (Root, &gAppleApfsVolumeInfoGuid);

      if (ApfsVolumeInfo != NULL) {
        CopyGuid (
          VolumeGuid,
          &ApfsVolumeInfo->Uuid
          );

        *VolumeRole = ApfsVolumeInfo->Role;

        FreePool ((VOID *)ApfsVolumeInfo);
      }

      Root->Close (Root);
    }
  }

  return Status;
}

STATIC
EFI_STATUS
InternalGetApfsBootFile (
  IN  EFI_HANDLE                Device,
  IN  EFI_FILE_PROTOCOL         *Root,
  IN  CONST GUID                *ContainerUuid,
  IN  CONST CHAR16              *VolumeUuid,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  OUT EFI_HANDLE                *VolumeHandle
  )
{
  EFI_STATUS                      Status;

  UINTN                           NumberOfHandles;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *HandleRoot;
  CONST APPLE_APFS_CONTAINER_INFO *ContainerInfo;
  BOOLEAN                         Equal;
  CONST APPLE_APFS_VOLUME_INFO    *VolumeInfo;
  CHAR16                          DirPathNameBuffer[38];
  EFI_FILE_PROTOCOL               *NewHandle;
  CONST EFI_FILE_INFO             *VolumeDirectoryInfo;
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

  Status = gBS->LocateHandleBuffer (
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
          ContainerInfo = MiscGetFileInformation (
                            HandleRoot,
                            &gAppleApfsContainerInfoGuid
                            );

          if (ContainerInfo != NULL) {
            Equal = CompareGuid (&ContainerInfo->Uuid, ContainerUuid);

            if (Equal) {
              VolumeInfo = MiscGetFileInformation (
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
                  VolumeDirectoryInfo = MiscGetFileInformation (
                                          NewHandle,
                                          &gEfiFileInfoGuid
                                          );

                  if (VolumeDirectoryInfo != NULL) {
                    if ((VolumeDirectoryInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
                      Status = EFI_NOT_FOUND;
                        
                      for (Index = 0; Index < ARRAY_SIZE (mBootPathNames); ++Index) {
                        FilePathName = mBootPathNames[Index];

                        DirectoryExists = FileExists (
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
                            if (DirPathNameBuffer[0] != L'\\') {
                              StrCpy (FullPathName, L"\\");
                            }

                            StrCat (FullPathName, &DirPathNameBuffer[0]);
                            StrCat (FullPathName, FilePathName);

                            FilePath = FileDevicePath (Device, FullPathName);

                            FreePool ((VOID *)FullPathName);

                            if (FilePath != NULL) {
                              Status = EFI_SUCCESS;

                              DevPathSize = GetDevicePathSize (FilePath);
                              DevPath     = *DevicePath;

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
                                  FreePool ((VOID *)DevPathWalker);

                                  break;
                                }

                                Result = CompareMem (
                                            (VOID *)DevPathWalker,
                                            (VOID *)FilePath,
                                            DevPathSize
                                            );

                                FreePool ((VOID *)DevPathWalker);
                              } while (Result != 0);
                            }
                          }

                          break;
                        }
                      }
                    }

                    FreePool ((VOID *)VolumeDirectoryInfo);
                  }
                }
              }

              if (VolumeInfo != NULL) {
                FreePool ((VOID *)VolumeInfo);
              }
            }

            FreePool ((VOID *)ContainerInfo);
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
EFI_STATUS
EFIAPI
InternalGetBootFileWorker (
  IN     EFI_HANDLE                  Device,
  IN     EFI_FILE_PROTOCOL           *Root,
  IN OUT CONST FILEPATH_DEVICE_PATH  **FilePath
  )
{
  EFI_STATUS                 Status;

  CONST FILEPATH_DEVICE_PATH *DevicePath;
  CONST CHAR16               *PathName;
  BOOLEAN                    FreePathName;

  Status = EFI_NOT_FOUND;

  DevicePath = CupertinoGetBlessedSystemFile (Root);

  if (DevicePath == NULL) {
    PathName = InternalGetBlessedSystemFolderPathName (Root);

    FreePathName = FALSE;

    if (PathName != NULL) {
      FreePathName = TRUE;
    } else {
      PathName = InternalGetProbedBootPathName (Root);
    }

    if (PathName != NULL) {
      DevicePath = (CONST FILEPATH_DEVICE_PATH *)(
                     FileDevicePath (Device, PathName)
                     );

      if (FreePathName) {
        FreePool ((VOID *)PathName);
      }
    }
  }

  *FilePath = DevicePath;

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
  IN     EFI_HANDLE                      Device,
  IN OUT CONST EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );

  if (!EFI_ERROR (Status)) {
    Status = FileSystem->OpenVolume (FileSystem, &Root);

    if (!EFI_ERROR (Status)) {
      Status = InternalGetBootFileWorker (Device, Root, FilePath);

      Root->Close (Root);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
BootPolicyGetBootFileEx (
  IN  EFI_HANDLE                Device,
  IN  UINT32                    Unused, OPTIONAL
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;
  APPLE_APFS_CONTAINER_INFO       *ContainerInfo;
  APPLE_APFS_VOLUME_INFO          *VolumeInfo;
  EFI_STATUS                      Status2;

  *FilePath = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );

  if (!EFI_ERROR (Status)) {
    Status = FileSystem->OpenVolume (FileSystem, &Root);

    if (!EFI_ERROR (Status)) {
      VolumeInfo = MiscGetFileInformation (Root, &gAppleApfsVolumeInfoGuid);

      if (VolumeInfo != NULL) {
        Status = EFI_NOT_FOUND;

        if ((VolumeInfo->Role & APPLE_APFS_VOLUME_ROLE_PREBOOT) != 0) {
          Status = InternalGetBootFileWorker (Device, Root, FilePath);

          if (EFI_ERROR (Status)) {
            ContainerInfo = MiscGetFileInformation (Root, &gAppleApfsContainerInfoGuid);

            if (ContainerInfo != NULL) {
              Status = InternalGetApfsBootFile (
                          Device,
                          Root,
                          &ContainerInfo->Uuid,
                          NULL,
                          FilePath,
                          NULL
                          );

              FreePool ((VOID *)ContainerInfo);
            }
          }
        }

        FreePool ((VOID *)VolumeInfo);
      } else {
        Status = InternalGetBootFileWorker (Device, Root, FilePath);
      }

      Root->Close (Root);
    }
  }

  return Status;
}