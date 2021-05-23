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

#include "BootManagementInternal.h"

#include <Guid/AppleFile.h>
#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>

#include <IndustryStandard/AppleCsrConfig.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/AppleKeyMapAggregator.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcAudio.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcTimerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcRtcLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
OcPlayAudioFile (
  IN     OC_PICKER_CONTEXT  *Context,
  IN     UINT32             File,
  IN     BOOLEAN            Fallback
  )
{
  EFI_STATUS  Status;

  if (!Context->PickerAudioAssist) {
    return EFI_SUCCESS;
  }

  if (Context->OcAudio == NULL) {
    Status = gBS->LocateProtocol (
      &gOcAudioProtocolGuid,
      NULL,
      (VOID **) &Context->OcAudio
      );
    if (EFI_ERROR (Status)) {
      Context->OcAudio = NULL;
    }
  }

  if (Context->OcAudio != NULL) {
    Status = Context->OcAudio->PlayFile (Context->OcAudio, File, TRUE);
  }

  if (Fallback && EFI_ERROR (Status)) {
    switch (File) {
      case AppleVoiceOverAudioFileBeep:
        Status = OcPlayAudioBeep (
            Context,
            OC_VOICE_OVER_SIGNALS_NORMAL,
            OC_VOICE_OVER_SIGNAL_NORMAL_MS,
            OC_VOICE_OVER_SILENCE_NORMAL_MS
            );
        break;
      case OcVoiceOverAudioFileEnterPassword:
        Status = OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_PASSWORD,
          OC_VOICE_OVER_SIGNAL_NORMAL_MS,
          OC_VOICE_OVER_SILENCE_NORMAL_MS
          );
        break;
      case OcVoiceOverAudioFilePasswordAccepted:
        Status = OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_PASSWORD_OK,
          OC_VOICE_OVER_SIGNAL_NORMAL_MS,
          OC_VOICE_OVER_SILENCE_NORMAL_MS
          );
        break;
      case OcVoiceOverAudioFilePasswordIncorrect:
        Status = OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_ERROR,
          OC_VOICE_OVER_SIGNAL_ERROR_MS,
          OC_VOICE_OVER_SILENCE_ERROR_MS
          );
        break;
      case OcVoiceOverAudioFilePasswordRetryLimit:
        Status = OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_HWERROR,
          OC_VOICE_OVER_SIGNAL_ERROR_MS,
          OC_VOICE_OVER_SILENCE_ERROR_MS
          );
        break;
      case OcVoiceOverAudioFileExecutionFailure:
        Status = OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_ERROR,
          OC_VOICE_OVER_SIGNAL_ERROR_MS,
          OC_VOICE_OVER_SIGNAL_NORMAL_MS
          );
        break;
      default:
        //
        // Should we introduce some special code?
        //
        break;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
OcPlayAudioBeep (
  IN     OC_PICKER_CONTEXT        *Context,
  IN     UINT32                   ToneCount,
  IN     UINT32                   ToneLength,
  IN     UINT32                   SilenceLength
  )
{
  EFI_STATUS  Status;

  if (!Context->PickerAudioAssist) {
    return EFI_SUCCESS;
  }

  if (Context->BeepGen == NULL) {
    Status = gBS->LocateProtocol (
      &gAppleBeepGenProtocolGuid,
      NULL,
      (VOID **) &Context->BeepGen
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Context->BeepGen->GenBeep == NULL) {
    return EFI_UNSUPPORTED;
  }

  return Context->BeepGen->GenBeep (ToneCount, ToneLength, SilenceLength);
}

EFI_STATUS
EFIAPI
OcPlayAudioEntry (
  IN     OC_PICKER_CONTEXT  *Context,
  IN     OC_BOOT_ENTRY      *Entry
  )
{
  OcPlayAudioFile (Context, OcVoiceOverAudioFileIndexBase + Entry->EntryIndex, FALSE);

  if (Entry->Type == OC_BOOT_APPLE_OS) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFilemacOS, FALSE);
  } else if (Entry->Type == OC_BOOT_APPLE_RECOVERY) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFilemacOS_Recovery, FALSE);
  } else if (Entry->Type == OC_BOOT_APPLE_TIME_MACHINE) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFilemacOS_TimeMachine, FALSE);
  } else if (Entry->Type == OC_BOOT_APPLE_FW_UPDATE) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFilemacOS_UpdateFw, FALSE);
  } else if (Entry->Type == OC_BOOT_WINDOWS) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFileWindows, FALSE);
  }  else if (Entry->Type == OC_BOOT_EXTERNAL_OS) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFileExternalOS, FALSE);
  } else if (Entry->Type == OC_BOOT_RESET_NVRAM) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFileResetNVRAM, FALSE);
  } else if (Entry->Type == OC_BOOT_TOGGLE_SIP) {
    ASSERT (
      StrCmp (Entry->Name, OC_MENU_SIP_IS_DISABLED) == 0 ||
      StrCmp (Entry->Name, OC_MENU_SIP_IS_ENABLED) == 0
      );
    if (StrCmp (Entry->Name, OC_MENU_SIP_IS_DISABLED) == 0) {
      OcPlayAudioFile (Context, OcVoiceOverAudioFileSIPIsDisabled, FALSE);
    } else {
      OcPlayAudioFile (Context, OcVoiceOverAudioFileSIPIsEnabled, FALSE);
    }
  } else if (Entry->Type == OC_BOOT_EXTERNAL_TOOL) {
    if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_RESET_NVRAM) != NULL) {
      OcPlayAudioFile (Context, OcVoiceOverAudioFileResetNVRAM, FALSE);
    } else if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_UEFI_SHELL) != NULL) {
      OcPlayAudioFile (Context, OcVoiceOverAudioFileUEFI_Shell, FALSE);
    } else {
      OcPlayAudioFile (Context, OcVoiceOverAudioFileExternalTool, FALSE);
    }
  } else {
    OcPlayAudioFile (Context, OcVoiceOverAudioFileOtherOS, FALSE);
  }

  if (Entry->IsExternal) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFileExternal, FALSE);
  }

  if (Entry->IsFolder) {
    OcPlayAudioFile (Context, OcVoiceOverAudioFileDiskImage, FALSE);
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OcToggleVoiceOver (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  UINT32             File  OPTIONAL
  )
{
  if (!Context->PickerAudioAssist) {
    Context->PickerAudioAssist = TRUE;
    OcPlayAudioFile (Context, OcVoiceOverAudioFileWelcome, FALSE);

    if (File != 0) {
      OcPlayAudioFile (Context, File, TRUE);
    }
  } else {
    OcPlayAudioBeep (
      Context,
      OC_VOICE_OVER_SIGNALS_ERROR,
      OC_VOICE_OVER_SIGNAL_ERROR_MS,
      OC_VOICE_OVER_SILENCE_ERROR_MS
      );
    Context->PickerAudioAssist = FALSE;
  }
}
