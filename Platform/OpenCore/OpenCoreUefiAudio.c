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

#include <OpenCore.h>

#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>
#include <Guid/GlobalVariable.h>

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
  UINT8   *Buffer;
  UINT32  Size;
} OC_AUDIO_FILE;

STATIC OC_AUDIO_FILE  mAppleAudioFiles[AppleVoiceOverAudioFileMax];
STATIC OC_AUDIO_FILE  mOcAudioFiles[OcVoiceOverAudioFileMax - OcVoiceOverAudioFileBase];
//
// Note, currently we are not I/O bound, so enabling this has no effect at all.
// Reconsider it when we resolve lags with AudioDxe.
//
STATIC BOOLEAN        mEnableAudioCaching = FALSE;

STATIC
EFI_STATUS
EFIAPI
OcAudioAcquireFile (
  IN  VOID                            *Context,
  IN  UINT32                          File,
  IN  APPLE_VOICE_OVER_LANGUAGE_CODE  LanguageCode,
  OUT UINT8                           **Buffer,
  OUT UINT32                          *BufferSize
  )
{
  EFI_STATUS          Status;
  CHAR8               IndexPath[8];
  CHAR16              FilePath[OC_STORAGE_SAFE_PATH_MAX];
  OC_STORAGE_CONTEXT  *Storage;
  CONST CHAR8         *BaseType;
  CONST CHAR8         *BasePath;
  BOOLEAN             Localised;
  OC_AUDIO_FILE       *CacheFile;

  Storage   = (OC_STORAGE_CONTEXT *) Context;
  Localised = TRUE;
  CacheFile = NULL;

  if (File >= OcVoiceOverAudioFileBase && File < OcVoiceOverAudioFileMax) {
    if (mEnableAudioCaching) {
      CacheFile = &mOcAudioFiles[File - OcVoiceOverAudioFileBase];
      if (CacheFile->Buffer != NULL) {
        *Buffer = CacheFile->Buffer;
        *BufferSize = CacheFile->Size;
        return EFI_SUCCESS;
      }
    }

    BaseType  = "OCEFIAudio";
    if (File > OcVoiceOverAudioFileIndexBase && File <= OcVoiceOverAudioFileIndexMax) {
      Status = OcAsciiSafeSPrint (
        IndexPath,
        sizeof (IndexPath),
        "%a%c",
        File >= OcVoiceOverAudioFileIndexAlphabetical ? "Letter" : "",
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[File - OcVoiceOverAudioFileIndexBase]
        );
      ASSERT_EFI_ERROR (Status);
      BasePath = IndexPath;
    } else {
      switch (File) {
        case OcVoiceOverAudioFileAbortTimeout:
          BasePath = "AbortTimeout";
          break;
        case OcVoiceOverAudioFileChooseOS:
          BasePath = "ChooseOS";
          break;
        case OcVoiceOverAudioFileDefault:
          BasePath = "Default";
          break;
        case OcVoiceOverAudioFileEnterPassword:
          BasePath = "EnterPassword";
          break;
        case OcVoiceOverAudioFileExecutionFailure:
          BasePath = "ExecutionFailure";
          break;
        case OcVoiceOverAudioFileExecutionSuccessful:
          BasePath = "ExecutionSuccessful";
          break;
        case OcVoiceOverAudioFileExternal:
          BasePath = "External";
          break;
        case OcVoiceOverAudioFileExternalOS:
          BasePath = "ExternalOS";
          break;
        case OcVoiceOverAudioFileExternalTool:
          BasePath = "ExternalTool";
          break;
        case OcVoiceOverAudioFileLoading:
          BasePath = "Loading";
          break;
        case OcVoiceOverAudioFilemacOS:
          BasePath = "macOS";
          break;
        case OcVoiceOverAudioFilemacOS_Recovery:
          BasePath = "macOS_Recovery";
          break;
        case OcVoiceOverAudioFilemacOS_TimeMachine:
          BasePath = "macOS_TimeMachine";
          break;
        case OcVoiceOverAudioFilemacOS_UpdateFw:
          BasePath = "macOS_UpdateFw";
          break;
        case OcVoiceOverAudioFileOtherOS:
          BasePath = "OtherOS";
          break;
        case OcVoiceOverAudioFilePasswordAccepted:
          BasePath = "PasswordAccepted";
          break;
        case OcVoiceOverAudioFilePasswordIncorrect:
          BasePath = "PasswordIncorrect";
          break;
        case OcVoiceOverAudioFilePasswordRetryLimit:
          BasePath = "PasswordRetryLimit";
          break;
        case OcVoiceOverAudioFileReloading:
          BasePath = "Reloading";
          break;
        case OcVoiceOverAudioFileResetNVRAM:
          BasePath = "ResetNVRAM";
          break;
        case OcVoiceOverAudioFileSelected:
          BasePath = "Selected";
          break;
        case OcVoiceOverAudioFileShowAuxiliary:
          BasePath = "ShowAuxiliary";
          break;
        case OcVoiceOverAudioFileTimeout:
          BasePath = "Timeout";
          break;
        case OcVoiceOverAudioFileUEFI_Shell:
          BasePath = "UEFI_Shell";
          break;
        case OcVoiceOverAudioFileWelcome:
          BasePath = "Welcome";
          break;
        case OcVoiceOverAudioFileWindows:
          BasePath = "Windows";
          break;
        default:
          BasePath = NULL;
          break;
      }
    }
  } else if (File < AppleVoiceOverAudioFileMax) {
    if (mEnableAudioCaching) {
      CacheFile = &mAppleAudioFiles[File];
      if (CacheFile->Buffer != NULL) {
        *Buffer = CacheFile->Buffer;
        *BufferSize = CacheFile->Size;
        return EFI_SUCCESS;
      }
    }

    BaseType  = "AXEFIAudio";
    switch (File) {
      case AppleVoiceOverAudioFileVoiceOverOn:
        BasePath = "VoiceOverOn";
        break;
      case AppleVoiceOverAudioFileVoiceOverOff:
        BasePath = "VoiceOverOff";
        break;
      case AppleVoiceOverAudioFileUsername:
        BasePath = "Username";
        break;
      case AppleVoiceOverAudioFilePassword:
        BasePath = "Password";
        break;
      case AppleVoiceOverAudioFileUsernameOrPasswordIncorrect:
        BasePath = "UsernameOrPasswordIncorrect";
        break;
      case AppleVoiceOverAudioFileAccountLockedTryLater:
        BasePath = "AccountLockedTryLater";
        break;
      case AppleVoiceOverAudioFileAccountLocked:
        BasePath = "AccountLocked";
        break;
      case AppleVoiceOverAudioFileVoiceOverBoot:
        BaseType = "OCEFIAudio";
        BasePath = "VoiceOver_Boot";
        Localised = FALSE;
        break;
      case AppleVoiceOverAudioFileVoiceOverBoot2:
        BasePath = "VoiceOver_Boot";
        Localised = FALSE;
        break;
      case AppleVoiceOverAudioFileClick:
        BasePath = "Click";
        Localised = FALSE;
        break;
      case AppleVoiceOverAudioFileBeep:
        BasePath = "Beep";
        Localised = FALSE;
        break;
      default:
        BasePath = NULL;
        break;
    }
  } else {
    BasePath = NULL;
  }

  if (BasePath == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Unknown Wave %d\n", File));
    return EFI_NOT_FOUND;
  }

  if (Localised) {
    Status = OcUnicodeSafeSPrint (
      FilePath,
      sizeof (FilePath),
      OPEN_CORE_AUDIO_PATH "%a_%a_%a.wav",
      BaseType,
      OcLanguageCodeToString (LanguageCode),
      BasePath
      );
    ASSERT_EFI_ERROR (Status);

    if (!OcStorageExistsFileUnicode (Context, FilePath)) {
      Status = OcUnicodeSafeSPrint (
        FilePath,
        sizeof (FilePath),
        OPEN_CORE_AUDIO_PATH "%a_%a_%a.wav",
        BaseType,
        OcLanguageCodeToString (AppleVoiceOverLanguageEn),
        BasePath
        );
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    Status = OcUnicodeSafeSPrint (
      FilePath,
      sizeof (FilePath),
      OPEN_CORE_AUDIO_PATH "%a_%a.wav",
      BaseType,
      BasePath
      );
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "OC: Wave %s was requested\n", FilePath));

  *Buffer = OcStorageReadFileUnicode (
    Storage,
    FilePath,
    BufferSize
    );
  if (*Buffer == NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OC: Wave %s cannot be found!\n",
      FilePath
      ));
    return EFI_NOT_FOUND;
  }

  if (CacheFile != NULL) {
    CacheFile->Buffer = *Buffer;
    CacheFile->Size   = *BufferSize;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcAudioReleaseFile (
  IN  VOID                            *Context,
  IN  UINT8                           *Buffer
  )
{
  if (!mEnableAudioCaching) {
    FreePool (Buffer);
  }
  return EFI_SUCCESS;
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
  EFI_STATUS                       Status;
  CHAR8                            *AsciiDevicePath;
  CHAR16                           *UnicodeDevicePath;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  OC_AUDIO_PROTOCOL                *OcAudio;
  UINT8                            VolumeLevel;
  BOOLEAN                          Muted;

  if (!Config->Uefi.Audio.AudioSupport) {
    DEBUG ((DEBUG_INFO, "OC: Requested not to use audio\n"));
    return;
  }

  VolumeLevel = OcGetVolumeLevel (Config->Uefi.Audio.VolumeAmplifier, &Muted);

  DevicePath        = NULL;
  AsciiDevicePath   = OC_BLOB_GET (&Config->Uefi.Audio.AudioDevice);
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

  OcAudio = OcAudioInstallProtocols (FALSE);
  if (OcAudio == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Cannot locate OcAudio protocol\n"));
    if (DevicePath != NULL) {
      FreePool (DevicePath);
    }
    return;
  }

  //
  // Never disable sound completely, as it is vital for accessability.
  //
  Status = OcAudio->Connect (
    OcAudio,
    DevicePath,
    Config->Uefi.Audio.AudioCodec,
    Config->Uefi.Audio.AudioOut,
    VolumeLevel < Config->Uefi.Audio.MinimumVolume
      ? Config->Uefi.Audio.MinimumVolume : VolumeLevel
    );

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Audio connection failed - %r\n", Status));
    return;
  }

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

  OcSetVoiceOverLanguage (NULL);

  if (Config->Uefi.Audio.PlayChime && VolumeLevel >= Config->Uefi.Audio.MinimumVolume && !Muted) {
    DEBUG ((DEBUG_INFO, "OC: Starting to play chime...\n"));
    Status = OcAudio->PlayFile (
      OcAudio,
      AppleVoiceOverAudioFileVoiceOverBoot,
      FALSE
      );
    DEBUG ((DEBUG_INFO, "OC: Play chime started - %r\n", Status));
  }

  OcScheduleExitBootServices (OcAudioExitBootServices, OcAudio);
}
