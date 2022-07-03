/** @file
  Implement all four UEFI Runtime Variable services for the nonvolatile
  and volatile storage space and install variable architecture protocol.

  Modified to work with UEFI1.1 for Apple firmware, and for immediate
  install.

Copyright (C) 2013, Red Hat, Inc.
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) Microsoft Corporation.<BR>
Additional modifications Copyright (c) 2022 Mike Beaton
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyLib.h>

EFI_STATUS
EFIAPI
ProtocolIsVariablePolicyEnabled (
  OUT BOOLEAN  *State
  );

EFI_HANDLE                      mHandle                      = NULL;
EFI_EVENT                       mVirtualAddressChangeEvent   = NULL;
VOID                            *mFtwRegistration            = NULL;
VOID                            ***mVarCheckAddressPointer   = NULL;
UINTN                           mVarCheckAddressPointerCount = 0;
EDKII_VARIABLE_LOCK_PROTOCOL    mVariableLock                = { VariableLockRequestToLock };
EDKII_VARIABLE_POLICY_PROTOCOL  mVariablePolicyProtocol      = {
  EDKII_VARIABLE_POLICY_PROTOCOL_REVISION,
  DisableVariablePolicy,
  ProtocolIsVariablePolicyEnabled,
  RegisterVariablePolicy,
  DumpVariablePolicy,
  LockVariablePolicy
};
EDKII_VAR_CHECK_PROTOCOL        mVarCheck = {
  VarCheckRegisterSetVariableCheckHandler,
  VarCheckVariablePropertySet,
  VarCheckVariablePropertyGet
};

/**
  Some Secure Boot Policy Variable may update following other variable changes(SecureBoot follows PK change, etc).
  Record their initial State when variable write service is ready.

**/
VOID
EFIAPI
RecordSecureBootPolicyVarData (
  VOID
  );

/**
  Return TRUE if ExitBootServices () has been called.

  @retval TRUE If ExitBootServices () has been called.
**/
BOOLEAN
AtRuntime (
  VOID
  )
{
  return EfiAtRuntime ();
}

/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state
  and returns the lock.  Each lock provides mutual exclusion access at its task
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK  *Lock,
  IN     EFI_TPL   Priority
  )
{
  return EfiInitializeLock (Lock, Priority);
}

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!AtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!AtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Retrieve the Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of Ftw protocol

  @retval EFI_SUCCESS           The FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         The FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID  **FtwProtocol
  )
{
  EFI_STATUS  Status;

  //
  // Locate Fault Tolerent Write protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiFaultTolerantWriteProtocolGuid,
                  NULL,
                  FtwProtocol
                  );
  return Status;
}

/**
  Retrieve the FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the FVB protocol interface on the handle
  //
  return gBS->HandleProtocol (
                FvBlockHandle,
                &gEfiFirmwareVolumeBlockProtocolGuid,
                (VOID **)FvBlock
                );
}

/**
  Function returns an array of handles that support the FVB protocol
  in a buffer allocated from pool.

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  )
{
  EFI_STATUS  Status;

  //
  // Locate all handles of Fvb protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  NumberHandles,
                  Buffer
                  );
  return Status;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  if (mVariableModuleGlobal->FvbInstance != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->GetBlockSize);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->GetPhysicalAddress);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->GetAttributes);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->SetAttributes);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->Read);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->Write);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance->EraseBlocks);
    EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->FvbInstance);
  }

  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->PlatformLangCodes);
  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->LangCodes);
  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->PlatformLang);
  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal->VariableGlobal.HobVariableBase);
  EfiConvertPointer (0x0, (VOID **)&mVariableModuleGlobal);
  EfiConvertPointer (0x0, (VOID **)&mNvVariableCache);
  EfiConvertPointer (0x0, (VOID **)&mNvFvHeaderCache);

  if (mAuthContextOut.AddressPointer != NULL) {
    for (Index = 0; Index < mAuthContextOut.AddressPointerCount; Index++) {
      EfiConvertPointer (0x0, (VOID **)mAuthContextOut.AddressPointer[Index]);
    }
  }

  if (mVarCheckAddressPointer != NULL) {
    for (Index = 0; Index < mVarCheckAddressPointerCount; Index++) {
      EfiConvertPointer (0x0, (VOID **)mVarCheckAddressPointer[Index]);
    }
  }
}

/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OnReadyToBoot (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (!mEndOfDxe) {
    MorLockInitAtEndOfDxe ();

    Status = LockVariablePolicy ();
    ASSERT_EFI_ERROR (Status);
    //
    // Set the End Of DXE bit in case the EFI_END_OF_DXE_EVENT_GROUP_GUID event is not signaled.
    //
    mEndOfDxe               = TRUE;
    mVarCheckAddressPointer = VarCheckLibInitializeAtEndOfDxe (&mVarCheckAddressPointerCount);
    //
    // The initialization for variable quota.
    //
    InitializeVariableQuota ();
  }

  ReclaimForOS ();
  if (FeaturePcdGet (PcdVariableCollectStatistics)) {
    if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
      gBS->InstallConfigurationTable (&gEfiAuthenticatedVariableGuid, gVariableInfo);
    } else {
      gBS->InstallConfigurationTable (&gEfiVariableGuid, gVariableInfo);
    }
  }

  gBS->CloseEvent (Event);
}

/**
  Notification function of EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  This is a notification function registered on EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OnEndOfDxe (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "[Variable]END_OF_DXE is signaled\n"));
  MorLockInitAtEndOfDxe ();
  Status = LockVariablePolicy ();
  ASSERT_EFI_ERROR (Status);
  mEndOfDxe               = TRUE;
  mVarCheckAddressPointer = VarCheckLibInitializeAtEndOfDxe (&mVarCheckAddressPointerCount);
  //
  // The initialization for variable quota.
  //
  InitializeVariableQuota ();
  if (PcdGetBool (PcdReclaimVariableSpaceAtEndOfDxe)) {
    ReclaimForOS ();
  }

  gBS->CloseEvent (Event);
}

/**
  Initializes variable write service for DXE.

**/
VOID
VariableWriteServiceInitializeDxe (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = VariableWriteServiceInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Variable write service initialization failed. Status = %r\n", Status));
  }

  //
  // Some Secure Boot Policy Var (SecureBoot, etc) updates following other
  // Secure Boot Policy Variable change. Record their initial value.
  //
  RecordSecureBootPolicyVarData ();

  //
  // Install the Variable Write Architectural protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableWriteArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Fault Tolerant Write protocol notification event handler.

  Non-Volatile variable write may needs FTW protocol to reclaim when
  writting variable.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
FtwNotificationEvent (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvbProtocol;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL   *FtwProtocol;
  EFI_PHYSICAL_ADDRESS                NvStorageVariableBase;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     GcdDescriptor;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  UINT64                              Length;
  EFI_PHYSICAL_ADDRESS                VariableStoreBase;
  UINT64                              VariableStoreLength;
  UINTN                               FtwMaxBlockSize;

  //
  // Ensure FTW protocol is installed.
  //
  Status = GetFtwProtocol ((VOID **)&FtwProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = FtwProtocol->GetMaxBlockSize (FtwProtocol, &FtwMaxBlockSize);
  if (!EFI_ERROR (Status)) {
    ASSERT (PcdGet32 (PcdFlashNvStorageVariableSize) <= FtwMaxBlockSize);
  }

  NvStorageVariableBase = NV_STORAGE_VARIABLE_BASE;
  VariableStoreBase     = NvStorageVariableBase + mNvFvHeaderCache->HeaderLength;

  //
  // Let NonVolatileVariableBase point to flash variable store base directly after FTW ready.
  //
  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = VariableStoreBase;

  //
  // Find the proper FVB protocol for variable.
  //
  Status = GetFvbInfoByAddress (NvStorageVariableBase, NULL, &FvbProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  mVariableModuleGlobal->FvbInstance = FvbProtocol;

  //
  // Mark the variable storage region of the FLASH as RUNTIME.
  //
  VariableStoreLength = mNvVariableCache->Size;
  BaseAddress         = VariableStoreBase & (~EFI_PAGE_MASK);
  Length              = VariableStoreLength + (VariableStoreBase - BaseAddress);
  Length              = (Length + EFI_PAGE_SIZE - 1) & (~EFI_PAGE_MASK);

  Status = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Variable driver failed to get flash memory attribute.\n"));
  } else {
    if ((GcdDescriptor.Attributes & EFI_MEMORY_RUNTIME) == 0) {
      Status = gDS->SetMemorySpaceAttributes (
                      BaseAddress,
                      Length,
                      GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "Variable driver failed to add EFI_MEMORY_RUNTIME attribute to Flash.\n"));
      }
    }
  }

  //
  // Initializes variable write service after FTW was ready.
  //
  VariableWriteServiceInitializeDxe ();

  //
  // Close the notify event to avoid install gEfiVariableWriteArchProtocolGuid again.
  //
  gBS->CloseEvent (Event);
}

/**
  This API function returns whether or not the policy engine is
  currently being enforced.

  @param[out]   State       Pointer to a return value for whether the policy enforcement
                            is currently enabled.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
EFI_STATUS
EFIAPI
ProtocolIsVariablePolicyEnabled (
  OUT BOOLEAN  *State
  )
{
  *State = IsVariablePolicyEnabled ();
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MapCreateEventEx (
  IN       UINT32            Type,
  IN       EFI_TPL           NotifyTpl,
  IN       EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
  IN CONST VOID              *NotifyContext OPTIONAL,
  IN CONST EFI_GUID          *EventGroup    OPTIONAL,
  OUT      EFI_EVENT         *Event
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;

  if (Type == EVT_NOTIFY_SIGNAL) {
    if (CompareGuid (EventGroup, &gEfiEventVirtualAddressChangeGuid)) {
      Type   = EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE;
      Status = EFI_SUCCESS;
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return gBS->CreateEvent (
                Type,
                NotifyTpl,
                NotifyFunction,
                (VOID *)NotifyContext,
                Event
                );
}

/**
  Variable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols
  for variable read and write services being available. It also registers
  a notification function for an EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  EFI_EVENT            ReadyToBootEvent;
  EFI_EVENT            EndOfDxeEvent;
  EFI_CREATE_EVENT_EX  OriginalCreateEventEx;

  //
  // Probably worth noting that attempting to remove any pre-existing protocols here
  // before installing them below seems to cause problems rather than solving them.
  //

  Status = VariableCommonInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEdkiiVariableLockProtocolGuid,
                  &mVariableLock,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEdkiiVarCheckProtocolGuid,
                  &mVarCheck,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  SystemTable->RuntimeServices->GetVariable         = VariableServiceGetVariable;
  SystemTable->RuntimeServices->GetNextVariableName = VariableServiceGetNextVariableName;
  SystemTable->RuntimeServices->SetVariable         = VariableServiceSetVariable;
  SystemTable->RuntimeServices->QueryVariableInfo   = VariableServiceQueryVariableInfo;

  //
  // Now install the Variable Runtime Architectural protocol on a new handle.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  if (!PcdGetBool (PcdEmuVariableNvModeEnable)) {
    //
    // Register FtwNotificationEvent () notify function.
    //
    EfiCreateProtocolNotifyEvent (
      &gEfiFaultTolerantWriteProtocolGuid,
      TPL_CALLBACK,
      FtwNotificationEvent,
      (VOID *)SystemTable,
      &mFtwRegistration
      );
  } else {
    //
    // Emulated non-volatile variable mode does not depend on FVB and FTW.
    //
    VariableWriteServiceInitializeDxe ();
  }

  OriginalCreateEventEx = gBS->CreateEventEx;
  gBS->CreateEventEx    = MapCreateEventEx;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VariableClassAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register the event handling function to reclaim variable for OS usage.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnReadyToBoot,
                  NULL,
                  &ReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register the event handling function to set the End Of DXE flag.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnEndOfDxe,
                  NULL,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  // Register and initialize the VariablePolicy engine.
  Status = InitVariablePolicyLib (VariableServiceGetVariable);
  ASSERT_EFI_ERROR (Status);
  Status = VarCheckRegisterSetVariableCheckHandler (ValidateSetVariable);
  ASSERT_EFI_ERROR (Status);
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEdkiiVariablePolicyProtocolGuid,
                  &mVariablePolicyProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  gBS->CreateEventEx = OriginalCreateEventEx;

  // Signal events immediately. These events occur in this order and before
  // OpenCore is loaded when the equivalent driver is part of OpenDuet.
  gBS->SignalEvent (EndOfDxeEvent);
  gBS->SignalEvent (ReadyToBootEvent);

  return EFI_SUCCESS;
}
