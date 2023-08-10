/** @file
  Copyright (C) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef LEGACY_BOOT_INTERNAL_H
#define LEGACY_BOOT_INTERNAL_H

extern EFI_HANDLE  gImageHandle;
extern BOOLEAN     gIsAppleInterfaceSupported;

/**
  Legacy operating system type.
**/
typedef enum OC_LEGACY_OS_TYPE_ {
  OcLegacyOsTypeNone,
  OcLegacyOsTypeWindowsNtldr,
  OcLegacyOsTypeWindowsBootmgr,
  OcLegacyOsTypeIsoLinux
} OC_LEGACY_OS_TYPE;

EFI_STATUS
InternalIsLegacyInterfaceSupported (
  OUT BOOLEAN  *IsAppleInterfaceSupported
  );

EFI_STATUS
InternalLoadAppleLegacyInterface (
  IN  EFI_HANDLE                ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath,
  OUT EFI_HANDLE                *ImageHandle
  );

OC_LEGACY_OS_TYPE
InternalGetPartitionLegacyOsType (
  IN  EFI_HANDLE  PartitionHandle
  );

EFI_STATUS
InternalLoadLegacyMbr (
  IN  EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
  );

#endif // LEGACY_BOOT_INTERNAL_H
