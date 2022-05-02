/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcMainLib.h>

#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/AudioDecode.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAfterBootCompatLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcAppleEventLib.h>
#include <Library/OcAppleImageConversionLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcInputLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcAppleUserInterfaceThemeLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/OcFirmwareVolumeLib.h>
#include <Library/OcHashServicesLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcSmcLib.h>
#include <Library/OcOSInfoLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

typedef struct OC_AUDIO_FILE_ {
  UINT8     *Buffer;
  UINT32    Size;
} OC_AUDIO_FILE;

STATIC EFI_AUDIO_DECODE_PROTOCOL  *mAudioDecodeProtocol = NULL;

STATIC
VOID *
OcAudioGetFileContents (
  IN  OC_STORAGE_CONTEXT              *Storage,
  IN  CONST CHAR8                     *BasePath,
  IN  CONST CHAR8                     *BaseType,
  IN  BOOLEAN                         Localised,
  IN  CONST CHAR8                     *Extension,
  IN  APPLE_VOICE_OVER_LANGUAGE_CODE  LanguageCode,
  OUT UINT32                          *BufferSize
  )
{
  EFI_STATUS  Status;
  CHAR16      FilePath[OC_STORAGE_SAFE_PATH_MAX];
  VOID        *Buffer;

  if (Localised) {
    Status = OcUnicodeSafeSPrint (
               FilePath,
               sizeof (FilePath),
               OPEN_CORE_AUDIO_PATH "%a_%a_%a.%a",
               BaseType,
               OcLanguageCodeToString (LanguageCode),
               BasePath,
               Extension
               );
    ASSERT_EFI_ERROR (Status);

    if (!OcStorageExistsFileUnicode (Storage, FilePath)) {
      Status = OcUnicodeSafeSPrint (
                 FilePath,
                 sizeof (FilePath),
                 OPEN_CORE_AUDIO_PATH "%a_%a_%a.%a",
                 BaseType,
                 OcLanguageCodeToString (AppleVoiceOverLanguageEn),
                 BasePath,
                 Extension
                 );
      ASSERT_EFI_ERROR (Status);

      if (!OcStorageExistsFileUnicode (Storage, FilePath)) {
        return NULL;
      }
    }
  } else {
    Status = OcUnicodeSafeSPrint (
               FilePath,
               sizeof (FilePath),
               OPEN_CORE_AUDIO_PATH "%a_%a.%a",
               BaseType,
               BasePath,
               Extension
               );
    ASSERT_EFI_ERROR (Status);

    if (!OcStorageExistsFileUnicode (Storage, FilePath)) {
      return NULL;
    }
  }

  Buffer = OcStorageReadFileUnicode (
             Storage,
             FilePath,
             BufferSize
             );
  if (Buffer == NULL) {
    return NULL;
  }

  return Buffer;
}

//
// Note, currently we are not I/O bound, so implementing caching has no effect at all.
// Reconsider it when we resolve lags with AudioDxe.
//
STATIC
EFI_STATUS
EFIAPI
OcAudioAcquireFile (
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
  )
{
  EFI_STATUS          Status;
  OC_STORAGE_CONTEXT  *Storage;
  UINT8               *FileBuffer;
  UINT32              FileBufferSize;

  if ((BasePath == NULL) || (BaseType == NULL) || (Buffer == NULL) || (*Buffer == NULL)) {
    DEBUG ((DEBUG_ERROR, "OC: Illegal wave parameters\n"));
    return EFI_INVALID_PARAMETER;
  }

  Storage = (OC_STORAGE_CONTEXT *)Context;

  FileBuffer = OcAudioGetFileContents (
                 Storage,
                 BasePath,
                 BaseType,
                 Localised,
                 "mp3",
                 LanguageCode,
                 &FileBufferSize
                 );
  if (FileBuffer == NULL) {
    FileBuffer = OcAudioGetFileContents (
                   Storage,
                   BasePath,
                   BaseType,
                   Localised,
                   "wav",
                   LanguageCode,
                   &FileBufferSize
                   );
  }

  if (FileBuffer == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Wave %a %a cannot be found!\n", BaseType, BasePath));
    return EFI_NOT_FOUND;
  }

  ASSERT (mAudioDecodeProtocol != NULL);

  Status = mAudioDecodeProtocol->DecodeAny (
                                   mAudioDecodeProtocol,
                                   FileBuffer,
                                   FileBufferSize,
                                   (VOID **)Buffer,
                                   BufferSize,
                                   Frequency,
                                   Bits,
                                   Channels
                                   );

  FreePool (FileBuffer);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Wave %a %a cannot be decoded - %r!\n", BaseType, BasePath, Status));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcAudioReleaseFile (
  IN  VOID   *Context,
  IN  UINT8  *Buffer
  )
{
  FreePool (Buffer);
  return EFI_SUCCESS;
}

STATIC
BOOLEAN
OcShouldPlayChime (
  IN CONST CHAR8  *Control
  )
{
  EFI_STATUS  Status;
  UINT8       Muted;
  UINTN       Size;

  if ((Control[0] == '\0') || (AsciiStrCmp (Control, "Auto") == 0)) {
    Muted  = 0;
    Size   = sizeof (Muted);
    Status = gRT->GetVariable (
                    APPLE_STARTUP_MUTE_VARIABLE_NAME,
                    &gAppleBootVariableGuid,
                    NULL,
                    &Size,
                    &Muted
                    );
    DEBUG ((DEBUG_INFO, "OC: Using StartupMute %d for chime - %r\n", Muted, Status));
    return Muted == 0;
  }

  if (AsciiStrCmp (Control, "Enabled") == 0) {
    return TRUE;
  }

  return FALSE;
}

STATIC
VOID
EFIAPI
OcAudioExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  OC_AUDIO_PROTOCOL  *OcAudio;

  OcAudio = Context;
  OcAudio->StopPlayback (OcAudio, TRUE);
}

VOID
OcLoadUefiAudioSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS                Status;
  CHAR8                     *AsciiDevicePath;
  CHAR16                    *UnicodeDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  OC_AUDIO_PROTOCOL         *OcAudio;
  UINT8                     RawGain;
  BOOLEAN                   Muted;
  INT8                      DecibelGain;
  BOOLEAN                   TryConversion;

  if (Config->Uefi.Audio.ResetTrafficClass) {
    ResetAudioTrafficClass ();
  }

  if (!Config->Uefi.Audio.AudioSupport) {
    DEBUG ((DEBUG_INFO, "OC: Requested not to use audio\n"));
    return;
  }

  Status = gBS->LocateProtocol (
                  &gEfiAudioDecodeProtocolGuid,
                  NULL,
                  (VOID **)&mAudioDecodeProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Cannot locate audio decoder protocol - %r\n", Status));
    return;
  }

  DevicePath      = NULL;
  AsciiDevicePath = OC_BLOB_GET (&Config->Uefi.Audio.AudioDevice);
  if (AsciiDevicePath[0] != '\0') {
    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    if (UnicodeDevicePath != NULL) {
      DevicePath = ConvertTextToDevicePath (UnicodeDevicePath);
      FreePool (UnicodeDevicePath);
    }

    if (DevicePath == NULL) {
      DEBUG ((DEBUG_INFO, "OC: Requested to use invalid audio device\n"));
      return;
    }
  }

  //
  // NULL DevicePath means choose the first audio device available on the platform.
  //

  OcAudio = OcAudioInstallProtocols (FALSE, FALSE);
  if (OcAudio == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Cannot locate OcAudio protocol\n"));
    if (DevicePath != NULL) {
      FreePool (DevicePath);
    }

    return;
  }

  Status = OcAudio->Connect (
                      OcAudio,
                      DevicePath,
                      Config->Uefi.Audio.AudioCodec,
                      Config->Uefi.Audio.AudioOutMask
                      );

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Audio connection failed - %r\n", Status));
    return;
  }

  OcGetAmplifierGain (
    &RawGain,
    &DecibelGain,
    &Muted,
    &TryConversion
    );

  //
  // Conversion will only be correct if current codec and channel(s)
  // correspond to the saved raw gain parameter.
  //
  if (TryConversion) {
    OcAudio->RawGainToDecibels (OcAudio, RawGain, &DecibelGain);
  }

  //
  // Have a max. volume to limit very loud user volume during boot, as Apple do.
  //
  if (DecibelGain > Config->Uefi.Audio.MaximumGain) {
    DEBUG ((
      DEBUG_INFO,
      "OC: Limiting gain %d dB -> %d dB\n",
      DecibelGain,
      Config->Uefi.Audio.MaximumGain
      ));
    DecibelGain = Config->Uefi.Audio.MaximumGain;
  }

  //
  // Never disable audio assist sound completely, as it is vital for accessibility.
  //
  Status = OcAudio->SetDefaultGain (
                      OcAudio,
                      DecibelGain < Config->Uefi.Audio.MinimumAssistGain
      ? Config->Uefi.Audio.MinimumAssistGain : DecibelGain
                      );

  Status = OcAudio->SetProvider (
                      OcAudio,
                      OcAudioAcquireFile,
                      OcAudioReleaseFile,
                      Storage
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Audio cannot set storage provider - %r\n", Status));
    return;
  }

  OcAudio->SetDelay (
             OcAudio,
             Config->Uefi.Audio.SetupDelay
             );

  OcSetVoiceOverLanguage (NULL);

  if (  !Muted
     && (DecibelGain >= Config->Uefi.Audio.MinimumAudibleGain)
     && OcShouldPlayChime (OC_BLOB_GET (&Config->Uefi.Audio.PlayChime)))
  {
    DEBUG ((DEBUG_INFO, "OC: Starting to play chime...\n"));
    Status = OcAudio->PlayFile (
                        OcAudio,
                        OC_VOICE_OVER_AUDIO_FILE_VOICE_OVER_BOOT,
                        OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE,
                        FALSE,
                        DecibelGain,
                        TRUE,
                        FALSE
                        );
    DEBUG ((DEBUG_INFO, "OC: Play chime started - %r\n", Status));
  }

  OcScheduleExitBootServices (OcAudioExitBootServices, OcAudio);
}
