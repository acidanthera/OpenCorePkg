/** @file
  Routines that use BIOS to support INT 13 devices.

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BiosBlkIo.h"

//
// Module global variables
//
//
// Address packet is a buffer under 1 MB for all version EDD calls
//
extern EDD_DEVICE_ADDRESS_PACKET  *mEddBufferUnder1Mb;

//
// This is a buffer for INT 13h func 48 information
//
extern BIOS_LEGACY_DRIVE  *mLegacyDriverUnder1Mb;

//
// Buffer of 0xFE00 bytes for EDD 1.1 transfer must be under 1 MB
//  0xFE00 bytes is the max transfer size supported.
//
extern VOID  *mEdd11Buffer;

/**
  Initialize block I/O device instance

  @param  Dev   Instance of block I/O device instance

  @retval TRUE  Initialization succeeds.
  @retval FALSE Initialization fails.

**/
BOOLEAN
BiosInitBlockIo (
  IN  BIOS_BLOCK_IO_DEV  *Dev
  )
{
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_BLOCK_IO_MEDIA     *BlockMedia;
  BIOS_LEGACY_DRIVE      *Bios;

  BlockIo        = &Dev->BlockIo;
  BlockIo->Media = &Dev->BlockMedia;
  BlockMedia     = BlockIo->Media;
  Bios           = &Dev->Bios;

  if (Int13GetDeviceParameters (Dev, Bios) != 0) {
    if (Int13Extensions (Dev, Bios) != 0) {
      BlockMedia->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
      BlockMedia->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;

      if ((Bios->Parameters.Flags & EDD_DEVICE_REMOVABLE) == EDD_DEVICE_REMOVABLE) {
        BlockMedia->RemovableMedia = TRUE;
      }
    } else {
      //
      // Legacy Interfaces
      //
      BlockMedia->BlockSize = 512;
      BlockMedia->LastBlock = (Bios->MaxHead + 1) * Bios->MaxSector * (Bios->MaxCylinder + 1) - 1;
    }

    DEBUG ((DEBUG_INIT, "BlockSize = %d  LastBlock = %d\n", BlockMedia->BlockSize, BlockMedia->LastBlock));

    BlockMedia->LogicalPartition = FALSE;
    BlockMedia->WriteCaching     = FALSE;

    //
    // BugBug: Need to set this for removable media devices if they do not
    //  have media present
    //
    BlockMedia->ReadOnly     = FALSE;
    BlockMedia->MediaPresent = TRUE;

    BlockIo->Reset       = BiosBlockIoReset;
    BlockIo->FlushBlocks = BiosBlockIoFlushBlocks;

    if (!Bios->ExtendedInt13) {
      //
      // Legacy interfaces
      //
      BlockIo->ReadBlocks  = BiosReadLegacyDrive;
      BlockIo->WriteBlocks = BiosWriteLegacyDrive;
    } else if ((Bios->EddVersion == EDD_VERSION_30) && (Bios->Extensions64Bit)) {
      //
      // EDD 3.0 Required for Device path, but extended reads are not required.
      //
      BlockIo->ReadBlocks  = Edd30BiosReadBlocks;
      BlockIo->WriteBlocks = Edd30BiosWriteBlocks;
    } else {
      //
      // Assume EDD 1.1 - Read and Write functions.
      //  This could be EDD 3.0 without Extensions64Bit being set.
      // If it's EDD 1.1 this will work, but the device path will not
      //  be correct. This will cause confusion to EFI OS installation.
      //
      BlockIo->ReadBlocks  = Edd11BiosReadBlocks;
      BlockIo->WriteBlocks = Edd11BiosWriteBlocks;
    }

    BlockMedia->LogicalPartition = FALSE;
    BlockMedia->WriteCaching     = FALSE;

    return TRUE;
  }

  return FALSE;
}

/**
  Gets parameters of block I/O device.

  @param  BiosBlockIoDev Instance of block I/O device.
  @param  Drive          Legacy drive.

  @return  Result of device parameter retrieval.

**/
UINTN
Int13GetDeviceParameters (
  IN  BIOS_BLOCK_IO_DEV  *BiosBlockIoDev,
  IN  BIOS_LEGACY_DRIVE  *Drive
  )
{
  UINTN              CarryFlag;
  UINT16             Cylinder;
  IA32_REGISTER_SET  Regs;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  Regs.H.AH = 0x08;
  Regs.H.DL = Drive->Number;
  CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
  DEBUG ((DEBUG_INIT, "Int13GetDeviceParameters: INT 13 08 DL=%02x : CF=%d AH=%02x\n", Drive->Number, CarryFlag, Regs.H.AH));
  if ((CarryFlag != 0) || (Regs.H.AH != 0x00)) {
    Drive->ErrorCode = Regs.H.AH;
    return FALSE;
  }

  if (Drive->Floppy) {
    if (Regs.H.BL == 0x10) {
      Drive->AtapiFloppy = TRUE;
    } else {
      Drive->MaxHead     = Regs.H.DH;
      Drive->MaxSector   = Regs.H.CL;
      Drive->MaxCylinder = Regs.H.CH;
      if (Drive->MaxSector == 0) {
        //
        // BugBug: You can not trust the Carry flag.
        //
        return FALSE;
      }
    }
  } else {
    Drive->MaxHead     = (UINT8)(Regs.H.DH & 0x3f);
    Cylinder           = (UINT16)(((UINT16)Regs.H.DH & 0xc0) << 4);
    Cylinder           = (UINT16)(Cylinder | ((UINT16)Regs.H.CL & 0xc0) << 2);
    Drive->MaxCylinder = (UINT16)(Cylinder + Regs.H.CH);
    Drive->MaxSector   = (UINT8)(Regs.H.CL & 0x3f);
  }

  return TRUE;
}

/**
  Extension of INT13 call.

  @param  BiosBlockIoDev Instance of block I/O device.
  @param  Drive          Legacy drive.

  @return  Result of this extension.

**/
UINTN
Int13Extensions (
  IN  BIOS_BLOCK_IO_DEV  *BiosBlockIoDev,
  IN  BIOS_LEGACY_DRIVE  *Drive
  )
{
  INTN               CarryFlag;
  IA32_REGISTER_SET  Regs;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  Regs.H.AH = 0x41;
  Regs.X.BX = 0x55aa;
  Regs.H.DL = Drive->Number;
  CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
  DEBUG ((DEBUG_INIT, "Int13Extensions: INT 13 41 DL=%02x : CF=%d BX=%04x\n", Drive->Number, CarryFlag, Regs.X.BX));

  if ((CarryFlag != 0) || (Regs.X.BX != 0xaa55)) {
    Drive->ExtendedInt13           = FALSE;
    Drive->DriveLockingAndEjecting = FALSE;
    Drive->Edd                     = FALSE;
    return FALSE;
  }

  Drive->EddVersion              = Regs.H.AH;
  Drive->ExtendedInt13           = (BOOLEAN)((Regs.X.CX & 0x01) == 0x01);
  Drive->DriveLockingAndEjecting = (BOOLEAN)((Regs.X.CX & 0x02) == 0x02);
  Drive->Edd                     = (BOOLEAN)((Regs.X.CX & 0x04) == 0x04);
  Drive->Extensions64Bit         = (BOOLEAN)(Regs.X.CX & 0x08);

  Drive->ParametersValid = (UINT8)GetDriveParameters (BiosBlockIoDev, Drive);
  return TRUE;
}

/**
  Gets parameters of legacy drive.

  @param  BiosBlockIoDev Instance of block I/O device.
  @param  Drive          Legacy drive.

  @return  Result of drive parameter retrieval.

**/
UINTN
GetDriveParameters (
  IN  BIOS_BLOCK_IO_DEV  *BiosBlockIoDev,
  IN  BIOS_LEGACY_DRIVE  *Drive
  )
{
  INTN               CarryFlag;
  IA32_REGISTER_SET  Regs;
  UINTN              PointerMath;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  Regs.H.AH = 0x48;
  Regs.H.DL = Drive->Number;

  //
  // EDD Buffer must be passed in with max buffer size as first entry in the buffer
  //
  mLegacyDriverUnder1Mb->Parameters.StructureSize = (UINT16)sizeof (EDD_DRIVE_PARAMETERS);
  Regs.E.DS                                       = EFI_SEGMENT ((UINTN)(&mLegacyDriverUnder1Mb->Parameters));
  Regs.X.SI                                       = EFI_OFFSET ((UINTN)(&mLegacyDriverUnder1Mb->Parameters));
  CarryFlag                                       = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
  DEBUG ((DEBUG_INIT, "GetDriveParameters: INT 13 48 DL=%02x : CF=%d AH=%02x\n", Drive->Number, CarryFlag, Regs.H.AH));
  if ((CarryFlag != 0) || (Regs.H.AH != 0x00)) {
    Drive->ErrorCode = Regs.H.AH;
    SetMem (&Drive->Parameters, sizeof (Drive->Parameters), 0xaf);
    return FALSE;
  }

  //
  // We only have one buffer < 1MB, so copy into our instance data
  //
  CopyMem (
    &Drive->Parameters,
    &mLegacyDriverUnder1Mb->Parameters,
    sizeof (Drive->Parameters)
    );

  if (Drive->AtapiFloppy) {
    //
    // Sense Media Type
    //
    Regs.H.AH = 0x20;
    Regs.H.DL = Drive->Number;
    CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
    DEBUG ((DEBUG_INIT, "GetDriveParameters: INT 13 20 DL=%02x : CF=%d AL=%02x\n", Drive->Number, CarryFlag, Regs.H.AL));
    if (CarryFlag != 0) {
      //
      // Media not present or unknown media present
      //
      if ((Drive->Parameters.Flags & EDD_GEOMETRY_VALID) == EDD_GEOMETRY_VALID) {
        Drive->MaxHead   = (UINT8)(Drive->Parameters.MaxHeads - 1);
        Drive->MaxSector = (UINT8)Drive->Parameters.SectorsPerTrack;
        ASSERT (Drive->MaxSector != 0);
        Drive->MaxCylinder = (UINT16)(Drive->Parameters.MaxCylinders - 1);
      } else {
        Drive->MaxHead     = 0;
        Drive->MaxSector   = 1;
        Drive->MaxCylinder = 0;
      }
    } else {
      //
      // Media Present
      //
      switch (Regs.H.AL) {
        case 0x03:
          //
          // 720 KB
          //
          Drive->MaxHead     = 1;
          Drive->MaxSector   = 9;
          Drive->MaxCylinder = 79;
          break;

        case 0x04:
          //
          // 1.44MB
          //
          Drive->MaxHead     = 1;
          Drive->MaxSector   = 18;
          Drive->MaxCylinder = 79;
          break;

        case 0x06:
          //
          // 2.88MB
          //
          Drive->MaxHead     = 1;
          Drive->MaxSector   = 36;
          Drive->MaxCylinder = 79;
          break;

        case 0x0C:
          //
          // 360 KB
          //
          Drive->MaxHead     = 1;
          Drive->MaxSector   = 9;
          Drive->MaxCylinder = 39;
          break;

        case 0x0D:
          //
          // 1.2 MB
          //
          Drive->MaxHead     = 1;
          Drive->MaxSector   = 15;
          Drive->MaxCylinder = 79;
          break;

        case 0x0E:
        //
        // Toshiba 3 mode
        //
        case 0x0F:
        //
        // NEC 3 mode
        //
        case 0x10:
          //
          // Default Media
          //
          if ((Drive->Parameters.Flags & EDD_GEOMETRY_VALID) == EDD_GEOMETRY_VALID) {
            Drive->MaxHead   = (UINT8)(Drive->Parameters.MaxHeads - 1);
            Drive->MaxSector = (UINT8)Drive->Parameters.SectorsPerTrack;
            ASSERT (Drive->MaxSector != 0);
            Drive->MaxCylinder = (UINT16)(Drive->Parameters.MaxCylinders - 1);
          } else {
            Drive->MaxHead     = 0;
            Drive->MaxSector   = 1;
            Drive->MaxCylinder = 0;
          }

          break;

        default:
          //
          // Unknown media type.
          //
          Drive->MaxHead     = 0;
          Drive->MaxSector   = 1;
          Drive->MaxCylinder = 0;
          break;
      }
    }

    Drive->Parameters.PhysicalSectors = (Drive->MaxHead + 1) * Drive->MaxSector * (Drive->MaxCylinder + 1);
    Drive->Parameters.BytesPerSector  = 512;
  }

  //
  // This data comes from the BIOS so it may not allways be valid
  //  since the BIOS may reuse this buffer for future accesses
  //
  PointerMath        = EFI_SEGMENT (Drive->Parameters.Fdpt) << 4;
  PointerMath       += EFI_OFFSET (Drive->Parameters.Fdpt);
  Drive->FdptPointer = (VOID *)PointerMath;

  return TRUE;
}

//
// Block IO Routines
//

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd30BiosReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA         *Media;
  BIOS_BLOCK_IO_DEV          *BiosBlockIoDev;
  EDD_DEVICE_ADDRESS_PACKET  *AddressPacket;
  IA32_REGISTER_SET          Regs;
  UINT64                     TransferBuffer;
  UINTN                      NumberOfBlocks;
  UINTN                      TransferByteSize;
  UINTN                      BlockSize;
  BIOS_LEGACY_DRIVE          *Bios;
  UINTN                      CarryFlag;
  UINTN                      MaxTransferBlocks;
  EFI_BLOCK_IO_PROTOCOL      *BlockIo;

  Media     = This->Media;
  BlockSize = Media->BlockSize;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);
  AddressPacket  = mEddBufferUnder1Mb;

  MaxTransferBlocks = MAX_EDD11_XFER / BlockSize;

  TransferBuffer = (UINT64)(UINTN)Buffer;
  for ( ; BufferSize > 0;) {
    NumberOfBlocks = BufferSize / BlockSize;
    NumberOfBlocks = NumberOfBlocks > MaxTransferBlocks ? MaxTransferBlocks : NumberOfBlocks;
    //
    // Max transfer MaxTransferBlocks
    //
    AddressPacket->PacketSizeInBytes = (UINT8)sizeof (EDD_DEVICE_ADDRESS_PACKET);
    AddressPacket->Zero              = 0;
    AddressPacket->NumberOfBlocks    = (UINT8)NumberOfBlocks;
    AddressPacket->Zero2             = 0;
    AddressPacket->SegOffset         = 0xffffffff;
    AddressPacket->Lba               = (UINT64)Lba;
    AddressPacket->TransferBuffer    = TransferBuffer;

    Regs.H.AH = 0x42;
    Regs.H.DL = BiosBlockIoDev->Bios.Number;
    Regs.X.SI = EFI_OFFSET (AddressPacket);
    Regs.E.DS = EFI_SEGMENT (AddressPacket);

    CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
    DEBUG (
      (
       DEBUG_BLKIO, "Edd30BiosReadBlocks: INT 13 42 DL=%02x : CF=%d AH=%02x\n", BiosBlockIoDev->Bios.Number,
       CarryFlag, Regs.H.AH
      )
      );

    Media->MediaPresent = TRUE;
    if (CarryFlag != 0) {
      //
      // Return Error Status
      //
      BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
      if (BiosBlockIoDev->Bios.ErrorCode == BIOS_DISK_CHANGED) {
        Media->MediaId++;
        Bios = &BiosBlockIoDev->Bios;
        if (Int13GetDeviceParameters (BiosBlockIoDev, Bios) != 0) {
          if (Int13Extensions (BiosBlockIoDev, Bios) != 0) {
            Media->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
            Media->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;
          } else {
            ASSERT (FALSE);
          }

          Media->ReadOnly = FALSE;
          gBS->HandleProtocol (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
          gBS->ReinstallProtocolInterface (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, BlockIo, BlockIo);
          return EFI_MEDIA_CHANGED;
        }
      }

      if (Media->RemovableMedia) {
        Media->MediaPresent = FALSE;
      }

      return EFI_DEVICE_ERROR;
    }

    TransferByteSize = NumberOfBlocks * BlockSize;
    BufferSize       = BufferSize - TransferByteSize;
    TransferBuffer  += TransferByteSize;
    Lba             += NumberOfBlocks;
  }

  return EFI_SUCCESS;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd30BiosWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA         *Media;
  BIOS_BLOCK_IO_DEV          *BiosBlockIoDev;
  EDD_DEVICE_ADDRESS_PACKET  *AddressPacket;
  IA32_REGISTER_SET          Regs;
  UINT64                     TransferBuffer;
  UINTN                      NumberOfBlocks;
  UINTN                      TransferByteSize;
  UINTN                      BlockSize;
  BIOS_LEGACY_DRIVE          *Bios;
  UINTN                      CarryFlag;
  UINTN                      MaxTransferBlocks;
  EFI_BLOCK_IO_PROTOCOL      *BlockIo;

  Media     = This->Media;
  BlockSize = Media->BlockSize;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_DEVICE_ERROR;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);
  AddressPacket  = mEddBufferUnder1Mb;

  MaxTransferBlocks = MAX_EDD11_XFER / BlockSize;

  TransferBuffer = (UINT64)(UINTN)Buffer;
  for ( ; BufferSize > 0;) {
    NumberOfBlocks = BufferSize / BlockSize;
    NumberOfBlocks = NumberOfBlocks > MaxTransferBlocks ? MaxTransferBlocks : NumberOfBlocks;
    //
    // Max transfer MaxTransferBlocks
    //
    AddressPacket->PacketSizeInBytes = (UINT8)sizeof (EDD_DEVICE_ADDRESS_PACKET);
    AddressPacket->Zero              = 0;
    AddressPacket->NumberOfBlocks    = (UINT8)NumberOfBlocks;
    AddressPacket->Zero2             = 0;
    AddressPacket->SegOffset         = 0xffffffff;
    AddressPacket->Lba               = (UINT64)Lba;
    AddressPacket->TransferBuffer    = TransferBuffer;

    Regs.H.AH = 0x43;
    Regs.H.AL = 0x00;
    //
    // Write Verify Off
    //
    Regs.H.DL = (UINT8)(BiosBlockIoDev->Bios.Number);
    Regs.X.SI = EFI_OFFSET (AddressPacket);
    Regs.E.DS = EFI_SEGMENT (AddressPacket);

    CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
    DEBUG (
      (
       DEBUG_BLKIO, "Edd30BiosWriteBlocks: INT 13 43 DL=%02x : CF=%d AH=%02x\n", BiosBlockIoDev->Bios.Number,
       CarryFlag, Regs.H.AH
      )
      );

    Media->MediaPresent = TRUE;
    if (CarryFlag != 0) {
      //
      // Return Error Status
      //
      BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
      if (BiosBlockIoDev->Bios.ErrorCode == BIOS_DISK_CHANGED) {
        Media->MediaId++;
        Bios = &BiosBlockIoDev->Bios;
        if (Int13GetDeviceParameters (BiosBlockIoDev, Bios) != 0) {
          if (Int13Extensions (BiosBlockIoDev, Bios) != 0) {
            Media->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
            Media->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;
          } else {
            ASSERT (FALSE);
          }

          Media->ReadOnly = FALSE;
          gBS->HandleProtocol (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
          gBS->ReinstallProtocolInterface (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, BlockIo, BlockIo);
          return EFI_MEDIA_CHANGED;
        }
      } else if (BiosBlockIoDev->Bios.ErrorCode == BIOS_WRITE_PROTECTED) {
        Media->ReadOnly = TRUE;
        return EFI_WRITE_PROTECTED;
      }

      if (Media->RemovableMedia) {
        Media->MediaPresent = FALSE;
      }

      return EFI_DEVICE_ERROR;
    }

    Media->ReadOnly  = FALSE;
    TransferByteSize = NumberOfBlocks * BlockSize;
    BufferSize       = BufferSize - TransferByteSize;
    TransferBuffer  += TransferByteSize;
    Lba             += NumberOfBlocks;
  }

  return EFI_SUCCESS;
}

/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
BiosBlockIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
BiosBlockIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                ExtendedVerification
  )
{
  BIOS_BLOCK_IO_DEV  *BiosBlockIoDev;
  IA32_REGISTER_SET  Regs;
  UINTN              CarryFlag;

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  Regs.H.AH = 0x00;
  Regs.H.DL = BiosBlockIoDev->Bios.Number;
  CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
  DEBUG (
    (
     DEBUG_INIT, "BiosBlockIoReset: INT 13 00 DL=%02x : CF=%d AH=%02x\n", BiosBlockIoDev->Bios.Number, CarryFlag,
     Regs.H.AH
    )
    );

  if (CarryFlag != 0) {
    if (Regs.H.AL == BIOS_RESET_FAILED) {
      Regs.H.AH = 0x00;
      Regs.H.DL = BiosBlockIoDev->Bios.Number;
      CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
      DEBUG (
        (
         DEBUG_INIT, "BiosBlockIoReset: INT 13 00 DL=%02x : CF=%d AH=%02x\n", BiosBlockIoDev->Bios.Number, CarryFlag,
         Regs.H.AH
        )
        );
      if (CarryFlag != 0) {
        BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
        return EFI_DEVICE_ERROR;
      }
    }
  }

  return EFI_SUCCESS;
}

//
//
// These functions need to double buffer all data under 1MB!
//
//

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd11BiosReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA         *Media;
  BIOS_BLOCK_IO_DEV          *BiosBlockIoDev;
  EDD_DEVICE_ADDRESS_PACKET  *AddressPacket;
  IA32_REGISTER_SET          Regs;
  UINT64                     TransferBuffer;
  UINTN                      NumberOfBlocks;
  UINTN                      TransferByteSize;
  UINTN                      BlockSize;
  BIOS_LEGACY_DRIVE          *Bios;
  UINTN                      CarryFlag;
  UINTN                      MaxTransferBlocks;
  EFI_BLOCK_IO_PROTOCOL      *BlockIo;

  Media     = This->Media;
  BlockSize = Media->BlockSize;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);
  AddressPacket  = mEddBufferUnder1Mb;

  MaxTransferBlocks = MAX_EDD11_XFER / BlockSize;

  TransferBuffer = (UINT64)(UINTN)mEdd11Buffer;
  for ( ; BufferSize > 0;) {
    NumberOfBlocks = BufferSize / BlockSize;
    NumberOfBlocks = NumberOfBlocks > MaxTransferBlocks ? MaxTransferBlocks : NumberOfBlocks;
    //
    // Max transfer MaxTransferBlocks
    //
    // ZeroMem (AddressPacket, sizeof (EDD_DEVICE_ADDRESS_PACKET));
    AddressPacket->PacketSizeInBytes = (UINT8)sizeof (EDD_DEVICE_ADDRESS_PACKET);
    AddressPacket->Zero              = 0;
    AddressPacket->NumberOfBlocks    = (UINT8)NumberOfBlocks;
    AddressPacket->Zero2             = 0;
    //
    // TransferBuffer has been 4KB alignment. Normalize TransferBuffer to make offset as 0 in seg:offset
    // format to transfer maximum 127 blocks of data.
    // Otherwise when offset adding data size exceeds 0xFFFF, if OpROM does not normalize TransferBuffer,
    // INT13 function 42H will return data boundary error 09H.
    //
    AddressPacket->SegOffset = (UINT32)LShiftU64 (RShiftU64 (TransferBuffer, 4), 16);
    AddressPacket->Lba       = (UINT64)Lba;

    Regs.H.AH = 0x42;
    Regs.H.DL = BiosBlockIoDev->Bios.Number;
    Regs.X.SI = EFI_OFFSET (AddressPacket);
    Regs.E.DS = EFI_SEGMENT (AddressPacket);

    CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
    DEBUG (
      (
       DEBUG_BLKIO, "Edd11BiosReadBlocks: INT 13 42 DL=%02x : CF=%d AH=%02x : LBA 0x%lx  Block(s) %0d \n",
       BiosBlockIoDev->Bios.Number, CarryFlag, Regs.H.AH, Lba, NumberOfBlocks
      )
      );

    Media->MediaPresent = TRUE;
    if (CarryFlag != 0) {
      //
      // Return Error Status
      //
      BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
      if (BiosBlockIoDev->Bios.ErrorCode == BIOS_DISK_CHANGED) {
        Media->MediaId++;
        Bios = &BiosBlockIoDev->Bios;
        if (Int13GetDeviceParameters (BiosBlockIoDev, Bios) != 0) {
          if (Int13Extensions (BiosBlockIoDev, Bios) != 0) {
            Media->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
            Media->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;
          } else {
            ASSERT (FALSE);
          }

          Media->ReadOnly = FALSE;
          gBS->HandleProtocol (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
          gBS->ReinstallProtocolInterface (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, BlockIo, BlockIo);
          return EFI_MEDIA_CHANGED;
        }
      }

      if (Media->RemovableMedia) {
        Media->MediaPresent = FALSE;
      }

      return EFI_DEVICE_ERROR;
    }

    TransferByteSize = NumberOfBlocks * BlockSize;
    CopyMem (Buffer, (VOID *)(UINTN)TransferBuffer, TransferByteSize);
    BufferSize = BufferSize - TransferByteSize;
    Buffer     = (VOID *)((UINT8 *)Buffer + TransferByteSize);
    Lba       += NumberOfBlocks;
  }

  return EFI_SUCCESS;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd11BiosWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA         *Media;
  BIOS_BLOCK_IO_DEV          *BiosBlockIoDev;
  EDD_DEVICE_ADDRESS_PACKET  *AddressPacket;
  IA32_REGISTER_SET          Regs;
  UINT64                     TransferBuffer;
  UINTN                      NumberOfBlocks;
  UINTN                      TransferByteSize;
  UINTN                      BlockSize;
  BIOS_LEGACY_DRIVE          *Bios;
  UINTN                      CarryFlag;
  UINTN                      MaxTransferBlocks;
  EFI_BLOCK_IO_PROTOCOL      *BlockIo;

  Media     = This->Media;
  BlockSize = Media->BlockSize;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);
  AddressPacket  = mEddBufferUnder1Mb;

  MaxTransferBlocks = MAX_EDD11_XFER / BlockSize;

  TransferBuffer = (UINT64)(UINTN)mEdd11Buffer;
  for ( ; BufferSize > 0;) {
    NumberOfBlocks = BufferSize / BlockSize;
    NumberOfBlocks = NumberOfBlocks > MaxTransferBlocks ? MaxTransferBlocks : NumberOfBlocks;
    //
    // Max transfer MaxTransferBlocks
    //
    AddressPacket->PacketSizeInBytes = (UINT8)sizeof (EDD_DEVICE_ADDRESS_PACKET);
    AddressPacket->Zero              = 0;
    AddressPacket->NumberOfBlocks    = (UINT8)NumberOfBlocks;
    AddressPacket->Zero2             = 0;
    //
    // TransferBuffer has been 4KB alignment. Normalize TransferBuffer to make offset as 0 in seg:offset
    // format to transfer maximum 127 blocks of data.
    // Otherwise when offset adding data size exceeds 0xFFFF, if OpROM does not normalize TransferBuffer,
    // INT13 function 42H will return data boundary error 09H.
    //
    AddressPacket->SegOffset = (UINT32)LShiftU64 (RShiftU64 (TransferBuffer, 4), 16);
    AddressPacket->Lba       = (UINT64)Lba;

    Regs.H.AH = 0x43;
    Regs.H.AL = 0x00;
    //
    // Write Verify disable
    //
    Regs.H.DL = BiosBlockIoDev->Bios.Number;
    Regs.X.SI = EFI_OFFSET (AddressPacket);
    Regs.E.DS = EFI_SEGMENT (AddressPacket);

    TransferByteSize = NumberOfBlocks * BlockSize;
    CopyMem ((VOID *)(UINTN)TransferBuffer, Buffer, TransferByteSize);

    CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
    DEBUG (
      (
       DEBUG_BLKIO, "Edd11BiosWriteBlocks: INT 13 43 DL=%02x : CF=%d AH=%02x\n: LBA 0x%lx  Block(s) %0d \n",
       BiosBlockIoDev->Bios.Number, CarryFlag, Regs.H.AH, Lba, NumberOfBlocks
      )
      );
    Media->MediaPresent = TRUE;
    if (CarryFlag != 0) {
      //
      // Return Error Status
      //
      BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
      if (BiosBlockIoDev->Bios.ErrorCode == BIOS_DISK_CHANGED) {
        Media->MediaId++;
        Bios = &BiosBlockIoDev->Bios;
        if (Int13GetDeviceParameters (BiosBlockIoDev, Bios) != 0) {
          if (Int13Extensions (BiosBlockIoDev, Bios) != 0) {
            Media->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
            Media->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;
          } else {
            ASSERT (FALSE);
          }

          Media->ReadOnly = FALSE;
          gBS->HandleProtocol (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
          gBS->ReinstallProtocolInterface (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, BlockIo, BlockIo);
          return EFI_MEDIA_CHANGED;
        }
      } else if (BiosBlockIoDev->Bios.ErrorCode == BIOS_WRITE_PROTECTED) {
        Media->ReadOnly = TRUE;
        return EFI_WRITE_PROTECTED;
      }

      if (Media->RemovableMedia) {
        Media->MediaPresent = FALSE;
      }

      return EFI_DEVICE_ERROR;
    }

    Media->ReadOnly = FALSE;
    BufferSize      = BufferSize - TransferByteSize;
    Buffer          = (VOID *)((UINT8 *)Buffer + TransferByteSize);
    Lba            += NumberOfBlocks;
  }

  return EFI_SUCCESS;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BiosReadLegacyDrive (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA     *Media;
  BIOS_BLOCK_IO_DEV      *BiosBlockIoDev;
  IA32_REGISTER_SET      Regs;
  UINTN                  UpperCylinder;
  UINTN                  Temp;
  UINTN                  Cylinder;
  UINTN                  Head;
  UINTN                  Sector;
  UINTN                  NumberOfBlocks;
  UINTN                  TransferByteSize;
  UINTN                  ShortLba;
  UINTN                  CheckLba;
  UINTN                  BlockSize;
  BIOS_LEGACY_DRIVE      *Bios;
  UINTN                  CarryFlag;
  UINTN                  Retry;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;

  Media     = This->Media;
  BlockSize = Media->BlockSize;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);
  ShortLba       = (UINTN)Lba;

  while (BufferSize != 0) {
    //
    // Compute I/O location in Sector, Head, Cylinder format
    //
    Sector   = (ShortLba % BiosBlockIoDev->Bios.MaxSector) + 1;
    Temp     = ShortLba / BiosBlockIoDev->Bios.MaxSector;
    Head     = Temp % (BiosBlockIoDev->Bios.MaxHead + 1);
    Cylinder = Temp / (BiosBlockIoDev->Bios.MaxHead + 1);

    //
    // Limit transfer to this Head & Cylinder
    //
    NumberOfBlocks = BufferSize / BlockSize;
    Temp           = BiosBlockIoDev->Bios.MaxSector - Sector + 1;
    NumberOfBlocks = NumberOfBlocks > Temp ? Temp : NumberOfBlocks;

    Retry = 3;
    do {
      //
      // Perform the IO
      //
      Regs.H.AH = 2;
      Regs.H.AL = (UINT8)NumberOfBlocks;
      Regs.H.DL = BiosBlockIoDev->Bios.Number;

      UpperCylinder = (Cylinder & 0x0f00) >> 2;

      CheckLba = Cylinder * (BiosBlockIoDev->Bios.MaxHead + 1) + Head;
      CheckLba = CheckLba * BiosBlockIoDev->Bios.MaxSector + Sector - 1;

      DEBUG (
        (DEBUG_BLKIO,
         "RLD: LBA %x (%x), Sector %x (%x), Head %x (%x), Cyl %x, UCyl %x\n",
         ShortLba,
         CheckLba,
         Sector,
         BiosBlockIoDev->Bios.MaxSector,
         Head,
         BiosBlockIoDev->Bios.MaxHead,
         Cylinder,
         UpperCylinder)
        );
      ASSERT (CheckLba == ShortLba);

      Regs.H.CL = (UINT8)((Sector & 0x3f) + (UpperCylinder & 0xff));
      Regs.H.DH = (UINT8)(Head & 0x3f);
      Regs.H.CH = (UINT8)(Cylinder & 0xff);

      Regs.X.BX = EFI_OFFSET (mEdd11Buffer);
      Regs.E.ES = EFI_SEGMENT (mEdd11Buffer);

      DEBUG (
        (DEBUG_BLKIO,
         "INT 13h: AX:(02%02x) DX:(%02x%02x) CX:(%02x%02x) BX:(%04x) ES:(%04x)\n",
         Regs.H.AL,
         (UINT8)(Head & 0x3f),
         Regs.H.DL,
         (UINT8)(Cylinder & 0xff),
         (UINT8)((Sector & 0x3f) + (UpperCylinder & 0xff)),
         EFI_OFFSET (mEdd11Buffer),
         EFI_SEGMENT (mEdd11Buffer))
        );

      CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
      DEBUG (
        (
         DEBUG_BLKIO, "BiosReadLegacyDrive: INT 13 02 DL=%02x : CF=%d AH=%02x\n", BiosBlockIoDev->Bios.Number,
         CarryFlag, Regs.H.AH
        )
        );
      Retry--;
    } while (CarryFlag != 0 && Retry != 0 && Regs.H.AH != BIOS_DISK_CHANGED);

    Media->MediaPresent = TRUE;
    if (CarryFlag != 0) {
      //
      // Return Error Status
      //
      BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
      if (BiosBlockIoDev->Bios.ErrorCode == BIOS_DISK_CHANGED) {
        Media->MediaId++;
        Bios = &BiosBlockIoDev->Bios;
        if (Int13GetDeviceParameters (BiosBlockIoDev, Bios) != 0) {
          //
          // If the size of the media changed we need to reset the disk geometry
          //
          if (Int13Extensions (BiosBlockIoDev, Bios) != 0) {
            Media->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
            Media->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;
          } else {
            //
            // Legacy Interfaces
            //
            Media->LastBlock = (Bios->MaxHead + 1) * Bios->MaxSector * (Bios->MaxCylinder + 1) - 1;
            Media->BlockSize = 512;
          }

          Media->ReadOnly = FALSE;
          gBS->HandleProtocol (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
          gBS->ReinstallProtocolInterface (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, BlockIo, BlockIo);
          return EFI_MEDIA_CHANGED;
        }
      }

      if (Media->RemovableMedia) {
        Media->MediaPresent = FALSE;
      }

      return EFI_DEVICE_ERROR;
    }

    TransferByteSize = NumberOfBlocks * BlockSize;
    CopyMem (Buffer, mEdd11Buffer, TransferByteSize);

    ShortLba   = ShortLba + NumberOfBlocks;
    BufferSize = BufferSize - TransferByteSize;
    Buffer     = (VOID *)((UINT8 *)Buffer + TransferByteSize);
  }

  return EFI_SUCCESS;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BiosWriteLegacyDrive (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA     *Media;
  BIOS_BLOCK_IO_DEV      *BiosBlockIoDev;
  IA32_REGISTER_SET      Regs;
  UINTN                  UpperCylinder;
  UINTN                  Temp;
  UINTN                  Cylinder;
  UINTN                  Head;
  UINTN                  Sector;
  UINTN                  NumberOfBlocks;
  UINTN                  TransferByteSize;
  UINTN                  ShortLba;
  UINTN                  CheckLba;
  UINTN                  BlockSize;
  BIOS_LEGACY_DRIVE      *Bios;
  UINTN                  CarryFlag;
  UINTN                  Retry;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;

  Media     = This->Media;
  BlockSize = Media->BlockSize;

  ZeroMem (&Regs, sizeof (IA32_REGISTER_SET));

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BiosBlockIoDev = BIOS_BLOCK_IO_FROM_THIS (This);
  ShortLba       = (UINTN)Lba;

  while (BufferSize != 0) {
    //
    // Compute I/O location in Sector, Head, Cylinder format
    //
    Sector   = (ShortLba % BiosBlockIoDev->Bios.MaxSector) + 1;
    Temp     = ShortLba / BiosBlockIoDev->Bios.MaxSector;
    Head     = Temp % (BiosBlockIoDev->Bios.MaxHead + 1);
    Cylinder = Temp / (BiosBlockIoDev->Bios.MaxHead + 1);

    //
    // Limit transfer to this Head & Cylinder
    //
    NumberOfBlocks = BufferSize / BlockSize;
    Temp           = BiosBlockIoDev->Bios.MaxSector - Sector + 1;
    NumberOfBlocks = NumberOfBlocks > Temp ? Temp : NumberOfBlocks;

    Retry = 3;
    do {
      //
      // Perform the IO
      //
      Regs.H.AH = 3;
      Regs.H.AL = (UINT8)NumberOfBlocks;
      Regs.H.DL = BiosBlockIoDev->Bios.Number;

      UpperCylinder = (Cylinder & 0x0f00) >> 2;

      CheckLba = Cylinder * (BiosBlockIoDev->Bios.MaxHead + 1) + Head;
      CheckLba = CheckLba * BiosBlockIoDev->Bios.MaxSector + Sector - 1;

      DEBUG (
        (DEBUG_BLKIO,
         "RLD: LBA %x (%x), Sector %x (%x), Head %x (%x), Cyl %x, UCyl %x\n",
         ShortLba,
         CheckLba,
         Sector,
         BiosBlockIoDev->Bios.MaxSector,
         Head,
         BiosBlockIoDev->Bios.MaxHead,
         Cylinder,
         UpperCylinder)
        );
      ASSERT (CheckLba == ShortLba);

      Regs.H.CL = (UINT8)((Sector & 0x3f) + (UpperCylinder & 0xff));
      Regs.H.DH = (UINT8)(Head & 0x3f);
      Regs.H.CH = (UINT8)(Cylinder & 0xff);

      Regs.X.BX = EFI_OFFSET (mEdd11Buffer);
      Regs.E.ES = EFI_SEGMENT (mEdd11Buffer);

      TransferByteSize = NumberOfBlocks * BlockSize;
      CopyMem (mEdd11Buffer, Buffer, TransferByteSize);

      DEBUG (
        (DEBUG_BLKIO,
         "INT 13h: AX:(03%02x) DX:(%02x%02x) CX:(%02x%02x) BX:(%04x) ES:(%04x)\n",
         Regs.H.AL,
         (UINT8)(Head & 0x3f),
         Regs.H.DL,
         (UINT8)(Cylinder & 0xff),
         (UINT8)((Sector & 0x3f) + (UpperCylinder & 0xff)),
         EFI_OFFSET (mEdd11Buffer),
         EFI_SEGMENT (mEdd11Buffer))
        );

      CarryFlag = LegacyBiosInt86 (BiosBlockIoDev, 0x13, &Regs);
      DEBUG (
        (
         DEBUG_BLKIO, "BiosWriteLegacyDrive: INT 13 03 DL=%02x : CF=%d AH=%02x\n", BiosBlockIoDev->Bios.Number,
         CarryFlag, Regs.H.AH
        )
        );
      Retry--;
    } while (CarryFlag != 0 && Retry != 0 && Regs.H.AH != BIOS_DISK_CHANGED);

    Media->MediaPresent = TRUE;
    if (CarryFlag != 0) {
      //
      // Return Error Status
      //
      BiosBlockIoDev->Bios.ErrorCode = Regs.H.AH;
      if (BiosBlockIoDev->Bios.ErrorCode == BIOS_DISK_CHANGED) {
        Media->MediaId++;
        Bios = &BiosBlockIoDev->Bios;
        if (Int13GetDeviceParameters (BiosBlockIoDev, Bios) != 0) {
          if (Int13Extensions (BiosBlockIoDev, Bios) != 0) {
            Media->LastBlock = (EFI_LBA)Bios->Parameters.PhysicalSectors - 1;
            Media->BlockSize = (UINT32)Bios->Parameters.BytesPerSector;
          } else {
            //
            // Legacy Interfaces
            //
            Media->LastBlock = (Bios->MaxHead + 1) * Bios->MaxSector * (Bios->MaxCylinder + 1) - 1;
            Media->BlockSize = 512;
          }

          //
          // If the size of the media changed we need to reset the disk geometry
          //
          Media->ReadOnly = FALSE;
          gBS->HandleProtocol (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
          gBS->ReinstallProtocolInterface (BiosBlockIoDev->Handle, &gEfiBlockIoProtocolGuid, BlockIo, BlockIo);
          return EFI_MEDIA_CHANGED;
        }
      } else if (BiosBlockIoDev->Bios.ErrorCode == BIOS_WRITE_PROTECTED) {
        Media->ReadOnly = TRUE;
        return EFI_WRITE_PROTECTED;
      }

      if (Media->RemovableMedia) {
        Media->MediaPresent = FALSE;
      }

      return EFI_DEVICE_ERROR;
    }

    Media->ReadOnly = FALSE;
    ShortLba        = ShortLba + NumberOfBlocks;
    BufferSize      = BufferSize - TransferByteSize;
    Buffer          = (VOID *)((UINT8 *)Buffer + TransferByteSize);
  }

  return EFI_SUCCESS;
}
