/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>

#include <Protocol/LoadedImage.h>

EFI_DEVICE_PATH_PROTOCOL *
OcGetNextLoadOptionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  )
{
  ASSERT (FALSE);

  return NULL;
}

EFI_STATUS
CachelessContextAddKext (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     CONST CHAR8        *InfoPlist,
  IN     UINT32             InfoPlistSize,
  IN     UINT8              *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL,
  OUT    CHAR8              BundleVersion[MAX_INFO_BUNDLE_VERSION_KEY_SIZE] OPTIONAL
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextAddPatch (
  IN OUT CACHELESS_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextAddQuirk (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     KERNEL_QUIRK_NAME  Quirk
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextBlock (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     CONST CHAR8        *Identifier,
  IN     BOOLEAN            Exclude
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextForceKext (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     CONST CHAR8        *Identifier
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

VOID
CachelessContextFree (
  IN OUT CACHELESS_CONTEXT  *Context
  )
{
  ASSERT (FALSE);
}

EFI_STATUS
CachelessContextHookBuiltin (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     CONST CHAR16       *FileName,
  IN     EFI_FILE_PROTOCOL  *File,
  OUT EFI_FILE_PROTOCOL     **VirtualFile
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextInit (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     CONST CHAR16       *FileName,
  IN     EFI_FILE_PROTOCOL  *ExtensionsDir,
  IN     UINT32             KernelVersion,
  IN     BOOLEAN            Is32Bit
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextOverlayExtensionsDir (
  IN OUT CACHELESS_CONTEXT  *Context,
  OUT EFI_FILE_PROTOCOL     **File
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CachelessContextPerformInject (
  IN OUT CACHELESS_CONTEXT  *Context,
  IN     CONST CHAR16       *FileName,
  OUT EFI_FILE_PROTOCOL     **File
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CreateRealFile (
  IN  EFI_FILE_PROTOCOL  *OriginalFile OPTIONAL,
  IN  EFI_FILE_OPEN      OpenCallback OPTIONAL,
  IN  BOOLEAN            CloseOnFailure,
  OUT EFI_FILE_PROTOCOL  **File
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
CreateVirtualFileFileNameCopy (
  IN  CONST CHAR16       *FileName,
  IN  VOID               *FileBuffer,
  IN  UINT64             FileSize,
  IN  CONST EFI_TIME     *ModificationTime OPTIONAL,
  OUT EFI_FILE_PROTOCOL  **File
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
DisableVirtualFs (
  IN OUT EFI_BOOT_SERVICES  *BootServices
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EnableVirtualFs (
  IN OUT EFI_BOOT_SERVICES  *BootServices,
  IN     EFI_FILE_OPEN      OpenCallback
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

BOOLEAN
OcAppendArgumentsToLoadedImage (
  IN OUT EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN     CONST CHAR8                **Arguments,
  IN     UINT32                     ArgumentCount,
  IN     BOOLEAN                    Replace
  )
{
  ASSERT (FALSE);

  return FALSE;
}

BOOLEAN
OcCheckArgumentFromEnv (
  IN     EFI_LOADED_IMAGE  *LoadedImage OPTIONAL,
  IN     EFI_GET_VARIABLE  GetVariable OPTIONAL,
  IN     CONST CHAR8       *Argument,
  IN     CONST UINTN       ArgumentLength,
  IN OUT CHAR8             **Value OPTIONAL
  )
{
  ASSERT (FALSE);

  return FALSE;
}

VOID
OcDirectorySeachContextInit (
  IN OUT DIRECTORY_SEARCH_CONTEXT  *Context
  )
{
  ASSERT (FALSE);
}

EFI_STATUS
OcFindWritableOcFileSystem (
  OUT EFI_FILE_PROTOCOL  **FileSystem
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

OC_BOOT_ENTRY_TYPE
OcGetBootDevicePathType (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT BOOLEAN                   *IsFolder   OPTIONAL,
  OUT BOOLEAN                   *IsGeneric  OPTIONAL
  )
{
  ASSERT (FALSE);

  return 0;
}

EFI_STATUS
OcGetFileModificationTime (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT EFI_TIME           *Time
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

EFI_STATUS
OcGetNewestFileFromDirectory (
  IN OUT DIRECTORY_SEARCH_CONTEXT  *Context,
  IN     EFI_FILE_PROTOCOL         *Directory,
  IN     CHAR16                    *FileNameStartsWith OPTIONAL,
  OUT EFI_FILE_INFO                **FileInfo
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

VOID
OcImageLoaderRegisterConfigure (
  IN OC_IMAGE_LOADER_CONFIGURE  Configure  OPTIONAL
  )
{
  ASSERT (FALSE);
}

BOOLEAN
OcPlatformIs64BitSupported (
  IN UINT32  KernelVersion
  )
{
  ASSERT (FALSE);

  return FALSE;
}

EFI_STATUS
OcSafeFileOpen (
  IN     CONST EFI_FILE_PROTOCOL  *Protocol,
  OUT       EFI_FILE_PROTOCOL     **NewHandle,
  IN     CONST CHAR16             *FileName,
  IN     CONST UINT64             OpenMode,
  IN     CONST UINT64             Attributes
  )
{
  ASSERT (FALSE);

  return EFI_UNSUPPORTED;
}

VOID *
OcStorageReadFileUnicode (
  IN  OC_STORAGE_CONTEXT  *Context,
  IN  CONST CHAR16        *FilePath,
  OUT UINT32              *FileSize OPTIONAL
  )
{
  ASSERT (FALSE);

  return NULL;
}

VOID *
OcReadFileFromDirectory (
  IN      CONST EFI_FILE_PROTOCOL  *RootDirectory,
  IN      CONST CHAR16             *FilePath,
  OUT       UINT32                 *FileSize OPTIONAL,
  IN            UINT32             MaxFileSize OPTIONAL
  )
{
  ASSERT (FALSE);

  return NULL;
}

EFI_MEMORY_DESCRIPTOR *
OcGetCurrentMemoryMap (
  OUT UINTN    *MemoryMapSize,
  OUT UINTN    *DescriptorSize,
  OUT UINTN    *MapKey                 OPTIONAL,
  OUT UINT32   *DescriptorVersion      OPTIONAL,
  OUT UINTN    *OriginalMemoryMapSize  OPTIONAL,
  IN  BOOLEAN  IncludeSplitSpace
  )
{
  ASSERT (FALSE);

  return NULL;
}

VOID *
OcGetFileInfo (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  EFI_GUID           *InformationType,
  IN  UINTN              MinFileInfoSize,
  OUT UINTN              *RealFileInfoSize  OPTIONAL
  )
{
  ASSERT (FALSE);

  return NULL;
}

CHAR16 *
OcGetVolumeLabel (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
  )
{
  ASSERT (FALSE);

  return NULL;
}

VOID *
OcReadFile (
  IN     CONST EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN     CONST CHAR16                           *FilePath,
  OUT       UINT32                              *FileSize OPTIONAL,
  IN     CONST UINT32                           MaxFileSize OPTIONAL
  )
{
  ASSERT (FALSE);

  return NULL;
}
