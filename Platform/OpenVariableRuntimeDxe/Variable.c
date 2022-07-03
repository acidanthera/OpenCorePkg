/** @file
  The common variable operation routines shared by DXE_RUNTIME variable
  module and DXE_SMM variable module.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. They may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  VariableServiceGetNextVariableName () and VariableServiceQueryVariableInfo() are external API.
  They need check input parameter.

  VariableServiceGetVariable() and VariableServiceSetVariable() are external API
  to receive datasize and data buffer. The size should be checked carefully.

  VariableServiceSetVariable() should also check authenticate data to avoid buffer overflow,
  integer overflow. It should also check attribute to avoid authentication bypass.

Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2018 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) Microsoft Corporation.<BR>
Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableNonVolatile.h"
#include "VariableParsing.h"
#include "VariableRuntimeCache.h"

VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;

///
/// Define a memory cache that improves the search performance for a variable.
/// For EmuNvMode == TRUE, it will be equal to NonVolatileVariableBase.
///
VARIABLE_STORE_HEADER  *mNvVariableCache = NULL;

///
/// Memory cache of Fv Header.
///
EFI_FIRMWARE_VOLUME_HEADER  *mNvFvHeaderCache = NULL;

///
/// The memory entry used for variable statistics data.
///
VARIABLE_INFO_ENTRY  *gVariableInfo = NULL;

///
/// The flag to indicate whether the platform has left the DXE phase of execution.
///
BOOLEAN  mEndOfDxe = FALSE;

///
/// It indicates the var check request source.
/// In the implementation, DXE is regarded as untrusted, and SMM is trusted.
///
VAR_CHECK_REQUEST_SOURCE  mRequestSource = VarCheckFromUntrusted;

//
// It will record the current boot error flag before EndOfDxe.
//
VAR_ERROR_FLAG  mCurrentBootVarErrFlag = VAR_ERROR_FLAG_NO_ERROR;

VARIABLE_ENTRY_PROPERTY  mVariableEntryProperty[] = {
  {
    &gEdkiiVarErrorFlagGuid,
    VAR_ERROR_FLAG_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (VAR_ERROR_FLAG),
      sizeof (VAR_ERROR_FLAG)
    }
  },
};

AUTH_VAR_LIB_CONTEXT_IN  mAuthContextIn = {
  AUTH_VAR_LIB_CONTEXT_IN_STRUCT_VERSION,
  //
  // StructSize, TO BE FILLED
  //
  0,
  //
  // MaxAuthVariableSize, TO BE FILLED
  //
  0,
  VariableExLibFindVariable,
  VariableExLibFindNextVariable,
  VariableExLibUpdateVariable,
  VariableExLibGetScratchBuffer,
  VariableExLibCheckRemainingSpaceForConsistency,
  VariableExLibAtRuntime,
};

AUTH_VAR_LIB_CONTEXT_OUT  mAuthContextOut;

/**

  This function writes data to the FWH at the correct LBA even if the LBAs
  are fragmented.

  @param Global                  Pointer to VARAIBLE_GLOBAL structure.
  @param Volatile                Point out the Variable is Volatile or Non-Volatile.
  @param SetByIndex              TRUE if target pointer is given as index.
                                 FALSE if target pointer is absolute.
  @param Fvb                     Pointer to the writable FVB protocol.
  @param DataPtrIndex            Pointer to the Data from the end of VARIABLE_STORE_HEADER
                                 structure.
  @param DataSize                Size of data to be written.
  @param Buffer                  Pointer to the buffer from which data is written.

  @retval EFI_INVALID_PARAMETER  Parameters not valid.
  @retval EFI_UNSUPPORTED        Fvb is a NULL for Non-Volatile variable update.
  @retval EFI_OUT_OF_RESOURCES   The remaining size is not enough.
  @retval EFI_SUCCESS            Variable store successfully updated.

**/
EFI_STATUS
UpdateVariableStore (
  IN  VARIABLE_GLOBAL                     *Global,
  IN  BOOLEAN                             Volatile,
  IN  BOOLEAN                             SetByIndex,
  IN  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  IN  UINTN                               DataPtrIndex,
  IN  UINT32                              DataSize,
  IN  UINT8                               *Buffer
  )
{
  EFI_FV_BLOCK_MAP_ENTRY  *PtrBlockMapEntry;
  UINTN                   BlockIndex2;
  UINTN                   LinearOffset;
  UINTN                   CurrWriteSize;
  UINTN                   CurrWritePtr;
  UINT8                   *CurrBuffer;
  EFI_LBA                 LbaNumber;
  UINTN                   Size;
  VARIABLE_STORE_HEADER   *VolatileBase;
  EFI_PHYSICAL_ADDRESS    FvVolHdr;
  EFI_PHYSICAL_ADDRESS    DataPtr;
  EFI_STATUS              Status;

  FvVolHdr = 0;
  DataPtr  = DataPtrIndex;

  //
  // Check if the Data is Volatile.
  //
  if (!Volatile && !mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    if (Fvb == NULL) {
      return EFI_UNSUPPORTED;
    }

    Status = Fvb->GetPhysicalAddress (Fvb, &FvVolHdr);
    ASSERT_EFI_ERROR (Status);

    //
    // Data Pointer should point to the actual Address where data is to be
    // written.
    //
    if (SetByIndex) {
      DataPtr += mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    }

    if ((DataPtr + DataSize) > (FvVolHdr + mNvFvHeaderCache->FvLength)) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // Data Pointer should point to the actual Address where data is to be
    // written.
    //
    if (Volatile) {
      VolatileBase = (VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
      if (SetByIndex) {
        DataPtr += mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
      }

      if ((DataPtr + DataSize) > ((UINTN)VolatileBase + VolatileBase->Size)) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      //
      // Emulated non-volatile variable mode.
      //
      if (SetByIndex) {
        DataPtr += (UINTN)mNvVariableCache;
      }

      if ((DataPtr + DataSize) > ((UINTN)mNvVariableCache + mNvVariableCache->Size)) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    //
    // If Volatile/Emulated Non-volatile Variable just do a simple mem copy.
    //
    CopyMem ((UINT8 *)(UINTN)DataPtr, Buffer, DataSize);
    return EFI_SUCCESS;
  }

  //
  // If we are here we are dealing with Non-Volatile Variables.
  //
  LinearOffset  = (UINTN)FvVolHdr;
  CurrWritePtr  = (UINTN)DataPtr;
  CurrWriteSize = DataSize;
  CurrBuffer    = Buffer;
  LbaNumber     = 0;

  if (CurrWritePtr < LinearOffset) {
    return EFI_INVALID_PARAMETER;
  }

  for (PtrBlockMapEntry = mNvFvHeaderCache->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      //
      // Check to see if the Variable Writes are spanning through multiple
      // blocks.
      //
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->Length)) {
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->Length)) {
          Status = Fvb->Write (
                          Fvb,
                          LbaNumber,
                          (UINTN)(CurrWritePtr - LinearOffset),
                          &CurrWriteSize,
                          CurrBuffer
                          );
          return Status;
        } else {
          Size   = (UINT32)(LinearOffset + PtrBlockMapEntry->Length - CurrWritePtr);
          Status = Fvb->Write (
                          Fvb,
                          LbaNumber,
                          (UINTN)(CurrWritePtr - LinearOffset),
                          &Size,
                          CurrBuffer
                          );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          CurrWritePtr  = LinearOffset + PtrBlockMapEntry->Length;
          CurrBuffer    = CurrBuffer + Size;
          CurrWriteSize = CurrWriteSize - Size;
        }
      }

      LinearOffset += PtrBlockMapEntry->Length;
      LbaNumber++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Record variable error flag.

  @param[in] Flag               Variable error flag to record.
  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Attributes         Attributes of the variable.
  @param[in] VariableSize       Size of the variable.

**/
VOID
RecordVarErrorFlag (
  IN VAR_ERROR_FLAG  Flag,
  IN CHAR16          *VariableName,
  IN EFI_GUID        *VendorGuid,
  IN UINT32          Attributes,
  IN UINTN           VariableSize
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  VAR_ERROR_FLAG          *VarErrFlag;
  VAR_ERROR_FLAG          TempFlag;

  DEBUG_CODE_BEGIN ();
  DEBUG ((DEBUG_ERROR, "RecordVarErrorFlag (0x%02x) %s:%g - 0x%08x - 0x%x\n", Flag, VariableName, VendorGuid, Attributes, VariableSize));
  if (Flag == VAR_ERROR_FLAG_SYSTEM_ERROR) {
    if (AtRuntime ()) {
      DEBUG ((DEBUG_ERROR, "CommonRuntimeVariableSpace = 0x%x - CommonVariableTotalSize = 0x%x\n", mVariableModuleGlobal->CommonRuntimeVariableSpace, mVariableModuleGlobal->CommonVariableTotalSize));
    } else {
      DEBUG ((DEBUG_ERROR, "CommonVariableSpace = 0x%x - CommonVariableTotalSize = 0x%x\n", mVariableModuleGlobal->CommonVariableSpace, mVariableModuleGlobal->CommonVariableTotalSize));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "CommonMaxUserVariableSpace = 0x%x - CommonUserVariableTotalSize = 0x%x\n", mVariableModuleGlobal->CommonMaxUserVariableSpace, mVariableModuleGlobal->CommonUserVariableTotalSize));
  }

  DEBUG_CODE_END ();

  if (!mEndOfDxe) {
    //
    // Before EndOfDxe, just record the current boot variable error flag to local variable,
    // and leave the variable error flag in NV flash as the last boot variable error flag.
    // After EndOfDxe in InitializeVarErrorFlag (), the variable error flag in NV flash
    // will be initialized to this local current boot variable error flag.
    //
    mCurrentBootVarErrFlag &= Flag;
    return;
  }

  //
  // Record error flag (it should have be initialized).
  //
  Status = FindVariable (
             VAR_ERROR_FLAG_NAME,
             &gEdkiiVarErrorFlagGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (!EFI_ERROR (Status)) {
    VarErrFlag = (VAR_ERROR_FLAG *)GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    TempFlag   = *VarErrFlag;
    TempFlag  &= Flag;
    if (TempFlag == *VarErrFlag) {
      return;
    }

    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               FALSE,
               mVariableModuleGlobal->FvbInstance,
               (UINTN)VarErrFlag - (UINTN)mNvVariableCache + (UINTN)mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
               sizeof (TempFlag),
               &TempFlag
               );
    if (!EFI_ERROR (Status)) {
      //
      // Update the data in NV cache.
      //
      *VarErrFlag = TempFlag;
      Status      =  SynchronizeRuntimeVariableCache (
                       &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeNvCache,
                       0,
                       mNvVariableCache->Size
                       );
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**
  Initialize variable error flag.

  Before EndOfDxe, the variable indicates the last boot variable error flag,
  then it means the last boot variable error flag must be got before EndOfDxe.
  After EndOfDxe, the variable indicates the current boot variable error flag,
  then it means the current boot variable error flag must be got after EndOfDxe.

**/
VOID
InitializeVarErrorFlag (
  VOID
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  VAR_ERROR_FLAG          Flag;
  VAR_ERROR_FLAG          VarErrFlag;

  if (!mEndOfDxe) {
    return;
  }

  Flag = mCurrentBootVarErrFlag;
  DEBUG ((DEBUG_INFO, "Initialize variable error flag (%02x)\n", Flag));

  Status = FindVariable (
             VAR_ERROR_FLAG_NAME,
             &gEdkiiVarErrorFlagGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (!EFI_ERROR (Status)) {
    VarErrFlag = *((VAR_ERROR_FLAG *)GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat));
    if (VarErrFlag == Flag) {
      return;
    }
  }

  UpdateVariable (
    VAR_ERROR_FLAG_NAME,
    &gEdkiiVarErrorFlagGuid,
    &Flag,
    sizeof (Flag),
    VARIABLE_ATTRIBUTE_NV_BS_RT,
    0,
    0,
    &Variable,
    NULL
    );
}

/**
  Is user variable?

  @param[in] Variable   Pointer to variable header.

  @retval TRUE          User variable.
  @retval FALSE         System variable.

**/
BOOLEAN
IsUserVariable (
  IN VARIABLE_HEADER  *Variable
  )
{
  VAR_CHECK_VARIABLE_PROPERTY  Property;

  //
  // Only after End Of Dxe, the variables belong to system variable are fixed.
  // If PcdMaxUserNvStorageVariableSize is 0, it means user variable share the same NV storage with system variable,
  // then no need to check if the variable is user variable or not specially.
  //
  if (mEndOfDxe && (mVariableModuleGlobal->CommonMaxUserVariableSpace != mVariableModuleGlobal->CommonVariableSpace)) {
    if (VarCheckLibVariablePropertyGet (
          GetVariableNamePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
          GetVendorGuidPtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
          &Property
          ) == EFI_NOT_FOUND)
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Calculate common user variable total size.

**/
VOID
CalculateCommonUserVariableTotalSize (
  VOID
  )
{
  VARIABLE_HEADER              *Variable;
  VARIABLE_HEADER              *NextVariable;
  UINTN                        VariableSize;
  VAR_CHECK_VARIABLE_PROPERTY  Property;

  //
  // Only after End Of Dxe, the variables belong to system variable are fixed.
  // If PcdMaxUserNvStorageVariableSize is 0, it means user variable share the same NV storage with system variable,
  // then no need to calculate the common user variable total size specially.
  //
  if (mEndOfDxe && (mVariableModuleGlobal->CommonMaxUserVariableSpace != mVariableModuleGlobal->CommonVariableSpace)) {
    Variable = GetStartPointer (mNvVariableCache);
    while (IsValidVariableHeader (Variable, GetEndPointer (mNvVariableCache))) {
      NextVariable = GetNextVariablePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      VariableSize = (UINTN)NextVariable - (UINTN)Variable;
      if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        if (VarCheckLibVariablePropertyGet (
              GetVariableNamePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
              GetVendorGuidPtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
              &Property
              ) == EFI_NOT_FOUND)
        {
          //
          // No property, it is user variable.
          //
          mVariableModuleGlobal->CommonUserVariableTotalSize += VariableSize;
        }
      }

      Variable = NextVariable;
    }
  }
}

/**
  Initialize variable quota.

**/
VOID
InitializeVariableQuota (
  VOID
  )
{
  if (!mEndOfDxe) {
    return;
  }

  InitializeVarErrorFlag ();
  CalculateCommonUserVariableTotalSize ();
}

/**

  Variable store garbage collection and reclaim operation.

  @param[in]      VariableBase            Base address of variable store.
  @param[out]     LastVariableOffset      Offset of last variable.
  @param[in]      IsVolatile              The variable store is volatile or not;
                                          if it is non-volatile, need FTW.
  @param[in, out] UpdatingPtrTrack        Pointer to updating variable pointer track structure.
  @param[in]      NewVariable             Pointer to new variable.
  @param[in]      NewVariableSize         New variable size.

  @return EFI_SUCCESS                  Reclaim operation has finished successfully.
  @return EFI_OUT_OF_RESOURCES         No enough memory resources or variable space.
  @return Others                       Unexpect error happened during reclaim operation.

**/
EFI_STATUS
Reclaim (
  IN     EFI_PHYSICAL_ADDRESS    VariableBase,
  OUT    UINTN                   *LastVariableOffset,
  IN     BOOLEAN                 IsVolatile,
  IN OUT VARIABLE_POINTER_TRACK  *UpdatingPtrTrack,
  IN     VARIABLE_HEADER         *NewVariable,
  IN     UINTN                   NewVariableSize
  )
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *AddedVariable;
  VARIABLE_HEADER        *NextVariable;
  VARIABLE_HEADER        *NextAddedVariable;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT8                  *ValidBuffer;
  UINTN                  MaximumBufferSize;
  UINTN                  VariableSize;
  UINTN                  NameSize;
  UINT8                  *CurrPtr;
  VOID                   *Point0;
  VOID                   *Point1;
  BOOLEAN                FoundAdded;
  EFI_STATUS             Status;
  EFI_STATUS             DoneStatus;
  UINTN                  CommonVariableTotalSize;
  UINTN                  CommonUserVariableTotalSize;
  UINTN                  HwErrVariableTotalSize;
  VARIABLE_HEADER        *UpdatingVariable;
  VARIABLE_HEADER        *UpdatingInDeletedTransition;
  BOOLEAN                AuthFormat;

  AuthFormat                  = mVariableModuleGlobal->VariableGlobal.AuthFormat;
  UpdatingVariable            = NULL;
  UpdatingInDeletedTransition = NULL;
  if (UpdatingPtrTrack != NULL) {
    UpdatingVariable            = UpdatingPtrTrack->CurrPtr;
    UpdatingInDeletedTransition = UpdatingPtrTrack->InDeletedTransitionPtr;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)VariableBase);

  CommonVariableTotalSize     = 0;
  CommonUserVariableTotalSize = 0;
  HwErrVariableTotalSize      = 0;

  if (IsVolatile || mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    //
    // Start Pointers for the variable.
    //
    Variable          = GetStartPointer (VariableStoreHeader);
    MaximumBufferSize = sizeof (VARIABLE_STORE_HEADER);

    while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))) {
      NextVariable = GetNextVariablePtr (Variable, AuthFormat);
      if (((Variable->State == VAR_ADDED) || (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) &&
          (Variable != UpdatingVariable) &&
          (Variable != UpdatingInDeletedTransition)
          )
      {
        VariableSize       = (UINTN)NextVariable - (UINTN)Variable;
        MaximumBufferSize += VariableSize;
      }

      Variable = NextVariable;
    }

    if (NewVariable != NULL) {
      //
      // Add the new variable size.
      //
      MaximumBufferSize += NewVariableSize;
    }

    //
    // Reserve the 1 Bytes with Oxff to identify the
    // end of the variable buffer.
    //
    MaximumBufferSize += 1;
    ValidBuffer        = AllocatePool (MaximumBufferSize);
    if (ValidBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // For NV variable reclaim, don't allocate pool here and just use mNvVariableCache
    // as the buffer to reduce SMRAM consumption for SMM variable driver.
    //
    MaximumBufferSize = mNvVariableCache->Size;
    ValidBuffer       = (UINT8 *)mNvVariableCache;
  }

  SetMem (ValidBuffer, MaximumBufferSize, 0xff);

  //
  // Copy variable store header.
  //
  CopyMem (ValidBuffer, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr = (UINT8 *)GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer);

  //
  // Reinstall all ADDED variables as long as they are not identical to Updating Variable.
  //
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))) {
    NextVariable = GetNextVariablePtr (Variable, AuthFormat);
    if ((Variable != UpdatingVariable) && (Variable->State == VAR_ADDED)) {
      VariableSize = (UINTN)NextVariable - (UINTN)Variable;
      CopyMem (CurrPtr, (UINT8 *)Variable, VariableSize);
      CurrPtr += VariableSize;
      if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        HwErrVariableTotalSize += VariableSize;
      } else if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        CommonVariableTotalSize += VariableSize;
        if (IsUserVariable (Variable)) {
          CommonUserVariableTotalSize += VariableSize;
        }
      }
    }

    Variable = NextVariable;
  }

  //
  // Reinstall all in delete transition variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))) {
    NextVariable = GetNextVariablePtr (Variable, AuthFormat);
    if ((Variable != UpdatingVariable) && (Variable != UpdatingInDeletedTransition) && (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) {
      //
      // Buffer has cached all ADDED variable.
      // Per IN_DELETED variable, we have to guarantee that
      // no ADDED one in previous buffer.
      //

      FoundAdded    = FALSE;
      AddedVariable = GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer);
      while (IsValidVariableHeader (AddedVariable, GetEndPointer ((VARIABLE_STORE_HEADER *)ValidBuffer))) {
        NextAddedVariable = GetNextVariablePtr (AddedVariable, AuthFormat);
        NameSize          = NameSizeOfVariable (AddedVariable, AuthFormat);
        if (CompareGuid (
              GetVendorGuidPtr (AddedVariable, AuthFormat),
              GetVendorGuidPtr (Variable, AuthFormat)
              ) && (NameSize == NameSizeOfVariable (Variable, AuthFormat)))
        {
          Point0 = (VOID *)GetVariableNamePtr (AddedVariable, AuthFormat);
          Point1 = (VOID *)GetVariableNamePtr (Variable, AuthFormat);
          if (CompareMem (Point0, Point1, NameSize) == 0) {
            FoundAdded = TRUE;
            break;
          }
        }

        AddedVariable = NextAddedVariable;
      }

      if (!FoundAdded) {
        //
        // Promote VAR_IN_DELETED_TRANSITION to VAR_ADDED.
        //
        VariableSize = (UINTN)NextVariable - (UINTN)Variable;
        CopyMem (CurrPtr, (UINT8 *)Variable, VariableSize);
        ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
        CurrPtr                            += VariableSize;
        if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          HwErrVariableTotalSize += VariableSize;
        } else if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          CommonVariableTotalSize += VariableSize;
          if (IsUserVariable (Variable)) {
            CommonUserVariableTotalSize += VariableSize;
          }
        }
      }
    }

    Variable = NextVariable;
  }

  //
  // Install the new variable if it is not NULL.
  //
  if (NewVariable != NULL) {
    if (((UINTN)CurrPtr - (UINTN)ValidBuffer) + NewVariableSize > VariableStoreHeader->Size) {
      //
      // No enough space to store the new variable.
      //
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    if (!IsVolatile) {
      if ((NewVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        HwErrVariableTotalSize += NewVariableSize;
      } else if ((NewVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        CommonVariableTotalSize += NewVariableSize;
        if (IsUserVariable (NewVariable)) {
          CommonUserVariableTotalSize += NewVariableSize;
        }
      }

      if ((HwErrVariableTotalSize > PcdGet32 (PcdHwErrStorageSize)) ||
          (CommonVariableTotalSize > mVariableModuleGlobal->CommonVariableSpace) ||
          (CommonUserVariableTotalSize > mVariableModuleGlobal->CommonMaxUserVariableSpace))
      {
        //
        // No enough space to store the new variable by NV or NV+HR attribute.
        //
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }

    CopyMem (CurrPtr, (UINT8 *)NewVariable, NewVariableSize);
    ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
    if (UpdatingVariable != NULL) {
      UpdatingPtrTrack->CurrPtr                = (VARIABLE_HEADER *)((UINTN)UpdatingPtrTrack->StartPtr + ((UINTN)CurrPtr - (UINTN)GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer)));
      UpdatingPtrTrack->InDeletedTransitionPtr = NULL;
    }

    CurrPtr += NewVariableSize;
  }

  if (IsVolatile || mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    //
    // If volatile/emulated non-volatile variable store, just copy valid buffer.
    //
    SetMem ((UINT8 *)(UINTN)VariableBase, VariableStoreHeader->Size, 0xff);
    CopyMem ((UINT8 *)(UINTN)VariableBase, ValidBuffer, (UINTN)CurrPtr - (UINTN)ValidBuffer);
    *LastVariableOffset = (UINTN)CurrPtr - (UINTN)ValidBuffer;
    if (!IsVolatile) {
      //
      // Emulated non-volatile variable mode.
      //
      mVariableModuleGlobal->HwErrVariableTotalSize      = HwErrVariableTotalSize;
      mVariableModuleGlobal->CommonVariableTotalSize     = CommonVariableTotalSize;
      mVariableModuleGlobal->CommonUserVariableTotalSize = CommonUserVariableTotalSize;
    }

    Status = EFI_SUCCESS;
  } else {
    //
    // If non-volatile variable store, perform FTW here.
    //
    Status = FtwVariableSpace (
               VariableBase,
               (VARIABLE_STORE_HEADER *)ValidBuffer
               );
    if (!EFI_ERROR (Status)) {
      *LastVariableOffset                                = (UINTN)CurrPtr - (UINTN)ValidBuffer;
      mVariableModuleGlobal->HwErrVariableTotalSize      = HwErrVariableTotalSize;
      mVariableModuleGlobal->CommonVariableTotalSize     = CommonVariableTotalSize;
      mVariableModuleGlobal->CommonUserVariableTotalSize = CommonUserVariableTotalSize;
    } else {
      mVariableModuleGlobal->HwErrVariableTotalSize      = 0;
      mVariableModuleGlobal->CommonVariableTotalSize     = 0;
      mVariableModuleGlobal->CommonUserVariableTotalSize = 0;
      Variable                                           = GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)VariableBase);
      while (IsValidVariableHeader (Variable, GetEndPointer ((VARIABLE_STORE_HEADER *)(UINTN)VariableBase))) {
        NextVariable = GetNextVariablePtr (Variable, AuthFormat);
        VariableSize = (UINTN)NextVariable - (UINTN)Variable;
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          mVariableModuleGlobal->HwErrVariableTotalSize += VariableSize;
        } else if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          mVariableModuleGlobal->CommonVariableTotalSize += VariableSize;
          if (IsUserVariable (Variable)) {
            mVariableModuleGlobal->CommonUserVariableTotalSize += VariableSize;
          }
        }

        Variable = NextVariable;
      }

      *LastVariableOffset = (UINTN)Variable - (UINTN)VariableBase;
    }
  }

Done:
  DoneStatus = EFI_SUCCESS;
  if (IsVolatile || mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    DoneStatus = SynchronizeRuntimeVariableCache (
                   &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeVolatileCache,
                   0,
                   VariableStoreHeader->Size
                   );
    ASSERT_EFI_ERROR (DoneStatus);
    FreePool (ValidBuffer);
  } else {
    //
    // For NV variable reclaim, we use mNvVariableCache as the buffer, so copy the data back.
    //
    CopyMem (mNvVariableCache, (UINT8 *)(UINTN)VariableBase, VariableStoreHeader->Size);
    DoneStatus = SynchronizeRuntimeVariableCache (
                   &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeNvCache,
                   0,
                   VariableStoreHeader->Size
                   );
    ASSERT_EFI_ERROR (DoneStatus);
  }

  if (!EFI_ERROR (Status) && EFI_ERROR (DoneStatus)) {
    Status = DoneStatus;
  }

  return Status;
}

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  If IgnoreRtCheck is TRUE, then we ignore the EFI_VARIABLE_RUNTIME_ACCESS attribute check
  at runtime when searching existing variable, only VariableName and VendorGuid are compared.
  Otherwise, variables without EFI_VARIABLE_RUNTIME_ACCESS are not visible at runtime.

  @param[in]   VariableName           Name of the variable to be found.
  @param[in]   VendorGuid             Vendor GUID to be found.
  @param[out]  PtrTrack               VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param[in]   Global                 Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.
  @param[in]   IgnoreRtCheck          Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                      check at runtime when searching variable.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global,
  IN  BOOLEAN                 IgnoreRtCheck
  )
{
  EFI_STATUS             Status;
  VARIABLE_STORE_HEADER  *VariableStoreHeader[VariableStoreTypeMax];
  VARIABLE_STORE_TYPE    Type;

  if ((VariableName[0] != 0) && (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // 0: Volatile, 1: HOB, 2: Non-Volatile.
  // The index and attributes mapping must be kept in this order as RuntimeServiceGetNextVariableName
  // make use of this mapping to implement search algorithm.
  //
  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)Global->VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)Global->HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  //
  // Find the variable by walk through HOB, volatile and non-volatile variable store.
  //
  for (Type = (VARIABLE_STORE_TYPE)0; Type < VariableStoreTypeMax; Type++) {
    if (VariableStoreHeader[Type] == NULL) {
      continue;
    }

    PtrTrack->StartPtr = GetStartPointer (VariableStoreHeader[Type]);
    PtrTrack->EndPtr   = GetEndPointer (VariableStoreHeader[Type]);
    PtrTrack->Volatile = (BOOLEAN)(Type == VariableStoreTypeVolatile);

    Status =  FindVariableEx (
                VariableName,
                VendorGuid,
                IgnoreRtCheck,
                PtrTrack,
                mVariableModuleGlobal->VariableGlobal.AuthFormat
                );
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get index from supported language codes according to language string.

  This code is used to get corresponding index in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation to find matched string and calculate the index.
  In RFC4646 language tags, take semicolon as a delimitation to find matched string and calculate the index.

  For example:
    SupportedLang  = "engfraengfra"
    Lang           = "eng"
    Iso639Language = TRUE
  The return value is "0".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Lang           = "fr-FR"
    Iso639Language = FALSE
  The return value is "3".

  @param  SupportedLang               Platform supported language codes.
  @param  Lang                        Configured language.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval The index of language in the language codes.

**/
UINTN
GetIndexFromSupportedLangCodes (
  IN  CHAR8    *SupportedLang,
  IN  CHAR8    *Lang,
  IN  BOOLEAN  Iso639Language
  )
{
  UINTN  Index;
  UINTN  CompareLength;
  UINTN  LanguageLength;

  if (Iso639Language) {
    CompareLength = ISO_639_2_ENTRY_SIZE;
    for (Index = 0; Index < AsciiStrLen (SupportedLang); Index += CompareLength) {
      if (AsciiStrnCmp (Lang, SupportedLang + Index, CompareLength) == 0) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        Index = Index / CompareLength;
        return Index;
      }
    }

    ASSERT (FALSE);
    return 0;
  } else {
    //
    // Compare RFC4646 language code
    //
    Index = 0;
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++) {
    }

    for (Index = 0; *SupportedLang != '\0'; Index++, SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for ( ; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++) {
      }

      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++) {
      }

      if ((CompareLength == LanguageLength) &&
          (AsciiStrnCmp (Lang, SupportedLang, CompareLength) == 0))
      {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        return Index;
      }
    }

    ASSERT (FALSE);
    return 0;
  }
}

/**
  Get language string from supported language codes according to index.

  This code is used to get corresponding language strings in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation. Find language string according to the index.
  In RFC4646 language tags, take semicolon as a delimitation. Find language string according to the index.

  For example:
    SupportedLang  = "engfraengfra"
    Index          = "1"
    Iso639Language = TRUE
  The return value is "fra".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Index          = "1"
    Iso639Language = FALSE
  The return value is "fr".

  @param  SupportedLang               Platform supported language codes.
  @param  Index                       The index in supported language codes.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval The language string in the language codes.

**/
CHAR8 *
GetLangFromSupportedLangCodes (
  IN  CHAR8    *SupportedLang,
  IN  UINTN    Index,
  IN  BOOLEAN  Iso639Language
  )
{
  UINTN  SubIndex;
  UINTN  CompareLength;
  CHAR8  *Supported;

  SubIndex  = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    //
    // According to the index of Lang string in SupportedLang string to get the language.
    // This code will be invoked in RUNTIME, therefore there is not a memory allocate/free operation.
    // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
    //
    CompareLength                              = ISO_639_2_ENTRY_SIZE;
    mVariableModuleGlobal->Lang[CompareLength] = '\0';
    return CopyMem (mVariableModuleGlobal->Lang, SupportedLang + Index * CompareLength, CompareLength);
  } else {
    while (TRUE) {
      //
      // Take semicolon as delimitation, sequentially traverse supported language codes.
      //
      for (CompareLength = 0; *Supported != ';' && *Supported != '\0'; CompareLength++) {
        Supported++;
      }

      if ((*Supported == '\0') && (SubIndex != Index)) {
        //
        // Have completed the traverse, but not find corrsponding string.
        // This case is not allowed to happen.
        //
        ASSERT (FALSE);
        return NULL;
      }

      if (SubIndex == Index) {
        //
        // According to the index of Lang string in SupportedLang string to get the language.
        // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
        // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
        //
        mVariableModuleGlobal->PlatformLang[CompareLength] = '\0';
        return CopyMem (mVariableModuleGlobal->PlatformLang, Supported - CompareLength, CompareLength);
      }

      SubIndex++;

      //
      // Skip ';' characters in Supported
      //
      for ( ; *Supported != '\0' && *Supported == ';'; Supported++) {
      }
    }
  }
}

/**
  Returns a pointer to an allocated buffer that contains the best matching language
  from a set of supported languages.

  This function supports both ISO 639-2 and RFC 4646 language codes, but language
  code types may not be mixed in a single call to this function. This function
  supports a variable argument list that allows the caller to pass in a prioritized
  list of language codes to test against all the language codes in SupportedLanguages.

  If SupportedLanguages is NULL, then ASSERT().

  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes in the format
                                  specified by Iso639Language.
  @param[in]  Iso639Language      If not zero, then all language codes are assumed to be
                                  in ISO 639-2 format.  If zero, then all language
                                  codes are assumed to be in RFC 4646 language format
  @param[in]  ...                 A variable argument list that contains pointers to
                                  Null-terminated ASCII strings that contain one or more
                                  language codes in the format specified by Iso639Language.
                                  The first language code from each of these language
                                  code lists is used to determine if it is an exact or
                                  close match to any of the language codes in
                                  SupportedLanguages.  Close matches only apply to RFC 4646
                                  language codes, and the matching algorithm from RFC 4647
                                  is used to determine if a close match is present.  If
                                  an exact or close match is found, then the matching
                                  language code from SupportedLanguages is returned.  If
                                  no matches are found, then the next variable argument
                                  parameter is evaluated.  The variable argument list
                                  is terminated by a NULL.

  @retval NULL   The best matching language could not be found in SupportedLanguages.
  @retval NULL   There are not enough resources available to return the best matching
                 language.
  @retval Other  A pointer to a Null-terminated ASCII string that is the best matching
                 language in SupportedLanguages.

**/
CHAR8 *
EFIAPI
VariableGetBestLanguage (
  IN CONST CHAR8  *SupportedLanguages,
  IN UINTN        Iso639Language,
  ...
  )
{
  VA_LIST      Args;
  CHAR8        *Language;
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CONST CHAR8  *Supported;
  CHAR8        *Buffer;

  if (SupportedLanguages == NULL) {
    return NULL;
  }

  VA_START (Args, Iso639Language);
  while ((Language = VA_ARG (Args, CHAR8 *)) != NULL) {
    //
    // Default to ISO 639-2 mode
    //
    CompareLength  = 3;
    LanguageLength = MIN (3, AsciiStrLen (Language));

    //
    // If in RFC 4646 mode, then determine the length of the first RFC 4646 language code in Language
    //
    if (Iso639Language == 0) {
      for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++) {
      }
    }

    //
    // Trim back the length of Language used until it is empty
    //
    while (LanguageLength > 0) {
      //
      // Loop through all language codes in SupportedLanguages
      //
      for (Supported = SupportedLanguages; *Supported != '\0'; Supported += CompareLength) {
        //
        // In RFC 4646 mode, then Loop through all language codes in SupportedLanguages
        //
        if (Iso639Language == 0) {
          //
          // Skip ';' characters in Supported
          //
          for ( ; *Supported != '\0' && *Supported == ';'; Supported++) {
          }

          //
          // Determine the length of the next language code in Supported
          //
          for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++) {
          }

          //
          // If Language is longer than the Supported, then skip to the next language
          //
          if (LanguageLength > CompareLength) {
            continue;
          }
        }

        //
        // See if the first LanguageLength characters in Supported match Language
        //
        if (AsciiStrnCmp (Supported, Language, LanguageLength) == 0) {
          VA_END (Args);

          Buffer                = (Iso639Language != 0) ? mVariableModuleGlobal->Lang : mVariableModuleGlobal->PlatformLang;
          Buffer[CompareLength] = '\0';
          return CopyMem (Buffer, Supported, CompareLength);
        }
      }

      if (Iso639Language != 0) {
        //
        // If ISO 639 mode, then each language can only be tested once
        //
        LanguageLength = 0;
      } else {
        //
        // If RFC 4646 mode, then trim Language from the right to the next '-' character
        //
        for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--) {
        }
      }
    }
  }

  VA_END (Args);

  //
  // No matches were found
  //
  return NULL;
}

/**
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param[in] Marker             VA_LIST style variable argument list.
                                The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
BOOLEAN
EFIAPI
CheckRemainingSpaceForConsistencyInternal (
  IN UINT32   Attributes,
  IN VA_LIST  Marker
  )
{
  EFI_STATUS                  Status;
  VA_LIST                     Args;
  VARIABLE_ENTRY_CONSISTENCY  *VariableEntry;
  UINT64                      MaximumVariableStorageSize;
  UINT64                      RemainingVariableStorageSize;
  UINT64                      MaximumVariableSize;
  UINTN                       TotalNeededSize;
  UINTN                       OriginalVarSize;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  VARIABLE_POINTER_TRACK      VariablePtrTrack;
  VARIABLE_HEADER             *NextVariable;
  UINTN                       VarNameSize;
  UINTN                       VarDataSize;

  //
  // Non-Volatile related.
  //
  VariableStoreHeader = mNvVariableCache;

  Status = VariableServiceQueryVariableInfoInternal (
             Attributes,
             &MaximumVariableStorageSize,
             &RemainingVariableStorageSize,
             &MaximumVariableSize
             );
  ASSERT_EFI_ERROR (Status);

  TotalNeededSize = 0;
  VA_COPY (Args, Marker);
  VariableEntry = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  while (VariableEntry != NULL) {
    //
    // Calculate variable total size.
    //
    VarNameSize                 = StrSize (VariableEntry->Name);
    VarNameSize                += GET_PAD_SIZE (VarNameSize);
    VarDataSize                 = VariableEntry->VariableSize;
    VarDataSize                += GET_PAD_SIZE (VarDataSize);
    VariableEntry->VariableSize = HEADER_ALIGN (
                                    GetVariableHeaderSize (
                                      mVariableModuleGlobal->VariableGlobal.AuthFormat
                                      ) + VarNameSize + VarDataSize
                                    );

    TotalNeededSize += VariableEntry->VariableSize;
    VariableEntry    = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  }

  VA_END (Args);

  if (RemainingVariableStorageSize >= TotalNeededSize) {
    //
    // Already have enough space.
    //
    return TRUE;
  } else if (AtRuntime ()) {
    //
    // At runtime, no reclaim.
    // The original variable space of Variables can't be reused.
    //
    return FALSE;
  }

  VA_COPY (Args, Marker);
  VariableEntry = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  while (VariableEntry != NULL) {
    //
    // Check if Variable[Index] has been present and get its size.
    //
    OriginalVarSize           = 0;
    VariablePtrTrack.StartPtr = GetStartPointer (VariableStoreHeader);
    VariablePtrTrack.EndPtr   = GetEndPointer (VariableStoreHeader);
    Status                    = FindVariableEx (
                                  VariableEntry->Name,
                                  VariableEntry->Guid,
                                  FALSE,
                                  &VariablePtrTrack,
                                  mVariableModuleGlobal->VariableGlobal.AuthFormat
                                  );
    if (!EFI_ERROR (Status)) {
      //
      // Get size of Variable[Index].
      //
      NextVariable    = GetNextVariablePtr (VariablePtrTrack.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      OriginalVarSize = (UINTN)NextVariable - (UINTN)VariablePtrTrack.CurrPtr;
      //
      // Add the original size of Variable[Index] to remaining variable storage size.
      //
      RemainingVariableStorageSize += OriginalVarSize;
    }

    if (VariableEntry->VariableSize > RemainingVariableStorageSize) {
      //
      // No enough space for Variable[Index].
      //
      VA_END (Args);
      return FALSE;
    }

    //
    // Sub the (new) size of Variable[Index] from remaining variable storage size.
    //
    RemainingVariableStorageSize -= VariableEntry->VariableSize;
    VariableEntry                 = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  }

  VA_END (Args);

  return TRUE;
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
CheckRemainingSpaceForConsistency (
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
  Hook the operations in PlatformLangCodes, LangCodes, PlatformLang and Lang.

  When setting Lang/LangCodes, simultaneously update PlatformLang/PlatformLangCodes.

  According to UEFI spec, PlatformLangCodes/LangCodes are only set once in firmware initialization,
  and are read-only. Therefore, in variable driver, only store the original value for other use.

  @param[in] VariableName       Name of variable.

  @param[in] Data               Variable data.

  @param[in] DataSize           Size of data. 0 means delete.

  @retval EFI_SUCCESS           The update operation is successful or ignored.
  @retval EFI_WRITE_PROTECTED   Update PlatformLangCodes/LangCodes at runtime.
  @retval EFI_OUT_OF_RESOURCES  No enough variable space to do the update operation.
  @retval Others                Other errors happened during the update operation.

**/
EFI_STATUS
AutoUpdateLangVariable (
  IN  CHAR16  *VariableName,
  IN  VOID    *Data,
  IN  UINTN   DataSize
  )
{
  EFI_STATUS                  Status;
  CHAR8                       *BestPlatformLang;
  CHAR8                       *BestLang;
  UINTN                       Index;
  UINT32                      Attributes;
  VARIABLE_POINTER_TRACK      Variable;
  BOOLEAN                     SetLanguageCodes;
  VARIABLE_ENTRY_CONSISTENCY  VariableEntry[2];

  //
  // Don't do updates for delete operation
  //
  if (DataSize == 0) {
    return EFI_SUCCESS;
  }

  SetLanguageCodes = FALSE;

  if (StrCmp (VariableName, EFI_PLATFORM_LANG_CODES_VARIABLE_NAME) == 0) {
    //
    // PlatformLangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (AtRuntime ()) {
      return EFI_WRITE_PROTECTED;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, PlatformLangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (mVariableModuleGlobal->PlatformLangCodes != NULL) {
      FreePool (mVariableModuleGlobal->PlatformLangCodes);
    }

    mVariableModuleGlobal->PlatformLangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->PlatformLangCodes != NULL);

    //
    // PlatformLang holds a single language from PlatformLangCodes,
    // so the size of PlatformLangCodes is enough for the PlatformLang.
    //
    if (mVariableModuleGlobal->PlatformLang != NULL) {
      FreePool (mVariableModuleGlobal->PlatformLang);
    }

    mVariableModuleGlobal->PlatformLang = AllocateRuntimePool (DataSize);
    ASSERT (mVariableModuleGlobal->PlatformLang != NULL);
  } else if (StrCmp (VariableName, EFI_LANG_CODES_VARIABLE_NAME) == 0) {
    //
    // LangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (AtRuntime ()) {
      return EFI_WRITE_PROTECTED;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, LangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (mVariableModuleGlobal->LangCodes != NULL) {
      FreePool (mVariableModuleGlobal->LangCodes);
    }

    mVariableModuleGlobal->LangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->LangCodes != NULL);
  }

  if (  SetLanguageCodes
     && (mVariableModuleGlobal->PlatformLangCodes != NULL)
     && (mVariableModuleGlobal->LangCodes != NULL))
  {
    //
    // Update Lang if PlatformLang is already set
    // Update PlatformLang if Lang is already set
    //
    Status = FindVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
    if (!EFI_ERROR (Status)) {
      //
      // Update Lang
      //
      VariableName = EFI_PLATFORM_LANG_VARIABLE_NAME;
      Data         = GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      DataSize     = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    } else {
      Status = FindVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
      if (!EFI_ERROR (Status)) {
        //
        // Update PlatformLang
        //
        VariableName = EFI_LANG_VARIABLE_NAME;
        Data         = GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
        DataSize     = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      } else {
        //
        // Neither PlatformLang nor Lang is set, directly return
        //
        return EFI_SUCCESS;
      }
    }
  }

  Status = EFI_SUCCESS;

  //
  // According to UEFI spec, "Lang" and "PlatformLang" is NV|BS|RT attributions.
  //
  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (StrCmp (VariableName, EFI_PLATFORM_LANG_VARIABLE_NAME) == 0) {
    //
    // Update Lang when PlatformLangCodes/LangCodes were set.
    //
    if ((mVariableModuleGlobal->PlatformLangCodes != NULL) && (mVariableModuleGlobal->LangCodes != NULL)) {
      //
      // When setting PlatformLang, firstly get most matched language string from supported language codes.
      //
      BestPlatformLang = VariableGetBestLanguage (mVariableModuleGlobal->PlatformLangCodes, FALSE, Data, NULL);
      if (BestPlatformLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mVariableModuleGlobal->PlatformLangCodes, BestPlatformLang, FALSE);

        //
        // Get the corresponding ISO639 language tag according to RFC4646 language tag.
        //
        BestLang = GetLangFromSupportedLangCodes (mVariableModuleGlobal->LangCodes, Index, TRUE);

        //
        // Check the variable space for both Lang and PlatformLang variable.
        //
        VariableEntry[0].VariableSize = ISO_639_2_ENTRY_SIZE + 1;
        VariableEntry[0].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[0].Name         = EFI_LANG_VARIABLE_NAME;

        VariableEntry[1].VariableSize = AsciiStrSize (BestPlatformLang);
        VariableEntry[1].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[1].Name         = EFI_PLATFORM_LANG_VARIABLE_NAME;
        if (!CheckRemainingSpaceForConsistency (VARIABLE_ATTRIBUTE_NV_BS_RT, &VariableEntry[0], &VariableEntry[1], NULL)) {
          //
          // No enough variable space to set both Lang and PlatformLang successfully.
          //
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          //
          // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
          //
          FindVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);

          Status = UpdateVariable (
                     EFI_LANG_VARIABLE_NAME,
                     &gEfiGlobalVariableGuid,
                     BestLang,
                     ISO_639_2_ENTRY_SIZE + 1,
                     Attributes,
                     0,
                     0,
                     &Variable,
                     NULL
                     );
        }

        DEBUG ((DEBUG_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a Status: %r\n", BestPlatformLang, BestLang, Status));
      }
    }
  } else if (StrCmp (VariableName, EFI_LANG_VARIABLE_NAME) == 0) {
    //
    // Update PlatformLang when PlatformLangCodes/LangCodes were set.
    //
    if ((mVariableModuleGlobal->PlatformLangCodes != NULL) && (mVariableModuleGlobal->LangCodes != NULL)) {
      //
      // When setting Lang, firstly get most matched language string from supported language codes.
      //
      BestLang = VariableGetBestLanguage (mVariableModuleGlobal->LangCodes, TRUE, Data, NULL);
      if (BestLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mVariableModuleGlobal->LangCodes, BestLang, TRUE);

        //
        // Get the corresponding RFC4646 language tag according to ISO639 language tag.
        //
        BestPlatformLang = GetLangFromSupportedLangCodes (mVariableModuleGlobal->PlatformLangCodes, Index, FALSE);

        //
        // Check the variable space for both PlatformLang and Lang variable.
        //
        VariableEntry[0].VariableSize = AsciiStrSize (BestPlatformLang);
        VariableEntry[0].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[0].Name         = EFI_PLATFORM_LANG_VARIABLE_NAME;

        VariableEntry[1].VariableSize = ISO_639_2_ENTRY_SIZE + 1;
        VariableEntry[1].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[1].Name         = EFI_LANG_VARIABLE_NAME;
        if (!CheckRemainingSpaceForConsistency (VARIABLE_ATTRIBUTE_NV_BS_RT, &VariableEntry[0], &VariableEntry[1], NULL)) {
          //
          // No enough variable space to set both PlatformLang and Lang successfully.
          //
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          //
          // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
          //
          FindVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);

          Status = UpdateVariable (
                     EFI_PLATFORM_LANG_VARIABLE_NAME,
                     &gEfiGlobalVariableGuid,
                     BestPlatformLang,
                     AsciiStrSize (BestPlatformLang),
                     Attributes,
                     0,
                     0,
                     &Variable,
                     NULL
                     );
        }

        DEBUG ((DEBUG_INFO, "Variable Driver Auto Update Lang, Lang:%a, PlatformLang:%a Status: %r\n", BestLang, BestPlatformLang, Status));
      }
    }
  }

  if (SetLanguageCodes) {
    //
    // Continue to set PlatformLangCodes or LangCodes.
    //
    return EFI_SUCCESS;
  } else {
    return Status;
  }
}

/**
  Update the variable region with Variable information. If EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is set,
  index of associated public key is needed.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in, out] CacheVariable The variable information which is used to keep track of variable usage.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
UpdateVariable (
  IN      CHAR16                  *VariableName,
  IN      EFI_GUID                *VendorGuid,
  IN      VOID                    *Data,
  IN      UINTN                   DataSize,
  IN      UINT32                  Attributes      OPTIONAL,
  IN      UINT32                  KeyIndex        OPTIONAL,
  IN      UINT64                  MonotonicCount  OPTIONAL,
  IN OUT  VARIABLE_POINTER_TRACK  *CacheVariable,
  IN      EFI_TIME                *TimeStamp      OPTIONAL
  )
{
  EFI_STATUS                          Status;
  VARIABLE_HEADER                     *NextVariable;
  UINTN                               ScratchSize;
  UINTN                               MaxDataSize;
  UINTN                               VarNameOffset;
  UINTN                               VarDataOffset;
  UINTN                               VarNameSize;
  UINTN                               VarSize;
  BOOLEAN                             Volatile;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINT8                               State;
  VARIABLE_POINTER_TRACK              *Variable;
  VARIABLE_POINTER_TRACK              NvVariable;
  VARIABLE_STORE_HEADER               *VariableStoreHeader;
  VARIABLE_RUNTIME_CACHE              *VolatileCacheInstance;
  UINT8                               *BufferForMerge;
  UINTN                               MergedBufSize;
  BOOLEAN                             DataReady;
  UINTN                               DataOffset;
  BOOLEAN                             IsCommonVariable;
  BOOLEAN                             IsCommonUserVariable;
  AUTHENTICATED_VARIABLE_HEADER       *AuthVariable;
  BOOLEAN                             AuthFormat;

  if ((mVariableModuleGlobal->FvbInstance == NULL) && !mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    //
    // The FVB protocol is not ready, so the EFI_VARIABLE_WRITE_ARCH_PROTOCOL is not installed.
    //
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      //
      // Trying to update NV variable prior to the installation of EFI_VARIABLE_WRITE_ARCH_PROTOCOL
      //
      DEBUG ((DEBUG_ERROR, "Update NV variable before EFI_VARIABLE_WRITE_ARCH_PROTOCOL ready - %r\n", EFI_NOT_AVAILABLE_YET));
      return EFI_NOT_AVAILABLE_YET;
    } else if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
      //
      // Trying to update volatile authenticated variable prior to the installation of EFI_VARIABLE_WRITE_ARCH_PROTOCOL
      // The authenticated variable perhaps is not initialized, just return here.
      //
      DEBUG ((DEBUG_ERROR, "Update AUTH variable before EFI_VARIABLE_WRITE_ARCH_PROTOCOL ready - %r\n", EFI_NOT_AVAILABLE_YET));
      return EFI_NOT_AVAILABLE_YET;
    }
  }

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // Check if CacheVariable points to the variable in variable HOB.
  // If yes, let CacheVariable points to the variable in NV variable cache.
  //
  if ((CacheVariable->CurrPtr != NULL) &&
      (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) &&
      (CacheVariable->StartPtr == GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase))
      )
  {
    CacheVariable->StartPtr = GetStartPointer (mNvVariableCache);
    CacheVariable->EndPtr   = GetEndPointer (mNvVariableCache);
    CacheVariable->Volatile = FALSE;
    Status                  = FindVariableEx (VariableName, VendorGuid, FALSE, CacheVariable, AuthFormat);
    if ((CacheVariable->CurrPtr == NULL) || EFI_ERROR (Status)) {
      //
      // There is no matched variable in NV variable cache.
      //
      if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0)) {
        //
        // It is to delete variable,
        // go to delete this variable in variable HOB and
        // try to flush other variables from HOB to flash.
        //
        UpdateVariableInfo (VariableName, VendorGuid, FALSE, FALSE, FALSE, TRUE, FALSE, &gVariableInfo);
        FlushHobVariableToFlash (VariableName, VendorGuid);
        return EFI_SUCCESS;
      }
    }
  }

  if ((CacheVariable->CurrPtr == NULL) || CacheVariable->Volatile) {
    Variable = CacheVariable;
  } else {
    //
    // Update/Delete existing NV variable.
    // CacheVariable points to the variable in the memory copy of Flash area
    // Now let Variable points to the same variable in Flash area.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
    Variable            = &NvVariable;
    Variable->StartPtr  = GetStartPointer (VariableStoreHeader);
    Variable->EndPtr    = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->EndPtr - (UINTN)CacheVariable->StartPtr));

    Variable->CurrPtr = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->CurrPtr - (UINTN)CacheVariable->StartPtr));
    if (CacheVariable->InDeletedTransitionPtr != NULL) {
      Variable->InDeletedTransitionPtr = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->InDeletedTransitionPtr - (UINTN)CacheVariable->StartPtr));
    } else {
      Variable->InDeletedTransitionPtr = NULL;
    }

    Variable->Volatile = FALSE;
  }

  Fvb = mVariableModuleGlobal->FvbInstance;

  //
  // Tricky part: Use scratch data area at the end of volatile variable store
  // as a temporary storage.
  //
  NextVariable = GetEndPointer ((VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase));
  ScratchSize  = mVariableModuleGlobal->ScratchBufferSize;
  SetMem (NextVariable, ScratchSize, 0xff);
  DataReady = FALSE;

  if (Variable->CurrPtr != NULL) {
    //
    // Update/Delete existing variable.
    //
    if (AtRuntime ()) {
      //
      // If AtRuntime and the variable is Volatile and Runtime Access,
      // the volatile is ReadOnly, and SetVariable should be aborted and
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable->Volatile) {
        Status = EFI_WRITE_PROTECTED;
        goto Done;
      }

      //
      // Only variable that have NV attributes can be updated/deleted in Runtime.
      //
      if ((CacheVariable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      //
      // Only variable that have RT attributes can be updated/deleted in Runtime.
      //
      if ((CacheVariable->CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Setting a data variable with no access, or zero DataSize attributes
    // causes it to be deleted.
    // When the EFI_VARIABLE_APPEND_WRITE attribute is set, DataSize of zero will
    // not delete the variable.
    //
    if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0)) {
      if (Variable->InDeletedTransitionPtr != NULL) {
        //
        // Both ADDED and IN_DELETED_TRANSITION variable are present,
        // set IN_DELETED_TRANSITION one to DELETED state first.
        //
        ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
        State  = CacheVariable->InDeletedTransitionPtr->State;
        State &= VAR_DELETED;
        Status = UpdateVariableStore (
                   &mVariableModuleGlobal->VariableGlobal,
                   Variable->Volatile,
                   FALSE,
                   Fvb,
                   (UINTN)&Variable->InDeletedTransitionPtr->State,
                   sizeof (UINT8),
                   &State
                   );
        if (!EFI_ERROR (Status)) {
          if (!Variable->Volatile) {
            CacheVariable->InDeletedTransitionPtr->State = State;
          }
        } else {
          goto Done;
        }
      }

      State  = CacheVariable->CurrPtr->State;
      State &= VAR_DELETED;

      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable->Volatile,
                 FALSE,
                 Fvb,
                 (UINTN)&Variable->CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (!EFI_ERROR (Status)) {
        UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, FALSE, TRUE, FALSE, &gVariableInfo);
        if (!Variable->Volatile) {
          CacheVariable->CurrPtr->State = State;
          FlushHobVariableToFlash (VariableName, VendorGuid);
        }
      }

      goto Done;
    }

    //
    // If the variable is marked valid, and the same data has been passed in,
    // then return to the caller immediately.
    //
    if ((DataSizeOfVariable (CacheVariable->CurrPtr, AuthFormat) == DataSize) &&
        (CompareMem (Data, GetVariableDataPtr (CacheVariable->CurrPtr, AuthFormat), DataSize) == 0) &&
        ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) &&
        (TimeStamp == NULL))
    {
      //
      // Variable content unchanged and no need to update timestamp, just return.
      //
      UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, TRUE, FALSE, FALSE, &gVariableInfo);
      Status = EFI_SUCCESS;
      goto Done;
    } else if ((CacheVariable->CurrPtr->State == VAR_ADDED) ||
               (CacheVariable->CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)))
    {
      //
      // EFI_VARIABLE_APPEND_WRITE attribute only effects for existing variable.
      //
      if ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0) {
        //
        // NOTE: From 0 to DataOffset of NextVariable is reserved for Variable Header and Name.
        // From DataOffset of NextVariable is to save the existing variable data.
        //
        DataOffset     = GetVariableDataOffset (CacheVariable->CurrPtr, AuthFormat);
        BufferForMerge = (UINT8 *)((UINTN)NextVariable + DataOffset);
        CopyMem (
          BufferForMerge,
          (UINT8 *)((UINTN)CacheVariable->CurrPtr + DataOffset),
          DataSizeOfVariable (CacheVariable->CurrPtr, AuthFormat)
          );

        //
        // Set Max Auth/Non-Volatile/Volatile Variable Data Size as default MaxDataSize.
        //
        if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
          MaxDataSize = mVariableModuleGlobal->MaxAuthVariableSize - DataOffset;
        } else if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
          MaxDataSize = mVariableModuleGlobal->MaxVariableSize - DataOffset;
        } else {
          MaxDataSize = mVariableModuleGlobal->MaxVolatileVariableSize - DataOffset;
        }

        //
        // Append the new data to the end of existing data.
        // Max Harware error record variable data size is different from common/auth variable.
        //
        if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          MaxDataSize = PcdGet32 (PcdMaxHardwareErrorVariableSize) - DataOffset;
        }

        if (DataSizeOfVariable (CacheVariable->CurrPtr, AuthFormat) + DataSize > MaxDataSize) {
          //
          // Existing data size + new data size exceed maximum variable size limitation.
          //
          Status = EFI_INVALID_PARAMETER;
          goto Done;
        }

        CopyMem (
          (UINT8 *)(
                    (UINTN)BufferForMerge + DataSizeOfVariable (CacheVariable->CurrPtr, AuthFormat)
                    ),
          Data,
          DataSize
          );
        MergedBufSize = DataSizeOfVariable (CacheVariable->CurrPtr, AuthFormat) +
                        DataSize;

        //
        // BufferForMerge(from DataOffset of NextVariable) has included the merged existing and new data.
        //
        Data      = BufferForMerge;
        DataSize  = MergedBufSize;
        DataReady = TRUE;
      }

      //
      // Mark the old variable as in delete transition.
      //
      State  = CacheVariable->CurrPtr->State;
      State &= VAR_IN_DELETED_TRANSITION;

      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable->Volatile,
                 FALSE,
                 Fvb,
                 (UINTN)&Variable->CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      if (!Variable->Volatile) {
        CacheVariable->CurrPtr->State = State;
      }
    }
  } else {
    //
    // Not found existing variable. Create a new variable.
    //

    if ((DataSize == 0) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0)) {
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with zero DataSize or no access attributes means to delete it.
    //
    if ((DataSize == 0) || ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0)) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Only variable have NV|RT attribute can be created in Runtime.
    //
    if (AtRuntime () &&
        (((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) || ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)))
    {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
  }

  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.
  //
  NextVariable->StartId = VARIABLE_DATA;
  //
  // NextVariable->State = VAR_ADDED;
  //
  NextVariable->Reserved = 0;
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    AuthVariable                 = (AUTHENTICATED_VARIABLE_HEADER *)NextVariable;
    AuthVariable->PubKeyIndex    = KeyIndex;
    AuthVariable->MonotonicCount = MonotonicCount;
    ZeroMem (&AuthVariable->TimeStamp, sizeof (EFI_TIME));

    if (((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) &&
        (TimeStamp != NULL))
    {
      if ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) {
        CopyMem (&AuthVariable->TimeStamp, TimeStamp, sizeof (EFI_TIME));
      } else {
        //
        // In the case when the EFI_VARIABLE_APPEND_WRITE attribute is set, only
        // when the new TimeStamp value is later than the current timestamp associated
        // with the variable, we need associate the new timestamp with the updated value.
        //
        if (Variable->CurrPtr != NULL) {
          if (VariableCompareTimeStampInternal (&(((AUTHENTICATED_VARIABLE_HEADER *)CacheVariable->CurrPtr)->TimeStamp), TimeStamp)) {
            CopyMem (&AuthVariable->TimeStamp, TimeStamp, sizeof (EFI_TIME));
          } else {
            CopyMem (&AuthVariable->TimeStamp, &(((AUTHENTICATED_VARIABLE_HEADER *)CacheVariable->CurrPtr)->TimeStamp), sizeof (EFI_TIME));
          }
        }
      }
    }
  }

  //
  // The EFI_VARIABLE_APPEND_WRITE attribute will never be set in the returned
  // Attributes bitmask parameter of a GetVariable() call.
  //
  NextVariable->Attributes = Attributes & (~EFI_VARIABLE_APPEND_WRITE);

  VarNameOffset = GetVariableHeaderSize (AuthFormat);
  VarNameSize   = StrSize (VariableName);
  CopyMem (
    (UINT8 *)((UINTN)NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);

  //
  // If DataReady is TRUE, it means the variable data has been saved into
  // NextVariable during EFI_VARIABLE_APPEND_WRITE operation preparation.
  //
  if (!DataReady) {
    CopyMem (
      (UINT8 *)((UINTN)NextVariable + VarDataOffset),
      Data,
      DataSize
      );
  }

  CopyMem (
    GetVendorGuidPtr (NextVariable, AuthFormat),
    VendorGuid,
    sizeof (EFI_GUID)
    );
  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->DataSize should not include pad size so that variable
  // service can get actual size in GetVariable.
  //
  SetNameSizeOfVariable (NextVariable, VarNameSize, AuthFormat);
  SetDataSizeOfVariable (NextVariable, DataSize, AuthFormat);

  //
  // The actual size of the variable that stores in storage should
  // include pad size.
  //
  VarSize = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);
  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    //
    // Create a nonvolatile variable.
    //
    Volatile = FALSE;

    IsCommonVariable     = FALSE;
    IsCommonUserVariable = FALSE;
    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) {
      IsCommonVariable     = TRUE;
      IsCommonUserVariable = IsUserVariable (NextVariable);
    }

    if (  (  ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0)
          && ((VarSize + mVariableModuleGlobal->HwErrVariableTotalSize) > PcdGet32 (PcdHwErrStorageSize)))
       || (IsCommonVariable && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > mVariableModuleGlobal->CommonVariableSpace))
       || (IsCommonVariable && AtRuntime () && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > mVariableModuleGlobal->CommonRuntimeVariableSpace))
       || (IsCommonUserVariable && ((VarSize + mVariableModuleGlobal->CommonUserVariableTotalSize) > mVariableModuleGlobal->CommonMaxUserVariableSpace)))
    {
      if (AtRuntime ()) {
        if (IsCommonUserVariable && ((VarSize + mVariableModuleGlobal->CommonUserVariableTotalSize) > mVariableModuleGlobal->CommonMaxUserVariableSpace)) {
          RecordVarErrorFlag (VAR_ERROR_FLAG_USER_ERROR, VariableName, VendorGuid, Attributes, VarSize);
        }

        if (IsCommonVariable && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > mVariableModuleGlobal->CommonRuntimeVariableSpace)) {
          RecordVarErrorFlag (VAR_ERROR_FLAG_SYSTEM_ERROR, VariableName, VendorGuid, Attributes, VarSize);
        }

        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      //
      // Perform garbage collection & reclaim operation, and integrate the new variable at the same time.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
                 &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                 FALSE,
                 Variable,
                 NextVariable,
                 HEADER_ALIGN (VarSize)
                 );
      if (!EFI_ERROR (Status)) {
        //
        // The new variable has been integrated successfully during reclaiming.
        //
        if (Variable->CurrPtr != NULL) {
          CacheVariable->CurrPtr                = (VARIABLE_HEADER *)((UINTN)CacheVariable->StartPtr + ((UINTN)Variable->CurrPtr - (UINTN)Variable->StartPtr));
          CacheVariable->InDeletedTransitionPtr = NULL;
        }

        UpdateVariableInfo (VariableName, VendorGuid, FALSE, FALSE, TRUE, FALSE, FALSE, &gVariableInfo);
        FlushHobVariableToFlash (VariableName, VendorGuid);
      } else {
        if (IsCommonUserVariable && ((VarSize + mVariableModuleGlobal->CommonUserVariableTotalSize) > mVariableModuleGlobal->CommonMaxUserVariableSpace)) {
          RecordVarErrorFlag (VAR_ERROR_FLAG_USER_ERROR, VariableName, VendorGuid, Attributes, VarSize);
        }

        if (IsCommonVariable && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > mVariableModuleGlobal->CommonVariableSpace)) {
          RecordVarErrorFlag (VAR_ERROR_FLAG_SYSTEM_ERROR, VariableName, VendorGuid, Attributes, VarSize);
        }
      }

      goto Done;
    }

    if (!mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
      //
      // Four steps
      // 1. Write variable header
      // 2. Set variable state to header valid
      // 3. Write variable data
      // 4. Set variable state to valid
      //
      //
      // Step 1:
      //
      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 FALSE,
                 TRUE,
                 Fvb,
                 mVariableModuleGlobal->NonVolatileLastVariableOffset,
                 (UINT32)GetVariableHeaderSize (AuthFormat),
                 (UINT8 *)NextVariable
                 );

      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Step 2:
      //
      NextVariable->State = VAR_HEADER_VALID_ONLY;
      Status              = UpdateVariableStore (
                              &mVariableModuleGlobal->VariableGlobal,
                              FALSE,
                              TRUE,
                              Fvb,
                              mVariableModuleGlobal->NonVolatileLastVariableOffset + OFFSET_OF (VARIABLE_HEADER, State),
                              sizeof (UINT8),
                              &NextVariable->State
                              );

      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Step 3:
      //
      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 FALSE,
                 TRUE,
                 Fvb,
                 mVariableModuleGlobal->NonVolatileLastVariableOffset + GetVariableHeaderSize (AuthFormat),
                 (UINT32)(VarSize - GetVariableHeaderSize (AuthFormat)),
                 (UINT8 *)NextVariable + GetVariableHeaderSize (AuthFormat)
                 );

      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Step 4:
      //
      NextVariable->State = VAR_ADDED;
      Status              = UpdateVariableStore (
                              &mVariableModuleGlobal->VariableGlobal,
                              FALSE,
                              TRUE,
                              Fvb,
                              mVariableModuleGlobal->NonVolatileLastVariableOffset + OFFSET_OF (VARIABLE_HEADER, State),
                              sizeof (UINT8),
                              &NextVariable->State
                              );

      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Update the memory copy of Flash region.
      //
      CopyMem ((UINT8 *)mNvVariableCache + mVariableModuleGlobal->NonVolatileLastVariableOffset, (UINT8 *)NextVariable, VarSize);
    } else {
      //
      // Emulated non-volatile variable mode.
      //
      NextVariable->State = VAR_ADDED;
      Status              = UpdateVariableStore (
                              &mVariableModuleGlobal->VariableGlobal,
                              FALSE,
                              TRUE,
                              Fvb,
                              mVariableModuleGlobal->NonVolatileLastVariableOffset,
                              (UINT32)VarSize,
                              (UINT8 *)NextVariable
                              );

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset += HEADER_ALIGN (VarSize);

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
      mVariableModuleGlobal->HwErrVariableTotalSize += HEADER_ALIGN (VarSize);
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += HEADER_ALIGN (VarSize);
      if (IsCommonUserVariable) {
        mVariableModuleGlobal->CommonUserVariableTotalSize += HEADER_ALIGN (VarSize);
      }
    }
  } else {
    //
    // Create a volatile variable.
    //
    Volatile = TRUE;

    if ((UINT32)(VarSize + mVariableModuleGlobal->VolatileLastVariableOffset) >
        ((VARIABLE_STORE_HEADER *)((UINTN)(mVariableModuleGlobal->VariableGlobal.VolatileVariableBase)))->Size)
    {
      //
      // Perform garbage collection & reclaim operation, and integrate the new variable at the same time.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.VolatileVariableBase,
                 &mVariableModuleGlobal->VolatileLastVariableOffset,
                 TRUE,
                 Variable,
                 NextVariable,
                 HEADER_ALIGN (VarSize)
                 );
      if (!EFI_ERROR (Status)) {
        //
        // The new variable has been integrated successfully during reclaiming.
        //
        if (Variable->CurrPtr != NULL) {
          CacheVariable->CurrPtr                = (VARIABLE_HEADER *)((UINTN)CacheVariable->StartPtr + ((UINTN)Variable->CurrPtr - (UINTN)Variable->StartPtr));
          CacheVariable->InDeletedTransitionPtr = NULL;
        }

        UpdateVariableInfo (VariableName, VendorGuid, TRUE, FALSE, TRUE, FALSE, FALSE, &gVariableInfo);
      }

      goto Done;
    }

    NextVariable->State = VAR_ADDED;
    Status              = UpdateVariableStore (
                            &mVariableModuleGlobal->VariableGlobal,
                            TRUE,
                            TRUE,
                            Fvb,
                            mVariableModuleGlobal->VolatileLastVariableOffset,
                            (UINT32)VarSize,
                            (UINT8 *)NextVariable
                            );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    mVariableModuleGlobal->VolatileLastVariableOffset += HEADER_ALIGN (VarSize);
  }

  //
  // Mark the old variable as deleted.
  //
  if (!EFI_ERROR (Status) && (Variable->CurrPtr != NULL)) {
    if (Variable->InDeletedTransitionPtr != NULL) {
      //
      // Both ADDED and IN_DELETED_TRANSITION old variable are present,
      // set IN_DELETED_TRANSITION one to DELETED state first.
      //
      ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
      State  = CacheVariable->InDeletedTransitionPtr->State;
      State &= VAR_DELETED;
      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable->Volatile,
                 FALSE,
                 Fvb,
                 (UINTN)&Variable->InDeletedTransitionPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (!EFI_ERROR (Status)) {
        if (!Variable->Volatile) {
          CacheVariable->InDeletedTransitionPtr->State = State;
        }
      } else {
        goto Done;
      }
    }

    State  = CacheVariable->CurrPtr->State;
    State &= VAR_DELETED;

    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               Variable->Volatile,
               FALSE,
               Fvb,
               (UINTN)&Variable->CurrPtr->State,
               sizeof (UINT8),
               &State
               );
    if (!EFI_ERROR (Status) && !Variable->Volatile) {
      CacheVariable->CurrPtr->State = State;
    }
  }

  if (!EFI_ERROR (Status)) {
    UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE, &gVariableInfo);
    if (!Volatile) {
      FlushHobVariableToFlash (VariableName, VendorGuid);
    }
  }

Done:
  if (!EFI_ERROR (Status)) {
    if (((Variable->CurrPtr != NULL) && !Variable->Volatile) || ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0)) {
      VolatileCacheInstance = &(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeNvCache);
    } else {
      VolatileCacheInstance = &(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeVolatileCache);
    }

    if (VolatileCacheInstance->Store != NULL) {
      Status =  SynchronizeRuntimeVariableCache (
                  VolatileCacheInstance,
                  0,
                  VolatileCacheInstance->Store->Size
                  );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return Status;
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if ((Variable.CurrPtr == NULL) || EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data size
  //
  VarDataSize = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  ASSERT (VarDataSize != 0);

  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    CopyMem (Data, GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat), VarDataSize);

    *DataSize = VarDataSize;
    UpdateVariableInfo (VariableName, VendorGuid, Variable.Volatile, TRUE, FALSE, FALSE, FALSE, &gVariableInfo);

    Status = EFI_SUCCESS;
    goto Done;
  } else {
    *DataSize = VarDataSize;
    Status    = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  if ((Status == EFI_SUCCESS) || (Status == EFI_BUFFER_TOO_SMALL)) {
    if ((Attributes != NULL) && (Variable.CurrPtr != NULL)) {
      *Attributes = Variable.CurrPtr->Attributes;
    }
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**

  This code Finds the Next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param VariableNameSize           The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The VariableNameSize is too small for the result.
                                    VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER     VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     The input values of VariableName and VendorGuid are not a name and
                                    GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER     Null-terminator is not found in the first VariableNameSize bytes of
                                    the input VariableName buffer.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS             Status;
  UINTN                  MaxLen;
  UINTN                  VarNameSize;
  BOOLEAN                AuthFormat;
  VARIABLE_HEADER        *VariablePtr;
  VARIABLE_STORE_HEADER  *VariableStoreHeader[VariableStoreTypeMax];

  if ((VariableNameSize == NULL) || (VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // Calculate the possible maximum length of name string, including the Null terminator.
  //
  MaxLen = *VariableNameSize / sizeof (CHAR16);
  if ((MaxLen == 0) || (StrnLenS (VariableName, MaxLen) == MaxLen)) {
    //
    // Null-terminator is not found in the first VariableNameSize bytes of the input VariableName buffer,
    // follow spec to return EFI_INVALID_PARAMETER.
    //
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // 0: Volatile, 1: HOB, 2: Non-Volatile.
  // The index and attributes mapping must be kept in this order as FindVariable
  // makes use of this mapping to implement search algorithm.
  //
  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  Status =  VariableServiceGetNextVariableInternal (
              VariableName,
              VendorGuid,
              VariableStoreHeader,
              &VariablePtr,
              AuthFormat
              );
  if (!EFI_ERROR (Status)) {
    VarNameSize = NameSizeOfVariable (VariablePtr, AuthFormat);
    ASSERT (VarNameSize != 0);
    if (VarNameSize <= *VariableNameSize) {
      CopyMem (
        VariableName,
        GetVariableNamePtr (VariablePtr, AuthFormat),
        VarNameSize
        );
      CopyMem (
        VendorGuid,
        GetVendorGuidPtr (VariablePtr, AuthFormat),
        sizeof (EFI_GUID)
        );
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *VariableNameSize = VarNameSize;
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
VariableServiceSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  EFI_PHYSICAL_ADDRESS    Point;
  UINTN                   PayloadSize;
  BOOLEAN                 AuthFormat;

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // Check input parameters.
  //
  if ((VariableName == NULL) || (VariableName[0] == 0) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataSize != 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for reserverd bit in variable attribute.
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated but we still allow
  // the delete operation of common authenticated variable at user physical presence.
  // So leave EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute check to AuthVariableLib
  //
  if ((Attributes & (~(EFI_VARIABLE_ATTRIBUTES_MASK | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS))) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the combination of attribute bits is valid.
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      return EFI_UNSUPPORTED;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  } else if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE) {
    //
    // Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
    if (!mVariableModuleGlobal->VariableGlobal.AuthSupport) {
      //
      // Not support authenticated variable write.
      //
      return EFI_INVALID_PARAMETER;
    }
  } else if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    if (PcdGet32 (PcdHwErrStorageSize) == 0) {
      //
      // Not support harware error record variable variable.
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS and EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute
  // cannot be set both.
  //
  if (  ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
     && ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS))
  {
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    //
    //  If DataSize == AUTHINFO_SIZE and then PayloadSize is 0.
    //  Maybe it's the delete operation of common authenticated variable at user physical presence.
    //
    if (DataSize != AUTHINFO_SIZE) {
      return EFI_UNSUPPORTED;
    }

    PayloadSize = DataSize - AUTHINFO_SIZE;
  } else if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
    //
    // Sanity check for EFI_VARIABLE_AUTHENTICATION_2 descriptor.
    //
    if ((DataSize < OFFSET_OF_AUTHINFO2_CERT_DATA) ||
        (((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->AuthInfo.Hdr.dwLength > DataSize - (OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo))) ||
        (((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->AuthInfo.Hdr.dwLength < OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)))
    {
      return EFI_SECURITY_VIOLATION;
    }

    //
    // The VariableSpeculationBarrier() call here is to ensure the above sanity
    // check for the EFI_VARIABLE_AUTHENTICATION_2 descriptor has been completed
    // before the execution of subsequent codes.
    //
    VariableSpeculationBarrier ();
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
  } else {
    PayloadSize = DataSize;
  }

  if ((UINTN)(~0) - PayloadSize < StrSize (VariableName)) {
    //
    // Prevent whole variable size overflow
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec#### variable.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if (StrSize (VariableName) + PayloadSize >
        PcdGet32 (PcdMaxHardwareErrorVariableSize) - GetVariableHeaderSize (AuthFormat))
    {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    //  The size of the VariableName, including the Unicode Null in bytes plus
    //  the DataSize is limited to maximum size of Max(Auth|Volatile)VariableSize bytes.
    //
    if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
      if (StrSize (VariableName) + PayloadSize >
          mVariableModuleGlobal->MaxAuthVariableSize -
          GetVariableHeaderSize (AuthFormat))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to set variable '%s' with Guid %g\n",
          __FUNCTION__,
          VariableName,
          VendorGuid
          ));
        DEBUG ((
          DEBUG_ERROR,
          "NameSize(0x%x) + PayloadSize(0x%x) > "
          "MaxAuthVariableSize(0x%x) - HeaderSize(0x%x)\n",
          StrSize (VariableName),
          PayloadSize,
          mVariableModuleGlobal->MaxAuthVariableSize,
          GetVariableHeaderSize (AuthFormat)
          ));
        return EFI_INVALID_PARAMETER;
      }
    } else if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      if (StrSize (VariableName) + PayloadSize >
          mVariableModuleGlobal->MaxVariableSize - GetVariableHeaderSize (AuthFormat))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to set variable '%s' with Guid %g\n",
          __FUNCTION__,
          VariableName,
          VendorGuid
          ));
        DEBUG ((
          DEBUG_ERROR,
          "NameSize(0x%x) + PayloadSize(0x%x) > "
          "MaxVariableSize(0x%x) - HeaderSize(0x%x)\n",
          StrSize (VariableName),
          PayloadSize,
          mVariableModuleGlobal->MaxVariableSize,
          GetVariableHeaderSize (AuthFormat)
          ));
        return EFI_INVALID_PARAMETER;
      }
    } else {
      if (StrSize (VariableName) + PayloadSize >
          mVariableModuleGlobal->MaxVolatileVariableSize - GetVariableHeaderSize (AuthFormat))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to set variable '%s' with Guid %g\n",
          __FUNCTION__,
          VariableName,
          VendorGuid
          ));
        DEBUG ((
          DEBUG_ERROR,
          "NameSize(0x%x) + PayloadSize(0x%x) > "
          "MaxVolatileVariableSize(0x%x) - HeaderSize(0x%x)\n",
          StrSize (VariableName),
          PayloadSize,
          mVariableModuleGlobal->MaxVolatileVariableSize,
          GetVariableHeaderSize (AuthFormat)
          ));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  //
  // Special Handling for MOR Lock variable.
  //
  Status = SetVariableCheckHandlerMor (VariableName, VendorGuid, Attributes, PayloadSize, (VOID *)((UINTN)Data + DataSize - PayloadSize));
  if (Status == EFI_ALREADY_STARTED) {
    //
    // EFI_ALREADY_STARTED means the SetVariable() action is handled inside of SetVariableCheckHandlerMor().
    // Variable driver can just return SUCCESS.
    //
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VarCheckLibSetVariableCheck (VariableName, VendorGuid, Attributes, PayloadSize, (VOID *)((UINTN)Data + DataSize - PayloadSize), mRequestSource);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Consider reentrant in MCA/INIT/NMI. It needs be reupdated.
  //
  if (1 < InterlockedIncrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState)) {
    Point = mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    //
    // Parse non-volatile variable data and get last variable offset.
    //
    NextVariable = GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)Point);
    while (IsValidVariableHeader (NextVariable, GetEndPointer ((VARIABLE_STORE_HEADER *)(UINTN)Point))) {
      NextVariable = GetNextVariablePtr (NextVariable, AuthFormat);
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN)NextVariable - (UINTN)Point;
  }

  //
  // Check whether the input variable is already existed.
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, TRUE);
  if (!EFI_ERROR (Status)) {
    if (((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) && AtRuntime ()) {
      Status = EFI_WRITE_PROTECTED;
      goto Done;
    }

    if ((Attributes != 0) && ((Attributes & (~EFI_VARIABLE_APPEND_WRITE)) != Variable.CurrPtr->Attributes)) {
      //
      // If a preexisting variable is rewritten with different attributes, SetVariable() shall not
      // modify the variable and shall return EFI_INVALID_PARAMETER. Two exceptions to this rule:
      // 1. No access attributes specified
      // 2. The only attribute differing is EFI_VARIABLE_APPEND_WRITE
      //
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((DEBUG_INFO, "[Variable]: Rewritten a preexisting variable(0x%08x) with different attributes(0x%08x) - %g:%s\n", Variable.CurrPtr->Attributes, Attributes, VendorGuid, VariableName));
      goto Done;
    }
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
    //
    // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang.
    //
    Status = AutoUpdateLangVariable (VariableName, Data, DataSize);
    if (EFI_ERROR (Status)) {
      //
      // The auto update operation failed, directly return to avoid inconsistency between PlatformLang and Lang.
      //
      goto Done;
    }
  }

  if (mVariableModuleGlobal->VariableGlobal.AuthSupport) {
    Status = AuthVariableLibProcessVariable (VariableName, VendorGuid, Data, DataSize, Attributes);
  } else {
    Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, 0, 0, &Variable, NULL);
  }

Done:
  InterlockedDecrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState);
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  if (!AtRuntime ()) {
    if (!EFI_ERROR (Status)) {
      SecureBootHook (
        VariableName,
        VendorGuid
        );
    }
  }

  return Status;
}

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_SUCCESS                   Query successfully.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfoInternal (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  VARIABLE_HEADER         *Variable;
  VARIABLE_HEADER         *NextVariable;
  UINT64                  VariableSize;
  VARIABLE_STORE_HEADER   *VariableStoreHeader;
  UINT64                  CommonVariableTotalSize;
  UINT64                  HwErrVariableTotalSize;
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;

  CommonVariableTotalSize = 0;
  HwErrVariableTotalSize  = 0;

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = mNvVariableCache;
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size).
  //
  *MaximumVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = PcdGet32 (PcdHwErrStorageSize);
    *MaximumVariableSize        =  PcdGet32 (PcdMaxHardwareErrorVariableSize) -
                                  GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      if (AtRuntime ()) {
        *MaximumVariableStorageSize = mVariableModuleGlobal->CommonRuntimeVariableSpace;
      } else {
        *MaximumVariableStorageSize = mVariableModuleGlobal->CommonVariableSpace;
      }
    }

    //
    // Let *MaximumVariableSize be Max(Auth|Volatile)VariableSize with the exception of the variable header size.
    //
    if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
      *MaximumVariableSize =  mVariableModuleGlobal->MaxAuthVariableSize -
                             GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    } else if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      *MaximumVariableSize =  mVariableModuleGlobal->MaxVariableSize -
                             GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    } else {
      *MaximumVariableSize =   mVariableModuleGlobal->MaxVolatileVariableSize -
                             GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    }
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);

  //
  // Now walk through the related variable store.
  //
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))) {
    NextVariable = GetNextVariablePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    VariableSize = (UINT64)(UINTN)NextVariable - (UINT64)(UINTN)Variable;

    if (AtRuntime ()) {
      //
      // We don't take the state of the variables in mind
      // when calculating RemainingVariableStorageSize,
      // since the space occupied by variables not marked with
      // VAR_ADDED is not allowed to be reclaimed in Runtime.
      //
      if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        HwErrVariableTotalSize += VariableSize;
      } else {
        CommonVariableTotalSize += VariableSize;
      }
    } else {
      //
      // Only care about Variables with State VAR_ADDED, because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if (Variable->State == VAR_ADDED) {
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          HwErrVariableTotalSize += VariableSize;
        } else {
          CommonVariableTotalSize += VariableSize;
        }
      } else if (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
        //
        // If it is a IN_DELETED_TRANSITION variable,
        // and there is not also a same ADDED one at the same time,
        // this IN_DELETED_TRANSITION variable is valid.
        //
        VariablePtrTrack.StartPtr = GetStartPointer (VariableStoreHeader);
        VariablePtrTrack.EndPtr   = GetEndPointer (VariableStoreHeader);
        Status                    = FindVariableEx (
                                      GetVariableNamePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
                                      GetVendorGuidPtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
                                      FALSE,
                                      &VariablePtrTrack,
                                      mVariableModuleGlobal->VariableGlobal.AuthFormat
                                      );
        if (!EFI_ERROR (Status) && (VariablePtrTrack.CurrPtr->State != VAR_ADDED)) {
          if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
            HwErrVariableTotalSize += VariableSize;
          } else {
            CommonVariableTotalSize += VariableSize;
          }
        }
      }
    }

    //
    // Go to the next one.
    //
    Variable = NextVariable;
  }

  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  } else {
    if (*MaximumVariableStorageSize < CommonVariableTotalSize) {
      *RemainingVariableStorageSize = 0;
    } else {
      *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
    }
  }

  if (*RemainingVariableStorageSize < GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat)) <
             *MaximumVariableSize
             )
  {
    *MaximumVariableSize = *RemainingVariableStorageSize -
                           GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  return EFI_SUCCESS;
}

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  EFI_STATUS  Status;

  if ((MaximumVariableStorageSize == NULL) || (RemainingVariableStorageSize == NULL) || (MaximumVariableSize == NULL) || (Attributes == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    //  Deprecated attribute, make this check as highest priority.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;
  } else if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE) {
    //
    // Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if (AtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    //
    // Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
    if (!mVariableModuleGlobal->VariableGlobal.AuthSupport) {
      //
      // Not support authenticated variable write.
      //
      return EFI_UNSUPPORTED;
    }
  } else if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    if (PcdGet32 (PcdHwErrStorageSize) == 0) {
      //
      // Not support harware error record variable variable.
      //
      return EFI_UNSUPPORTED;
    }
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = VariableServiceQueryVariableInfoInternal (
             Attributes,
             MaximumVariableStorageSize,
             RemainingVariableStorageSize,
             MaximumVariableSize
             );

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**
  This function reclaims variable storage if free size is below the threshold.

  Caution: This function may be invoked at SMM mode.
  Care must be taken to make sure not security issue.

**/
VOID
ReclaimForOS (
  VOID
  )
{
  EFI_STATUS      Status;
  UINTN           RemainingCommonRuntimeVariableSpace;
  UINTN           RemainingHwErrVariableSpace;
  STATIC BOOLEAN  Reclaimed;

  //
  // This function will be called only once at EndOfDxe or ReadyToBoot event.
  //
  if (Reclaimed) {
    return;
  }

  Reclaimed = TRUE;

  Status = EFI_SUCCESS;

  if (mVariableModuleGlobal->CommonRuntimeVariableSpace < mVariableModuleGlobal->CommonVariableTotalSize) {
    RemainingCommonRuntimeVariableSpace = 0;
  } else {
    RemainingCommonRuntimeVariableSpace = mVariableModuleGlobal->CommonRuntimeVariableSpace - mVariableModuleGlobal->CommonVariableTotalSize;
  }

  RemainingHwErrVariableSpace = PcdGet32 (PcdHwErrStorageSize) - mVariableModuleGlobal->HwErrVariableTotalSize;

  //
  // Check if the free area is below a threshold.
  //
  if (((RemainingCommonRuntimeVariableSpace < mVariableModuleGlobal->MaxVariableSize) ||
       (RemainingCommonRuntimeVariableSpace < mVariableModuleGlobal->MaxAuthVariableSize)) ||
      ((PcdGet32 (PcdHwErrStorageSize) != 0) &&
       (RemainingHwErrVariableSpace < PcdGet32 (PcdMaxHardwareErrorVariableSize))))
  {
    Status = Reclaim (
               mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
               &mVariableModuleGlobal->NonVolatileLastVariableOffset,
               FALSE,
               NULL,
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Get maximum variable size, covering both non-volatile and volatile variables.

  @return Maximum variable size.

**/
UINTN
GetMaxVariableSize (
  VOID
  )
{
  UINTN  MaxVariableSize;

  MaxVariableSize = GetNonVolatileMaxVariableSize ();
  //
  // The condition below fails implicitly if PcdMaxVolatileVariableSize equals
  // the default zero value.
  //
  if (MaxVariableSize < PcdGet32 (PcdMaxVolatileVariableSize)) {
    MaxVariableSize = PcdGet32 (PcdMaxVolatileVariableSize);
  }

  return MaxVariableSize;
}

/**
  Flush the HOB variable to flash.

  @param[in] VariableName       Name of variable has been updated or deleted.
  @param[in] VendorGuid         Guid of variable has been updated or deleted.

**/
VOID
FlushHobVariableToFlash (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS              Status;
  VARIABLE_STORE_HEADER   *VariableStoreHeader;
  VARIABLE_HEADER         *Variable;
  VOID                    *VariableData;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;
  BOOLEAN                 ErrorFlag;
  BOOLEAN                 AuthFormat;

  ErrorFlag  = FALSE;
  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // Flush the HOB variable to flash.
  //
  if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
    //
    // Set HobVariableBase to 0, it can avoid SetVariable to call back.
    //
    mVariableModuleGlobal->VariableGlobal.HobVariableBase = 0;
    for ( Variable = GetStartPointer (VariableStoreHeader)
          ; IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader))
          ; Variable = GetNextVariablePtr (Variable, AuthFormat)
          )
    {
      if (Variable->State != VAR_ADDED) {
        //
        // The HOB variable has been set to DELETED state in local.
        //
        continue;
      }

      ASSERT ((Variable->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0);
      if ((VendorGuid == NULL) || (VariableName == NULL) ||
          !CompareGuid (VendorGuid, GetVendorGuidPtr (Variable, AuthFormat)) ||
          (StrCmp (VariableName, GetVariableNamePtr (Variable, AuthFormat)) != 0))
      {
        VariableData = GetVariableDataPtr (Variable, AuthFormat);
        FindVariable (
          GetVariableNamePtr (Variable, AuthFormat),
          GetVendorGuidPtr (Variable, AuthFormat),
          &VariablePtrTrack,
          &mVariableModuleGlobal->VariableGlobal,
          FALSE
          );
        Status = UpdateVariable (
                   GetVariableNamePtr (Variable, AuthFormat),
                   GetVendorGuidPtr (Variable, AuthFormat),
                   VariableData,
                   DataSizeOfVariable (Variable, AuthFormat),
                   Variable->Attributes,
                   0,
                   0,
                   &VariablePtrTrack,
                   NULL
                   );
        DEBUG ((
          DEBUG_INFO,
          "Variable driver flush the HOB variable to flash: %g %s %r\n",
          GetVendorGuidPtr (Variable, AuthFormat),
          GetVariableNamePtr (Variable, AuthFormat),
          Status
          ));
      } else {
        //
        // The updated or deleted variable is matched with this HOB variable.
        // Don't break here because we will try to set other HOB variables
        // since this variable could be set successfully.
        //
        Status = EFI_SUCCESS;
      }

      if (!EFI_ERROR (Status)) {
        //
        // If set variable successful, or the updated or deleted variable is matched with the HOB variable,
        // set the HOB variable to DELETED state in local.
        //
        DEBUG ((
          DEBUG_INFO,
          "Variable driver set the HOB variable to DELETED state in local: %g %s\n",
          GetVendorGuidPtr (Variable, AuthFormat),
          GetVariableNamePtr (Variable, AuthFormat)
          ));
        Variable->State &= VAR_DELETED;
      } else {
        ErrorFlag = TRUE;
      }
    }

    if (mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeHobCache.Store != NULL) {
      Status =  SynchronizeRuntimeVariableCache (
                  &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeHobCache,
                  0,
                  mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeHobCache.Store->Size
                  );
      ASSERT_EFI_ERROR (Status);
    }

    if (ErrorFlag) {
      //
      // We still have HOB variable(s) not flushed in flash.
      //
      mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VariableStoreHeader;
    } else {
      //
      // All HOB variables have been flushed in flash.
      //
      DEBUG ((DEBUG_INFO, "Variable driver: all HOB variables have been flushed in flash.\n"));
      if (mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.HobFlushComplete != NULL) {
        *(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.HobFlushComplete) = TRUE;
      }

      if (!AtRuntime ()) {
        FreePool ((VOID *)VariableStoreHeader);
      }
    }
  }
}

/**
  Initializes variable write service.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
VariableWriteServiceInitialize (
  VOID
  )
{
  EFI_STATUS               Status;
  UINTN                    Index;
  UINT8                    Data;
  VARIABLE_ENTRY_PROPERTY  *VariableEntry;

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Check if the free area is really free.
  //
  for (Index = mVariableModuleGlobal->NonVolatileLastVariableOffset; Index < mNvVariableCache->Size; Index++) {
    Data = ((UINT8 *)mNvVariableCache)[Index];
    if (Data != 0xff) {
      //
      // There must be something wrong in variable store, do reclaim operation.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
                 &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                 FALSE,
                 NULL,
                 NULL,
                 0
                 );
      if (EFI_ERROR (Status)) {
        ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
        return Status;
      }

      break;
    }
  }

  FlushHobVariableToFlash (NULL, NULL);

  Status = EFI_SUCCESS;
  ZeroMem (&mAuthContextOut, sizeof (mAuthContextOut));
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    //
    // Authenticated variable initialize.
    //
    mAuthContextIn.StructSize          = sizeof (AUTH_VAR_LIB_CONTEXT_IN);
    mAuthContextIn.MaxAuthVariableSize =  mVariableModuleGlobal->MaxAuthVariableSize -
                                         GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    Status = AuthVariableLibInitialize (&mAuthContextIn, &mAuthContextOut);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Variable driver will work with auth variable support!\n"));
      mVariableModuleGlobal->VariableGlobal.AuthSupport = TRUE;
      if (mAuthContextOut.AuthVarEntry != NULL) {
        for (Index = 0; Index < mAuthContextOut.AuthVarEntryCount; Index++) {
          VariableEntry = &mAuthContextOut.AuthVarEntry[Index];
          Status        = VarCheckLibVariablePropertySet (
                            VariableEntry->Name,
                            VariableEntry->Guid,
                            &VariableEntry->VariableProperty
                            );
          ASSERT_EFI_ERROR (Status);
        }
      }
    } else if (Status == EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_INFO, "NOTICE - AuthVariableLibInitialize() returns %r!\n", Status));
      DEBUG ((DEBUG_INFO, "Variable driver will continue to work without auth variable support!\n"));
      mVariableModuleGlobal->VariableGlobal.AuthSupport = FALSE;
      Status                                            = EFI_SUCCESS;
    }
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < ARRAY_SIZE (mVariableEntryProperty); Index++) {
      VariableEntry = &mVariableEntryProperty[Index];
      Status        = VarCheckLibVariablePropertySet (VariableEntry->Name, VariableEntry->Guid, &VariableEntry->VariableProperty);
      ASSERT_EFI_ERROR (Status);
    }
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Initialize MOR Lock variable.
  //
  MorLockInit ();

  return Status;
}

/**
  Convert normal variable storage to the allocated auth variable storage.

  @param[in]  NormalVarStorage  Pointer to the normal variable storage header

  @retval the allocated auth variable storage
**/
VOID *
ConvertNormalVarStorageToAuthVarStorage (
  VARIABLE_STORE_HEADER  *NormalVarStorage
  )
{
  VARIABLE_HEADER                *StartPtr;
  UINT8                          *NextPtr;
  VARIABLE_HEADER                *EndPtr;
  UINTN                          AuthVarStroageSize;
  AUTHENTICATED_VARIABLE_HEADER  *AuthStartPtr;
  VARIABLE_STORE_HEADER          *AuthVarStorage;

  AuthVarStroageSize = sizeof (VARIABLE_STORE_HEADER);
  //
  // Set AuthFormat as FALSE for normal variable storage
  //
  mVariableModuleGlobal->VariableGlobal.AuthFormat = FALSE;

  //
  // Calculate Auth Variable Storage Size
  //
  StartPtr = GetStartPointer (NormalVarStorage);
  EndPtr   = GetEndPointer (NormalVarStorage);
  while (StartPtr < EndPtr) {
    if (StartPtr->State == VAR_ADDED) {
      AuthVarStroageSize  = HEADER_ALIGN (AuthVarStroageSize);
      AuthVarStroageSize += sizeof (AUTHENTICATED_VARIABLE_HEADER);
      AuthVarStroageSize += StartPtr->NameSize + GET_PAD_SIZE (StartPtr->NameSize);
      AuthVarStroageSize += StartPtr->DataSize + GET_PAD_SIZE (StartPtr->DataSize);
    }

    StartPtr = GetNextVariablePtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  //
  // Allocate Runtime memory for Auth Variable Storage
  //
  AuthVarStorage = AllocateRuntimeZeroPool (AuthVarStroageSize);
  ASSERT (AuthVarStorage != NULL);
  if (AuthVarStorage == NULL) {
    return NULL;
  }

  //
  // Copy Variable from Normal storage to Auth storage
  //
  StartPtr     = GetStartPointer (NormalVarStorage);
  EndPtr       = GetEndPointer (NormalVarStorage);
  AuthStartPtr = (AUTHENTICATED_VARIABLE_HEADER *)GetStartPointer (AuthVarStorage);
  while (StartPtr < EndPtr) {
    if (StartPtr->State == VAR_ADDED) {
      AuthStartPtr = (AUTHENTICATED_VARIABLE_HEADER *)HEADER_ALIGN (AuthStartPtr);
      //
      // Copy Variable Header
      //
      AuthStartPtr->StartId    = StartPtr->StartId;
      AuthStartPtr->State      = StartPtr->State;
      AuthStartPtr->Attributes = StartPtr->Attributes;
      AuthStartPtr->NameSize   = StartPtr->NameSize;
      AuthStartPtr->DataSize   = StartPtr->DataSize;
      CopyGuid (&AuthStartPtr->VendorGuid, &StartPtr->VendorGuid);
      //
      // Copy Variable Name
      //
      NextPtr = (UINT8 *)(AuthStartPtr + 1);
      CopyMem (
        NextPtr,
        GetVariableNamePtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat),
        AuthStartPtr->NameSize
        );
      //
      // Copy Variable Data
      //
      NextPtr = NextPtr + AuthStartPtr->NameSize + GET_PAD_SIZE (AuthStartPtr->NameSize);
      CopyMem (NextPtr, GetVariableDataPtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat), AuthStartPtr->DataSize);
      //
      // Go to next variable
      //
      AuthStartPtr = (AUTHENTICATED_VARIABLE_HEADER *)(NextPtr + AuthStartPtr->DataSize + GET_PAD_SIZE (AuthStartPtr->DataSize));
    }

    StartPtr = GetNextVariablePtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  //
  // Update Auth Storage Header
  //
  AuthVarStorage->Format = NormalVarStorage->Format;
  AuthVarStorage->State  = NormalVarStorage->State;
  AuthVarStorage->Size   = (UINT32)((UINTN)AuthStartPtr - (UINTN)AuthVarStorage);
  CopyGuid (&AuthVarStorage->Signature, &gEfiAuthenticatedVariableGuid);
  ASSERT (AuthVarStorage->Size <= AuthVarStroageSize);

  //
  // Restore AuthFormat
  //
  mVariableModuleGlobal->VariableGlobal.AuthFormat = TRUE;
  return AuthVarStorage;
}

/**
  Get HOB variable store.

  @param[in] VariableGuid       NV variable store signature.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
GetHobVariableStore (
  IN EFI_GUID  *VariableGuid
  )
{
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT64                 VariableStoreLength;
  EFI_HOB_GUID_TYPE      *GuidHob;
  BOOLEAN                NeedConvertNormalToAuth;

  //
  // Make sure there is no more than one Variable HOB.
  //
  DEBUG_CODE_BEGIN ();
  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    if ((GetNextGuidHob (&gEfiAuthenticatedVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Found two Auth Variable HOBs\n"));
      ASSERT (FALSE);
    } else if (GetFirstGuidHob (&gEfiVariableGuid) != NULL) {
      DEBUG ((DEBUG_ERROR, "ERROR: Found one Auth + one Normal Variable HOBs\n"));
      ASSERT (FALSE);
    }
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      if ((GetNextGuidHob (&gEfiVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
        DEBUG ((DEBUG_ERROR, "ERROR: Found two Normal Variable HOBs\n"));
        ASSERT (FALSE);
      }
    }
  }

  DEBUG_CODE_END ();

  //
  // Combinations supported:
  // 1. Normal NV variable store +
  //    Normal HOB variable store
  // 2. Auth NV variable store +
  //    Auth HOB variable store
  // 3. Auth NV variable store +
  //    Normal HOB variable store (code will convert it to Auth Format)
  //
  NeedConvertNormalToAuth = FALSE;
  GuidHob                 = GetFirstGuidHob (VariableGuid);
  if ((GuidHob == NULL) && (VariableGuid == &gEfiAuthenticatedVariableGuid)) {
    //
    // Try getting it from normal variable HOB
    //
    GuidHob                 = GetFirstGuidHob (&gEfiVariableGuid);
    NeedConvertNormalToAuth = TRUE;
  }

  if (GuidHob != NULL) {
    VariableStoreHeader = GET_GUID_HOB_DATA (GuidHob);
    VariableStoreLength = GuidHob->Header.HobLength - sizeof (EFI_HOB_GUID_TYPE);
    if (GetVariableStoreStatus (VariableStoreHeader) == EfiValid) {
      if (!NeedConvertNormalToAuth) {
        mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimeCopyPool ((UINTN)VariableStoreLength, (VOID *)VariableStoreHeader);
      } else {
        mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)ConvertNormalVarStorageToAuthVarStorage ((VOID *)VariableStoreHeader);
      }

      if (mVariableModuleGlobal->VariableGlobal.HobVariableBase == 0) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "HOB Variable Store header is corrupted!\n"));
    }
  }

  return EFI_SUCCESS;
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  VOID
  )
{
  EFI_STATUS             Status;
  VARIABLE_STORE_HEADER  *VolatileVariableStore;
  UINTN                  ScratchSize;
  EFI_GUID               *VariableGuid;

  //
  // Allocate runtime memory for variable driver global structure.
  //
  mVariableModuleGlobal = AllocateRuntimeZeroPool (sizeof (VARIABLE_MODULE_GLOBAL));
  if (mVariableModuleGlobal == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeLock (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock, TPL_NOTIFY);

  //
  // Init non-volatile variable store.
  //
  Status = InitNonVolatileVariableStore ();
  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    return Status;
  }

  //
  // mVariableModuleGlobal->VariableGlobal.AuthFormat
  // has been initialized in InitNonVolatileVariableStore().
  //
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    DEBUG ((DEBUG_INFO, "Variable driver will work with auth variable format!\n"));
    //
    // Set AuthSupport to FALSE first, VariableWriteServiceInitialize() will initialize it.
    //
    mVariableModuleGlobal->VariableGlobal.AuthSupport = FALSE;
    VariableGuid                                      = &gEfiAuthenticatedVariableGuid;
  } else {
    DEBUG ((DEBUG_INFO, "Variable driver will work without auth variable support!\n"));
    mVariableModuleGlobal->VariableGlobal.AuthSupport = FALSE;
    VariableGuid                                      = &gEfiVariableGuid;
  }

  //
  // Get HOB variable store.
  //
  Status = GetHobVariableStore (VariableGuid);
  if (EFI_ERROR (Status)) {
    if (mNvFvHeaderCache != NULL) {
      FreePool (mNvFvHeaderCache);
    }

    FreePool (mVariableModuleGlobal);
    return Status;
  }

  mVariableModuleGlobal->MaxVolatileVariableSize = ((PcdGet32 (PcdMaxVolatileVariableSize) != 0) ?
                                                    PcdGet32 (PcdMaxVolatileVariableSize) :
                                                    mVariableModuleGlobal->MaxVariableSize
                                                    );
  //
  // Allocate memory for volatile variable store, note that there is a scratch space to store scratch data.
  //
  ScratchSize                              = GetMaxVariableSize ();
  mVariableModuleGlobal->ScratchBufferSize = ScratchSize;
  VolatileVariableStore                    = AllocateRuntimePool (PcdGet32 (PcdVariableStoreSize) + ScratchSize);
  if (VolatileVariableStore == NULL) {
    if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
      FreePool ((VOID *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase);
    }

    if (mNvFvHeaderCache != NULL) {
      FreePool (mNvFvHeaderCache);
    }

    FreePool (mVariableModuleGlobal);
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VolatileVariableStore, PcdGet32 (PcdVariableStoreSize) + ScratchSize, 0xff);

  //
  // Initialize Variable Specific Data.
  //
  mVariableModuleGlobal->VariableGlobal.VolatileVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VolatileVariableStore;
  mVariableModuleGlobal->VolatileLastVariableOffset          = (UINTN)GetStartPointer (VolatileVariableStore) - (UINTN)VolatileVariableStore;

  CopyGuid (&VolatileVariableStore->Signature, VariableGuid);
  VolatileVariableStore->Size      = PcdGet32 (PcdVariableStoreSize);
  VolatileVariableStore->Format    = VARIABLE_STORE_FORMATTED;
  VolatileVariableStore->State     = VARIABLE_STORE_HEALTHY;
  VolatileVariableStore->Reserved  = 0;
  VolatileVariableStore->Reserved1 = 0;

  return EFI_SUCCESS;
}

/**
  Get the proper fvb handle and/or fvb protocol by the given Flash address.

  @param[in]  Address       The Flash address.
  @param[out] FvbHandle     In output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   In output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                Attributes;
  UINTN                               BlockSize;
  UINTN                               NumberOfBlocks;

  HandleBuffer = NULL;
  //
  // Get all FVB handles.
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the FVB to access variable store.
  //
  Fvb = NULL;
  for (Index = 0; Index < HandleCount; Index += 1, Status = EFI_NOT_FOUND, Fvb = NULL) {
    Status = GetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    // Ensure this FVB protocol supported Write operation.
    //
    Status = Fvb->GetAttributes (Fvb, &Attributes);
    if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
      continue;
    }

    //
    // Compare the address and select the right one.
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Assume one FVB has one type of BlockSize.
    //
    Status = Fvb->GetBlockSize (Fvb, 0, &BlockSize, &NumberOfBlocks);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((Address >= FvbBaseAddress) && (Address < (FvbBaseAddress + BlockSize * NumberOfBlocks))) {
      if (FvbHandle != NULL) {
        *FvbHandle = HandleBuffer[Index];
      }

      if (FvbProtocol != NULL) {
        *FvbProtocol = Fvb;
      }

      Status = EFI_SUCCESS;
      break;
    }
  }

  FreePool (HandleBuffer);

  if (Fvb == NULL) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}
