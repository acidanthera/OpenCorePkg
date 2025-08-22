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

//
// Control character mapping structure for compatibility
//
typedef struct {
  UINT8    ControlChar;
  CHAR8    *Description;
  CHAR8    *ShellFunction;
} CONTROL_CHAR_MAPPING;

/**
  Get control character mapping for a given character.

  @param[in]  ControlChar  The control character to get mapping for.

  @retval  Pointer to CONTROL_CHAR_MAPPING structure, or NULL if not found.
**/
CONTROL_CHAR_MAPPING *
EFIAPI
GetControlCharMapping (
  IN UINT8  ControlChar
  );

/**
  Compatibility implementation of SimpleTextInputEx Reset for EFI 1.1 systems.

  @param[in]  This                      Protocol instance pointer.
  @param[in]  ExtendedVerification      Indicates that the driver may perform a more
                                        exhaustive verification operation of the device
                                        during reset.

  @retval EFI_SUCCESS                   The device was reset.
  @retval EFI_DEVICE_ERROR              The device is not functioning properly and could
                                        not be reset.
**/
EFI_STATUS
EFIAPI
OcCompatReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
  Compatibility implementation of SimpleTextInputEx ReadKeyStrokeEx for EFI 1.1 systems.

  @param[in]   This                     Protocol instance pointer.
  @param[out]  KeyData                  A pointer to a buffer that is filled in with
                                        the keystroke state data for the key that was
                                        pressed.

  @retval EFI_SUCCESS                   The keystroke information was returned.
  @retval EFI_NOT_READY                 There was no keystroke data available.
  @retval EFI_DEVICE_ERROR              The keystroke information was not returned due
                                        to hardware errors.
  @retval EFI_INVALID_PARAMETER         KeyData is NULL.
**/
EFI_STATUS
EFIAPI
OcCompatReadKeyStrokeEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Compatibility implementation of SimpleTextInputEx SetState for EFI 1.1 systems.

  @param[in]  This                      Protocol instance pointer.
  @param[in]  KeyToggleState            A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                        state for the input device.

  @retval EFI_SUCCESS                   The device state was set successfully.
  @retval EFI_UNSUPPORTED               The device does not support the ability to have its state set.
**/
EFI_STATUS
EFIAPI
OcCompatSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Compatibility implementation of SimpleTextInputEx RegisterKeyNotify for EFI 1.1 systems.

  @param[in]   This                     Protocol instance pointer.
  @param[in]   KeyData                  A pointer to a buffer that defines the keystroke
                                        information for the notification function.
  @param[in]   KeyNotificationFunction  Points to the function to be called when the key
                                        sequence is typed in the specified input device.
  @param[out]  NotifyHandle             Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                   The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES          Failed to allocate memory for the dummy handle.
**/
EFI_STATUS
EFIAPI
OcCompatRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  );

/**
  Compatibility implementation of SimpleTextInputEx UnregisterKeyNotify for EFI 1.1 systems.

  @param[in]  This                      Protocol instance pointer.
  @param[in]  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS                   The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER         The NotificationHandle is not valid.
**/
EFI_STATUS
EFIAPI
OcCompatUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
  Shared function to read a key stroke from text input with EFI 1.1 compatibility.

  This function provides the common key processing logic that can be shared
  between the library and driver implementations.

  @param  KeyData         Pointer to a buffer to fill with the key data.

  @retval EFI_SUCCESS     The key stroke was successfully read.
  @retval EFI_NOT_READY   There was no key stroke data available.
  @retval EFI_INVALID_PARAMETER KeyData is NULL.
**/
EFI_STATUS
EFIAPI
OcReadKeyStrokeFromTextInput (
  OUT EFI_KEY_DATA  *KeyData
  );

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
