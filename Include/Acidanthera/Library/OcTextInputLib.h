/** @file
   OcTextInputLib - SimpleTextInputEx Protocol Compatibility Library

   This library provides SimpleTextInputEx protocol compatibility for systems
   that only have SimpleTextInput protocol available (EFI 1.1 systems).

   Copyright (c) 2025, OpenCore Team. All rights reserved.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#ifndef OC_TEXT_INPUT_LIB_H
#define OC_TEXT_INPUT_LIB_H

#include <Uefi.h>
#include <Protocol/SimpleTextInEx.h>

/**
   Install SimpleTextInputEx compatibility protocol on the console input handle.

   This function checks if SimpleTextInputEx is already available, and if not,
   installs a compatibility version that wraps SimpleTextInput protocol.

   For the "normal" variant, this uses standard gBS->InstallProtocolInterface.
   For the "local" variant, this uses OcRegisterBootServicesProtocol for
   use within OpenShell.

   @retval EFI_SUCCESS          Protocol installed successfully or already present
   @retval EFI_ALREADY_STARTED  Protocol already exists, no action taken
   @retval Others               Installation failed
 **/
EFI_STATUS
OcInstallSimpleTextInputEx (
  VOID
  );

/**
   Uninstall SimpleTextInputEx compatibility protocol.

   This should be called during cleanup if the compatibility protocol
   was installed by this library.

   @retval EFI_SUCCESS          Protocol uninstalled successfully
   @retval EFI_NOT_FOUND        Protocol was not installed by this library
   @retval Others               Uninstallation failed
 **/
EFI_STATUS
OcUninstallSimpleTextInputEx (
  VOID
  );

/**
   Test CTRL key detection on the current system.

   This function can be used to verify that CTRL key combinations
   are properly detected on EFI 1.1 systems. Useful for debugging
   compatibility issues like those seen on cMP5,1.

   @retval EFI_SUCCESS          Test completed, check debug output
   @retval Others               Test failed
 **/
EFI_STATUS
OcTestCtrlKeyDetection (
  VOID
  );

#endif // OC_TEXT_INPUT_LIB_H
