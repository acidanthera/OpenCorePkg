/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_PARTITION_INFO_H
#define APPLE_PARTITION_INFO_H

// APPLE_PARTITION_INFO_PROTOCOL_GUID
#define APPLE_PARTITION_INFO_PROTOCOL_GUID  \
  { 0x68425EE5, 0x1C43, 0x4BAA,             \
    { 0x84, 0xF7, 0x9A, 0xA8, 0xA4, 0xD8, 0xE1, 0x1E } }

// APPLE_PARTITION_INFO_PROTOCOL
typedef struct {
  UINT32   Revision;
  UINT32   PartitionNumber;
  UINT64   PartitionStart;
  UINT64   PartitionSize;
  UINT8    Signature[16];
  UINT8    MBRType;
  UINT8    SignatureType;
  UINT64   Attributes;
  CHAR16   PartitionName[36];
  UINT8    PartitionType[16];
} APPLE_PARTITION_INFO_PROTOCOL;

// gApplePartitionInfoProtocolGuid
extern EFI_GUID gApplePartitionInfoProtocolGuid;

#endif // APPLE_PARTITION_INFO_H
