/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Library/DebugLib.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <Library/OcStringLib.h>

#include <Guid/GlobalVariable.h>

/** Log the boot options passed

  @param[in] BootOrder        A pointer to the boot order list.
  @param[in] BootOrderSize    Size of the boot order list.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
LogBootOrder (
  IN  INT16   *BootOrder,
  IN  UINTN   BootOrderSize
  )
{
  EFI_STATUS  Status;
  CHAR8       *BootOrderString;
  INT16       *BootOrderBuffer;
  UINTN       BootOrderCount;
  UINTN       Index;

  BootOrderBuffer = NULL;

  if (BootOrder == NULL && BootOrderSize == 0) {

    Status = GetVariable2 (
               L"BootOrder",
               &gEfiGlobalVariableGuid,
               (VOID **)&BootOrderBuffer,
               &BootOrderSize
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (BootOrder == NULL || BootOrderSize == 0) {
      return EFI_NOT_FOUND;
    }

    BootOrder = BootOrderBuffer;

  } else if (BootOrder == NULL || BootOrderSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_OUT_OF_RESOURCES;

  BootOrderCount  = (UINTN)DivU64x32 (BootOrderSize, sizeof (INT16));

  BootOrderString = AllocateZeroPool (BootOrderCount * 4);

  if (BootOrderString != NULL) {

    Status = EFI_SUCCESS;

    // Create hex ascii string representation of BootOrder variable
    for (Index = 0; Index < BootOrderCount; Index++) {

      AsciiSPrint (
        &BootOrderString[Index * 3],
        4,
        "%02X ",
        BootOrder[Index]);
    }
  
    // Remove any trailing white space
    *(BootOrderString + Index * 3 - sizeof (CHAR8)) = '\0';

    // Log BootOrder variable string
    DEBUG ((DEBUG_INFO, "%a %a\n", "BootOrder", BootOrderString));

    FreePool (BootOrderString);

  }

  if (BootOrderBuffer != NULL) {
    FreePool (BootOrderString);
  }

  return Status;
}
