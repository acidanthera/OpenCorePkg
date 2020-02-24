/** @file
  This library implements audio interaction.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef OC_AUDIO_LIB_H
#define OC_AUDIO_LIB_H

#include <Uefi.h>
#include <Protocol/OcAudio.h>

/**
  Install audio support protocols.

  @param[in] Reinstall  Overwrite installed protocols.

  @retval installed protocol.
  @retval NULL when conflicting audio implementation is present.
  @retval NULL when installation failed.
**/
OC_AUDIO_PROTOCOL *
OcAudioInstallProtocols (
  IN BOOLEAN  Reinstall
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
  Get system volume level in 0~100 range.

  @param[out]  Muted   Whether volume is off.

  @retval ASCII string.
**/
UINT8
OcGetVolumeLevel (
  OUT BOOLEAN  *Muted
  );

#endif // OC_AUDIO_LIB_H
