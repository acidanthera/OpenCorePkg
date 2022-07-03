/** @file
  Polymorphic functions that are called from both the privileged driver (i.e.,
  the DXE_SMM variable module) and the non-privileged drivers (i.e., one or
  both of the DXE_RUNTIME variable modules).

  Each of these functions has two implementations, appropriate for privileged
  vs. non-privileged driver code.

  Copyright (c) 2017, Red Hat, Inc.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _PRIVILEGE_POLYMORPHIC_H_
#define _PRIVILEGE_POLYMORPHIC_H_

#include <Uefi/UefiBaseType.h>

/**
  SecureBoot Hook for auth variable update.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  );

/**
  Initialization for MOR Control Lock.

  @retval EFI_SUCCESS     MorLock initialization success.
  @return Others          Some error occurs.
**/
EFI_STATUS
MorLockInit (
  VOID
  );

/**
  Delayed initialization for MOR Control Lock at EndOfDxe.

  This function performs any operations queued by MorLockInit().
**/
VOID
MorLockInitAtEndOfDxe (
  VOID
  );

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
  );

/**
  This service is consumed by the variable modules to place a barrier to stop
  speculative execution.

  Ensures that no later instruction will execute speculatively, until all prior
  instructions have completed.

**/
VOID
VariableSpeculationBarrier (
  VOID
  );

/**
  Notify the system that the SMM variable driver is ready.
**/
VOID
VariableNotifySmmReady (
  VOID
  );

/**
  Notify the system that the SMM variable write driver is ready.
**/
VOID
VariableNotifySmmWriteReady (
  VOID
  );

/**
  Variable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols
  for variable read and write services being available. It also registers
  a notification function for an EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @retval EFI_SUCCESS       Variable service successfully initialized.
**/
EFI_STATUS
EFIAPI
MmVariableServiceInitialize (
  VOID
  );

/**
  This function checks if the buffer is valid per processor architecture and
  does not overlap with SMRAM.

  @param Buffer The buffer start address to be checked.
  @param Length The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and does not
                overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlaps
                with SMRAM.
**/
BOOLEAN
VariableSmmIsBufferOutsideSmmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

/**
  Whether the TCG or TCG2 protocols are installed in the UEFI protocol database.
  This information is used by the MorLock code to infer whether an existing
  MOR variable is legitimate or not.

  @retval TRUE  Either the TCG or TCG2 protocol is installed in the UEFI
                protocol database
  @retval FALSE Neither the TCG nor the TCG2 protocol is installed in the UEFI
                protocol database
**/
BOOLEAN
VariableHaveTcgProtocols (
  VOID
  );

#endif
