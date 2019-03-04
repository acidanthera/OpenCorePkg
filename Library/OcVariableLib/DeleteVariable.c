/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

// DeleteVariables
/** Deletes all EFI Variables matching the pattern.

  @param[in] VendorGuid  A unique identifier for the vendor.
  @param[in] SearchName  A Null-terminated Unicode string that is the name of the vendor’s variable. Each SearchName is
                         unique for each VendorGuid.
                         SearchName must contain 1 or more Unicode characters. If SearchName is an empty Unicode
                         string, then EFI_INVALID_PARAMETER is returned.

  @retval EFI_SUCCESS  The variable was deleted successfully.
**/
EFI_STATUS
DeleteVariables (
  IN EFI_GUID  *VendorGuid,
  IN CHAR16    *SearchName
  )
{
  EFI_STATUS Status;

  EFI_GUID   VariableGuid;
  UINTN      VariableNameBufferSize;
  UINTN      VariableNameSize;
  CHAR16     *VariableName;

  Status = EFI_INVALID_PARAMETER;

  if ((VendorGuid != NULL) && (SearchName != NULL)) {

    // Start with a minimal buffer to find the name of the first entry
    VariableNameBufferSize = sizeof (*VariableName);
    VariableName = AllocateZeroPool (VariableNameBufferSize);

    DEBUG ((
      DEBUG_VARIABLE, 
      "Delete Guid %g : Search String \'%s\'\n",
      VendorGuid,
      SearchName
      ));

    while (VariableName != NULL) {
      VariableNameSize = VariableNameBufferSize;
      Status           = gRT->GetNextVariableName (
                                &VariableNameSize,
                                VariableName,
                                &VariableGuid
                                );

      if (Status == EFI_BUFFER_TOO_SMALL) {
        VariableName = ReallocatePool (
                         VariableNameBufferSize,
                         VariableNameSize,
                         VariableName
                         );

        if (VariableName == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        VariableNameBufferSize = VariableNameSize;
        Status                 = gRT->GetNextVariableName (
                                        &VariableNameSize,
                                        VariableName,
                                        &VariableGuid
                                        );
      }

      if (EFI_ERROR (Status)) {
        if (Status == EFI_NOT_FOUND) {
          Status = EFI_SUCCESS;
        }

        break;
      }

      // If provided are we looking for this Guid ?, otherwise continue
      if (VendorGuid != NULL && (!CompareGuid (&VariableGuid, VendorGuid))) {
        continue;
      }

      // If provided does the name contain our search string ?, otherwise continue
      if (SearchName != NULL && (!StrStr (VariableName, SearchName))) {
        continue;
      }

      // FIXME: Skip gOpenCoreOverridesGuid:BootLog to avoid multiple hits

      Status = gRT->SetVariable (
                      VariableName,
                      &VariableGuid,
                      0,
                      0,
                      NULL
                      );

      DEBUG ((DEBUG_VARIABLE, "Delete %g:%s - %r\n", &VariableGuid, VariableName, Status));

    }

    if (VariableName != NULL) {   
      FreePool ((VOID *)VariableName);
    }

  }

  return Status;
}
