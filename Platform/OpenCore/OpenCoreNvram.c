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

#include <Guid/OcVariables.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Force the assertions in case we forget about them.
//
OC_GLOBAL_STATIC_ASSERT (
  L_STR_LEN (OPEN_CORE_VERSION) == 5,
  "OPEN_CORE_VERSION must follow X.Y.Z format, where X.Y.Z are single digits."
  );

OC_GLOBAL_STATIC_ASSERT (
  L_STR_LEN (OPEN_CORE_TARGET) == 3,
  "OPEN_CORE_TARGET must XYZ format, where XYZ is build target."
  );

STATIC CHAR8 mOpenCoreVersion[] = {
  /* [0]  = */ OPEN_CORE_TARGET[0],
  /* [1]  = */ OPEN_CORE_TARGET[1],
  /* [2]  = */ OPEN_CORE_TARGET[2],
  /* [3]  = */ '-',
  /* [4]  = */ OPEN_CORE_VERSION[0],
  /* [5]  = */ OPEN_CORE_VERSION[2],
  /* [6]  = */ OPEN_CORE_VERSION[4],
  /* [7]  = */ '-',
  /* [8]  = */ __DATE__[7],
  /* [9]  = */ __DATE__[8],
  /* [10] = */ __DATE__[9],
  /* [11] = */ __DATE__[10],
  /* [12] = */ '-',
  /* [13] = */ 'M',
  /* [14] = */ 'M',
  /* [15] = */ '-',
  /* [16] = */ 'D',
  /* [17] = */ 'D',
  /* [18] = */ '\0'
};

STATIC
VOID
OcReportVersion (
  VOID
  )
{
  UINT32  Month;

  Month =
    (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ?  1 :
    (__DATE__[0] == 'F' && __DATE__[2] == 'e' && __DATE__[2] == 'b') ?  2 :
    (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ?  3 :
    (__DATE__[0] == 'A' && __DATE__[1] == 'p' && __DATE__[2] == 'r') ?  4 :
    (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ?  5 :
    (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ?  6 :
    (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ?  7 :
    (__DATE__[0] == 'A' && __DATE__[1] == 'u' && __DATE__[2] == 'g') ?  8 :
    (__DATE__[0] == 'S' && __DATE__[1] == 'e' && __DATE__[2] == 'p') ?  9 :
    (__DATE__[0] == 'O' && __DATE__[1] == 'c' && __DATE__[2] == 't') ? 10 :
    (__DATE__[0] == 'N' && __DATE__[1] == 'o' && __DATE__[2] == 'v') ? 11 :
    (__DATE__[0] == 'D' && __DATE__[1] == 'e' && __DATE__[2] == 'c') ? 12 : 0;

  mOpenCoreVersion[13] = Month < 10 ? '0' : '1';
  mOpenCoreVersion[14] = '0' + (Month % 10);
  mOpenCoreVersion[16] = __DATE__[4] >= '0' ? __DATE__[4] : '0';
  mOpenCoreVersion[17] = __DATE__[5];

  DEBUG ((DEBUG_INFO, "OC: Current version is %a\n", mOpenCoreVersion));

  gRT->SetVariable (
    OC_VERSION_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    OPEN_CORE_NVRAM_ATTR,
    sizeof (mOpenCoreVersion),
    &mOpenCoreVersion[0]
    );
}

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
        AsciiVariableName,
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
        AsciiVariableName,
        Status
        ));

      if (Status != EFI_BUFFER_TOO_SMALL) {
        Status = gRT->SetVariable (
          UnicodeVariableName,
          &VariableGuid,
          OPEN_CORE_NVRAM_ATTR,
          VariableSize,
          VariableData
          );
        DEBUG ((
          EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
          "OC: Setting NVRAM %g:%a - %r\n",
          &VariableGuid,
          AsciiVariableName,
          Status
          ));
      }

      FreePool (UnicodeVariableName);
    }
  }

  OcReportVersion ();
}
