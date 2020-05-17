/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DATA_HUB_LIB_H
#define APPLE_DATA_HUB_LIB_H

// DataHubLogData
VOID
DataHubLogData (
  IN  EFI_GUID  *DataRecordGuid,
  IN  EFI_GUID  *ProducerName,
  IN  VOID      *RawData,
  IN  UINT32    RawDataSize
  );

// DataHubLogApplePlatformData
VOID
DataHubLogApplePlatformData (
  IN CHAR16    *Key,
  IN EFI_GUID  *DataRecordGuid,
  IN VOID      *Value,
  IN UINTN     ValueSize
  );

#endif // APPLE_DATA_HUB_LIB_H
