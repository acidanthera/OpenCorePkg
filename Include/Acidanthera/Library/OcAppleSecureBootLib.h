/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_APPLE_SECURE_BOOT_LIB_H
#define OC_APPLE_SECURE_BOOT_LIB_H

#include <Protocol/AppleSecureBoot.h>
#include <Protocol/AppleImg4Verification.h>

/**
  Bootstrap NVRAM and library values for secure booting.

  @param[in] Model          Secure boot model (without ap suffix in lower-case).
  @param[in] Ecid           Enclave identifier, optional.

  @retval EFI_SUCCESS  On success.
**/
EFI_STATUS
OcAppleSecureBootBootstrapValues (
  IN CONST CHAR8  *Model,
  IN UINT64       Ecid  OPTIONAL
  );

/**
  Install and initialise the Apple Secure Boot protocol.

  @param[in] Reinstall          Replace any installed protocol.
  @param[in] SbPolicy           Apple Secure Boot Policy to install.
  @param[in] SbWinPolicy        Apple Secure Boot Windows Policy to install.
  @param[in] SbWinPolicyValid   Whether SbWinPolicy should be installed.

  @returns Installed or located protocol.
  @retval NULL  There was an error locating or installing the protocol.
**/
APPLE_SECURE_BOOT_PROTOCOL *
OcAppleSecureBootInstallProtocol (
  IN BOOLEAN  Reinstall,
  IN UINT8    SbPolicy,
  IN UINT8    SbWinPolicy OPTIONAL,
  IN BOOLEAN  SbWinPolicyValid
  );

/**
  Obtain initialised Apple Secure Boot protocol.

  @returns initialised protocol.
**/
APPLE_SECURE_BOOT_PROTOCOL *
OcAppleSecureBootGetProtocol (
  VOID
  );

/**
  Report DMG loading to Apple Secure Boot protocol.

  @param[in]  LoadingDmg  TRUE after loading DMG.
**/
VOID
OcAppleSecureBootSetDmgLoading (
  IN BOOLEAN  LoadingDmg
  );

/**
  Get DMG loading status on Apple Secure Boot protocol.

  @retval TRUE when loading DMG.
**/
BOOLEAN
OcAppleSecureBootGetDmgLoading (
  VOID
  );

/**
  Perform image verification at path.

  @param[in] DevicePath     Path to the image.
  @param[in] SourceBuffer   Image contents.
  @param[in] SourceSize     Image size.

  @retval EFI_SUCCESS on success.
  @retval EFI_SECURITY_VIOLATION when corrupted signature (should abort and die).
  @retval EFI_ERROR when other errors happened (can continue with UEFI loader).
**/
EFI_STATUS
OcAppleSecureBootVerify (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath,
  IN  VOID                         *SourceBuffer,
  IN  UINTN                        SourceSize
  );

#endif // OC_APPLE_SECURE_BOOT_LIB_H
