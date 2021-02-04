/** @file
  APM partitioning scheme.

Copyright (c) 2016-2021, Acidanthera. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef APM_H
#define APM_H

#pragma pack(1)

#define MBR_TYPE_APPLE_PARTITION_TABLE_HEADER  0x20

#define APM_ENTRY_TYPE_APM   "Apple_partition_map"
#define APM_ENTRY_TYPE_FREE  "Apple_Free"

typedef PACKED struct {
  UINT32 BlockStart;
  UINT16 NumberOfBlocks;
  UINT16 SystemType;
} APM_DRIVER_DESCRIPTOR_MAP_ENTRY;

#define APM_DRIVER_DESCRIPTOR_MAP_SIGNATURE  SIGNATURE_16 ('E', 'R')

//
// APM Driver Descriptor Map Header
//
typedef PACKED struct {
  UINT16                          Signature;
  UINT16                          BlockSize;
  UINT32                          BlockCount;
  UINT16                          DeviceType;
  UINT16                          DeviceId;
  UINT32                          DriverData;
  UINT16                          DriverDescriptorCount;
  APM_DRIVER_DESCRIPTOR_MAP_ENTRY DriverDescriptors[8];
  UINT8                           Reserved[430];
} APM_DRIVER_DESCRIPTOR_MAP;

#define APM_ENTRY_SIGNATURE  SIGNATURE_16 ('P', 'M')

//
// APM Entry Flags
//
#define APM_ENTRY_FLAGS_VALID          BIT0
#define APM_ENTRY_FLAGS_ALLOCATED      BIT1
#define APM_ENTRY_FLAGS_IN_USE         BIT2
#define APM_ENTRY_FLAGS_BOOTABLE       BIT3
#define APM_ENTRY_FLAGS_READABLE       BIT4
#define APM_ENTRY_FLAGS_WRITABLE       BIT5
#define APM_ENTRY_FLAGS_OS_PIC_CODE    BIT6
#define APM_ENTRY_FLAGS_OS_SPECIFIC_2  BIT7
#define APM_ENTRY_FLAGS_OS_SPECIFIC_1  BIT8
#define APM_ENTRY_FLAGS_RESERVED       0xFFFFFE00

//
// APM Entry Header
//
typedef PACKED struct {
  UINT16 Signature;
  UINT16 Reserved1;
  UINT32 NumberOfPartitionEntries;
  UINT32 PartitionStart;
  UINT32 PartitionSize;
  CHAR8  PartitionName[32];
  CHAR8  PartitionType[32];
  UINT32 LBAStart;
  UINT32 LBASize;
  UINT32 PartitionFlags;
  UINT32 BootStrapCodeLBA;
  UINT32 BootStrapCodeSize;
  UINT32 BootStrapCodeLoadAddress;
  UINT32 BootStrapCodeLoadAddress2;
  UINT32 BootStrapCodeEntry;
  UINT32 BootStrapCodeEntry2;
  UINT32 BootStrapCodeChecksum;
  UINT8  ProcessorType[16];
  UINT32 Reserved2[32];
  UINT32 Reserved3[62];
} APM_ENTRY;

#pragma pack()

#endif // APM_H
