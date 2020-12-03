/** @file
  Run Apple Boot Picker.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Guid/AppleFile.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>

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

  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  OcProvideConsoleGop (FALSE);

  OcSetConsoleResolution (0, 0, 0, FALSE);

  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &Gop
    );

  if (EFI_ERROR (Status)) {
    //
    // Note: Ensure that stall value is within UINT32 in nanoseconds.
    //
    for (Index = 0; Index < 10; ++Index) {
      gBS->Stall (SECONDS_TO_MICROSECONDS (1));
    }
    return Status;
  }

  Status = OcRunFirmwareApplication (&gAppleBootPickerFileGuid, TRUE);

  Pixel.Raw = 0x0;
  if (Status == EFI_NOT_FOUND) {
    //
    // Red. No BootPicker in firmware or we cannot get it.
    //
    Pixel.Pixel.Red   = 0xFF;
  } else if (Status == EFI_UNSUPPORTED) {
    //
    // Yellow. BootPicker does not start.
    //
    Pixel.Pixel.Red   = 0xFF;
    Pixel.Pixel.Green = 0xFF;
  } else if (EFI_ERROR (Status) /* Status == EFI_INVALID_PARAMETER */) {
    //
    // Fuchsia. BootPicker does not load.
    //
    Pixel.Pixel.Blue = 0xFF;
    Pixel.Pixel.Red  = 0xFF;
  } else {
    //
    // Green. BootPicker started but returned.
    //
    Pixel.Pixel.Green = 0xFF;
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
