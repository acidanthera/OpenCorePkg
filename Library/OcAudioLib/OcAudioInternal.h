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

#ifndef OC_AUDIO_INTERNAL_H
#define OC_AUDIO_INTERNAL_H

#include <Library/OcAudioLib.h>
#include <Protocol/AudioIo.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/AppleVoiceOver.h>
#include <Protocol/OcAudio.h>

#define OC_AUDIO_PROTOCOL_PRIVATE_SIGNATURE  \
  SIGNATURE_32 ('D', 'J', 'B', 'n')

#define OC_AUDIO_PROTOCOL_PRIVATE_FROM_OC_AUDIO(Proto)      \
  CR (                                                      \
    (Proto),                                                \
    OC_AUDIO_PROTOCOL_PRIVATE,                              \
    OcAudio,                                                \
    OC_AUDIO_PROTOCOL_PRIVATE_SIGNATURE                     \
    )

#define OC_AUDIO_PROTOCOL_PRIVATE_FROM_VOICE_OVER(Proto)    \
  CR (                                                      \
    (Proto),                                                \
    OC_AUDIO_PROTOCOL_PRIVATE,                              \
    VoiceOver,                                              \
    OC_AUDIO_PROTOCOL_PRIVATE_SIGNATURE                     \
    )

typedef struct {
  UINT32                                Signature;
  EFI_AUDIO_IO_PROTOCOL                 *AudioIo;
  OC_AUDIO_PROVIDER_ACQUIRE             ProviderAcquire;
  OC_AUDIO_PROVIDER_RELEASE             ProviderRelease;
  VOID                                  *ProviderContext;
  VOID                                  *CurrentBuffer;
  EFI_EVENT                             PlaybackEvent;
  UINTN                                 PlaybackDelay;
  UINT8                                 Language;
  UINT8                                 OutputIndex;
  UINT8                                 Volume;
  OC_AUDIO_PROTOCOL                     OcAudio;
  APPLE_BEEP_GEN_PROTOCOL               BeepGen;
  APPLE_VOICE_OVER_AUDIO_PROTOCOL       VoiceOver;
} OC_AUDIO_PROTOCOL_PRIVATE;

EFI_STATUS
EFIAPI
InternalOcAudioConnect (
  IN OUT OC_AUDIO_PROTOCOL         *This,
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL,
  IN     UINT8                     CodecAddress OPTIONAL,
  IN     UINT8                     OutputIndex  OPTIONAL,
  IN     UINT8                     Volume
  );

EFI_STATUS
EFIAPI
InternalOcAudioSetProvider (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     OC_AUDIO_PROVIDER_ACQUIRE  Acquire,
  IN     OC_AUDIO_PROVIDER_RELEASE  Release  OPTIONAL,
  IN     VOID                       *Context
  );

EFI_STATUS
EFIAPI
InternalOcAudioPlayFile (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     UINT32                     File,
  IN     BOOLEAN                    Wait
  );

EFI_STATUS
EFIAPI
InternalOcAudioStopPlayBack (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     BOOLEAN                    Wait
  );

UINTN
EFIAPI
InternalOcAudioSetDelay (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     UINTN                      Delay
  );

EFI_STATUS
EFIAPI
InternalOcAudioGenBeep (
  IN UINT32  ToneCount,
  IN UINTN   ToneLength,
  IN UINTN   SilenceLength
  );

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverPlay (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     UINT8                            File
  );

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverSetLanguageCode (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     UINT8                            LanguageCode
  );

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverSetLanguageString (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     CONST CHAR8                      *LanguageString
  );

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverGetLanguage (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  OUT    UINT8                            *LanguageCode,
  OUT    CONST CHAR8                      **LanguageString
  );

#endif // OC_AUDIO_INTERNAL_H
