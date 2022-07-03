/** @file
  Measure TCG required variable.

Copyright (c) 2013 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/ImageAuthentication.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/TpmMeasurementLib.h>

#include "PrivilegePolymorphic.h"

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
} VARIABLE_TYPE;

VARIABLE_TYPE  mVariableType[] = {
  { EFI_SECURE_BOOT_MODE_NAME,    &gEfiGlobalVariableGuid        },
  { EFI_PLATFORM_KEY_NAME,        &gEfiGlobalVariableGuid        },
  { EFI_KEY_EXCHANGE_KEY_NAME,    &gEfiGlobalVariableGuid        },
  { EFI_IMAGE_SECURITY_DATABASE,  &gEfiImageSecurityDatabaseGuid },
  { EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid },
  { EFI_IMAGE_SECURITY_DATABASE2, &gEfiImageSecurityDatabaseGuid },
};

//
// "SecureBoot" may update following PK Del/Add
//  Cache its value to detect value update
//
UINT8  *mSecureBootVarData    = NULL;
UINTN  mSecureBootVarDataSize = 0;

/**
  This function will return if this variable is SecureBootPolicy Variable.

  @param[in]  VariableName      A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.

  @retval TRUE  This is SecureBootPolicy Variable
  @retval FALSE This is not SecureBootPolicy Variable
**/
BOOLEAN
IsSecureBootPolicyVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mVariableType)/sizeof (mVariableType[0]); Index++) {
    if ((StrCmp (VariableName, mVariableType[Index].VariableName) == 0) &&
        (CompareGuid (VendorGuid, mVariableType[Index].VendorGuid)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Measure and log an EFI variable, and extend the measurement result into a specific PCR.

  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureVariable (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  IN      VOID      *VarData,
  IN      UINTN     VarSize
  )
{
  EFI_STATUS          Status;
  UINTN               VarNameLength;
  UEFI_VARIABLE_DATA  *VarLog;
  UINT32              VarLogSize;

  ASSERT ((VarSize == 0 && VarData == NULL) || (VarSize != 0 && VarData != NULL));

  VarNameLength = StrLen (VarName);
  VarLogSize    = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                           - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (UEFI_VARIABLE_DATA *)AllocateZeroPool (VarLogSize);
  if (VarLog == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&VarLog->VariableName, VendorGuid, sizeof (VarLog->VariableName));
  VarLog->UnicodeNameLength  = VarNameLength;
  VarLog->VariableDataLength = VarSize;
  CopyMem (
    VarLog->UnicodeName,
    VarName,
    VarNameLength * sizeof (*VarName)
    );
  if (VarSize != 0) {
    CopyMem (
      (CHAR16 *)VarLog->UnicodeName + VarNameLength,
      VarData,
      VarSize
      );
  }

  DEBUG ((DEBUG_INFO, "VariableDxe: MeasureVariable (Pcr - %x, EventType - %x, ", (UINTN)7, (UINTN)EV_EFI_VARIABLE_DRIVER_CONFIG));
  DEBUG ((DEBUG_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  Status = TpmMeasureAndLogData (
             7,
             EV_EFI_VARIABLE_DRIVER_CONFIG,
             VarLog,
             VarLogSize,
             VarLog,
             VarLogSize
             );
  FreePool (VarLog);
  return Status;
}

/**
  Returns the status whether get the variable success. The function retrieves
  variable  through the UEFI Runtime Service GetVariable().  The
  returned buffer is allocated using AllocatePool().  The caller is responsible
  for freeing this buffer with FreePool().

  This API is only invoked in boot time. It may NOT be invoked at runtime.

  @param[in]  Name  The pointer to a Null-terminated Unicode string.
  @param[in]  Guid  The pointer to an EFI_GUID structure
  @param[out] Value The buffer point saved the variable info.
  @param[out] Size  The buffer size of the variable.

  @return EFI_OUT_OF_RESOURCES      Allocate buffer failed.
  @return EFI_SUCCESS               Find the specified variable.
  @return Others Errors             Return errors from call to gRT->GetVariable.

**/
EFI_STATUS
InternalGetVariable (
  IN CONST CHAR16    *Name,
  IN CONST EFI_GUID  *Guid,
  OUT VOID           **Value,
  OUT UINTN          *Size
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  //
  // Try to get the variable size.
  //
  BufferSize = 0;
  *Value     = NULL;
  if (Size != NULL) {
    *Size = 0;
  }

  Status = gRT->GetVariable ((CHAR16 *)Name, (EFI_GUID *)Guid, NULL, &BufferSize, *Value);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Allocate buffer to get the variable.
  //
  *Value = AllocatePool (BufferSize);
  ASSERT (*Value != NULL);
  if (*Value == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get the variable data.
  //
  Status = gRT->GetVariable ((CHAR16 *)Name, (EFI_GUID *)Guid, NULL, &BufferSize, *Value);
  if (EFI_ERROR (Status)) {
    FreePool (*Value);
    *Value = NULL;
  }

  if (Size != NULL) {
    *Size = BufferSize;
  }

  return Status;
}

/**
  SecureBoot Hook for SetVariable.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.

**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS  Status;
  UINTN       VariableDataSize;
  VOID        *VariableData;

  if (!IsSecureBootPolicyVariable (VariableName, VendorGuid)) {
    return;
  }

  //
  // We should NOT use Data and DataSize here,because it may include signature,
  // or is just partial with append attributes, or is deleted.
  // We should GetVariable again, to get full variable content.
  //
  Status = InternalGetVariable (
             VariableName,
             VendorGuid,
             &VariableData,
             &VariableDataSize
             );
  if (EFI_ERROR (Status)) {
    //
    // Measure DBT only if present and not empty
    //
    if ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE2) == 0) &&
        CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid))
    {
      DEBUG ((DEBUG_INFO, "Skip measuring variable %s since it's deleted\n", EFI_IMAGE_SECURITY_DATABASE2));
      return;
    } else {
      VariableData     = NULL;
      VariableDataSize = 0;
    }
  }

  Status = MeasureVariable (
             VariableName,
             VendorGuid,
             VariableData,
             VariableDataSize
             );
  DEBUG ((DEBUG_INFO, "MeasureBootPolicyVariable - %r\n", Status));

  if (VariableData != NULL) {
    FreePool (VariableData);
  }

  //
  // "SecureBoot" is 8bit & read-only. It can only be changed according to PK update
  //
  if ((StrCmp (VariableName, EFI_PLATFORM_KEY_NAME) == 0) &&
      CompareGuid (VendorGuid, &gEfiGlobalVariableGuid))
  {
    Status = InternalGetVariable (
               EFI_SECURE_BOOT_MODE_NAME,
               &gEfiGlobalVariableGuid,
               &VariableData,
               &VariableDataSize
               );
    if (EFI_ERROR (Status)) {
      return;
    }

    //
    // If PK update is successful. "SecureBoot" shall always exist ever since variable write service is ready
    //
    ASSERT (mSecureBootVarData != NULL);

    if (CompareMem (mSecureBootVarData, VariableData, VariableDataSize) != 0) {
      FreePool (mSecureBootVarData);
      mSecureBootVarData     = VariableData;
      mSecureBootVarDataSize = VariableDataSize;

      DEBUG ((DEBUG_INFO, "%s variable updated according to PK change. Remeasure the value!\n", EFI_SECURE_BOOT_MODE_NAME));
      Status = MeasureVariable (
                 EFI_SECURE_BOOT_MODE_NAME,
                 &gEfiGlobalVariableGuid,
                 mSecureBootVarData,
                 mSecureBootVarDataSize
                 );
      DEBUG ((DEBUG_INFO, "MeasureBootPolicyVariable - %r\n", Status));
    } else {
      //
      // "SecureBoot" variable is not changed
      //
      FreePool (VariableData);
    }
  }

  return;
}

/**
  Some Secure Boot Policy Variable may update following other variable changes(SecureBoot follows PK change, etc).
  Record their initial State when variable write service is ready.

**/
VOID
EFIAPI
RecordSecureBootPolicyVarData (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Record initial "SecureBoot" variable value.
  // It is used to detect SecureBoot variable change in SecureBootHook.
  //
  Status = InternalGetVariable (
             EFI_SECURE_BOOT_MODE_NAME,
             &gEfiGlobalVariableGuid,
             (VOID **)&mSecureBootVarData,
             &mSecureBootVarDataSize
             );
  if (EFI_ERROR (Status)) {
    //
    // Read could fail when Auth Variable solution is not supported
    //
    DEBUG ((DEBUG_INFO, "RecordSecureBootPolicyVarData GetVariable %s Status %x\n", EFI_SECURE_BOOT_MODE_NAME, Status));
  }
}
