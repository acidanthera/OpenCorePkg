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
#include <Guid/OcVariables.h>

#include <IndustryStandard/AppleCsrConfig.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/AppleKeyMapAggregator.h>
#include <Protocol/LoadedImage.h>
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
OcShowSimpleBootMenu (
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntries,
  IN  UINTN                       Count,
  IN  UINTN                       DefaultEntry,
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
  BOOLEAN                            SetDefault;

  ChosenEntry    = -1;
  Code[1]        = '\0';
  TimeOutSeconds = Context->TimeoutSeconds;

  KeyMap = OcAppleKeyMapInstallProtocols (FALSE);
  if (KeyMap == NULL) {
    DEBUG ((DEBUG_ERROR, "OCB: Missing AppleKeyMapAggregator\n"));
    return EFI_UNSUPPORTED;
  }

  OcConsoleControlSetMode (EfiConsoleControlScreenText);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  if (Context->ConsoleAttributes != 0) {
    gST->ConOut->SetAttribute (gST->ConOut, Context->ConsoleAttributes & 0x7FU);
  }

  while (TRUE) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->OutputString (gST->ConOut, L"OpenCore Boot Menu");

    if (Context->TitleSuffix != NULL) {
      Length = AsciiStrLen (Context->TitleSuffix);
      gST->ConOut->OutputString (gST->ConOut, L" (");
      for (Index = 0; Index < Length; ++Index) {
        Code[0] = Context->TitleSuffix[Index];
        gST->ConOut->OutputString (gST->ConOut, Code);
      }
      gST->ConOut->OutputString (gST->ConOut, L")");
    }

    gST->ConOut->OutputString (gST->ConOut, L"\r\n\r\n");

    for (Index = 0; Index < MIN (Count, OC_INPUT_MAX); ++Index) {
      if (DefaultEntry == Index && TimeOutSeconds > 0) {
        gST->ConOut->OutputString (gST->ConOut, L"* ");
      } else if (ChosenEntry >= 0 && (UINTN) ChosenEntry == Index) {
        gST->ConOut->OutputString (gST->ConOut, L"> ");
      } else {
        gST->ConOut->OutputString (gST->ConOut, L"  ");
      }

      Code[0] = OC_INPUT_STR[Index];
      gST->ConOut->OutputString (gST->ConOut, Code);
      gST->ConOut->OutputString (gST->ConOut, L". ");
      gST->ConOut->OutputString (gST->ConOut, BootEntries[Index].Name);
      if (BootEntries[Index].IsExternal) {
        gST->ConOut->OutputString (gST->ConOut, L" (external)");
      }
      if (BootEntries[Index].IsFolder) {
        gST->ConOut->OutputString (gST->ConOut, L" (dmg)");
      }
      gST->ConOut->OutputString (gST->ConOut, L"\r\n");
    }

    if (Index < Count) {
      gST->ConOut->OutputString (gST->ConOut, L"WARN: Some entries were skipped!\r\n");
    }

    gST->ConOut->OutputString (gST->ConOut, L"\r\nChoose boot entry: ");

    while (TRUE) {
      KeyIndex = OcWaitForAppleKeyIndex (
        Context,
        KeyMap,
        TimeOutSeconds,
        Context->PollAppleHotKeys,
        &SetDefault
        );

      if (KeyIndex == OC_INPUT_TIMEOUT) {
        *ChosenBootEntry = &BootEntries[DefaultEntry];
        gST->ConOut->OutputString (gST->ConOut, L"Timeout\r\n");
        return EFI_SUCCESS;
      } else if (KeyIndex == OC_INPUT_CONTINUE) {
        *ChosenBootEntry = &BootEntries[ChosenEntry >= 0 ? (UINTN) ChosenEntry : DefaultEntry];
        (*ChosenBootEntry)->SetDefault = SetDefault;
        gST->ConOut->OutputString (gST->ConOut, L"OK\r\n");
        return EFI_SUCCESS;
      } else if (KeyIndex == OC_INPUT_ABORTED) {
        gST->ConOut->OutputString (gST->ConOut, L"Aborted\r\n");
        return EFI_ABORTED;
      } else if (KeyIndex == OC_INPUT_UP) {
        if (TimeOutSeconds > 0) {
          ChosenEntry = (INTN) DefaultEntry;
        }
        if (ChosenEntry < 0) {
          ChosenEntry = 0;
        } else if (ChosenEntry == 0) {
          ChosenEntry = (INTN) (MIN (Count, OC_INPUT_MAX) - 1);
        } else {
          --ChosenEntry;
        }
        TimeOutSeconds = 0;
        break;
      } else if (KeyIndex == OC_INPUT_DOWN) {
        if (TimeOutSeconds > 0) {
          ChosenEntry = (INTN) DefaultEntry;
        }
        if (ChosenEntry < 0) {
          ChosenEntry = 0;
        } else if (ChosenEntry == (INTN) (MIN (Count, OC_INPUT_MAX) - 1)) {
          ChosenEntry = 0;
        } else {
          ++ChosenEntry;
        }
        TimeOutSeconds = 0;
        break;
      } else if (KeyIndex == OC_INPUT_TOP || KeyIndex == OC_INPUT_LEFT) {
        ChosenEntry = 0;
        TimeOutSeconds = 0;
        break;
      } else if (KeyIndex == OC_INPUT_BOTTOM || KeyIndex == OC_INPUT_RIGHT) {
        ChosenEntry = (INTN) (MIN (Count, OC_INPUT_MAX) - 1);
        TimeOutSeconds = 0;
        break;
      } else if (KeyIndex != OC_INPUT_INVALID && (UINTN)KeyIndex < Count) {
        ASSERT (KeyIndex >= 0);
        *ChosenBootEntry = &BootEntries[KeyIndex];
        (*ChosenBootEntry)->SetDefault = SetDefault;
        Code[0] = OC_INPUT_STR[KeyIndex];
        gST->ConOut->OutputString (gST->ConOut, Code);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        return EFI_SUCCESS;
      }

      if (TimeOutSeconds > 0) {
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
  IN VOID                *Context,
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

  if (Context == NULL) {
    return EFI_SUCCESS;
  }

  Privilege = (OC_PRIVILEGE_CONTEXT *)Context;

  if (Privilege->CurrentLevel >= Level) {
    return EFI_SUCCESS;
  }

  OcConsoleControlSetMode (EfiConsoleControlScreenText);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->ClearScreen (gST->ConOut);

  for (Index = 0; Index < 3; ++Index) {
    PwIndex = 0;
    //
    // Skip previously pressed characters.
    //
    do {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    } while (!EFI_ERROR (Status));

    gST->ConOut->OutputString (gST->ConOut, L"Password: ");

    while (TRUE) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (Status == EFI_NOT_READY) {
        continue;
      } else if (EFI_ERROR (Status)) {
        gST->ConOut->ClearScreen (gST->ConOut);
        SecureZeroMem (Password, PwIndex);
        SecureZeroMem (&Key.UnicodeChar, sizeof (Key.UnicodeChar));

        DEBUG ((DEBUG_ERROR, "Input device error\r\n"));
        return EFI_ABORTED;
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

        continue;
      } else if (Key.UnicodeChar == CHAR_NULL
       || (UINT8)Key.UnicodeChar != Key.UnicodeChar) {
        //
        // Only ASCII characters are supported.
        //
        continue;
      }

      if (PwIndex == ARRAY_SIZE (Password)) {
        continue;
      }

      gST->ConOut->OutputString (gST->ConOut, L"*");

      Password[PwIndex] = (UINT8)Key.UnicodeChar;
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
      return EFI_SUCCESS;
    }
  }

  gST->ConOut->ClearScreen (gST->ConOut);
  DEBUG ((DEBUG_WARN, "Password retry limit exceeded.\r\n"));

  gBS->Stall (5000000);
  gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  return EFI_ACCESS_DENIED;
}

EFI_STATUS
OcRunSimpleBootPicker (
  IN OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS                         Status;
  APPLE_BOOT_POLICY_PROTOCOL         *AppleBootPolicy;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  OC_BOOT_ENTRY                      *Chosen;
  OC_BOOT_ENTRY                      *Entries;
  UINTN                              EntryCount;
  INTN                               DefaultEntry;
  BOOLEAN                            ForbidApple;

  AppleBootPolicy = OcAppleBootPolicyInstallProtocol (FALSE);
  if (AppleBootPolicy == NULL) {
    DEBUG ((DEBUG_ERROR, "OCB: AppleBootPolicy locate failure\n"));
    return EFI_NOT_FOUND;
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
                        Context->PrivilegeContext,
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
    ForbidApple = TRUE;
  } else {
    ForbidApple = FALSE;
  }

  while (TRUE) {
    DEBUG ((DEBUG_INFO, "OCB: Performing OcScanForBootEntries...\n"));

    Status = OcScanForBootEntries (
      AppleBootPolicy,
      Context,
      &Entries,
      &EntryCount,
      NULL,
      TRUE
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCB: OcScanForBootEntries failure - %r\n", Status));
      return Status;
    }

    if (EntryCount == 0) {
      DEBUG ((DEBUG_WARN, "OCB: OcScanForBootEntries has no entries\n"));
      return EFI_NOT_FOUND;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCB: Performing OcShowSimpleBootMenu... %d\n",
      Context->PollAppleHotKeys
      ));

    DefaultEntry = OcGetDefaultBootEntry (Context, Entries, EntryCount);

    if (Context->PickerCommand == OcPickerShowPicker) {
      if (!ForbidApple && Context->PickerMode == OcPickerModeApple) {
        Status = OcRunAppleBootPicker ();
        DEBUG ((DEBUG_INFO, "OCB: Apple BootPicker failed on error - %r, fallback to builtin\n", Status));
        ForbidApple = TRUE;
      }

      Status = OcShowSimpleBootMenu (
        Context,
        Entries,
        EntryCount,
        DefaultEntry,
        &Chosen
        );
    } else if (Context->PickerCommand == OcPickerResetNvram) {
      return InternalSystemActionResetNvram ();
    } else {
      Chosen = &Entries[DefaultEntry];
      Status = EFI_SUCCESS;
    }

    if (EFI_ERROR (Status) && Status != EFI_ABORTED) {
      DEBUG ((DEBUG_ERROR, "OCB: OcShowSimpleBootMenu failed - %r\n", Status));
      OcFreeBootEntries (Entries, EntryCount);
      return Status;
    }

    Context->TimeoutSeconds = 0;

    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Should boot from %s (T:%d|F:%d|DEF:%d)\n",
        Chosen->Name,
        Chosen->Type,
        Chosen->IsFolder,
        Chosen->SetDefault
        ));

      if (Chosen->SetDefault) {
        Status = OcSetDefaultBootEntry (Context, Chosen);
        DEBUG ((DEBUG_INFO, "OCB: Setting default - %r\n", Status));
      }

      //
      // Clear screen before loading the entry.
      //
      if (Context->PickerCommand == OcPickerShowPicker) {
        gST->ConOut->ClearScreen (gST->ConOut);
      }

      Status = OcLoadBootEntry (
        AppleBootPolicy,
        Context,
        Chosen,
        gImageHandle
        );

      //
      // Do not wait on successful return code.
      //
      if (EFI_ERROR (Status)) {
        gBS->Stall (SECONDS_TO_MICROSECONDS (3));
        //
        // Show picker on first failure.
        //
        Context->PickerCommand = OcPickerShowPicker;
      }

      //
      // Ensure that we flush all pressed keys after the application.
      // This resolves the problem of application-pressed keys being used to control the menu.
      //
      OcKeyMapFlush (KeyMap, 0, TRUE);
    }

    OcFreeBootEntries (Entries, EntryCount);
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
    DEBUG ((DEBUG_INFO, "OCB: OcRunAppleBootPicker attempting to start...\n"));
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
