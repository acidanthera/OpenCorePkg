/** @file
   OcTextInputLibInternal - Internal declarations for OcTextInputLib

   Internal function prototypes and definitions used within OcTextInputLib
   implementation files. Not exposed in the public API.

   Copyright (c) 2025, OpenCore Team. All rights reserved.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#ifndef OC_TEXT_INPUT_LIB_INTERNAL_H
#define OC_TEXT_INPUT_LIB_INTERNAL_H

#include <Uefi.h>

/**
   Internal implementation for installing SimpleTextInputEx protocol.

   @param  UseLocalRegistration  If TRUE, use local registration method (for OpenShell)
                                If FALSE, use standard gBS method (for drivers)

   @retval EFI_SUCCESS          Protocol installed successfully or already present
   @retval EFI_ALREADY_STARTED  Protocol already exists, no action taken
   @retval Others               Installation failed
 **/
EFI_STATUS
OcInstallSimpleTextInputExInternal (
  IN BOOLEAN  UseLocalRegistration
  );

#endif // OC_TEXT_INPUT_LIB_INTERNAL_H
