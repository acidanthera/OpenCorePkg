/** @file
  RTC memory read/write.

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
#include <Library/OcRtcLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

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
  UINTN                                 Addr;
  UINTN                                 Value;
  UINTN                                 Index;
  UINT8                                 Rtc[256];

  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  OcProvideConsoleGop (FALSE);

  OcConsoleControlSetMode (EfiConsoleControlScreenText);

  OcSetConsoleResolution (0, 0, 0, FALSE);

  Status = GetArguments (&Argc, &Argv);
  if (!EFI_ERROR (Status) && Argc >= 2) {
    if (OcStriCmp (Argv[1], L"dump") == 0 && Argc == 2) {
      for (Index = 0; Index < ARRAY_SIZE (Rtc); ++Index) {
        Rtc[Index] = OcRtcRead ((UINT8) Index);
      }

      for (Index = 0; Index < ARRAY_SIZE (Rtc); Index += 16) {
        Print(
          L"%02Xh: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
          Index,
          Rtc[Index+0],
          Rtc[Index+1],
          Rtc[Index+2],
          Rtc[Index+3],
          Rtc[Index+4],
          Rtc[Index+5],
          Rtc[Index+6],
          Rtc[Index+7],
          Rtc[Index+8],
          Rtc[Index+9],
          Rtc[Index+10],
          Rtc[Index+11],
          Rtc[Index+12],
          Rtc[Index+13],
          Rtc[Index+14],
          Rtc[Index+15]
          );
      }

      return EFI_SUCCESS;
    }

    if (OcStriCmp (Argv[1], L"read") == 0 && Argc == 3)  {
      Status = StrHexToUintnS (Argv[2], NULL, &Addr);
      if (EFI_ERROR (Status) || Addr > 0xFF) {
        Print (L"Invalid addr %LX - %r\n", (UINT64) Addr, Status);
        return EFI_SUCCESS;
      }

      Print (L"Rtc[0x%Xh] = 0x%02X\n", (UINT32) Addr, OcRtcRead ((UINT8) Addr));
      return EFI_SUCCESS;
    }

    if (OcStriCmp (Argv[1], L"write") == 0 && Argc == 4)  {
      Status = StrHexToUintnS (Argv[2], NULL, &Addr);
      if (EFI_ERROR (Status) || Addr > 0xFF) {
        Print (L"Invalid addr %LX - %r\n", (UINT64) Addr, Status);
        return EFI_SUCCESS;
      }

      Status = StrHexToUintnS (Argv[3], NULL, &Value);
      if (EFI_ERROR (Status) || Value > 0xFF) {
        Print (L"Invalid value %LX - %r\n", (UINT64) Value, Status);
        return EFI_SUCCESS;
      }

      Print (
        L"Rtc[0x%X] = 0x%02X -> 0x%02X\n",
        (UINT32) Addr,
        OcRtcRead ((UINT8) Addr),
        (UINT8) Value
        );
      OcRtcWrite ((UINT8) Addr, (UINT8) Value);
      return EFI_SUCCESS;
    }
  }

  Print (L"Usage: RtcRw <dump|read|write> <addr> <value>\n");
  return EFI_SUCCESS;
}
