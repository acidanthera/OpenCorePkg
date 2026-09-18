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

#ifndef OC_AUDIO_PROTOCOL_H
#define OC_AUDIO_PROTOCOL_H

#include <Protocol/AudioIo.h>
#include <Protocol/AppleVoiceOver.h>
#include <Protocol/DevicePath.h>

#define OC_AUDIO_PROTOCOL_REVISION  0x070000

//
// OC_AUDIO_PROTOCOL_GUID
// 4B228577-6274-4A48-82AE-0713A1171987
//
#define OC_AUDIO_PROTOCOL_GUID  \
  { 0x4B228577, 0x6274, 0x4A48, \
    { 0x82, 0xAE, 0x07, 0x13, 0xA1, 0x17, 0x19, 0x87 } }

typedef struct OC_AUDIO_PROTOCOL_ OC_AUDIO_PROTOCOL;

/**
  Voice over base types.
**/
#define OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE      "AXEFIAudio"
#define OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE  "OCEFIAudio"

/**
  Custom OpenCore audio files.
**/
#define  OC_VOICE_OVER_AUDIO_FILE_ABORT_TIMEOUT         "AbortTimeout"
#define  OC_VOICE_OVER_AUDIO_FILE_CHOOSE_OS             "ChooseOS"
#define  OC_VOICE_OVER_AUDIO_FILE_DEFAULT               "Default"
#define  OC_VOICE_OVER_AUDIO_FILE_DISK_IMAGE            "DiskImage"
#define  OC_VOICE_OVER_AUDIO_FILE_ENTER_PASSWORD        "EnterPassword"
#define  OC_VOICE_OVER_AUDIO_FILE_EXECUTION_FAILURE     "ExecutionFailure"
#define  OC_VOICE_OVER_AUDIO_FILE_EXECUTION_SUCCESSFUL  "ExecutionSuccessful"
#define  OC_VOICE_OVER_AUDIO_FILE_EXTERNAL              "External"
#define  OC_VOICE_OVER_AUDIO_FILE_EXTERNAL_OS           "ExternalOS"
#define  OC_VOICE_OVER_AUDIO_FILE_EXTERNAL_TOOL         "ExternalTool"
#define  OC_VOICE_OVER_AUDIO_FILE_FIRMWARE_SETTINGS     "FirmwareSettings"
#define  OC_VOICE_OVER_AUDIO_FILE_LOADING               "Loading"
#define  OC_VOICE_OVER_AUDIO_FILE_MAC_OS                "macOS"
#define  OC_VOICE_OVER_AUDIO_FILE_MAC_OS_RECOVERY       "macOS_Recovery"
#define  OC_VOICE_OVER_AUDIO_FILE_MAC_OS_TIME_MACHINE   "macOS_TimeMachine"
#define  OC_VOICE_OVER_AUDIO_FILE_MAC_OS_UPDATE_FW      "macOS_UpdateFw"
#define  OC_VOICE_OVER_AUDIO_FILE_NETWORK_BOOT          "NetworkBoot"
#define  OC_VOICE_OVER_AUDIO_FILE_OTHER_OS              "OtherOS"
#define  OC_VOICE_OVER_AUDIO_FILE_PASSWORD_ACCEPTED     "PasswordAccepted"
#define  OC_VOICE_OVER_AUDIO_FILE_PASSWORD_INCORRECT    "PasswordIncorrect"
#define  OC_VOICE_OVER_AUDIO_FILE_PASSWORD_RETRY_LIMIT  "PasswordRetryLimit"
#define  OC_VOICE_OVER_AUDIO_FILE_RELOADING             "Reloading"
#define  OC_VOICE_OVER_AUDIO_FILE_RESET_NVRAM           "ResetNVRAM"
#define  OC_VOICE_OVER_AUDIO_FILE_RESTART               "Restart"
#define  OC_VOICE_OVER_AUDIO_FILE_SELECTED              "Selected"
#define  OC_VOICE_OVER_AUDIO_FILE_SHOW_AUXILIARY        "ShowAuxiliary"
#define  OC_VOICE_OVER_AUDIO_FILE_SHUT_DOWN             "ShutDown"
#define  OC_VOICE_OVER_AUDIO_FILE_SIP_IS_DISABLED       "SIPIsDisabled"
#define  OC_VOICE_OVER_AUDIO_FILE_SIP_IS_ENABLED        "SIPIsEnabled"
#define  OC_VOICE_OVER_AUDIO_FILE_TIMEOUT               "Timeout"
#define  OC_VOICE_OVER_AUDIO_FILE_UEFI_SHELL            "UEFI_Shell"
#define  OC_VOICE_OVER_AUDIO_FILE_VOICE_OVER_BOOT       "VoiceOver_Boot"
#define  OC_VOICE_OVER_AUDIO_FILE_WELCOME               "Welcome"
#define  OC_VOICE_OVER_AUDIO_FILE_WINDOWS               "Windows"

/**
  Connect to Audio I/O.

  @param[in,out] This             Audio protocol instance.
  @param[in]     DevicePath       Controller device path, optional.
  @param[in]     CodecAddress     Codec address, optional.
  @param[in]     OutputIndexMask  Output index mask.

  @retval EFI_SUCESS on success.
  @retval EFI_NOT_FOUND when missing.
  @retval EFI_UNSUPPORTED on failure.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_CONNECT)(
  IN OUT OC_AUDIO_PROTOCOL         *This,
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath      OPTIONAL,
  IN     UINT8                     CodecAddress     OPTIONAL,
  IN     UINT64                    OutputIndexMask
  );

/**
  Set Audio I/O default gain.

  @param[in,out] This             Audio protocol instance.
  @param[in]     Gain             The amplifier gain (or attenuation if negative) in dB to use, relative to 0 dB level (0 dB
                                  is usually at at or near max available volume, but is not required to be so in the spec).

  @retval EFI_SUCESS on success.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_SET_DEFAULT_GAIN)(
  IN OUT OC_AUDIO_PROTOCOL         *This,
  IN     INT8                      Gain
  );

/**
  Retrieve file contents callback.

  @param[in,out]  Context      Externally specified context.
  @param[in]      BasePath     File base path.
  @param[in]      BaseType     Audio base type.
  @param[in]      Localised    Is file localised?
  @param[in]      LanguageCode Language code for the file.
  @param[out]     Buffer       Pointer to buffer.
  @param[out]     BufferSize   Pointer to buffer size.
  @param[out]     Frequency    Decoded PCM frequency.
  @param[out]     Bits         Decoded bit count.
  @param[out]     Channels     Decoded amount of channels.

  @retval EFI_SUCCESS on successful file lookup.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_PROVIDER_ACQUIRE)(
  IN  VOID                            *Context,
  IN  CONST CHAR8                     *BasePath,
  IN  CONST CHAR8                     *BaseType,
  IN  BOOLEAN                         Localised,
  IN  APPLE_VOICE_OVER_LANGUAGE_CODE  LanguageCode,
  OUT UINT8                           **Buffer,
  OUT UINT32                          *BufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ      *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS      *Bits,
  OUT UINT8                           *Channels
  );

/**
  Release file contents given by acquire callback.

  @param[in,out]  Context      Externally specified context.
  @param[out]     Buffer       Pointer to buffer.

  @retval EFI_SUCCESS on successful release.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_PROVIDER_RELEASE)(
  IN  VOID                            *Context,
  IN  UINT8                           *Buffer
  );

/**
  Set resource provider.

  @param[in,out] This         Audio protocol instance.
  @param[in]     Acquire      Resource acquire handler.
  @param[in]     Release      Resource release handler, optional.
  @param[in]     Context      Resource handler context.

  @retval EFI_SUCCESS on successful provider update.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_SET_PROVIDER)(
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     OC_AUDIO_PROVIDER_ACQUIRE  Acquire,
  IN     OC_AUDIO_PROVIDER_RELEASE  Release  OPTIONAL,
  IN     VOID                       *Context
  );

/**
  Convert raw amplifier gain setting to decibel gain value; converts using the parameters of the first
  channel specified for sound on the current codec which has non-zero amp capabilities.

  @param[in,out] This         Audio protocol instance.
  @param[in]     GainParam    Raw codec gain param.
  @param[out]    Gain         The amplifier gain (or attenuation if negative) in dB to use, relative to 0 dB level.

  @retval EFI_SUCCESS on successful conversion.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_RAW_GAIN_TO_DECIBELS)(
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     UINT8                      GainParam,
  OUT INT8                       *Gain
  );

/**
  Play file.

  @param[in,out] This         Audio protocol instance.
  @param[in]     BasePath     File base path.
  @param[in]     BaseType     Audio base type.
  @param[in]     Localised    Is file localised?
  @param[in]     Gain         The amplifier gain (or attenuation if negative) in dB to use, relative to 0 dB level.
  @param[in]     UseGain      If TRUE use provided volume level, otherwise use stored global volume level.
  @param[in]     Wait         Wait for completion of the previous track.

  @retval EFI_SUCCESS on successful playback startup.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_PLAY_FILE)(
  IN OUT OC_AUDIO_PROTOCOL              *This,
  IN     CONST CHAR8                    *BasePath,
  IN     CONST CHAR8                    *BaseType,
  IN     BOOLEAN                        Localised,
  IN     INT8                           Gain  OPTIONAL,
  IN     BOOLEAN                        UseGain,
  IN     BOOLEAN                        Wait
  );

/**
  Stop playback.

  @param[in,out] This         Audio protocol instance.
  @param[in]     Wait         Wait for audio completion.

  @retval EFI_SUCCESS on successful playback stop.
**/
typedef
EFI_STATUS
(EFIAPI *OC_AUDIO_STOP_PLAYBACK)(
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     BOOLEAN                    Wait
  );

/**
  Set playback delay.

  @param[in,out] This         Audio protocol instance.
  @param[in]     Delay        Delay after audio configuration in milliseconds.

  @return previous delay, defaults to 0.
**/
typedef
UINTN
(EFIAPI *OC_AUDIO_SET_DELAY)(
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     UINTN                      Delay
  );

//
// Includes a revision for debugging reasons.
//
struct OC_AUDIO_PROTOCOL_ {
  UINTN                            Revision;
  OC_AUDIO_CONNECT                 Connect;
  OC_AUDIO_RAW_GAIN_TO_DECIBELS    RawGainToDecibels;
  OC_AUDIO_SET_DEFAULT_GAIN        SetDefaultGain;
  OC_AUDIO_SET_PROVIDER            SetProvider;
  OC_AUDIO_PLAY_FILE               PlayFile;
  OC_AUDIO_STOP_PLAYBACK           StopPlayback;
  OC_AUDIO_SET_DELAY               SetDelay;
};

extern EFI_GUID  gOcAudioProtocolGuid;

#endif // OC_AUDIO_PROTOCOL_H
