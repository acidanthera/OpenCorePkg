/** @file
  This code supports a the private implementation
  of the Data Hub protocol

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DATA_HUB_H_
#define _DATA_HUB_H_


#include <Uefi.h>

#include <Protocol/DataHub.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define DATA_HUB_INSTANCE_SIGNATURE SIGNATURE_32 ('D', 'H', 'u', 'b')
typedef struct {
  UINT32                Signature;

  EFI_HANDLE            Handle;

  //
  // Produced protocol(s)
  //
  EFI_DATA_HUB_PROTOCOL DataHub;

  //
  // Private Data
  //
  //
  // Updates to GlobalMonotonicCount, LogListHead, and FilterDriverListHead
  //  must be locked.
  //
  EFI_LOCK              DataLock;

  //
  // Runing Monotonic Count to use for each error record.
  //  Increment AFTER use in an error record.
  //
  UINT64                GlobalMonotonicCount;

  //
  // List of EFI_DATA_ENTRY structures. This is the data log! The list
  //  must be in assending order of LogMonotonicCount.
  //
  LIST_ENTRY            DataListHead;

  //
  // List of EFI_DATA_HUB_FILTER_DRIVER structures. Represents all
  //  the registered filter drivers.
  //
  LIST_ENTRY            FilterDriverListHead;

} DATA_HUB_INSTANCE;

#define DATA_HUB_INSTANCE_FROM_THIS(this) CR (this, DATA_HUB_INSTANCE, DataHub, DATA_HUB_INSTANCE_SIGNATURE)

//
// Private data structure to contain the data log. One record per
//  structure. Head pointer to the list is the Log member of
//  EFI_DATA_ENTRY. Record is a copy of the data passed in.
//
#define EFI_DATA_ENTRY_SIGNATURE  SIGNATURE_32 ('D', 'r', 'e', 'c')
typedef struct {
  UINT32                  Signature;
  LIST_ENTRY              Link;

  EFI_DATA_RECORD_HEADER  *Record;

  UINTN                   RecordSize;

} EFI_DATA_ENTRY;

#define DATA_ENTRY_FROM_LINK(link)  CR (link, EFI_DATA_ENTRY, Link, EFI_DATA_ENTRY_SIGNATURE)

//
// Private data to contain the filter driver Event and its
//  associated EFI_TPL.
//
#define EFI_DATA_HUB_FILTER_DRIVER_SIGNATURE  SIGNATURE_32 ('D', 'h', 'F', 'd')

typedef struct {
  UINT32          Signature;
  LIST_ENTRY      Link;

  //
  // Store Filter Driver Event and Tpl level it can be Signaled at.
  //
  EFI_EVENT       Event;
  EFI_TPL         Tpl;

  //
  // Monotonic count on the get next operation for Event.
  //  Zero indicates get next has not been called for this event yet.
  //
  UINT64          GetNextMonotonicCount;

  //
  // Filter driver will register what class filter should be used.
  //
  UINT64          ClassFilter;

  //
  // Filter driver will register what record guid filter should be used.
  //
  EFI_GUID        FilterDataRecordGuid;

} DATA_HUB_FILTER_DRIVER;

#define FILTER_ENTRY_FROM_LINK(link)  CR (link, DATA_HUB_FILTER_DRIVER, Link, EFI_DATA_HUB_FILTER_DRIVER_SIGNATURE)

#endif
