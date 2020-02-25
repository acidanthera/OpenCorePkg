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

#define OC_MENU_BOOT_MENU            L"OpenCore Boot Menu"
#define OC_MENU_RESET_NVRAM_ENTRY    L"Reset NVRAM"
#define OC_MENU_UEFI_SHELL_ENTRY     L"UEFI Shell"
#define OC_MENU_PASSWORD_REQUEST     L"Password: "
#define OC_MENU_PASSWORD_RETRY_LIMIT L"Password retry limit exceeded."
#define OC_MENU_CHOOSE_OS            L"Choose the Operating System: "
#define OC_MENU_SHOW_AUXILIARY       L"Show Auxiliary"
#define OC_MENU_RELOADING            L"Reloading"
#define OC_MENU_TIMEOUT              L"Timeout"
#define OC_MENU_OK                   L"OK"
#define OC_MENU_DISK_IMAGE           L" (dmg)"
#define OC_MENU_EXTERNAL             L" (external)"

#define OC_VOICE_OVER_IDLE_TIMEOUT_MS     500  ///< Experimental, less is problematic.

#define OC_VOICE_OVER_SIGNAL_NORMAL_MS    200  ///< From boot.efi, constant.
#define OC_VOICE_OVER_SILENCE_NORMAL_MS   150  ///< From boot.efi, constant.
#define OC_VOICE_OVER_SIGNALS_NORMAL      1    ///< Username prompt or any input for boot.efi
#define OC_VOICE_OVER_SIGNALS_PASSWORD    2    ///< Password prompt for boot.efi
#define OC_VOICE_OVER_SIGNALS_PASSWORD_OK 3    ///< Password correct for boot.efi

#define OC_VOICE_OVER_SIGNAL_ERROR_MS     1000
#define OC_VOICE_OVER_SILENCE_ERROR_MS    150
#define OC_VOICE_OVER_SIGNALS_ERROR       1    ///< Password verification error or boot failure.
#define OC_VOICE_OVER_SIGNALS_HWERROR     3    ///< Hardware error

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

RETURN_STATUS
InternalCheckScanPolicy (
  IN  EFI_HANDLE                       Handle,
  IN  UINT32                           Policy,
  OUT BOOLEAN                          *External OPTIONAL
  );

EFI_DEVICE_PATH_PROTOCOL *
InternalLoadDmg (
  IN OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN     APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN     UINT32                      Policy
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

CHAR16 *
InternalGetAppleRecoveryName (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName
  );

EFI_STATUS
InternalGetRecoveryOsBooter (
  IN  EFI_HANDLE                Device,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  IN  BOOLEAN                   Basic
  );

VOID
InternalSetBootEntryFlags (
  IN OUT OC_BOOT_ENTRY   *BootEntry
  );

EFI_STATUS
InternalLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle,
  OUT INTERNAL_DMG_LOAD_CONTEXT   *DmgLoadContext
  );

EFI_STATUS
InternalPrepareScanInfo (
  IN     APPLE_BOOT_POLICY_PROTOCOL       *BootPolicy,
  IN     OC_PICKER_CONTEXT                *Context,
  IN     EFI_HANDLE                       *Handles,
  IN     UINTN                            Index,
  IN OUT INTERNAL_DEV_PATH_SCAN_INFO      *DevPathScanInfo
  );

UINTN
InternalFillValidBootEntries (
  IN     APPLE_BOOT_POLICY_PROTOCOL   *BootPolicy,
  IN     OC_PICKER_CONTEXT            *Context,
  IN     INTERNAL_DEV_PATH_SCAN_INFO  *DevPathScanInfo,
  IN     EFI_DEVICE_PATH_PROTOCOL     *DevicePathWalker,
  IN OUT OC_BOOT_ENTRY                *Entries,
  IN     UINTN                        EntryIndex
  );

/**
  Resets selected NVRAM variables and reboots the system.
**/
EFI_STATUS
InternalSystemActionResetNvram (
  VOID
  );

#endif // BOOT_MANAGEMENET_INTERNAL_H
