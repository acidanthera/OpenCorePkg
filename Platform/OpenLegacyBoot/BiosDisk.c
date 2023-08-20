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

#define BIOS_DISK_CHECK_BUFFER_SECTOR_COUNT  4

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

  if (CarryFlag != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalGetBiosDiskAddress (
  IN  THUNK_CONTEXT             *ThunkContext,
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  IN  EFI_HANDLE                DiskHandle,
  OUT UINT8                     *DriveAddress
  )
{
  EFI_STATUS             Status;
  OC_DISK_CONTEXT        DiskContext;
  UINT8                  DriveAddr;
  EFI_PHYSICAL_ADDRESS   DeviceAddressPacketAddress;
  DEVICE_ADDRESS_PACKET  *DeviceAddressPacket;
  UINT8                  *BiosBuffer;
  UINTN                  BiosBufferPages;
  UINT32                 BiosCrc32;
  UINT8                  *DiskBuffer;
  UINTN                  DiskBufferSize;
  UINT32                 DiskCrc32;
  BOOLEAN                MatchedDisk;

  //
  // Read sectors from EFI disk device.
  //
  Status = OcDiskInitializeContext (&DiskContext, DiskHandle, TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DiskBufferSize = ALIGN_VALUE (BIOS_DISK_CHECK_BUFFER_SECTOR_COUNT * MBR_SIZE, DiskContext.BlockSize);
  DiskBuffer     = AllocatePool (DiskBufferSize);
  if (DiskBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = OcDiskRead (
             &DiskContext,
             0,
             DiskBufferSize,
             DiskBuffer
             );
  if (EFI_ERROR (Status)) {
    FreePool (DiskBuffer);
    return Status;
  }

  gBS->CalculateCrc32 (
         DiskBuffer,
         BIOS_DISK_CHECK_BUFFER_SECTOR_COUNT * MBR_SIZE,
         &DiskCrc32
         );
  DEBUG ((DEBUG_INFO, "OLB: EFI disk CRC32: 0x%X\n", DiskCrc32));

  //
  // Allocate low memory buffer for disk reads.
  //
  DeviceAddressPacketAddress = (SIZE_1MB - 1);
  BiosBufferPages            = EFI_SIZE_TO_PAGES (sizeof (*DeviceAddressPacket) + (BIOS_DISK_CHECK_BUFFER_SECTOR_COUNT * MBR_SIZE)) + 1;
  Status                     = gBS->AllocatePages (
                                      AllocateMaxAddress,
                                      EfiBootServicesData,
                                      BiosBufferPages,
                                      &DeviceAddressPacketAddress
                                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OLB: Failure allocating low memory packet for BIOS disk read - %r\n", Status));
    FreePool (DiskBuffer);
    return Status;
  }

  DeviceAddressPacket = (DEVICE_ADDRESS_PACKET *)(UINTN)DeviceAddressPacketAddress;
  BiosBuffer          = (UINT8 *)(UINTN)DeviceAddressPacketAddress + 0x200;

  //
  // Read sectors from each BIOS disk.
  // Compare against sectors from EFI disk to determine BIOS disk address.
  //
  MatchedDisk = FALSE;
  for (DriveAddr = 0x80; DriveAddr < 0x88; DriveAddr++) {
    DEBUG ((DEBUG_INFO, "OLB: Reading BIOS drive 0x%X\n", DriveAddr));

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
    // Read first 4 sectors from disk.
    //
    Status = BiosDiskReadExtSectors (
               ThunkContext,
               Legacy8259,
               DeviceAddressPacket,
               DriveAddr,
               0,
               BIOS_DISK_CHECK_BUFFER_SECTOR_COUNT,
               BiosBuffer
               );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Calculate CRC32 of BIOS disk sectors.
    //
    gBS->CalculateCrc32 (
           BiosBuffer,
           BIOS_DISK_CHECK_BUFFER_SECTOR_COUNT * MBR_SIZE,
           &BiosCrc32
           );
    DEBUG ((DEBUG_INFO, "OLB: BIOS disk CRC32: 0x%X\n", BiosCrc32));

    if (BiosCrc32 == DiskCrc32) {
      DEBUG ((DEBUG_INFO, "OLB: Matched BIOS disk address 0x%X\n", DriveAddr));

      MatchedDisk   = TRUE;
      *DriveAddress = DriveAddr;
      break;
    }
  }

  FreePool (DiskBuffer);
  gBS->FreePages (
         DeviceAddressPacketAddress,
         BiosBufferPages
         );

  return MatchedDisk ? EFI_SUCCESS : EFI_NOT_FOUND;
}
