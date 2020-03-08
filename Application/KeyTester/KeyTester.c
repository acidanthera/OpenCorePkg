/** @file
  Test keyboard input through standard protocol.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS     Status;
  UINTN          EventIndex;
  EFI_INPUT_KEY  Key;

  STATIC CHAR16  Output[] = L"Received symbol:  \r\n";

  do {
    //
    // Read a key
    //
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    ZeroMem (&Key, sizeof (Key));
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (EFI_ERROR (Status)) {

      if (Status == EFI_NOT_READY) {
        continue;
      }

      break;
    }

    Output[ARRAY_SIZE (Output) - 4] = Key.UnicodeChar;
    gST->ConOut->OutputString (gST->ConOut, Output);
    DEBUG ((DEBUG_INFO, "KKT: Received %X %X\n", Key.ScanCode, Key.UnicodeChar));
  } while (TRUE);

  return EFI_SUCCESS;
}
