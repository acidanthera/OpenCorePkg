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

#ifndef OC_FIRMWARE_RUNTIME_PROTOCOL_H
#define OC_FIRMWARE_RUNTIME_PROTOCOL_H

#include <Uefi.h>

#define OC_FIRMWARE_RUNTIME_REVISION 11

/**
  OC_FIRMWARE_RUNTIME_PROTOCOL_GUID
  570332E4-FC50-4B21-ABE8-AE72F05B4FF7
**/
#define OC_FIRMWARE_RUNTIME_PROTOCOL_GUID   \
  { 0x570332E4, 0xFC50, 0x4B21,             \
    { 0xAB, 0xE8, 0xAE, 0x72, 0xF0, 0x5B, 0x4F, 0xF7 } }
/**
  Configuration request to change firmware runtime behaviour.
**/
typedef struct OC_FWRT_CONFIG_ {
  ///
  /// Enforce restricted access to OpenCore read-only and write-only GUIDs.
  ///
  BOOLEAN  RestrictedVariables;
  ///
  /// Enforce BootXXXX variable redirection to OpenCore vendor GUID.
  ///
  BOOLEAN  BootVariableRedirect;
  ///
  /// Make SetVariable do nothing and always return EFI_SECURITY_VIOLATION.
  /// When we do not want variables to be stored in NVRAM or NVRAM implementation
  /// is buggy we can disable variable writing.
  ///
  BOOLEAN  WriteProtection;
  ///
  /// Make UEFI runtime services drop CR0 WP bit on calls to allow writing
  /// to read only memory. This workarounds a bug in many APTIO firmwares
  /// that do not survive W^X.
  /// Latest Windows brings Virtualization-based security and monitors
  /// CR0 by launching itself under a hypevisor. Since we need WP disable
  /// on macOS to let NVRAM work, and for the time being no other OS
  /// requires it, here we decide to use it for macOS exclusively.
  ///
  BOOLEAN  WriteUnprotector;
  ///
  /// Secure boot variable protection.
  ///
  BOOLEAN  ProtectSecureBoot;
} OC_FWRT_CONFIG;

/**
  Get current used configuration data.

  @param[out]  Config      Current configuration to store.
**/
typedef
VOID
(EFIAPI *OC_FWRT_GET_CURRENT_CONFIG) (
  OUT OC_FWRT_CONFIG  *Config
  );

/**
  Set main configuration.

  @param[in]  Config        Runtime services configuration to apply.
**/
typedef
VOID
(EFIAPI *OC_FWRT_SET_MAIN_CONFIG) (
  IN CONST OC_FWRT_CONFIG  *Config
  );

/**
  Perform configuration override, NULL Config implies disable override.

  @param[in]  Config        Runtime services configuration to apply, optional.
**/
typedef
VOID
(EFIAPI *OC_FWRT_SET_OVERRIDE_CONFIG) (
  IN CONST OC_FWRT_CONFIG  *Config OPTIONAL
  );

/**
  Set GetVariable override for customising values.

  @param[in]   GetVariable     GetVariable to call on each call.
  @param[out]  OrgGetVariable  Original GetVariable to call from GetVariable.

  @retval EFI_SUCCESS on successful override.
**/
typedef
EFI_STATUS
(EFIAPI *OC_FWRT_ON_GET_VARIABLE) (
  IN  EFI_GET_VARIABLE  GetVariable,
  OUT EFI_GET_VARIABLE  *OrgGetVariable  OPTIONAL
  );

/**
  Obtain area that needs to be executable in OS runtime.

  @param[out]  BaseAddress   Executable area start.
  @param[out]  Pages         Number of executable pages.

  @retval EFI_SUCCESS on success.
  @retval EFI_UNSUPPORTED not required.
**/
typedef
EFI_STATUS
(EFIAPI *OC_FWRT_GET_EXEC_AREA) (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINTN                 *Pages
  );

/**
  Firmware runtime protocol instance.
  Check for revision to ensure binary compatibility.
**/
typedef struct {
  UINTN                        Revision;
  OC_FWRT_GET_CURRENT_CONFIG   GetCurrent;
  OC_FWRT_SET_MAIN_CONFIG      SetMain;
  OC_FWRT_SET_OVERRIDE_CONFIG  SetOverride;
  OC_FWRT_ON_GET_VARIABLE      OnGetVariable;
  OC_FWRT_GET_EXEC_AREA        GetExecArea;
} OC_FIRMWARE_RUNTIME_PROTOCOL;

/**
  Firmware runtime protocol GUID.
**/
extern EFI_GUID gOcFirmwareRuntimeProtocolGuid;

#endif // OC_FIRMWARE_RUNTIME_PROTOCOL_H
