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
#include <IndustryStandard/PeImage.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcApfsLib.h>
#include <Library/OcGuardLib.h>

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

EFI_STATUS
InternalApfsGetDriverVersion (
  IN  VOID                 *DriverBuffer,
  IN  UINTN                DriverSize,
  OUT APFS_DRIVER_VERSION  **DriverVersionPtr
  )
{
  //
  // apfs.efi versioning is more restricted than generic PE parsing.
  // In future we can use our PE library, but for now we directly reimplement
  // EfiGetAPFSDriverVersion from apfs kernel extension.
  // Note, EfiGetAPFSDriverVersion is really badly implemented and is full of typos.
  //

  EFI_IMAGE_DOS_HEADER      *DosHeader;
  EFI_IMAGE_NT_HEADERS64    *NtHeaders;
  EFI_IMAGE_SECTION_HEADER  *SectionHeader;
  APFS_DRIVER_VERSION       *DriverVersion;
  UINTN                     RemainingSize;
  UINTN                     Result;
  UINT32                    ImageVersion;

  if (DriverSize < sizeof (*DosHeader)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  DosHeader = DriverBuffer;

  if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }

  if (OcOverflowAddUN (DosHeader->e_lfanew, sizeof (*NtHeaders), &Result)
    || Result > DriverSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  RemainingSize = DriverSize - Result - OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader);
  NtHeaders     = (VOID *) ((UINT8 *) DriverBuffer + DosHeader->e_lfanew);

  if (NtHeaders->Signature != EFI_IMAGE_NT_SIGNATURE
    || NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_X64
    || (NtHeaders->FileHeader.Characteristics & EFI_IMAGE_FILE_EXECUTABLE_IMAGE) == 0
    || NtHeaders->FileHeader.SizeOfOptionalHeader < sizeof (NtHeaders->OptionalHeader)
    || NtHeaders->FileHeader.NumberOfSections == 0) {
    return EFI_UNSUPPORTED;
  }

  if (RemainingSize < NtHeaders->FileHeader.SizeOfOptionalHeader) {
    return EFI_BUFFER_TOO_SMALL;
  }

  RemainingSize -= NtHeaders->FileHeader.SizeOfOptionalHeader;

  if ((NtHeaders->OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
      && NtHeaders->OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    || NtHeaders->OptionalHeader.Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER) {
    return EFI_UNSUPPORTED;
  }

  ImageVersion = (UINT32) NtHeaders->OptionalHeader.MajorImageVersion << 16
    | (UINT32) NtHeaders->OptionalHeader.MinorImageVersion;

  if (OcOverflowMulUN (NtHeaders->FileHeader.NumberOfSections, sizeof (*SectionHeader), &Result)
    || Result > RemainingSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  SectionHeader = (VOID *) ((UINT8 *) &NtHeaders->OptionalHeader + NtHeaders->FileHeader.SizeOfOptionalHeader);

  if (AsciiStrnCmp ((CHAR8*) SectionHeader->Name, ".text", sizeof (SectionHeader->Name)) != 0) {
    return EFI_UNSUPPORTED;
  }

  if (OcOverflowAddUN (SectionHeader->VirtualAddress, sizeof (APFS_DRIVER_VERSION), &Result)
    || RemainingSize < Result) {
    return EFI_BUFFER_TOO_SMALL;
  }

  DriverVersion = (VOID *) ((UINT8 *) DriverBuffer + SectionHeader->VirtualAddress);

  if (DriverVersion->Magic != APFS_DRIVER_VERSION_MAGIC
    || DriverVersion->ImageVersion != ImageVersion) {
    return EFI_INVALID_PARAMETER;
  }

  *DriverVersionPtr = DriverVersion;
  return EFI_SUCCESS;
}
