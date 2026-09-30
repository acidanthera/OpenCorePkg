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

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/AppleHda.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/AppleVoiceOver.h>

#include "OcAudioInternal.h"

//
// OC audio protocol must come first in this list.
//
STATIC
EFI_GUID *
  mAudioProtocols[] = {
  &gOcAudioProtocolGuid,
  &gAppleBeepGenProtocolGuid,
  &gAppleVOAudioProtocolGuid,
  &gAppleHighDefinitionAudioProtocolGuid
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
  .PlaybackDelay   = 0,
  .Language        = AppleVoiceOverLanguageEn,
  .OutputIndexMask = 0,
  .Gain            = APPLE_SYSTEM_AUDIO_VOLUME_DB_MIN,
  .OcAudio         = {
    .Revision          = OC_AUDIO_PROTOCOL_REVISION,
    .Connect           = InternalOcAudioConnect,
    .RawGainToDecibels = InternalOcAudioRawGainToDecibels,
    .SetDefaultGain    = InternalOcAudioSetDefaultGain,
    .SetProvider       = InternalOcAudioSetProvider,
    .PlayFile          = InternalOcAudioPlayFile,
    .StopPlayback      = InternalOcAudioStopPlayback,
    .SetDelay          = InternalOcAudioSetDelay,
    .GetProgress       = InternalOcAudioGetProgress
  },
  .BeepGen             = {
    .GenBeep           = InternalOcAudioGenBeep,
  },
  .VoiceOver           = {
    .Play              = InternalOcAudioVoiceOverPlay,
    .SetLanguageCode   = InternalOcAudioVoiceOverSetLanguageCode,
    .SetLanguageString = InternalOcAudioVoiceOverSetLanguageString,
    .GetLanguage       = InternalOcAudioVoiceOverGetLanguage,
  }
};

OC_AUDIO_PROTOCOL *
OcAudioInstallProtocols (
  IN BOOLEAN  Reinstall,
  IN BOOLEAN  DisconnectHda
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  VOID        *Protocol;
  EFI_HANDLE  NewHandle;

  DEBUG ((DEBUG_INFO, "OCAU: OcAudioInstallProtocols (%u, %u)\n", Reinstall, DisconnectHda));

  if (DisconnectHda) {
    OcDisconnectHdaControllers ();
  }

  if (Reinstall) {
    for (Index = 0; Index < ARRAY_SIZE (mAudioProtocols); ++Index) {
      Status = OcUninstallAllProtocolInstances (mAudioProtocols[Index]);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "OCAU: Uninstall %g failed - %r\n", mAudioProtocols[Index], Status));
        return NULL;
      }
    }
  } else {
    DEBUG_CODE_BEGIN ();
    for (Index = 0; Index < ARRAY_SIZE (mAudioProtocols); ++Index) {
      Status = gBS->LocateProtocol (
                      mAudioProtocols[Index],
                      NULL,
                      &Protocol
                      );
      DEBUG ((DEBUG_INFO, "OCAU: %g protocol - %r\n", mAudioProtocols[Index], Status));
    }

    DEBUG_CODE_END ();
    for (Index = 0; Index < ARRAY_SIZE (mAudioProtocols); ++Index) {
      Status = gBS->LocateProtocol (
                      mAudioProtocols[Index],
                      NULL,
                      &Protocol
                      );
      if (!EFI_ERROR (Status)) {
        if (Index == 0) {
          return (OC_AUDIO_PROTOCOL *)Protocol;
        }

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
  Status    = gBS->InstallMultipleProtocolInterfaces (
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

VOID
OcGetAmplifierGain (
  OUT UINT8    *RawGain,
  OUT INT8     *DecibelGain,
  OUT BOOLEAN  *Muted,
  OUT BOOLEAN  *TryConversion
  )
{
  EFI_STATUS  Status1;
  EFI_STATUS  Status2;
  UINTN       Size;

  //
  // Get mute setting and raw codec gain setting (all versions of macOS).
  //
  Size    = sizeof (*RawGain);
  Status1 = gRT->GetVariable (
                   APPLE_SYSTEM_AUDIO_VOLUME_VARIABLE_NAME,
                   &gAppleBootVariableGuid,
                   NULL,
                   &Size,
                   RawGain
                   );
  if (!EFI_ERROR (Status1)) {
    *Muted    = (*RawGain & APPLE_SYSTEM_AUDIO_VOLUME_MUTED) != 0;
    *RawGain &= APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK;
  } else {
    *Muted   = FALSE;
    *RawGain = 0;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCAU: System raw gain 0x%X, audio mute (for chime) %u - %r\n",
    *RawGain,
    *Muted,
    Status1
    ));

  //
  // Get dB gain setting, which can be correctly applied to any amp on any codec if available.
  // (Not present at least in Lion 10.7 and earlier.)
  //
  Size    = sizeof (*DecibelGain);
  Status2 = gRT->GetVariable (
                   APPLE_SYSTEM_AUDIO_VOLUME_DB_VARIABLE_NAME,
                   &gAppleBootVariableGuid,
                   NULL,
                   &Size,
                   DecibelGain
                   );
  if (EFI_ERROR (Status2)) {
    *DecibelGain = OC_AUDIO_DEFAULT_GAIN;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCAU: System decibel gain %d dB%a - %r\n",
    *DecibelGain,
    *Muted ? " (probably invalid due to mute)" : "",
    Status2
    ));

  //
  // SystemAudioVolumeDB does not contain sane values when SystemAudioVolume indicates muted, so we have
  // to fall back to default gain value or conversion; for use with audio assist, which never mutes.
  //
  if (*Muted) {
    *DecibelGain = OC_AUDIO_DEFAULT_GAIN;
    Status2      = EFI_INVALID_PARAMETER;
  }

  //
  // If no saved decibel gain, but saved raw gain, it is worth trying to convert.
  //
  *TryConversion = !EFI_ERROR (Status1) && EFI_ERROR (Status2);
}

EFI_STATUS
OcAudioGetPlaybackProgress (
  OUT OC_AUDIO_PROGRESS  *Progress
  )
{
  EFI_STATUS                   Status;
  EFI_AUDIO_PROGRESS_PROTOCOL  *AudioProgress;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Progress, sizeof (*Progress));

  AudioProgress = mAudioProtocol.Progress;
  if (AudioProgress == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = AudioProgress->GetPosition (
                            AudioProgress,
                            &Progress->Played,
                            &Progress->Total,
                            &Progress->Playing
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // A position without a format is still useful, so a format the driver cannot
  // report is not a failure: the fields simply stay zeroed.
  //
  Status = AudioProgress->GetFormat (
                            AudioProgress,
                            &Progress->SampleRate,
                            &Progress->Channels,
                            &Progress->BitsPerSample
                            );
  if (EFI_ERROR (Status)) {
    Progress->SampleRate    = 0;
    Progress->Channels      = 0;
    Progress->BitsPerSample = 0;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InternalOcAudioGetProgress (
  IN OUT OC_AUDIO_PROTOCOL  *This,
  OUT UINT32                *BytesConsumed  OPTIONAL,
  OUT UINT32                *BytesTotal     OPTIONAL,
  OUT BOOLEAN               *Playing        OPTIONAL,
  OUT UINT32                *SampleRate     OPTIONAL,
  OUT UINT8                 *Channels       OPTIONAL,
  OUT UINT8                 *BitsPerSample  OPTIONAL
  )
{
  OC_AUDIO_PROGRESS  Progress;
  EFI_STATUS         Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (  (BytesConsumed == NULL) && (BytesTotal == NULL) && (Playing == NULL)
     && (SampleRate == NULL) && (Channels == NULL) && (BitsPerSample == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = OcAudioGetPlaybackProgress (&Progress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BytesConsumed != NULL) {
    *BytesConsumed = Progress.Played;
  }

  if (BytesTotal != NULL) {
    *BytesTotal = Progress.Total;
  }

  if (Playing != NULL) {
    *Playing = Progress.Playing;
  }

  if (SampleRate != NULL) {
    *SampleRate = Progress.SampleRate;
  }

  if (Channels != NULL) {
    *Channels = Progress.Channels;
  }

  if (BitsPerSample != NULL) {
    *BitsPerSample = Progress.BitsPerSample;
  }

  return EFI_SUCCESS;
}
