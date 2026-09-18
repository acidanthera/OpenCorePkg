/** @file
  Temporary location of the RequestToLock shim code while projects
  are moved to VariablePolicy. Should be removed when deprecated.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/VariablePolicyLib.h>
#include <Library/VariablePolicyHelperLib.h>
#include <Protocol/VariableLock.h>

/**
  DEPRECATED. THIS IS ONLY HERE AS A CONVENIENCE WHILE PORTING.
  Mark a variable that will become read-only after leaving the DXE phase of
  execution. Write request coming from SMM environment through
  EFI_SMM_VARIABLE_PROTOCOL is allowed.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made
                           read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made
                           read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and
                                the VendorGuid was marked as pending to be
                                read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or
                                EFI_EVENT_GROUP_READY_TO_BOOT has already been
                                signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock
                                request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL  *This,
  IN CHAR16                              *VariableName,
  IN EFI_GUID                            *VendorGuid
  )
{
  EFI_STATUS             Status;
  VARIABLE_POLICY_ENTRY  *NewPolicy;

  DEBUG ((DEBUG_WARN, "!!! DEPRECATED INTERFACE !!! %a() will go away soon!\n", __func__));
  DEBUG ((DEBUG_WARN, "!!! DEPRECATED INTERFACE !!! Please move to use Variable Policy!\n"));
  DEBUG ((DEBUG_WARN, "!!! DEPRECATED INTERFACE !!! Variable: %g %s\n", VendorGuid, VariableName));

  NewPolicy = NULL;
  Status    = CreateBasicVariablePolicy (
                VendorGuid,
                VariableName,
                VARIABLE_POLICY_NO_MIN_SIZE,
                VARIABLE_POLICY_NO_MAX_SIZE,
                VARIABLE_POLICY_NO_MUST_ATTR,
                VARIABLE_POLICY_NO_CANT_ATTR,
                VARIABLE_POLICY_TYPE_LOCK_NOW,
                &NewPolicy
                );
  if (!EFI_ERROR (Status)) {
    Status = RegisterVariablePolicy (NewPolicy);

    //
    // If the error returned is EFI_ALREADY_STARTED, we need to check the
    // current database for the variable and see whether it's locked. If it's
    // locked, we're still fine, but also generate a DEBUG_WARN message so the
    // duplicate lock can be removed.
    //
    if (Status == EFI_ALREADY_STARTED) {
      Status = ValidateSetVariable (VariableName, VendorGuid, 0, 0, NULL);
      if (Status == EFI_WRITE_PROTECTED) {
        DEBUG ((DEBUG_WARN, "  Variable: %g %s is already locked!\n", VendorGuid, VariableName));
        Status = EFI_SUCCESS;
      } else {
        DEBUG ((DEBUG_ERROR, "  Variable: %g %s can not be locked!\n", VendorGuid, VariableName));
        Status = EFI_ACCESS_DENIED;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to lock variable %s! %r\n", __func__, VariableName, Status));
  }

  if (NewPolicy != NULL) {
    FreePool (NewPolicy);
  }

  return Status;
}
