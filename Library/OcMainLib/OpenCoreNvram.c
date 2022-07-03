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

#include <Library/OcMainLib.h>

#include <Guid/OcVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
VOID
OcReportVersion (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  CONST CHAR8  *Version;

  Version = OcMiscGetVersionString ();

  DEBUG ((DEBUG_INFO, "OC: Current version is %a\n", Version));

  if ((Config->Misc.Security.ExposeSensitiveData & OCS_EXPOSE_VERSION_VAR) != 0) {
    OcSetSystemVariable (
      OC_VERSION_VARIABLE_NAME,
      OPEN_CORE_NVRAM_ATTR,
      AsciiStrLen (Version),
      (VOID *)Version,
      NULL
      );
  } else {
    OcSetSystemVariable (
      OC_VERSION_VARIABLE_NAME,
      OPEN_CORE_NVRAM_ATTR,
      L_STR_LEN ("UNK-000-0000-00-00"),
      "UNK-000-0000-00-00",
      NULL
      );
  }
}

STATIC
VOID
OcDeleteNvram (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  EFI_STATUS   Status;
  UINT32       DeleteGuidIndex;
  UINT32       AddGuidIndex;
  UINT32       DeleteVariableIndex;
  UINT32       AddVariableIndex;
  CONST CHAR8  *AsciiVariableName;
  CHAR16       *UnicodeVariableName;
  GUID         VariableGuid;
  OC_ASSOC     *VariableMap;
  VOID         *CurrentValue;
  UINTN        CurrentValueSize;
  BOOLEAN      SameContents;

  for (DeleteGuidIndex = 0; DeleteGuidIndex < Config->Nvram.Delete.Count; ++DeleteGuidIndex) {
    Status = OcProcessVariableGuid (
               OC_BLOB_GET (Config->Nvram.Delete.Keys[DeleteGuidIndex]),
               &VariableGuid,
               NULL,
               NULL
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // When variable is set and non-volatile variable setting is used,
    // we do not want a variable to be constantly removed and added every reboot,
    // as it will negatively impact flash memory. In case the variable is already set
    // and has the same value we do not delete it.
    //
    for (AddGuidIndex = 0; AddGuidIndex < Config->Nvram.Add.Count; ++AddGuidIndex) {
      if (AsciiStrCmp (
            OC_BLOB_GET (Config->Nvram.Delete.Keys[DeleteGuidIndex]),
            OC_BLOB_GET (Config->Nvram.Add.Keys[AddGuidIndex])
            ) == 0)
      {
        break;
      }
    }

    for (DeleteVariableIndex = 0; DeleteVariableIndex < Config->Nvram.Delete.Values[DeleteGuidIndex]->Count; ++DeleteVariableIndex) {
      AsciiVariableName = OC_BLOB_GET (Config->Nvram.Delete.Values[DeleteGuidIndex]->Values[DeleteVariableIndex]);

      //
      // '#' is filtered in all keys, but for values we need to do it ourselves.
      //
      if (AsciiVariableName[0] == '#') {
        DEBUG ((DEBUG_INFO, "OC: Variable skip deleting %a\n", AsciiVariableName));
        continue;
      }

      UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

      if (UnicodeVariableName == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
        continue;
      }

      if (AddGuidIndex != Config->Nvram.Add.Count) {
        VariableMap = NULL;
        for (AddVariableIndex = 0; AddVariableIndex < Config->Nvram.Add.Values[AddGuidIndex]->Count; ++AddVariableIndex) {
          if (AsciiStrCmp (AsciiVariableName, OC_BLOB_GET (Config->Nvram.Add.Values[AddGuidIndex]->Keys[AddVariableIndex])) == 0) {
            VariableMap = Config->Nvram.Add.Values[AddGuidIndex];
            break;
          }
        }

        if (VariableMap != NULL) {
          Status = GetVariable2 (UnicodeVariableName, &VariableGuid, &CurrentValue, &CurrentValueSize);

          if (!EFI_ERROR (Status)) {
            SameContents = CurrentValueSize == VariableMap->Values[AddVariableIndex]->Size
                           && CompareMem (OC_BLOB_GET (VariableMap->Values[AddVariableIndex]), CurrentValue, CurrentValueSize) == 0;
            FreePool (CurrentValue);
          } else if ((Status == EFI_NOT_FOUND) && (VariableMap->Values[AddVariableIndex]->Size == 0)) {
            SameContents = TRUE;
          } else {
            SameContents = FALSE;
          }

          if (SameContents) {
            DEBUG ((DEBUG_INFO, "OC: Not deleting NVRAM %g:%a, matches add\n", &VariableGuid, AsciiVariableName));
            FreePool (UnicodeVariableName);
            continue;
          }
        }
      }

      Status = gRT->SetVariable (UnicodeVariableName, &VariableGuid, 0, 0, 0);
      DEBUG ((
        EFI_ERROR (Status) && Status != EFI_NOT_FOUND ? DEBUG_WARN : DEBUG_INFO,
        "OC: Deleting NVRAM %g:%a - %r\n",
        &VariableGuid,
        AsciiVariableName,
        Status
        ));

      FreePool (UnicodeVariableName);
    }
  }
}

STATIC
VOID
OcAddNvram (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  EFI_STATUS  Status;
  UINT32      GuidIndex;
  UINT32      VariableIndex;
  GUID        VariableGuid;
  OC_ASSOC    *VariableMap;

  for (GuidIndex = 0; GuidIndex < Config->Nvram.Add.Count; ++GuidIndex) {
    Status = OcProcessVariableGuid (
               OC_BLOB_GET (Config->Nvram.Add.Keys[GuidIndex]),
               &VariableGuid,
               NULL,
               NULL
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    VariableMap = Config->Nvram.Add.Values[GuidIndex];

    for (VariableIndex = 0; VariableIndex < VariableMap->Count; ++VariableIndex) {
      OcSetNvramVariable (
        OC_BLOB_GET (VariableMap->Keys[VariableIndex]),
        &VariableGuid,
        Config->Nvram.WriteFlash ? OPEN_CORE_NVRAM_NV_ATTR : OPEN_CORE_NVRAM_ATTR,
        VariableMap->Values[VariableIndex]->Size,
        OC_BLOB_GET (VariableMap->Values[VariableIndex]),
        NULL,
        FALSE
        );
    }
  }
}

VOID
OcLoadNvramSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  OcLoadLegacyNvram (
    Storage,
    &Config->Nvram.Legacy,
    Config->Nvram.LegacyOverwrite,
    Config->Uefi.Quirks.RequestBootVarRouting
    );

  OcDeleteNvram (Config);

  OcAddNvram (Config);

  OcReportVersion (Config);
}
