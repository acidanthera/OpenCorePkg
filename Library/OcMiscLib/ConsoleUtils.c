/** @file
  Misc console utils.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>

VOID
OcConsoleFlush (
  VOID
  )
{
  EFI_STATUS          Status;
  EFI_INPUT_KEY       EfiKey;

  do {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &EfiKey);
  } while (!EFI_ERROR (Status));

  //
  // This one is required on APTIO IV after holding OPT key.
  // Interestingly it does not help adding this after OPT key handling.
  //
  gST->ConIn->Reset (gST->ConIn, FALSE);
}
