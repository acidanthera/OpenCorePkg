/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_APFS_INFO_H
#define APPLE_APFS_INFO_H

#define APPLE_APFS_PARTITION_TYPE_GUID  \
  { 0x7C3457EF, 0x0000, 0x11AA,         \
    { 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } }

extern EFI_GUID gAppleApfsPartitionTypeGuid;

#define APPLE_APFS_CONTAINER_INFO_GUID  \
  { 0x3533CF0D, 0x685F, 0x5EBF,         \
    { 0x8D, 0xC6, 0x73, 0x93, 0x48, 0x5B, 0xAF, 0xA2 } }

typedef struct {
  UINT32 Always1;
  GUID   Uuid;
} APPLE_APFS_CONTAINER_INFO;

extern EFI_GUID gAppleApfsContainerInfoGuid;

#define APPLE_APFS_VOLUME_INFO_GUID  \
  { 0x900C7693, 0x8C14, 0x58BA,      \
    { 0xB4, 0x4E, 0x97, 0x45, 0x15, 0xD2, 0x7C, 0x78 } }


#define APPLE_APFS_VOLUME_ROLE_RECOVERY  BIT2
#define APPLE_APFS_VOLUME_ROLE_PREBOOT   BIT4

typedef UINT32 APPLE_APFS_VOLUME_ROLE;

typedef struct {
  UINT32                 Always1;
  GUID                   Uuid;
  APPLE_APFS_VOLUME_ROLE Role;
} APPLE_APFS_VOLUME_INFO;

extern EFI_GUID gAppleApfsVolumeInfoGuid;

#endif // APPLE_APFS_H
