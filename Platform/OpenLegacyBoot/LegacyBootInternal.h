/** @file
  Copyright (C) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef LEGACY_BOOT_INTERNAL_H
#define LEGACY_BOOT_INTERNAL_H

#include <Uefi.h>

#include <IndustryStandard/Mbr.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcLegacyThunkLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/Legacy8259.h>

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
InternalGetBiosDiskNumber (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN      EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  OUT  UINT8                        *DriveNumber
  );

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
InternalLoadLegacyPbr (
  IN  EFI_DEVICE_PATH_PROTOCOL  *PartitionPath
  );

#endif // LEGACY_BOOT_INTERNAL_H
