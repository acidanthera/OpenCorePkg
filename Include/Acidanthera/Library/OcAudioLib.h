/** @file
  This library implements audio interaction.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef OC_AUDIO_LIB_H
#define OC_AUDIO_LIB_H

#include <Uefi.h>
#include <Library/OcFileLib.h>
#include <Protocol/OcAudio.h>

#define OC_AUDIO_DEFAULT_GAIN  (-30)

/**
  Install audio support protocols.

  @param[in] Reinstall           Overwrite installed protocols.
  @param[in] DisconnectHda       Attempt to disconnect HDA controller first.

  @retval installed protocol.
  @retval NULL when conflicting audio implementation is present.
  @retval NULL when installation failed.
**/
OC_AUDIO_PROTOCOL *
OcAudioInstallProtocols (
  IN BOOLEAN  Reinstall,
  IN BOOLEAN  DisconnectHda
  );

/**
  Convert language code to ASCII string.

  @param[in]  LanguageCode   Code.

  @retval ASCII string.
**/
CONST CHAR8 *
OcLanguageCodeToString (
  IN APPLE_VOICE_OVER_LANGUAGE_CODE  LanguageCode
  );

/**
  Get system amplifier gain.

  @param[out]  RawGain        Raw codec gain setting.
  @param[out]  DecibelGain    Decibel gain setting.
  @param[out]  Muted          Whether amplifier should be muted.
  @param[out]  TryConversion  TRUE when decibel gain setting is a default value and
                              raw codec gain setting is a real value.
**/
VOID
OcGetAmplifierGain (
  OUT UINT8    *RawGain,
  OUT INT8     *DecibelGain,
  OUT BOOLEAN  *Muted,
  OUT BOOLEAN  *TryConversion
  );

/**
  Set VoiceOver language from string.

  @param[in]  Language   Language string, optional for system.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetVoiceOverLanguage (
  CONST CHAR8  *Language  OPTIONAL
  );

/**
  Dump audio data to the specified directory.

  @param[in] Root  Directory to write audio data.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcAudioDump (
  IN EFI_FILE_PROTOCOL  *Root
  );

/**
  Playback progress of the most recent audio stream.

  The byte counts come from the AudioDxe driver's own DMA accounting, so Played
  describes data the controller has consumed rather than data the listener has
  heard, and it leads the audible output slightly. Played and Total keep the
  result of the last playback after it ends, while Playing turns FALSE.
**/
typedef struct {
  UINT32     Played;
  UINT32     Total;
  UINT32     SampleRate;
  UINT8      Channels;
  UINT8      BitsPerSample;
  BOOLEAN    Playing;
} OC_AUDIO_PROGRESS;

/**
  Get playback progress of the most recent audio stream.

  Requires an AudioDxe build that produces EFI_AUDIO_PROGRESS_PROTOCOL. The
  format fields stay 0 when the driver cannot report them; a position without a
  format is still reported, so only a failure of the position query is an error.

  @param[out] Progress  Playback progress.

  @retval EFI_SUCCESS      Progress was retrieved.
  @retval EFI_UNSUPPORTED  AudioDxe does not report playback progress.
  @retval other            Error returned by the playback progress protocol.
**/
EFI_STATUS
OcAudioGetPlaybackProgress (
  OUT OC_AUDIO_PROGRESS  *Progress
  );

//
// Base path and base type for a given APPLE_VOICE_OVER_AUDIO_FILE index.
//
typedef struct {
  CONST CHAR8    *BasePath;
  CONST CHAR8    *BaseType;
} APPLE_VOICE_OVER_FILE_MAP;

#endif // OC_AUDIO_LIB_H
