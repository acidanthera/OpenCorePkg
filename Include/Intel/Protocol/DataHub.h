/** @file
  The data hub protocol is used both by agents wishing to log
  data and those wishing to be made aware of all information that
  has been logged.  This protocol may only be called <= TPL_NOTIFY.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  The Data Hub Protocol is defined in Framework for EFI Data Hub Specification
  Version 0.9.

**/

#ifndef __DATA_HUB_H__
#define __DATA_HUB_H__

#define EFI_DATA_HUB_PROTOCOL_GUID \
  { \
    0xae80d021, 0x618e, 0x11d4, {0xbc, 0xd7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

//
// EFI generic Data Hub Header
//
// A Data Record is an EFI_DATA_RECORD_HEADER followed by RecordSize bytes of
//  data. The format of the data is defined by the DataRecordGuid.
//
// If EFI_DATA_RECORD_HEADER is extended in the future, the Version number and HeaderSize must
//  change.
//
// The logger is responcible for initializing:
//  Version, HeaderSize, RecordSize, DataRecordGuid, DataRecordClass
//
// The Data Hub driver is responcible for initializing:
//   LogTime and LogMonotonicCount.
//
#define EFI_DATA_RECORD_HEADER_VERSION  0x0100
typedef struct {
  UINT16      Version;
  UINT16      HeaderSize;
  UINT32      RecordSize;
  EFI_GUID    DataRecordGuid;
  EFI_GUID    ProducerName;
  UINT64      DataRecordClass;
  EFI_TIME    LogTime;
  UINT64      LogMonotonicCount;
} EFI_DATA_RECORD_HEADER;

//
// Definition of DataRecordClass. These are used to filter out class types
// at a very high level. The DataRecordGuid still defines the format of
// the data. See the Data Hub Specification for rules on what can and can not be a
// new DataRecordClass
//
#define EFI_DATA_RECORD_CLASS_DEBUG          0x0000000000000001
#define EFI_DATA_RECORD_CLASS_ERROR          0x0000000000000002
#define EFI_DATA_RECORD_CLASS_DATA           0x0000000000000004
#define EFI_DATA_RECORD_CLASS_PROGRESS_CODE  0x0000000000000008

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_DATA_HUB_PROTOCOL EFI_DATA_HUB_PROTOCOL;

/**
  Logs a data record to the system event log.

  @param  This                  The EFI_DATA_HUB_PROTOCOL instance.
  @param  DataRecordGuid        A GUID that indicates the format of the data passed into RawData.
  @param  ProducerName          A GUID that indicates the identity of the caller to this API.
  @param  DataRecordClass       This class indicates the generic type of the data record.
  @param  RawData               The DataRecordGuid-defined data to be logged.
  @param  RawDataSize           The size in bytes of RawData.

  @retval EFI_SUCCESS           Data was logged.
  @retval EFI_OUT_OF_RESOURCES  Data was not logged due to lack of system resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_LOG_DATA)(
  IN  EFI_DATA_HUB_PROTOCOL   *This,
  IN  EFI_GUID                *DataRecordGuid,
  IN  EFI_GUID                *ProducerName,
  IN  UINT64                  DataRecordClass,
  IN  VOID                    *RawData,
  IN  UINT32                  RawDataSize
  );

/**
  Allows the system data log to be searched.

  @param  This                  The EFI_DATA_HUB_PROTOCOL instance.
  @param  MonotonicCount        On input, it specifies the Record to return.
                                An input of zero means to return the first record,
                                as does an input of one.
  @param  FilterDriver          If FilterDriver is not passed in a MonotonicCount
                                of zero, it means to return the first data record.
                                If FilterDriver is passed in, then a MonotonicCount
                                of zero means to return the first data not yet read
                                by FilterDriver.
  @param  Record                Returns a dynamically allocated memory buffer with
                                a data record that matches MonotonicCount.

  @retval EFI_SUCCESS           Data was returned in Record.
  @retval EFI_INVALID_PARAMETER FilterDriver was passed in but does not exist.
  @retval EFI_NOT_FOUND         MonotonicCount does not match any data record
                                in the system. If a MonotonicCount of zero was
                                passed in, then no data records exist in the system.
  @retval EFI_OUT_OF_RESOURCES  Record was not returned due to lack
                                of system resources.
  @note  Inconsistent with specification here:
         In Framework for EFI Data Hub Specification, Version 0.9, This definition
         is named as EFI_DATA_HUB_GET_NEXT_DATA_RECORD. The inconsistency is
         maintained for backward compatibility.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_GET_NEXT_RECORD)(
  IN EFI_DATA_HUB_PROTOCOL    *This,
  IN OUT  UINT64              *MonotonicCount,
  IN  EFI_EVENT               *FilterDriver OPTIONAL,
  OUT EFI_DATA_RECORD_HEADER  **Record
  );

/**
  Registers an event to be signaled every time a data record is logged in the system.

  @param  This                  The EFI_DATA_HUB_PROTOCOL instance.
  @param  FilterEvent           The EFI_EVENT to signal whenever data that matches
                                FilterClass is logged in the system.
  @param  FilterTpl             The maximum EFI_TPL at which FilterEvent can be
                                signaled. It is strongly recommended that you use
                                the lowest EFI_TPL possible.
  @param  FilterClass           FilterEvent will be signaled whenever a bit
                                in EFI_DATA_RECORD_HEADER.DataRecordClass is also
                                set in FilterClass. If FilterClass is zero, no
                                class-based filtering will be performed.
  @param  FilterDataRecordGuid  FilterEvent will be signaled whenever
                                FilterDataRecordGuid matches
                                EFI_DATA_RECORD_HEADER.DataRecordGuid.
                                If FilterDataRecordGuid is NULL, then no GUID-based
                                filtering will be performed.

  @retval EFI_SUCCESS           The filter driver event was registered
  @retval EFI_ALREADY_STARTED   FilterEvent was previously registered and cannot
                                be registered again.
  @retval EFI_OUT_OF_RESOURCES  The filter driver event was not registered
                                due to lack of system resources.
  @note  Inconsistent with specification here:
         In Framework for EFI Data Hub Specification, Version 0.9, This definition
         is named as EFI_DATA_HUB_REGISTER_DATA_FILTER_DRIVER. The inconsistency
         is maintained for backward compatibility.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_REGISTER_FILTER_DRIVER)(
  IN EFI_DATA_HUB_PROTOCOL    *This,
  IN EFI_EVENT                FilterEvent,
  IN EFI_TPL                  FilterTpl,
  IN UINT64                   FilterClass,
  IN EFI_GUID                 *FilterDataRecordGuid OPTIONAL
  );

/**
  Stops a filter driver from being notified when data records are logged.

  @param  This                  The EFI_DATA_HUB_PROTOCOL instance.
  @param  FilterEvent           The EFI_EVENT to remove from the list of events to be
                                signaled every time errors are logged.

  @retval EFI_SUCCESS           The filter driver represented by FilterEvent was shut off.
  @retval EFI_NOT_FOUND         FilterEvent did not exist.
  @note  Inconsistent with specification here:
         In Framework for EFI Data Hub Specification, Version 0.9, This definition
         is named as EFI_DATA_HUB_UNREGISTER_DATA_FILTER_DRIVER. The inconsistency
         is maintained for backward compatibility.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_UNREGISTER_FILTER_DRIVER)(
  IN EFI_DATA_HUB_PROTOCOL    *This,
  IN EFI_EVENT                FilterEvent
  );

/**
  This protocol is used to log information and register filter drivers
  to receive data records.
**/
struct _EFI_DATA_HUB_PROTOCOL {
  ///
  /// Logs a data record.
  ///
  EFI_DATA_HUB_LOG_DATA                    LogData;

  ///
  /// Gets a data record. Used both to view the memory-based log and to
  /// get information about which data records have been consumed by a filter driver.
  ///
  EFI_DATA_HUB_GET_NEXT_RECORD             GetNextRecord;

  ///
  /// Allows the registration of an EFI event to act as a filter driver for all data records that are logged.
  ///
  EFI_DATA_HUB_REGISTER_FILTER_DRIVER      RegisterFilterDriver;

  ///
  /// Used to remove a filter driver that was added with RegisterFilterDriver().
  ///
  EFI_DATA_HUB_UNREGISTER_FILTER_DRIVER    UnregisterFilterDriver;
};

extern EFI_GUID  gEfiDataHubProtocolGuid;

#endif
