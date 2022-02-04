/** @file
  Test graphics output protocol.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Guid/AppleFile.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcFileLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>

#define NUM_COLORS 5
#define NUM_BLITS  200

STATIC UINT32 mColors[NUM_COLORS] = {
  0xFFFFFFFF,
  0xFFFFFF00,
  0xFFFF00FF,
  0xFF00FFFF,
  0xFF0000FF,
};

STATIC
EFI_STATUS
PerfConsoleGop (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *Gop;
  VOID                                  *Pools[NUM_COLORS];
  UINTN                                 Index;
  UINTN                                 Index2;
  UINT64                                PerfStart;
  UINT64                                PerfResult;
  UINT64                                FrameTimeNs;
  UINT64                                FpMs;
  UINT64                                Fps;
  UINT64                                Remainder;
  EFI_TPL                               OldTpl;
  BOOLEAN                               HasInterrupts;

  Status = OcHandleProtocolFallback (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &Gop
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "GPF: No GOP - %r\n", Status));
    return Status;
  }

  if (Gop->Mode == NULL || Gop->Mode->Info == NULL) {
    DEBUG ((DEBUG_WARN, "GPF: Invalid GOP\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Gop->Mode->Info->HorizontalResolution == 0
    || Gop->Mode->Info->VerticalResolution == 0) {
    DEBUG ((DEBUG_WARN, "GPF: Empty resolution on GOP\n"));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < NUM_COLORS; ++Index) {
    Pools[Index] = AllocatePool (
      Gop->Mode->Info->HorizontalResolution * Gop->Mode->Info->VerticalResolution * sizeof (UINT32)
      );
    ASSERT (Pools[Index] != NULL);
    SetMem32 (
      Pools[Index],
      Gop->Mode->Info->HorizontalResolution * Gop->Mode->Info->VerticalResolution * sizeof (UINT32),
      mColors[Index]
      );
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  HasInterrupts = SaveAndDisableInterrupts ();

  PerfStart = GetPerformanceCounter ();

  for (Index = 0; Index < NUM_BLITS; ++Index) {
    for (Index2 = 0; Index2 < NUM_COLORS; ++Index2) {
      Gop->Blt (
        Gop,
        Pools[Index2],
        EfiBltBufferToVideo,
        0,
        0,
        0,
        0,
        Gop->Mode->Info->HorizontalResolution,
        Gop->Mode->Info->VerticalResolution,
        0
        );
    }
  }

  PerfResult = GetPerformanceCounter () - PerfStart;

  if (HasInterrupts) {
    EnableInterrupts ();
  }
  gBS->RestoreTPL (OldTpl);

  if (GetTimeInNanoSecond (PerfResult) != 0) {

    FrameTimeNs = DivU64x64Remainder (
      GetTimeInNanoSecond (PerfResult),
      (UINT64) NUM_BLITS * NUM_COLORS,
      NULL
      );

    FpMs = DivU64x64Remainder (1000000000ULL * 1000ULL, FrameTimeNs, NULL);
    Fps  = DivU64x64Remainder (FpMs, 1000, &Remainder);

    DEBUG ((
      DEBUG_WARN,
      "GPF: Total blits %Lu on %ux%u over %Lu ms, about %Lu.%03Lu FPS, %Lu ms TPF\n",
      (UINT64) NUM_BLITS * NUM_COLORS,
      Gop->Mode->Info->HorizontalResolution,
      Gop->Mode->Info->VerticalResolution,
      DivU64x64Remainder (GetTimeInNanoSecond (PerfResult), 1000000ULL, NULL),
      Fps,
      Remainder,
      DivU64x64Remainder (FrameTimeNs, 1000000, NULL)
      ));
  } else {
    DEBUG ((DEBUG_WARN, "GPF: Unable to measure time\n"));
  }


  for (Index = 0; Index < NUM_COLORS; ++Index) {
    FreePool (Pools[Index]);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // 1. Disable watchdog timer.
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // 2. Run test.
  //
  PerfConsoleGop ();

  return EFI_SUCCESS;
}
