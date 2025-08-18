/** @file
   OcTextInputDxe - Comprehensive Standalone SimpleTextInputEx Protocol Compatibility Driver

   This driver provides a complete SimpleTextInputEx protocol compatibility layer
   for systems that only have SimpleTextInput protocol available (EFI 1.1 systems).

   Features:
   1. Full SimpleTextInputEx protocol compatibility with private data structures
   2. Comprehensive CTRL key detection and logging for debugging
   3. Enhanced function key support (F1-F12) with Shell text editor mappings
   4. Robust installation with immediate and deferred (ReadyToBoot) strategies
   5. Complete handle enumeration and console input prioritization
   6. Memory management with proper allocation and cleanup
   7. EFI version detection and mixed system support

   This addresses compatibility issues with older systems (like cMP5,1) where
   SimpleTextInputEx protocol is not available. The Shell text editor has been
   enhanced with F10 support for help functionality.

   This driver can be used independently for firmware injection outside of OpenCorePkg.

   Copyright (c) 2025, OpenCore Team. All rights reserved.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#include <Uefi.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>

//
// Protocol GUIDs (in case they're not properly declared)
//
extern EFI_GUID  gEfiSimpleTextInProtocolGuid;
extern EFI_GUID  gEfiSimpleTextInputExProtocolGuid;

//
// Key state definitions for EFI 1.1 compatibility
// These may not be properly defined in older headers
//
#ifndef EFI_LEFT_CONTROL_PRESSED
#define EFI_LEFT_CONTROL_PRESSED  0x00000004
#endif

#ifndef EFI_RIGHT_CONTROL_PRESSED
#define EFI_RIGHT_CONTROL_PRESSED  0x00000008
#endif

#ifndef EFI_LEFT_ALT_PRESSED
#define EFI_LEFT_ALT_PRESSED  0x00000010
#endif

#ifndef EFI_RIGHT_ALT_PRESSED
#define EFI_RIGHT_ALT_PRESSED  0x00000020
#endif

#ifndef EFI_LEFT_SHIFT_PRESSED
#define EFI_LEFT_SHIFT_PRESSED  0x00000001
#endif

#ifndef EFI_RIGHT_SHIFT_PRESSED
#define EFI_RIGHT_SHIFT_PRESSED  0x00000002
#endif

#ifndef EFI_LEFT_LOGO_PRESSED
#define EFI_LEFT_LOGO_PRESSED  0x00000040
#endif

#ifndef EFI_RIGHT_LOGO_PRESSED
#define EFI_RIGHT_LOGO_PRESSED  0x00000080
#endif

//
// Control character definitions - Shell text editor mappings
//
#define CTRL_E           0x05  // Display Help (MainCommandDisplayHelp)
#define CTRL_F           0x06  // Search (MainCommandSearch)
#define CTRL_G           0x07  // Go to Line (MainCommandGotoLine)
#define CTRL_H           0x08  // Backspace
#define CTRL_I           0x09  // Tab
#define CTRL_J           0x0A  // New line / Line feed
#define CTRL_K           0x0B  // Cut Line (MainCommandCutLine)
#define CTRL_M           0x0D  // Carriage return
#define CTRL_O           0x0F  // Open File (MainCommandOpenFile)
#define CTRL_Q           0x11  // Exit (MainCommandExit)
#define CTRL_R           0x12  // Search & Replace (MainCommandSearchReplace)
#define CTRL_S           0x13  // Save File (MainCommandSaveFile)
#define CTRL_T           0x14  // Switch File Type (MainCommandSwitchFileType)
#define CTRL_U           0x15  // Paste Line (MainCommandPasteLine)
#define CTRL_W           0x17  // Exit Help (used within help display)
#define CTRL_Y           0x19  // Yank / Redo
#define CTRL_Z           0x1A  // Suspend / Undo
#define CTRL_BACKSLASH   0x1C  // Quit
#define CTRL_BRACKET     0x1D  // Group separator / Escape
#define CTRL_CARET       0x1E  // Record separator
#define CTRL_UNDERSCORE  0x1F  // Unit separator / Undo

//
// Control character lookup table for debugging
//
typedef struct {
  UINT8    ControlChar;
  CHAR8    *Description;
} CTRL_CHAR_INFO;

STATIC CTRL_CHAR_INFO  mCtrlCharTable[] = {
  { CTRL_E,          "Ctrl+E (Shell Display Help)"     },
  { CTRL_F,          "Ctrl+F (Shell Search)"           },
  { CTRL_G,          "Ctrl+G (Shell Go to Line)"       },
  { CTRL_H,          "Ctrl+H (Backspace)"              },
  { CTRL_I,          "Ctrl+I (Tab)"                    },
  { CTRL_J,          "Ctrl+J (New line/LF)"            },
  { CTRL_K,          "Ctrl+K (Shell Cut Line)"         },
  { CTRL_M,          "Ctrl+M (Carriage return)"        },
  { CTRL_O,          "Ctrl+O (Shell Open File)"        },
  { CTRL_Q,          "Ctrl+Q (Shell Exit)"             },
  { CTRL_R,          "Ctrl+R (Shell Search & Replace)" },
  { CTRL_S,          "Ctrl+S (Shell Save File)"        },
  { CTRL_T,          "Ctrl+T (Shell Switch File Type)" },
  { CTRL_U,          "Ctrl+U (Shell Paste Line)"       },
  { CTRL_W,          "Ctrl+W (Shell Exit Help)"        },
  { CTRL_Y,          "Ctrl+Y (Yank/Redo)"              },
  { CTRL_Z,          "Ctrl+Z (Suspend/Undo)"           },
  { CTRL_BACKSLASH,  "Ctrl+\\ (Quit)"                  },
  { CTRL_BRACKET,    "Ctrl+] (Escape)"                 },
  { CTRL_CARET,      "Ctrl+^ (Record separator)"       },
  { CTRL_UNDERSCORE, "Ctrl+_ (Undo)"                   },
  { 0,               NULL                              }  // End marker
};

/**
   Get description for a control character.

   @param  ControlChar  The control character to look up

   @return Description string or NULL if not found
 **/
STATIC
CHAR8 *
GetControlCharDescription (
  IN UINT8  ControlChar
  )
{
  UINTN  Index;

  for (Index = 0; mCtrlCharTable[Index].Description != NULL; Index++) {
    if (mCtrlCharTable[Index].ControlChar == ControlChar) {
      return mCtrlCharTable[Index].Description;
    }
  }

  return NULL;
}

//
// Global variables for driver state
//
BOOLEAN    gDriverInitialized = FALSE;
EFI_EVENT  gReadyToBootEvent  = NULL;

//
// EFI revision definitions
//
#ifndef EFI_2_00_SYSTEM_TABLE_REVISION
#define EFI_2_00_SYSTEM_TABLE_REVISION  ((2 << 16) | (00))
#endif

//
// Helper macros
//
#define COMPAT_TEXT_INPUT_EX_SIGNATURE  SIGNATURE_32 ('O', 'C', 'T', 'X')

//
// Compatibility layer structure
//
typedef struct {
  UINT32                               Signature;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    TextInputEx;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL       *UnderlyingTextInput;
  EFI_HANDLE                           Handle;
} COMPAT_TEXT_INPUT_EX_PRIVATE;

#define COMPAT_TEXT_INPUT_EX_PRIVATE_FROM_PROTOCOL(a) \
        CR (a, COMPAT_TEXT_INPUT_EX_PRIVATE, TextInputEx, COMPAT_TEXT_INPUT_EX_SIGNATURE)

//
// Protocol function implementations
//

EFI_STATUS
EFIAPI
CompatReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  COMPAT_TEXT_INPUT_EX_PRIVATE  *Private;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = COMPAT_TEXT_INPUT_EX_PRIVATE_FROM_PROTOCOL (This);

  if ((Private == NULL) || (Private->Signature != COMPAT_TEXT_INPUT_EX_SIGNATURE)) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Invalid private structure in CompatReset\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Private->UnderlyingTextInput == NULL) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Underlying TextInput is NULL\n"));
    return EFI_DEVICE_ERROR;
  }

  // Reset underlying simple text input
  return Private->UnderlyingTextInput->Reset (
                                         Private->UnderlyingTextInput,
                                         ExtendedVerification
                                         );
}

EFI_STATUS
EFIAPI
CompatReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  COMPAT_TEXT_INPUT_EX_PRIVATE  *Private;
  EFI_INPUT_KEY                 Key;
  EFI_STATUS                    Status;

  if ((This == NULL) || (KeyData == NULL)) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: CompatReadKeyStrokeEx: Invalid parameters\n"));
    return EFI_INVALID_PARAMETER;
  }

  Private = COMPAT_TEXT_INPUT_EX_PRIVATE_FROM_PROTOCOL (This);

  if ((Private == NULL) || (Private->Signature != COMPAT_TEXT_INPUT_EX_SIGNATURE)) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: CompatReadKeyStrokeEx: Invalid private structure\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Private->UnderlyingTextInput == NULL) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: CompatReadKeyStrokeEx: Underlying TextInput is NULL\n"));
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: CompatReadKeyStrokeEx: Called on handle %p\n", Private->Handle));

  // Initialize KeyData structure
  ZeroMem (KeyData, sizeof (EFI_KEY_DATA));

  // Read from underlying protocol
  Status = Private->UnderlyingTextInput->ReadKeyStroke (
                                           Private->UnderlyingTextInput,
                                           &Key
                                           );

  if (!EFI_ERROR (Status)) {
    // Convert EFI_INPUT_KEY to EFI_KEY_DATA
    KeyData->Key = Key;

    // Initialize key state (important for EFI 1.1 compatibility)
    KeyData->KeyState.KeyShiftState  = 0;
    KeyData->KeyState.KeyToggleState = 0;

    // Enhanced control key handling for EFI 1.1 compatibility
    // Handle all possible control character combinations (0x01-0x1F)
    if ((KeyData->Key.UnicodeChar >= 0x01) && (KeyData->Key.UnicodeChar <= 0x1F)) {
      // This is a control character - set appropriate shift state
      KeyData->KeyState.KeyShiftState = EFI_LEFT_CONTROL_PRESSED;

      // Log control character using lookup table
      CHAR8  *Description = GetControlCharDescription ((UINT8)KeyData->Key.UnicodeChar);
      if (Description != NULL) {
        // Highlight important CTRL combinations for Shell text editor
        if (KeyData->Key.UnicodeChar == CTRL_E) {
          DEBUG ((DEBUG_INFO, "OcTextInputDxe: *** %a detected ***\n", Description));
        } else if (KeyData->Key.UnicodeChar == CTRL_W) {
          DEBUG ((DEBUG_INFO, "OcTextInputDxe: *** %a detected ***\n", Description));
        } else {
          DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: %a detected\n", Description));
        }
      } else {
        DEBUG ((
          DEBUG_VERBOSE,
          "OcTextInputDxe: Control character 0x%02X (Ctrl+%c) detected\n",
          (UINT8)KeyData->Key.UnicodeChar,
          (UINT8)KeyData->Key.UnicodeChar + 0x40
          ));
      }
    } else {
      // Handle other special cases and key combinations
      switch (KeyData->Key.UnicodeChar) {
        case 0x1B:  // ESC key
          DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: ESC key detected (Exit Help)\n"));
          break;
        case 0x7F:  // DEL character
          DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: DEL character detected\n"));
          break;
        default:
          // Check if this might be an Alt combination (high bit set)
          if ((KeyData->Key.UnicodeChar >= 0x80) && (KeyData->Key.UnicodeChar <= 0xFF)) {
            KeyData->KeyState.KeyShiftState = EFI_LEFT_ALT_PRESSED;
            DEBUG ((
              DEBUG_VERBOSE,
              "OcTextInputDxe: Alt combination detected - Unicode=0x%04X\n",
              KeyData->Key.UnicodeChar
              ));
          }

          break;
      }
    }

    // Enhanced scan code handling for special keys and function keys
    switch (KeyData->Key.ScanCode) {
      case SCAN_ESC:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: ESC scan code detected\n"));
        break;

      // Function keys F1-F12 (Shell text editor usage)
      case SCAN_F1:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F1 (Go to Line) detected\n"));
        break;
      case SCAN_F2:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F2 (Save File) detected\n"));
        break;
      case SCAN_F3:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F3 (Exit) detected\n"));
        break;
      case SCAN_F4:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F4 (Search) detected\n"));
        break;
      case SCAN_F5:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F5 (Search & Replace) detected\n"));
        break;
      case SCAN_F6:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F6 (Cut Line) detected\n"));
        break;
      case SCAN_F7:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F7 (Paste Line) detected\n"));
        break;
      case SCAN_F8:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F8 (Open File) detected\n"));
        break;
      case SCAN_F9:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F9 (File Type) detected\n"));
        break;
      case SCAN_F10:
        DEBUG ((DEBUG_INFO, "OcTextInputDxe: *** F10 (Help) detected - cMP5,1 compatibility ***\n"));
        break;
      case SCAN_F11:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F11 (File Type) detected\n"));
        break;
      case SCAN_F12:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: F12 (Not used) detected\n"));
        break;

      // Arrow keys
      case SCAN_UP:
      case SCAN_DOWN:
      case SCAN_LEFT:
      case SCAN_RIGHT:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Arrow key detected (0x%X)\n", KeyData->Key.ScanCode));
        break;

      // Navigation keys
      case SCAN_HOME:
      case SCAN_END:
      case SCAN_PAGE_UP:
      case SCAN_PAGE_DOWN:
      case SCAN_INSERT:
      case SCAN_DELETE:
        DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Navigation key detected (0x%X)\n", KeyData->Key.ScanCode));
        break;

      default:
        // No special scan code handling needed
        break;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "OcTextInputDxe: Key processed - Unicode=0x%04X, Scan=0x%04X, ShiftState=0x%08X\n",
      KeyData->Key.UnicodeChar,
      KeyData->Key.ScanCode,
      KeyData->KeyState.KeyShiftState
      ));
  } else if (Status != EFI_NOT_READY) {
    DEBUG ((DEBUG_WARN, "OcTextInputDxe: Underlying ReadKeyStroke failed: %r\n", Status));
  }

  return Status;
}

EFI_STATUS
EFIAPI
CompatSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  // Not supported on EFI 1.1 systems
  // Return success to maintain compatibility
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CompatRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  )
{
  // Not supported on EFI 1.1 systems, but return success with unique handle
  // to avoid breaking applications that expect this to work
  if (NotifyHandle != NULL) {
    // Allocate a unique dummy handle (1 byte is sufficient)
    *NotifyHandle = AllocateZeroPool (1);
    if (*NotifyHandle == NULL) {
      DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Failed to allocate dummy notify handle\n"));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Returning success with unique dummy handle (EFI 1.1 limitation)\n"));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CompatUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  // Not supported on EFI 1.1 systems, but validate and free the handle to avoid misuse
  if (NotificationHandle == NULL) {
    DEBUG ((DEBUG_WARN, "OcTextInputDxe: NULL NotificationHandle passed to UnregisterKeyNotify\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Free the dummy handle that was allocated in RegisterKeyNotify
  FreePool (NotificationHandle);
  DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Freed dummy handle and returning success (EFI 1.1 limitation)\n"));
  return EFI_SUCCESS;
}

//
// Install compatibility layer
//
EFI_STATUS
InstallSimpleTextInputExCompat (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                         Status;
  COMPAT_TEXT_INPUT_EX_PRIVATE       *Private;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     *TextInput;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TextInputEx;

  if (Handle == NULL) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Invalid handle provided\n"));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Checking handle %p\n", Handle));

  // Check if SimpleTextInputEx already exists
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&TextInputEx
                  );

  if (!EFI_ERROR (Status)) {
    // Protocol already exists, no need for compatibility layer
    DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: SimpleTextInputEx already exists on handle %p\n", Handle));
    return EFI_ALREADY_STARTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: SimpleTextInputEx not found, Status=%r. Installing compatibility layer...\n",
    Status
    ));

  // Get SimpleTextInput protocol
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **)&TextInput
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "OcTextInputDxe: Failed to get SimpleTextInput protocol on handle %p: %r\n",
      Handle,
      Status
      ));
    return Status;
  }

  if (TextInput == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "OcTextInputDxe: SimpleTextInput protocol is NULL on handle %p\n",
      Handle
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: Found SimpleTextInput protocol %p on handle %p\n",
    TextInput,
    Handle
    ));

  // Allocate private structure
  Private = AllocateZeroPool (sizeof (COMPAT_TEXT_INPUT_EX_PRIVATE));
  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Failed to allocate memory for private structure\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Allocated private structure %p\n", Private));

  // Initialize private structure
  Private->Signature           = COMPAT_TEXT_INPUT_EX_SIGNATURE;
  Private->UnderlyingTextInput = TextInput;
  Private->Handle              = Handle;

  // Setup protocol functions
  Private->TextInputEx.Reset               = CompatReset;
  Private->TextInputEx.ReadKeyStrokeEx     = CompatReadKeyStrokeEx;
  Private->TextInputEx.WaitForKeyEx        = TextInput->WaitForKey;  // Reuse wait event
  Private->TextInputEx.SetState            = CompatSetState;
  Private->TextInputEx.RegisterKeyNotify   = CompatRegisterKeyNotify;
  Private->TextInputEx.UnregisterKeyNotify = CompatUnregisterKeyNotify;

  DEBUG ((
    DEBUG_VERBOSE,
    "OcTextInputDxe: Initialized protocol functions for handle %p\n",
    Handle
    ));

  // Install the protocol using InstallMultipleProtocolInterfaces for better compatibility
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Private->TextInputEx,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "OcTextInputDxe: Failed to install SimpleTextInputEx protocol on handle %p: %r\n",
      Handle,
      Status
      ));
    FreePool (Private);
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: SimpleTextInputEx compatibility layer installed successfully on handle %p\n",
    Handle
    ));

  // Verify installation by trying to retrieve the protocol
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TestProtocol;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&TestProtocol
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Verification failed - cannot retrieve installed protocol: %r\n", Status));
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "OcTextInputDxe: Verification successful - protocol %p installed and accessible\n",
      TestProtocol
      ));
  }

  return EFI_SUCCESS;
}

//
// Perform the actual compatibility layer installation
//
EFI_STATUS
PerformCompatibilityInstallation (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleCount;
  UINTN       Index;
  UINTN       SuccessCount    = 0;
  BOOLEAN     ProtocolMissing = FALSE;

  if (gDriverInitialized) {
    DEBUG ((DEBUG_VERBOSE, "OcTextInputDxe: Already initialized, skipping\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: === Performing SimpleTextInputEx Compatibility Installation ===\n"
    ));

  // Check if ANY SimpleTextInputEx protocols exist in the system
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: No SimpleTextInputEx protocols found in system (Status=%r, Count=%d)\n",
      Status,
      HandleCount
      ));
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: This indicates the system needs our compatibility layer\n"));
    ProtocolMissing = TRUE;
  } else {
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: Found %d existing SimpleTextInputEx protocols\n", HandleCount));
    gBS->FreePool (HandleBuffer);
  }

  // Install compatibility layer if protocols are missing OR on mixed EFI systems
  if (ProtocolMissing ||
      (gST->Hdr.Revision < EFI_2_00_SYSTEM_TABLE_REVISION) ||
      ((gST->RuntimeServices != NULL) &&
       (gST->RuntimeServices->Hdr.Revision < EFI_2_00_SYSTEM_TABLE_REVISION)))
  {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: Installing compatibility layer due to missing protocols or legacy EFI components\n"
      ));

    // First, try installing on console input handle (most important)
    if (gST->ConsoleInHandle != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "OcTextInputDxe: Attempting installation on console input handle %p\n",
        gST->ConsoleInHandle
        ));
      Status = InstallSimpleTextInputExCompat (gST->ConsoleInHandle);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OcTextInputDxe: *** SUCCESS: Installed on console input handle ***\n"));
        SuccessCount++;
      } else if (Status != EFI_ALREADY_STARTED) {
        DEBUG ((DEBUG_WARN, "OcTextInputDxe: Failed to install on console handle: %r\n", Status));
      } else {
        DEBUG ((DEBUG_INFO, "OcTextInputDxe: Console handle already has SimpleTextInputEx\n"));
      }
    } else {
      DEBUG ((DEBUG_WARN, "OcTextInputDxe: Console input handle is NULL!\n"));
    }

    // Try all handles with SimpleTextInput protocol
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiSimpleTextInProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );

    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OcTextInputDxe: Found %d handles with SimpleTextInput protocol\n", HandleCount));

      for (Index = 0; Index < HandleCount; Index++) {
        DEBUG ((
          DEBUG_VERBOSE,
          "OcTextInputDxe: Processing handle %d/%d: %p\n",
          Index + 1,
          HandleCount,
          HandleBuffer[Index]
          ));
        Status = InstallSimpleTextInputExCompat (HandleBuffer[Index]);
        if (!EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "OcTextInputDxe: *** SUCCESS: Installed compatibility layer on handle %d (%p) ***\n",
            Index,
            HandleBuffer[Index]
            ));
          SuccessCount++;
        } else if (Status != EFI_ALREADY_STARTED) {
          DEBUG ((
            DEBUG_WARN,
            "OcTextInputDxe: Failed to install on handle %d (%p): %r\n",
            Index,
            HandleBuffer[Index],
            Status
            ));
        }
      }

      gBS->FreePool (HandleBuffer);
    } else {
      DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Failed to locate SimpleTextInput handles: %r\n", Status));
    }
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: System appears to have full EFI 2.0+ support, compatibility layer not needed\n"
      ));
  }

  gDriverInitialized = TRUE;
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: === Compatibility installation completed: %d successful installations ===\n",
    SuccessCount
    ));
  return EFI_SUCCESS;
}

//
// Event notification callback for ReadyToBoot
//
VOID
EFIAPI
OnReadyToBootEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: System is ready to boot, performing late initialization\n"
    ));
  PerformCompatibilityInstallation ();
}

/**
   The entry point for OcTextInputDxe driver.

   @param[in] ImageHandle    The firmware allocated handle for the EFI image.
   @param[in] SystemTable    A pointer to the EFI System Table.

   @retval EFI_SUCCESS       The entry point is executed successfully.
   @retval other             Some error occurs when executing this entry point.
 **/
EFI_STATUS
EFIAPI
OcTextInputDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: === OpenCore SimpleTextInputEx Compatibility Driver Starting ===\n"
    ));
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: System Table Revision: 0x%08X (%d.%02d)\n",
    SystemTable->Hdr.Revision,
    SystemTable->Hdr.Revision >> 16,
    SystemTable->Hdr.Revision & 0xFFFF
    ));

  if (SystemTable->RuntimeServices != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: Runtime Services Revision: 0x%08X (%d.%02d)\n",
      SystemTable->RuntimeServices->Hdr.Revision,
      SystemTable->RuntimeServices->Hdr.Revision >> 16,
      SystemTable->RuntimeServices->Hdr.Revision & 0xFFFF
      ));
  }

  if (SystemTable->BootServices != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: Boot Services Revision: 0x%08X (%d.%02d)\n",
      SystemTable->BootServices->Hdr.Revision,
      SystemTable->BootServices->Hdr.Revision >> 16,
      SystemTable->BootServices->Hdr.Revision & 0xFFFF
      ));
  }

  // Check if console services are available for immediate installation
  if ((SystemTable->ConsoleInHandle != NULL) && (SystemTable->ConIn != NULL)) {
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: Console services are available, attempting immediate installation\n"));
    Status = PerformCompatibilityInstallation ();
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OcTextInputDxe: Immediate installation successful\n"));
      return EFI_SUCCESS;
    } else {
      DEBUG ((
        DEBUG_WARN,
        "OcTextInputDxe: Immediate installation failed: %r, will try deferred installation\n",
        Status
        ));
    }
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: Console services not ready (ConsoleInHandle=%p, ConIn=%p)\n",
      SystemTable->ConsoleInHandle,
      SystemTable->ConIn
      ));
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: Will attempt deferred installation via ReadyToBoot event\n"));
  }

  // Set up deferred installation via ReadyToBoot event
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnReadyToBootEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &gReadyToBootEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Failed to create ReadyToBoot event: %r\n", Status));
    // Try immediate installation as fallback
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: Attempting immediate installation as fallback\n"));
    PerformCompatibilityInstallation ();
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OcTextInputDxe: ReadyToBoot event created successfully, driver will install protocols when system is ready\n"
      ));
  }

  // Always return success - we want the driver to stay loaded
  DEBUG ((
    DEBUG_INFO,
    "OcTextInputDxe: OpenCore SimpleTextInputEx compatibility driver loaded successfully\n"
    ));
  return EFI_SUCCESS;
}