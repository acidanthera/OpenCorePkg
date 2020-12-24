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

UINT32
CheckNVRAM (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32           ErrorCount;
  EFI_STATUS       Status;
  UINT32           GuidIndex;
  UINT32           VariableIndex;
  OC_NVRAM_CONFIG  *UserNVRAM;
  CONST CHAR8      *AsciiGuid;
  CONST CHAR8      *AsciiNVRAMKey;
  GUID             VariableGuid;
  OC_ASSOC         *VariableMap;

  DEBUG ((DEBUG_VERBOSE, "config loaded into NVRAM checker!\n"));

  ErrorCount = 0;
  UserNVRAM  = &Config->Nvram;

  for (GuidIndex = 0; GuidIndex < UserNVRAM->Add.Count; ++GuidIndex) {
    AsciiGuid = OC_BLOB_GET (UserNVRAM->Add.Keys[GuidIndex]);
    Status    = AsciiStrToGuid (AsciiGuid, &VariableGuid);

    if (EFI_ERROR (Status)) {
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
  }

  for (GuidIndex = 0; GuidIndex < UserNVRAM->Delete.Count; ++GuidIndex) {
    AsciiGuid = OC_BLOB_GET (UserNVRAM->Delete.Keys[GuidIndex]);
    Status    = AsciiStrToGuid (AsciiGuid, &VariableGuid);

    if (EFI_ERROR (Status)) {
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
  }

  for (GuidIndex = 0; GuidIndex < UserNVRAM->Legacy.Count; ++GuidIndex) {
    AsciiGuid = OC_BLOB_GET (UserNVRAM->Legacy.Keys[GuidIndex]);
    Status    = AsciiStrToGuid (AsciiGuid, &VariableGuid);

    if (EFI_ERROR (Status)) {
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
  }

  return ReportError (__func__, ErrorCount);
}
