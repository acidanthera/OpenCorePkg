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

#ifndef BOOT_MANAGEMENET_INTERNAL_H
#define BOOT_MANAGEMENET_INTERNAL_H

#include <Uefi.h>

#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcBootManagementLib.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#define OC_CUSTOM_FS_HANDLE ((EFI_HANDLE)(UINTN) 0x2007C5F5U)

///
/// Identifies the DevicePath structure for OpenCore custom entries.
///
#define OC_CUSTOM_BOOT_DEVICE_PATH_GUID  \
  { 0xd6f263f9, 0x0b19, 0x4670,          \
    { 0xb0, 0xa4, 0x9d, 0x95, 0x9f, 0x58, 0xdf, 0x65 } }

#pragma pack(1)

///
/// DevicePath to describe OpenCore custom entries.
///
typedef PACKED struct {
  VENDOR_DEVICE_PATH   Hdr;
  FILEPATH_DEVICE_PATH EntryName;
} OC_CUSTOM_BOOT_DEVICE_PATH;

//
// Ideally, a variant of FILEPATH_DEVICE_PATH will be used with PathName as a
// flexible array. Such cannot be used for declarations, so provide an
// alternative.
//
typedef PACKED struct {
  VENDOR_DEVICE_PATH       Header;
  EFI_DEVICE_PATH_PROTOCOL EntryName;
} OC_CUSTOM_BOOT_DEVICE_PATH_DECL;

#pragma pack()

///
/// The size of a OC_CUSTOM_BOOT_DEVICE_PATH structure excluding the name.
///
#define SIZE_OF_OC_CUSTOM_BOOT_DEVICE_PATH  \
  (sizeof (VENDOR_DEVICE_PATH) + SIZE_OF_FILEPATH_DEVICE_PATH)

//
// Max. supported Apple version string size
//
#define OC_APPLE_VERSION_MAX_SIZE (16)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  OC_APPLE_DISK_IMAGE_CONTEXT    *DmgContext;
  EFI_HANDLE                     BlockIoHandle;
} INTERNAL_DMG_LOAD_CONTEXT;

typedef struct {
  EFI_HANDLE               Device;
  UINTN                    NumBootInstances;
  UINTN                    HdPrefixSize;
  EFI_DEVICE_PATH_PROTOCOL *HdDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *BootDevicePath;
  BOOLEAN                  IsExternal;
  BOOLEAN                  SkipRecovery;
} INTERNAL_DEV_PATH_SCAN_INFO;

EFI_STATUS
InternalCheckScanPolicy (
  IN  EFI_HANDLE                       Handle,
  IN  UINT32                           Policy,
  OUT BOOLEAN                          *External OPTIONAL
  );

EFI_DEVICE_PATH_PROTOCOL *
InternalLoadDmg (
  IN OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN     OC_DMG_LOADING_SUPPORT      DmgLoading
  );

VOID
InternalUnloadDmg (
  IN INTERNAL_DMG_LOAD_CONTEXT  *DmgLoadContext
  );

CHAR16 *
InternalGetAppleDiskLabel (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName,
  IN  CONST CHAR16                     *LabelFilename
  );

CHAR8 *
InternalGetContentFlavour (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName,
  IN  CONST CHAR16                     *FlavourFilename
  );

EFI_STATUS
InternalGetAppleImage (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *DirectoryName,
  IN  CONST CHAR16                     *LabelFilename,
  OUT VOID                             **ImageData,
  OUT UINT32                           *DataSize
  );

EFI_STATUS
InternalGetRecoveryOsBooter (
  IN  EFI_HANDLE                Device,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  IN  BOOLEAN                   Basic
  );

EFI_STATUS
InternalLoadBootEntry (
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle,
  OUT INTERNAL_DMG_LOAD_CONTEXT   *DmgLoadContext
  );

UINT16 *
InternalGetBootOrderForBooting (
  IN  EFI_GUID  *BootVariableGuid,
  IN  BOOLEAN   BlacklistAppleUpdate,
  OUT UINTN     *BootOrderCount
  );

VOID
InternalDebugBootEnvironment (
  IN CONST UINT16             *BootOrder,
  IN EFI_GUID                 *BootGuid,
  IN UINTN                    BootOrderCount
  );

EFI_LOAD_OPTION *
InternalGetBootOptionData (
  OUT UINTN           *OptionSize,
  IN  UINT16          BootOption,
  IN  CONST EFI_GUID  *BootGuid
  );

EFI_DEVICE_PATH_PROTOCOL *
InternalGetBootOptionPath (
  IN EFI_LOAD_OPTION  *LoadOption,
  IN UINTN            LoadOptionSize
  );

/**
  Describe boot entry contents by setting fields other than DevicePath.

  @param[in]      BootContext   Boot context.
  @param[in,out]  BootEntry     Located boot entry.

  @retval EFI_SUCCESS   The entry point is described successfully.
**/
EFI_STATUS
InternalDescribeBootEntry (
  IN     OC_BOOT_CONTEXT  *BootContext,
  IN OUT OC_BOOT_ENTRY    *BootEntry
  );

BOOLEAN
InternalIsAppleLegacyLoadApp (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Finds filesystem for discovered filesystem handle.
  This solves the problem of checking scan policy multiple times
  as well as the problem of finding the filesystem to add entries too.

  @param[in] BootContext       Context of filesystems.
  @param[in] FileSystemHandle  Partition handle.
  @param[in] LazyScan          Lazy filesystem scanning.

  @retval discovered filesystem (legit).
  @retcal NULL when booting is not allowed from this filesystem.
**/
OC_BOOT_FILESYSTEM *
InternalFileSystemForHandle (
  IN OC_BOOT_CONTEXT  *BootContext,
  IN EFI_HANDLE       FileSystemHandle,
  IN BOOLEAN          LazyScan
  );

/**
  Resets selected NVRAM variables and reboots the system.
**/
EFI_STATUS
InternalSystemActionResetNvram (
  VOID
  );

/**
  Toggles SIP.
**/
EFI_STATUS
InternalSystemActionToggleSip (
  VOID
  );

/**
  Determines whether DevicePath is an OpenCore custom boot entry.

  @returns  The OpenCore custom boot entry, or NULL.
**/
CONST OC_CUSTOM_BOOT_DEVICE_PATH *
InternalGetOcCustomDevPath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

EFI_STATUS
InternalRunRequestPrivilege (
  IN OC_PICKER_CONTEXT   *PickerContext,
  IN OC_PRIVILEGE_LEVEL  Level
  );

#endif // BOOT_MANAGEMENET_INTERNAL_H
