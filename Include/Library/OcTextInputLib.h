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
#include <Library/OcTextInputCommon.h>

//
// Type definition for compatibility
//
typedef OCTI_CONTROL_CHAR_MAPPING CONTROL_CHAR_MAPPING;

/**
  Get control character mapping for a given character.

  @param[in]  ControlChar  The control character to get mapping for.

  @retval  Pointer to CONTROL_CHAR_MAPPING structure, or NULL if not found.
**/
CONTROL_CHAR_MAPPING *
EFIAPI
GetControlCharMapping (
  IN CHAR16  ControlChar
  );

/**
  Compatibility implementation of SimpleTextInputEx ReadKeyStrokeEx.

  @param[in]   This               Protocol instance pointer.
  @param[out]  KeyData            A pointer to a buffer that is filled in with
                                  the keystroke state data for the key that was
                                  pressed.

  @retval EFI_SUCCESS             The keystroke information was returned.
  @retval EFI_NOT_READY           There was no keystroke data available.
  @retval EFI_DEVICE_ERROR        The keystroke information was not returned due
                                  to an error.
  @retval EFI_INVALID_PARAMETER   KeyData is NULL.
**/
EFI_STATUS
EFIAPI
CompatReadKeyStrokeEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Install SimpleTextInputEx Protocol compatibility.

  This function installs a SimpleTextInputEx protocol instance that provides
  compatibility for systems that only have SimpleTextInput protocol available.

  @param[in]  UseLocalRegistration  If TRUE, register protocol with OpenCore's
                                    internal table. If FALSE, register with
                                    system's boot services table.

  @retval EFI_SUCCESS               Protocol installed successfully.
  @retval EFI_OUT_OF_RESOURCES      Failed to allocate memory.
  @retval Other                     Installation failed.
**/
EFI_STATUS
OcInstallSimpleTextInputExInternal (
  IN BOOLEAN  UseLocalRegistration
  );

#endif // OC_TEXT_INPUT_LIB_H
