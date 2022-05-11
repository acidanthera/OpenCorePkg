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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/AppleHda.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/AppleVoiceOver.h>

#include "OcAudioInternal.h"

//
// Convert from Apple file id to file base name and type.
//
STATIC
CONST
APPLE_VOICE_OVER_FILE_MAP
  mAppleVoiceOverLocalisedAudioFiles[AppleVoiceOverAudioFileIndexLocalisedMax - AppleVoiceOverAudioFileIndexLocalisedMin] = {
  [AppleVoiceOverAudioFileVoiceOverOn - AppleVoiceOverAudioFileIndexLocalisedMin] =                 {
    APPLE_VOICE_OVER_AUDIO_FILE_VOICE_OVER_ON,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileVoiceOverOff - AppleVoiceOverAudioFileIndexLocalisedMin] =                {
    APPLE_VOICE_OVER_AUDIO_FILE_VOICE_OVER_OFF,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileUsername - AppleVoiceOverAudioFileIndexLocalisedMin] =                    {
    APPLE_VOICE_OVER_AUDIO_FILE_USERNAME,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFilePassword - AppleVoiceOverAudioFileIndexLocalisedMin] =                    {
    APPLE_VOICE_OVER_AUDIO_FILE_PASSWORD,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileUsernameOrPasswordIncorrect - AppleVoiceOverAudioFileIndexLocalisedMin] = {
    APPLE_VOICE_OVER_AUDIO_FILE_USERNAME_OR_PASSWORD_INCORRECT,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileAccountLockedTryLater - AppleVoiceOverAudioFileIndexLocalisedMin] =       {
    APPLE_VOICE_OVER_AUDIO_FILE_ACCOUNT_LOCKED_TRY_LATER,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileAccountLocked - AppleVoiceOverAudioFileIndexLocalisedMin] =               {
    APPLE_VOICE_OVER_AUDIO_FILE_ACCOUNT_LOCKED,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  }
};

STATIC
CONST
APPLE_VOICE_OVER_FILE_MAP
  mAppleVoiceOverNonLocalisedAudioFiles[AppleVoiceOverAudioFileIndexNonLocalisedMax - AppleVoiceOverAudioFileIndexNonLocalisedMin] = {
  [AppleVoiceOverAudioFileVoiceOverBoot - AppleVoiceOverAudioFileIndexNonLocalisedMin] =  {
    OC_VOICE_OVER_AUDIO_FILE_VOICE_OVER_BOOT,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE
  },
  [AppleVoiceOverAudioFileVoiceOverBoot2 - AppleVoiceOverAudioFileIndexNonLocalisedMin] = {
    APPLE_VOICE_OVER_AUDIO_FILE_VOICE_OVER_BOOT,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileClick - AppleVoiceOverAudioFileIndexNonLocalisedMin] =          {
    APPLE_VOICE_OVER_AUDIO_FILE_CLICK,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  },
  [AppleVoiceOverAudioFileBeep - AppleVoiceOverAudioFileIndexNonLocalisedMin] =           {
    APPLE_VOICE_OVER_AUDIO_FILE_BEEP,
    OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE
  }
};

STATIC CONST CHAR8  *mLanguagePairing[] = {
  NULL,
  "ar",
  "ca",
  "cs",
  "da",
  "de",
  "el",
  "en",
  "es",
  "es-419",
  "es-MX",
  "fi",
  "fr",
  "he",
  "hi",
  "hr",
  "hu",
  "id",
  "it",
  "ja",
  "ko",
  "nl",
  "no",
  "my",
  "pl",
  "pt",
  "pt-PT",
  "ro",
  "ru",
  "sk",
  "sv",
  "th",
  "tr",
  "uk",
  "vi",
  "zh-CN", ///< originally zh-Hans
  "zh-TW"  ///< originally zh-Hant
};

CONST CHAR8 *
OcLanguageCodeToString (
  IN APPLE_VOICE_OVER_LANGUAGE_CODE  LanguageCode
  )
{
  return mLanguagePairing[LanguageCode];
}

EFI_STATUS
OcSetVoiceOverLanguage (
  CONST CHAR8  *Language  OPTIONAL
  )
{
  EFI_STATUS                       Status;
  UINTN                            LanguageSize;
  CHAR8                            LanguageData[16];
  APPLE_VOICE_OVER_AUDIO_PROTOCOL  *VoiceOver;

  if (Language == NULL) {
    LanguageSize = sizeof (LanguageData) - 1;
    Status       = gRT->GetVariable (
                          APPLE_PREV_LANG_KBD_VARIABLE_NAME,
                          &gAppleBootVariableGuid,
                          NULL,
                          &LanguageSize,
                          &LanguageData[0]
                          );
    if (!EFI_ERROR (Status)) {
      LanguageData[LanguageSize] = 0;
      Language                   = LanguageData;
    }
  } else {
    Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (
                    &gAppleVOAudioProtocolGuid,
                    NULL,
                    (VOID **)&VoiceOver
                    );

    if (!EFI_ERROR (Status)) {
      Status = VoiceOver->SetLanguageString (VoiceOver, Language);
    }

    DEBUG ((DEBUG_INFO, "OCAU: Language for audio %a - %r\n", Language, Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAU: No language for audio - %r\n", Status));
  }

  return Status;
}

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverPlay (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     UINT8                            File
  )
{
  OC_AUDIO_PROTOCOL_PRIVATE        *Private;
  CONST APPLE_VOICE_OVER_FILE_MAP  *Map;
  BOOLEAN                          Localised;

  Localised = FALSE;
  Map       = NULL;

  if (  (File >= AppleVoiceOverAudioFileIndexLocalisedMin)
     && (File < AppleVoiceOverAudioFileIndexLocalisedMax))
  {
    Localised = TRUE;
    Map       = &mAppleVoiceOverLocalisedAudioFiles[File - AppleVoiceOverAudioFileIndexLocalisedMin];
  } else if (  (File >= AppleVoiceOverAudioFileIndexNonLocalisedMin)
            && (File < AppleVoiceOverAudioFileIndexNonLocalisedMax))
  {
    Map = &mAppleVoiceOverNonLocalisedAudioFiles[File - AppleVoiceOverAudioFileIndexNonLocalisedMin];
  }

  if ((Map == NULL) || (Map->BasePath == NULL) || (Map->BaseType == NULL)) {
    DEBUG ((DEBUG_INFO, "OCAU: Unsupported Apple voice over file index %u\n", File));
    return EFI_UNSUPPORTED;
  }

  Private = OC_AUDIO_PROTOCOL_PRIVATE_FROM_VOICE_OVER (This);
  return Private->OcAudio.PlayFile (&Private->OcAudio, Map->BasePath, Map->BaseType, Localised, 0, FALSE, TRUE);
}

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverSetLanguageCode (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     UINT8                            LanguageCode
  )
{
  OC_AUDIO_PROTOCOL_PRIVATE  *Private;

  if ((LanguageCode == 0) || (LanguageCode >= ARRAY_SIZE (mLanguagePairing))) {
    return EFI_INVALID_PARAMETER;
  }

  Private           = OC_AUDIO_PROTOCOL_PRIVATE_FROM_VOICE_OVER (This);
  Private->Language = LanguageCode;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverSetLanguageString (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     CONST CHAR8                      *LanguageString
  )
{
  OC_AUDIO_PROTOCOL_PRIVATE  *Private;
  UINTN                      Index;

  DEBUG ((DEBUG_INFO, "OCAU: Setting language to %a\n", LanguageString));

  Private = OC_AUDIO_PROTOCOL_PRIVATE_FROM_VOICE_OVER (This);

  for (Index = 1; Index < ARRAY_SIZE (mLanguagePairing); ++Index) {
    if (AsciiStrCmp (mLanguagePairing[Index], LanguageString) == 0) {
      Private->Language = (UINT8)Index;
      return EFI_SUCCESS;
    }
  }

  for (Index = 1; Index < ARRAY_SIZE (mLanguagePairing); ++Index) {
    if (  (AsciiStrLen (mLanguagePairing[Index]) == 2)
       && (AsciiStrnCmp (mLanguagePairing[Index], LanguageString, 2) == 0))
    {
      Private->Language = (UINT8)Index;
      return EFI_SUCCESS;
    }
  }

  if (AsciiStrnCmp (LanguageString, "zh", 2) == 0) {
    Private->Language = AppleVoiceOverLanguageZhHans;
    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
InternalOcAudioVoiceOverGetLanguage (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  OUT    UINT8                            *LanguageCode,
  OUT    CONST CHAR8                      **LanguageString
  )
{
  OC_AUDIO_PROTOCOL_PRIVATE  *Private;

  Private         = OC_AUDIO_PROTOCOL_PRIVATE_FROM_VOICE_OVER (This);
  *LanguageCode   = Private->Language;
  *LanguageString = mLanguagePairing[Private->Language];
  return EFI_SUCCESS;
}
