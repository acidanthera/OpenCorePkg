/** @file
  Copyright (C) 2016, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Guid/AppleVariable.h>
#include <Guid/GlobalVariable.h>
#include <Guid/MicrosoftVariable.h>
#include <Guid/OcVariable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/OcFirmwareRuntime.h>

STATIC
EFI_GUID
mOzmosisProprietary1Guid     = { 0x4D1FDA02, 0x38C7, 0x4A6A, { 0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x01, 0x02 } };

STATIC
EFI_GUID
mOzmosisProprietary2Guid     = { 0x1F8E0C02, 0x58A9, 0x4E34, { 0xAE, 0x22, 0x2B, 0x63, 0x74, 0x5F, 0xA1, 0x01 } };

STATIC
EFI_GUID
mOzmosisProprietary3Guid     = { 0x9480E8A1, 0x1793, 0x46C9, { 0x91, 0xD8, 0x11, 0x08, 0xDB, 0xA4, 0x73, 0x1C } };

STATIC
EFI_GUID
mBootChimeVendorVariableGuid = {0x89D4F995, 0x67E3, 0x4895, { 0x8F, 0x18, 0x45, 0x4B, 0x65, 0x1D, 0x92, 0x15 } };


STATIC
BOOLEAN
IsDeletableVariable (
  IN CHAR16    *Name,
  IN EFI_GUID  *Guid
  )
{
  //
  // Apple GUIDs
  //
  if (CompareGuid (Guid, &gAppleVendorVariableGuid)
    || CompareGuid (Guid, &gAppleBootVariableGuid)
    || CompareGuid (Guid, &gAppleCoreStorageVariableGuid)
    || CompareGuid (Guid, &gAppleTamperResistantBootVariableGuid)
    || CompareGuid (Guid, &gAppleWirelessNetworkVariableGuid)
    || CompareGuid (Guid, &gApplePersonalizationVariableGuid)
    || CompareGuid (Guid, &gAppleNetbootVariableGuid)
    || CompareGuid (Guid, &gAppleSecureBootVariableGuid)
    || CompareGuid (Guid, &gAppleTamperResistantBootSecureVariableGuid)
    || CompareGuid (Guid, &gAppleTamperResistantBootEfiUserVariableGuid)) {
    return TRUE;
  //
  // Global variable boot options
  //
  } else if (CompareGuid (Guid, &gEfiGlobalVariableGuid)) {
    //
    // Only erase boot and driver entries for BDS
    // I.e. BootOrder, Boot####, DriverOrder, Driver####
    //
    if (StrnCmp (Name, L"Boot", StrLen(L"Boot")) == 0
      || StrnCmp (Name, L"Driver", StrLen(L"Driver")) == 0) {
      return TRUE;
    }
  //
  // Lilu & OpenCore extensions if present
  //
  } else if (CompareGuid (Guid, &gOcVendorVariableGuid)) {
    //
    // Do not remove OpenCore critical variables.
    //
    if (StrCmp (Name, OC_BOOT_REDIRECT_VARIABLE_NAME) != 0
      && StrCmp (Name, OC_SCAN_POLICY_VARIABLE_NAME) != 0) {
      return TRUE;
    }
  } else if (CompareGuid (Guid, &gOcReadOnlyVariableGuid)
    || CompareGuid (Guid, &gOcWriteOnlyVariableGuid)) {
    return TRUE;
  //
  // Ozmozis extensions if present
  //
  } else if (CompareGuid (Guid, &mOzmosisProprietary1Guid)
    || CompareGuid (Guid, &mOzmosisProprietary2Guid)
    || CompareGuid (Guid, &mOzmosisProprietary3Guid)) {
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

STATIC
VOID
DeleteVariables (
  VOID
  )
{
  EFI_GUID     CurrentGuid;
  EFI_STATUS   Status;
  CHAR16       *Buffer;
  CHAR16       *TmpBuffer;
  UINTN        BufferSize;
  UINTN        RequestedSize;
  BOOLEAN      Restart;
  BOOLEAN      CriticalFailure;

  //
  // Request 1024 byte buffer.
  //
  Buffer = NULL;
  BufferSize = 0;
  RequestedSize = 1024;

  //
  // Force starting from 0th GUID.
  //
  Restart = TRUE;

  //
  // Assume we have not failed yet.
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
        Buffer = TmpBuffer;
        BufferSize = RequestedSize;
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCB: Failed to allocate variable name buffer of %u bytes\n",
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
    Status = gRT->GetNextVariableName (&RequestedSize, Buffer, &CurrentGuid);

    if (!EFI_ERROR (Status)) {
      if (IsDeletableVariable (Buffer, &CurrentGuid)) {
        Status = gRT->SetVariable (Buffer, &CurrentGuid, 0, 0, NULL);
        if (!EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "Deleting %g:%s... OK\n",
            &CurrentGuid,
            Buffer
            ));
          //
          // Calls to SetVariable() between calls to GetNextVariableName()
          // may produce unpredictable results, so we restart.
          //
          Restart = TRUE;
        } else if (Status == EFI_NOT_FOUND || Status == EFI_SECURITY_VIOLATION) {
          DEBUG ((
            DEBUG_INFO,
            "Deleting %g:%s... SKIP - %r\n",
            &CurrentGuid,
            Buffer,
            Status
            ));
        } else {
          DEBUG ((
            DEBUG_INFO,
            "Deleting %g:%s... FAIL - %r\n",
            &CurrentGuid,
            Buffer,
            Status
            ));
          break;
        }
      } else {
        // Print (L"Skipping %g:%s\n", &CurrentGuid, Buffer);
      }
    } else if (Status != EFI_BUFFER_TOO_SMALL && Status != EFI_NOT_FOUND) {
      if (!CriticalFailure) {
        DEBUG ((DEBUG_INFO, "OCB: Unexpected error (%r), trying to rescan\n", Status));
        CriticalFailure = TRUE;
      } else {
        DEBUG ((DEBUG_INFO, "OCB: Unexpected error (%r), aborting\n", Status));
        break;
      }
    }
  } while (Status != EFI_NOT_FOUND);

  if (Buffer != NULL) {
    FreePool (Buffer);
  }
}

VOID
OcDeleteVariables (
  VOID
  )
{
  EFI_STATUS                   Status;
  OC_FIRMWARE_RUNTIME_PROTOCOL *FwRuntime;
  OC_FWRT_CONFIG               Config;
  UINTN                        BootProtectSize;
  EFI_GUID                     *BootProtectGuid;
  UINT32                       BootProtect;
  UINT16                       BootOrder;
  VOID                         *BootOption;
  UINTN                        BootOptionSize;

  DEBUG ((DEBUG_INFO, "OCB: NVRAM cleanup...\n"));

  //
  // Obtain boot protection marker.
  //
  BootProtectSize = sizeof (BootProtect);
  Status = gRT->GetVariable (
    OC_BOOT_PROTECT_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    NULL,
    &BootProtectSize,
    &BootProtect
    );
  if (EFI_ERROR (Status)) {
    BootProtect = 0;
  }

  Status = gBS->LocateProtocol (
    &gOcFirmwareRuntimeProtocolGuid,
    NULL,
    (VOID **) &FwRuntime
    );

  if (!EFI_ERROR (Status) && FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION) {
    ZeroMem (&Config, sizeof (Config));
    FwRuntime->SetOverride (&Config);
    DEBUG ((DEBUG_INFO, "OCB: Found FW NVRAM, full access %d\n", Config.BootVariableRedirect));
  } else {
    FwRuntime = NULL;
    DEBUG ((DEBUG_INFO, "OCB: Missing compatible FW NVRAM, going on...\n"));
  }

  if ((BootProtect & OC_BOOT_PROTECT_VARIABLE_BOOTSTRAP) != 0) {
    BootProtectGuid = (BootProtect & OC_BOOT_PROTECT_VARIABLE_NAMESPACE) != 0 ? &gOcVendorVariableGuid : &gEfiGlobalVariableGuid;
    Status = GetVariable2 (
      OC_BOOT_OPTION_VARIABLE_NAME,
      BootProtectGuid,
      &BootOption,
      &BootOptionSize
      );
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Found %g:%s for preservation of %u bytes\n",
        BootProtectGuid,
        OC_BOOT_OPTION_VARIABLE_NAME,
        (UINT32) BootOptionSize
        ));
    } else {
      BootProtect = 0;
    }
  }

  DeleteVariables ();

  if ((BootProtect & OC_BOOT_PROTECT_VARIABLE_BOOTSTRAP) != 0) {
    Status = gRT->SetVariable (
      OC_BOOT_OPTION_VARIABLE_NAME,
      BootProtectGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
      BootOptionSize,
      BootOption
      );
    BootOrder = OC_BOOT_OPTION;
    if (!EFI_ERROR (Status)) {
      Status = gRT->SetVariable (
        EFI_BOOT_ORDER_VARIABLE_NAME,
        BootProtectGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
        sizeof (BootOrder),
        &BootOrder
        );
    }

    DEBUG ((DEBUG_INFO, "OCB: Restored %s - %r\n", OC_BOOT_OPTION_VARIABLE_NAME, Status));
    FreePool (BootOption);
  }

  if (FwRuntime != NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Restoring FW NVRAM...\n"));
    FwRuntime->SetOverride (NULL);
  }
}

EFI_STATUS
InternalSystemActionResetNvram (
  VOID
  )
{
  OcDeleteVariables ();
  DirectRestCold ();
  return EFI_DEVICE_ERROR;
}
