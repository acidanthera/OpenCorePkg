/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_PLATFORM_INFO_DATABASE_H
#define APPLE_PLATFORM_INFO_DATABASE_H

// APPLE_PLATFORM_INFO_DATABASE_PROTOCOL_REVISION
#define APPLE_PLATFORM_INFO_DATABASE_PROTOCOL_REVISION  0x00000001

// APPLE_PLATFORM_INFO_DATABASE_PROTOCOL_GUID
#define APPLE_PLATFORM_INFO_DATABASE_PROTOCOL_GUID        \
  { 0xAC5E4829, 0xA8FD, 0x440B,                           \
    { 0xAF, 0x33, 0x9F, 0xFE, 0x01, 0x3B, 0x12, 0xD8 } }

typedef
struct APPLE_PLATFORM_INFO_DATABASE_PROTOCOL
APPLE_PLATFORM_INFO_DATABASE_PROTOCOL;

// PLATFORM_INFO_GET_FIRST_DATA
typedef
EFI_STATUS
(EFIAPI *PLATFORM_INFO_GET_FIRST_DATA)(
  IN     APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *This,
  IN     EFI_GUID                               *NameGuid,
  IN OUT VOID                                   *Data, OPTIONAL
  IN OUT UINT32                                 *Size
  );

// PLATFORM_INFO_GET_FIRST_DATA_SIZE 
typedef
EFI_STATUS
(EFIAPI *PLATFORM_INFO_GET_FIRST_DATA_SIZE)(
  IN     APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *This,
  IN     EFI_GUID                               *NameGuid,
  IN OUT UINT32                                 *Size
  );

// PLATFORM_INFO_GET_DATA
typedef
EFI_STATUS
(EFIAPI *PLATFORM_INFO_GET_DATA)(
  IN     APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *This,
  IN     EFI_GUID                               *NameGuid,
  IN     UINTN                                  Index, OPTIONAL
  IN OUT VOID                                   *Data, OPTIONAL
  IN OUT UINT32                                 *Size
  );

// PLATFORM_INFO_GET_DATA_SIZE
typedef
EFI_STATUS
(EFIAPI *PLATFORM_INFO_GET_DATA_SIZE)(
  IN     APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *This,
  IN     EFI_GUID                               *NameGuid, OPTIONAL
  IN     UINTN                                  Index, OPTIONAL
  IN OUT UINT32                                 *Size
  );

// APPLE_PLATFORM_INFO_DATABASE_PROTOCOL
struct APPLE_PLATFORM_INFO_DATABASE_PROTOCOL {
  UINTN                             Revision;
  PLATFORM_INFO_GET_FIRST_DATA      GetFirstData;
  PLATFORM_INFO_GET_FIRST_DATA_SIZE GetFirstDataSize;
  PLATFORM_INFO_GET_DATA            GetData;
  PLATFORM_INFO_GET_DATA_SIZE       GetDataSize;
};

// gApplePlatformInfoDatabaseProtocolGuid
extern EFI_GUID gApplePlatformInfoDatabaseProtocolGuid;

#endif // APPLE_PLATFORM_INFO_DATABASE_H
