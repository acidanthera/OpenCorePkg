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
#include <Library/OcPeCoffLib.h>

STATIC
UINT64
ApfsFletcher64 (
  VOID    *Data,
  UINTN   DataSize
  )
{
  UINT32        *Walker;
  UINT32        *WalkerEnd;
  UINT64        Sum1;
  UINT64        Sum2;
  UINT32        Rem;

  //
  // For APFS we have the following guarantees (checked outside).
  // - DataSize is always divisible by 4 (UINT32), the only potential exceptions
  //   are multiples of block sizes of 1 and 2, which we do not support and filter out.
  // - DataSize is always between 0x1000-8 and 0x10000-8, i.e. within UINT16.
  //
  ASSERT (DataSize >= APFS_NX_MINIMUM_BLOCK_SIZE - sizeof (UINT64));
  ASSERT (DataSize <= APFS_NX_MAXIMUM_BLOCK_SIZE - sizeof (UINT64));
  ASSERT (DataSize % sizeof (UINT32) == 0);

  Sum1 = 0;
  Sum2 = 0;

  Walker     = Data;
  WalkerEnd  = Walker + DataSize / sizeof (UINT32);

  //
  // Do usual Fletcher-64 rounds without modulo due to impossible overflow.
  //
  while (Walker < WalkerEnd) {
    //
    // Sum1 never overflows, because 0xFFFFFFFF * (0x10000-8) < MAX_UINT64.
    // This is just a normal sum of data values.
    //
    Sum1 += *Walker;
    //
    // Sum2 never overflows, because 0xFFFFFFFF * (0x4000-1) * 0x1FFF < MAX_UINT64.
    // This is just a normal arithmetical progression of sums.
    //
    Sum2 += Sum1;
    ++Walker;
  }

  //
  // Split Fletcher-64 halves.
  // As per Chinese remainder theorem, perform the modulo now.
  // No overflows also possible as seen from Sum1/Sum2 upper bounds above.
  //

  Sum2 += Sum1;
  APFS_MOD_MAX_UINT32 (Sum2, &Rem);
  Sum2  = ~Rem;

  Sum1 += Sum2;
  APFS_MOD_MAX_UINT32 (Sum1, &Rem);
  Sum1  = ~Rem;

  return (Sum1 << 32U) | Sum2;
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

  NewChecksum = ApfsFletcher64 (
    &Block->ObjectOid,
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

  DEBUG ((
    DEBUG_INFO,
    "OCJS: Block (P:%d|F:%d) read req %Lx -> %Lx of %x (mask %u, mul %u) - %r\n",
    BlockIo == PrivateData->BlockIo,
    PrivateData->IsFusion,
    PrivateData->EfiJumpStart,
    Lba,
    PrivateData->ApfsBlockSize,
    PrivateData->FusionMask,
    PrivateData->LbaMultiplier,
    Status
    ));

  if (EFI_ERROR (Status)) {
    FreePool (JumpStart);
    return Status;
  }

  //
  // Jump start is expected to have JSDR magic.
  // Version is not checked by ApfsJumpStart driver.
  //
  if (JumpStart->Magic != APFS_NX_EFI_JUMPSTART_MAGIC) {
    DEBUG ((DEBUG_INFO, "OCJS: Unknown JSDR magic %08x, expected %08x\n", JumpStart->Magic, APFS_NX_EFI_JUMPSTART_MAGIC));
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
    DEBUG ((DEBUG_INFO, "OCJS: Invalid extent count %u / %u\n", JumpStart->NumExtents, MaxExtents));
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
  OUT UINT32                 *DriverSize,
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

    if (JumpStart->RecordExtents[Index].BlockCount > MAX_UINTN
      || OcOverflowMulUN ((UINTN) JumpStart->RecordExtents[Index].BlockCount, PrivateData->ApfsBlockSize, &ChunkSize)
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
    // Ensure APFS block size is:
    // - A multiple of disk block size.
    // - Divisible by UINT32 for fletcher checksum to work (e.g. when block size is 1 or 2).
    // - Within minimum and maximum edges.
    //
    if (SuperBlock->BlockSize < BlockIo->Media->BlockSize
      || (SuperBlock->BlockSize & (BlockIo->Media->BlockSize - 1)) != 0
      || (SuperBlock->BlockSize & (sizeof (UINT32) - 1)) != 0
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
InternalApfsReadDriver (
  IN  APFS_PRIVATE_DATA    *PrivateData,
  OUT UINT32               *DriverSize,
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
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to read JumpStart for %g - %r\n",
      &PrivateData->LocationInfo.ContainerUuid,
      Status
      ));
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
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to read driver for %g - %r\n",
      &PrivateData->LocationInfo.ContainerUuid,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}
