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

#include <Protocol/AppleVoiceOver.h>
#include <Protocol/DevicePath.h>

#define OC_AUDIO_PROTOCOL_REVISION  0x010000

//
// OC_AUDIO_PROTOCOL_GUID
// 4B228577-6274-4A48-82AE-0713A1171987
//
#define OC_AUDIO_PROTOCOL_GUID  \
  { 0x4B228577, 0x6274, 0x4A48, \
    { 0x82, 0xAE, 0x07, 0x13, 0xA1, 0x17, 0x19, 0x87 } }

typedef struct OC_AUDIO_PROTOCOL_ OC_AUDIO_PROTOCOL;

/**
  Custom OpenCore audio files.
**/
typedef enum {
  OcVoiceOverAudioFileBase                  = 0x1000,

  OcVoiceOverAudioFileIndexBase             = 0x1000,
  OcVoiceOverAudioFile1                     = 0x1001,
  OcVoiceOverAudioFile2                     = 0x1002,
  OcVoiceOverAudioFile3                     = 0x1003,
  OcVoiceOverAudioFile4                     = 0x1004,
  OcVoiceOverAudioFile5                     = 0x1005,
  OcVoiceOverAudioFile6                     = 0x1006,
  OcVoiceOverAudioFile7                     = 0x1007,
  OcVoiceOverAudioFile8                     = 0x1008,
  OcVoiceOverAudioFile9                     = 0x1009,
  OcVoiceOverAudioFileIndexAlphabetical     = 0x100A,
  OcVoiceOverAudioFileLetterA               = 0x100A,
  OcVoiceOverAudioFileLetterB               = 0x100B,
  OcVoiceOverAudioFileLetterC               = 0x100C,
  OcVoiceOverAudioFileLetterD               = 0x100D,
  OcVoiceOverAudioFileLetterE               = 0x100E,
  OcVoiceOverAudioFileLetterF               = 0x100F,
  OcVoiceOverAudioFileLetterG               = 0x1010,
  OcVoiceOverAudioFileLetterH               = 0x1011,
  OcVoiceOverAudioFileLetterI               = 0x1012,
  OcVoiceOverAudioFileLetterJ               = 0x1013,
  OcVoiceOverAudioFileLetterK               = 0x1014,
  OcVoiceOverAudioFileLetterL               = 0x1015,
  OcVoiceOverAudioFileLetterM               = 0x1016,
  OcVoiceOverAudioFileLetterN               = 0x1017,
  OcVoiceOverAudioFileLetterO               = 0x1018,
  OcVoiceOverAudioFileLetterP               = 0x1019,
  OcVoiceOverAudioFileLetterQ               = 0x101A,
  OcVoiceOverAudioFileLetterR               = 0x101B,
  OcVoiceOverAudioFileLetterS               = 0x101C,
  OcVoiceOverAudioFileLetterT               = 0x101D,
  OcVoiceOverAudioFileLetterU               = 0x101E,
  OcVoiceOverAudioFileLetterV               = 0x101F,
  OcVoiceOverAudioFileLetterW               = 0x1020,
  OcVoiceOverAudioFileLetterX               = 0x1021,
  OcVoiceOverAudioFileLetterY               = 0x1022,
  OcVoiceOverAudioFileLetterZ               = 0x1023,
  OcVoiceOverAudioFileIndexMax              = 0x1023,

  OcVoiceOverAudioFileAbortTimeout          = 0x1030,
  OcVoiceOverAudioFileChooseOS              = 0x1031,
  OcVoiceOverAudioFileDefault               = 0x1032,
  OcVoiceOverAudioFileEnterPassword         = 0x1033,
  OcVoiceOverAudioFileExecutionFailure      = 0x1034,
  OcVoiceOverAudioFileExecutionSuccessful   = 0x1035,
  OcVoiceOverAudioFileExternal              = 0x1036,
  OcVoiceOverAudioFileExternalOS            = 0x1037,
  OcVoiceOverAudioFileExternalTool          = 0x1038,
  OcVoiceOverAudioFileLoading               = 0x1039,
  OcVoiceOverAudioFilemacOS                 = 0x103A,
  OcVoiceOverAudioFilemacOS_Recovery        = 0x103B,
  OcVoiceOverAudioFilemacOS_TimeMachine     = 0x103C,
  OcVoiceOverAudioFilemacOS_UpdateFw        = 0x103D,
  OcVoiceOverAudioFileOtherOS               = 0x103E,
  OcVoiceOverAudioFilePasswordAccepted      = 0x103F,
  OcVoiceOverAudioFilePasswordIncorrect     = 0x1040,
  OcVoiceOverAudioFilePasswordRetryLimit    = 0x1041,
  OcVoiceOverAudioFileReloading             = 0x1042,
  OcVoiceOverAudioFileResetNVRAM            = 0x1043,
  OcVoiceOverAudioFileSelected              = 0x1044,
  OcVoiceOverAudioFileShowAuxiliary         = 0x1045,
  OcVoiceOverAudioFileTimeout               = 0x1046,
  OcVoiceOverAudioFileUEFI_Shell            = 0x1047,
  OcVoiceOverAudioFileWelcome               = 0x1048,
  OcVoiceOverAudioFileWindows               = 0x1049,

  OcVoiceOverAudioFileMax                   = 0x104A,
} OC_VOICE_OVER_AUDIO_FILE;

STATIC_ASSERT (OcVoiceOverAudioFileIndexMax - OcVoiceOverAudioFileIndexBase == 9 + 26, "Invalid index count");

/**
  Connect to Audio I/O.

  @param[in,out] This         Audio protocol instance.
  @param[in]     DevicePath   Controller device path, optional.
  @param[in]     CodecAddress Codec address, optional.
  @param[in]     OutputIndex  Output index, optional.
  @param[in]     Volume       Raw volume level from 0 to 100.

  @retval EFI_SUCESS on success.
  @retval EFI_NOT_FOUND when missing.
  @retval EFI_UNSUPPORTED on failure.
**/
typedef
EFI_STATUS
(EFIAPI* OC_AUDIO_CONNECT) (
  IN OUT OC_AUDIO_PROTOCOL         *This,
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL,
  IN     UINT8                     CodecAddress OPTIONAL,
  IN     UINT8                     OutputIndex  OPTIONAL,
  IN     UINT8                     Volume
  );

/**
  Retrive file contents callback.

  @param[in,out]  Context      Externally specified context.
  @param[in]      File         File identifier, see APPLE_VOICE_OVER_AUDIO_FILE.
  @paran[in]      LanguageCode Language code for the file.
  @param[out]     Buffer       Pointer to buffer.
  @param[out]     BufferSize   Pointer to buffer size.

  @retval EFI_SUCCESS on successful file lookup.
**/
typedef
EFI_STATUS
(EFIAPI* OC_AUDIO_PROVIDER_ACQUIRE) (
  IN  VOID                            *Context,
  IN  UINT32                          File,
  IN  APPLE_VOICE_OVER_LANGUAGE_CODE  LanguageCode,
  OUT UINT8                           **Buffer,
  OUT UINT32                          *BufferSize
  );

/**
  Release file contents given by acquire callback.

  @param[in,out]  Context      Externally specified context.
  @param[out]     Buffer       Pointer to buffer.

  @retval EFI_SUCCESS on successful release.
**/
typedef
EFI_STATUS
(EFIAPI* OC_AUDIO_PROVIDER_RELEASE) (
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
(EFIAPI* OC_AUDIO_SET_PROVIDER) (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     OC_AUDIO_PROVIDER_ACQUIRE  Acquire,
  IN     OC_AUDIO_PROVIDER_RELEASE  Release  OPTIONAL,
  IN     VOID                       *Context
  );

/**
  Play file.

  @param[in,out] This         Audio protocol instance.
  @param[in]     File         File to play.
  @param[in]     Wait         Wait for completion of the previous track.

  @retval EFI_SUCCESS on successful playback startup.
**/
typedef
EFI_STATUS
(EFIAPI* OC_AUDIO_PLAY_FILE) (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     UINT32                     File,
  IN     BOOLEAN                    Wait
  );

/**
  Stop playback.

  @param[in,out] This         Audio protocol instance.
  @param[in]     Wait         Wait for audio completion.

  @retval EFI_SUCCESS on successful playback stop.
**/
typedef
EFI_STATUS
(EFIAPI* OC_AUDIO_STOP_PLAYBACK) (
  IN OUT OC_AUDIO_PROTOCOL          *This,
  IN     BOOLEAN                    Wait
  );

//
// Includes a revision for debugging reasons.
//
struct OC_AUDIO_PROTOCOL_ {
  UINTN                   Revision;
  OC_AUDIO_CONNECT        Connect;
  OC_AUDIO_SET_PROVIDER   SetProvider;
  OC_AUDIO_PLAY_FILE      PlayFile;
  OC_AUDIO_STOP_PLAYBACK  StopPlayback;
};

extern EFI_GUID gOcAudioProtocolGuid;

#endif // OC_AUDIO_PROTOCOL_H
