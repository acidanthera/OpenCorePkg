/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_DISK_IMAGE_LIB_INTERNAL_H_
#define APPLE_DISK_IMAGE_LIB_INTERNAL_H_

// Common libraries.
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcXmlLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// Consumed protocols.
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

// Sizes.
#define BASE_256B   0x100
#define SIZE_512B   0x200

// Used for determining absolute sector start within chunk.
#define DMG_SECTOR_START_ABS(b, c)  ((b->SectorNumber) + (c->SectorNumber))

#define DMG_PLIST_RESOURCE_FORK_KEY     "resource-fork"
#define DMG_PLIST_BLOCK_LIST_KEY        "blkx"
#define DMG_PLIST_ATTRIBUTES            "Attributes"
#define DMG_PLIST_CFNAME                "CFName"
#define DMG_PLIST_DATA                  "Data"
#define DMG_PLIST_ID                    "ID"
#define DMG_PLIST_NAME                  "Name"

//
// Device path stuff.
//
#pragma pack(1)

// DMG controller device path.
#define DMG_CONTROLLER_DP_GUID { \
    0x957932CC, 0x7E8E, 0x433B, { 0x8F, 0x41, 0xD3, 0x91, 0xEA, 0x3C, 0x10, 0xF8 } \
}
extern EFI_GUID gDmgControllerDpGuid;

// DMG vendor device path node.
typedef struct {
    EFI_DEVICE_PATH_PROTOCOL Header;
    EFI_GUID Guid;
    UINT32 Key;
} DMG_CONTROLLER_DEVICE_PATH;

// DMG size device path.
#define DMG_SIZE_DP_GUID { \
    0x004B07E8, 0x0B9C, 0x427E, { 0xB0, 0xD4, 0xA4, 0x66, 0xE6, 0xE5, 0x7A, 0x62 } \
}
extern EFI_GUID gDmgSizeDpGuid;

// DMG size device path node.
typedef struct {
    EFI_DEVICE_PATH_PROTOCOL Header;
    EFI_GUID Guid;
    UINT64 Length;
} DMG_SIZE_DEVICE_PATH;
#pragma pack()

//
// RAM DMG structures used by boot.efi.
//
#pragma pack(1)

#define RAM_DMG_SIGNATURE   0x544E5458444D4152ULL // "RAMDXTNT"
#define RAM_DMG_VERSION     0x10000

// RAM DMG extent info.
typedef struct {
    UINT64 Start;
    UINT64 Length;
} RAM_DMG_EXTENT_INFO;

// RAM DMG header.
typedef struct {
    UINT64 Signature;
    UINT32 Version;
    UINT32 ExtentCount;
    RAM_DMG_EXTENT_INFO ExtentInfo[0xFE];
    UINT64 Reserved;
    UINT64 Signature2;
} RAM_DMG_HEADER;
#pragma pack()

//
// Mounted disk image private data.
//
#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE SIGNATURE_32('D','m','g','I')
typedef struct {
    // Signature.
    UINTN Signature;

    // Protocols.
    EFI_BLOCK_IO_PROTOCOL BlockIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_HANDLE Handle;

    // Disk image data.
    OC_APPLE_DISK_IMAGE_CONTEXT *ImageContext;
    RAM_DMG_HEADER *RamDmgHeader;
} OC_APPLE_DISK_IMAGE_MOUNTED_DATA;
#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS(This) \
    CR(This, OC_APPLE_DISK_IMAGE_MOUNTED_DATA, BlockIo, OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE)

EFI_STATUS
EFIAPI
FindPlistDictChild(
    IN  XML_NODE *Node,
    IN  CHAR8 *KeyName,
    OUT XML_NODE **Key,
    OUT XML_NODE **Value);

EFI_STATUS
EFIAPI
ParsePlist(
    IN  VOID *Buffer,
    IN  UINT64 XmlOffset,
    IN  UINT64 XmlLength,
    OUT UINT32 *BlockCount,
    OUT OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT **Blocks);

EFI_STATUS
EFIAPI
VerifyCrc32(
    IN VOID *Buffer,
    IN UINTN Length,
    IN UINT32 Checksum);

EFI_STATUS
EFIAPI
GetBlockChunk(
    IN  OC_APPLE_DISK_IMAGE_CONTEXT *Context,
    IN  EFI_LBA Lba,
    OUT APPLE_DISK_IMAGE_BLOCK_DATA **Data,
    OUT APPLE_DISK_IMAGE_CHUNK **Chunk);

//
// Block I/O protocol template.
//
extern EFI_BLOCK_IO_PROTOCOL mDiskImageBlockIo;

//
// Block I/O protocol functions.
//
EFI_STATUS
EFIAPI
DiskImageBlockIoReset(
    IN EFI_BLOCK_IO_PROTOCOL *This,
    IN BOOLEAN ExtendedVerification);

EFI_STATUS
EFIAPI
DiskImageBlockIoReadBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This,
    IN UINT32 MediaId,
    IN EFI_LBA Lba,
    IN UINTN BufferSize,
    OUT VOID *Buffer);

EFI_STATUS
EFIAPI
DiskImageBlockIoWriteBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This,
    IN UINT32 MediaId,
    IN EFI_LBA Lba,
    IN UINTN BufferSize,
    IN VOID *Buffer);

EFI_STATUS
EFIAPI
DiskImageBlockIoFlushBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This);

#endif
