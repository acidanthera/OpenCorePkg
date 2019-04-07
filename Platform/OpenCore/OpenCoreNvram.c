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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
CONST UINT32
mDefaultAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

VOID
OcLoadNvramSupport (
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS    Status;
  UINT32        GuidIndex;
  UINT32        VariableIndex;
  CONST CHAR8   *AsciiVariableGuid;
  CONST CHAR8   *AsciiVariableName;
  CHAR16        *UnicodeVariableName;
  GUID          VariableGuid;
  OC_ASSOC      *VariableMap;
  UINT8         *VariableData;
  UINT32        VariableSize;
  UINTN         OriginalVariableSize;

  for (GuidIndex = 0; GuidIndex < Config->Nvram.Block.Count; ++GuidIndex) {
    //
    // FIXME: Checking string length manually is due to inadequate assertions.
    //
    AsciiVariableGuid = OC_BLOB_GET (Config->Nvram.Block.Keys[GuidIndex]);
    if (AsciiStrLen (AsciiVariableGuid) == GUID_STRING_LENGTH) {
      Status = AsciiStrToGuid (AsciiVariableGuid, &VariableGuid);
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM GUID %a - %r\n", AsciiVariableGuid, Status));
      continue;
    }

    for (VariableIndex = 0; VariableIndex < Config->Nvram.Block.Values[GuidIndex]->Count; ++VariableIndex) {
      AsciiVariableName   = OC_BLOB_GET (Config->Nvram.Block.Values[GuidIndex]->Values[VariableIndex]);
      UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

      if (UnicodeVariableName == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
        continue;
      }

      Status = gRT->SetVariable (UnicodeVariableName, &VariableGuid, 0, 0, 0);
      DEBUG ((
        EFI_ERROR (Status) && Status != EFI_NOT_FOUND ? DEBUG_WARN : DEBUG_INFO,
        "OC: Deleting NVRAM %g:%a - %r\n",
        &VariableGuid,
        AsciiVariableGuid,
        Status
        ));

      FreePool (UnicodeVariableName);
    }
  }

  for (GuidIndex = 0; GuidIndex < Config->Nvram.Add.Count; ++GuidIndex) {
    VariableMap       = Config->Nvram.Add.Values[GuidIndex];
    //
    // FIXME: Checking string length manually is due to inadequate assertions.
    //
    AsciiVariableGuid = OC_BLOB_GET (Config->Nvram.Add.Keys[GuidIndex]);
    if (AsciiStrLen (AsciiVariableGuid) == GUID_STRING_LENGTH) {
      Status = AsciiStrToGuid (AsciiVariableGuid, &VariableGuid);
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM GUID %a - %r\n", AsciiVariableGuid, Status));
      continue;
    }

    for (VariableIndex = 0; VariableIndex < VariableMap->Count; ++VariableIndex) {
      AsciiVariableName   = OC_BLOB_GET (VariableMap->Keys[VariableIndex]);
      VariableData        = OC_BLOB_GET (VariableMap->Values[VariableIndex]);
      VariableSize        = VariableMap->Values[VariableIndex]->Size;
      UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

      if (UnicodeVariableName == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
        continue;
      }

      OriginalVariableSize = 0;
      Status = gRT->GetVariable (
        UnicodeVariableName,
        &VariableGuid,
        NULL,
        &OriginalVariableSize,
        NULL
        );

      DEBUG ((
        DEBUG_INFO,
        "OC: Getting NVRAM %g:%a - %r (will skip on too small)\n",
        &VariableGuid,
        AsciiVariableGuid,
        Status
        ));

      if (Status != EFI_BUFFER_TOO_SMALL) {
        Status = gRT->SetVariable (
          UnicodeVariableName,
          &VariableGuid,
          mDefaultAttributes,
          VariableSize,
          VariableData
          );
        DEBUG ((
          EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
          "OC: Setting NVRAM %g:%a - %r\n",
          &VariableGuid,
          AsciiVariableGuid,
          Status
          ));
      }

      FreePool (UnicodeVariableName);
    }
  }
}
