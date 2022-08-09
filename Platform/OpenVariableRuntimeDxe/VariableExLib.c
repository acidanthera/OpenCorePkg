/** @file
  Provides variable driver extended services.

Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableParsing.h"

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the variable found.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
EFIAPI
VariableExLibFindVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  EFI_STATUS                     Status;
  VARIABLE_POINTER_TRACK         Variable;
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  Status = FindVariable (
             VariableName,
             VendorGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    AuthVariableInfo->Data           = NULL;
    AuthVariableInfo->DataSize       = 0;
    AuthVariableInfo->Attributes     = 0;
    AuthVariableInfo->PubKeyIndex    = 0;
    AuthVariableInfo->MonotonicCount = 0;
    AuthVariableInfo->TimeStamp      = NULL;
    return Status;
  }

  AuthVariableInfo->DataSize   = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Data       = GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Attributes = Variable.CurrPtr->Attributes;
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    AuthVariable                     = (AUTHENTICATED_VARIABLE_HEADER *)Variable.CurrPtr;
    AuthVariableInfo->PubKeyIndex    = AuthVariable->PubKeyIndex;
    AuthVariableInfo->MonotonicCount = ReadUnaligned64 (&(AuthVariable->MonotonicCount));
    AuthVariableInfo->TimeStamp      = &AuthVariable->TimeStamp;
  }

  return EFI_SUCCESS;
}

/**
  Finds next variable in storage blocks of volatile and non-volatile storage areas.

  This code finds next variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the next variable.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
EFIAPI
VariableExLibFindNextVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  EFI_STATUS                     Status;
  VARIABLE_HEADER                *VariablePtr;
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariablePtr;
  VARIABLE_STORE_HEADER          *VariableStoreHeader[VariableStoreTypeMax];

  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  Status = VariableServiceGetNextVariableInternal (
             VariableName,
             VendorGuid,
             VariableStoreHeader,
             &VariablePtr,
             mVariableModuleGlobal->VariableGlobal.AuthFormat
             );
  if (EFI_ERROR (Status)) {
    AuthVariableInfo->VariableName   = NULL;
    AuthVariableInfo->VendorGuid     = NULL;
    AuthVariableInfo->Data           = NULL;
    AuthVariableInfo->DataSize       = 0;
    AuthVariableInfo->Attributes     = 0;
    AuthVariableInfo->PubKeyIndex    = 0;
    AuthVariableInfo->MonotonicCount = 0;
    AuthVariableInfo->TimeStamp      = NULL;
    return Status;
  }

  AuthVariableInfo->VariableName = GetVariableNamePtr (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->VendorGuid   = GetVendorGuidPtr (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->DataSize     = DataSizeOfVariable (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Data         = GetVariableDataPtr (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Attributes   = VariablePtr->Attributes;
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    AuthVariablePtr                  = (AUTHENTICATED_VARIABLE_HEADER *)VariablePtr;
    AuthVariableInfo->PubKeyIndex    = AuthVariablePtr->PubKeyIndex;
    AuthVariableInfo->MonotonicCount = ReadUnaligned64 (&(AuthVariablePtr->MonotonicCount));
    AuthVariableInfo->TimeStamp      = &AuthVariablePtr->TimeStamp;
  }

  return EFI_SUCCESS;
}

/**
  Update the variable region with Variable information.

  @param[in] AuthVariableInfo       Pointer AUTH_VARIABLE_INFO structure for
                                    input of the variable.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
EFI_STATUS
EFIAPI
VariableExLibUpdateVariable (
  IN AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  VARIABLE_POINTER_TRACK  Variable;

  FindVariable (AuthVariableInfo->VariableName, AuthVariableInfo->VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  return UpdateVariable (
           AuthVariableInfo->VariableName,
           AuthVariableInfo->VendorGuid,
           AuthVariableInfo->Data,
           AuthVariableInfo->DataSize,
           AuthVariableInfo->Attributes,
           AuthVariableInfo->PubKeyIndex,
           AuthVariableInfo->MonotonicCount,
           &Variable,
           AuthVariableInfo->TimeStamp
           );
}

/**
  Get scratch buffer.

  @param[in, out] ScratchBufferSize Scratch buffer size. If input size is greater than
                                    the maximum supported buffer size, this value contains
                                    the maximum supported buffer size as output.
  @param[out]     ScratchBuffer     Pointer to scratch buffer address.

  @retval EFI_SUCCESS       Get scratch buffer successfully.
  @retval EFI_UNSUPPORTED   If input size is greater than the maximum supported buffer size.

**/
EFI_STATUS
EFIAPI
VariableExLibGetScratchBuffer (
  IN OUT UINTN  *ScratchBufferSize,
  OUT    VOID   **ScratchBuffer
  )
{
  UINTN  MaxBufferSize;

  MaxBufferSize = mVariableModuleGlobal->ScratchBufferSize;
  if (*ScratchBufferSize > MaxBufferSize) {
    *ScratchBufferSize = MaxBufferSize;
    return EFI_UNSUPPORTED;
  }

  *ScratchBuffer = GetEndPointer ((VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase));
  return EFI_SUCCESS;
}

/**
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param ...                    The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
BOOLEAN
EFIAPI
VariableExLibCheckRemainingSpaceForConsistency (
  IN UINT32  Attributes,
  ...
  )
{
  VA_LIST  Marker;
  BOOLEAN  Return;

  VA_START (Marker, Attributes);

  Return = CheckRemainingSpaceForConsistencyInternal (Attributes, Marker);

  VA_END (Marker);

  return Return;
}

/**
  Return TRUE if at OS runtime.

  @retval TRUE If at OS runtime.
  @retval FALSE If at boot time.

**/
BOOLEAN
EFIAPI
VariableExLibAtRuntime (
  VOID
  )
{
  return AtRuntime ();
}
