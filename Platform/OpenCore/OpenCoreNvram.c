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
  UINT32        GuidIndex;
  UINT32        VariableIndex;
  CONST CHAR8   *AsciiVariableGuid;
  CONST CHAR8   *AsciiVariableName;
  CHAR16        *UnicodeVariableName;
  EFI_GUID      VariableGuid;
  OC_ASSOC      *VariableMap;
  UINT8         *VariableData;
  UINT32        VariableSize;

  for (GuidIndex = 0; GuidIndex < Config->Nvram.Block.Count; ++GuidIndex) {
    AsciiVariableGuid = OC_BLOB_GET (Config->Nvram.Block.Keys[GuidIndex]);
    ZeroMem (&VariableGuid, sizeof (VariableGuid));

    //
    // TODO: string to EFI_GUID
    //
    (VOID) AsciiVariableGuid;
    (VOID) mDefaultAttributes;

    for (VariableIndex = 0; VariableIndex < Config->Nvram.Block.Values[GuidIndex]->Count; ++VariableIndex) {
      AsciiVariableName   = OC_BLOB_GET (Config->Nvram.Block.Values[GuidIndex]->Values[VariableIndex]);
      UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

      if (UnicodeVariableName == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
        continue;
      }

#if 0
      Status = gRT->SetVariable (UnicodeVariableName, &VariableGuid, 0, 0, 0);
      DEBUG ((DEBUG_INFO, "OC: Deleting NVRAM %g:%a - %r\n", &VariableGuid, AsciiVariableGuid, Status));
#endif

      FreePool (UnicodeVariableName);
    }
  }

  for (GuidIndex = 0; GuidIndex < Config->Nvram.Add.Count; ++GuidIndex) {
    VariableMap       = Config->Nvram.Add.Values[GuidIndex];
    AsciiVariableGuid = OC_BLOB_GET (VariableMap->Keys[GuidIndex]);
    ZeroMem (&VariableGuid, sizeof (VariableGuid));

    //
    // TODO: string to EFI_GUID
    //
    (VOID) AsciiVariableGuid;

    for (VariableIndex = 0; VariableIndex < VariableMap->Count; ++VariableIndex) {
      AsciiVariableName   = OC_BLOB_GET (VariableMap->Keys[VariableIndex]);
      VariableData        = OC_BLOB_GET (VariableMap->Values[VariableIndex]);
      VariableSize        = VariableMap->Values[VariableIndex]->Size;
      UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

      if (UnicodeVariableName == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
        continue;
      }

#if 0
      Status = gRT->SetVariable (
        UnicodeVariableName,
        &VariableGuid,
        mDefaultAttributes,
        VariableData,
        VariableSize
        );
      DEBUG ((DEBUG_INFO, "OC: Setting NVRAM %g:%a - %r\n", &VariableGuid, AsciiVariableGuid, Status));
#endif

      FreePool (UnicodeVariableName);
    }
  }
}
