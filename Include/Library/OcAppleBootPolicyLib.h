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

#ifndef OC_APPLE_BOOT_POLICY_LIB_H
#define OC_APPLE_BOOT_POLICY_LIB_H

#include <Protocol/AppleBootPolicy.h>

/**
  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
OcAppleBootPolicyInstallProtocol (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  @param[in]  BootPolicy     Apple Boot Policy Protocol.
  @param[in]  DevicePath     Located device path boot entry.
  @param[out] BootEntryName  Obtained human visible name (optional).
  @param[out] BootPathName   Obtained boot path directory (optional).

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
OcDescribeBootEntry (
  IN     APPLE_BOOT_POLICY_PROTOCOL *BootPolicy,
  IN     EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN OUT CHAR16                     **BootEntryName OPTIONAL,
  IN OUT CHAR16                     **BootPathName OPTIONAL
  );

#endif // OC_APPLE_BOOT_POLICY_LIB_H
