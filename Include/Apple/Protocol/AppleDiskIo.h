/** @file
  Apple Disk IO protocol.

  Copyright (c) 2012, tiamo. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_DISK_IO_H
#define APPLE_DISK_IO_H

/**
  Apple Disk IO protocol GUID.
  5B27263B-9083-415E-889E-6432CAA9B813
**/
#define APPLE_DISK_IO_PROTOCOL_GUID \
 { 0x5B27263B, 0x9083, 0x415E,    \
    { 0x88, 0x9E, 0x64, 0x32, 0xCA, 0xA9, 0xB8, 0x13 }}

typedef struct APPLE_DISK_IO_PROTOCOL_ APPLE_DISK_IO_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_DISK_IO_READ)(
  IN  APPLE_DISK_IO_PROTOCOL *This,
  IN  UINT32                 MediaId,
  IN  UINT64                 LBA,
  IN  UINTN                  BufferSize,
  OUT VOID                  *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_DISK_IO_FLUSH)(
  IN APPLE_DISK_IO_PROTOCOL *This
  );

/**
  Apple Disk IO protocol instance.
**/
struct APPLE_DISK_IO_PROTOCOL_ {
  APPLE_DISK_IO_READ     ReadDisk;
  APPLE_DISK_IO_FLUSH    Flush;
};

extern EFI_GUID  gAppleDiskIoProtocolGuid;

#endif // APPLE_DISK_IO_H
