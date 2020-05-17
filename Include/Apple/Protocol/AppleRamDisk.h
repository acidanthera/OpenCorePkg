/** @file
  Apple RAM Disk protocol.

Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_RAM_DISK_PROTOCOL_H
#define APPLE_RAM_DISK_PROTOCOL_H

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

/**
  Apple RAM Disk protocol GUID.
  957932CC-7E8E-433B-8F41-D391EA3C10F8
**/
#define APPLE_RAM_DISK_PROTOCOL_GUID \
  { 0x957932CC, 0x7E8E, 0x433B,      \
    { 0x8F, 0x41, 0xD3, 0x91, 0xEA, 0x3C, 0x10, 0xF8 } }

/**
  Apple RAM Disk protocol revision.
**/
#define APPLE_DMG_BOOT_PROTOCOL_REVISION 3

/**
  RAM Disk extent signature, "RAMDXTNT".
**/
#define APPLE_RAM_DISK_EXTENT_SIGNATURE  0x544E5458444D4152ULL

/**
  RAM Disk extent version.
**/
#define APPLE_RAM_DISK_EXTENT_VERSION    0x10000U

/**
  RAM Disk maximum extent count.
**/
#define APPLE_RAM_DISK_MAX_EXTENTS       0xFE

#pragma pack(push, 1)

/**
  RAM Disk extent.
  When automatically allocating at RAM disk creation, extents are created
  on demand as long as contiguous memory is found in the firmware.

  Please note, that this must match IOAddressRange type in XNU,
  which can be found in iokit/IOKit/IOTypes.h.
**/
typedef PACKED struct {
  ///
  /// Extentent address pointing to a sequence of allocated pages from
  /// EfiACPIMemoryNVS or EfiBootServicesData depending on RamDisk init.
  ///
  UINT64                Start;
  ///
  /// Actual size of the extent. Allocated area size may be >= Length.
  ///
  UINT64                Length;
} APPLE_RAM_DISK_EXTENT;

/**
  RAM Disk externally accessible header containing references to extents.
**/
typedef PACKED struct {
  ///
  /// Set to APPLE_RAM_DISK_EXTENT_SIGNATURE.
  ///
  UINT64                Signature;
  ///
  /// Set to APPLE_RAM_DISK_EXTENT_VERSION.
  ///
  UINT32                Version;
  ///
  /// Externally visible amount of extents.
  ///
  UINT32                ExtentCount;
  ///
  /// Externally visible extent array.
  ///
  APPLE_RAM_DISK_EXTENT Extents[APPLE_RAM_DISK_MAX_EXTENTS];
  ///
  /// Currently reserved or rather unknown.
  ///
  UINT64                Reserved;
  ///
  /// Set to APPLE_RAM_DISK_EXTENT_SIGNATURE.
  ///
  UINT64                Signature2;
} APPLE_RAM_DISK_EXTENT_TABLE;

/**
  RAM Disk protocol context. This might be called scatter pool.
**/
typedef PACKED struct {
  ///
  /// Internally visible amount of extents.
  ///
  UINT64                       ExtentCount;
  ///
  /// Currently reserved or rather unknown. Padding?
  ///
  UINT64                       Reserved1;
  ///
  /// Total addressible disk size specified at creation.
  ///
  UINT64                       DiskSize;
  ///
  /// Internally visible extent array.
  ///
  APPLE_RAM_DISK_EXTENT        Extents[APPLE_RAM_DISK_MAX_EXTENTS];
  ///
  /// Currently reserved or rather unknown. Padding?
  ///
  UINT64                       Reserved2;
  ///
  /// Normal extent table.
  ///
  APPLE_RAM_DISK_EXTENT_TABLE  ExtentTable;
  ///
  /// More data may follow, for Apple implementation it will be 0x9FD000 bytes.
  /// (This struct and 1 page for internal header.)
  ///
} APPLE_RAM_DISK_CONTEXT;

/**
  RAM Disk vendor device path, 24 bytes in total.
**/
typedef PACKED struct {
  ///
  /// Vendor device path.
  /// Type    = HARDWARE_DEVICE_PATH.
  /// Subtype = HW_VENDOR_DP.
  /// Length  = sizeof (VENDOR_DEVICE_PATH) + sizeof (UINT32).
  /// Guid    = APPLE_RAM_DISK_PROTOCOL_GUID.
  ///
  VENDOR_DEVICE_PATH        Vendor;
  ///
  /// Globally incremented counter to make every path unique.
  ///
  UINT32                    Counter;
} APPLE_RAM_DISK_DP_VENDOR;

/**
  RAM Disk device path header for endpoint devices.
**/
typedef PACKED struct {
  ///
  /// Vendor device path.
  ///
  APPLE_RAM_DISK_DP_VENDOR  Vendor;
  ///
  /// Memmap device path.
  ///
  MEMMAP_DEVICE_PATH        MemMap;
} APPLE_RAM_DISK_DP_HEADER;

/**
  RAM Disk device path, 52 bytes in total.
**/
typedef PACKED struct {
  ///
  /// Vendor device path with APPLE_RAM_DISK_PROTOCOL_GUID.
  /// Type             = HARDWARE_DEVICE_PATH.
  /// Subtype          = HW_MEMMAP_DP.
  /// Length           = sizeof (APPLE_RAM_DISK_ENTRY_DP).
  /// StartingAddresss = APPLE_RAM_DISK_EXTENT_TABLE pointer.
  /// EndingAddress    = StartingAddresss + sizeof (APPLE_RAM_DISK_EXTENT_TABLE).
  /// MemoryType       = EfiACPIMemoryNVS or EfiBootServicesData as allocated.
  ///
  /// Note: EndingAddress, per UEFI specification and edk2 implementation, is supposed
  /// to be the top usable address, not the top of the buffer. Perhaps there is a mistake here.
  ///
  APPLE_RAM_DISK_DP_VENDOR  Vendor;
  ///
  /// Memmap device path.
  ///
  MEMMAP_DEVICE_PATH        MemMap;
  ///
  /// Device path end.
  ///
  EFI_DEVICE_PATH_PROTOCOL  End;
} APPLE_RAM_DISK_DP;

#pragma pack(pop)

/**
  Create new RAM disk providing relevant protocols:
  - gEfiDevicePathProtocolGuid
  - gEfiBlockIoProtocolGuid
  - gAppleRamDiskProtocolGuid (as NULL)

  When AllocateMemory is TRUE, a writeable RAM disk is created.
  Its contents are initially zeroed, and then may be modified through normal writing
  and via RAM Disk context, which is accessible through GetRamDiskContext.

  When AllocateMemory is FALSE, a read only RAM disk is created.
  Its contents are not allocated but reported to be all zero.
  MemMap StartingAddresss will be 0 and EndingAddress will be sizeof (APPLE_RAM_DISK_EXTENT_TABLE).
  Vit: Since no context is allocated, there is no way to initialise this disk image except in an
  implementatation specific way. AllocateMemory = FALSE is never used, so it may be a bug.

  @param[in]  BlockCount         Block amount, ignored if AllocateMemory is FALSE.
  @param[in]  BlockSize          Block size, ignored if AllocateMemory is FALSE.
  @param[in]  AllocateMemory     Allocate disk memory.
  @param[in]  ReserveMemory      Mark memory as EfiACPIMemoryNVS, otherwise EfiBootServicesData.
  @param[in]  Handle             Resulting handle.

  @retval EFI_SUCCESS            RAM disk was successfully created.
  @retval EFI_OUT_OF_RESOURCES   Requested RAM disk of more than SIZE_16TB.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation error happened.
  @retval EFI_INVALID_PARAMETER  Too many extents are needed for the allocation.
  @retval EFI_BUFFER_TOO_SMALL   Not enough extents to allocate context extent.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_RAM_DISK_CREATE) (
  IN  UINT64       BlockCount     OPTIONAL,
  IN  UINT32       BlockSize      OPTIONAL,
  IN  BOOLEAN      AllocateMemory,
  IN  BOOLEAN      ReserveMemory,
  OUT EFI_HANDLE   *Handle
  );

/**
  Destroy RAM disk freeing resources and uninstalling relevant protocols:
  - gEfiDevicePathProtocolGuid
  - gEfiBlockIoProtocolGuid
  - gAppleRamDiskProtocolGuid
  - gTDMApprovedGuid

  @param[in]  Handle             RAM disk handle.

  @retval EFI_INVALID_PARAMETER  Not a RAM disk handle.
  @retval EFI_NOT_FOUND          Missing Block I/O protocol.
  @retval EFI_SUCCESS            Destroyed successfully.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_RAM_DISK_DESTROY) (
  IN  EFI_HANDLE   Handle
  );

/**
  Obtain RAM disk context. Context exists only for "allocated" RAM disks,
  i.e. disks created with AllocateMemory = TRUE. When an unallocated
  RAM disk is created, NULL Context will be returned.

  @param[in]  Handle             RAM disk handle.
  @param[out] Context            RAM disk context.

  @retval EFI_INVALID_PARAMETER  Not a RAM disk handle.
  @retval EFI_NOT_FOUND          Missing Block I/O protocol.
  @retval EFI_SUCCESS            Obtained NULL or non-NULL context successfully.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_RAM_DISK_GET_CONTEXT) (
  IN  EFI_HANDLE              Handle,
  OUT APPLE_RAM_DISK_CONTEXT  *Context
  );

/**
  Apple RAM Disk protocol.
**/
typedef struct {
  UINT64                      Revision;
  APPLE_RAM_DISK_CREATE       CreateRamDisk;
  APPLE_RAM_DISK_DESTROY      DestroyRamDisk;
  APPLE_RAM_DISK_GET_CONTEXT  GetRamDiskContext;
} APPLE_RAM_DISK_PROTOCOL;

STATIC_ASSERT (sizeof (APPLE_RAM_DISK_DP_VENDOR) == 24, "Invalid APPLE_RAM_DISK_DP_VENDOR size");
STATIC_ASSERT (sizeof (APPLE_RAM_DISK_DP) == 52, "Invalid APPLE_RAM_DISK_DP size");
STATIC_ASSERT (sizeof (APPLE_RAM_DISK_EXTENT) == 16, "Invalid APPLE_RAM_DISK_EXTENT size");
STATIC_ASSERT (sizeof (APPLE_RAM_DISK_EXTENT_TABLE) == 4096, "Invalid APPLE_RAM_DISK_EXTENT_TABLE size");
STATIC_ASSERT (sizeof (APPLE_RAM_DISK_CONTEXT) == 8192, "Invalid APPLE_RAM_DISK_CONTEXT size");

extern EFI_GUID gAppleRamDiskProtocolGuid;

/**
  Vit: Below come the implementation specific details, which probably do not belong here.
  Since this is the only way to initialize RAM disk when it was not allocated at creation time,
  I put it here for now.
**/

/**
  RAM Disk instance signature, "RAMD".
**/
#define APPLE_RAM_DISK_INSTANCE_SIGNATURE  0x444D4152U

/**
  RAM Disk default size.
  This value is used when allocating APPLE_RAM_DISK structure.
**/
#define APPLE_RAM_DISK_DEFAULT_SIZE        0xA00000U

/**
  RAM Disk outermost structure. Allocated in pages with matching MemoryType.
**/
typedef struct {
  ///
  /// Amount of allocated extents in the RAM disk.
  ///
  UINT64                 AllocatedExtents;
  ///
  /// Amount of allocated pages in APPLE_RAM_DISK structure.
  /// Normally EFI_SIZE_TO_PAGES (APPLE_RAM_DISK_DEFAULT_SIZE).
  ///
  UINT64                 PageCount;
  ///
  /// Total disk size allocated from memory map.
  /// It is bigger by sizeof(APPLE_RAM_DISK_CONTEXT) than actual requested size,
  /// when AllocateMemory = TRUE is specified.
  ///
  UINT64                 TotalSize;
  ///
  /// All allocated and owned extents.
  /// First extent is special, as it points to OwnExtentData, and thus
  /// has APPLE_RAM_DISK_DEFAULT_SIZE - sizeof (APPLE_RAM_DISK) bytes.
  ///
  APPLE_RAM_DISK_EXTENT  Extents[APPLE_RAM_DISK_MAX_EXTENTS];
  ///
  /// Currently reserved or rather unknown. Padding?
  ///
  UINT64                 Reserved;
  ///
  /// Own extent data lasting till PageCount end.
  ///
  UINT8                  OwnExtentData[];
} APPLE_RAM_DISK;

/**
  Apple RAM Disk instance structure.
**/
typedef struct {
  ///
  /// Set to APPLE_RAM_DISK_INSTANCE_SIGNATURE.
  ///
  UINT32                       Magic;
  ///
  /// Set to RAM disk handle, formerly returned with CreateRamDisk.
  ///
  EFI_HANDLE                   Handle;
  ///
  /// RAM disk device path.
  ///
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  ///
  /// RAM disk Block I/O protocol.
  ///
  EFI_BLOCK_IO_PROTOCOL        BlockIo;
  ///
  /// Outer RAM disk structure, which owns all allocated extents.
  /// This field is NULL when AllocateMemory = FALSE.
  ///
  APPLE_RAM_DISK               *RamDisk;
  ///
  /// RAM disk context allocated within RAM disk structure memory.
  /// Apple implementation puts it to the end of RamDisk structure.
  /// This field is NULL when AllocateMemory = FALSE.
  ///
  APPLE_RAM_DISK_CONTEXT       *Context;
  ///
  /// Extra pointer to Context->Extents.
  /// This field is NULL when AllocateMemory = FALSE.
  ///
  APPLE_RAM_DISK_EXTENT_TABLE  *Extents;
  ///
  /// Total addressible disk size specified at creation.
  /// This field is -1 when AllocateMemory = FALSE.
  ///
  INT64                        DiskSize;
} APPLE_RAM_DISK_INSTANCE;

#endif // APPLE_RAM_DISK_PROTOCOL_H
