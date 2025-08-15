/*++

Copyright (c) 2025, Enhanced Shell Compatibility Layer
All rights reserved.

Module Name:
  SimpleTextInputExCompat.c

Abstract:
  Compatibility layer for SimpleTextInputEx protocol on EFI 1.1 systems
  Enables OpenShell edit command to work on legacy systems

--*/

#include <Uefi.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
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
extern EFI_GUID gEfiSimpleTextInProtocolGuid;
extern EFI_GUID gEfiSimpleTextInputExProtocolGuid;

//
// Key state definitions for EFI 1.1 compatibility
// These may not be properly defined in older headers
//
#ifndef EFI_LEFT_CONTROL_PRESSED
#define EFI_LEFT_CONTROL_PRESSED    0x00000004
#endif

#ifndef EFI_RIGHT_CONTROL_PRESSED
#define EFI_RIGHT_CONTROL_PRESSED   0x00000008
#endif

#ifndef EFI_LEFT_ALT_PRESSED
#define EFI_LEFT_ALT_PRESSED        0x00000010
#endif

#ifndef EFI_RIGHT_ALT_PRESSED
#define EFI_RIGHT_ALT_PRESSED       0x00000020
#endif

#ifndef EFI_LEFT_SHIFT_PRESSED
#define EFI_LEFT_SHIFT_PRESSED      0x00000001
#endif

#ifndef EFI_RIGHT_SHIFT_PRESSED
#define EFI_RIGHT_SHIFT_PRESSED     0x00000002
#endif

#ifndef EFI_LEFT_LOGO_PRESSED
#define EFI_LEFT_LOGO_PRESSED       0x00000040
#endif

#ifndef EFI_RIGHT_LOGO_PRESSED
#define EFI_RIGHT_LOGO_PRESSED      0x00000080
#endif

//
// Control character definitions matching OpenShell text editor usage
//
#define CTRL_A              0x01    // (Not used in OpenShell)
#define CTRL_B              0x02    // (Not used in OpenShell)
#define CTRL_C              0x03    // (Not used in OpenShell)
#define CTRL_D              0x04    // (Not used in OpenShell)
#define CTRL_E              0x05    // Display Help (MainCommandDisplayHelp)
#define CTRL_F              0x06    // Search (MainCommandSearch)
#define CTRL_G              0x07    // Go to Line (MainCommandGotoLine)
#define CTRL_H              0x08    // Backspace
#define CTRL_I              0x09    // Tab
#define CTRL_J              0x0A    // New line / Line feed
#define CTRL_K              0x0B    // Cut Line (MainCommandCutLine)
#define CTRL_L              0x0C    // (Not used in OpenShell)
#define CTRL_M              0x0D    // Carriage return
#define CTRL_N              0x0E    // (Not used in OpenShell)
#define CTRL_O              0x0F    // Open File (MainCommandOpenFile)
#define CTRL_P              0x10    // (Not used in OpenShell)
#define CTRL_Q              0x11    // Exit (MainCommandExit)
#define CTRL_R              0x12    // Search & Replace (MainCommandSearchReplace)
#define CTRL_S              0x13    // Save File (MainCommandSaveFile)
#define CTRL_T              0x14    // Switch File Type (MainCommandSwitchFileType)
#define CTRL_U              0x15    // Paste Line (MainCommandPasteLine)
#define CTRL_V              0x16    // (Not used in OpenShell)
#define CTRL_W              0x17    // (Not used in OpenShell)
#define CTRL_X              0x18    // (Not used in OpenShell)
#define CTRL_Y              0x19    // Yank / Redo
#define CTRL_Z              0x1A    // Suspend / Undo
#define CTRL_BACKSLASH      0x1C    // Quit
#define CTRL_BRACKET        0x1D    // Group separator / Escape
#define CTRL_CARET          0x1E    // Record separator
#define CTRL_UNDERSCORE     0x1F    // Unit separator / Undo

//
// Global variables for driver state
//
BOOLEAN  gDriverInitialized = FALSE;
EFI_EVENT gReadyToBootEvent = NULL;

//
// EFI revision definitions
//
#ifndef EFI_2_00_SYSTEM_TABLE_REVISION
#define EFI_2_00_SYSTEM_TABLE_REVISION  ((2 << 16) | (00))
#endif

//
// Helper macros
//
#define COMPAT_TEXT_INPUT_EX_SIGNATURE  SIGNATURE_32('C','T','I','X')

//
// Compatibility layer structure
//
typedef struct {
  UINT32                              Signature;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL   TextInputEx;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      *UnderlyingTextInput;
  EFI_HANDLE                          Handle;
} COMPAT_TEXT_INPUT_EX_PRIVATE;

#define COMPAT_TEXT_INPUT_EX_PRIVATE_FROM_PROTOCOL(a) \
  CR(a, COMPAT_TEXT_INPUT_EX_PRIVATE, TextInputEx, COMPAT_TEXT_INPUT_EX_SIGNATURE)

//
// Protocol function implementations
//

EFI_STATUS
EFIAPI
CompatReset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  IN BOOLEAN                           ExtendedVerification
  )
{
  COMPAT_TEXT_INPUT_EX_PRIVATE *Private;
  
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Private = COMPAT_TEXT_INPUT_EX_PRIVATE_FROM_PROTOCOL(This);
  
  if (Private == NULL || Private->Signature != COMPAT_TEXT_INPUT_EX_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "STX: Invalid private structure in CompatReset\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  if (Private->UnderlyingTextInput == NULL) {
    DEBUG ((DEBUG_ERROR, "STX: Underlying TextInput is NULL\n"));
    return EFI_DEVICE_ERROR;
  }
  
  // Reset underlying simple text input
  return Private->UnderlyingTextInput->Reset(
           Private->UnderlyingTextInput,
           ExtendedVerification
         );
}

EFI_STATUS
EFIAPI
CompatReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  COMPAT_TEXT_INPUT_EX_PRIVATE *Private;
  EFI_INPUT_KEY                Key;
  EFI_STATUS                   Status;
  
  if (This == NULL || KeyData == NULL) {
    DEBUG ((DEBUG_ERROR, "STX: CompatReadKeyStrokeEx: Invalid parameters\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  Private = COMPAT_TEXT_INPUT_EX_PRIVATE_FROM_PROTOCOL(This);
  
  if (Private == NULL || Private->Signature != COMPAT_TEXT_INPUT_EX_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "STX: CompatReadKeyStrokeEx: Invalid private structure\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  if (Private->UnderlyingTextInput == NULL) {
    DEBUG ((DEBUG_ERROR, "STX: CompatReadKeyStrokeEx: Underlying TextInput is NULL\n"));
    return EFI_DEVICE_ERROR;
  }
  
  DEBUG ((DEBUG_INFO, "STX: CompatReadKeyStrokeEx: Called on handle %p\n", Private->Handle));
  
  // Initialize KeyData structure
  ZeroMem (KeyData, sizeof(EFI_KEY_DATA));
  
  // Read from underlying protocol
  Status = Private->UnderlyingTextInput->ReadKeyStroke(
             Private->UnderlyingTextInput,
             &Key
           );
  
  if (!EFI_ERROR (Status)) {
    // Convert EFI_INPUT_KEY to EFI_KEY_DATA
    KeyData->Key = Key;
    
    // Initialize key state
    KeyData->KeyState.KeyShiftState = 0;
    KeyData->KeyState.KeyToggleState = 0;
    
    // Enhanced control key handling for EFI 1.1 compatibility
    // Handle all possible control character combinations (0x01-0x1F)
    if (KeyData->Key.UnicodeChar >= 0x01 && KeyData->Key.UnicodeChar <= 0x1F) {
      // This is a control character - set appropriate shift state
      KeyData->KeyState.KeyShiftState = EFI_LEFT_CONTROL_PRESSED;
      
      // Detailed logging for all control characters
      switch (KeyData->Key.UnicodeChar) {
        case CTRL_A:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+A (Select All/Beginning) detected\n"));
          break;
        case CTRL_B:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+B (Bold/Backward) detected\n"));
          break;
        case CTRL_C:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+C (Copy/Cancel) detected\n"));
          break;
        case CTRL_D:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+D (Delete/EOF) detected\n"));
          break;
        case CTRL_E:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+E (OpenShell Display Help) detected\n"));
          break;
        case CTRL_F:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+F (OpenShell Search) detected\n"));
          break;
        case CTRL_G:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+G (OpenShell Go to Line) detected\n"));
          break;
        case CTRL_H:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+H (Backspace) detected\n"));
          break;
        case CTRL_I:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+I (Tab) detected\n"));
          break;
        case CTRL_J:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+J (New line/LF) detected\n"));
          break;
        case CTRL_K:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+K (OpenShell Cut Line) detected\n"));
          break;
        case CTRL_L:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+L (Not used in OpenShell) detected\n"));
          break;
        case CTRL_M:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+M (Carriage return) detected\n"));
          break;
        case CTRL_N:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+N (Not used in OpenShell) detected\n"));
          break;
        case CTRL_O:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+O (OpenShell Open File) detected\n"));
          break;
        case CTRL_P:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+P (Not used in OpenShell) detected\n"));
          break;
        case CTRL_Q:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+Q (OpenShell Exit) detected\n"));
          break;
        case CTRL_R:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+R (OpenShell Search & Replace) detected\n"));
          break;
        case CTRL_S:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+S (OpenShell Save File) detected\n"));
          break;
        case CTRL_T:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+T (OpenShell Switch File Type) detected\n"));
          break;
        case CTRL_U:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+U (OpenShell Paste Line) detected\n"));
          break;
        case CTRL_V:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+V (Paste/Quote next) detected\n"));
          break;
        case CTRL_W:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+W (Kill word/Close window) detected\n"));
          break;
        case CTRL_X:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+X (Cut/Exit) detected\n"));
          break;
        case CTRL_Y:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+Y (Yank/Redo) detected\n"));
          break;
        case CTRL_Z:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+Z (Suspend/Undo) detected\n"));
          break;
        case CTRL_BACKSLASH:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+\\ (Quit) detected\n"));
          break;
        case CTRL_BRACKET:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+] (Escape) detected\n"));
          break;
        case CTRL_CARET:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+^ (Record separator) detected\n"));
          break;
        case CTRL_UNDERSCORE:
          DEBUG ((DEBUG_INFO, "STX: Ctrl+_ (Undo) detected\n"));
          break;
        default:
          DEBUG ((DEBUG_INFO, "STX: Control character 0x%02X (Ctrl+%c) detected\n", 
                 KeyData->Key.UnicodeChar, KeyData->Key.UnicodeChar + 0x40));
          break;
      }
    } else {
      // Handle other special cases and key combinations
      switch (KeyData->Key.UnicodeChar) {
        case 0x1B: // ESC key
          DEBUG ((DEBUG_INFO, "STX: ESC key detected\n"));
          break;
        case 0x7F: // DEL character
          DEBUG ((DEBUG_INFO, "STX: DEL character detected\n"));
          break;
        default:
          // Check if this might be an Alt combination (high bit set)
          if (KeyData->Key.UnicodeChar >= 0x80 && KeyData->Key.UnicodeChar <= 0xFF) {
            KeyData->KeyState.KeyShiftState = EFI_LEFT_ALT_PRESSED;
            DEBUG ((DEBUG_INFO, "STX: Alt combination detected - Unicode=0x%04X\n", 
                   KeyData->Key.UnicodeChar));
          }
          break;
      }
    }
    
    // Enhanced scan code handling for special keys and function keys
    switch (KeyData->Key.ScanCode) {
      case SCAN_ESC:
        DEBUG ((DEBUG_INFO, "STX: ESC scan code detected\n"));
        break;
      
      // Function keys F1-F12 (OpenShell text editor usage)
      case SCAN_F1:
        DEBUG ((DEBUG_INFO, "STX: F1 (OpenShell Go to Line) detected\n"));
        break;
      case SCAN_F2:
        DEBUG ((DEBUG_INFO, "STX: F2 (OpenShell Save File) detected\n"));
        break;
      case SCAN_F3:
        DEBUG ((DEBUG_INFO, "STX: F3 (OpenShell Exit) detected\n"));
        break;
      case SCAN_F4:
        DEBUG ((DEBUG_INFO, "STX: F4 (OpenShell Search) detected\n"));
        break;
      case SCAN_F5:
        DEBUG ((DEBUG_INFO, "STX: F5 (OpenShell Search & Replace) detected\n"));
        break;
      case SCAN_F6:
        DEBUG ((DEBUG_INFO, "STX: F6 (OpenShell Cut Line) detected\n"));
        break;
      case SCAN_F7:
        DEBUG ((DEBUG_INFO, "STX: F7 (OpenShell Paste Line) detected\n"));
        break;
      case SCAN_F8:
        DEBUG ((DEBUG_INFO, "STX: F8 (OpenShell Open File) detected\n"));
        break;
      case SCAN_F9:
        DEBUG ((DEBUG_INFO, "STX: F9 (OpenShell File Type) detected\n"));
        break;
      case SCAN_F10:
        DEBUG ((DEBUG_INFO, "STX: F10 (Not used in OpenShell) detected\n"));
        break;
      case SCAN_F11:
        DEBUG ((DEBUG_INFO, "STX: F11 (OpenShell File Type) detected\n"));
        break;
      case SCAN_F12:
        DEBUG ((DEBUG_INFO, "STX: F12 (Not used in OpenShell) detected\n"));
        break;
      
      // Arrow keys
      case SCAN_UP:
        DEBUG ((DEBUG_INFO, "STX: Arrow UP detected\n"));
        break;
      case SCAN_DOWN:
        DEBUG ((DEBUG_INFO, "STX: Arrow DOWN detected\n"));
        break;
      case SCAN_LEFT:
        DEBUG ((DEBUG_INFO, "STX: Arrow LEFT detected\n"));
        break;
      case SCAN_RIGHT:
        DEBUG ((DEBUG_INFO, "STX: Arrow RIGHT detected\n"));
        break;
      
      // Navigation keys
      case SCAN_HOME:
        DEBUG ((DEBUG_INFO, "STX: HOME key detected\n"));
        break;
      case SCAN_END:
        DEBUG ((DEBUG_INFO, "STX: END key detected\n"));
        break;
      case SCAN_PAGE_UP:
        DEBUG ((DEBUG_INFO, "STX: PAGE UP detected\n"));
        break;
      case SCAN_PAGE_DOWN:
        DEBUG ((DEBUG_INFO, "STX: PAGE DOWN detected\n"));
        break;
      case SCAN_INSERT:
        DEBUG ((DEBUG_INFO, "STX: INSERT key detected\n"));
        break;
      case SCAN_DELETE:
        DEBUG ((DEBUG_INFO, "STX: DELETE key detected\n"));
        break;
      
      default:
        // No special scan code handling needed
        break;
    }
    
    DEBUG ((DEBUG_INFO, "STX: Key processed - Unicode=0x%04X, Scan=0x%04X, ShiftState=0x%08X\n", 
           KeyData->Key.UnicodeChar, KeyData->Key.ScanCode, KeyData->KeyState.KeyShiftState));
  } else if (Status != EFI_NOT_READY) {
    DEBUG ((DEBUG_WARN, "STX: Underlying ReadKeyStroke failed: %r\n", Status));
  }
  
  return Status;
}

EFI_STATUS
EFIAPI
CompatSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  IN EFI_KEY_TOGGLE_STATE              *KeyToggleState
  )
{
  // Not supported on EFI 1.1 systems
  // Return success to maintain compatibility
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CompatRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  IN  EFI_KEY_DATA                      *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION           KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  // Not supported on EFI 1.1 systems, but return success with dummy handle
  // to avoid breaking applications that expect this to work
  if (NotifyHandle != NULL) {
    *NotifyHandle = (VOID*)0xDEADBEEF; // Dummy handle
  }
  DEBUG ((DEBUG_INFO, "STX: Returning success with dummy handle (EFI 1.1 limitation)\n"));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CompatUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  IN VOID                              *NotificationHandle
  )
{
  // Not supported on EFI 1.1 systems, but return success to avoid breaking apps
  DEBUG ((DEBUG_INFO, "STX: Returning success (EFI 1.1 limitation)\n"));
  return EFI_SUCCESS;
}

//
// Install compatibility layer
//
EFI_STATUS
InstallSimpleTextInputExCompat (
  IN EFI_HANDLE Handle
  )
{
  EFI_STATUS                       Status;
  COMPAT_TEXT_INPUT_EX_PRIVATE     *Private;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *TextInput;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *TextInputEx;
  
  if (Handle == NULL) {
    DEBUG ((DEBUG_ERROR, "STX: Invalid handle provided\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  DEBUG ((DEBUG_INFO, "STX: Checking handle %p\n", Handle));
  
  // Check if SimpleTextInputEx already exists
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID**)&TextInputEx
                );
  
  if (!EFI_ERROR (Status)) {
    // Protocol already exists, no need for compatibility layer
    DEBUG ((DEBUG_INFO, "STX: SimpleTextInputEx already exists on handle %p\n", Handle));
    return EFI_ALREADY_STARTED;
  }
  
  DEBUG ((DEBUG_INFO, "STX: SimpleTextInputEx not found, Status=%r. Installing compatibility layer...\n", Status));
  
  // Get SimpleTextInput protocol
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID**)&TextInput
                );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "STX: Failed to get SimpleTextInput protocol on handle %p: %r\n", Handle, Status));
    return Status;
  }
  
  if (TextInput == NULL) {
    DEBUG ((DEBUG_ERROR, "STX: SimpleTextInput protocol is NULL on handle %p\n", Handle));
    return EFI_NOT_FOUND;
  }
  
  DEBUG ((DEBUG_INFO, "STX: Found SimpleTextInput protocol %p on handle %p\n", TextInput, Handle));
  
  // Allocate private structure
  Private = AllocateZeroPool (sizeof(COMPAT_TEXT_INPUT_EX_PRIVATE));
  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "STX: Failed to allocate memory for private structure\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  
  DEBUG ((DEBUG_INFO, "STX: Allocated private structure %p\n", Private));
  
  // Initialize private structure
  Private->Signature = COMPAT_TEXT_INPUT_EX_SIGNATURE;
  Private->UnderlyingTextInput = TextInput;
  Private->Handle = Handle;
  
  // Setup protocol functions
  Private->TextInputEx.Reset = CompatReset;
  Private->TextInputEx.ReadKeyStrokeEx = CompatReadKeyStrokeEx;
  Private->TextInputEx.WaitForKeyEx = TextInput->WaitForKey; // Reuse wait event
  Private->TextInputEx.SetState = CompatSetState;
  Private->TextInputEx.RegisterKeyNotify = CompatRegisterKeyNotify;
  Private->TextInputEx.UnregisterKeyNotify = CompatUnregisterKeyNotify;
  
  DEBUG ((DEBUG_INFO, "STX: Initialized protocol functions for handle %p\n", Handle));
  
  // Install the protocol using InstallMultipleProtocolInterfaces for better compatibility
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Private->TextInputEx,
                  NULL
                );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "STX: Failed to install SimpleTextInputEx protocol on handle %p: %r\n", Handle, Status));
    FreePool (Private);
    return Status;
  }
  
  DEBUG ((DEBUG_INFO, "STX: SimpleTextInputEx compatibility layer installed successfully on handle %p\n", Handle));
  
  // Verify installation by trying to retrieve the protocol
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *TestProtocol;
  Status = gBS->HandleProtocol(
                  Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID**)&TestProtocol
                );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "STX: Verification failed - cannot retrieve installed protocol: %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "STX: Verification successful - protocol %p installed and accessible\n", TestProtocol));
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
  EFI_STATUS Status;
  EFI_HANDLE *HandleBuffer;
  UINTN      HandleCount;
  UINTN      Index;
  UINTN      SuccessCount = 0;
  BOOLEAN    ProtocolMissing = FALSE;
  
  if (gDriverInitialized) {
    DEBUG ((DEBUG_INFO, "STX: Already initialized, skipping\n"));
    return EFI_SUCCESS;
  }
  
  DEBUG ((DEBUG_INFO, "STX: === Performing SimpleTextInputEx Compatibility Installation ===\n"));
  
  // Check if ANY SimpleTextInputEx protocols exist in the system
  Status = gBS->LocateHandleBuffer(
                  ByProtocol,
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                );
  
  if (EFI_ERROR (Status) || HandleCount == 0) {
    DEBUG ((DEBUG_INFO, "STX: No SimpleTextInputEx protocols found in system (Status=%r, Count=%d)\n", Status, HandleCount));
    DEBUG ((DEBUG_INFO, "STX: This indicates the system needs our compatibility layer\n"));
    ProtocolMissing = TRUE;
  } else {
    DEBUG ((DEBUG_INFO, "STX: Found %d existing SimpleTextInputEx protocols\n", HandleCount));
    gBS->FreePool (HandleBuffer);
  }
  
  // Install compatibility layer if protocols are missing OR on mixed EFI systems
  if (ProtocolMissing || 
      gST->Hdr.Revision < EFI_2_00_SYSTEM_TABLE_REVISION ||
      (gST->RuntimeServices != NULL && 
       gST->RuntimeServices->Hdr.Revision < EFI_2_00_SYSTEM_TABLE_REVISION)) {
    
    DEBUG ((DEBUG_INFO, "STX: Installing compatibility layer due to missing protocols or legacy EFI components\n"));
    
    // First, try installing on console input handle (most important)
    if (gST->ConsoleInHandle != NULL) {
      DEBUG ((DEBUG_INFO, "STX: Attempting installation on console input handle %p\n", gST->ConsoleInHandle));
      Status = InstallSimpleTextInputExCompat (gST->ConsoleInHandle);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "STX: *** SUCCESS: Installed on console input handle ***\n"));
        SuccessCount++;
      } else if (Status != EFI_ALREADY_STARTED) {
        DEBUG ((DEBUG_WARN, "STX: Failed to install on console handle: %r\n", Status));
      } else {
        DEBUG ((DEBUG_INFO, "STX: Console handle already has SimpleTextInputEx\n"));
      }
    } else {
      DEBUG ((DEBUG_WARN, "STX: Console input handle is NULL!\n"));
    }
    
    // Try all handles with SimpleTextInput protocol
    Status = gBS->LocateHandleBuffer(
                    ByProtocol,
                    &gEfiSimpleTextInProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                  );
    
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "STX: Found %d handles with SimpleTextInput protocol\n", HandleCount));
      
      for (Index = 0; Index < HandleCount; Index++) {
        DEBUG ((DEBUG_INFO, "STX: Processing handle %d/%d: %p\n", Index + 1, HandleCount, HandleBuffer[Index]));
        Status = InstallSimpleTextInputExCompat (HandleBuffer[Index]);
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "STX: *** SUCCESS: Installed compatibility layer on handle %d (%p) ***\n", Index, HandleBuffer[Index]));
          SuccessCount++;
        } else if (Status != EFI_ALREADY_STARTED) {
          DEBUG ((DEBUG_WARN, "STX: Failed to install on handle %d (%p): %r\n", Index, HandleBuffer[Index], Status));
        }
      }
      
      gBS->FreePool (HandleBuffer);
    } else {
      DEBUG ((DEBUG_ERROR, "STX: Failed to locate SimpleTextInput handles: %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "STX: System appears to have full EFI 2.0+ support, compatibility layer not needed\n"));
  }
  
  gDriverInitialized = TRUE;
  DEBUG ((DEBUG_INFO, "STX: === Compatibility installation completed: %d successful installations ===\n", SuccessCount));
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
  DEBUG ((DEBUG_INFO, "STX: System is ready to boot, performing late initialization\n"));
  PerformCompatibilityInstallation ();
}

EFI_STATUS
EFIAPI
SimpleTextInputExCompatEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  
  DEBUG ((DEBUG_INFO, "STX: === SimpleTextInputEx Compatibility Driver Starting ===\n"));
  DEBUG ((DEBUG_INFO, "STX: System Table Revision: 0x%08X (%d.%02d)\n", 
         SystemTable->Hdr.Revision, 
         SystemTable->Hdr.Revision >> 16, 
         SystemTable->Hdr.Revision & 0xFFFF));
  
  if (SystemTable->RuntimeServices != NULL) {
    DEBUG ((DEBUG_INFO, "STX: Runtime Services Revision: 0x%08X (%d.%02d)\n", 
           SystemTable->RuntimeServices->Hdr.Revision,
           SystemTable->RuntimeServices->Hdr.Revision >> 16,
           SystemTable->RuntimeServices->Hdr.Revision & 0xFFFF));
  }
  
  if (SystemTable->BootServices != NULL) {
    DEBUG ((DEBUG_INFO, "STX: Boot Services Revision: 0x%08X (%d.%02d)\n", 
           SystemTable->BootServices->Hdr.Revision,
           SystemTable->BootServices->Hdr.Revision >> 16,
           SystemTable->BootServices->Hdr.Revision & 0xFFFF));
  }
  
  // Check if console services are available for immediate installation
  if (SystemTable->ConsoleInHandle != NULL && SystemTable->ConIn != NULL) {
    DEBUG ((DEBUG_INFO, "STX: Console services are available, attempting immediate installation\n"));
    Status = PerformCompatibilityInstallation ();
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "STX: Immediate installation successful\n"));
      return EFI_SUCCESS;
    } else {
      DEBUG ((DEBUG_WARN, "STX: Immediate installation failed: %r, will try deferred installation\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "STX: Console services not ready (ConsoleInHandle=%p, ConIn=%p)\n", 
           SystemTable->ConsoleInHandle, SystemTable->ConIn));
    DEBUG ((DEBUG_INFO, "STX: Will attempt deferred installation via ReadyToBoot event\n"));
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
    DEBUG ((DEBUG_ERROR, "STX: Failed to create ReadyToBoot event: %r\n", Status));
    // Try immediate installation as fallback
    DEBUG ((DEBUG_INFO, "STX: Attempting immediate installation as fallback\n"));
    PerformCompatibilityInstallation ();
  } else {
    DEBUG ((DEBUG_INFO, "STX: ReadyToBoot event created successfully, driver will install protocols when system is ready\n"));
  }

  // Always return success - we want the driver to stay loaded
  DEBUG ((DEBUG_INFO, "STX: SimpleTextInputEx compatibility driver loaded successfully\n"));
  return EFI_SUCCESS;
}
