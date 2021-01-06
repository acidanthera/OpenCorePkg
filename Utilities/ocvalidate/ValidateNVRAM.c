/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "ocvalidate.h"
#include "OcValidateLib.h"

/**
  Callback funtion to verify whether one entry is duplicated in NVRAM->Add.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
NVRAMAddHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_STRING             *NVRAMAddPrimaryEntry;
  CONST OC_STRING             *NVRAMAddSecondaryEntry;
  CONST CHAR8                 *NVRAMAddPrimaryGUIDString;
  CONST CHAR8                 *NVRAMAddSecondaryGUIDString;

  NVRAMAddPrimaryEntry        = *(CONST OC_STRING **) PrimaryEntry;
  NVRAMAddSecondaryEntry      = *(CONST OC_STRING **) SecondaryEntry;
  NVRAMAddPrimaryGUIDString   = OC_BLOB_GET (NVRAMAddPrimaryEntry);
  NVRAMAddSecondaryGUIDString = OC_BLOB_GET (NVRAMAddSecondaryEntry);

  return StringIsDuplicated ("NVRAM->Add", NVRAMAddPrimaryGUIDString, NVRAMAddSecondaryGUIDString);
}

/**
  Callback funtion to verify whether one entry is duplicated in NVRAM->Delete.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
NVRAMDeleteHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_STRING                *NVRAMDeletePrimaryEntry;
  CONST OC_STRING                *NVRAMDeleteSecondaryEntry;
  CONST CHAR8                    *NVRAMDeletePrimaryGUIDString;
  CONST CHAR8                    *NVRAMDeleteSecondaryGUIDString;

  NVRAMDeletePrimaryEntry        = *(CONST OC_STRING **) PrimaryEntry;
  NVRAMDeleteSecondaryEntry      = *(CONST OC_STRING **) SecondaryEntry;
  NVRAMDeletePrimaryGUIDString   = OC_BLOB_GET (NVRAMDeletePrimaryEntry);
  NVRAMDeleteSecondaryGUIDString = OC_BLOB_GET (NVRAMDeleteSecondaryEntry);

  return StringIsDuplicated ("NVRAM->Delete", NVRAMDeletePrimaryGUIDString, NVRAMDeleteSecondaryGUIDString);
}

/**
  Callback funtion to verify whether one entry is duplicated in NVRAM->LegacySchema.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
NVRAMLegacySchemaHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_STRING                      *NVRAMLegacySchemaPrimaryEntry;
  CONST OC_STRING                      *NVRAMLegacySchemaSecondaryEntry;
  CONST CHAR8                          *NVRAMLegacySchemaPrimaryGUIDString;
  CONST CHAR8                          *NVRAMLegacySchemaSecondaryGUIDString;

  NVRAMLegacySchemaPrimaryEntry        = *(CONST OC_STRING **) PrimaryEntry;
  NVRAMLegacySchemaSecondaryEntry      = *(CONST OC_STRING **) SecondaryEntry;
  NVRAMLegacySchemaPrimaryGUIDString   = OC_BLOB_GET (NVRAMLegacySchemaPrimaryEntry);
  NVRAMLegacySchemaSecondaryGUIDString = OC_BLOB_GET (NVRAMLegacySchemaSecondaryEntry);

  return StringIsDuplicated ("NVRAM->LegacySchema", NVRAMLegacySchemaPrimaryGUIDString, NVRAMLegacySchemaSecondaryGUIDString);
}

STATIC
UINT32
CheckNVRAMAdd (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32           ErrorCount;
  UINT32           GuidIndex;
  UINT32           VariableIndex;
  OC_NVRAM_CONFIG  *UserNVRAM;
  CONST CHAR8      *AsciiGuid;
  CONST CHAR8      *AsciiNVRAMKey;
  OC_ASSOC         *VariableMap;

  ErrorCount = 0;
  UserNVRAM  = &Config->Nvram;

  for (GuidIndex = 0; GuidIndex < UserNVRAM->Add.Count; ++GuidIndex) {
    AsciiGuid = OC_BLOB_GET (UserNVRAM->Add.Keys[GuidIndex]);

    if (!AsciiGuidIsLegal (AsciiGuid)) {
      DEBUG ((DEBUG_WARN, "NVRAM->Add[%u] has borked GUID!\n", GuidIndex));
      ++ErrorCount;
    }

    VariableMap = UserNVRAM->Add.Values[GuidIndex];

    for (VariableIndex = 0; VariableIndex < VariableMap->Count; ++VariableIndex) {
      AsciiNVRAMKey = OC_BLOB_GET (VariableMap->Keys[VariableIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiPropertyIsLegal (AsciiNVRAMKey)) {
        DEBUG ((
          DEBUG_WARN,
          "NVRAM->Add[%u]->Key[%u] contains illegal character!\n",
          GuidIndex,
          VariableIndex
          ));
        ++ErrorCount;
      }
    }

    //
    // Check duplicated properties in NVRAM->Add.
    //
    ErrorCount += FindArrayDuplication (
      VariableMap->Keys,
      VariableMap->Count,
      sizeof (VariableMap->Keys[0]),
      NVRAMAddHasDuplication
      );
  }

  //
  // Check duplicated entries in NVRAM->Add.
  //
  ErrorCount += FindArrayDuplication (
    UserNVRAM->Add.Keys,
    UserNVRAM->Add.Count,
    sizeof (UserNVRAM->Add.Keys[0]),
    NVRAMAddHasDuplication
    );

  return ErrorCount;
}

STATIC
UINT32
CheckNVRAMDelete (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32           ErrorCount;
  UINT32           GuidIndex;
  UINT32           VariableIndex;
  OC_NVRAM_CONFIG  *UserNVRAM;
  CONST CHAR8      *AsciiGuid;
  CONST CHAR8      *AsciiNVRAMKey;

  ErrorCount = 0;
  UserNVRAM  = &Config->Nvram;

  for (GuidIndex = 0; GuidIndex < UserNVRAM->Delete.Count; ++GuidIndex) {
    AsciiGuid = OC_BLOB_GET (UserNVRAM->Delete.Keys[GuidIndex]);

    if (!AsciiGuidIsLegal (AsciiGuid)) {
      DEBUG ((DEBUG_WARN, "NVRAM->Delete[%u] has borked GUID!\n", GuidIndex));
      ++ErrorCount;
    }

    for (VariableIndex = 0; VariableIndex < UserNVRAM->Delete.Values[GuidIndex]->Count; ++VariableIndex) {
      AsciiNVRAMKey = OC_BLOB_GET (UserNVRAM->Delete.Values[GuidIndex]->Values[VariableIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiPropertyIsLegal (AsciiNVRAMKey)) {
        DEBUG ((
          DEBUG_WARN,
          "NVRAM->Delete[%u]->Key[%u] contains illegal character!\n",
          GuidIndex,
          VariableIndex
          ));
        ++ErrorCount;
      }
    }

    //
    // Check duplicated properties in NVRAM->Delete.
    //
    ErrorCount += FindArrayDuplication (
      UserNVRAM->Delete.Values[GuidIndex]->Values,
      UserNVRAM->Delete.Values[GuidIndex]->Count,
      sizeof (UserNVRAM->Delete.Values[GuidIndex]->Values[0]),
      NVRAMDeleteHasDuplication
      );
  }

  //
  // Check duplicated entries in NVRAM->Delete.
  //
  ErrorCount += FindArrayDuplication (
    UserNVRAM->Delete.Keys,
    UserNVRAM->Delete.Count,
    sizeof (UserNVRAM->Delete.Keys[0]),
    NVRAMDeleteHasDuplication
    );

  return ErrorCount;
}

STATIC
UINT32
CheckNVRAMSchema (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32           ErrorCount;
  UINT32           GuidIndex;
  UINT32           VariableIndex;
  OC_NVRAM_CONFIG  *UserNVRAM;
  CONST CHAR8      *AsciiGuid;
  CONST CHAR8      *AsciiNVRAMKey;

  ErrorCount = 0;
  UserNVRAM  = &Config->Nvram;

  for (GuidIndex = 0; GuidIndex < UserNVRAM->Legacy.Count; ++GuidIndex) {
    AsciiGuid = OC_BLOB_GET (UserNVRAM->Legacy.Keys[GuidIndex]);

    if (!AsciiGuidIsLegal (AsciiGuid)) {
      DEBUG ((DEBUG_WARN, "NVRAM->LegacySchema[%u] has borked GUID!\n", GuidIndex));
      ++ErrorCount;
    }

    for (VariableIndex = 0; VariableIndex < UserNVRAM->Legacy.Values[GuidIndex]->Count; ++VariableIndex) {
      AsciiNVRAMKey = OC_BLOB_GET (UserNVRAM->Legacy.Values[GuidIndex]->Values[VariableIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiPropertyIsLegal (AsciiNVRAMKey)) {
        DEBUG ((
          DEBUG_WARN,
          "NVRAM->LegacySchema[%u]->Key[%u] contains illegal character!\n",
          GuidIndex,
          VariableIndex
          ));
        ++ErrorCount;
      }
    }

    //
    // Check duplicated properties in NVRAM->LegacySchema.
    //
    ErrorCount += FindArrayDuplication (
      UserNVRAM->Legacy.Values[GuidIndex]->Values,
      UserNVRAM->Legacy.Values[GuidIndex]->Count,
      sizeof (UserNVRAM->Legacy.Values[GuidIndex]->Values[0]),
      NVRAMLegacySchemaHasDuplication
      );
  }

  //
  // Check duplicated entries in NVRAM->LegacySchema.
  //
  ErrorCount += FindArrayDuplication (
    UserNVRAM->Legacy.Keys,
    UserNVRAM->Legacy.Count,
    sizeof (UserNVRAM->Legacy.Keys[0]),
    NVRAMLegacySchemaHasDuplication
    );

  return ErrorCount;
}


UINT32
CheckNVRAM (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32  ErrorCount;
  UINTN   Index;
  STATIC CONFIG_CHECK NVRAMCheckers[] = {
    &CheckNVRAMAdd,
    &CheckNVRAMDelete,
    &CheckNVRAMSchema
  };

  DEBUG ((DEBUG_VERBOSE, "config loaded into %a!\n", __func__));

  ErrorCount = 0;

  for (Index = 0; Index < ARRAY_SIZE (NVRAMCheckers); ++Index) {
    ErrorCount += NVRAMCheckers[Index] (Config);
  }

  return ReportError (__func__, ErrorCount);
}
