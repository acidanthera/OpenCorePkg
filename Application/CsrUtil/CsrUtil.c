/** @file
  Basic reimplementation of SIP aspects of csrutil.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/


#include <Uefi.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/AppleCsrConfig.h>
#include <Guid/AppleVariable.h>

//#define PRINT_ARGUMENTS

#define MAX_FIRST_ARG_LEN 15

STATIC
EFI_STATUS
SplitArguments (
  UINTN               *Argc,
  CHAR16              ***Argv
  )
{
  CHAR16              *Space;

  STATIC CHAR16       *NewArgs[3];
  STATIC CHAR16       FirstArg[MAX_FIRST_ARG_LEN + 1];

  if (*Argc != 2) {
    return EFI_SUCCESS;
  }

  Space = StrStr ((*Argv)[1], L" ");
  if (Space == NULL) {
    return EFI_SUCCESS;
  }

  if (Space - (*Argv)[1] > MAX_FIRST_ARG_LEN) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrnCpyS (FirstArg, ARRAY_SIZE (FirstArg), (*Argv)[1], Space - (*Argv)[1]);

  NewArgs[0] = L"Self";
  NewArgs[1] = FirstArg;
  NewArgs[2] = ++Space;

  *Argc = 3;
  *Argv = NewArgs;

  return EFI_SUCCESS;
}

#ifdef PRINT_ARGUMENTS
STATIC
VOID
PrintArguments (
  UINTN         Argc,
  CHAR16        **Argv
  )
{
  UINTN     Index;

  for (Index = 0; Index < Argc; ++Index) {
    Print (L"%u: %s\n", Index, Argv[Index]);
  }
}
#endif

STATIC
VOID
PrintUsage (
  VOID
  )
{
  Print (L"usage: csrutil <command> [<csr-value>]\n");
  Print (L"Modify the System Integrity Protection configuration.\n");
  Print (L"Available commands:\n");
  Print (L"\n");
  Print (L"    clear\n");
  Print (L"        Clear the existing configuration.\n");
  Print (L"    disable [<csr-value>]\n");
  Print (L"        Disable the protection on the machine (use default 0x%x or csr value).\n", OC_CSR_DISABLE_FLAGS);
  Print (L"    enable [<csr-value>]\n");
  Print (L"        Enable the protection on the machine (use 0 or other legal csr value).\n");
  Print (L"    toggle [<csr-value>]\n");
  Print (L"        Toggle the protection on the machine (use default 0x%x or csr value).\n", OC_CSR_DISABLE_FLAGS);
  Print (L"    status\n");
  Print (L"        Display the current configuration.\n");
  Print (L"\n");
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                            OldStatus;
  EFI_STATUS                            Status;
  UINTN                                 Data;
  CHAR16                                *EndPtr;
  UINTN                                 Argc;
  CHAR16                                **Argv;
  UINT32                                CsrConfig;
  UINT32                                Attributes;

  Status = GetArguments (&Argc, &Argv);

  if (EFI_ERROR (Status)) {
    Print (L"GetArguments - %r\n", Status);
    return Status;
  }

  Status = SplitArguments (&Argc, &Argv);

#ifdef PRINT_ARGUMENTS
  if (!EFI_ERROR (Status)) {
    PrintArguments (Argc, Argv);
  }
#endif

  if (EFI_ERROR (Status) || Argc < 2) {
    PrintUsage ();
    return EFI_SUCCESS;
  }

  if (Argc > 2) {
    Data = 0;
    if (OcUnicodeStartsWith (Argv[2], L"0x", TRUE)) {
      Status = StrHexToUintnS (Argv[2], &EndPtr, &Data);
    } else {
      Status = StrDecimalToUintnS (Argv[2], &EndPtr, &Data);
    }

    if (!EFI_ERROR (Status) && (EndPtr != &Argv[2][StrLen (Argv[2])] || (Data & MAX_UINT32) != Data)) {
      Status = EFI_UNSUPPORTED;
    }
  }

  if (EFI_ERROR (Status)) {
    PrintUsage ();
    return Status;
  }

  if (Argc == 2 && StrCmp (Argv[1], L"status") == 0) {
    //
    // Status
    //
    Print (L"System Integrity Protection status: ");
  } else {
    //
    // When changing status, use existing attributes where present
    // (e.g. keep changes made while WriteFlash=false as volatile only)
    //
    Status = OcGetSip (&CsrConfig, &Attributes);

    if (Status != EFI_NOT_FOUND && EFI_ERROR (Status)) {
      Print (L"Error getting SIP status - %r\n", Status);
      return Status;
    }

    if (Status == EFI_NOT_FOUND) {
      Attributes = CSR_APPLE_SIP_NVRAM_NV_ATTR;
    } else {
      //
      // We are finding other bits set on Apl, specifically 0x80000000,
      // so only consider relevant bits.
      //
      Attributes &= CSR_APPLE_SIP_NVRAM_NV_ATTR;
    }

    if (Argc == 2 && StrCmp (Argv[1], L"clear") == 0) {
      //
      // Clear
      //
      OldStatus = Status;

      Status = OcSetSip(NULL, Attributes);

      if (EFI_ERROR (Status) && !(OldStatus == EFI_NOT_FOUND && Status == EFI_NOT_FOUND)) {
        Print (L"Error clearing SIP - r\n", Status);
        return Status;
      }
      
      Print (L"Successfully cleared system integrity configuration: ");
    } else if (Argc <= 3 && StrCmp (Argv[1], L"disable") == 0) {
      //
      // Disable; allow anything except valid enable values
      //
      if (Argc == 2) {
        CsrConfig = OC_CSR_DISABLE_FLAGS;
      } else {
        if ((Data & ~CSR_ALLOW_APPLE_INTERNAL) == 0) {
          Print (L"Illegal value for %s\n", L"disable");
          return EFI_UNSUPPORTED;
        }
        CsrConfig = (UINT32) Data;
      }

      Status = OcSetSip(&CsrConfig, Attributes);

      if (EFI_ERROR (Status)) {
        Print (L"Error disabling SIP - r\n", Status);
        return Status;
      }
      
      Print (L"System Integrity Protection is ");
    } else if (Argc <= 3 && StrCmp (Argv[1], L"enable") == 0) {
      //
      // Enable; allow user-specified Apple internal (which reports as enabled) or zero only
      //
      if (Argc == 2) {
        CsrConfig = 0;
      } else {
        if ((Data & ~CSR_ALLOW_APPLE_INTERNAL) != 0) {
          Print (L"Illegal value for %s\n", L"enable");
          return EFI_UNSUPPORTED;
        }
        CsrConfig = (UINT32) Data;
      }

      Status = OcSetSip(&CsrConfig, Attributes);

      if (EFI_ERROR (Status)) {
        Print (L"Error enabling SIP - r\n", Status);
        return Status;
      }

      Print (L"System Integrity Protection is ");
    } else if (Argc <= 3 && StrCmp (Argv[1], L"toggle") == 0) {
      //
      // Toggle; allow anything except valid enable values
      //
      if (Argc == 2) {
        CsrConfig = OC_CSR_DISABLE_FLAGS;
      } else {
        if ((Data & ~CSR_ALLOW_APPLE_INTERNAL) == 0) {
          Print (L"Illegal value for %s\n", L"toggle");
          return EFI_UNSUPPORTED;
        }
        CsrConfig = (UINT32) Data;
      }

      Status = OcToggleSip(CsrConfig);

      if (EFI_ERROR (Status)) {
        Print (L"Error toggling SIP - r\n", Status);
        return Status;
      }

      Print (L"System Integrity Protection value was toggled to ");
    } else {
      //
      // Unsupported
      //
      PrintUsage ();
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Add result
  //
  Status = OcGetSip (&CsrConfig, &Attributes);

  if (Status != EFI_NOT_FOUND && EFI_ERROR (Status)) {
    Print (L"error getting SIP status - %r\n", Status);
    return Status;
  }

  if (Status != EFI_NOT_FOUND) {
    Attributes &= CSR_APPLE_SIP_NVRAM_NV_ATTR;
  }

  Print (L"%sd (", OcIsSipEnabled (Status, CsrConfig) ? L"enable" : L"disable");
  if (Status == EFI_NOT_FOUND) {
    Print (L"nvram var not set");
  } else {
    Print (L"0x%x", CsrConfig);
    if ((Attributes & ~EFI_VARIABLE_NON_VOLATILE) != CSR_APPLE_SIP_NVRAM_ATTR) {
      Print (L", UNKNOWN: 0x%x", Attributes);
    } else if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
      Print (L", volatile");
    }
  }
  Print (L")\n");

  if (StrCmp (Argv[0], L"Self") == 0) {
    //
    // Pause if detect called as tool
    //
    WaitForKeyPress (L"Press any key to continue...");
  }

  return EFI_SUCCESS;
}
