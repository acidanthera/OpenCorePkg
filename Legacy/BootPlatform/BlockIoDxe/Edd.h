/** @file
  Include file to suport EDD 3.0.
  This file is coded to T13 D1386 Revision 3
  Availible on http://www.t13.org/#Project drafts
  Currently at ftp://fission.dt.wdc.com/pub/standards/x3t13/project/d1386r3.pdf

Copyright (c) 1999 - 2010, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDD_H_
#define _EDD_H_

//
// packing with no compiler padding, so that the fields
// of the following architected structures can be
// properly accessed from C code.
//
#pragma pack(1)

typedef struct {
  UINT8     Bus;
  UINT8     Device;
  UINT8     Function;
  UINT8     Controller;
  UINT32    Reserved;
} EDD_PCI;

typedef struct {
  UINT16    Base;
  UINT16    Reserved;
  UINT32    Reserved2;
} EDD_LEGACY;

typedef union {
  EDD_PCI       Pci;
  EDD_LEGACY    Legacy;
} EDD_INTERFACE_PATH;

typedef struct {
  UINT8    Master;
  UINT8    Reserved[15];
} EDD_ATA;

typedef struct {
  UINT8    Master;
  UINT8    Lun;
  UINT8    Reserved[14];
} EDD_ATAPI;

typedef struct {
  UINT16    Pun;
  UINT64    Lun;
  UINT8     Reserved[6];
} EDD_SCSI;

typedef struct {
  UINT64    SerialNumber;
  UINT64    Reserved;
} EDD_USB;

typedef struct {
  UINT64    Guid;
  UINT64    Reserved;
} EDD_1394;

typedef struct {
  UINT64    Wwn;
  UINT64    Lun;
} EDD_FIBRE;

typedef union {
  EDD_ATA      Ata;
  EDD_ATAPI    Atapi;
  EDD_SCSI     Scsi;
  EDD_USB      Usb;
  EDD_1394     FireWire;
  EDD_FIBRE    FibreChannel;
} EDD_DEVICE_PATH;

typedef struct {
  UINT16                StructureSize;
  UINT16                Flags;
  UINT32                MaxCylinders;
  UINT32                MaxHeads;
  UINT32                SectorsPerTrack;
  UINT64                PhysicalSectors;
  UINT16                BytesPerSector;
  UINT32                Fdpt;
  UINT16                Key;
  UINT8                 DevicePathLength;
  UINT8                 Reserved1;
  UINT16                Reserved2;
  CHAR8                 HostBusType[4];
  CHAR8                 InterfaceType[8];
  EDD_INTERFACE_PATH    InterfacePath;
  EDD_DEVICE_PATH       DevicePath;
  UINT8                 Reserved3;
  UINT8                 Checksum;
} EDD_DRIVE_PARAMETERS;

//
// EDD_DRIVE_PARAMETERS.Flags defines
//
#define EDD_GEOMETRY_VALID          0x02
#define EDD_DEVICE_REMOVABLE        0x04
#define EDD_WRITE_VERIFY_SUPPORTED  0x08
#define EDD_DEVICE_CHANGE           0x10
#define EDD_DEVICE_LOCKABLE         0x20

//
// BUGBUG: This bit does not follow the spec. It tends to be always set
//  to work properly with Win98.
//
#define EDD_DEVICE_GEOMETRY_MAX  0x40

typedef struct {
  UINT8     PacketSizeInBytes; // 0x18
  UINT8     Zero;
  UINT8     NumberOfBlocks;   // Max 0x7f
  UINT8     Zero2;
  UINT32    SegOffset;
  UINT64    Lba;
  UINT64    TransferBuffer;
  UINT32    ExtendedBlockCount; // Max 0xffffffff
  UINT32    Zero3;
} EDD_DEVICE_ADDRESS_PACKET;

#define EDD_VERSION_30  0x30

//
// Int 13 BIOS Errors
//
#define BIOS_PASS                   0x00
#define BIOS_WRITE_PROTECTED        0x03
#define BIOS_SECTOR_NOT_FOUND       0x04
#define BIOS_RESET_FAILED           0x05
#define BIOS_DISK_CHANGED           0x06
#define BIOS_DRIVE_DOES_NOT_EXIST   0x07
#define BIOS_DMA_ERROR              0x08
#define BIOS_DATA_BOUNDRY_ERROR     0x09
#define BIOS_BAD_SECTOR             0x0a
#define BIOS_BAD_TRACK              0x0b
#define BIOS_MEADIA_TYPE_NOT_FOUND  0x0c
#define BIOS_INVALED_FORMAT         0x0d
#define BIOS_ECC_ERROR              0x10
#define BIOS_ECC_CORRECTED_ERROR    0x11
#define BIOS_HARD_DRIVE_FAILURE     0x20
#define BIOS_SEEK_FAILED            0x40
#define BIOS_DRIVE_TIMEOUT          0x80
#define BIOS_DRIVE_NOT_READY        0xaa
#define BIOS_UNDEFINED_ERROR        0xbb
#define BIOS_WRITE_FAULT            0xcc
#define BIOS_SENSE_FAILED           0xff

#define MAX_EDD11_XFER  0xfe00

#pragma pack()
//
// Internal Data Structures
//
typedef struct {
  CHAR8                   Letter;
  UINT8                   Number;
  UINT8                   EddVersion;
  BOOLEAN                 ExtendedInt13;
  BOOLEAN                 DriveLockingAndEjecting;
  BOOLEAN                 Edd;
  BOOLEAN                 Extensions64Bit;
  BOOLEAN                 ParametersValid;
  UINT8                   ErrorCode;
  VOID                    *FdptPointer;
  BOOLEAN                 Floppy;
  BOOLEAN                 AtapiFloppy;
  UINT8                   MaxHead;
  UINT8                   MaxSector;
  UINT16                  MaxCylinder;
  UINT16                  Pad;
  EDD_DRIVE_PARAMETERS    Parameters;
} BIOS_LEGACY_DRIVE;

#define BIOS_CONSOLE_BLOCK_IO_DEV_SIGNATURE  SIGNATURE_32 ('b', 'b', 'i', 'o')
typedef struct {
  UINTN                       Signature;

  EFI_HANDLE                  Handle;
  EFI_HANDLE                  ControllerHandle;
  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          BlockMedia;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  EFI_LEGACY_8259_PROTOCOL    *Legacy8259;
  THUNK_CONTEXT               *ThunkContext;

  BIOS_LEGACY_DRIVE           Bios;
} BIOS_BLOCK_IO_DEV;

#define BIOS_BLOCK_IO_FROM_THIS(a)  CR (a, BIOS_BLOCK_IO_DEV, BlockIo, BIOS_CONSOLE_BLOCK_IO_DEV_SIGNATURE)

#endif
