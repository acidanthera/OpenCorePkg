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
#include <Library/OcTextInputLib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>

//
// Global variables for driver state
//
BOOLEAN    gDriverInitialized = FALSE;
EFI_EVENT  gReadyToBootEvent  = NULL;

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
// Protocol function implementations (all delegated to library functions)
//

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

  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: SimpleTextInputEx not found, Status=%r. Installing compatibility layer...\n",
     Status
    )
    );

  // Get SimpleTextInput protocol
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **)&TextInput
                  );

  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "OcTextInputDxe: Failed to get SimpleTextInput protocol on handle %p: %r\n",
       Handle,
       Status
      )
      );
    return Status;
  }

  if (TextInput == NULL) {
    DEBUG (
      (
       DEBUG_ERROR,
       "OcTextInputDxe: SimpleTextInput protocol is NULL on handle %p\n",
       Handle
      )
      );
    return EFI_NOT_FOUND;
  }

  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: Found SimpleTextInput protocol %p on handle %p\n",
     TextInput,
     Handle
    )
    );

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

  // Setup protocol functions - use library functions directly
  Private->TextInputEx.Reset               = OcCompatReset;
  Private->TextInputEx.ReadKeyStrokeEx     = OcCompatReadKeyStrokeEx;
  Private->TextInputEx.WaitForKeyEx        = TextInput->WaitForKey;  // Reuse wait event
  Private->TextInputEx.SetState            = OcCompatSetState;
  Private->TextInputEx.RegisterKeyNotify   = OcCompatRegisterKeyNotify;
  Private->TextInputEx.UnregisterKeyNotify = OcCompatUnregisterKeyNotify;

  DEBUG (
    (
     DEBUG_VERBOSE,
     "OcTextInputDxe: Initialized protocol functions for handle %p\n",
     Handle
    )
    );

  // Install the protocol using InstallMultipleProtocolInterfaces for better compatibility
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Private->TextInputEx,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "OcTextInputDxe: Failed to install SimpleTextInputEx protocol on handle %p: %r\n",
       Handle,
       Status
      )
      );
    FreePool (Private);
    return Status;
  }

  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: SimpleTextInputEx compatibility layer installed successfully on handle %p\n",
     Handle
    )
    );

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
    DEBUG (
      (
       DEBUG_VERBOSE,
       "OcTextInputDxe: Verification successful - protocol %p installed and accessible\n",
       TestProtocol
      )
      );
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

  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: === Performing SimpleTextInputEx Compatibility Installation ===\n"
    )
    );

  // Check if ANY SimpleTextInputEx protocols exist in the system
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: No SimpleTextInputEx protocols found in system (Status=%r, Count=%d)\n",
       Status,
       HandleCount
      )
      );
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
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: Installing compatibility layer due to missing protocols or legacy EFI components\n"
      )
      );

    // First, try installing on console input handle (most important)
    if (gST->ConsoleInHandle != NULL) {
      DEBUG (
        (
         DEBUG_INFO,
         "OcTextInputDxe: Attempting installation on console input handle %p\n",
         gST->ConsoleInHandle
        )
        );
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
        DEBUG (
          (
           DEBUG_VERBOSE,
           "OcTextInputDxe: Processing handle %d/%d: %p\n",
           Index + 1,
           HandleCount,
           HandleBuffer[Index]
          )
          );
        Status = InstallSimpleTextInputExCompat (HandleBuffer[Index]);
        if (!EFI_ERROR (Status)) {
          DEBUG (
            (
             DEBUG_INFO,
             "OcTextInputDxe: *** SUCCESS: Installed compatibility layer on handle %d (%p) ***\n",
             Index,
             HandleBuffer[Index]
            )
            );
          SuccessCount++;
        } else if (Status != EFI_ALREADY_STARTED) {
          DEBUG (
            (
             DEBUG_WARN,
             "OcTextInputDxe: Failed to install on handle %d (%p): %r\n",
             Index,
             HandleBuffer[Index],
             Status
            )
            );
        }
      }

      gBS->FreePool (HandleBuffer);
    } else {
      DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Failed to locate SimpleTextInput handles: %r\n", Status));
    }
  } else {
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: System appears to have full EFI 2.0+ support, compatibility layer not needed\n"
      )
      );
  }

  gDriverInitialized = TRUE;
  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: === Compatibility installation completed: %d successful installations ===\n",
     SuccessCount
    )
    );
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
  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: System is ready to boot, performing late initialization\n"
    )
    );
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

  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: === OpenCore SimpleTextInputEx Compatibility Driver Starting ===\n"
    )
    );
  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: System Table Revision: 0x%08X (%d.%02d)\n",
     SystemTable->Hdr.Revision,
     SystemTable->Hdr.Revision >> 16,
     SystemTable->Hdr.Revision & 0xFFFF
    )
    );

  if (SystemTable->RuntimeServices != NULL) {
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: Runtime Services Revision: 0x%08X (%d.%02d)\n",
       SystemTable->RuntimeServices->Hdr.Revision,
       SystemTable->RuntimeServices->Hdr.Revision >> 16,
       SystemTable->RuntimeServices->Hdr.Revision & 0xFFFF
      )
      );
  }

  if (SystemTable->BootServices != NULL) {
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: Boot Services Revision: 0x%08X (%d.%02d)\n",
       SystemTable->BootServices->Hdr.Revision,
       SystemTable->BootServices->Hdr.Revision >> 16,
       SystemTable->BootServices->Hdr.Revision & 0xFFFF
      )
      );
  }

  // Check if console services are available for immediate installation
  if ((SystemTable->ConsoleInHandle != NULL) && (SystemTable->ConIn != NULL)) {
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: Console services are available, attempting immediate installation\n"));
    Status = PerformCompatibilityInstallation ();
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OcTextInputDxe: Immediate installation successful\n"));
      return EFI_SUCCESS;
    } else {
      DEBUG (
        (
         DEBUG_WARN,
         "OcTextInputDxe: Immediate installation failed: %r, will try deferred installation\n",
         Status
        )
        );
    }
  } else {
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: Console services not ready (ConsoleInHandle=%p, ConIn=%p)\n",
       SystemTable->ConsoleInHandle,
       SystemTable->ConIn
      )
      );
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
    DEBUG (
      (
       DEBUG_INFO,
       "OcTextInputDxe: ReadyToBoot event created successfully, driver will install protocols when system is ready\n"
      )
      );
  }

  // Always return success - we want the driver to stay loaded
  DEBUG (
    (
     DEBUG_INFO,
     "OcTextInputDxe: OpenCore SimpleTextInputEx compatibility driver loaded successfully\n"
    )
    );
  return EFI_SUCCESS;
}
