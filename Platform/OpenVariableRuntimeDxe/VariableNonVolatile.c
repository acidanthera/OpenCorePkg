/** @file
  Common variable non-volatile store routines.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableNonVolatile.h"
#include "VariableParsing.h"

extern VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;

/**
  Get non-volatile maximum variable size.

  @return Non-volatile maximum variable size.

**/
UINTN
GetNonVolatileMaxVariableSize (
  VOID
  )
{
  if (PcdGet32 (PcdHwErrStorageSize) != 0) {
    return MAX (
             MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxAuthVariableSize)),
             PcdGet32 (PcdMaxHardwareErrorVariableSize)
             );
  } else {
    return MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxAuthVariableSize));
  }
}

/**
  Init emulated non-volatile variable store.

  @param[out] VariableStoreBase Output pointer to emulated non-volatile variable store base.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
InitEmuNonVolatileVariableStore (
  OUT EFI_PHYSICAL_ADDRESS  *VariableStoreBase
  )
{
  VARIABLE_STORE_HEADER  *VariableStore;
  UINT32                 VariableStoreLength;
  BOOLEAN                FullyInitializeStore;
  UINT32                 HwErrStorageSize;

  FullyInitializeStore = TRUE;

  VariableStoreLength = PcdGet32 (PcdVariableStoreSize);
  ASSERT (sizeof (VARIABLE_STORE_HEADER) <= VariableStoreLength);

  //
  // Allocate memory for variable store.
  //
  if (PcdGet64 (PcdEmuVariableNvStoreReserved) == 0) {
    VariableStore = (VARIABLE_STORE_HEADER *)AllocateRuntimePool (VariableStoreLength);
    if (VariableStore == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // A memory location has been reserved for the NV variable store.  Certain
    // platforms may be able to preserve a memory range across system resets,
    // thereby providing better NV variable emulation.
    //
    VariableStore =
      (VARIABLE_STORE_HEADER *)(VOID *)(UINTN)
      PcdGet64 (PcdEmuVariableNvStoreReserved);
    if ((VariableStore->Size == VariableStoreLength) &&
        (CompareGuid (&VariableStore->Signature, &gEfiAuthenticatedVariableGuid) ||
         CompareGuid (&VariableStore->Signature, &gEfiVariableGuid)) &&
        (VariableStore->Format == VARIABLE_STORE_FORMATTED) &&
        (VariableStore->State == VARIABLE_STORE_HEALTHY))
    {
      DEBUG ((
        DEBUG_INFO,
        "Variable Store reserved at %p appears to be valid\n",
        VariableStore
        ));
      FullyInitializeStore = FALSE;
    }
  }

  if (FullyInitializeStore) {
    SetMem (VariableStore, VariableStoreLength, 0xff);
    //
    // Use gEfiAuthenticatedVariableGuid for potential auth variable support.
    //
    CopyGuid (&VariableStore->Signature, &gEfiAuthenticatedVariableGuid);
    VariableStore->Size      = VariableStoreLength;
    VariableStore->Format    = VARIABLE_STORE_FORMATTED;
    VariableStore->State     = VARIABLE_STORE_HEALTHY;
    VariableStore->Reserved  = 0;
    VariableStore->Reserved1 = 0;
  }

  *VariableStoreBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VariableStore;

  HwErrStorageSize = PcdGet32 (PcdHwErrStorageSize);

  //
  // Note that in EdkII variable driver implementation, Hardware Error Record type variable
  // is stored with common variable in the same NV region. So the platform integrator should
  // ensure that the value of PcdHwErrStorageSize is less than the value of
  // (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)).
  //
  ASSERT (HwErrStorageSize < (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)));

  mVariableModuleGlobal->CommonVariableSpace        = ((UINTN)VariableStoreLength - sizeof (VARIABLE_STORE_HEADER) - HwErrStorageSize);
  mVariableModuleGlobal->CommonMaxUserVariableSpace = mVariableModuleGlobal->CommonVariableSpace;
  mVariableModuleGlobal->CommonRuntimeVariableSpace = mVariableModuleGlobal->CommonVariableSpace;

  return EFI_SUCCESS;
}

/**
  Init real non-volatile variable store.

  @param[out] VariableStoreBase Output pointer to real non-volatile variable store base.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.
  @retval EFI_VOLUME_CORRUPTED  Variable Store or Firmware Volume for Variable Store is corrupted.

**/
EFI_STATUS
InitRealNonVolatileVariableStore (
  OUT EFI_PHYSICAL_ADDRESS  *VariableStoreBase
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  VARIABLE_STORE_HEADER                 *VariableStore;
  UINT32                                VariableStoreLength;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_PHYSICAL_ADDRESS                  NvStorageBase;
  UINT8                                 *NvStorageData;
  UINT32                                NvStorageSize;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *FtwLastWriteData;
  UINT32                                BackUpOffset;
  UINT32                                BackUpSize;
  UINT32                                HwErrStorageSize;
  UINT32                                MaxUserNvVariableSpaceSize;
  UINT32                                BoottimeReservedNvVariableSpaceSize;
  EFI_STATUS                            Status;
  VOID                                  *FtwProtocol;

  mVariableModuleGlobal->FvbInstance = NULL;

  //
  // Allocate runtime memory used for a memory copy of the FLASH region.
  // Keep the memory and the FLASH in sync as updates occur.
  //
  NvStorageSize = PcdGet32 (PcdFlashNvStorageVariableSize);
  NvStorageData = AllocateRuntimeZeroPool (NvStorageSize);
  if (NvStorageData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NvStorageBase = NV_STORAGE_VARIABLE_BASE;
  ASSERT (NvStorageBase != 0);

  //
  // Copy NV storage data to the memory buffer.
  //
  CopyMem (NvStorageData, (UINT8 *)(UINTN)NvStorageBase, NvStorageSize);

  Status = GetFtwProtocol ((VOID **)&FtwProtocol);
  //
  // If FTW protocol has been installed, no need to check FTW last write data hob.
  //
  if (EFI_ERROR (Status)) {
    //
    // Check the FTW last write data hob.
    //
    GuidHob = GetFirstGuidHob (&gEdkiiFaultTolerantWriteGuid);
    if (GuidHob != NULL) {
      FtwLastWriteData = (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA *)GET_GUID_HOB_DATA (GuidHob);
      if (FtwLastWriteData->TargetAddress == NvStorageBase) {
        DEBUG ((DEBUG_INFO, "Variable: NV storage is backed up in spare block: 0x%x\n", (UINTN)FtwLastWriteData->SpareAddress));
        //
        // Copy the backed up NV storage data to the memory buffer from spare block.
        //
        CopyMem (NvStorageData, (UINT8 *)(UINTN)(FtwLastWriteData->SpareAddress), NvStorageSize);
      } else if ((FtwLastWriteData->TargetAddress > NvStorageBase) &&
                 (FtwLastWriteData->TargetAddress < (NvStorageBase + NvStorageSize)))
      {
        //
        // Flash NV storage from the Offset is backed up in spare block.
        //
        BackUpOffset = (UINT32)(FtwLastWriteData->TargetAddress - NvStorageBase);
        BackUpSize   = NvStorageSize - BackUpOffset;
        DEBUG ((DEBUG_INFO, "Variable: High partial NV storage from offset: %x is backed up in spare block: 0x%x\n", BackUpOffset, (UINTN)FtwLastWriteData->SpareAddress));
        //
        // Copy the partial backed up NV storage data to the memory buffer from spare block.
        //
        CopyMem (NvStorageData + BackUpOffset, (UINT8 *)(UINTN)FtwLastWriteData->SpareAddress, BackUpSize);
      }
    }
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)NvStorageData;

  //
  // Check if the Firmware Volume is not corrupted
  //
  if ((FvHeader->Signature != EFI_FVH_SIGNATURE) || (!CompareGuid (&gEfiSystemNvDataFvGuid, &FvHeader->FileSystemGuid))) {
    FreePool (NvStorageData);
    DEBUG ((DEBUG_ERROR, "Firmware Volume for Variable Store is corrupted\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  VariableStore       = (VARIABLE_STORE_HEADER *)((UINTN)FvHeader + FvHeader->HeaderLength);
  VariableStoreLength = NvStorageSize - FvHeader->HeaderLength;
  ASSERT (sizeof (VARIABLE_STORE_HEADER) <= VariableStoreLength);
  ASSERT (VariableStore->Size == VariableStoreLength);

  //
  // Check if the Variable Store header is not corrupted
  //
  if (GetVariableStoreStatus (VariableStore) != EfiValid) {
    FreePool (NvStorageData);
    DEBUG ((DEBUG_ERROR, "Variable Store header is corrupted\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  mNvFvHeaderCache = FvHeader;

  *VariableStoreBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VariableStore;

  HwErrStorageSize                    = PcdGet32 (PcdHwErrStorageSize);
  MaxUserNvVariableSpaceSize          = PcdGet32 (PcdMaxUserNvVariableSpaceSize);
  BoottimeReservedNvVariableSpaceSize = PcdGet32 (PcdBoottimeReservedNvVariableSpaceSize);

  //
  // Note that in EdkII variable driver implementation, Hardware Error Record type variable
  // is stored with common variable in the same NV region. So the platform integrator should
  // ensure that the value of PcdHwErrStorageSize is less than the value of
  // (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)).
  //
  ASSERT (HwErrStorageSize < (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)));
  //
  // Ensure that the value of PcdMaxUserNvVariableSpaceSize is less than the value of
  // (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)) - PcdGet32 (PcdHwErrStorageSize).
  //
  ASSERT (MaxUserNvVariableSpaceSize < (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER) - HwErrStorageSize));
  //
  // Ensure that the value of PcdBoottimeReservedNvVariableSpaceSize is less than the value of
  // (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)) - PcdGet32 (PcdHwErrStorageSize).
  //
  ASSERT (BoottimeReservedNvVariableSpaceSize < (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER) - HwErrStorageSize));

  mVariableModuleGlobal->CommonVariableSpace        = ((UINTN)VariableStoreLength - sizeof (VARIABLE_STORE_HEADER) - HwErrStorageSize);
  mVariableModuleGlobal->CommonMaxUserVariableSpace = ((MaxUserNvVariableSpaceSize != 0) ? MaxUserNvVariableSpaceSize : mVariableModuleGlobal->CommonVariableSpace);
  mVariableModuleGlobal->CommonRuntimeVariableSpace = mVariableModuleGlobal->CommonVariableSpace - BoottimeReservedNvVariableSpaceSize;

  DEBUG ((
    DEBUG_INFO,
    "Variable driver common space: 0x%x 0x%x 0x%x\n",
    mVariableModuleGlobal->CommonVariableSpace,
    mVariableModuleGlobal->CommonMaxUserVariableSpace,
    mVariableModuleGlobal->CommonRuntimeVariableSpace
    ));

  //
  // The max NV variable size should be < (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)).
  //
  ASSERT (GetNonVolatileMaxVariableSize () < (VariableStoreLength - sizeof (VARIABLE_STORE_HEADER)));

  return EFI_SUCCESS;
}

/**
  Init non-volatile variable store.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.
  @retval EFI_VOLUME_CORRUPTED  Variable Store or Firmware Volume for Variable Store is corrupted.

**/
EFI_STATUS
InitNonVolatileVariableStore (
  VOID
  )
{
  VARIABLE_HEADER       *Variable;
  VARIABLE_HEADER       *NextVariable;
  EFI_PHYSICAL_ADDRESS  VariableStoreBase;
  UINTN                 VariableSize;
  EFI_STATUS            Status;

  if (PcdGetBool (PcdEmuVariableNvModeEnable)) {
    Status = InitEmuNonVolatileVariableStore (&VariableStoreBase);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    mVariableModuleGlobal->VariableGlobal.EmuNvMode = TRUE;
    DEBUG ((DEBUG_INFO, "Variable driver will work at emulated non-volatile variable mode!\n"));
  } else {
    Status = InitRealNonVolatileVariableStore (&VariableStoreBase);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    mVariableModuleGlobal->VariableGlobal.EmuNvMode = FALSE;
  }

  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = VariableStoreBase;
  mNvVariableCache                                              = (VARIABLE_STORE_HEADER *)(UINTN)VariableStoreBase;
  mVariableModuleGlobal->VariableGlobal.AuthFormat              = (BOOLEAN)(CompareGuid (&mNvVariableCache->Signature, &gEfiAuthenticatedVariableGuid));

  mVariableModuleGlobal->MaxVariableSize     = PcdGet32 (PcdMaxVariableSize);
  mVariableModuleGlobal->MaxAuthVariableSize = ((PcdGet32 (PcdMaxAuthVariableSize) != 0) ? PcdGet32 (PcdMaxAuthVariableSize) : mVariableModuleGlobal->MaxVariableSize);

  //
  // Parse non-volatile variable data and get last variable offset.
  //
  Variable = GetStartPointer (mNvVariableCache);
  while (IsValidVariableHeader (Variable, GetEndPointer (mNvVariableCache))) {
    NextVariable = GetNextVariablePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    VariableSize = (UINTN)NextVariable - (UINTN)Variable;
    if ((Variable->Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
      mVariableModuleGlobal->HwErrVariableTotalSize += VariableSize;
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += VariableSize;
    }

    Variable = NextVariable;
  }

  mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN)Variable - (UINTN)mNvVariableCache;

  return EFI_SUCCESS;
}
