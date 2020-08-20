/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

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

STATIC
EFI_STATUS
RunShowMenu (
  IN  OC_BOOT_CONTEXT             *BootContext,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  )
{
  EFI_STATUS      Status;
  OC_BOOT_ENTRY   **BootEntries;
  UINT32          EntryReason;

  if (!BootContext->PickerContext->ApplePickerUnsupported
    && BootContext->PickerContext->PickerMode == OcPickerModeApple) {
    Status = OcRunAppleBootPicker ();
    //
    // This should not return on success.
    //
    DEBUG ((DEBUG_INFO, "OCB: Apple BootPicker failed on error - %r, fallback to builtin\n", Status));
    BootContext->PickerContext->ApplePickerUnsupported = TRUE;
  }

  BootEntries = OcEnumerateEntries (BootContext);
  if (BootEntries == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // We are not allowed to have no default entry.
  // However, if default entry is a tool or a system entry, never autoboot it.
  //
  if (BootContext->DefaultEntry == NULL) {
    BootContext->DefaultEntry = BootEntries[0];
    BootContext->PickerContext->TimeoutSeconds = 0;
  }

  //
  // Ensure that picker entry reason is set as it can be read by boot.efi.
  //
  EntryReason = ApplePickerEntryReasonUnknown;
  gRT->SetVariable (
    APPLE_PICKER_ENTRY_REASON_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (EntryReason),
    &EntryReason
    );

  Status = BootContext->PickerContext->ShowMenu (
    BootContext,
    BootEntries,
    ChosenBootEntry
    );
  FreePool (BootEntries);

  return Status;
}

EFI_STATUS
EFIAPI
OcShowSimpleBootMenu (
  IN  OC_BOOT_CONTEXT             *BootContext,
  IN  OC_BOOT_ENTRY               **BootEntries,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  )
{
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  UINTN                              Index;
  UINTN                              Length;
  INTN                               KeyIndex;
  INTN                               ChosenEntry;
  CHAR16                             Code[2];
  UINT32                             TimeOutSeconds;
  UINT32                             Count;
  BOOLEAN                            SetDefault;
  BOOLEAN                            PlayedOnce;
  BOOLEAN                            PlayChosen;

  ChosenEntry    = -1;
  Code[1]        = '\0';

  TimeOutSeconds = BootContext->PickerContext->TimeoutSeconds;
  ASSERT (BootContext->DefaultEntry != NULL);

  PlayedOnce     = FALSE;
  PlayChosen     = FALSE;

  KeyMap = OcAppleKeyMapInstallProtocols (FALSE);
  if (KeyMap == NULL) {
    DEBUG ((DEBUG_ERROR, "OCB: Missing AppleKeyMapAggregator\n"));
    return EFI_UNSUPPORTED;
  }

  Count = (UINT32) BootContext->BootEntryCount;

  if (Count != MIN (Count, OC_INPUT_MAX)) {
    DEBUG ((DEBUG_WARN, "OCB: Cannot display all entries in the menu!\n"));
  }

  OcConsoleControlSetMode (EfiConsoleControlScreenText);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  if (BootContext->PickerContext->ConsoleAttributes != 0) {
    gST->ConOut->SetAttribute (gST->ConOut, BootContext->PickerContext->ConsoleAttributes & 0x7FU);
  }

  //
  // Extension for OpenCore direct text render for faster redraw with custom background.
  //
  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->TestString (gST->ConOut, OC_CONSOLE_MARK_CONTROLLED);

  while (TRUE) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->OutputString (gST->ConOut, OC_MENU_BOOT_MENU);

    if (BootContext->PickerContext->TitleSuffix != NULL) {
      Length = AsciiStrLen (BootContext->PickerContext->TitleSuffix);
      gST->ConOut->OutputString (gST->ConOut, L" (");
      for (Index = 0; Index < Length; ++Index) {
        Code[0] = BootContext->PickerContext->TitleSuffix[Index];
        gST->ConOut->OutputString (gST->ConOut, Code);
      }
      gST->ConOut->OutputString (gST->ConOut, L")");
    }

    gST->ConOut->OutputString (gST->ConOut, L"\r\n\r\n");

    for (Index = 0; Index < MIN (Count, OC_INPUT_MAX); ++Index) {
      if (TimeOutSeconds > 0 && BootContext->DefaultEntry->EntryIndex - 1 == Index) {
        gST->ConOut->OutputString (gST->ConOut, L"* ");
      } else if (ChosenEntry >= 0 && (UINTN) ChosenEntry == Index) {
        gST->ConOut->OutputString (gST->ConOut, L"> ");
      } else {
        gST->ConOut->OutputString (gST->ConOut, L"  ");
      }

      Code[0] = OC_INPUT_STR[Index];
      gST->ConOut->OutputString (gST->ConOut, Code);
      gST->ConOut->OutputString (gST->ConOut, L". ");
      gST->ConOut->OutputString (gST->ConOut, BootEntries[Index]->Name);
      if (BootEntries[Index]->IsExternal) {
        gST->ConOut->OutputString (gST->ConOut, OC_MENU_EXTERNAL);
      }
      gST->ConOut->OutputString (gST->ConOut, L"\r\n");
    }

    gST->ConOut->OutputString (gST->ConOut, L"\r\n");
    gST->ConOut->OutputString (gST->ConOut, OC_MENU_CHOOSE_OS);

    if (!PlayedOnce && BootContext->PickerContext->PickerAudioAssist) {
      OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileChooseOS, FALSE);
      for (Index = 0; Index < Count; ++Index) {
        OcPlayAudioEntry (BootContext->PickerContext, BootEntries[Index]);
        if (TimeOutSeconds > 0 && BootContext->DefaultEntry->EntryIndex - 1 == Index) {
          OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileDefault, FALSE);
        }
      }
      OcPlayAudioBeep (
        BootContext->PickerContext,
        OC_VOICE_OVER_SIGNALS_NORMAL,
        OC_VOICE_OVER_SIGNAL_NORMAL_MS,
        OC_VOICE_OVER_SILENCE_NORMAL_MS
        );
      PlayedOnce = TRUE;
    }

    while (TRUE) {
      //
      // Pronounce entry name only after N ms of idleness.
      //
      KeyIndex = OcWaitForAppleKeyIndex (
        BootContext->PickerContext,
        KeyMap,
        PlayChosen ? OC_VOICE_OVER_IDLE_TIMEOUT_MS : TimeOutSeconds * 1000,
        &SetDefault
        );

      if (PlayChosen && KeyIndex == OC_INPUT_TIMEOUT) {
        OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileSelected, FALSE);
        OcPlayAudioEntry (BootContext->PickerContext, BootEntries[ChosenEntry]);
        PlayChosen = FALSE;
        continue;
      } else if (KeyIndex == OC_INPUT_TIMEOUT) {
        *ChosenBootEntry = BootEntries[BootContext->DefaultEntry->EntryIndex - 1];
        gST->ConOut->OutputString (gST->ConOut, OC_MENU_TIMEOUT);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileTimeout, FALSE);
        return EFI_SUCCESS;
      } else if (KeyIndex == OC_INPUT_CONTINUE) {
        if (ChosenEntry >= 0) {
          *ChosenBootEntry = BootEntries[(UINTN) ChosenEntry];
        } else {
          *ChosenBootEntry = BootContext->DefaultEntry;
        }
        (*ChosenBootEntry)->SetDefault = SetDefault;
        gST->ConOut->OutputString (gST->ConOut, OC_MENU_OK);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        return EFI_SUCCESS;
      } else if (KeyIndex == OC_INPUT_ABORTED) {
        gST->ConOut->OutputString (gST->ConOut, OC_MENU_RELOADING);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileReloading, FALSE);
        return EFI_ABORTED;
      } else if (KeyIndex == OC_INPUT_MORE && BootContext->PickerContext->HideAuxiliary) {
        gST->ConOut->OutputString (gST->ConOut, OC_MENU_SHOW_AUXILIARY);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileShowAuxiliary, FALSE);
        BootContext->PickerContext->HideAuxiliary = FALSE;
        return EFI_ABORTED;
      } else if (KeyIndex == OC_INPUT_UP) {
        if (TimeOutSeconds > 0) {
          ChosenEntry = (INTN) (BootContext->DefaultEntry->EntryIndex - 1);
        }
        if (ChosenEntry < 0) {
          ChosenEntry = 0;
        } else if (ChosenEntry == 0) {
          ChosenEntry = (INTN) (MIN (Count, OC_INPUT_MAX) - 1);
        } else {
          --ChosenEntry;
        }
        TimeOutSeconds = 0;
        if (BootContext->PickerContext->PickerAudioAssist) {
          PlayChosen = TRUE;
        }
        break;
      } else if (KeyIndex == OC_INPUT_DOWN) {
        if (TimeOutSeconds > 0) {
          ChosenEntry = (INTN) (BootContext->DefaultEntry->EntryIndex - 1);
        }
        if (ChosenEntry < 0) {
          ChosenEntry = 0;
        } else if (ChosenEntry == (INTN) (MIN (Count, OC_INPUT_MAX) - 1)) {
          ChosenEntry = 0;
        } else {
          ++ChosenEntry;
        }
        TimeOutSeconds = 0;
        if (BootContext->PickerContext->PickerAudioAssist) {
          PlayChosen = TRUE;
        }
        break;
      } else if (KeyIndex == OC_INPUT_TOP || KeyIndex == OC_INPUT_LEFT) {
        ChosenEntry = 0;
        TimeOutSeconds = 0;
        if (BootContext->PickerContext->PickerAudioAssist) {
          PlayChosen = TRUE;
        }
        break;
      } else if (KeyIndex == OC_INPUT_BOTTOM || KeyIndex == OC_INPUT_RIGHT) {
        ChosenEntry = (INTN) (MIN (Count, OC_INPUT_MAX) - 1);
        TimeOutSeconds = 0;
        if (BootContext->PickerContext->PickerAudioAssist) {
          PlayChosen = TRUE;
        }
        break;
      } else if (KeyIndex == OC_INPUT_VOICE_OVER) {
        OcToggleVoiceOver (BootContext->PickerContext, 0);
        break;
      } else if (KeyIndex != OC_INPUT_INVALID && KeyIndex >= 0 && (UINTN)KeyIndex < Count) {
        *ChosenBootEntry = BootEntries[KeyIndex];
        (*ChosenBootEntry)->SetDefault = SetDefault;
        Code[0] = OC_INPUT_STR[KeyIndex];
        gST->ConOut->OutputString (gST->ConOut, Code);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        return EFI_SUCCESS;
      }

      if (TimeOutSeconds > 0) {
        OcPlayAudioFile (BootContext->PickerContext, OcVoiceOverAudioFileAbortTimeout, FALSE);
        TimeOutSeconds = 0;
        break;
      }
    }
  }

  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
OcShowSimplePasswordRequest (
  IN OC_PICKER_CONTEXT   *Context,
  IN OC_PRIVILEGE_LEVEL  Level
  )
{
  OC_PRIVILEGE_CONTEXT *Privilege;

  BOOLEAN              Result;

  UINT8                Password[32];
  UINT32               PwIndex;

  UINT8                Index;
  EFI_STATUS           Status;
  EFI_INPUT_KEY        Key;

  Privilege = Context->PrivilegeContext;

  if (Privilege == NULL || Privilege->CurrentLevel >= Level) {
    return EFI_SUCCESS;
  }

  OcConsoleControlSetMode (EfiConsoleControlScreenText);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->TestString (gST->ConOut, OC_CONSOLE_MARK_CONTROLLED);

  for (Index = 0; Index < 3; ++Index) {
    PwIndex = 0;
    //
    // Skip previously pressed characters.
    //
    do {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    } while (!EFI_ERROR (Status));

    gST->ConOut->OutputString (gST->ConOut, OC_MENU_PASSWORD_REQUEST);
    OcPlayAudioFile (Context, OcVoiceOverAudioFileEnterPassword, TRUE);

    while (TRUE) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (Status == EFI_NOT_READY) {
        continue;
      } else if (EFI_ERROR (Status)) {
        gST->ConOut->ClearScreen (gST->ConOut);
        SecureZeroMem (Password, PwIndex);
        SecureZeroMem (&Key.UnicodeChar, sizeof (Key.UnicodeChar));

        DEBUG ((DEBUG_ERROR, "Input device error\r\n"));
        OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_HWERROR,
          OC_VOICE_OVER_SIGNAL_ERROR_MS,
          OC_VOICE_OVER_SILENCE_ERROR_MS
          );
        return EFI_ABORTED;
      }

      //
      // TODO: We should really switch to Apple input here.
      //
      if (Key.ScanCode == SCAN_F5) {
        OcToggleVoiceOver (Context, OcVoiceOverAudioFileEnterPassword);
      }

      if (Key.ScanCode == SCAN_ESC) {
        gST->ConOut->ClearScreen (gST->ConOut);
        SecureZeroMem (Password, PwIndex);
        //
        // ESC aborts the input.
        //
        return EFI_ABORTED;
      } else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        gST->ConOut->ClearScreen (gST->ConOut);
        //
        // RETURN finalizes the input.
        //
        break;
      } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
        //
        // Delete the last entered character, if such exists.
        //
        if (PwIndex != 0) {
          --PwIndex;
          Password[PwIndex] = 0;
          //
          // Overwrite current character with a space.
          //
          gST->ConOut->SetCursorPosition (
                         gST->ConOut,
                         gST->ConOut->Mode->CursorColumn - 1,
                         gST->ConOut->Mode->CursorRow
                         );
          gST->ConOut->OutputString (gST->ConOut, L" ");
          gST->ConOut->SetCursorPosition (
                         gST->ConOut,
                         gST->ConOut->Mode->CursorColumn - 1,
                         gST->ConOut->Mode->CursorRow
                         );
        }

        OcPlayAudioFile (Context, AppleVoiceOverAudioFileBeep, TRUE);
        continue;
      } else if (Key.UnicodeChar == CHAR_NULL
       || (UINT8)Key.UnicodeChar != Key.UnicodeChar) {
        //
        // Only ASCII characters are supported.
        //
        OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_ERROR,
          OC_VOICE_OVER_SIGNAL_ERROR_MS,
          OC_VOICE_OVER_SILENCE_ERROR_MS
          );
        continue;
      }

      if (PwIndex == ARRAY_SIZE (Password)) {
        OcPlayAudioBeep (
          Context,
          OC_VOICE_OVER_SIGNALS_ERROR,
          OC_VOICE_OVER_SIGNAL_ERROR_MS,
          OC_VOICE_OVER_SILENCE_ERROR_MS
          );
        continue;
      }

      gST->ConOut->OutputString (gST->ConOut, L"*");
      Password[PwIndex] = (UINT8)Key.UnicodeChar;
      OcPlayAudioFile (Context, AppleVoiceOverAudioFileBeep, TRUE);
      ++PwIndex;
    }

    Result = OcVerifyPasswordSha512 (
               Password,
               PwIndex,
               Privilege->Salt,
               Privilege->SaltSize,
               Privilege->Hash
               );

    SecureZeroMem (Password, PwIndex);

    if (Result) {
      gST->ConOut->ClearScreen (gST->ConOut);
      Privilege->CurrentLevel = Level;
      OcPlayAudioFile (Context, OcVoiceOverAudioFilePasswordAccepted, TRUE);
      return EFI_SUCCESS;
    } else {
      OcPlayAudioFile (Context, OcVoiceOverAudioFilePasswordIncorrect, TRUE);
    }
  }

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->OutputString (gST->ConOut, OC_MENU_PASSWORD_RETRY_LIMIT);
  gST->ConOut->OutputString (gST->ConOut, L"\r\n");
  OcPlayAudioFile (Context, OcVoiceOverAudioFilePasswordRetryLimit, TRUE);
  DEBUG ((DEBUG_WARN, "OCB: User failed to verify password for 3 times running\n"));

  gBS->Stall (5000000);
  gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  return EFI_ACCESS_DENIED;
}

EFI_STATUS
OcRunBootPicker (
  IN OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS                         Status;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  OC_BOOT_CONTEXT                    *BootContext;
  OC_BOOT_ENTRY                      *Chosen;
  BOOLEAN                            SaidWelcome;

  SaidWelcome = FALSE;

  Status = InternalInitImageLoader ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Reset NVRAM right away if requested by a key combination.
  // This function should not return under normal conditions.
  //
  if (Context->PickerCommand == OcPickerResetNvram) {
    return InternalSystemActionResetNvram ();
  }

  KeyMap = OcAppleKeyMapInstallProtocols (FALSE);
  if (KeyMap == NULL) {
    DEBUG ((DEBUG_ERROR, "OCB: AppleKeyMap locate failure\n"));
    return EFI_NOT_FOUND;
  }

  //
  // This one is handled as is for Apple BootPicker for now.
  //
  if (Context->PickerCommand != OcPickerDefault) {
    Status = Context->RequestPrivilege (
      Context,
      OcPrivilegeAuthorized
      );
    if (EFI_ERROR (Status)) {
      if (Status != EFI_ABORTED) {
        ASSERT (FALSE);
        return Status;
      }

      Context->PickerCommand = OcPickerDefault;
    }
  }

  if (Context->PickerCommand == OcPickerShowPicker && Context->PickerMode == OcPickerModeApple) {
    Status = OcRunAppleBootPicker ();
    DEBUG ((DEBUG_INFO, "OCB: Apple BootPicker failed - %r, fallback to builtin\n", Status));
    Context->ApplePickerUnsupported = TRUE;
  }

  if (Context->PickerCommand != OcPickerShowPicker && Context->PickerCommand != OcPickerDefault) {
    //
    // We cannot ignore auxiliary entries for all other modes.
    //
    Context->HideAuxiliary = FALSE;
  }

  while (TRUE) {
    //
    // Turbo-boost scanning when bypassing picker.
    //
    if (Context->PickerCommand == OcPickerDefault) {
      BootContext = OcScanForDefaultBootEntry (Context);
    } else {
      ASSERT (
        Context->PickerCommand == OcPickerShowPicker
        || Context->PickerCommand == OcPickerBootApple
        || Context->PickerCommand == OcPickerBootAppleRecovery
        );

      BootContext = OcScanForBootEntries (Context);
    }

    //
    // We have no entries at all or have auxiliary entries.
    // Fallback to showing menu in the latter case.
    //
    if (BootContext == NULL) {
      if (Context->HideAuxiliary) {
        DEBUG ((DEBUG_WARN, "OCB: System has no boot entries, retrying with auxiliary\n"));
        Context->PickerCommand = OcPickerShowPicker;
        Context->HideAuxiliary = FALSE;
        continue;
      }

      DEBUG ((DEBUG_WARN, "OCB: System has no boot entries\n"));
      return EFI_NOT_FOUND;
    }

    if (Context->PickerCommand == OcPickerShowPicker) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Showing menu... %a\n",
        Context->PollAppleHotKeys ? "(polling hotkeys)" : ""
        ));

      if (!SaidWelcome) {
        OcPlayAudioFile (Context, OcVoiceOverAudioFileWelcome, FALSE);
        SaidWelcome = TRUE;
      }

      Status = RunShowMenu (BootContext, &Chosen);

      if (EFI_ERROR (Status) && Status != EFI_ABORTED) {
        DEBUG ((DEBUG_ERROR, "OCB: ShowMenu failed - %r\n", Status));
        OcFreeBootContext (BootContext);
        return Status;
      }
    } else if (BootContext->DefaultEntry != NULL) {
      Chosen = BootContext->DefaultEntry;
      Status = EFI_SUCCESS;
    } else {
      //
      // This can only be failed macOS or macOS recovery boot.
      // We may actually not rescan here.
      //
      ASSERT (Context->PickerCommand == OcPickerBootApple || Context->PickerCommand == OcPickerBootAppleRecovery);
      DEBUG ((DEBUG_INFO, "OCB: System has no default boot entry, showing menu\n"));
      Context->PickerCommand = OcPickerShowPicker;
      OcFreeBootContext (BootContext);
      continue;
    }

    ASSERT (!EFI_ERROR (Status) || Status == EFI_ABORTED);

    Context->TimeoutSeconds = 0;

    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Should boot from %u. %s (T:%d|F:%d|G:%d|E:%d|DEF:%d)\n",
        Chosen->EntryIndex,
        Chosen->Name,
        Chosen->Type,
        Chosen->IsFolder,
        Chosen->IsGeneric,
        Chosen->IsExternal,
        Chosen->SetDefault
        ));

      if (Context->PickerCommand == OcPickerShowPicker) {
        ASSERT (Chosen->EntryIndex > 0);

        if (Chosen->SetDefault) {
          if (Context->PickerCommand == OcPickerShowPicker) {
            OcPlayAudioFile (Context, OcVoiceOverAudioFileSelected, FALSE);
            OcPlayAudioFile (Context, OcVoiceOverAudioFileDefault, FALSE);
            OcPlayAudioEntry (Context, Chosen);
          }
          Status = OcSetDefaultBootEntry (Context, Chosen);
          DEBUG ((DEBUG_INFO, "OCB: Setting default - %r\n", Status));
        }

        //
        // Do screen clearing for builtin menu here, so that it is possible to see the action.
        // TODO: Probably remove that completely.
        //
        if (Context->ShowMenu == OcShowSimpleBootMenu) {
          //
          // Clear screen from picker contents before loading the entry.
          //
          gST->ConOut->ClearScreen (gST->ConOut);
          gST->ConOut->TestString (gST->ConOut, OC_CONSOLE_MARK_UNCONTROLLED);
        }

        //
        // Voice chosen information.
        //
        OcPlayAudioFile (Context, OcVoiceOverAudioFileLoading, FALSE);
        Status = OcPlayAudioEntry (Context, Chosen);
        if (EFI_ERROR (Status)) {
          OcPlayAudioBeep (
            Context,
            OC_VOICE_OVER_SIGNALS_PASSWORD_OK,
            OC_VOICE_OVER_SIGNAL_NORMAL_MS,
            OC_VOICE_OVER_SILENCE_NORMAL_MS
            );
        }
      }

      Status = OcLoadBootEntry (
        Context,
        Chosen,
        gImageHandle
        );

      //
      // Do not wait on successful return code.
      //
      if (EFI_ERROR (Status)) {
        OcPlayAudioFile (Context, OcVoiceOverAudioFileExecutionFailure, TRUE);
        gBS->Stall (SECONDS_TO_MICROSECONDS (3));
        //
        // Show picker on first failure.
        //
        Context->PickerCommand = OcPickerShowPicker;
      } else {
        OcPlayAudioFile (Context, OcVoiceOverAudioFileExecutionSuccessful, FALSE);
      }

      //
      // Ensure that we flush all pressed keys after the application.
      // This resolves the problem of application-pressed keys being used to control the menu.
      //
      OcKeyMapFlush (KeyMap, 0, TRUE);
    }

    OcFreeBootContext (BootContext);
  }
}

EFI_STATUS
OcRunAppleBootPicker (
  VOID
  )
{
  EFI_STATUS                           Status;
  EFI_HANDLE                           NewHandle;
  EFI_DEVICE_PATH_PROTOCOL             *Dp;
  APPLE_PICKER_ENTRY_REASON            PickerEntryReason;

  DEBUG ((DEBUG_INFO, "OCB: OcRunAppleBootPicker attempting to find...\n"));

  Dp = CreateFvFileDevicePath (&gAppleBootPickerFileGuid);
  if (Dp != NULL) {
    DEBUG ((DEBUG_INFO, "OCB: OcRunAppleBootPicker attempting to load...\n"));
    NewHandle = NULL;
    Status = gBS->LoadImage (
      FALSE,
      gImageHandle,
      Dp,
      NULL,
      0,
      &NewHandle
      );
    if (EFI_ERROR (Status)) {
      Status = EFI_INVALID_PARAMETER;
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (!EFI_ERROR (Status)) {
    PickerEntryReason = ApplePickerEntryReasonUnknown;
    Status = gRT->SetVariable (
      APPLE_PICKER_ENTRY_REASON_VARIABLE_NAME,
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof (PickerEntryReason),
      &PickerEntryReason
      );

    DEBUG ((DEBUG_INFO, "OCB: OcRunAppleBootPicker attempting to start with var %r...\n", Status));
    Status = gBS->StartImage (
      NewHandle,
      NULL,
      NULL
      );

    if (EFI_ERROR (Status)) {
      Status = EFI_UNSUPPORTED;
    }
  }

  return Status;
}
