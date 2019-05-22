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

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  OC_APPLE_DISK_IMAGE_CONTEXT    *DmgContext;
  EFI_HANDLE                     BlockIoHandle;
} INTERNAL_DMG_LOAD_CONTEXT;

EFI_STATUS
InternalCheckScanPolicy (
  IN  EFI_HANDLE                       Handle,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs,
  IN  UINT32                           Policy
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

OC_BOOT_ENTRY *
InternalGetDefaultBootEntry (
  IN OUT OC_BOOT_ENTRY  *BootEntries,
  IN     UINTN          NumBootEntries,
  IN     BOOLEAN        CustomBootGuid,
  IN     EFI_HANDLE     LoadHandle  OPTIONAL
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

#endif // BOOT_MANAGEMENET_INTERNAL_H
