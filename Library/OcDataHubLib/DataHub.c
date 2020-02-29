/** @file
  This code produces the Data Hub protocol. It preloads the data hub
  with status information copied in from PEI HOBs.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DataHub.h"

//
//  Since this driver will only ever produce one instance of the Logging Hub
//  protocol you are not required to dynamically allocate the PrivateData.
//
DATA_HUB_INSTANCE mPrivateData;

/**
  Log data record into the data logging hub

  @param This                   Protocol instance structure
  @param DataRecordGuid         GUID that defines record contents
  @param ProducerName           GUID that defines the name of the producer of the data
  @param DataRecordClass        Class that defines generic record type
  @param RawData                Data Log record as defined by DataRecordGuid
  @param RawDataSize            Size of Data Log data in bytes

  @retval EFI_SUCCESS           If data was logged
  @retval EFI_OUT_OF_RESOURCES  If data was not logged due to lack of system
                                resources.
**/
EFI_STATUS
EFIAPI
DataHubLogData (
  IN  EFI_DATA_HUB_PROTOCOL   *This,
  IN  EFI_GUID                *DataRecordGuid,
  IN  EFI_GUID                *ProducerName,
  IN  UINT64                  DataRecordClass,
  IN  VOID                    *RawData,
  IN  UINT32                  RawDataSize
  )
{
  EFI_STATUS              Status;
  DATA_HUB_INSTANCE       *Private;
  EFI_DATA_ENTRY          *LogEntry;
  UINT32                  TotalSize;
  UINT32                  RecordSize;
  EFI_DATA_RECORD_HEADER  *Record;
  VOID                    *Raw;
  DATA_HUB_FILTER_DRIVER  *FilterEntry;
  LIST_ENTRY              *Link;
  LIST_ENTRY              *Head;
  EFI_TIME                LogTime;

  Private = DATA_HUB_INSTANCE_FROM_THIS (This);

  //
  // Combine the storage for the internal structs and a copy of the log record.
  //  Record follows PrivateLogEntry. The consumer will be returned a pointer
  //  to Record so we don't what it to be the thing that was allocated from
  //  pool, so the consumer can't free an data record by mistake.
  //
  RecordSize  = sizeof (EFI_DATA_RECORD_HEADER) + RawDataSize;
  TotalSize   = sizeof (EFI_DATA_ENTRY) + RecordSize;

  //
  // First try to get log time at TPL level <= TPL_CALLBACK.
  //
  ZeroMem (&LogTime, sizeof (LogTime));
  if (EfiGetCurrentTpl() <= TPL_CALLBACK) {
    gRT->GetTime (&LogTime, NULL);
  }

  //
  // The Logging action is the critical section, so it is locked.
  //  The MTC asignment & update and logging must be an
  //  atomic operation, so use the lock.
  //
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    //
    // Reentrancy detected so exit!
    //
    return Status;
  }

  LogEntry = AllocatePool (TotalSize);

  if (LogEntry == NULL) {
    EfiReleaseLock (&Private->DataLock);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (LogEntry, TotalSize);

  Record  = (EFI_DATA_RECORD_HEADER *) (LogEntry + 1);
  Raw     = (VOID *) (Record + 1);

  //
  // Build Standard Log Header
  //
  Record->Version     = EFI_DATA_RECORD_HEADER_VERSION;
  Record->HeaderSize  = (UINT16) sizeof (EFI_DATA_RECORD_HEADER);
  Record->RecordSize  = RecordSize;
  CopyMem (&Record->DataRecordGuid, DataRecordGuid, sizeof (EFI_GUID));
  CopyMem (&Record->ProducerName, ProducerName, sizeof (EFI_GUID));
  Record->DataRecordClass   = DataRecordClass;

  //
  // Ensure LogMonotonicCount is not zero
  //
  Record->LogMonotonicCount = ++Private->GlobalMonotonicCount;

  CopyMem (&Record->LogTime, &LogTime, sizeof (LogTime));

  //
  // Insert log into the internal linked list.
  //
  LogEntry->Signature   = EFI_DATA_ENTRY_SIGNATURE;
  LogEntry->Record      = Record;
  LogEntry->RecordSize  = sizeof (EFI_DATA_ENTRY) + RawDataSize;
  InsertTailList (&Private->DataListHead, &LogEntry->Link);

  CopyMem (Raw, RawData, RawDataSize);

  EfiReleaseLock (&Private->DataLock);

  //
  // Send Signal to all the filter drivers which are interested
  //  in the record's class and guid.
  //
  Head = &Private->FilterDriverListHead;
  for (Link = GetFirstNode(Head); Link != Head; Link = GetNextNode(Head, Link)) {
    FilterEntry = FILTER_ENTRY_FROM_LINK (Link);
    if (((FilterEntry->ClassFilter & DataRecordClass) != 0) &&
        (IsZeroGuid (&FilterEntry->FilterDataRecordGuid) ||
         CompareGuid (&FilterEntry->FilterDataRecordGuid, DataRecordGuid))) {
      gBS->SignalEvent (FilterEntry->Event);
    }
  }

  return EFI_SUCCESS;
}

/**
  Search the Head doubly linked list for the passed in MTC. Return the
  matching element in Head and the MTC on the next entry.

  @param Head             Head of Data Log linked list.
  @param ClassFilter      Only match the MTC if it is in the same Class as the
                          ClassFilter.
  @param PtrCurrentMTC    On IN contians MTC to search for. On OUT contians next
                          MTC in the data log list or zero if at end of the list.

  @retval EFI_DATA_LOG_ENTRY  Return pointer to data log data from Head list.
  @retval NULL                If no data record exists.

**/
EFI_DATA_RECORD_HEADER *
GetNextDataRecord (
  IN  LIST_ENTRY          *Head,
  IN  UINT64              ClassFilter,
  IN OUT  UINT64          *PtrCurrentMTC
  )

{
  EFI_DATA_ENTRY          *LogEntry;
  LIST_ENTRY              *Link;
  BOOLEAN                 ReturnFirstEntry;
  EFI_DATA_RECORD_HEADER  *Record;
  EFI_DATA_ENTRY          *NextLogEntry;

  //
  // If MonotonicCount == 0 just return the first one
  //
  ReturnFirstEntry  = (BOOLEAN) (*PtrCurrentMTC == 0);

  Record            = NULL;
  for (Link = GetFirstNode(Head); Link != Head; Link = GetNextNode(Head, Link)) {
    LogEntry = DATA_ENTRY_FROM_LINK (Link);
    if ((LogEntry->Record->DataRecordClass & ClassFilter) == 0) {
      //
      // Skip any entry that does not have the correct ClassFilter
      //
      continue;
    }

    if ((LogEntry->Record->LogMonotonicCount == *PtrCurrentMTC) || ReturnFirstEntry) {
      //
      // Return record to the user
      //
      Record = LogEntry->Record;

      //
      // Calculate the next MTC value. If there is no next entry set
      // MTC to zero.
      //
      *PtrCurrentMTC = 0;
      for (Link = GetNextNode(Head, Link); Link != Head; Link = GetNextNode(Head, Link)) {
        NextLogEntry = DATA_ENTRY_FROM_LINK (Link);
        if ((NextLogEntry->Record->DataRecordClass & ClassFilter) != 0) {
          //
          // Return the MTC of the next thing to search for if found
          //
          *PtrCurrentMTC = NextLogEntry->Record->LogMonotonicCount;
          break;
        }
      }
      //
      // Record found exit loop and return
      //
      break;
    }
  }

  return Record;
}

/**
  Search the Head list for a EFI_DATA_HUB_FILTER_DRIVER member that
  represents Event and return it.

  @param Head   Pointer to head of dual linked list of EFI_DATA_HUB_FILTER_DRIVER structures.
  @param Event  Event to be search for in the Head list.

  @retval EFI_DATA_HUB_FILTER_DRIVER  Returned if Event stored in the Head doubly linked list.
  @retval NULL                        If Event is not in the list

**/
DATA_HUB_FILTER_DRIVER *
FindFilterDriverByEvent (
  IN  LIST_ENTRY      *Head,
  IN  EFI_EVENT       Event
  )
{
  DATA_HUB_FILTER_DRIVER  *FilterEntry;
  LIST_ENTRY              *Link;

  for (Link = GetFirstNode(Head); Link != Head; Link = GetNextNode(Head, Link)) {
    FilterEntry = FILTER_ENTRY_FROM_LINK (Link);
    if (FilterEntry->Event == Event) {
      return FilterEntry;
    }
  }

  return NULL;
}

/**

  Get a previously logged data record and the MonotonicCount for the next
  available Record. This allows all records or all records later
  than a give MonotonicCount to be returned. If an optional FilterDriverEvent
  is passed in with a MonotonicCout of zero return the first record
  not yet read by the filter driver. If FilterDriverEvent is NULL and
  MonotonicCount is zero return the first data record.

  @param This                     Pointer to the EFI_DATA_HUB_PROTOCOL instance.
  @param MonotonicCount           Specifies the Record to return. On input, zero means
                                  return the first record. On output, contains the next
                                  record to available. Zero indicates no more records.
  @param FilterDriverEvent        If FilterDriverEvent is not passed in a MonotonicCount
                                  of zero, it means to return the first data record.
                                  If FilterDriverEvent is passed in, then a MonotonicCount
                                  of zero means to return the first data not yet read by
                                  FilterDriverEvent.
  @param Record                   Returns a dynamically allocated memory buffer with a data
                                  record that matches MonotonicCount.

  @retval EFI_SUCCESS             Data was returned in Record.
  @retval EFI_INVALID_PARAMETER   FilterDriverEvent was passed in but does not exist.
  @retval EFI_NOT_FOUND           MonotonicCount does not match any data record in the
                                  system. If a MonotonicCount of zero was passed in, then
                                  no data records exist in the system.
  @retval EFI_OUT_OF_RESOURCES    Record was not returned due to lack of system resources.

**/
EFI_STATUS
EFIAPI
DataHubGetNextRecord (
  IN EFI_DATA_HUB_PROTOCOL            *This,
  IN OUT UINT64                       *MonotonicCount,
  IN EFI_EVENT                        *FilterDriverEvent, OPTIONAL
  OUT EFI_DATA_RECORD_HEADER          **Record
  )
{
  DATA_HUB_INSTANCE       *Private;
  DATA_HUB_FILTER_DRIVER  *FilterDriver;
  UINT64                  ClassFilter;

  Private               = DATA_HUB_INSTANCE_FROM_THIS (This);

  FilterDriver          = NULL;
  ClassFilter = EFI_DATA_RECORD_CLASS_DEBUG |
    EFI_DATA_RECORD_CLASS_ERROR |
    EFI_DATA_RECORD_CLASS_DATA |
    EFI_DATA_RECORD_CLASS_PROGRESS_CODE;

  //
  // If FilterDriverEvent is NULL, then return the next record
  //
  if (FilterDriverEvent == NULL) {
    *Record = GetNextDataRecord (&Private->DataListHead, ClassFilter, MonotonicCount);
    if (*Record == NULL) {
      return EFI_NOT_FOUND;
    }
    return EFI_SUCCESS;
  }

  //
  // For events the beginning is the last unread record. This info is
  // stored in the instance structure, so we must look up the event
  // to get the data.
  //
  FilterDriver = FindFilterDriverByEvent (
                  &Private->FilterDriverListHead,
                  *FilterDriverEvent
                  );
  if (FilterDriver == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Use the Class filter the event was created with.
  //
  ClassFilter = FilterDriver->ClassFilter;

  //
  // Retrieve the next record or the first record.
  //
  if (*MonotonicCount != 0 || FilterDriver->GetNextMonotonicCount == 0) {
    *Record = GetNextDataRecord (&Private->DataListHead, ClassFilter, MonotonicCount);
    if (*Record == NULL) {
      return EFI_NOT_FOUND;
    }

    if (*MonotonicCount != 0) {
      //
      // If this was not the last record then update the count associated with the filter
      //
      FilterDriver->GetNextMonotonicCount = *MonotonicCount;
    } else {
      //
      // Save the MonotonicCount of the last record which has been read
      //
      FilterDriver->GetNextMonotonicCount = (*Record)->LogMonotonicCount;
    }
    return EFI_SUCCESS;
  }

  //
  // This is a request to read the first record that has not been read yet.
  // Set MonotoicCount to the last record successfuly read
  //
  *MonotonicCount = FilterDriver->GetNextMonotonicCount;

  //
  // Retrieve the last record successfuly read again, but do not return it since
  // it has already been returned before.
  //
  *Record = GetNextDataRecord (&Private->DataListHead, ClassFilter, MonotonicCount);
  if (*Record == NULL) {
    return EFI_NOT_FOUND;
  }

  if (*MonotonicCount != 0) {
    //
    // Update the count associated with the filter
    //
    FilterDriver->GetNextMonotonicCount = *MonotonicCount;

    //
    // Retrieve the record after the last record successfuly read
    //
    *Record = GetNextDataRecord (&Private->DataListHead, ClassFilter, MonotonicCount);
    if (*Record == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function registers the data hub filter driver that is represented
  by FilterEvent. Only one instance of each FilterEvent can be registered.
  After the FilterEvent is registered, it will be signaled so it can sync
  with data records that have been recorded prior to the FilterEvent being
  registered.

  @param This                   Pointer to  The EFI_DATA_HUB_PROTOCOL instance.
  @param FilterEvent            The EFI_EVENT to signal whenever data that matches
                                FilterClass is logged in the system.
  @param FilterTpl              The maximum EFI_TPL at which FilterEvent can be
                                signaled. It is strongly recommended that you use the
                                lowest EFI_TPL possible.
  @param FilterClass            FilterEvent will be signaled whenever a bit in
                                EFI_DATA_RECORD_HEADER.DataRecordClass is also set in
                                FilterClass. If FilterClass is zero, no class-based
                                filtering will be performed.
  @param FilterDataRecordGuid   FilterEvent will be signaled whenever FilterDataRecordGuid
                                matches EFI_DATA_RECORD_HEADER.DataRecordGuid. If
                                FilterDataRecordGuid is NULL, then no GUID-based filtering
                                will be performed.

  @retval EFI_SUCCESS           The filter driver event was registered.
  @retval EFI_ALREADY_STARTED   FilterEvent was previously registered and cannot be
                                registered again.
  @retval EFI_OUT_OF_RESOURCES  The filter driver event was not registered due to lack of
                                system resources.

**/
EFI_STATUS
EFIAPI
DataHubRegisterFilterDriver (
  IN EFI_DATA_HUB_PROTOCOL    * This,
  IN EFI_EVENT                FilterEvent,
  IN EFI_TPL                  FilterTpl,
  IN UINT64                   FilterClass,
  IN EFI_GUID                 * FilterDataRecordGuid OPTIONAL
  )

{
  DATA_HUB_INSTANCE       *Private;
  DATA_HUB_FILTER_DRIVER  *FilterDriver;

  Private       = DATA_HUB_INSTANCE_FROM_THIS (This);

  FilterDriver  = (DATA_HUB_FILTER_DRIVER *) AllocateZeroPool (sizeof (DATA_HUB_FILTER_DRIVER));
  if (FilterDriver == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize filter driver info
  //
  FilterDriver->Signature             = EFI_DATA_HUB_FILTER_DRIVER_SIGNATURE;
  FilterDriver->Event                 = FilterEvent;
  FilterDriver->Tpl                   = FilterTpl;
  FilterDriver->GetNextMonotonicCount = 0;
  if (FilterClass == 0) {
    FilterDriver->ClassFilter = EFI_DATA_RECORD_CLASS_DEBUG |
      EFI_DATA_RECORD_CLASS_ERROR |
      EFI_DATA_RECORD_CLASS_DATA |
      EFI_DATA_RECORD_CLASS_PROGRESS_CODE;
  } else {
    FilterDriver->ClassFilter = FilterClass;
  }

  if (FilterDataRecordGuid != NULL) {
    CopyMem (&FilterDriver->FilterDataRecordGuid, FilterDataRecordGuid, sizeof (EFI_GUID));
  }
  //
  // Search for duplicate entries
  //
  if (FindFilterDriverByEvent (&Private->FilterDriverListHead, FilterEvent) != NULL) {
    FreePool (FilterDriver);
    return EFI_ALREADY_STARTED;
  }
  //
  // Make insertion an atomic operation with the lock.
  //
  EfiAcquireLock (&Private->DataLock);
  InsertTailList (&Private->FilterDriverListHead, &FilterDriver->Link);
  EfiReleaseLock (&Private->DataLock);

  //
  // Signal the Filter driver we just loaded so they will recieve all the
  // previous history. If we did not signal here we would have to wait until
  // the next data was logged to get the history. In a case where no next
  // data was logged we would never get synced up.
  //
  gBS->SignalEvent (FilterEvent);

  return EFI_SUCCESS;
}

/**
  Remove a Filter Driver, so it no longer gets called when data
   information is logged.

  @param This           Protocol instance structure

  @param FilterEvent    Event that represents a filter driver that is to be
                        Unregistered.

  @retval EFI_SUCCESS   If FilterEvent was unregistered
  @retval EFI_NOT_FOUND If FilterEvent does not exist
**/
EFI_STATUS
EFIAPI
DataHubUnregisterFilterDriver (
  IN EFI_DATA_HUB_PROTOCOL    *This,
  IN EFI_EVENT                FilterEvent
  )
{
  DATA_HUB_INSTANCE       *Private;
  DATA_HUB_FILTER_DRIVER  *FilterDriver;

  Private = DATA_HUB_INSTANCE_FROM_THIS (This);

  //
  // Search for duplicate entries
  //
  FilterDriver = FindFilterDriverByEvent (
                  &Private->FilterDriverListHead,
                  FilterEvent
                  );
  if (FilterDriver == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Make removal an atomic operation with the lock
  //
  EfiAcquireLock (&Private->DataLock);
  RemoveEntryList (&FilterDriver->Link);
  EfiReleaseLock (&Private->DataLock);

  return EFI_SUCCESS;
}


//
// CHANGE: Function prototype has been adapted to fit the other OC APIs.
//
/**
  Driver's Entry point routine that install Driver to produce Data Hub protocol.

**/
EFI_DATA_HUB_PROTOCOL *
DataHubInstall (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      HighMontonicCount;

  mPrivateData.Signature                      = DATA_HUB_INSTANCE_SIGNATURE;
  mPrivateData.DataHub.LogData                = DataHubLogData;
  mPrivateData.DataHub.GetNextRecord          = DataHubGetNextRecord;
  mPrivateData.DataHub.RegisterFilterDriver   = DataHubRegisterFilterDriver;
  mPrivateData.DataHub.UnregisterFilterDriver = DataHubUnregisterFilterDriver;

  //
  // Initialize Private Data in CORE_LOGGING_HUB_INSTANCE that is
  // required by this protocol
  //
  InitializeListHead (&mPrivateData.DataListHead);
  InitializeListHead (&mPrivateData.FilterDriverListHead);

  EfiInitializeLock (&mPrivateData.DataLock, TPL_NOTIFY);

  //
  // Make sure we get a bigger MTC number on every boot!
  //
  Status = gRT->GetNextHighMonotonicCount (&HighMontonicCount);
  if (EFI_ERROR (Status)) {
    //
    // if system service fails pick a sane value.
    //
    mPrivateData.GlobalMonotonicCount = 0;
  } else {
    mPrivateData.GlobalMonotonicCount = LShiftU64 ((UINT64) HighMontonicCount, 32);
  }
  //
  // Make a new handle and install the protocol
  //
  mPrivateData.Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiDataHubProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.DataHub
                  );
  //
  // CHANGE: Return the interface instead of the install status.
  //
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mPrivateData.DataHub;
}

