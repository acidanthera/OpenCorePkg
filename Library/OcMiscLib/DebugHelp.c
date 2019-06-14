/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcMiscLib.h>

VOID
#ifdef __GNUC__
__attribute__ ((noinline))
#endif
DebugBreak (
  VOID
  )
{
  //
  // This function has no code, debuggers may break on it.
  //
}

VOID
WaitForKeyPress (
  CONST CHAR16 *Message
  )
{
  EFI_STATUS        Status;
  EFI_INPUT_KEY     Key;
  volatile BOOLEAN  Proceed;

  //
  // Print message.
  //  
  gST->ConOut->OutputString (gST->ConOut, (CHAR16 *) Message);
  gST->ConOut->OutputString (gST->ConOut, (CHAR16 *) L"\r\n");

  //
  // Skip previously pressed characters.
  //
  do {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  } while (!EFI_ERROR (Status));

  //
  // Wait for debugger signal or key press.
  //
  Proceed = FALSE;
  while (EFI_ERROR (Status) && !Proceed) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    DebugBreak ();
  }
}

INTN
WaitForKeyIndex (
  UINTN  Timeout
  )
{
  EFI_STATUS        Status;
  EFI_INPUT_KEY     Key;
  INTN              Index;
  EFI_EVENT         TimerEvent;

  //
  // Skip previously pressed characters.
  //
  do {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  } while (!EFI_ERROR (Status));

  //
  // Prepare timer if we have any.
  //
  TimerEvent = NULL;
  if (Timeout > 0) {
    Status = gBS->CreateEvent (EVT_TIMER, TPL_CALLBACK, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      Status = gBS->SetTimer (TimerEvent, TimerRelative, 10000000 * Timeout);
      if (EFI_ERROR (Status)) {
        gBS->CloseEvent (TimerEvent);
        TimerEvent = NULL;
      }
    }
  }

  //
  // Wait for the keystroke event or the timer
  //
  if (TimerEvent != NULL) {
    do {
      //
      // Read our key otherwise.
      //
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (!EFI_ERROR (Status)) {
        break;
      }

      Status = gBS->CheckEvent (TimerEvent);
      //
      // Check for the timer expiration
      //
      if (!EFI_ERROR (Status)) {
        gBS->CloseEvent (TimerEvent);
        return OC_INPUT_TIMEOUT;
      }
    } while (Status == EFI_NOT_READY);

    gBS->CloseEvent (TimerEvent);
  } else {
    //
    // Read our key otherwise.
    //
    do {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    } while (EFI_ERROR (Status));
  }

  if (Key.ScanCode == SCAN_ESC || Key.UnicodeChar == '0') {
    return OC_INPUT_ABORTED;
  }

  //
  // Using loop to allow OC_INPUT_STR changes.
  //
  for (Index = 0; Index < OC_INPUT_MAX; ++Index) {
    if (OC_INPUT_STR[Index] == Key.UnicodeChar
      || (OC_INPUT_STR[Index] >= 'A' && OC_INPUT_STR[Index] <= 'Z'
        && (OC_INPUT_STR[Index] | 0x20U) == Key.UnicodeChar))  {
      return Index;
    }
  }

  return OC_INPUT_INVALID;
}
