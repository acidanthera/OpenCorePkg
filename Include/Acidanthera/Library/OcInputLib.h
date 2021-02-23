/** @file
  OC Apple generic input library.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_APPLE_GENERIC_INPUT_LIB_H
#define OC_APPLE_GENERIC_INPUT_LIB_H

typedef enum {
  OcInputPointerModeAsus,
  OcInputPointerModeMax
} OC_INPUT_POINTER_MODE;

typedef enum {
  OcInputKeyModeAuto,
  OcInputKeyModeV1,
  OcInputKeyModeV2,
  OcInputKeyModeAmi,
  OcInputKeyModeMax
} OC_INPUT_KEY_MODE;

EFI_STATUS
OcAppleGenericInputTimerQuirkInit (
  IN UINT32  TimerResolution
  );

EFI_STATUS
OcAppleGenericInputTimerQuirkExit (
  VOID
  );

EFI_STATUS
OcAppleGenericInputPointerInit (
  IN OC_INPUT_POINTER_MODE  Mode
  );

EFI_STATUS
OcAppleGenericInputPointerExit (
  VOID
  );

EFI_STATUS
OcAppleGenericInputKeycodeInit (
  IN OC_INPUT_KEY_MODE  Mode,
  IN UINT8              KeyForgotThreshold,
  IN BOOLEAN            KeySwap,
  IN BOOLEAN            KeyFiltering
  );

EFI_STATUS
OcAppleGenericInputKeycodeExit (
  VOID
  );

#endif // OC_APPLE_GENERIC_INPUT_LIB_H
