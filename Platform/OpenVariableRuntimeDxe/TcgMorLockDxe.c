/** @file
  TCG MOR (Memory Overwrite Request) Lock Control support (DXE version).

  This module clears MemoryOverwriteRequestControlLock variable to indicate
  MOR lock control unsupported.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/MemoryOverwriteControl.h>
#include <IndustryStandard/MemoryOverwriteRequestControlLock.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "Variable.h"

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyHelperLib.h>

/**
  This service is an MOR/MorLock checker handler for the SetVariable().

  @param[in]  VariableName the name of the vendor's variable, as a
                           Null-Terminated Unicode String
  @param[in]  VendorGuid   Unify identifier for vendor.
  @param[in]  Attributes   Attributes bitmask to set for the variable.
  @param[in]  DataSize     The size in bytes of Data-Buffer.
  @param[in]  Data         Point to the content of the variable.

  @retval  EFI_SUCCESS            The MOR/MorLock check pass, and Variable
                                  driver can store the variable data.
  @retval  EFI_INVALID_PARAMETER  The MOR/MorLock data or data size or
                                  attributes is not allowed for MOR variable.
  @retval  EFI_ACCESS_DENIED      The MOR/MorLock is locked.
  @retval  EFI_ALREADY_STARTED    The MorLock variable is handled inside this
                                  function. Variable driver can just return
                                  EFI_SUCCESS.
**/
EFI_STATUS
SetVariableCheckHandlerMor (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  //
  // Just let it pass. No need provide protection for DXE version.
  //
  return EFI_SUCCESS;
}

/**
  Initialization for MOR Control Lock.

  @retval EFI_SUCCESS     MorLock initialization success.
  @return Others          Some error occurs.
**/
EFI_STATUS
MorLockInit (
  VOID
  )
{
  //
  // Always clear variable to report unsupported to OS.
  // The reason is that the DXE version is not proper to provide *protection*.
  // BIOS should use SMM version variable driver to provide such capability.
  //
  VariableServiceSetVariable (
    MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
    &gEfiMemoryOverwriteRequestControlLockGuid,
    0,                                          // Attributes
    0,                                          // DataSize
    NULL                                        // Data
    );

  //
  // The MOR variable can effectively improve platform security only when the
  // MorLock variable protects the MOR variable. In turn MorLock cannot be made
  // secure without SMM support in the platform firmware (see above).
  //
  // Thus, delete the MOR variable, should it exist for any reason (some OSes
  // are known to create MOR unintentionally, in an attempt to set it), then
  // also lock the MOR variable, in order to prevent other modules from
  // creating it.
  //
  VariableServiceSetVariable (
    MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
    &gEfiMemoryOverwriteControlDataGuid,
    0,                                      // Attributes
    0,                                      // DataSize
    NULL                                    // Data
    );

  return EFI_SUCCESS;
}

/**
  Delayed initialization for MOR Control Lock at EndOfDxe.

  This function performs any operations queued by MorLockInit().
**/
VOID
MorLockInitAtEndOfDxe (
  VOID
  )
{
  EFI_STATUS                      Status;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  // First, we obviously need to locate the VariablePolicy protocol.
  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Could not locate VariablePolicy protocol! %r\n", __func__, Status));
    return;
  }

  // If we're successful, go ahead and set the policies to protect the target variables.
  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &gEfiMemoryOverwriteRequestControlLockGuid,
             MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
             VARIABLE_POLICY_NO_MIN_SIZE,
             VARIABLE_POLICY_NO_MAX_SIZE,
             VARIABLE_POLICY_NO_MUST_ATTR,
             VARIABLE_POLICY_NO_CANT_ATTR,
             VARIABLE_POLICY_TYPE_LOCK_NOW
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Could not lock variable %s! %r\n", __func__, MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME, Status));
  }

  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &gEfiMemoryOverwriteControlDataGuid,
             MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
             VARIABLE_POLICY_NO_MIN_SIZE,
             VARIABLE_POLICY_NO_MAX_SIZE,
             VARIABLE_POLICY_NO_MUST_ATTR,
             VARIABLE_POLICY_NO_CANT_ATTR,
             VARIABLE_POLICY_TYPE_LOCK_NOW
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Could not lock variable %s! %r\n", __func__, MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME, Status));
  }

  return;
}
