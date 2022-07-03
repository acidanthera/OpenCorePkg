/** @file
  Copyright (C) 2016-2022, vit9696, mikebeaton. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/AppleCsrConfig.h>
#include <Guid/AppleVariable.h>
#include <Guid/GlobalVariable.h>
#include <Guid/MicrosoftVariable.h>
#include <Guid/OcVariable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDirectResetLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVariableLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/OcFirmwareRuntime.h>

STATIC
EFI_GUID
  mOzmosisProprietary1Guid = {
  0x1F8E0C02, 0x58A9, 0x4E34, { 0xAE, 0x22, 0x2B, 0x63, 0x74, 0x5F, 0xA1, 0x01 }
};

STATIC
EFI_GUID
  mOzmosisProprietary2Guid = {
  0x9480E8A1, 0x1793, 0x46C9, { 0x91, 0xD8, 0x11, 0x08, 0xDB, 0xA4, 0x73, 0x1C }
};

STATIC
EFI_GUID
  mBootChimeVendorVariableGuid = {
  0x89D4F995, 0x67E3, 0x4895, { 0x8F, 0x18, 0x45, 0x4B, 0x65, 0x1D, 0x92, 0x15 }
};

EFI_LOAD_OPTION *
OcGetBootOptionData (
  OUT UINTN           *OptionSize,
  IN  UINT16          BootOption,
  IN  CONST EFI_GUID  *BootGuid
  )
{
  EFI_STATUS       Status;
  CHAR16           BootVarName[L_STR_LEN (L"Boot####") + 1];
  UINTN            LoadOptionSize;
  EFI_LOAD_OPTION  *LoadOption;

  if (CompareGuid (BootGuid, &gOcVendorVariableGuid)) {
    UnicodeSPrint (
      BootVarName,
      sizeof (BootVarName),
      OC_VENDOR_BOOT_VARIABLE_PREFIX L"%04x",
      BootOption
      );
  } else {
    UnicodeSPrint (BootVarName, sizeof (BootVarName), L"Boot%04x", BootOption);
  }

  Status = GetVariable2 (
             BootVarName,
             BootGuid,
             (VOID **)&LoadOption,
             &LoadOptionSize
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (LoadOptionSize < sizeof (*LoadOption)) {
    FreePool (LoadOption);
    return NULL;
  }

  *OptionSize = LoadOptionSize;
  return LoadOption;
}

STATIC
BOOLEAN
IsDeletableVariable (
  IN CHAR16    *Name,
  IN EFI_GUID  *Guid,
  IN BOOLEAN   PreserveBoot
  )
{
  //
  // Apple GUIDs
  //
  if (  CompareGuid (Guid, &gAppleVendorVariableGuid)
     || CompareGuid (Guid, &gAppleBootVariableGuid)
     || CompareGuid (Guid, &gAppleCoreStorageVariableGuid)
     || CompareGuid (Guid, &gAppleTamperResistantBootVariableGuid)
     || CompareGuid (Guid, &gAppleWirelessNetworkVariableGuid)
     || CompareGuid (Guid, &gApplePersonalizationVariableGuid)
     || CompareGuid (Guid, &gAppleNetbootVariableGuid)
     || CompareGuid (Guid, &gAppleSecureBootVariableGuid)
     || CompareGuid (Guid, &gAppleTamperResistantBootSecureVariableGuid)
     || CompareGuid (Guid, &gAppleTamperResistantBootEfiUserVariableGuid))
  {
    return TRUE;
    //
    // Global variable boot options
    //
  } else if (CompareGuid (Guid, &gEfiGlobalVariableGuid)) {
    //
    // Only erase boot and driver entries for BDS
    // I.e. BootOrder, Boot####, DriverOrder, Driver####
    // Preserve Boot#### entries except BootNext when PreserveBoot is TRUE.
    //
    if (  (StrCmp (Name, L"BootNext") == 0)
       || (  !PreserveBoot
          && (StrnCmp (Name, L"Boot", L_STR_LEN (L"Boot")) == 0)
          && (StrCmp (Name, L"BootOptionSupport") != 0))
       || (StrnCmp (Name, L"Driver", L_STR_LEN (L"Driver")) == 0))
    {
      return TRUE;
    }

    //
    // Lilu & OpenCore extensions if present
    //
  } else if (CompareGuid (Guid, &gOcVendorVariableGuid)) {
    //
    // Do not remove OpenCore critical variables.
    //
    if (  (StrCmp (Name, OC_BOOT_REDIRECT_VARIABLE_NAME) != 0)
       && (StrCmp (Name, OC_SCAN_POLICY_VARIABLE_NAME) != 0))
    {
      return TRUE;
    }
  } else if (  CompareGuid (Guid, &gOcReadOnlyVariableGuid)
            || CompareGuid (Guid, &gOcWriteOnlyVariableGuid))
  {
    return TRUE;
    //
    // Ozmozis extensions if present
    //
  } else if (  CompareGuid (Guid, &mOzmosisProprietary1Guid)
            || CompareGuid (Guid, &mOzmosisProprietary2Guid))
  {
    return TRUE;
    //
    // Boot Chime preferences if present
    //
  } else if (CompareGuid (Guid, &mBootChimeVendorVariableGuid)) {
    return TRUE;
    //
    // Microsoft certificates if present
    //
  } else if (CompareGuid (Guid, &gMicrosoftVariableGuid)) {
    //
    // Do not remove current OEM policy.
    //
    if (StrCmp (Name, L"CurrentPolicy") != 0) {
      return TRUE;
    }
  }

  return FALSE;
}

VOID
OcScanVariables (
  IN OC_PROCESS_VARIABLE  ProcessVariable,
  IN VOID                 *Context
  )
{
  EFI_GUID                    CurrentGuid;
  EFI_STATUS                  Status;
  CHAR16                      *Buffer;
  CHAR16                      *TmpBuffer;
  UINTN                       BufferSize;
  UINTN                       RequestedSize;
  BOOLEAN                     Restart;
  BOOLEAN                     CriticalFailure;
  OC_PROCESS_VARIABLE_RESULT  ProcessResult;

  //
  // Request 1024 byte buffer.
  //
  Buffer        = NULL;
  BufferSize    = 0;
  RequestedSize = 1024;

  //
  // Force starting from 0th GUID.
  //
  Restart = TRUE;

  //
  // Assume we have not failed yet.
  // TODO: The reasoning behind the logic of CriticalFailure seems not entirely
  // clear here, if anyone can add a note reminding? (Should be safe, but why 2
  // failures, not 2 per variable? Or n? Also, was it ever meant to restart the
  // scan entirely? These possible behaviours could be configured as params to
  // OcScanVariables.)
  //
  CriticalFailure = FALSE;

  do {
    //
    // Allocate new buffer if needed.
    //
    if (RequestedSize > BufferSize) {
      TmpBuffer = AllocateZeroPool (RequestedSize);
      if (TmpBuffer != NULL) {
        if (Buffer != NULL) {
          CopyMem (TmpBuffer, Buffer, BufferSize);
          FreePool (Buffer);
        }

        Buffer     = TmpBuffer;
        BufferSize = RequestedSize;
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCVAR: Failed to allocate variable name buffer of %u bytes\n",
          (UINT32)RequestedSize
          ));
        break;
      }
    }

    //
    // To start the search variable name should be L"".
    //
    if (Restart) {
      ZeroMem (&CurrentGuid, sizeof (CurrentGuid));
      ZeroMem (Buffer, BufferSize);
      Restart = FALSE;
    }

    //
    // Always pass maximum variable name size to reduce reallocations.
    //
    RequestedSize = BufferSize;
    Status        = gRT->GetNextVariableName (&RequestedSize, Buffer, &CurrentGuid);

    if (!EFI_ERROR (Status)) {
      ProcessResult = ProcessVariable (&CurrentGuid, Buffer, Context);
      if (ProcessResult == OcProcessVariableAbort) {
        break;
      }

      if (ProcessResult == OcProcessVariableRestart) {
        Restart = TRUE;
        continue;
      }

      ASSERT (ProcessResult == OcProcessVariableContinue);
    } else if ((Status != EFI_BUFFER_TOO_SMALL) && (Status != EFI_NOT_FOUND)) {
      if (!CriticalFailure) {
        DEBUG ((DEBUG_INFO, "OCVAR: Unexpected error (%r), trying to rescan\n", Status));
        CriticalFailure = TRUE;
      } else {
        DEBUG ((DEBUG_INFO, "OCVAR: Unexpected error (%r), aborting\n", Status));
        break;
      }
    }
  } while (Status != EFI_NOT_FOUND);

  if (Buffer != NULL) {
    FreePool (Buffer);
  }
}

STATIC
OC_PROCESS_VARIABLE_RESULT
EFIAPI
DeleteVariable (
  IN EFI_GUID  *Guid,
  IN CHAR16    *Name,
  IN VOID      *Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Restart;
  BOOLEAN     *PreserveBoot;

  ASSERT (Guid != NULL);
  ASSERT (Name != NULL);
  ASSERT (Context != NULL);

  PreserveBoot = Context;
  Restart      = FALSE;

  if (IsDeletableVariable (Name, Guid, *PreserveBoot)) {
    Status = gRT->SetVariable (Name, Guid, 0, 0, NULL);
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "Deleting %g:%s... OK\n",
        Guid,
        Name
        ));
      //
      // Calls to SetVariable() between calls to GetNextVariableName()
      // may produce unpredictable results, so we restart.
      //
      Restart = TRUE;
    } else if ((Status == EFI_NOT_FOUND) || (Status == EFI_SECURITY_VIOLATION)) {
      DEBUG ((
        DEBUG_INFO,
        "Deleting %g:%s... SKIP - %r\n",
        Guid,
        Name,
        Status
        ));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "Deleting %g:%s... FAIL - %r\n",
        Guid,
        Name,
        Status
        ));
    }
  } else {
    // Print (L"Skipping %g:%s\n", Guid, Name);
  }

  return Restart ? OcProcessVariableRestart : OcProcessVariableContinue;
}

STATIC
VOID
DeleteVariables (
  IN BOOLEAN  PreserveBoot
  )
{
  OcScanVariables (DeleteVariable, &PreserveBoot);
}

STATIC
VOID *
GetBootstrapBootData (
  OUT UINTN   *OptionSize,
  OUT UINT16  *Option
  )
{
  EFI_STATUS  Status;
  UINTN       BootOrderSize;
  UINT16      *BootOrder;
  VOID        *OptionData;

  BootOrderSize = 0;
  Status        = gRT->GetVariable (
                         EFI_BOOT_ORDER_VARIABLE_NAME,
                         &gEfiGlobalVariableGuid,
                         NULL,
                         &BootOrderSize,
                         NULL
                         );

  DEBUG ((
    DEBUG_INFO,
    "OCVAR: Have existing order of size %u - %r\n",
    (UINT32)BootOrderSize,
    Status
    ));

  if ((Status != EFI_BUFFER_TOO_SMALL) || (BootOrderSize == 0) || (BootOrderSize % sizeof (UINT16) != 0)) {
    return NULL;
  }

  BootOrder = AllocatePool (BootOrderSize);
  if (BootOrder == NULL) {
    DEBUG ((DEBUG_INFO, "OCVAR: Failed to allocate boot order\n"));
    return NULL;
  }

  Status = gRT->GetVariable (
                  EFI_BOOT_ORDER_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &BootOrderSize,
                  BootOrder
                  );
  if (EFI_ERROR (Status) || (BootOrderSize == 0) || (BootOrderSize % sizeof (UINT16) != 0)) {
    DEBUG ((DEBUG_INFO, "OCVAR: Failed to obtain boot order %u - %r\n", (UINT32)BootOrderSize, Status));
    FreePool (BootOrder);
    return NULL;
  }

  //
  // OpenCore moved Bootstrap to BootOrder[0] on initialisation.
  //
  OptionData = OcGetBootOptionData (
                 OptionSize,
                 BootOrder[0],
                 &gEfiGlobalVariableGuid
                 );
  *Option = BootOrder[0];

  FreePool (BootOrder);

  return OptionData;
}

VOID
OcDeleteVariables (
  IN BOOLEAN  PreserveBoot
  )
{
  EFI_STATUS                    Status;
  OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime;
  OC_FWRT_CONFIG                Config;
  UINTN                         BootProtectSize;
  UINT32                        BootProtect;
  VOID                          *BootOption;
  UINTN                         BootOptionSize;
  UINT16                        BootOptionIndex;

  DEBUG ((DEBUG_INFO, "OCVAR: NVRAM cleanup %d...\n", PreserveBoot));

  //
  // Obtain boot protection marker.
  //
  if (PreserveBoot) {
    //
    // We do not need or want to re-create entry and overwrite boot order, in this case.
    //
    BootProtect = 0;
  } else {
    BootProtectSize = sizeof (BootProtect);
    Status          = gRT->GetVariable (
                             OC_BOOT_PROTECT_VARIABLE_NAME,
                             &gOcVendorVariableGuid,
                             NULL,
                             &BootProtectSize,
                             &BootProtect
                             );
    if (EFI_ERROR (Status) || (BootProtectSize != sizeof (BootProtect))) {
      BootProtect = 0;
    }
  }

  Status = gBS->LocateProtocol (
                  &gOcFirmwareRuntimeProtocolGuid,
                  NULL,
                  (VOID **)&FwRuntime
                  );

  if (!EFI_ERROR (Status) && (FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION)) {
    ZeroMem (&Config, sizeof (Config));
    FwRuntime->SetOverride (&Config);
    DEBUG ((DEBUG_INFO, "OCVAR: Found FW NVRAM, full access %d\n", Config.BootVariableRedirect));
  } else {
    FwRuntime = NULL;
    DEBUG ((DEBUG_INFO, "OCVAR: Missing compatible FW NVRAM, going on...\n"));
  }

  if ((BootProtect & OC_BOOT_PROTECT_VARIABLE_BOOTSTRAP) != 0) {
    BootOption = GetBootstrapBootData (&BootOptionSize, &BootOptionIndex);
    if (BootOption != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "OCVAR: Found %g:Boot%04x for preservation of %u bytes\n",
        &gEfiGlobalVariableGuid,
        BootOptionIndex,
        (UINT32)BootOptionSize
        ));
    } else {
      BootProtect = 0;
    }
  }

  DeleteVariables (PreserveBoot);

  if ((BootProtect & OC_BOOT_PROTECT_VARIABLE_BOOTSTRAP) != 0) {
    BootOptionIndex = 1;
    Status          = gRT->SetVariable (
                             L"Boot0001",
                             &gEfiGlobalVariableGuid,
                             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                             BootOptionSize,
                             BootOption
                             );
    if (!EFI_ERROR (Status)) {
      Status = gRT->SetVariable (
                      EFI_BOOT_ORDER_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      sizeof (BootOptionIndex),
                      &BootOptionIndex
                      );
    }

    DEBUG ((DEBUG_INFO, "OCVAR: Set bootstrap option to Boot%04x - %r\n", BootOptionIndex, Status));
    FreePool (BootOption);
  }

  if (FwRuntime != NULL) {
    DEBUG ((DEBUG_INFO, "OCVAR: Restoring FW NVRAM...\n"));
    FwRuntime->SetOverride (NULL);
  }
}

EFI_STATUS
OcResetNvram (
  IN     BOOLEAN  PreserveBoot
  )
{
  OcDeleteVariables (PreserveBoot);
  DirectResetCold ();
  return EFI_DEVICE_ERROR;
}
