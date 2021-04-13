/** @file
  Provides interface to parsing both handle and protocol database.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2013-2016 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2021 vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HANDLE_PARSING_H
#define HANDLE_PARSING_H

typedef struct {
  LIST_ENTRY  Link;
  EFI_HANDLE  TheHandle;
  UINTN       TheIndex;
} HANDLE_LIST;

typedef struct {
  HANDLE_LIST   List;
  UINTN         NextIndex;
} HANDLE_INDEX_LIST;

#define HR_UNKNOWN                     0
#define HR_IMAGE_HANDLE                BIT1
#define HR_DRIVER_BINDING_HANDLE       BIT2 // has driver binding
#define HR_DEVICE_DRIVER               BIT3 // device driver (hybrid?)
#define HR_BUS_DRIVER                  BIT4 // a bus driver  (hybrid?)
#define HR_DRIVER_CONFIGURATION_HANDLE BIT5
#define HR_DRIVER_DIAGNOSTICS_HANDLE   BIT6
#define HR_COMPONENT_NAME_HANDLE       BIT7
#define HR_DEVICE_HANDLE               BIT8
#define HR_PARENT_HANDLE               BIT9
#define HR_CONTROLLER_HANDLE           BIT10
#define HR_CHILD_HANDLE                BIT11
#define HR_VALID_MASK                  (BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11)

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
  );

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
  );

/**
  Gets handles for any UEFI drivers of the passed in controller.

  @param[in] ControllerHandle       The handle of the controller.
  @param[in] Count                  The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] Buffer                The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
#define PARSE_HANDLE_DATABASE_UEFI_DRIVERS(ControllerHandle, Count, Buffer) \
  InternalParseHandleDatabaseByRelationship(NULL, ControllerHandle, HR_DRIVER_BINDING_HANDLE|HR_DEVICE_DRIVER, Count, Buffer)


#endif // HANDLE_PARSING_H
