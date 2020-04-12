/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "OcApfsInternal.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcApfsLib.h>
#include <Library/OcGuardLib.h>

STATIC
UINT64
ApfsChecksumCalculate (
  UINT32  *Data,
  UINTN   DataSize
  )
{
  //
  // TODO: This needs to be replaced with an optimised version.
  //

  UINTN         Index;
  UINT64        Sum1;
  UINT64        Check1;
  UINT64        Sum2;
  UINT64        Check2;
  UINT32        Remainder;
  CONST UINT32  ModValue = 0xFFFFFFFF;

  Sum1 = 0;
  Sum2 = 0;

  for (Index = 0; Index < DataSize / sizeof (UINT32); Index++) {
    DivU64x32Remainder (Sum1 + Data[Index], ModValue, &Remainder);
    Sum1 = Remainder;
    DivU64x32Remainder (Sum2 + Sum1, ModValue, &Remainder);
    Sum2 = Remainder;
  }

  DivU64x32Remainder (Sum1 + Sum2, ModValue, &Remainder);
  Check1 = ModValue - Remainder;
  DivU64x32Remainder (Sum1 + Check1, ModValue, &Remainder);
  Check2 = ModValue - Remainder;

  return (Check2 << 32U) | Check1;
}

STATIC
BOOLEAN
ApfsBlockChecksumVerify (
  APFS_OBJ_PHYS   *Block,
  UINTN           DataSize
  )
{
  UINT64  NewChecksum;

  ASSERT (DataSize > sizeof (*Block));

  NewChecksum = ApfsChecksumCalculate (
    (VOID *) &Block->ObjectOid,
    DataSize - sizeof (Block->Checksum)
    );

  if (NewChecksum == Block->Checksum) {
    return TRUE;
  }

  DEBUG ((DEBUG_INFO, "OCJS: Checksum mismatch for %Lx\n", Block->ObjectOid));
  return FALSE;
}

STATIC
EFI_STATUS
ApfsReadJumpStart (
  IN  APFS_PRIVATE_DATA      *PrivateData,
  OUT APFS_NX_EFI_JUMPSTART  **JumpStartPtr
  )
{
  EFI_STATUS             Status;
  APFS_NX_EFI_JUMPSTART  *JumpStart;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_LBA                Lba;
  UINT32                 MaxExtents;

  //
  // No jump start driver, ignore.
  //
  if (PrivateData->EfiJumpStart == 0) {
    DEBUG ((DEBUG_INFO, "OCJS: Missing JumpStart for %g\n", &PrivateData->LocationInfo.ContainerUuid));
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate memory for jump start.
  //
  JumpStart = AllocateZeroPool (PrivateData->ApfsBlockSize);
  if (JumpStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlockIo = InternalApfsTranslateBlock (PrivateData, PrivateData->EfiJumpStart, &Lba);

  //
  // Read jump start and abort on failure.
  //
  Status = BlockIo->ReadBlocks (
    BlockIo,
    BlockIo->Media->MediaId,
    Lba,
    PrivateData->ApfsBlockSize,
    JumpStart
    );
  if (EFI_ERROR (Status)) {
    FreePool (JumpStart);
    return Status;
  }

  //
  // Jump start is expected to have JSDR magic.
  // Version is not checked by ApfsJumpStart driver.
  //
  if (JumpStart->Magic != APFS_NX_EFI_JUMPSTART_MAGIC) {
    FreePool (JumpStart);
    return EFI_UNSUPPORTED;
  }

  //
  // Calculate and verify checksum.
  //
  if (!ApfsBlockChecksumVerify (&JumpStart->BlockHeader, PrivateData->ApfsBlockSize)) {
    FreePool (JumpStart);
    return EFI_UNSUPPORTED;
  }

  //
  // Ensure that extent count does not overflow.
  //
  MaxExtents = (PrivateData->ApfsBlockSize - sizeof (*JumpStart)) / sizeof (JumpStart->RecordExtents[0]);
  if (MaxExtents < JumpStart->NumExtents) {
    FreePool (JumpStart);
    return EFI_UNSUPPORTED;
  }

  *JumpStartPtr = JumpStart;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ApfsReadDriver (
  IN  APFS_PRIVATE_DATA      *PrivateData,
  IN  APFS_NX_EFI_JUMPSTART  *JumpStart,
  OUT UINTN                  *DriverSize,
  OUT VOID                   **DriverBuffer
  )
{
  EFI_STATUS             Status;
  VOID                   *EfiFile;
  UINTN                  EfiFileSize;
  UINTN                  OrgEfiFileSize;
  UINT8                  *ChunkPtr;
  UINTN                  ChunkSize;
  UINTN                  Index;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_LBA                Lba;

  EfiFileSize = JumpStart->EfiFileLen / PrivateData->ApfsBlockSize + 1;
  if (OcOverflowMulUN (EfiFileSize, PrivateData->ApfsBlockSize, &EfiFileSize)) {
    return EFI_SECURITY_VIOLATION;
  }

  OrgEfiFileSize = EfiFileSize;

  EfiFile = AllocateZeroPool (EfiFileSize);
  if (EfiFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ChunkPtr = EfiFile;

  for (Index = 0; Index < JumpStart->NumExtents || EfiFileSize != 0; ++Index) {
    BlockIo = InternalApfsTranslateBlock (
      PrivateData,
      JumpStart->RecordExtents[Index].StartPhysicalAddr,
      &Lba
      );

    if (OcOverflowMulUN (JumpStart->RecordExtents[Index].BlockCount, PrivateData->ApfsBlockSize, &ChunkSize)
      || ChunkSize > EfiFileSize) {
      FreePool (EfiFile);
      return EFI_SECURITY_VIOLATION;
    }

    Status = BlockIo->ReadBlocks (
      BlockIo,
      BlockIo->Media->MediaId,
      Lba,
      ChunkSize,
      ChunkPtr
      );

    if (EFI_ERROR (Status)) {
      FreePool (EfiFile);
      return Status;
    }

    ChunkPtr    += ChunkSize;
    EfiFileSize -= ChunkSize;
  }

  //
  // Ensure that we do not have meaningful trailing memory just in case.
  //
  if (OrgEfiFileSize != JumpStart->EfiFileLen) {
    ChunkPtr  = EfiFile;
    ChunkPtr += JumpStart->EfiFileLen;
    ZeroMem (ChunkPtr, OrgEfiFileSize - JumpStart->EfiFileLen);
  }

  *DriverSize   = JumpStart->EfiFileLen;
  *DriverBuffer = EfiFile;

  return EFI_SUCCESS;
}

EFI_STATUS
InternalApfsReadSuperBlock (
  IN  EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  OUT APFS_NX_SUPERBLOCK     **SuperBlockPtr
  )
{
  EFI_STATUS           Status;
  APFS_NX_SUPERBLOCK   *SuperBlock;
  UINTN                ReadSize;
  UINTN                Retry;

  //
  // According to APFS spec APFS block size is a multiple of disk block size.
  // Start by reading APFS_NX_MINIMUM_BLOCK_SIZE aligned to block size. 
  //
  ReadSize = ALIGN_VALUE (APFS_NX_MINIMUM_BLOCK_SIZE, BlockIo->Media->BlockSize);

  SuperBlock = NULL;

  //
  // Second attempt is given for cases when block size is bigger than our guessed size.
  //
  for (Retry = 0; Retry < 2; ++Retry) {
    //
    // Allocate memory for super block.
    //
    SuperBlock = AllocateZeroPool (ReadSize);
    if (SuperBlock == NULL) {
      break;
    }

    //
    // Read super block and abort on failure.
    //
    Status = BlockIo->ReadBlocks (
      BlockIo,
      BlockIo->Media->MediaId,
      0,
      ReadSize,
      SuperBlock
      );
    if (EFI_ERROR (Status)) {
      break;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "OCJS: Testing disk with %8X magic %u block\n",
      SuperBlock->Magic,
      SuperBlock->BlockSize
      ));

    //
    // Super block is expected to have NXSB magic.
    //
    if (SuperBlock->Magic != APFS_NX_SIGNATURE) {
      break;
    }

    //
    // Ensure APFS block size is a multiple of disk block size.
    //
    if (SuperBlock->BlockSize < BlockIo->Media->BlockSize
      || (SuperBlock->BlockSize & (BlockIo->Media->BlockSize - 1)) != 0
      || SuperBlock->BlockSize < APFS_NX_MINIMUM_BLOCK_SIZE
      || SuperBlock->BlockSize > APFS_NX_MAXIMUM_BLOCK_SIZE) {
      break;
    }

    //
    // Check if we can calculate the checksum and try again on failure.
    //
    if (SuperBlock->BlockSize > ReadSize) {
      ReadSize = SuperBlock->BlockSize;
      FreePool (SuperBlock);
      SuperBlock = NULL;
      continue;
    }

    //
    // Calculate and verify checksum.
    //
    if (!ApfsBlockChecksumVerify (&SuperBlock->BlockHeader, SuperBlock->BlockSize)) {
      break;
    }

    //
    // Verify object type and flags.
    // SubType being 0 comes from ApfsJumpStart and is not documented.
    // ObjectOid being 1 comes from ApfsJumpStart and is not documented.
    //
    if (SuperBlock->BlockHeader.ObjectType != (APFS_OBJ_EPHEMERAL | APFS_OBJECT_TYPE_NX_SUPERBLOCK)
      || SuperBlock->BlockHeader.ObjectSubType != 0
      || SuperBlock->BlockHeader.ObjectOid != 1) {
      break;
    }

    //
    // Super block is assumed to be legit.
    //
    *SuperBlockPtr = SuperBlock;
    return EFI_SUCCESS;
  }

  //
  // All retry attempts exceeded.
  //
  if (SuperBlock != NULL) {
    FreePool (SuperBlock);
  }
  return EFI_UNSUPPORTED;
}

EFI_STATUS
InternalApfsReadJumpStartDriver (
  IN  APFS_PRIVATE_DATA    *PrivateData,
  OUT UINTN                *DriverSize,
  OUT VOID                 **DriverBuffer
  )
{
  EFI_STATUS             Status;
  APFS_NX_EFI_JUMPSTART  *JumpStart;

  Status = ApfsReadJumpStart (
    PrivateData,
    &JumpStart
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCJS: Failed to read JumpStart for %g\n", &PrivateData->LocationInfo.ContainerUuid));
    return Status;
  }

  Status = ApfsReadDriver (
    PrivateData,
    JumpStart,
    DriverSize,
    DriverBuffer
    );

  FreePool (JumpStart);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCJS: Failed to read driver for %g\n", &PrivateData->LocationInfo.ContainerUuid));
    return Status;
  }

  return EFI_SUCCESS;
}
