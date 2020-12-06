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
#include <MicrosoftWindows.h>

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/AppleBootPolicy.h>

#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
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
/// On Apple Macs this list includes:
/// 1. APPLE_BOOTER_DEFAULT_FILE_NAME  -- \System\Library\CoreServices\boot.efi
/// 2. APPLE_REMOVABLE_MEDIA_FILE_NAME -- \EFI\APPLE\X64\BOOT.EFI
/// 3. EFI_REMOVABLE_MEDIA_FILE_NAME   -- \EFI\BOOT\BOOTX64.EFI
/// 4. APPLE_BOOTER_ROOT_FILE_NAME     -- \boot.efi
///
/// Since in real world only 1st and 3rd entries are used, we do not include
/// 2nd and 4th until further notice. However, we do include a custom entry
/// for Windows, for the reason OpenCore, unlike Apple EFI, may override
/// BOOTx64.efi, and this is not the case for Apple EFI. So we end up with:
/// 1. APPLE_BOOTER_DEFAULT_FILE_NAME  -- \System\Library\CoreServices\boot.efi
/// 2. MS_BOOTER_DEFAULT_FILE_NAME     -- \EFI\Microsoft\Boot\bootmgfw.efi
/// 3. EFI_REMOVABLE_MEDIA_FILE_NAME   -- \EFI\BOOT\BOOTX64.EFI
///
/// This resolves a problem when Windows installer does not replace our BOOTx64.efi
/// file with Windows file and then NVRAM reset or Boot Camp software reboot to macOS
/// results in the removal of the Windows boot entry from NVRAM making Windows
/// disappear from the list of OpenCore entries without BlessOverride.
///
/// Linux and related entries are not present here, because they have fine working
/// software for boot management and do not use BOOTx64.efi in the first place.
///
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR16 *gAppleBootPolicyPredefinedPaths[] = {
  APPLE_BOOTER_DEFAULT_FILE_NAME,
  MS_BOOTER_DEFAULT_FILE_NAME,
  EFI_REMOVABLE_MEDIA_FILE_NAME
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN gAppleBootPolicyNumPredefinedPaths =
  ARRAY_SIZE (gAppleBootPolicyPredefinedPaths);

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN gAppleBootPolicyCoreNumPredefinedPaths = 2;

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
  IN  BOOT_POLICY_ACTION        Action,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

EFI_STATUS
EFIAPI
BootPolicyDevicePathToDirPath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  );

EFI_STATUS
EFIAPI
BootPolicyGetApfsRecoveryFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  OUT CHAR16                    **FullPathName,
  OUT VOID                      **Reserved,
  OUT EFI_FILE_PROTOCOL         **Root,
  OUT EFI_HANDLE                *DeviceHandle
  );

EFI_STATUS
EFIAPI
BootPolicyGetAllApfsRecoveryFilePath (
  IN  EFI_HANDLE  Handle OPTIONAL,
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
  BootPolicyDevicePathToDirPath,
  BootPolicyGetApfsRecoveryFilePath,
  BootPolicyGetAllApfsRecoveryFilePath
};

/**
  Checks whether the given file exists or not.

  @param[in] Root      The volume's opened root.
  @param[in] FileName  The path of the file to check.

  @return  Returned is whether the specified file exists or not.

**/
STATIC
EFI_STATUS
InternalFileExists (
  IN EFI_FILE_HANDLE  Root,
  IN CONST CHAR16     *FileName
  )
{
  EFI_STATUS      Status;
  EFI_FILE_HANDLE FileHandle;

  Status = SafeFileOpen (
    Root,
    &FileHandle,
    FileName,
    EFI_FILE_MODE_READ,
    0
    );

  if (!EFI_ERROR (Status)) {
    FileHandle->Close (FileHandle);
    return EFI_SUCCESS;
  }

  return Status;
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
      sizeof (**VolumeInfo),
      NULL
      );

    if (*VolumeInfo == NULL) {
      DEBUG ((DEBUG_VERBOSE, "OCBP: APFS Volume Info is missing\n"));
      return EFI_NOT_FOUND;
    }

    DEBUG ((
      DEBUG_BULK_INFO,
      "OCBP: APFS Volume Info - %p (%u, %g, %u)\n",
      *VolumeInfo,
      (*VolumeInfo)->Always1,
      &(*VolumeInfo)->Uuid,
      (*VolumeInfo)->Role
      ));
  }

  if (ContainerInfo != NULL) {
    *ContainerInfo = GetFileInfo (
      Root,
      &gAppleApfsContainerInfoGuid,
      sizeof (**ContainerInfo),
      NULL
      );

    if (*ContainerInfo == NULL) {
      DEBUG ((DEBUG_BULK_INFO, "OCBP: APFS Container Info is missing\n"));
      if (VolumeInfo != NULL) {
        FreePool (*VolumeInfo);
      }
      return EFI_NOT_FOUND;
    }

    DEBUG ((
      DEBUG_BULK_INFO,
      "OCBP: APFS Container Info - %p (%u, %g)\n",
      *ContainerInfo,
      (*ContainerInfo)->Always1,
      &(*ContainerInfo)->Uuid
      ));
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
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed file is missing\n"));
    return EFI_NOT_FOUND;
  }

  DebugPrintHexDump (DEBUG_BULK_INFO, "OCBP: BlessedFileHEX", (UINT8 *) *FilePath, FilePathSize);

  if (!IsDevicePathValid (*FilePath, FilePathSize)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed file is invalid\n"));
    FreePool (*FilePath);
    *FilePath = NULL;
    return EFI_NOT_FOUND;
  }

  DebugPrintDevicePath (DEBUG_BULK_INFO, "OCBP: BlessedFileDP", *FilePath);

  DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed file is valid\n"));

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
  EFI_STATUS                Status;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  FILEPATH_DEVICE_PATH      *FolderDevicePath;
  UINTN                     DevicePathSize;
  UINTN                     BooterDirPathSize;
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
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed folder is missing\n"));
    return Status;
  }

  DebugPrintHexDump (DEBUG_BULK_INFO, "OCBP: BlessedFolderHEX", (UINT8 *) DevicePath, DevicePathSize);

  if (!IsDevicePathValid (DevicePath, DevicePathSize)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed folder is invalid\n"));
    FreePool (DevicePath);
    return EFI_NOT_FOUND;
  }

  DebugPrintDevicePath (DEBUG_BULK_INFO, "OCBP: BlessedFolderDP", DevicePath);

  DevicePathWalker = DevicePath;

  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)) {

      FolderDevicePath  = (FILEPATH_DEVICE_PATH *) DevicePathWalker;
      BooterDirPathSize = OcFileDevicePathNameSize (FolderDevicePath);
      BooterPathSize    = BooterDirPathSize + L_STR_SIZE (APPLE_BOOTER_ROOT_FILE_NAME) - sizeof (CHAR16);
      BooterPath        = AllocateZeroPool (BooterPathSize);

      if (BooterPath != NULL) {
        //
        // FolderDevicePath->PathName may be unaligned, thus byte copying is needed.
        //
        CopyMem (BooterPath, FolderDevicePath->PathName, BooterDirPathSize);
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
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed folder append failed - %r\n", Status));
    return Status;
  }

  Status = InternalFileExists (Root, BooterPath);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Blessed folder %s is missing - %r\n", BooterPath, Status));
    return EFI_NOT_FOUND;
  }

  *FilePath = FileDevicePath (Device, BooterPath);
  return EFI_SUCCESS;
}

EFI_STATUS
OcGetBooterFromPredefinedPathList (
  IN  EFI_HANDLE                Device,
  IN  EFI_FILE_PROTOCOL         *Root,
  IN  CONST CHAR16              **PredefinedPaths,
  IN  UINTN                     NumPredefinedPaths,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath  OPTIONAL,
  IN  CHAR16                    *Prefix       OPTIONAL
  )
{
  UINTN         Index;
  UINTN         FullPathSize;
  CHAR16        *FullPath;
  CONST CHAR16  *PathName;
  EFI_STATUS    Status;

  for (Index = 0; Index < NumPredefinedPaths; ++Index) {
    PathName = PredefinedPaths[Index];

    //
    // For relative paths (i.e. when Prefix is a volume GUID) we must
    // not use leading slash. This is what AppleBootPolicy does.
    //
    if (Prefix != NULL) {
      ASSERT (PathName[0] == L'\\');
    }
    Status = InternalFileExists (
      Root,
      Prefix != NULL ? &PathName[1] : &PathName[0]
      );
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_BULK_INFO,
        "OCBP: Predefined %s %s was found\n",
        Prefix != NULL ? Prefix : L"<nil>",
        PathName
        ));
      if (DevicePath != NULL) {
        //
        // Append volume directory prefix if any.
        //
        if (Prefix != NULL) {
          ASSERT (Prefix[0] != L'\\');
          FullPathSize = StrSize (Prefix) + StrSize (PathName);
          FullPath     = AllocatePool (FullPathSize);
          if (FullPath != NULL) {
            UnicodeSPrint (FullPath, FullPathSize, L"\\%s%s", Prefix, PathName);
            *DevicePath = FileDevicePath (Device, FullPath);
            FreePool (FullPath);
          } else {
            return EFI_OUT_OF_RESOURCES;
          }
        } else {
          *DevicePath = FileDevicePath (Device, PathName);
        }
      }
      ASSERT (DevicePath == NULL || *DevicePath != NULL);
      return EFI_SUCCESS;
    } else {
      DEBUG ((
        DEBUG_BULK_INFO,
        "OCBP: Predefined %s %s is missing - %r\n",
        Prefix != NULL ? Prefix : L"<nil>",
        PathName,
        Status
        ));
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
InternalGetBooterFromApfsVolumePredefinedPathList (
  IN     EFI_HANDLE                      Device,
  IN     EFI_FILE_PROTOCOL               *PrebootRoot,
  IN     CHAR16                          *VolumeDirectoryName,
  IN     CONST CHAR16                    **PredefinedPaths,
  IN     UINTN                           NumPredefinedPaths,
  IN OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath  OPTIONAL
  )
{
  EFI_STATUS        Status;
  EFI_FILE_PROTOCOL *VolumeDirectoryHandle;
  EFI_FILE_INFO     *VolumeDirectoryInfo;

  Status = SafeFileOpen (
    PrebootRoot,
    &VolumeDirectoryHandle,
    VolumeDirectoryName,
    EFI_FILE_MODE_READ,
    0
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Missing partition %s on preboot - %r\n", VolumeDirectoryName, Status));
    return Status;
  } else {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Found partition %s on preboot\n", VolumeDirectoryName));
  }

  VolumeDirectoryInfo = GetFileInfo (
                          VolumeDirectoryHandle,
                          &gEfiFileInfoGuid,
                          sizeof (*VolumeDirectoryInfo),
                          NULL
                          );

  if (VolumeDirectoryInfo == NULL) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Missing volume file info %s - %r\n", VolumeDirectoryName, Status));
    VolumeDirectoryHandle->Close (VolumeDirectoryHandle);
    return EFI_NOT_FOUND;
  }

  Status = EFI_NOT_FOUND;

  DEBUG ((
    DEBUG_BULK_INFO,
    "OCBP: Want predefined list for APFS %u at %s\n",
    VolumeDirectoryInfo->Attribute,
    VolumeDirectoryName
    ));

  if ((VolumeDirectoryInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
    Status = OcGetBooterFromPredefinedPathList (
      Device,
      VolumeDirectoryHandle,
      PredefinedPaths,
      NumPredefinedPaths,
      DevicePath,
      VolumeDirectoryName
      );
  }

  FreePool (VolumeDirectoryInfo);
  VolumeDirectoryHandle->Close (VolumeDirectoryHandle);

  return Status;
}

/**
  APFS boot happens from Preboot volume, which normally contains blessed file
  or folder. In case blessed path location fails, we iterate every volume
  within the container, and try to locate a predefined booter path on Preboot
  volume (e.g. Preboot://{VolumeUuid}/System/Library/CoreServices/boot.efi).
**/
STATIC
EFI_STATUS
InternalGetBooterFromApfsPredefinedPathList (
  IN  EFI_HANDLE                      Device,
  IN  EFI_FILE_PROTOCOL               *PrebootRoot,
  IN  CONST GUID                      *ContainerUuid,
  IN  CONST CHAR16                    *VolumeUuid  OPTIONAL,
  IN  CONST CHAR16                    **PredefinedPaths,
  IN  UINTN                           NumPredefinedPaths,
  OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath  OPTIONAL,
  OUT EFI_HANDLE                      *VolumeHandle  OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      TmpStatus;

  UINTN                           NumberOfHandles;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *HandleRoot;
  APPLE_APFS_CONTAINER_INFO       *ContainerInfo;
  APPLE_APFS_VOLUME_INFO          *VolumeInfo;
  CHAR16                          VolumeDirectoryName[GUID_STRING_LENGTH+1];
  EFI_DEVICE_PATH_PROTOCOL        *VolumeDevPath;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevPath;
  BOOLEAN                         ContainerMatch;

  ASSERT (VolumeUuid == NULL || (VolumeUuid != NULL && VolumeHandle != NULL && *VolumeHandle == NULL));

  NumberOfHandles = 0;
  Status =  gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleFileSystemProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );

  DEBUG ((DEBUG_BULK_INFO, "OCBP: %u filesystems for APFS - %r\n", (UINT32) NumberOfHandles, Status));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < NumberOfHandles; ++Index) {
    TmpStatus = gBS->HandleProtocol (
                       HandleBuffer[Index],
                       &gEfiSimpleFileSystemProtocolGuid,
                       (VOID **) &FileSystem
                       );
    if (EFI_ERROR (TmpStatus)) {
      DEBUG ((
        DEBUG_BULK_INFO,
        "OCBP: Borked APFS filesystem %u of %u - %r\n",
        (UINT32) (Index + 1),
        (UINT32) NumberOfHandles,
        TmpStatus
        ));
      continue;
    }

    TmpStatus = FileSystem->OpenVolume (FileSystem, &HandleRoot);
    if (EFI_ERROR (TmpStatus)) {
      DEBUG ((
        DEBUG_BULK_INFO,
        "OCBP: Borked APFS root volume %u of %u - %r\n",
        (UINT32) (Index + 1),
        (UINT32) NumberOfHandles,
        TmpStatus
        ));
      continue;
    }

    TmpStatus = InternalGetApfsSpecialFileInfo (HandleRoot, &VolumeInfo, &ContainerInfo);

    HandleRoot->Close (HandleRoot);

    if (EFI_ERROR (TmpStatus)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "OCBP: No APFS info %u of %u - %r\n",
        (UINT32) (Index + 1),
        (UINT32) NumberOfHandles,
        TmpStatus
        ));
      continue;
    }

    ContainerMatch = CompareGuid (&ContainerInfo->Uuid, ContainerUuid);

    DEBUG ((
      DEBUG_BULK_INFO,
      "OCBP: APFS match container %g vs %g for %u of %u - %d\n",
      &ContainerInfo->Uuid,
      ContainerUuid,
      (UINT32) (Index + 1),
      (UINT32) NumberOfHandles,
      ContainerMatch
      ));

    if (!ContainerMatch) {
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

    TmpStatus = InternalGetBooterFromApfsVolumePredefinedPathList (
      Device,
      PrebootRoot,
      VolumeDirectoryName,
      PredefinedPaths,
      NumPredefinedPaths,
      DevicePath != NULL ? &VolumeDevPath : NULL
      );

    if (EFI_ERROR (TmpStatus)) {
      DEBUG ((
        DEBUG_BULK_INFO,
        "OCBP: No APFS booter %u of %u for %s - %r\n",
        (UINT32) (Index + 1),
        (UINT32) NumberOfHandles,
        VolumeDirectoryName,
        TmpStatus
        ));
    } else {
      Status = EFI_SUCCESS;

      DEBUG ((
        DEBUG_BULK_INFO,
        "OCBP: Found APFS booter %u of %u for %s (%p)\n",
        (UINT32) (Index + 1),
        (UINT32) NumberOfHandles,
        VolumeDirectoryName,
        DevicePath
        ));

      if (DevicePath != NULL) {
        TempDevPath = *DevicePath;
        *DevicePath = OcAppendDevicePathInstanceDedupe (
                        TempDevPath,
                        VolumeDevPath
                        );
        if (TempDevPath != NULL) {
          FreePool (TempDevPath);
        }
      }
    }
  }

  FreePool (HandleBuffer);

  DEBUG ((
    DEBUG_BULK_INFO,
    "OCBP: APFS bless for %g:%s is %r\n",
    ContainerUuid,
    VolumeUuid,
    Status
    ));

  return Status;
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

    Size = OcFileDevicePathNameSize (FilePath);

    PathNameSize = Size + sizeof (CHAR16);
    PathName = AllocateZeroPool (PathNameSize);

    if (PathName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (PathName, FilePath->PathName, Size);

    Slash = StrStr (PathName, L"\\");

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
  IN  CONST CHAR16              **PredefinedPaths,
  IN  UINTN                     NumPredefinedPaths,
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
    //
    // FIXME: ApfsVolumeHandle is only returned when a predefined path exists
    //
    Status = InternalGetBooterFromApfsPredefinedPathList (
      DeviceHandle,
      Root,
      &ContainerInfo->Uuid,
      FilePathName,
      PredefinedPaths,
      NumPredefinedPaths,
      NULL,
      ApfsVolumeHandle
      );

    FreePool (ContainerInfo);
  }

  Root->Close (Root);

  return Status;
}

EFI_STATUS
OcBootPolicyGetBootFile (
  IN     EFI_HANDLE                Device,
  IN  CONST CHAR16                 **PredefinedPaths,
  IN  UINTN                        NumPredefinedPaths,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;

  ASSERT (Device != NULL);
  ASSERT (FilePath != NULL);

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
      Status = OcGetBooterFromPredefinedPathList (
                 Device,
                 Root,
                 PredefinedPaths,
                 NumPredefinedPaths,
                 FilePath,
                 NULL
                 );
    }
  }

  Root->Close (Root);

  return Status;
}

EFI_STATUS
EFIAPI
BootPolicyGetBootFile (
  IN     EFI_HANDLE                Device,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  if (Device == NULL || FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return OcBootPolicyGetBootFile (
    Device,
    gAppleBootPolicyPredefinedPaths,
    ARRAY_SIZE (gAppleBootPolicyPredefinedPaths),
    FilePath
    );
}

EFI_STATUS
OcBootPolicyGetBootFileEx (
  IN  EFI_HANDLE                      Device,
  IN  CONST CHAR16                    **PredefinedPaths,
  IN  UINTN                           NumPredefinedPaths,
  OUT EFI_DEVICE_PATH_PROTOCOL        **FilePath
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      TmpStatus;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;
  APPLE_APFS_CONTAINER_INFO       *ContainerInfo;
  APPLE_APFS_VOLUME_INFO          *VolumeInfo;

  ASSERT (Device != NULL);
  ASSERT (FilePath != NULL);

  *FilePath = NULL;
  Root = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &FileSystem
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Missing filesystem - %r\n", Status));
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: Invalid root volume - %r\n", Status));
    return Status;
  }

  Status = InternalGetApfsSpecialFileInfo (Root, &VolumeInfo, &ContainerInfo);
  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    if ((VolumeInfo->Role & APPLE_APFS_VOLUME_ROLE_PREBOOT) != 0) {
      TmpStatus = InternalGetBooterFromBlessedSystemFilePath (Root, FilePath);
      if (EFI_ERROR (TmpStatus)) {
        TmpStatus = InternalGetBooterFromBlessedSystemFolderPath (Device, Root, FilePath);
      }

      //
      // Blessed entry is always first, and subsequent entries are added with deduplication.
      //
      Status = InternalGetBooterFromApfsPredefinedPathList (
                 Device,
                 Root,
                 &ContainerInfo->Uuid,
                 NULL,
                 PredefinedPaths,
                 NumPredefinedPaths,
                 FilePath,
                 NULL
                 );
      if (!EFI_ERROR (TmpStatus)) {
        Status = TmpStatus;
      }
    }

    FreePool (VolumeInfo);
    FreePool (ContainerInfo);
  } else {
    Status = InternalGetBooterFromBlessedSystemFilePath (Root, FilePath);
    if (EFI_ERROR (Status)) {
      Status = InternalGetBooterFromBlessedSystemFolderPath (Device, Root, FilePath);
      if (EFI_ERROR (Status)) {
        Status = OcGetBooterFromPredefinedPathList (
                   Device,
                   Root,
                   PredefinedPaths,
                   NumPredefinedPaths,
                   FilePath,
                   NULL
                   );
      }
    }
  }

  Root->Close (Root);

  return Status;
}

EFI_STATUS
EFIAPI
BootPolicyGetBootFileEx (
  IN  EFI_HANDLE                      Device,
  IN  BOOT_POLICY_ACTION              Action,
  OUT EFI_DEVICE_PATH_PROTOCOL        **FilePath
  )
{
  if (Device == NULL || FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return OcBootPolicyGetBootFileEx (
    Device,
    gAppleBootPolicyPredefinedPaths,
    ARRAY_SIZE (gAppleBootPolicyPredefinedPaths),
    FilePath
    );
}

EFI_STATUS
OcBootPolicyDevicePathToDirPath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device
  )
{
  EFI_STATUS Status;

  ASSERT (DevicePath != NULL);
  ASSERT (BootPathName != NULL);
  ASSERT (Device != NULL);

  *BootPathName     = NULL;
  *Device           = NULL;

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &DevicePath,
                  Device
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalGetBootPathName (DevicePath, BootPathName);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
OcBootPolicyDevicePathToDirPathAndApfsHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              **PredefinedPaths,
  IN  UINTN                     NumPredefinedPaths,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  )
{
  EFI_STATUS Status;

  ASSERT (DevicePath != NULL);
  ASSERT (BootPathName != NULL);
  ASSERT (Device != NULL);
  ASSERT (ApfsVolumeHandle != NULL);

  Status = OcBootPolicyDevicePathToDirPath (
    DevicePath,
    BootPathName,
    Device
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // InternalGetApfsVolumeHandle status code is ignored, as ApfsVolumeHandle
  // may not exist.
  //
  *ApfsVolumeHandle = NULL;
  (VOID) InternalGetApfsVolumeHandle (
    *Device,
    *BootPathName,
    PredefinedPaths,
    NumPredefinedPaths,
    ApfsVolumeHandle
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootPolicyDevicePathToDirPath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  )
{
  EFI_STATUS Status;

  if (DevicePath       == NULL
   || BootPathName     == NULL
   || Device           == NULL
   || ApfsVolumeHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OcBootPolicyDevicePathToDirPathAndApfsHandle (
    DevicePath,
    gAppleBootPolicyPredefinedPaths,
    ARRAY_SIZE (gAppleBootPolicyPredefinedPaths),
    BootPathName,
    Device,
    ApfsVolumeHandle
    );
  if (EFI_ERROR (Status)) {
    *BootPathName     = NULL;
    *Device           = NULL;
    *ApfsVolumeHandle = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcBootPolicyGetApfsRecoveryFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  IN  CONST CHAR16              **PredefinedPaths,
  IN  UINTN                     NumPredefinedPaths,
  OUT CHAR16                    **FullPathName,
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

  ASSERT (DevicePath != NULL);
  ASSERT (PathName != NULL);
  ASSERT (FullPathName != NULL);
  ASSERT (Root != NULL);
  ASSERT (DeviceHandle != NULL);

  NewHandle     = NULL;
  *Root         = NULL;
  *FullPathName = NULL;

  Status = OcBootPolicyDevicePathToDirPathAndApfsHandle (
             DevicePath,
             PredefinedPaths,
             NumPredefinedPaths,
             &BootPathName,
             &Device,
             &VolumeHandle
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: APFS recovery boot info failed - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  if (VolumeHandle == NULL) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: APFS recovery volume handle missing - %s\n", BootPathName));
    FreePool (BootPathName);
    return EFI_NOT_FOUND;
  }

  FreePool (BootPathName);

  Status = InternalGetApfsVolumeInfo (
             VolumeHandle,
             &ContainerGuid,
             &VolumeGuid,
             &VolumeRole
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "OCBP: APFS recovery volume info missing\n"));
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
    DEBUG ((DEBUG_BULK_INFO, "OCBP: APFS recovery simple fs missing - %r\n", Status));
    return Status;
  }

  Result = EFI_NOT_FOUND;

  for (Index = 0; Index < NoHandles; ++Index) {
    ZeroMem (&ContainerGuid2, sizeof (ContainerGuid2));
    ZeroMem (&VolumeGuid2, sizeof (VolumeGuid2));
    VolumeRole2 = 0;

    Status = InternalGetApfsVolumeInfo (
               HandleBuffer[Index],
               &ContainerGuid2,
               &VolumeGuid2,
               &VolumeRole2
               );

    DEBUG ((
      DEBUG_BULK_INFO,
      "OCBP: APFS recovery info %u/%u due to %g/%g/%X - %r\n",
      (UINT32) Index,
      (UINT32) NoHandles,
      &ContainerGuid2,
      &ContainerGuid,
      (UINT32) VolumeRole2,
      Status
      ));

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

    Status = SafeFileOpen (
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
BootPolicyGetApfsRecoveryFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  OUT CHAR16                    **FullPathName,
  OUT VOID                      **Reserved,
  OUT EFI_FILE_PROTOCOL         **Root,
  OUT EFI_HANDLE                *DeviceHandle
  )
{
  if (DevicePath   == NULL
   || PathName     == NULL
   || FullPathName == NULL
   || Reserved     == NULL
   || Root         == NULL
   || DeviceHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Reserved = NULL;

  return OcBootPolicyGetApfsRecoveryFilePath (
    DevicePath,
    PathName,
    gAppleBootPolicyPredefinedPaths,
    ARRAY_SIZE (gAppleBootPolicyPredefinedPaths),
    FullPathName,
    Root,
    DeviceHandle
    );
}

EFI_STATUS
OcBootPolicyGetAllApfsRecoveryFilePath (
  IN  EFI_HANDLE  Handle OPTIONAL,
  OUT VOID        **Volumes,
  OUT UINTN       *NumberOfEntries
  )
{
  EFI_STATUS                      Status;

  UINTN                           NumberOfHandles;
  EFI_HANDLE                      *HandleBuffer;
  APFS_VOLUME_INFO                *VolumeInfo;
  GUID                            *ContainerGuids;
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

  ASSERT (Volumes != NULL);
  ASSERT (NumberOfEntries != NULL);

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

  VolumeInfo = AllocateZeroPool (NumberOfHandles * sizeof (*VolumeInfo));
  ContainerGuids = AllocateZeroPool (NumberOfHandles * sizeof (*ContainerGuids));

  if (VolumeInfo == NULL || ContainerGuids == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
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
      if (CompareGuid (&ContainerGuids[Index2], &VolumeInfo[NumberOfVolumeInfos].ContainerGuid)) {
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

        Status = SafeFileOpen (
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

EFI_STATUS
EFIAPI
BootPolicyGetAllApfsRecoveryFilePath (
  IN  EFI_HANDLE  Handle OPTIONAL,
  OUT VOID        **Volumes,
  OUT UINTN       *NumberOfEntries
  )
{
  if (Volumes == NULL || NumberOfEntries == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return OcBootPolicyGetAllApfsRecoveryFilePath (
    Handle,
    Volumes,
    NumberOfEntries
    );
}

APPLE_BOOT_POLICY_PROTOCOL *
OcAppleBootPolicyInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS Status;

  APPLE_BOOT_POLICY_PROTOCOL  *Protocol;
  EFI_HANDLE                  Handle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleBootPolicyProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCBP: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleBootPolicyProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gAppleBootPolicyProtocolGuid,
    (VOID **) &mAppleBootPolicyProtocol,
    NULL
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleBootPolicyProtocol;
}
