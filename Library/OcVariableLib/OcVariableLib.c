/** @file
  OpenCore Variable library.

  Copyright (c) 2021, Marvin Häuser. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Guid/OcVariable.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC BOOLEAN mForceOcWriteFlash = FALSE;

STATIC BOOLEAN mDebugInitialized = FALSE;

VOID
OcVariableInit (
  IN BOOLEAN  ForceOcWriteFlash
  )
{
  mForceOcWriteFlash = ForceOcWriteFlash;

  DEBUG_CODE_BEGIN ();
  mDebugInitialized = TRUE;
  DEBUG_CODE_END ();
}

EFI_STATUS
OcSetSystemVariable (
  IN CHAR16    *VariableName,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN EFI_GUID  *VendorGuid  OPTIONAL
  )
{
  EFI_STATUS Status;
  INTN       CmpResult;

  UINTN      OldDataSize;
  VOID       *OldData;

  UINT8      StackBuffer[256];

  if (VendorGuid == NULL) {
    VendorGuid = &gOcVendorVariableGuid;
  }

  DEBUG_CODE_BEGIN ();
  ASSERT (mDebugInitialized);
  DEBUG_CODE_END ();
  //
  // All OC system variables are volatile as of now. Below logic must be adapted
  // in case non-volatile requests are to be supported.
  //
  ASSERT ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0);
  //
  // If the data to be written is non-volatile, check first that the existing
  // data is different. Handle errors tolerantly as this is only a shortcut.
  //
  if (mForceOcWriteFlash) {
    //
    // Try to read the old data to the stack first to save allocations.
    //
    OldDataSize = sizeof (StackBuffer);
    OldData     = StackBuffer;

    Status = gRT->GetVariable (
      VariableName,
      VendorGuid,
      NULL,
      &OldDataSize,
      OldData
      );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // If the stack buffer is too small, allocate dynamically and retry.
      //
      OldData = AllocatePool (OldDataSize);
      if (OldData != NULL) {
        Status = gRT->GetVariable (
          VariableName,
          VendorGuid,
          NULL,
          &OldDataSize,
          OldData
          );
        if (EFI_ERROR (Status)) {
          FreePool (OldData);
        }
      }
    }

    if (!EFI_ERROR (Status)) {
      //
      // Status may only be a success value if the allocation did not fail.
      //
      ASSERT (OldData != NULL);
      //
      // Compare the current variable data to the new data to be written.
      //
      CmpResult = 1;
      if (OldDataSize == DataSize) {
        CmpResult = CompareMem (OldData, Data, DataSize);
      }

      if (OldData != StackBuffer) {
        FreePool (OldData);
      }
      //
      // If the old data is equal to the new data, skip the write.
      //
      if (CmpResult == 0) {
        return EFI_SUCCESS;
      }
    }
    //
    // Force-write the OC system variable.
    //
    Attributes |= EFI_VARIABLE_NON_VOLATILE;
  }

  return gRT->SetVariable (
    VariableName,
    VendorGuid,
    Attributes,
    DataSize,
    Data
    );
}
