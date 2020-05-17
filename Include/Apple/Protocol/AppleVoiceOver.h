/** @file
  Apple VoiceOver Audio protocol.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_VO_AUDIO_H
#define APPLE_VO_AUDIO_H

/**
  Apple VoiceOver Audio protocol GUID.
  This protocol is present on Gibraltar Macs (ones with T1/T2 and custom HDA).
  F4CB0B78-243B-11E7-A524-B8E8562CBAFA
**/
#define APPLE_VOICE_OVER_AUDIO_PROTOCOL_GUID \
  { 0xF4CB0B78, 0x243B, 0x11E7,              \
    { 0xA5, 0x24, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA } }

typedef struct APPLE_VOICE_OVER_AUDIO_PROTOCOL_ APPLE_VOICE_OVER_AUDIO_PROTOCOL;

/**
  Apple VoiceOver Audio protocol revision.
**/
#define APPLE_VOICE_OVER_AUDIO_PROTOCOL_REVISION 0x10000


/**
  These files can be found either on BridgeOS volume:
  /System/Library/PrivateFrameworks/BridgeAccessibilitySupport.framework/AXEFIAudio_[ru]_[VoiceOverOn].aiff
  Here language code (ru) can be both full (ru_RU) and short (ru).
  For unlocalised files language code can be missing entirely, e.g. AXEFIAudio_Beep.aiff.
  Or in /Volumes/Preboot/.../System/Library/Caches/com.apple.corestorage/EFILoginLocalizations/sound.efires
  For preboot the file format is sound_SCREFIAudio.VoiceOverOn.
  Files marked with * are only present on BridgeOS.
**/
typedef enum {
  AppleVoiceOverAudioFileVoiceOverOn                 = 0x01, ///< VoiceOverOn
  AppleVoiceOverAudioFileVoiceOverOff                = 0x02, ///< VoiceOverOff
  AppleVoiceOverAudioFileUsername                    = 0x03, ///< Username
  AppleVoiceOverAudioFilePassword                    = 0x04, ///< Password
  AppleVoiceOverAudioFileUsernameOrPasswordIncorrect = 0x05, ///< UsernameOrPasswordIncorrect
  AppleVoiceOverAudioFileAccountLockedTryLater       = 0x06, ///< AccountLockedTryLater (*)
  AppleVoiceOverAudioFileAccountLocked               = 0x07, ///< AccountLocked (*)
  AppleVoiceOverAudioFileVoiceOverBoot               = 0x3B, ///< VoiceOver_Boot (*)
  AppleVoiceOverAudioFileVoiceOverBoot2              = 0x3C, ///< VoiceOver_Boot (*)
  AppleVoiceOverAudioFileClick                       = 0x3D, ///< Click (*)
  AppleVoiceOverAudioFileBeep                        = 0x3E, ///< Beep
  AppleVoiceOverAudioFileMax                         = 0x3F,
} APPLE_VOICE_OVER_AUDIO_FILE;

/**
  VoiceOver language codes.
**/
typedef enum {
  AppleVoiceOverLanguageAr     = 0x01, ///< ar
  AppleVoiceOverLanguageCa     = 0x02, ///< ca
  AppleVoiceOverLanguageCs     = 0x03, ///< cs
  AppleVoiceOverLanguageDa     = 0x04, ///< da
  AppleVoiceOverLanguageDe     = 0x05, ///< de
  AppleVoiceOverLanguageEl     = 0x06, ///< el
  AppleVoiceOverLanguageEn     = 0x07, ///< en
  AppleVoiceOverLanguageEs     = 0x08, ///< es
  AppleVoiceOverLanguageEs419  = 0x09, ///< es-419
  AppleVoiceOverLanguageEsMx   = 0x0A, ///< es-MX
  AppleVoiceOverLanguageFi     = 0x0B, ///< fi
  AppleVoiceOverLanguageFr     = 0x0C, ///< fr
  AppleVoiceOverLanguageHe     = 0x0D, ///< he
  AppleVoiceOverLanguageHi     = 0x0E, ///< hi
  AppleVoiceOverLanguageHr     = 0x0F, ///< hr
  AppleVoiceOverLanguageHu     = 0x10, ///< hu
  AppleVoiceOverLanguageId     = 0x11, ///< id
  AppleVoiceOverLanguageIt     = 0x12, ///< it
  AppleVoiceOverLanguageJa     = 0x13, ///< ja
  AppleVoiceOverLanguageKo     = 0x14, ///< ko
  AppleVoiceOverLanguageNl     = 0x15, ///< nl
  AppleVoiceOverLanguageNo     = 0x16, ///< no
  AppleVoiceOverLanguageMy     = 0x17, ///< my
  AppleVoiceOverLanguagePl     = 0x18, ///< pl
  AppleVoiceOverLanguagePt     = 0x19, ///< pt
  AppleVoiceOverLanguagePtPt   = 0x1A, ///< pt-PT
  AppleVoiceOverLanguageRo     = 0x1B, ///< ro
  AppleVoiceOverLanguageRu     = 0x1C, ///< ru
  AppleVoiceOverLanguageSk     = 0x1D, ///< sk
  AppleVoiceOverLanguageSv     = 0x1E, ///< sv
  AppleVoiceOverLanguageTh     = 0x1F, ///< th
  AppleVoiceOverLanguageTr     = 0x20, ///< tr
  AppleVoiceOverLanguageUk     = 0x21, ///< uk
  AppleVoiceOverLanguageVi     = 0x22, ///< vi
  AppleVoiceOverLanguageZhHans = 0x23, ///< zh-Hans
  AppleVoiceOverLanguageZhHant = 0x24, ///< zh-Hant
} APPLE_VOICE_OVER_LANGUAGE_CODE;

/**
  Play audio file.

  @param[in]  This      VoiceOver protocol instance.
  @param[in]  File      File to play.

  @retval EFI_SUCCESS on successful playback startup.
  @retval EFI_NOT_FOUND when audio device is missing.
  @retval EFI_INVALID_PARAMETER when audio file is not valid.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_VOICE_OVER_AUDIO_PLAY) (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     UINT8                            File
  );

/**
  Set language for VoiceOver support via code.

  @param[in]  This          VoiceOver protocol instance.
  @param[in]  LanguageCode  Language code (e.g. AppleVoiceOverLanguageEn).

  @retval EFI_SUCCESS on successful language update.
  @retval EFI_INVALID_PARAMETER when the language is unsupported.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_VOICE_OVER_AUDIO_SET_LANGUAGE_CODE) (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     UINT8                            LanguageCode
  );

/**
  Set language for VoiceOver support via string.

  @param[in]  This            VoiceOver protocol instance.
  @param[in]  LanguageString  Language string (e.g. ru or ru-RU).

  @retval EFI_SUCCESS on successful language update.
  @retval EFI_INVALID_PARAMETER when the language is unsupported.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_VOICE_OVER_AUDIO_SET_LANGUAGE_STRING) (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  IN     CONST CHAR8                      *LanguageString
  );

/**
  Get current VoiceOver language.

  @param[in]  This             VoiceOver protocol instance.
  @param[out] LanguageCode     Current language code (e.g. AppleVoiceOverLanguageRu).
  @param[out] LanguageString   Current language string (e.g. ru) or NULL.

  @retval EFI_SUCCESS on successful language update.
  @retval EFI_INVALID_PARAMETER when the language is unsupported.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_VOICE_OVER_AUDIO_GET_LANGUAGE) (
  IN     APPLE_VOICE_OVER_AUDIO_PROTOCOL  *This,
  OUT    UINT8                            *LanguageCode,
  OUT    CONST CHAR8                      **LanguageString
  );

/**
  VoiceOver protocol.
**/
struct APPLE_VOICE_OVER_AUDIO_PROTOCOL_ {
  UINTN Revision;
  APPLE_VOICE_OVER_AUDIO_PLAY                 Play;
  APPLE_VOICE_OVER_AUDIO_SET_LANGUAGE_CODE    SetLanguageCode;
  APPLE_VOICE_OVER_AUDIO_SET_LANGUAGE_STRING  SetLanguageString;
  APPLE_VOICE_OVER_AUDIO_GET_LANGUAGE         GetLanguage;
};

extern EFI_GUID gAppleVOAudioProtocolGuid;

#endif // APPLE_VO_AUDIO_H
