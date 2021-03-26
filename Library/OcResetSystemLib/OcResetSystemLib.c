/** @file
  Reset System Library instance that accounts for defective UEFI systems.

  Copyright (c) 2021, Marvin Häuser. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

VOID
EFIAPI
ResetCold (
  VOID
  )
{
  gRT->ResetSystem (
    EfiResetCold,
    EFI_SUCCESS,
    0,
    NULL
    );
  //
  // Attempt to perform a cold reset manually if the UEFI call does not work.
  //
  DirectResetCold ();
  CpuDeadLoop ();
}

VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  //
  // Warm resets can cause issues, reset cold instead.
  //
  ResetCold ();
}

VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  gRT->ResetSystem (
    EfiResetShutdown,
    EFI_SUCCESS,
    0,
    NULL
    );
  //
  // Perform cold reset when shutdown fails (e.g. DUET).
  //
  ResetCold ();
}

VOID
EFIAPI
InternalResetPlatformSpecific (
  IN EFI_STATUS  ResetStatus,
  IN UINTN       DataSize,
  IN VOID        *ResetData
  )
{
  gRT->ResetSystem (
    EfiResetPlatformSpecific,
    ResetStatus,
    DataSize,
    ResetData
    );
  ResetCold ();
}

VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  )
{
  InternalResetPlatformSpecific (EFI_SUCCESS, DataSize, ResetData);
}

VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  switch (ResetType) {
    case EfiResetCold:
    {
      ResetCold ();
      break;
    }

    case EfiResetWarm:
    {
      ResetWarm ();
      break;
    }

    case EfiResetShutdown:
    {
      ResetShutdown ();
      break;
    }

    case EfiResetPlatformSpecific:
    {
      InternalResetPlatformSpecific (ResetStatus, DataSize, ResetData);
      break;
    }
  }

  ASSERT (FALSE);
}
