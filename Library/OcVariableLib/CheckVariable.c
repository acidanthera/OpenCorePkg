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

// CheckVariableBoolean
/** Check the boolean-value of an EFI Variable.

  @param[in] Name        The firmware allocated handle for the EFI image.
  @param[in] VendorGuid  A unique identifier for the vendor.

  @retval  Boolean value indicating TRUE, if variable exists.
**/
BOOLEAN
CheckVariableBoolean (
  IN CHAR16    *Name,
  IN EFI_GUID  *VendorGuid
  )
{
  BOOLEAN    Value;

  EFI_STATUS Status;
  UINTN      VariableSize;

  VariableSize = sizeof (Value);
  Status       = gRT->GetVariable (
                        Name,
                        VendorGuid,
                        NULL,
                        &VariableSize,
                        &Value
                        );

  if (EFI_ERROR (Status)) {
    Value = FALSE;
  }

  return Value;
}
