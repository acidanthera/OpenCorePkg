/** @file
  Timer booster

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/Timer.h>

STATIC UINT64                   mOriginalTimerPeriod;
STATIC EFI_TIMER_ARCH_PROTOCOL  *mTimerProtocol;

EFI_STATUS
OcAppleGenericInputTimerQuirkInit (
  IN UINT32  TimerResolution
  )
{
  EFI_STATUS  Status;
  //
  // Refresh rate needs to be increased to poll mouse and keyboard frequently enough
  //
  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **)&mTimerProtocol);
  if (!EFI_ERROR (Status)) {
    Status = mTimerProtocol->GetTimerPeriod (mTimerProtocol, &mOriginalTimerPeriod);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: AIFTimerBoostInit Current timer is %u\n", mOriginalTimerPeriod));
      if (mOriginalTimerPeriod > TimerResolution) {
        Status = mTimerProtocol->SetTimerPeriod (mTimerProtocol, TimerResolution);
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCII: AIFTimerBoostInit changed period %d to %d\n",
            mOriginalTimerPeriod, TimerResolution));
        } else {
          DEBUG ((DEBUG_INFO, "OCII: AIFTimerBoostInit failed to change period %d to %d, error - %r\n",
            mOriginalTimerPeriod, TimerResolution, Status));
          mTimerProtocol = NULL;
        }
      } else {
        mTimerProtocol = NULL;
      }
    } else {
      DEBUG ((DEBUG_INFO, "OCII: AIFTimerBoostInit failed to obtain previous period - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCII: AIFTimerBoostInit gEfiTimerArchProtocolGuid not found - %r\n", Status));
  }

  return Status;
}

EFI_STATUS
OcAppleGenericInputTimerQuirkExit (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mTimerProtocol != NULL) {
    //
    // You are not allowed to call this after ExitBootServices (e.g. as an Event) on
    // APTIO IV, as it results in an interrupt with 0x0 pointer handler during XNU boot.
    //
    Status = mTimerProtocol->SetTimerPeriod (mTimerProtocol, mOriginalTimerPeriod);
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "AmiShimTimerBoostExit changed current period to %Lu\n",
        mOriginalTimerPeriod
        ));
    } else {
       DEBUG ((
        DEBUG_INFO,
        "AmiShimTimerBoostExit failed to change current period to %Lu, error - %r\n",
        mOriginalTimerPeriod,
        Status
        ));
    }
    mTimerProtocol = NULL;
  }

  return Status;
}
