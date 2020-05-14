/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/AppleVariable.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/AppleHda.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/AppleVoiceOver.h>

#include "OcAudioInternal.h"

STATIC
EFI_GUID *
mAudioProtocols[] = {
  &gOcAudioProtocolGuid,
  &gAppleBeepGenProtocolGuid,
  &gAppleVOAudioProtocolGuid,
  &gAppleHighDefinitionAudioProtocolGuid,
};

STATIC
OC_AUDIO_PROTOCOL_PRIVATE
mAudioProtocol = {
  .Signature       = OC_AUDIO_PROTOCOL_PRIVATE_SIGNATURE,
  .AudioIo         = NULL,
  .ProviderAcquire = NULL,
  .ProviderRelease = NULL,
  .ProviderContext = NULL,
  .CurrentBuffer   = NULL,
  .PlaybackEvent   = NULL,
  .Language        = AppleVoiceOverLanguageEn,
  .OutputIndex     = 0,
  .Volume          = 100,
  .OcAudio         = {
    .Revision           = OC_AUDIO_PROTOCOL_REVISION,
    .Connect            = InternalOcAudioConnect,
    .SetProvider        = InternalOcAudioSetProvider,
    .PlayFile           = InternalOcAudioPlayFile,
    .StopPlayback       = InternalOcAudioStopPlayBack,
  },
  .BeepGen         = {
    .GenBeep            = InternalOcAudioGenBeep,
  },
  .VoiceOver       = {
    .Play               = InternalOcAudioVoiceOverPlay,
    .SetLanguageCode    = InternalOcAudioVoiceOverSetLanguageCode,
    .SetLanguageString  = InternalOcAudioVoiceOverSetLanguageString,
    .GetLanguage        = InternalOcAudioVoiceOverGetLanguage,
  }
};

OC_AUDIO_PROTOCOL *
OcAudioInstallProtocols (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS         Status;
  UINTN              Index;
  VOID               *Protocol;
  EFI_HANDLE         NewHandle;

  if (Reinstall) {
    for (Index = 0; Index < ARRAY_SIZE (mAudioProtocols); ++Index) {
      Status = OcUninstallAllProtocolInstances (mAudioProtocols[Index]);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "OCAU: Uninstall %g failed: %r\n", mAudioProtocols[Index], Status));
        return NULL;
      }
    }
  } else {
    for (Index = 0; Index < ARRAY_SIZE (mAudioProtocols); ++Index) {
      Status = gBS->LocateProtocol (
        mAudioProtocols[Index],
        NULL,
        &Protocol
        );
      if (!EFI_ERROR (Status)) {
        if (Index == 0) {
          return (OC_AUDIO_PROTOCOL *) Protocol;
        }
        DEBUG ((DEBUG_INFO, "OCAU: Found %g protocol\n", mAudioProtocols[Index]));
        return NULL;
      }
    }
  }

  Status = gBS->CreateEvent (
    0,
    TPL_NOTIFY,
    NULL,
    NULL,
    &mAudioProtocol.PlaybackEvent
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAU: Unable to create audio completion event - %r\n", Status));
    return NULL;
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gOcAudioProtocolGuid,
    &mAudioProtocol.OcAudio,
    &gAppleBeepGenProtocolGuid,
    &mAudioProtocol.BeepGen,
    &gAppleVOAudioProtocolGuid,
    &mAudioProtocol.VoiceOver,
    NULL
    );

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mAudioProtocol.PlaybackEvent);
    mAudioProtocol.PlaybackEvent = NULL;
    return NULL;
  }

  return &mAudioProtocol.OcAudio;
}

UINT8
OcGetVolumeLevel (
  IN  UINT32   Amplifier,
  OUT BOOLEAN  *Muted
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINT8       Value;
  UINT8       NewValue;

  Size   = sizeof (Value);
  Status = gRT->GetVariable (
    APPLE_SYSTEM_AUDIO_VOLUME_VARIABLE_NAME,
    &gAppleBootVariableGuid,
    NULL,
    &Size,
    &Value
    );
  if (EFI_ERROR (Status)) {
    Value = OC_AUDIO_DEFAULT_VOLUME_LEVEL;
  }

  if ((Value & APPLE_SYSTEM_AUDIO_VOLUME_MUTED) != 0) {
    Value  &= APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK;
    *Muted = TRUE;
  } else {
    *Muted = FALSE;
  }

  if (Amplifier > 0) {
    NewValue = (UINT8) (Value * Amplifier / 100);
  } else {
    NewValue = Value;
  }

  NewValue = MIN (NewValue, 100);

  DEBUG ((
    DEBUG_INFO,
    "OCAU: System volume is %d (calculated from %d) - %r\n",
    NewValue,
    Value,
    Status
    ));

  return NewValue;
}
