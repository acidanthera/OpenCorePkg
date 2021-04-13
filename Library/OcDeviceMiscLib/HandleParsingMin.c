/** @file
  Provides interface to shell functionality for shell commands and applications.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright 2016-2018 Dell Technologies.<BR>
  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>

#include "HandleParsingMin.h"

STATIC HANDLE_INDEX_LIST mHandleList = {{{NULL,NULL},0,0},0};

/**
  Function to initialize the file global mHandleList object for use in
  vonverting handles to index and index to handle.

  @retval EFI_SUCCESS     The operation was successful.
**/
STATIC
EFI_STATUS
InternalShellInitHandleList(
  VOID
  )
{
  EFI_STATUS   Status;
  EFI_HANDLE   *HandleBuffer;
  UINTN        HandleCount;
  HANDLE_LIST  *ListWalker;

  if (mHandleList.NextIndex != 0) {
    return EFI_SUCCESS;
  }
  InitializeListHead(&mHandleList.List.Link);
  mHandleList.NextIndex = 1;
  Status = gBS->LocateHandleBuffer (
                AllHandles,
                NULL,
                NULL,
                &HandleCount,
                &HandleBuffer
               );
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    return (Status);
  }
  for (mHandleList.NextIndex = 1 ; mHandleList.NextIndex <= HandleCount ; mHandleList.NextIndex++){
    ListWalker = AllocateZeroPool(sizeof(HANDLE_LIST));
    if (ListWalker != NULL) {
      ListWalker->TheHandle = HandleBuffer[mHandleList.NextIndex - 1];
      ListWalker->TheIndex = mHandleList.NextIndex;
      InsertTailList (&mHandleList.List.Link, &ListWalker->Link);
    }
  }
  FreePool(HandleBuffer);
  return (EFI_SUCCESS);
}

/**
  Function to retrieve the human-friendly index of a given handle.  If the handle
  does not have a index one will be automatically assigned.  The index value is valid
  until the termination of the shell application.

  @param[in] TheHandle    The handle to retrieve an index for.

  @retval 0               A memory allocation failed.
  @return                 The index of the handle.

**/
UINTN
EFIAPI
InternalConvertHandleToHandleIndex(
  IN CONST EFI_HANDLE TheHandle
  )
{
  EFI_STATUS   Status;
  EFI_GUID     **ProtocolBuffer;
  UINTN        ProtocolCount;
  HANDLE_LIST  *ListWalker;

  if (TheHandle == NULL) {
    return 0;
  }

  InternalShellInitHandleList();

  for (ListWalker = (HANDLE_LIST*)GetFirstNode(&mHandleList.List.Link)
    ;  !IsNull(&mHandleList.List.Link,&ListWalker->Link)
    ;  ListWalker = (HANDLE_LIST*)GetNextNode(&mHandleList.List.Link,&ListWalker->Link)
   ){
    if (ListWalker->TheHandle == TheHandle) {
      //
      // Verify that TheHandle is still present in the Handle Database
      //
      Status = gBS->ProtocolsPerHandle(TheHandle, &ProtocolBuffer, &ProtocolCount);
      if (EFI_ERROR (Status)) {
        //
        // TheHandle is not present in the Handle Database, so delete from the handle list
        //
        RemoveEntryList (&ListWalker->Link);
        return 0;
      }
      FreePool (ProtocolBuffer);
      return (ListWalker->TheIndex);
    }
  }

  //
  // Verify that TheHandle is valid handle
  //
  Status = gBS->ProtocolsPerHandle(TheHandle, &ProtocolBuffer, &ProtocolCount);
  if (EFI_ERROR (Status)) {
    //
    // TheHandle is not valid, so do not add to handle list
    //
    return 0;
  }
  FreePool (ProtocolBuffer);

  ListWalker = AllocateZeroPool(sizeof(HANDLE_LIST));
  if (ListWalker == NULL) {
    return 0;
  }
  ListWalker->TheHandle = TheHandle;
  ListWalker->TheIndex  = mHandleList.NextIndex++;
  InsertTailList(&mHandleList.List.Link,&ListWalker->Link);
  return (ListWalker->TheIndex);
}

/**
  Gets all the related EFI_HANDLEs based on the mask supplied.

  This function scans all EFI_HANDLES in the UEFI environment's handle database
  and returns the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a successful return the memory must be
  caller freed.

  @param[in] DriverBindingHandle    The handle with Driver Binding protocol on it.
  @param[in] ControllerHandle       The handle with Device Path protocol on it.
  @param[in] MatchingHandleCount    The pointer to UINTN that specifies the number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a successful return, a buffer of MatchingHandleCount
                                    EFI_HANDLEs with a terminating NULL EFI_HANDLE.
  @param[out] HandleType            An array of type information.

  @retval EFI_SUCCESS               The operation was successful, and any related handles
                                    are in MatchingHandleBuffer.
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
**/
STATIC
EFI_STATUS
EFIAPI
ParseHandleDatabaseByRelationshipWithType (
  IN CONST EFI_HANDLE DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE ControllerHandle OPTIONAL,
  IN UINTN            *HandleCount,
  OUT EFI_HANDLE      **HandleBuffer,
  OUT UINTN           **HandleType
  )
{
  EFI_STATUS                          Status;
  UINTN                               HandleIndex;
  EFI_GUID                            **ProtocolGuidArray;
  UINTN                               ArrayCount;
  UINTN                               ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfo;
  UINTN                               OpenInfoCount;
  UINTN                               OpenInfoIndex;
  UINTN                               ChildIndex;
  INTN                                DriverBindingHandleIndex;

  ASSERT(HandleCount  != NULL);
  ASSERT(HandleBuffer != NULL);
  ASSERT(HandleType   != NULL);
  ASSERT(DriverBindingHandle != NULL || ControllerHandle != NULL);

  *HandleCount                  = 0;
  *HandleBuffer                 = NULL;
  *HandleType                   = NULL;

  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
                AllHandles,
                NULL,
                NULL,
                HandleCount,
                HandleBuffer
               );
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  *HandleType = AllocateZeroPool (*HandleCount * sizeof (UINTN));
  if (*HandleType == NULL) {
    if (*HandleBuffer != NULL) {
      FreePool (*HandleBuffer);
      *HandleBuffer = NULL;
    }
    *HandleCount = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  DriverBindingHandleIndex = -1;
  for (HandleIndex = 0; HandleIndex < *HandleCount; HandleIndex++) {
    if (DriverBindingHandle != NULL && (*HandleBuffer)[HandleIndex] == DriverBindingHandle) {
      DriverBindingHandleIndex = (INTN)HandleIndex;
    }
  }

  for (HandleIndex = 0; HandleIndex < *HandleCount; HandleIndex++) {
    //
    // Retrieve the list of all the protocols on each handle
    //
    Status = gBS->ProtocolsPerHandle (
                  (*HandleBuffer)[HandleIndex],
                  &ProtocolGuidArray,
                  &ArrayCount
                 );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {

      //
      // Set the bit describing what this handle has
      //
      if        (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiLoadedImageProtocolGuid)         ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_IMAGE_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverBindingProtocolGuid)       ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_BINDING_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfiguration2ProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_CONFIGURATION_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfigurationProtocolGuid) ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_CONFIGURATION_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnostics2ProtocolGuid)  ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_DIAGNOSTICS_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnosticsProtocolGuid)   ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_DIAGNOSTICS_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentName2ProtocolGuid)      ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_COMPONENT_NAME_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentNameProtocolGuid)       ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_COMPONENT_NAME_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDevicePathProtocolGuid)          ) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DEVICE_HANDLE;
      }
      //
      // Retrieve the list of agents that have opened each protocol
      //
      Status = gBS->OpenProtocolInformation (
                      (*HandleBuffer)[HandleIndex],
                      ProtocolGuidArray[ProtocolIndex],
                      &OpenInfo,
                      &OpenInfoCount
                     );
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (ControllerHandle == NULL) {
        //
        // ControllerHandle == NULL and DriverBindingHandle != NULL.
        // Return information on all the controller handles that the driver specified by DriverBindingHandle is managing
        //
        for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
          if (OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle && (OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
            (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
            if (DriverBindingHandleIndex != -1) {
              (*HandleType)[DriverBindingHandleIndex] |= (UINTN)HR_DEVICE_DRIVER;
            }
          }
          if (OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle && (OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
            (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
            if (DriverBindingHandleIndex != -1) {
              (*HandleType)[DriverBindingHandleIndex] |= (UINTN)(HR_BUS_DRIVER | HR_DEVICE_DRIVER);
            }
            for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
              if (OpenInfo[OpenInfoIndex].ControllerHandle == (*HandleBuffer)[ChildIndex]) {
                (*HandleType)[ChildIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
              }
            }
          }
        }
      }
      if (DriverBindingHandle == NULL && ControllerHandle != NULL) {
        if (ControllerHandle == (*HandleBuffer)[HandleIndex]) {
          (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
              for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)HR_DEVICE_DRIVER;
                }
              }
            }
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)(HR_BUS_DRIVER | HR_DEVICE_DRIVER);
                }
                if (OpenInfo[OpenInfoIndex].ControllerHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
                }
              }
            }
          }
        } else {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              if (OpenInfo[OpenInfoIndex].ControllerHandle == ControllerHandle) {
                (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_PARENT_HANDLE);
              }
            }
          }
        }
      }
      if (DriverBindingHandle != NULL && ControllerHandle != NULL) {
        if (ControllerHandle == (*HandleBuffer)[HandleIndex]) {
          (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
              if (OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) {
                if (DriverBindingHandleIndex != -1) {
                  (*HandleType)[DriverBindingHandleIndex] |= (UINTN)HR_DEVICE_DRIVER;
                }
              }
            }
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              if (OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) {
                for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                  if (OpenInfo[OpenInfoIndex].ControllerHandle == (*HandleBuffer)[ChildIndex]) {
                    (*HandleType)[ChildIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
                  }
                }
              }

              for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)(HR_BUS_DRIVER | HR_DEVICE_DRIVER);
                }
              }
            }
          }
        } else {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              if (OpenInfo[OpenInfoIndex].ControllerHandle == ControllerHandle) {
                (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_PARENT_HANDLE);
              }
            }
          }
        }
      }
      FreePool (OpenInfo);
    }
    FreePool (ProtocolGuidArray);
  }
  return EFI_SUCCESS;
}

/**
  Gets all the related EFI_HANDLEs based on the single EFI_HANDLE and the mask
  supplied.

  This function will scan all EFI_HANDLES in the UEFI environment's handle database
  and return all the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a sucessful return the memory must be
  caller freed.

  @param[in] DriverBindingHandle    Handle to a object with Driver Binding protocol
                                    on it.
  @param[in] ControllerHandle       Handle to a device with Device Path protocol on it.
  @param[in] Mask                   Mask of what relationship(s) is desired.
  @param[in] MatchingHandleCount    Poitner to UINTN specifying number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a sucessful return a buffer of MatchingHandleCount
                                    EFI_HANDLEs and a terminating NULL EFI_HANDLE.

  @retval EFI_SUCCESS               The operation was sucessful and any related handles
                                    are in MatchingHandleBuffer;
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
**/
EFI_STATUS
EFIAPI
InternalParseHandleDatabaseByRelationship (
  IN CONST EFI_HANDLE       DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE       ControllerHandle OPTIONAL,
  IN CONST UINTN            Mask,
  IN UINTN                  *MatchingHandleCount,
  OUT EFI_HANDLE            **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_STATUS            Status;
  UINTN                 HandleCount;
  EFI_HANDLE            *HandleBuffer;
  UINTN                 *HandleType;
  UINTN                 HandleIndex;

  ASSERT(MatchingHandleCount != NULL);
  ASSERT(DriverBindingHandle != NULL || ControllerHandle != NULL);

  if ((Mask & HR_VALID_MASK) != Mask) {
    return (EFI_INVALID_PARAMETER);
  }

  if ((Mask & HR_CHILD_HANDLE) != 0 && DriverBindingHandle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *MatchingHandleCount = 0;
  if (MatchingHandleBuffer != NULL) {
    *MatchingHandleBuffer = NULL;
  }

  HandleBuffer  = NULL;
  HandleType    = NULL;

  Status = ParseHandleDatabaseByRelationshipWithType (
            DriverBindingHandle,
            ControllerHandle,
            &HandleCount,
            &HandleBuffer,
            &HandleType
           );
  if (!EFI_ERROR (Status)) {
    //
    // Count the number of handles that match the attributes in Mask
    //
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      if ((HandleType[HandleIndex] & Mask) == Mask) {
        (*MatchingHandleCount)++;
      }
    }
    //
    // If no handles match the attributes in Mask then return EFI_NOT_FOUND
    //
    if (*MatchingHandleCount == 0) {
      Status = EFI_NOT_FOUND;
    } else {

      if (MatchingHandleBuffer == NULL) {
        //
        // Someone just wanted the count...
        //
        Status = EFI_SUCCESS;
      } else {
        //
        // Allocate a handle buffer for the number of handles that matched the attributes in Mask
        //
        *MatchingHandleBuffer = AllocateZeroPool ((*MatchingHandleCount +1)* sizeof (EFI_HANDLE));
        if (*MatchingHandleBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          for (HandleIndex = 0, *MatchingHandleCount = 0
               ;  HandleIndex < HandleCount
               ;  HandleIndex++
               ) {
            //
            // Fill the allocated buffer with the handles that matched the attributes in Mask
            //
            if ((HandleType[HandleIndex] & Mask) == Mask) {
              (*MatchingHandleBuffer)[(*MatchingHandleCount)++] = HandleBuffer[HandleIndex];
            }
          }

          //
          // Make the last one NULL
          //
          (*MatchingHandleBuffer)[*MatchingHandleCount] = NULL;

          Status = EFI_SUCCESS;
        } // *MatchingHandleBuffer == NULL (ELSE)
      } // MacthingHandleBuffer == NULL (ELSE)
    } // *MatchingHandleCount  == 0 (ELSE)
  } // no error on ParseHandleDatabaseByRelationshipWithType

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (HandleType != NULL) {
    FreePool (HandleType);
  }

  ASSERT ((MatchingHandleBuffer == NULL) ||
          (*MatchingHandleCount == 0 && *MatchingHandleBuffer == NULL) ||
          (*MatchingHandleCount != 0 && *MatchingHandleBuffer != NULL));
  return Status;
}
