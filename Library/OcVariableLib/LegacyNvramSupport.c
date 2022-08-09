/** @file
  Supports legacy NVRAM configuration.

  Used by OcMainLib and OcVariableRuntimeLib (i.e. emulated NVRAM support).

  References OpenCore global config (for legacy NVRAM). Despite referencing
  global config directly, is required to live outside OcMainLib to avoid
  having to link OcMainLib and all its dependencies to OpenDuet, when we
  link OcVariableRuntimeLib to OpenDuet.

  Copyright (c) 2019-2022, vit9696, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/
#include <Uefi.h>

#include <Guid/OcVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

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
    DEBUG ((DEBUG_WARN, "OCVAR: Failed to convert NVRAM GUID %a - %r\n", AsciiVariableGuid, Status));
  }

  if (!EFI_ERROR (Status) && (Schema != NULL)) {
    for (GuidIndex = 0; GuidIndex < Schema->Count; ++GuidIndex) {
      if (AsciiStrCmp (AsciiVariableGuid, OC_BLOB_GET (Schema->Keys[GuidIndex])) == 0) {
        *SchemaEntry = Schema->Values[GuidIndex];
        return Status;
      }
    }

    DEBUG ((DEBUG_INFO, "OCVAR: Ignoring NVRAM GUID %a\n", AsciiVariableGuid));
    Status = EFI_SECURITY_VIOLATION;
  }

  return Status;
}

BOOLEAN
OcVariableIsAllowedBySchemaEntry (
  IN OC_NVRAM_LEGACY_ENTRY  *SchemaEntry,
  IN EFI_GUID               *VariableGuid OPTIONAL,
  IN CONST VOID             *VariableName,
  IN OC_STRING_FORMAT       StringFormat
  )
{
  BOOLEAN  IsAllowed;
  UINT32   VariableIndex;

  if (SchemaEntry == NULL) {
    return TRUE;
  }

  IsAllowed = FALSE;

  //
  // TODO: Consider optimising lookup if it causes problems...
  //
  for (VariableIndex = 0; VariableIndex < SchemaEntry->Count; ++VariableIndex) {
    if ((VariableIndex == 0) && (AsciiStrCmp ("*", OC_BLOB_GET (SchemaEntry->Values[VariableIndex])) == 0)) {
      IsAllowed = TRUE;
      break;
    }

    if (  (StringFormat == OcStringFormatAscii)
       && (AsciiStrCmp ((CONST CHAR8 *)VariableName, OC_BLOB_GET (SchemaEntry->Values[VariableIndex])) == 0))
    {
      IsAllowed = TRUE;
      break;
    }

    if (  (StringFormat == OcStringFormatUnicode)
       && (MixedStrCmp ((CONST CHAR16 *)VariableName, OC_BLOB_GET (SchemaEntry->Values[VariableIndex])) == 0))
    {
      IsAllowed = TRUE;
      break;
    }
  }

  if (!IsAllowed) {
    DEBUG ((DEBUG_INFO, "OCVAR: NVRAM %g:%s is not permitted\n", VariableGuid, VariableName));
  }

  return IsAllowed;
}

VOID
OcSetNvramVariable (
  IN CONST CHAR8            *AsciiVariableName,
  IN EFI_GUID               *VariableGuid,
  IN UINT32                 Attributes,
  IN UINT32                 VariableSize,
  IN VOID                   *VariableData,
  IN OC_NVRAM_LEGACY_ENTRY  *SchemaEntry OPTIONAL,
  IN BOOLEAN                Overwrite
  )
{
  EFI_STATUS  Status;
  UINTN       OriginalVariableSize;
  CHAR16      *UnicodeVariableName;
  VOID        *OrgValue;
  UINTN       OrgSize;
  UINT32      OrgAttributes;

  if (!OcVariableIsAllowedBySchemaEntry (SchemaEntry, VariableGuid, AsciiVariableName, OcStringFormatAscii)) {
    return;
  }

  UnicodeVariableName = AsciiStrCopyToUnicode (AsciiVariableName, 0);

  if (UnicodeVariableName == NULL) {
    DEBUG ((DEBUG_WARN, "OCVAR: Failed to convert NVRAM variable name %a\n", AsciiVariableName));
    return;
  }

  OriginalVariableSize = 0;
  Status               = gRT->GetVariable (
                                UnicodeVariableName,
                                VariableGuid,
                                NULL,
                                &OriginalVariableSize,
                                NULL
                                );

  if ((Status == EFI_BUFFER_TOO_SMALL) && Overwrite) {
    Status = GetVariable3 (UnicodeVariableName, VariableGuid, &OrgValue, &OrgSize, &OrgAttributes);
    if (!EFI_ERROR (Status)) {
      //
      // Do not allow overwriting BS-only variables. Ideally we also check for NV attribute,
      // but it is not set by Duet.
      //
      if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS))
          == (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS))
      {
        Status = gRT->SetVariable (UnicodeVariableName, VariableGuid, 0, 0, 0);

        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "OCVAR: Failed to delete overwritten variable %g:%a - %r\n",
            VariableGuid,
            AsciiVariableName,
            Status
            ));
          Status = EFI_BUFFER_TOO_SMALL;
        }
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCVAR: Overwritten variable %g:%a has invalid attrs - %X\n",
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
        "OCVAR: Overwritten variable %g:%a has unknown attrs - %r\n",
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
      "OCVAR: Setting NVRAM %g:%a - %r\n",
      VariableGuid,
      AsciiVariableName,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OCVAR: Setting NVRAM %g:%a - ignored, exists\n",
      VariableGuid,
      AsciiVariableName,
      Status
      ));
  }

  FreePool (UnicodeVariableName);
}
