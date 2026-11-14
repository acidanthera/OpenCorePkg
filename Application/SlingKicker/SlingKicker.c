/** @file
  Run Apple SlingShot.

  Copyright (c) 2020, vit9696. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>

#include <Protocol/GraphicsOutput.h>

STATIC
EFI_STATUS
PrintHelp (
  VOID
  )
{
  Print (L"Usage: SlingKicker <RecDisk|RecDiskLatest|RecNet|RecNetLatest|DiagDisk|DiagNet>\n");
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                           Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL         *Gop;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Pixel;
  UINTN                                Index;
  UINTN                                Argc;
  CHAR16                               **Argv;
  SLING_SHOT_BOOT_MODE                 BootMode;

  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  OcProvideConsoleGop (FALSE);

  OcSetConsoleResolution (0, 0, 0, FALSE);

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );

  if (EFI_ERROR (Status)) {
    //
    // Note: Ensure that stall value is within UINT32 in nanoseconds, for
    // firmware such as APTIO IV with less robust Stall implementations.
    //
    for (Index = 0; Index < 10; ++Index) {
      gBS->Stall (SECONDS_TO_MICROSECONDS (1));
    }

    return Status;
  }

  Status = GetArguments (&Argc, &Argv);

  if (EFI_ERROR (Status) || (Argc < 2) || (Argc > 2)) {
    return PrintHelp ();
  }

  if (OcStriCmp (Argv[1], L"RecDisk") == 0) {
    BootMode = SlingShotModeRecDisk;
  } else if (OcStriCmp (Argv[1], L"RecDiskLatest") == 0) {
    BootMode = SlingShotModeRecDiskLatest;
  } else if (OcStriCmp (Argv[1], L"RecNet") == 0) {
    BootMode = SlingShotModeRecNet;
  } else if (OcStriCmp (Argv[1], L"RecNetLatest") == 0) {
    BootMode = SlingShotModeRecNetLatest;
  } else if (OcStriCmp (Argv[1], L"DiagDisk") == 0) {
    BootMode = SlingShotModeDiagDisk;
  } else if (OcStriCmp (Argv[1], L"DiagNet") == 0) {
    BootMode = SlingShotModeDiagNet;
  } else {
    return PrintHelp ();
  }

  Status = OcLaunchAppleSlingShot (BootMode);

  Pixel.Raw = 0x0;
  if (Status == EFI_NOT_FOUND) {
    //
    // Red. No SlingShot in firmware or we cannot get it.
    //
    Pixel.Pixel.Red = 0xFF;
  } else if (Status == EFI_UNSUPPORTED) {
    //
    // Yellow. SlingShot does not start.
    //
    Pixel.Pixel.Red   = 0xFF;
    Pixel.Pixel.Green = 0xFF;
  } else {
    //
    // Fuchsia. SlingShot does not load.
    //
    Pixel.Pixel.Blue = 0xFF;
    Pixel.Pixel.Red  = 0xFF;
  }

  Gop->Blt (
         Gop,
         &Pixel.Pixel,
         EfiBltVideoFill,
         0,
         0,
         0,
         0,
         Gop->Mode->Info->HorizontalResolution,
         Gop->Mode->Info->VerticalResolution,
         0
         );

  //
  // Note: Ensure that stall value is within UINT32 in nanoseconds.
  //
  for (Index = 0; Index < 10; ++Index) {
    gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  }

  return EFI_SUCCESS;
}
