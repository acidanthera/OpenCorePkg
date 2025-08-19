/** @file
  OcTextInputDxe - Standalone SimpleTextInputEx Protocol Compatibility Driver
  
  This driver provides SimpleTextInputEx protocol compatibility for systems
  that only have SimpleTextInput protocol available (EFI 1.1 systems).
  
  This addresses compatibility issues with older systems (like cMP5,1) where
  CTRL key combinations may not work reliably in applications like the Shell
  text editor. The text editor has been enhanced with F10/ESC alternatives.
  
  This driver can be used for firmware injection outside of OpenCorePkg.

  Copyright (c) 2025, OpenCore Team. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcTextInputCommon.h>

//
// Global variables for compatibility protocol
//
STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *mOriginalSimpleTextInputEx = NULL;
STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  mCompatSimpleTextInputEx;
STATIC BOOLEAN                            mProtocolInstalled = FALSE;

/**
  Compatibility implementation of Reset for SimpleTextInputEx.
  
  @param[in]  This                Protocol instance pointer.
  @param[in]  ExtendedVerification Driver may perform diagnostics on reset.
                                  
  @retval EFI_SUCCESS             Reset completed successfully.
  @retval EFI_DEVICE_ERROR        Reset failed.
**/
STATIC
EFI_STATUS
EFIAPI
CompatReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  return gST->ConIn->Reset (gST->ConIn, ExtendedVerification);
}

/**
  Compatibility implementation of SimpleTextInputEx ReadKeyStrokeEx.
  
  @param[in]   This               Protocol instance pointer.
  @param[out]  KeyData            A pointer to a buffer that is filled in with
                                  the keystroke state data for the key that was
                                  pressed.
  
  @retval EFI_SUCCESS             The keystroke information was returned.
  @retval EFI_NOT_READY           There was no keystroke data available.
  @retval EFI_DEVICE_ERROR        The keystroke information was not returned due
                                  to hardware errors.
  @retval EFI_INVALID_PARAMETER   KeyData is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
CompatReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  EFI_STATUS  Status;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Clear the KeyData structure
  ZeroMem (KeyData, sizeof (EFI_KEY_DATA));

  // Get keystroke from standard SimpleTextInput protocol
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &KeyData->Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Initialize KeyState - no shift states detected from basic input
  KeyData->KeyState.KeyShiftState     = EFI_SHIFT_STATE_VALID;
  KeyData->KeyState.KeyToggleState    = EFI_TOGGLE_STATE_VALID;

  // Enhanced logging for control characters (0x01-0x1F)
  if ((KeyData->Key.UnicodeChar >= 0x01) && (KeyData->Key.UnicodeChar <= 0x1F)) {
    OctiLogControlChar (KeyData->Key.UnicodeChar);

    // For control characters, we can infer CTRL key was pressed
    KeyData->KeyState.KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }

  OCTI_DEBUG_VERBOSE (
    "KeyStroke - Unicode: 0x%04X, ScanCode: 0x%02X, ShiftState: 0x%08X\n",
    KeyData->Key.UnicodeChar,
    KeyData->Key.ScanCode,
    KeyData->KeyState.KeyShiftState
  );

  return EFI_SUCCESS;
}

/**
  SetState implementation for SimpleTextInputEx compatibility.
  
  @param[in]  This                Pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.
  @param[in]  KeyToggleState      A pointer to the EFI_KEY_TOGGLE_STATE to set the 
                                  state for the input device.
                                  
  @retval EFI_SUCCESS             The device state was set successfully.
  @retval EFI_DEVICE_ERROR        The device is not functioning correctly and could 
                                  not have the setting adjusted.
  @retval EFI_UNSUPPORTED         The device does not have the ability to set its state.
  @retval EFI_INVALID_PARAMETER   KeyToggleState is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
CompatSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Basic input protocol doesn't support state setting
  OCTI_DEBUG_VERBOSE ("SetState called - operation not supported\n");
  return EFI_UNSUPPORTED;
}

/**
  RegisterKeyNotify implementation for SimpleTextInputEx compatibility.
  
  @param[in]   This                    Pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.
  @param[in]   KeyData                 A pointer to a buffer that is filled in with the keystroke 
                                       information data for the key that was pressed.
  @param[in]   KeyNotificationFunction Points to the function to be called when the key 
                                       sequence is typed specified by KeyData.
  @param[out]  NotifyHandle            Points to the unique handle assigned to the registered notification.
                                       
  @retval EFI_SUCCESS                  The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES         Unable to allocate resources for necessary data structures.
  @retval EFI_INVALID_PARAMETER        KeyData or KeyNotificationFunction or NotifyHandle is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
CompatRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  )
{
  if ((KeyData == NULL) || (KeyNotificationFunction == NULL) || (NotifyHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Basic input protocol doesn't support key notifications
  OCTI_DEBUG_VERBOSE ("RegisterKeyNotify called - operation not supported\n");
  *NotifyHandle = NULL;
  return EFI_UNSUPPORTED;
}

/**
  UnregisterKeyNotify implementation for SimpleTextInputEx compatibility.
  
  @param[in]  This                    Pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.
  @param[in]  NotificationHandle      The handle of the notification function being unregistered.
  
  @retval EFI_SUCCESS                 The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER       The NotificationHandle is not valid.
**/
STATIC
EFI_STATUS
EFIAPI
CompatUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Basic input protocol doesn't support key notifications
  OCTI_DEBUG_VERBOSE ("UnregisterKeyNotify called - operation not supported\n");
  return EFI_UNSUPPORTED;
}

/**
  Install SimpleTextInputEx Protocol compatibility.
  
  @retval EFI_SUCCESS               Protocol installed successfully.
  @retval EFI_ALREADY_STARTED      Protocol already exists, no action taken.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Other                    Installation failed.
**/
STATIC
EFI_STATUS
InstallSimpleTextInputExCompat (
  VOID
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *NativeSimpleTextInputEx;

  if (mProtocolInstalled) {
    OCTI_DEBUG_VERBOSE ("SimpleTextInputEx compatibility already installed\n");
    return EFI_ALREADY_STARTED;
  }

  // Check if SimpleTextInputEx is already available
  NativeSimpleTextInputEx = NULL;
  Status                  = gBS->HandleProtocol (
                                   gST->ConsoleInHandle,
                                   &gEfiSimpleTextInputExProtocolGuid,
                                   (VOID **)&NativeSimpleTextInputEx
                                   );

  if (!EFI_ERROR (Status) && (NativeSimpleTextInputEx != NULL)) {
    OCTI_DEBUG_INFO ("Native SimpleTextInputEx protocol already present, no compatibility needed\n");
    mOriginalSimpleTextInputEx = NativeSimpleTextInputEx;
    return EFI_ALREADY_STARTED;
  }

  OCTI_DEBUG_INFO ("Installing SimpleTextInputEx compatibility layer\n");

  // Initialize our compatibility protocol structure
  mCompatSimpleTextInputEx.Reset                  = CompatReset;
  mCompatSimpleTextInputEx.ReadKeyStrokeEx        = CompatReadKeyStrokeEx;
  mCompatSimpleTextInputEx.WaitForKeyEx           = gST->ConIn->WaitForKey;
  mCompatSimpleTextInputEx.SetState               = CompatSetState;
  mCompatSimpleTextInputEx.RegisterKeyNotify      = CompatRegisterKeyNotify;
  mCompatSimpleTextInputEx.UnregisterKeyNotify    = CompatUnregisterKeyNotify;

  // Install the protocol on the console input handle
  Status = gBS->InstallProtocolInterface (
                  &gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mCompatSimpleTextInputEx
                  );

  if (EFI_ERROR (Status)) {
    OCTI_DEBUG_ERROR ("Failed to install SimpleTextInputEx protocol - %r\n", Status);
    return Status;
  }

  mProtocolInstalled = TRUE;
  OCTI_DEBUG_INFO ("SimpleTextInputEx compatibility installed successfully\n");

  return EFI_SUCCESS;
}

/**
  Driver entry point for OcTextInputDxe.
  
  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The driver was initialized successfully.
  @retval Others            The driver failed to initialize.
**/
EFI_STATUS
EFIAPI
OcTextInputDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  OCTI_DEBUG_INFO ("OcTextInputDxe driver starting...\n");

  Status = InstallSimpleTextInputExCompat ();
  if (EFI_ERROR (Status)) {
    OCTI_DEBUG_ERROR ("Failed to install SimpleTextInputEx compatibility - %r\n", Status);
    return Status;
  }

  OCTI_DEBUG_INFO ("OcTextInputDxe driver loaded successfully\n");
  return EFI_SUCCESS;
}
