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
#include <Library/BaseOverflowLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
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
OcPreLocateAudioProtocol (
  IN     OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;

  if (Context->OcAudio == NULL) {
    Status = gBS->LocateProtocol (
                    &gOcAudioProtocolGuid,
                    NULL,
                    (VOID **)&Context->OcAudio
                    );
    if (EFI_ERROR (Status)) {
      Context->OcAudio = NULL;
    }
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

EFI_STATUS
EFIAPI
OcPlayAudioFile (
  IN     OC_PICKER_CONTEXT  *Context,
  IN     CONST CHAR8        *BasePath,
  IN     CONST CHAR8        *BaseType,
  IN     BOOLEAN            Fallback
  )
{
  EFI_STATUS  Status;

  if (!Context->PickerAudioAssist) {
    return EFI_SUCCESS;
  }

  if (Context->OcAudio != NULL) {
    Status = Context->OcAudio->PlayFile (Context->OcAudio, BasePath, BaseType, TRUE, 0, FALSE, TRUE);
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (Fallback && EFI_ERROR (Status)) {
    if (AsciiStrCmp (BaseType, OC_VOICE_OVER_AUDIO_BASE_TYPE_APPLE) == 0) {
      if (AsciiStrCmp (BasePath, APPLE_VOICE_OVER_AUDIO_FILE_BEEP) == 0) {
        Status = OcPlayAudioBeep (
                   Context,
                   OC_VOICE_OVER_SIGNALS_NORMAL,
                   OC_VOICE_OVER_SIGNAL_NORMAL_MS,
                   OC_VOICE_OVER_SILENCE_NORMAL_MS
                   );
      }
    } else if (AsciiStrCmp (BaseType, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE) == 0) {
      if (AsciiStrCmp (BasePath, OC_VOICE_OVER_AUDIO_FILE_ENTER_PASSWORD) == 0) {
        Status = OcPlayAudioBeep (
                   Context,
                   OC_VOICE_OVER_SIGNALS_PASSWORD,
                   OC_VOICE_OVER_SIGNAL_NORMAL_MS,
                   OC_VOICE_OVER_SILENCE_NORMAL_MS
                   );
      } else if (AsciiStrCmp (BasePath, OC_VOICE_OVER_AUDIO_FILE_PASSWORD_ACCEPTED) == 0) {
        Status = OcPlayAudioBeep (
                   Context,
                   OC_VOICE_OVER_SIGNALS_PASSWORD_OK,
                   OC_VOICE_OVER_SIGNAL_NORMAL_MS,
                   OC_VOICE_OVER_SILENCE_NORMAL_MS
                   );
      } else if (AsciiStrCmp (BasePath, OC_VOICE_OVER_AUDIO_FILE_PASSWORD_INCORRECT) == 0) {
        Status = OcPlayAudioBeep (
                   Context,
                   OC_VOICE_OVER_SIGNALS_ERROR,
                   OC_VOICE_OVER_SIGNAL_ERROR_MS,
                   OC_VOICE_OVER_SILENCE_ERROR_MS
                   );
      } else if (AsciiStrCmp (BasePath, OC_VOICE_OVER_AUDIO_FILE_PASSWORD_RETRY_LIMIT) == 0) {
        Status = OcPlayAudioBeep (
                   Context,
                   OC_VOICE_OVER_SIGNALS_HWERROR,
                   OC_VOICE_OVER_SIGNAL_ERROR_MS,
                   OC_VOICE_OVER_SILENCE_ERROR_MS
                   );
      } else if (AsciiStrCmp (BasePath, OC_VOICE_OVER_AUDIO_FILE_EXECUTION_FAILURE) == 0) {
        Status = OcPlayAudioBeep (
                   Context,
                   OC_VOICE_OVER_SIGNALS_ERROR,
                   OC_VOICE_OVER_SIGNAL_ERROR_MS,
                   OC_VOICE_OVER_SIGNAL_NORMAL_MS
                   );
      }
    }

    //
    // Should we introduce some fallback code?
    //
  }

  return Status;
}

EFI_STATUS
EFIAPI
OcPlayAudioBeep (
  IN     OC_PICKER_CONTEXT  *Context,
  IN     UINT32             ToneCount,
  IN     UINT32             ToneLength,
  IN     UINT32             SilenceLength
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
                    (VOID **)&Context->BeepGen
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
  CHAR8  BasePath[ARRAY_SIZE ("Letter") + 1];

  if ((Entry->EntryIndex > 0) && (Entry->EntryIndex <= 9 + 26)) {
    OcAsciiSafeSPrint (
      BasePath,
      sizeof (BasePath),
      "%a%c",
      Entry->EntryIndex > 9 ? "Letter" : "",
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Entry->EntryIndex]
      );
    OcPlayAudioFile (Context, BasePath, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  }

  if (Entry->Type == OC_BOOT_APPLE_OS) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_MAC_OS, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  } else if (Entry->Type == OC_BOOT_APPLE_RECOVERY) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_MAC_OS_RECOVERY, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  } else if (Entry->Type == OC_BOOT_APPLE_TIME_MACHINE) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_MAC_OS_TIME_MACHINE, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  } else if (Entry->Type == OC_BOOT_APPLE_FW_UPDATE) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_MAC_OS_UPDATE_FW, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  } else if (Entry->Type == OC_BOOT_WINDOWS) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_WINDOWS, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  } else if (Entry->Type == OC_BOOT_EXTERNAL_OS) {
    if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_NETWORK_BOOT) != NULL) {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_NETWORK_BOOT, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    } else {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_EXTERNAL_OS, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    }
  } else if (Entry->Type == OC_BOOT_SYSTEM) {
    OcPlayAudioFile (Context, Entry->AudioBasePath, Entry->AudioBaseType, FALSE);
  } else if (Entry->Type == OC_BOOT_EXTERNAL_TOOL) {
    if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_RESET_NVRAM) != NULL) {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_RESET_NVRAM, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    } else if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_TOGGLE_SIP_ENABLED) != NULL) {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_SIP_IS_ENABLED, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    } else if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_TOGGLE_SIP_DISABLED) != NULL) {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_SIP_IS_DISABLED, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    } else if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_UEFI_SHELL) != NULL) {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_UEFI_SHELL, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    } else if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_FIRMWARE_SETTINGS) != NULL) {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_FIRMWARE_SETTINGS, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    } else {
      OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_EXTERNAL_TOOL, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
    }
  } else {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_OTHER_OS, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  }

  if (Entry->IsExternal) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_EXTERNAL, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  }

  if (Entry->IsFolder) {
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_DISK_IMAGE, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OcToggleVoiceOver (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  CONST CHAR8        *BasePath     OPTIONAL,
  IN  CONST CHAR8        *BaseType     OPTIONAL
  )
{
  if (!Context->PickerAudioAssist) {
    Context->PickerAudioAssist = TRUE;
    OcPlayAudioFile (Context, OC_VOICE_OVER_AUDIO_FILE_WELCOME, OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE, FALSE);

    if ((BasePath != NULL) && (BaseType != NULL)) {
      OcPlayAudioFile (Context, BasePath, BaseType, TRUE);
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
