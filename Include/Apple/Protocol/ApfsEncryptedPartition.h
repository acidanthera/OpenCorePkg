/** @file
  Apple FileSystem encrypted partition.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APFS_ENCRYPTED_PARTITION_H
#define APFS_ENCRYPTED_PARTITION_H

/**
  Set for APFSLCKD partitions.
  59D76AE4-37E3-55A7-B460-EF13D46E6020
**/

#define APFS_ENCRYPTED_PARTITION_PROTOCOL_GUID \
  { 0x59D76AE4, 0x37E3, 0x55A7,                \
    { 0xB4, 0x60, 0xEF, 0x13, 0xD4, 0x6E, 0x60, 0x20 } }

extern EFI_GUID gApfsEncryptedPartitionProtocolGuid;

#endif // APFS_ENCRYPTED_PARTITION_H
