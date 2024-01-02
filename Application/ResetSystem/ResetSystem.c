/** @file
  Reset system.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/OcBootManagementLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Argc;
  CHAR16      **Argv;
  CHAR16      *Mode;

  Status = GetArguments (&Argc, &Argv);
  if (!EFI_ERROR (Status) && (Argc >= 2)) {
    Mode = Argv[1];
  } else {
    DEBUG ((DEBUG_INFO, "OCRST: Assuming default to be coldreset - %r\n", Status));
    Mode = L"coldreset";
  }

  return OcResetSystem (Mode);
}
