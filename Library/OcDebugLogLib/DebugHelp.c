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
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>

VOID
#if defined(__GNUC__) || defined(__clang__)
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
