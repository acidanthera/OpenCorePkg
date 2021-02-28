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
#include <Library/PrintLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Safe version check, documented in config.
**/
#define OC_NVRAM_STORAGE_VERSION 1

/**
  Structure declaration for nvram file.
**/
#define OC_NVRAM_STORAGE_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
  OC_DECLARE (OC_NVRAM_STORAGE_MAP)

#define OC_NVRAM_STORAGE_FIELDS(_, __) \
  _(UINT32                      , Version  ,     , 0                                       , () ) \
  _(OC_NVRAM_STORAGE_MAP        , Add      ,     , OC_CONSTR (OC_NVRAM_STORAGE_MAP, _, __) , OC_DESTR (OC_NVRAM_STORAGE_MAP))
  OC_DECLARE (OC_NVRAM_STORAGE)

OC_MAP_STRUCTORS (OC_NVRAM_STORAGE_MAP)
OC_STRUCTORS (OC_NVRAM_STORAGE, ())

/**
  Schema definition for nvram file.
**/

STATIC
OC_SCHEMA
mNvramStorageEntrySchema = OC_SCHEMA_MDATA (NULL);

STATIC
OC_SCHEMA
mNvramStorageAddSchema = OC_SCHEMA_MAP (NULL, &mNvramStorageEntrySchema);

STATIC
OC_SCHEMA
mNvramStorageNodesSchema[] = {
  OC_SCHEMA_MAP_IN     ("Add",     OC_STORAGE_VAULT, Files, &mNvramStorageAddSchema),
  OC_SCHEMA_INTEGER_IN ("Version", OC_STORAGE_VAULT, Version),
};

STATIC
OC_SCHEMA_INFO
mNvramStorageRootSchema = {
  .Dict = {mNvramStorageNodesSchema, ARRAY_SIZE (mNvramStorageNodesSchema)}
};

STATIC
VOID
OcReportVersion (
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  CONST CHAR8  *Version;

  Version = OcMiscGetVersionString ();

  DEBUG ((DEBUG_INFO, "OC: Current version is %a\n", Version));

  if ((Config->Misc.Security.ExposeSensitiveData & OCS_EXPOSE_VERSION_VAR) != 0) {
    gRT->SetVariable (
      OC_VERSION_VARIABLE_NAME,
      &gOcVendorVariableGuid,
      OPEN_CORE_NVRAM_ATTR,
      AsciiStrLen (Version),
      (VOID *) Version
      );
  }
}

STATIC
EFI_STATUS
OcProcessVariableGuid (
  IN  CONST CHAR8            *AsciiVariableGuid,
  OUT GUID                   *VariableGuid,
  IN  OC_NVRAM_LEGACY_MAP    *Schema  OPTIONAL,
  OUT OC_NVRAM_LEGACY_ENTRY  **SchemaEntry  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      GuidIndex;

  Status = AsciiStrToGuid (AsciiVariableGuid, VariableGuid);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM GUID %a - %r\n", AsciiVariableGuid, Status));
  }

  if (!EFI_ERROR (Status) && Schema != NULL) {
    for (GuidIndex = 0; GuidIndex < Schema->Count; ++GuidIndex) {
      if (AsciiStrCmp (AsciiVariableGuid, OC_BLOB_GET (Schema->Keys[GuidIndex])) == 0) {
        *SchemaEntry = Schema->Values[GuidIndex];
        return Status;
      }
    }

    DEBUG ((DEBUG_INFO, "OC: Ignoring NVRAM GUID %a\n", AsciiVariableGuid));
    Status = EFI_SECURITY_VIOLATION;
  }

  return Status;
}

STATIC
VOID
OcSetNvramVariable (
  IN CONST CHAR8            *AsciiVariableName,
  IN EFI_GUID               *VariableGuid,
  IN UINT32                 Attributes,
  IN UINT32                 VariableSize,
  IN VOID                   *VariableData,
  IN OC_NVRAM_LEGACY_ENTRY  *SchemaEntry,
  IN BOOLEAN                Overwrite
  )
{
  EFI_STATUS            Status;
  UINTN                 OriginalVariableSize;
  CHAR16                *UnicodeVariableName;
  BOOLEAN               IsAllowed;
  UINT32                VariableIndex;
  VOID                  *OrgValue;
  UINTN                 OrgSize;
  UINT32                OrgAttributes;

  if (SchemaEntry != NULL) {
    IsAllowed = FALSE;

    //
    // TODO: Consider optimising lookup if it causes problems...
    //
    for (VariableIndex = 0; VariableIndex < SchemaEntry->Count; ++VariableIndex) {
      if (VariableIndex == 0 && AsciiStrCmp ("*", OC_BLOB_GET (SchemaEntry->Values[VariableIndex])) == 0) {
        IsAllowed = TRUE;
        break;
      }

      if (AsciiStrCmp (AsciiVariableName, OC_BLOB_GET (SchemaEntry->Values[VariableIndex])) == 0) {
        IsAllowed = TRUE;
        break;
      }
    }

    if (!IsAllowed) {
      DEBUG ((DEBUG_INFO, "OC: Setting NVRAM %g:%a is not permitted\n", VariableGuid, AsciiVariableName));
      return;
    }
  }

  UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

  if (UnicodeVariableName == NULL) {
    DEBUG ((DEBUG_WARN, "OC: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
    return;
  }

  OriginalVariableSize = 0;
  Status = gRT->GetVariable (
    UnicodeVariableName,
    VariableGuid,
    NULL,
    &OriginalVariableSize,
    NULL
    );

  if (Status == EFI_BUFFER_TOO_SMALL && Overwrite) {
    Status = GetVariable3 (UnicodeVariableName, VariableGuid, &OrgValue, &OrgSize, &OrgAttributes);
    if (!EFI_ERROR (Status)) {
      //
      // Do not allow overwriting BS-only variables. Ideally we also check for NV attribute,
      // but it is not set by Duet.
      //
      if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS))
        == (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) {
        Status = gRT->SetVariable (UnicodeVariableName, VariableGuid, 0, 0, 0);

        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "OC: Failed to delete overwritten variable %g:%a - %r\n",
            VariableGuid,
            AsciiVariableName,
            Status
            ));
          Status = EFI_BUFFER_TOO_SMALL;
        }
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OC: Overwritten variable %g:%a has invalid attrs - %X\n",
          VariableGuid,
          AsciiVariableName,
          Attributes
          ));
        Status = EFI_BUFFER_TOO_SMALL;
      }

      FreePool (OrgValue);
    } else {
      DEBUG ((
        DEBUG_INFO,
        "OC: Overwritten variable %g:%a has unknown attrs - %r\n",
        VariableGuid,
        AsciiVariableName,
        Status
        ));
      Status = EFI_BUFFER_TOO_SMALL;
    }
  }

  if (Status != EFI_BUFFER_TOO_SMALL) {
    Status = gRT->SetVariable (
      UnicodeVariableName,
      VariableGuid,
      Attributes,
      VariableSize,
      VariableData
      );
    DEBUG ((
      EFI_ERROR (Status) && VariableSize > 0 ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting NVRAM %g:%a - %r\n",
      VariableGuid,
      AsciiVariableName,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OC: Setting NVRAM %g:%a - ignored, exists\n",
      VariableGuid,
      AsciiVariableName,
      Status
      ));
  }

  FreePool (UnicodeVariableName);
}

STATIC
VOID
OcLoadLegacyNvram (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem,
  IN OC_GLOBAL_CONFIG                *Config
  )
{
  UINT8                 *FileBuffer;
  UINT32                FileSize;
  OC_NVRAM_STORAGE      Nvram;
  BOOLEAN               IsValid;
  EFI_STATUS            Status;
  UINT32                GuidIndex;
  UINT32                VariableIndex;
  GUID                  VariableGuid;
  OC_ASSOC              *VariableMap;
  OC_NVRAM_LEGACY_ENTRY *SchemaEntry;
  OC_NVRAM_LEGACY_MAP   *Schema;

  Schema = &Config->Nvram.Legacy;

  FileBuffer = ReadFile (FileSystem, OPEN_CORE_NVRAM_PATH, &FileSize, BASE_1MB);
  if (FileBuffer == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Invalid nvram data\n"));
    return;
  }

  OC_NVRAM_STORAGE_CONSTRUCT (&Nvram, sizeof (Nvram));
  IsValid = ParseSerialized (&Nvram, &mNvramStorageRootSchema, FileBuffer, FileSize, NULL);
  FreePool (FileBuffer);

  if (!IsValid || Nvram.Version != OC_NVRAM_STORAGE_VERSION) {
    DEBUG ((
      DEBUG_WARN,
      "OC: Incompatible nvram data, version %u vs %d\n",
      Nvram.Version,
      OC_NVRAM_STORAGE_VERSION
      ));
    OC_NVRAM_STORAGE_DESTRUCT (&Nvram, sizeof (Nvram));
    return;
  }

  for (GuidIndex = 0; GuidIndex < Nvram.Add.Count; ++GuidIndex) {
    Status = OcProcessVariableGuid (
      OC_BLOB_GET (Nvram.Add.Keys[GuidIndex]),
      &VariableGuid,
      Schema,
      &SchemaEntry
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    VariableMap = Nvram.Add.Values[GuidIndex];

    for (VariableIndex = 0; VariableIndex < VariableMap->Count; ++VariableIndex) {
      OcSetNvramVariable (
        OC_BLOB_GET (VariableMap->Keys[VariableIndex]),
        &VariableGuid,
        Config->Nvram.WriteFlash ? OPEN_CORE_NVRAM_NV_ATTR : OPEN_CORE_NVRAM_ATTR,
        VariableMap->Values[VariableIndex]->Size,
        OC_BLOB_GET (VariableMap->Values[VariableIndex]),
        SchemaEntry,
        Config->Nvram.LegacyOverwrite
        );
    }
  }

  OC_NVRAM_STORAGE_DESTRUCT (&Nvram, sizeof (Nvram));
}

STATIC
VOID
OcDeleteNvram (
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS    Status;
  UINT32        DeleteGuidIndex;
  UINT32        AddGuidIndex;
  UINT32        DeleteVariableIndex;
  UINT32        AddVariableIndex;
  CONST CHAR8   *AsciiVariableName;
  CHAR16        *UnicodeVariableName;
  GUID          VariableGuid;
  OC_ASSOC      *VariableMap;
  VOID          *CurrentValue;
  UINTN         CurrentValueSize;
  BOOLEAN       SameContents;

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
        OC_BLOB_GET (Config->Nvram.Add.Keys[AddGuidIndex])) == 0) {
        break;
      }
    }

    for (DeleteVariableIndex = 0; DeleteVariableIndex < Config->Nvram.Delete.Values[DeleteGuidIndex]->Count; ++DeleteVariableIndex) {
      AsciiVariableName   = OC_BLOB_GET (Config->Nvram.Delete.Values[DeleteGuidIndex]->Values[DeleteVariableIndex]);

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
          } else if (Status == EFI_NOT_FOUND && VariableMap->Values[AddVariableIndex]->Size == 0) {
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
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS    Status;
  UINT32        GuidIndex;
  UINT32        VariableIndex;
  GUID          VariableGuid;
  OC_ASSOC      *VariableMap;

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

    VariableMap       = Config->Nvram.Add.Values[GuidIndex];
    
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
  if (Config->Nvram.LegacyEnable && Storage->FileSystem != NULL) {
    OcLoadLegacyNvram (Storage->FileSystem, Config);
  }

  OcDeleteNvram (Config);

  OcAddNvram (Config);

  OcReportVersion (Config);
}
