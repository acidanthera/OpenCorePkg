/** @file
  Play beep.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/AppleHda.h>
#include <Protocol/AppleBeepGen.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Argc;
  CHAR16                                **Argv;
  APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL  *HdaProtocol;
  APPLE_BEEP_GEN_PROTOCOL               *BeepGenProtocol;
  UINTN                                 Count;
  UINTN                                 Signal;
  UINTN                                 Silence;
  UINTN                                 Frequency;

  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  OcProvideConsoleGop (FALSE);

  OcConsoleControlSetMode (EfiConsoleControlScreenText);

  OcSetConsoleResolution (0, 0, 0);

  Status = GetArguments (&Argc, &Argv);
  if (EFI_ERROR (Status) || Argc < 5) {
    Print (L"Usage: ChipTune <any|hda|beep> <count> <signal> <silence> [<frequency>]\n");
    return EFI_SUCCESS;
  }

  Status = StrDecimalToUintnS (Argv[2], NULL, &Count);
  if (EFI_ERROR (Status)) {
    Print (L"Invalid count value - %r\n", Status);
    return EFI_SUCCESS;
  }

  Status = StrDecimalToUintnS (Argv[3], NULL, &Signal);
  if (EFI_ERROR (Status)) {
    Print (L"Invalid signal length value - %r\n", Status);
    return EFI_SUCCESS;
  }

  Status = StrDecimalToUintnS (Argv[4], NULL, &Silence);
  if (EFI_ERROR (Status)) {
    Print (L"Invalid silence length value - %r\n", Status);
    return EFI_SUCCESS;
  }

  if (Argc >= 6) {
    Status = StrDecimalToUintnS (Argv[5], NULL, &Frequency);
    if (EFI_ERROR (Status)) {
      Print (L"Invalid frequency value - %r\n", Status);
      return EFI_SUCCESS;
    }
  } else {
    Frequency = 0;
  }

  HdaProtocol     = NULL;
  BeepGenProtocol = NULL;

  if (StrCmp (Argv[1], L"any") == 0 || StrCmp (Argv[1], L"beep") == 0) {
    Status = gBS->LocateProtocol (
      &gAppleBeepGenProtocolGuid,
      NULL,
      (VOID **) &BeepGenProtocol
      );
    if (EFI_ERROR (Status) || BeepGenProtocol->GenBeep == NULL) {
      Print (L"Beep protocol is unusable - %r\n", Status);
      BeepGenProtocol = NULL;
    }
  }

  if (BeepGenProtocol == NULL && (StrCmp (Argv[1], L"any") == 0 || StrCmp (Argv[1], L"hda") == 0)) {
    Status = gBS->LocateProtocol (
      &gAppleHighDefinitionAudioProtocolGuid,
      NULL,
      (VOID **) &HdaProtocol
      );
    if (EFI_ERROR (Status) || HdaProtocol->PlayTone == NULL) {
      Print (L"HDA protocol is unusable - %r\n", Status);
      HdaProtocol = NULL;
    }
  }

  Print (
    L"Trying playback %u %Lu %Lu %d\n",
    (UINT32) Count,
    (UINT64) Signal,
    (UINT64) Silence,
    (UINT64) Frequency
    );

  if (BeepGenProtocol != NULL) {
    Status = BeepGenProtocol->GenBeep (
      (UINT32) Count,
      Signal,
      Silence
      );
  } else if (HdaProtocol != NULL) {
    Status = HdaProtocol->PlayTone (
      HdaProtocol,
      (UINT32) Count,
      Signal,
      Silence,
      Frequency
      );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    Print (L"Playback failure - %r\n", Status);
  }

  return EFI_SUCCESS;
}
