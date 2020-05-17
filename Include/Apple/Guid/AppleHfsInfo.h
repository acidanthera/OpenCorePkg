/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_HFS_INFO_H
#define APPLE_HFS_INFO_H

/**
  Normal HFS+ volume.
**/
#define APPLE_HFS_PARTITION_TYPE_GUID   \
  { 0x48465300, 0x0000, 0x11AA,         \
    { 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } }

extern EFI_GUID gAppleHfsPartitionTypeGuid;

/**
  CoreStorage HFS+ volume.
**/
#define APPLE_HFS_CS_PARTITION_TYPE_GUID  \
  { 0x53746F72, 0x6167, 0x11AA,           \
    { 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } }

extern EFI_GUID gAppleHfsCsPartitionTypeGuid;

/**
  Boot (Recovery) HFS+ volume.
**/
#define APPLE_HFS_BOOT_PARTITION_TYPE_GUID  \
  { 0x426F6F74, 0x0000, 0x11AA,             \
    { 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } }

extern EFI_GUID gAppleHfsBootPartitionTypeGuid;

/**
  Accessible from EFI_FILE_PROTOCOL::GetInfo, this GUID
  allows to quickly obtain volume UUID.
**/
#define APPLE_HFS_UUID_INFO_GUID \
  { 0xFA99420C, 0x88F1, 0x11E7,  \
    { 0x95, 0xF6, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA } }

extern EFI_GUID gAppleHfsUuidInfoGuid;

#endif // APPLE_HFS_INFO_H
