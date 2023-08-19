/** @file
  Copyright (C) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LegacyBootInternal.h"

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

typedef struct {
  UINT8     PacketSizeInBytes; // 0x10
  UINT8     Zero;
  UINT8     NumberOfBlocks;   // Max 0x7f
  UINT8     Zero2;
  UINT32    SegOffset;
  UINT64    Lba;
} DEVICE_ADDRESS_PACKET;

STATIC
EFI_STATUS
BiosDiskReset (
  IN  THUNK_CONTEXT             *ThunkContext,
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  IN  UINT8                     DriveNumber
  )
{
  IA32_REGISTER_SET  Regs;
  UINTN              CarryFlag;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  Regs.H.AH = 0x00;
  Regs.H.DL = DriveNumber;
  CarryFlag = OcLegacyThunkBiosInt86 (ThunkContext, Legacy8259, 0x13, &Regs);

  if (CarryFlag != 0) {
    DEBUG ((DEBUG_INFO, "OLB: Failed to reset BIOS disk %u, error 0x%X\n", DriveNumber, Regs.H.AL));
    if (Regs.H.AL == BIOS_RESET_FAILED) {
      Regs.H.AH = 0x00;
      Regs.H.DL = DriveNumber;
      CarryFlag = OcLegacyThunkBiosInt86 (ThunkContext, Legacy8259, 0x13, &Regs);
      if (CarryFlag != 0) {
        DEBUG ((DEBUG_INFO, "OLB: Failed to reset BIOS disk %u, error 0x%X\n", DriveNumber, Regs.H.AH));
        return EFI_DEVICE_ERROR;
      }
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BiosDiskExtensionsSupported (
  IN  THUNK_CONTEXT             *ThunkContext,
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  IN  UINT8                     DriveNumber
  )
{
  INTN               CarryFlag;
  IA32_REGISTER_SET  Regs;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  Regs.H.AH = 0x41;
  Regs.X.BX = 0x55aa;
  Regs.H.DL = DriveNumber;
  CarryFlag = OcLegacyThunkBiosInt86 (ThunkContext, Legacy8259, 0x13, &Regs);

  if ((CarryFlag != 0) || (Regs.X.BX != 0xaa55)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BiosDiskReadExtSectors (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN      EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  IN      DEVICE_ADDRESS_PACKET     *DeviceAddressPacket,
  IN      UINT8                     DriveNumber,
  IN      UINT64                    Lba,
  IN      UINT8                     NumSectors,
  IN OUT  UINT8                     *Buffer
  )
{
  INTN               CarryFlag;
  IA32_REGISTER_SET  Regs;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  DeviceAddressPacket->PacketSizeInBytes = sizeof (*DeviceAddressPacket);
  DeviceAddressPacket->Zero              = 0;
  DeviceAddressPacket->NumberOfBlocks    = NumSectors;
  DeviceAddressPacket->Zero2             = 0;
  DeviceAddressPacket->SegOffset         = (UINT32)LShiftU64 (RShiftU64 ((UINTN)Buffer, 4), 16);
  DeviceAddressPacket->Lba               = Lba;

  Regs.H.AH = 0x42;
  Regs.H.DL = DriveNumber;
  Regs.X.SI = EFI_OFFSET (DeviceAddressPacket);
  Regs.E.DS = EFI_SEGMENT (DeviceAddressPacket);
  CarryFlag = OcLegacyThunkBiosInt86 (ThunkContext, Legacy8259, 0x13, &Regs);

  DEBUG ((DEBUG_INFO, "carry %u\n", CarryFlag));

  return EFI_SUCCESS;
}

EFI_STATUS
InternalGetBiosDiskNumber (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN      EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  OUT  UINT8                        *DriveNumber
  )
{
  EFI_STATUS             Status;
  UINT8                  DriveAddr;
  EFI_PHYSICAL_ADDRESS   DeviceAddressPacketAddress;
  DEVICE_ADDRESS_PACKET  *DeviceAddressPacket;
  UINT8                  *Buffer;

  //
  // Allocate low memory buffer for disk reads.
  //
  DeviceAddressPacketAddress = (SIZE_1MB - 1);
  Status                     = gBS->AllocatePages (
                                      AllocateMaxAddress,
                                      EfiBootServicesData,
                                      EFI_SIZE_TO_PAGES (sizeof (*DeviceAddressPacket) + (MBR_SIZE * 4)) + 1,
                                      &DeviceAddressPacketAddress
                                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OLB: Failure allocating low memory packet for BIOS disk read - %r\n", Status));
    return Status;
  }

  DeviceAddressPacket = (DEVICE_ADDRESS_PACKET *)(UINTN)DeviceAddressPacketAddress;
  Buffer              = (UINT8 *)(UINTN)DeviceAddressPacketAddress + 0x200;

  //
  // Locate disk with matching checksum of first few sectors.
  //
  for (DriveAddr = 0x80; DriveAddr < 0x88; DriveAddr++) {
    DEBUG ((DEBUG_INFO, "drive %X\n", DriveAddr));

    //
    // Reset disk and verify INT13H extensions are supported.
    //
    Status = BiosDiskReset (ThunkContext, Legacy8259, DriveAddr);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = BiosDiskExtensionsSupported (ThunkContext, Legacy8259, DriveAddr);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Read sectors from disk.
    //
    Status = BiosDiskReadExtSectors (
               ThunkContext,
               Legacy8259,
               DeviceAddressPacket,
               DriveAddr,
               0,
               4,
               Buffer
               );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DebugPrintHexDump (DEBUG_INFO, "raw bios bytes", Buffer, 512);
  }

  return EFI_SUCCESS;
}
