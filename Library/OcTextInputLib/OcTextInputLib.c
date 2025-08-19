/** @file
   OcTextInputLib - SimpleTextInputEx Protocol Compatibility Implementation

   Provides EFI 1.1 compatibility by implementing SimpleTextInputEx protocol
   as a wrapper around the standard SimpleTextInput protocol.

   This compatibility layer addresses issues with older systems (like cMP5,1)
   where CTRL key combinations may not work reliably. The Shell text editor
   natively supports F10 for help and ESC/CTRL+W to exit help, providing
   full compatibility without requiring key mapping changes.

   Copyright (c) 2025, OpenCore Team. All rights reserved.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#include <Uefi.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/OcTextInputLib.h>

//
// Helper macros
//
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(Array)  (sizeof(Array) / sizeof((Array)[0]))
#endif

//
// Enhanced control character mappings (from deprecated SimpleTextInputExCompatDxe)
//
typedef struct {
  UINT8    ControlChar;
  CHAR8    *Description;
  CHAR8    *ShellFunction;
} CONTROL_CHAR_MAPPING;

STATIC CONTROL_CHAR_MAPPING  mControlCharTable[] = {
  { 0x01, "CTRL+A",  "SelectAll"                 },
  { 0x02, "CTRL+B",  "MoveCursorLeft"            },
  { 0x03, "CTRL+C",  "Copy"                      },
  { 0x04, "CTRL+D",  "Delete"                    },
  { 0x05, "CTRL+E",  "MainCommandDisplayHelp"    }, // Help - our primary focus
  { 0x06, "CTRL+F",  "MainCommandSearch"         }, // Search
  { 0x07, "CTRL+G",  "MainCommandGotoLine"       }, // Go to Line
  { 0x08, "CTRL+H",  "Backspace"                 },
  { 0x09, "CTRL+I",  "Tab"                       },
  { 0x0A, "CTRL+J",  "NewLine"                   },
  { 0x0B, "CTRL+K",  "MainCommandCutLine"        }, // Cut Line
  { 0x0C, "CTRL+L",  "Refresh"                   },
  { 0x0D, "CTRL+M",  "CarriageReturn"            },
  { 0x0E, "CTRL+N",  "NewFile"                   },
  { 0x0F, "CTRL+O",  "MainCommandOpenFile"       }, // Open File
  { 0x10, "CTRL+P",  "Print"                     },
  { 0x11, "CTRL+Q",  "MainCommandExit"           }, // Exit
  { 0x12, "CTRL+R",  "MainCommandSearchReplace"  }, // Search & Replace
  { 0x13, "CTRL+S",  "MainCommandSaveFile"       }, // Save File
  { 0x14, "CTRL+T",  "MainCommandSwitchFileType" }, // File Type
  { 0x15, "CTRL+U",  "MainCommandPasteLine"      }, // Paste Line
  { 0x16, "CTRL+V",  "Paste"                     },
  { 0x17, "CTRL+W",  "ExitHelpContext"           }, // Exit Help - used within help display only
  { 0x18, "CTRL+X",  "Cut"                       },
  { 0x19, "CTRL+Y",  "Redo"                      },
  { 0x1A, "CTRL+Z",  "Undo"                      },
  { 0x1B, "ESC",     "ExitHelpContext"           }, // ESC - F10 alternative for help exit
  { 0x1C, "CTRL+\\", "FileSeparator"             },
  { 0x1D, "CTRL+]",  "GroupSeparator"            },
  { 0x1E, "CTRL+^",  "RecordSeparator"           },
  { 0x1F, "CTRL+_",  "UnitSeparator"             }
};

/**
   Get description for a control character.

   @param  ControlChar  The control character code (0x01-0x1F)

   @return Pointer to description string, or NULL if not found
 **/
STATIC
CHAR8 *
GetControlCharDescription (
  IN UINT8  ControlChar
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mControlCharTable); Index++) {
    if (mControlCharTable[Index].ControlChar == ControlChar) {
      return mControlCharTable[Index].Description;
    }
  }

  return NULL;
}

/**
   Get Shell function name for a control character.

   @param  ControlChar  The control character code (0x01-0x1F)

   @return Pointer to Shell function name, or NULL if not found
 **/
STATIC
CHAR8 *
GetControlCharShellFunction (
  IN UINT8  ControlChar
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mControlCharTable); Index++) {
    if (mControlCharTable[Index].ControlChar == ControlChar) {
      return mControlCharTable[Index].ShellFunction;
    }
  }

  return NULL;
}

// Global variables for compatibility protocol
STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *mOriginalSimpleTextInputEx = NULL;
STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  mCompatSimpleTextInputEx;
STATIC BOOLEAN                            mProtocolInstalled = FALSE;

/**
   Compatibility implementation of ReadKeyStrokeEx that falls back to ReadKeyStroke.

   On EFI 1.1 systems like cMP5,1, the SimpleTextInputEx protocol is not available,
   and CTRL key combinations are handled differently. This function provides:

   1. SimpleTextInputEx protocol compatibility
   2. Key remapping for enhanced cMP5,1 compatibility:
      - F10 → CTRL+E (Help functionality for programs that use CTRL+E)

   ESC key is left unchanged as most programs expect ESC to work normally.
   This allows programs to work with F10 as an alternative to CTRL+E on systems
   where CTRL combinations may not be reliable.

   @param  This                 Protocol instance pointer.
   @param  KeyData              A pointer to a buffer that is filled in with the keystroke
                               state data for the key that was pressed.

   @retval EFI_SUCCESS          The keystroke information was returned.
   @retval EFI_NOT_READY        There was no keystroke data available.
   @retval EFI_DEVICE_ERROR     The keystroke information was not returned due to
                               hardware errors.
 **/
STATIC
EFI_STATUS
EFIAPI
OcCompatReadKeyStrokeEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  EFI_STATUS  Status;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Clear the KeyData structure
  ZeroMem (KeyData, sizeof (EFI_KEY_DATA));

  // Try to read from the standard SimpleTextInput protocol
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &KeyData->Key);

  if (!EFI_ERROR (Status)) {
    // Set default shift state (no modifiers detected on EFI 1.1)
    KeyData->KeyState.KeyShiftState  = 0;
    KeyData->KeyState.KeyToggleState = 0;

    // On EFI 1.1 systems, CTRL+key combinations are typically translated
    // to the ASCII control code (key - 'A' + 1). The Shell text editor
    // expects these to be handled as simple UnicodeChar values (1-26) with
    // NO shift state flags, so we deliberately do NOT set EFI_LEFT_CONTROL_PRESSED.
    // This ensures compatibility with MenuBarDispatchControlHotKey which has
    // two different code paths for CTRL handling.
    if ((KeyData->Key.UnicodeChar >= 1) && (KeyData->Key.UnicodeChar <= 26)) {
      // This looks like a CTRL+letter combination (CTRL+A = 1, CTRL+B = 2, etc.)
      // Leave KeyShiftState as 0 to use the "no shift state" code path in text editor
      CHAR8  *Description   = GetControlCharDescription ((UINT8)KeyData->Key.UnicodeChar);
      CHAR8  *ShellFunction = GetControlCharShellFunction ((UINT8)KeyData->Key.UnicodeChar);

      // Highlight important CTRL combinations for Shell text editor
      if (KeyData->Key.UnicodeChar == 5) {
        // CTRL+E
        DEBUG ((
          DEBUG_INFO,
          "OcTextInputLib: *** %a detected (%a) - code %d ***\n",
          Description ? Description : "CTRL+E",
          ShellFunction ? ShellFunction : "Help",
          KeyData->Key.UnicodeChar
          ));
      } else if (KeyData->Key.UnicodeChar == 23) {
        // CTRL+W
        DEBUG ((
          DEBUG_INFO,
          "OcTextInputLib: *** %a detected (%a) - code %d ***\n",
          Description ? Description : "CTRL+W",
          ShellFunction ? ShellFunction : "Exit Help",
          KeyData->Key.UnicodeChar
          ));
      } else {
        // Other CTRL combinations
        if (Description && ShellFunction) {
          DEBUG ((
            DEBUG_VERBOSE,
            "OcTextInputLib: %a detected (%a) - code %d\n",
            Description,
            ShellFunction,
            KeyData->Key.UnicodeChar
            ));
        } else {
          // Fallback to basic display
          CHAR16  CtrlChar = (CHAR16)('A' + KeyData->Key.UnicodeChar - 1);
          DEBUG ((
            DEBUG_VERBOSE,
            "OcTextInputLib: CTRL+%c detected - code %d\n",
            CtrlChar,
            KeyData->Key.UnicodeChar
            ));
        }
      }
    }
    // Handle function keys and scan codes
    else if (KeyData->Key.ScanCode != SCAN_NULL) {
      // Specifically handle function keys that the Shell text editor uses
      switch (KeyData->Key.ScanCode) {
        case SCAN_F1: // Go To Line
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F1 key\n"));
          break;
        case SCAN_F2: // Save File
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F2 key\n"));
          break;
        case SCAN_F3: // Exit
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F3 key\n"));
          break;
        case SCAN_F4: // Search
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F4 key\n"));
          break;
        case SCAN_F5: // Search/Replace
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F5 key\n"));
          break;
        case SCAN_F6: // Cut Line
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F6 key\n"));
          break;
        case SCAN_F7: // Paste Line
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F7 key\n"));
          break;
        case SCAN_F8: // Open File
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F8 key\n"));
          break;
        case SCAN_F9: // File Type
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F9 key\n"));
          break;
        case SCAN_F10: // Help (our new addition)
          DEBUG ((DEBUG_INFO, "OcTextInputLib: F10 key detected - remapping to CTRL+E (Help)\n"));
          // Remap F10 to CTRL+E for programs that expect CTRL+E for help
          KeyData->Key.ScanCode    = SCAN_NULL;
          KeyData->Key.UnicodeChar = 5; // CTRL+E
          break;
        case SCAN_F11: // File Type (duplicate)
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F11 key\n"));
          break;
        case SCAN_F12: // Not used by text editor
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected F12 key\n"));
          break;
        default:
          DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: Detected scan code 0x%X\n", KeyData->Key.ScanCode));
          break;
      }
    }

    // Handle ESC key - keep it as ESC since most programs expect it
    if (KeyData->Key.UnicodeChar == 0x1B) {
      DEBUG ((DEBUG_VERBOSE, "OcTextInputLib: ESC key detected - keeping as ESC\n"));
      // ESC is universally expected to work as ESC in most programs
      // We don't remap it to CTRL+W as that would break normal ESC functionality
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "OcTextInputLib: Key - Unicode: 0x%X, Scan: 0x%X, ShiftState: 0x%X\n",
      KeyData->Key.UnicodeChar,
      KeyData->Key.ScanCode,
      KeyData->KeyState.KeyShiftState
      ));
  }

  return Status;
}

/**
   Compatibility implementation of SetState (does nothing on EFI 1.1).

   @param  This                 Protocol instance pointer.
   @param  KeyToggleState       A pointer to the EFI_KEY_TOGGLE_STATE to set the
                               state for the input device.

   @retval EFI_SUCCESS          The device state was set successfully.
   @retval EFI_UNSUPPORTED      The device does not support the ability to have its state set.
 **/
STATIC
EFI_STATUS
EFIAPI
OcCompatSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  // Always return success to prevent calling applications from failing
  // EFI 1.1 systems don't support toggle state, but we pretend we do
  // to maintain compatibility with applications that expect this to work
  return EFI_SUCCESS;
}/**
   Compatibility implementation of RegisterKeyNotify (does nothing on EFI 1.1).

   @param  This                 Protocol instance pointer.
   @param  KeyData              A pointer to a buffer that defines the keystroke
                               information for the notification function.
   @param  KeyNotificationFunction Points to the function to be called when the key
                               sequence is typed in the specified input device.
   @param  NotifyHandle         Points to the unique handle assigned to the registered notification.

   @retval EFI_SUCCESS          The notification function was registered successfully.
   @retval EFI_OUT_OF_RESOURCES Failed to allocate memory for the dummy handle.
 **/

STATIC
EFI_STATUS
EFIAPI
OcCompatRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  // Not supported on EFI 1.1 systems, but return success with unique handle
  // to avoid breaking applications that expect this to work
  if (NotifyHandle != NULL) {
    // Allocate a unique dummy handle (1 byte is sufficient)
    *NotifyHandle = AllocateZeroPool (1);
    if (*NotifyHandle == NULL) {
      DEBUG ((DEBUG_ERROR, "OcTextInputLib: Failed to allocate dummy notify handle\n"));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/**
   Compatibility implementation of UnregisterKeyNotify (does nothing on EFI 1.1).

   @param  This                 Protocol instance pointer.
   @param  NotificationHandle   The handle of the notification function being unregistered.

   @retval EFI_SUCCESS          The notification function was unregistered successfully.
   @retval EFI_INVALID_PARAMETER The NotificationHandle is not valid.
 **/
STATIC
EFI_STATUS
EFIAPI
OcCompatUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  // Not supported on EFI 1.1 systems, but validate and free the handle to avoid misuse
  if (NotificationHandle == NULL) {
    DEBUG ((DEBUG_WARN, "OcTextInputLib: NULL NotificationHandle passed to UnregisterKeyNotify\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Free the dummy handle that was allocated in RegisterKeyNotify
  FreePool (NotificationHandle);
  return EFI_SUCCESS;
}

/**
   Reset implementation wrapper for SimpleTextInputEx compatibility.

   @param  This                     Protocol instance pointer.
   @param  ExtendedVerification     Indicates that the driver may perform a more
                                   exhaustive verification operation of the device
                                   during reset.

   @retval EFI_SUCCESS              The device was reset.
   @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.
 **/
STATIC
EFI_STATUS
EFIAPI
OcCompatReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  return gST->ConIn->Reset (gST->ConIn, ExtendedVerification);
}

/**
   Initialize the compatibility protocol structure.
 **/
STATIC
VOID
OcInitializeCompatibilityProtocol (
  VOID
  )
{
  // Initialize the compatibility protocol structure
  mCompatSimpleTextInputEx.Reset               = OcCompatReset;
  mCompatSimpleTextInputEx.ReadKeyStrokeEx     = OcCompatReadKeyStrokeEx;
  mCompatSimpleTextInputEx.WaitForKeyEx        = gST->ConIn->WaitForKey;// Reuse the same event
  mCompatSimpleTextInputEx.SetState            = OcCompatSetState;
  mCompatSimpleTextInputEx.RegisterKeyNotify   = OcCompatRegisterKeyNotify;
  mCompatSimpleTextInputEx.UnregisterKeyNotify = OcCompatUnregisterKeyNotify;
}/**
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
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *NativeSimpleTextInputEx;

  // Check if SimpleTextInputEx is already available
  NativeSimpleTextInputEx = NULL;
  Status                  = gBS->HandleProtocol (
                                   gST->ConsoleInHandle,
                                   &gEfiSimpleTextInputExProtocolGuid,
                                   (VOID **)&NativeSimpleTextInputEx
                                   );

  if (!EFI_ERROR (Status)) {
    // Protocol already exists, no need for compatibility
    mOriginalSimpleTextInputEx = NativeSimpleTextInputEx;
    DEBUG ((DEBUG_INFO, "OcTextInputLib: Native SimpleTextInputEx already available\n"));
    return EFI_ALREADY_STARTED;
  }

  // Initialize our compatibility protocol
  OcInitializeCompatibilityProtocol ();

  if (UseLocalRegistration) {
    // WARNING: Local registration is not currently implemented.
    // For OpenShell integration: should use OcRegisterBootServicesProtocol
    // Current behavior: Falls back to standard gBS->InstallProtocolInterface method
    DEBUG ((DEBUG_WARN, "OcTextInputLib: UseLocalRegistration=TRUE, but local registration not implemented. Falling back to standard protocol installation.\n"));
    // TODO: Call OcRegisterBootServicesProtocol when available
  }

  // Install the compatibility protocol on the console input handle
  // NOTE: This always uses standard method regardless of UseLocalRegistration flag
  Status = gBS->InstallProtocolInterface (
                  &gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mCompatSimpleTextInputEx
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OcTextInputLib: Failed to install compatibility protocol - %r\n", Status));
    return Status;
  }

  mProtocolInstalled = TRUE;
  DEBUG ((DEBUG_INFO, "OcTextInputLib: Successfully installed SimpleTextInputEx compatibility protocol\n"));
  return EFI_SUCCESS;
}

/**
   Install SimpleTextInputEx compatibility protocol on the console input handle.

   This function checks if SimpleTextInputEx is already available, and if not,
   installs a compatibility version that wraps SimpleTextInput protocol.
   Uses standard gBS->InstallProtocolInterface for driver use.

   @retval EFI_SUCCESS          Protocol installed successfully or already present
   @retval EFI_ALREADY_STARTED  Protocol already exists, no action taken
   @retval Others               Installation failed
 **/
EFI_STATUS
OcInstallSimpleTextInputEx (
  VOID
  )
{
  return OcInstallSimpleTextInputExInternal (FALSE);
}

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
  )
{
  EFI_STATUS  Status;

  // Only uninstall if we installed the compatibility version
  if (mOriginalSimpleTextInputEx != NULL) {
    // Native protocol was present, nothing to uninstall
    DEBUG ((DEBUG_INFO, "OcTextInputLib: Native protocol was present, nothing to uninstall\n"));
    return EFI_NOT_FOUND;
  }

  if (!mProtocolInstalled) {
    // We didn't install anything
    DEBUG ((DEBUG_INFO, "OcTextInputLib: No compatibility protocol was installed\n"));
    return EFI_NOT_FOUND;
  }

  Status = gBS->UninstallProtocolInterface (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &mCompatSimpleTextInputEx
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OcTextInputLib: Failed to uninstall compatibility protocol - %r\n", Status));
  } else {
    mProtocolInstalled = FALSE;
    DEBUG ((DEBUG_INFO, "OcTextInputLib: Successfully uninstalled compatibility protocol\n"));
  }

  return Status;
}

/**
   Check if SimpleTextInputEx protocol is available on console input handle.

   @retval TRUE   SimpleTextInputEx is available (native or compatibility)
   @retval FALSE  SimpleTextInputEx is not available
 **/
STATIC
BOOLEAN
OcIsSimpleTextInputExAvailable (
  VOID
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleTextInputEx;

  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleTextInputEx
                  );

  return !EFI_ERROR (Status);
}

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
  )
{
  DEBUG ((DEBUG_INFO, "=== OcTextInputLib: CTRL Key Detection Test ===\n"));
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputLib: SimpleTextInputEx Available: %s\n",
    OcIsSimpleTextInputExAvailable () ? L"YES" : L"NO"
    ));

  // Test Shell text editor function key assignments and our remapping
  DEBUG ((DEBUG_INFO, "OcTextInputLib: Key Remapping Behavior:\n"));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F10 (0x%02X) -> REMAPPED TO -> CTRL+E (Help)\n", SCAN_F10));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: ESC (0x1B) -> KEPT AS -> ESC (Universal)\n"));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: \n"));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: Shell Text Editor Key Assignments:\n"));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F1  (0x%02X) -> Go To Line\n", SCAN_F1));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F2  (0x%02X) -> Save File\n", SCAN_F2));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F3  (0x%02X) -> Exit\n", SCAN_F3));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F4  (0x%02X) -> Search\n", SCAN_F4));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F5  (0x%02X) -> Search/Replace\n", SCAN_F5));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F6  (0x%02X) -> Cut Line\n", SCAN_F6));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F7  (0x%02X) -> Paste Line\n", SCAN_F7));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F8  (0x%02X) -> Open File\n", SCAN_F8));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F9  (0x%02X) -> File Type\n", SCAN_F9));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F10 (0x%02X) -> Help (NEW)\n", SCAN_F10));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F11 (0x%02X) -> File Type\n", SCAN_F11));

  // Test expected CTRL codes for Shell text editor
  DEBUG ((DEBUG_INFO, "OcTextInputLib: Shell Text Editor Key Mappings:\n"));
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputLib: CTRL+E (Help) -> Unicode char 5 (0x05) -> %a\n",
    GetControlCharShellFunction (5) ? GetControlCharShellFunction (5) : "MainCommandDisplayHelp"
    ));
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputLib: CTRL+W (Exit Help) -> Unicode char 23 (0x17) -> %a\n",
    GetControlCharShellFunction (23) ? GetControlCharShellFunction (23) : "ExitHelpContext"
    ));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: ESC (Exit Help) -> Unicode char 27 (0x1B) -> ExitHelpContext\n"));
  DEBUG ((DEBUG_INFO, "OcTextInputLib: F10 (Help) -> Scan code 0x%02X -> MainCommandDisplayHelp\n", SCAN_F10));

  // Show comprehensive CTRL mapping table
  DEBUG ((DEBUG_INFO, "OcTextInputLib: Complete Control Character Support:\n"));
  for (UINTN Index = 0; Index < ARRAY_SIZE (mControlCharTable); Index++) {
    if (mControlCharTable[Index].ControlChar <= 0x1A) {
      // CTRL+A through CTRL+Z
      DEBUG ((
        DEBUG_VERBOSE,
        "OcTextInputLib: %a -> %a\n",
        mControlCharTable[Index].Description,
        mControlCharTable[Index].ShellFunction
        ));
    }
  }

  // Show the current protocol status
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleTextInputEx;

  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleTextInputEx
                  );

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputLib: Using %s SimpleTextInputEx protocol\n",
      SimpleTextInputEx == &mCompatSimpleTextInputEx ? L"COMPATIBILITY" : L"NATIVE"
      ));
  } else {
    DEBUG ((DEBUG_WARN, "OcTextInputLib: No SimpleTextInputEx protocol available!\n"));
  }

  DEBUG ((DEBUG_INFO, "=== Test Complete ===\n"));
  return EFI_SUCCESS;
}
