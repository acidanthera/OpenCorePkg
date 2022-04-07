/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

#define IS_COMPRESSED_BLOCK 0x8000
#define BLOCK_LENGTH_BITS   0xFFF
#define UNIT_MASK           0xF

extern UINTN  mFileRecordSize;
extern UINTN  mSectorSize;
extern UINTN  mClusterSize;
extern UINT64 BufferSize;
extern UINT64 mUnitSize;

STATIC
EFI_STATUS
GetNextCluster (
  IN OUT COMPRESSED *Clusters
  )
{
  EFI_STATUS   Status;

  if (Clusters->Head >= Clusters->Tail) {
    DEBUG ((DEBUG_INFO, "NTFS: Compression block overflown\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Status = DiskRead (
    Clusters->FileSystem,
    (Clusters->Elements[Clusters->Head].Lcn - Clusters->Elements[Clusters->Head].Vcn + Clusters->CurrentVcn) * mClusterSize,
    mClusterSize,
    Clusters->Cluster
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ++Clusters->CurrentVcn;

  if (Clusters->CurrentVcn >= Clusters->Elements[Clusters->Head].Vcn) {
    ++Clusters->Head;
  }

  Clusters->ClusterOffset = 0;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetDataRunByte (
  IN  COMPRESSED *Clusters,
  OUT UINT8      *Result
  )
{
  EFI_STATUS   Status;

  ASSERT (Clusters != NULL);
  ASSERT (Result   != NULL);

  if (Clusters->ClusterOffset >= mClusterSize) {
    Status = GetNextCluster (Clusters);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *Result = Clusters->Cluster[Clusters->ClusterOffset++];

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetTwoDataRunBytes (
  IN  COMPRESSED *Clusters,
  OUT UINT16     *Result
  )
{
  EFI_STATUS   Status;
  UINT8        ByteLow;
  UINT8        ByteHigh;

  ASSERT (Clusters != NULL);
  ASSERT (Result   != NULL);

  Status = GetDataRunByte (Clusters, &ByteLow);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetDataRunByte (Clusters, &ByteHigh);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Result = (((UINT16) ByteHigh) << 8U) + ByteLow;

  return EFI_SUCCESS;
}

/**
  * The basic idea is that substrings of the block which have been seen before
    are compressed by referencing the string rather than mentioning it again.

    #include <ntfs.h>\n
    #include <stdio.h>\n

    This is compressed to #include <ntfs.h>\n(-18,10)stdio(-17,4)

  * Pairs like (-18,10) are recorded in two bytes. ->
    The shortest possible substring is 3 bytes long. ->
    One can subtract 3 from the length before encoding it.

  * The references are always backward, and never 0. ->
    One can store them as positive numbers, and subtract one. ->
    (-18,10) -> (17,7); (-17,4) -> (16,1).

  * Given that a block is 4096 in size, you might need 12 bits to encode the back reference.
    This means that you have only 4 bits left to encode the length.

  * Dynamic allocation of bits for the back-reference.

    for (i = clear_pos - 1, lmask = 0xFFF, dshift = 12; i >= 0x10; i >>= 1) {
      lmask >>= 1; // bit mask for length
      dshift--;    // shift width for delta
     }

  * Now that we can encode a (offset,length) pair as 2 bytes,
    we still have to know whether a token is back-reference, or plain-text.
    This is 1 bit per token. 8 tokens are grouped together
    and preceded with the tags byte.

    ">\n(-18,10)stdio" would be encoded as "00000100"
    (the 1 bit indicates the back reference).

    "00000000"#include"00000000" <ntfs.h"00000100">\n(17,7)stdio"00000001"(16,1)

  * As a compression unit consists of 16 clusters (default), it usually contains more than one of these blocks.
    If you want to access the second block, it would be a waste of time to decompress the first one.
    Instead, each block is preceded by a 2-byte length.
    The lower 12 bits are the length, the higher 4 bits are of unknown purpose.
    Actually, (n-1) is stored in the low 12 bits.

  * The compression method is based on independently compressing blocks of X clusters,
    where X = 2 ^ ATTR_HEADER_NONRES.CompressionUnitSize.

  * If the block grows in size, it will be stored uncompressed.
    A length of exactly 4095 is used to indicate this case.

  * Bit 0x8000 is the flag specifying that the block is compressed.
**/
STATIC
EFI_STATUS
DecompressBlock (
  IN  COMPRESSED *Clusters,
  OUT UINT8      *Dest       OPTIONAL
  )
{
  EFI_STATUS   Status;
  UINT16       BlockParameters;
  UINTN        BlockLength;
  UINT8        TagsByte;
  UINT8        Tokens;
  UINT16       ClearTextPointer;
  UINT8        PlainText;
  UINT16       Index;
  UINT16       Length;
  UINT16       Lmask;
  UINT16       Delta;
  UINT16       Dshift;
  UINT16       BackReference;
  UINTN        SpareBytes;

  ASSERT (Clusters != NULL);

  Status = GetTwoDataRunBytes (Clusters, &BlockParameters);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BlockLength = (BlockParameters & BLOCK_LENGTH_BITS) + 1;

  if (Dest != NULL) {
    if ((BlockParameters & IS_COMPRESSED_BLOCK) != 0) {
      ClearTextPointer = 0;
      Tokens           = 0;
      TagsByte         = 0;
      while (BlockLength > 0) {
        if (ClearTextPointer > COMPRESSION_BLOCK) {
          DEBUG ((DEBUG_INFO, "NTFS: Compression block too large\n"));
          return EFI_VOLUME_CORRUPTED;
        }

        if (Tokens == 0) {
          Status = GetDataRunByte (Clusters, &TagsByte);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          Tokens = 8;
          --BlockLength;
          if (BlockLength == 0) {
            break;
          }
        }

        if ((TagsByte & 1U) == 1) {
          //
          // Back-reference
          //
          Status = GetTwoDataRunBytes (Clusters, &BackReference);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          BlockLength -= 2;

          if (ClearTextPointer == 0) {
            DEBUG ((DEBUG_INFO, "NTFS: Nontext window empty\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          Lmask = BLOCK_LENGTH_BITS;
          Dshift = 12;
          for (Index = ClearTextPointer - 1; Index >= 0x10; Index >>= 1) {
            Lmask >>= 1U;
            --Dshift;
          }

          Delta = BackReference >> Dshift;
          Length = (BackReference & Lmask) + 3;

          if ((Delta > (ClearTextPointer - 1)) || (Length >= COMPRESSION_BLOCK)) {
            DEBUG ((DEBUG_INFO, "NTFS: Invalid back-reference.\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          if (BufferSize < Length) {
            DEBUG ((DEBUG_INFO, "NTFS: (DecompressBlock #1) Buffer overflow.\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          for (Index = 0; Index < Length; ++Index) {
            Dest[ClearTextPointer] = Dest[ClearTextPointer - Delta - 1];
            ++ClearTextPointer;
          }

          BufferSize -= Length;
        } else {
          //
          // Plain text
          //
          Status = GetDataRunByte (Clusters, &PlainText);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          if (BufferSize == 0) {
            DEBUG ((DEBUG_INFO, "NTFS: (DecompressBlock #2) Buffer overflow.\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          Dest[ClearTextPointer++] = PlainText;

          --BlockLength;
          --BufferSize;
        }

        TagsByte >>= 1U;
        --Tokens;
      }

      return EFI_SUCCESS;
    }

    if (BlockLength != COMPRESSION_BLOCK) {
      DEBUG ((DEBUG_INFO, "NTFS: Invalid compression block size %d\n", BlockLength));
      return EFI_VOLUME_CORRUPTED;
    }
  }

  while (BlockLength > 0) {
    SpareBytes = mClusterSize - Clusters->ClusterOffset;
    if (SpareBytes > BlockLength) {
      SpareBytes = BlockLength;
    }

    if ((Dest != NULL) && (SpareBytes != 0)) {
      if (BufferSize < SpareBytes) {
        DEBUG ((DEBUG_INFO, "NTFS: (DecompressBlock #3) Buffer overflow.\n"));
        return EFI_VOLUME_CORRUPTED;
      }

      CopyMem (Dest, &Clusters->Cluster[Clusters->ClusterOffset], SpareBytes);
      Dest += SpareBytes;
      BufferSize -= SpareBytes;
    }

    BlockLength -= SpareBytes;
    Clusters->ClusterOffset += SpareBytes;
    if (BlockLength != 0) {
      Status = GetNextCluster (Clusters);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  * The set of VCNs containing the stream of a compressed file attribute is divided
    in compression units (also called chunks) of 16 cluster.

  * The alpha stage: if all the 16 clusters of a compression unit are full of zeroes,
    this compression unit is called a Sparse unit and is not physically stored.
    Instead, an element with no Offset field (F=0, the Offset is assumed to be 0 too)
    and a Length of 16 clusters is put in the Runlist.

  * The beta stage: if the compression of the unit is possible,
    N (< 16) clusters are physically stored, and an element with a Length of N
    is put in the Runlist, followed by another element with no Offset field
    (F=0, the Offset is assumed to be 0 too) and a Length of 16 - N.

  * If the unit is not compressed, 16 clusters are physically stored,
    and an element with a Length of 16 is put in the Runlist.

Example

Runlist:
    21 14 00 01 11 10 18 11 05 15 01 27 11 20 05

Decode
    0x14   at   0x100
    0x10   at  + 0x18
    0x05   at  + 0x15
    0x27   at  + none
    0x20   at  + 0x05

Absolute LCNs
    0x14   at   0x100
    0x10   at   0x118
    0x05   at   0x12D
    0x27   at   none
    0x20   at   0x132

Regroup
    0x10   at   0x100   Unit not compressed

    0x04   at   0x110   Unit not compressed
    0x0C   at   0x118

    0x04   at   0x118   Compressed unit
    0x05   at   0x12D
    0x07   at   none

    0x10   at   none    Sparse unit

    0x10   at   none    Sparse unit

    0x10   at   0x132   Unit not compressed

    0x10   at   0x142   Unit not compressed
**/
STATIC
EFI_STATUS
ReadCompressedBlock (
  IN  RUNLIST *Runlist,
  OUT UINT8   *Buffer      OPTIONAL,
  IN  UINTN   BlocksTotal
  )
{
  EFI_STATUS   Status;
  UINTN        SpareBlocks;
  UINT64       SpareClusters;
  UINT64       BlocksPerCluster;
  UINT64       ClustersPerBlock;
  UINT64       ClearTextClusters;

  ASSERT (Runlist != NULL);

  BlocksPerCluster = 0;
  ClustersPerBlock = 0;

  BufferSize = BlocksTotal * COMPRESSION_BLOCK;

  if (mClusterSize >= COMPRESSION_BLOCK) {
    BlocksPerCluster = DivU64x64Remainder (mClusterSize, COMPRESSION_BLOCK, NULL);
  } else {
    ClustersPerBlock = DivU64x64Remainder (COMPRESSION_BLOCK, mClusterSize, NULL);
  }

  while (BlocksTotal != 0) {
    if ((Runlist->TargetVcn & UNIT_MASK) == 0)  {
      if ((Runlist->Unit.Head != Runlist->Unit.Tail) && (Runlist->IsSparse == FALSE)) {
        DEBUG ((DEBUG_INFO, "NTFS: Invalid compression block\n"));
        return EFI_VOLUME_CORRUPTED;
      }

      Runlist->Unit.Head = Runlist->Unit.Tail = 0;
      Runlist->Unit.CurrentVcn = Runlist->TargetVcn;
      Runlist->Unit.ClusterOffset = mClusterSize;
      if (Runlist->TargetVcn >= Runlist->NextVcn) {
        Status = ReadRunListElement (Runlist);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      while ((Runlist->TargetVcn + mUnitSize) > Runlist->NextVcn) {
        if (Runlist->IsSparse) {
          break;
        }

        Runlist->Unit.Elements[Runlist->Unit.Tail].Vcn = Runlist->NextVcn;
        Runlist->Unit.Elements[Runlist->Unit.Tail].Lcn = Runlist->CurrentLcn + Runlist->NextVcn - Runlist->CurrentVcn;
        ++Runlist->Unit.Tail;

        Status = ReadRunListElement (Runlist);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
    }

    if (mClusterSize >= COMPRESSION_BLOCK) {
      SpareBlocks = (UINTN) ((mUnitSize - (Runlist->TargetVcn & UNIT_MASK)) * BlocksPerCluster);
    } else {
      SpareBlocks = (UINTN) DivU64x64Remainder (mUnitSize - (Runlist->TargetVcn & UNIT_MASK), ClustersPerBlock, NULL);
    }

    if (SpareBlocks > BlocksTotal) {
      SpareBlocks = BlocksTotal;
    }
    BlocksTotal -= SpareBlocks;

    if (Runlist->IsSparse) {
      if (mClusterSize >= COMPRESSION_BLOCK) {
        Runlist->TargetVcn += DivU64x64Remainder (SpareBlocks, BlocksPerCluster, NULL);
      } else {
        Runlist->TargetVcn += SpareBlocks * ClustersPerBlock;
      }

      if (Runlist->Unit.Tail == 0) {
        if (Buffer != NULL) {
          if (BufferSize < (SpareBlocks * COMPRESSION_BLOCK)) {
            DEBUG ((DEBUG_INFO, "NTFS: (ReadCompressedBlock #1) Buffer overflow.\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          ZeroMem (Buffer, SpareBlocks * COMPRESSION_BLOCK);
          Buffer += SpareBlocks * COMPRESSION_BLOCK;
          BufferSize -= SpareBlocks * COMPRESSION_BLOCK;
        }
      } else {
        while (SpareBlocks != 0) {
          Status = DecompressBlock (&Runlist->Unit, Buffer);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          if (Buffer != NULL) {
            Buffer += COMPRESSION_BLOCK;
          }

          --SpareBlocks;
        }
      }
    } else {
      if (mClusterSize >= COMPRESSION_BLOCK) {
        SpareClusters = DivU64x64Remainder (SpareBlocks, BlocksPerCluster, NULL);
      } else {
        SpareClusters = SpareBlocks * ClustersPerBlock;
      }

      while ((Runlist->Unit.Head < Runlist->Unit.Tail) && (SpareClusters != 0)) {
        ClearTextClusters = Runlist->Unit.Elements[Runlist->Unit.Head].Vcn - Runlist->TargetVcn;
        if (ClearTextClusters > SpareClusters) {
          ClearTextClusters = SpareClusters;
        }

        Runlist->TargetVcn += ClearTextClusters;
        if (Buffer != NULL) {
          if (BufferSize < (ClearTextClusters * mClusterSize)) {
            DEBUG ((DEBUG_INFO, "NTFS: (ReadCompressedBlock #2) Buffer overflow.\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          Status = DiskRead (
            Runlist->Unit.FileSystem,
            Runlist->Unit.Elements[Runlist->Unit.Head].Lcn * mClusterSize,
            (UINTN) (ClearTextClusters * mClusterSize),
            Buffer
            );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          Buffer += ClearTextClusters * mClusterSize;
          BufferSize -= ClearTextClusters * mClusterSize;
        }

        SpareClusters -= ClearTextClusters;
        ++Runlist->Unit.Head;
      }

      if (SpareClusters != 0) {
        if (Buffer != NULL) {
          if (BufferSize < (SpareClusters * mClusterSize)) {
            DEBUG ((DEBUG_INFO, "NTFS: (ReadCompressedBlock #3) Buffer overflow.\n"));
            return EFI_VOLUME_CORRUPTED;
          }

          Status = DiskRead (
            Runlist->Unit.FileSystem,
            (Runlist->TargetVcn - Runlist->CurrentVcn + Runlist->CurrentLcn) * mClusterSize,
            (UINTN) (SpareClusters * mClusterSize),
            Buffer
            );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          Buffer += SpareClusters * mClusterSize;
          BufferSize -= SpareClusters * mClusterSize;
        }
        Runlist->TargetVcn += SpareClusters;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
Decompress (
  IN  RUNLIST *Runlist,
  IN  UINT64  Offset,
  IN  UINTN   Length,
  OUT UINT8   *Dest
  )
{
  EFI_STATUS   Status;
  UINT64       Vcn;
  UINT64       Target;
  UINTN        SpareBytes;
  UINTN        Residual;
  UINT64       BlocksPerCluster;
  UINT64       ClustersPerBlock;

  ASSERT (Runlist != NULL);
  ASSERT (Dest    != NULL);

  if (Runlist->Unit.ClearTextBlock != NULL) {
    if ((Offset & (~(COMPRESSION_BLOCK - 1U))) == Runlist->Unit.SavedPosition) {
      Residual = (UINTN) (COMPRESSION_BLOCK - (Offset - Runlist->Unit.SavedPosition));
      if (Residual > Length) {
        Residual = Length;
      }

      CopyMem (Dest, Runlist->Unit.ClearTextBlock + Offset - Runlist->Unit.SavedPosition, Residual);
      if (Residual == Length) {
        return EFI_SUCCESS;
      }

      Dest += Residual;
      Length -= Residual;
      Offset += Residual;
    }
  } else {
    Runlist->Unit.ClearTextBlock = AllocateZeroPool (COMPRESSION_BLOCK);
    if (Runlist->Unit.ClearTextBlock == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Runlist->Unit.SavedPosition = 1;
  }

  Vcn = Runlist->TargetVcn;
  Runlist->TargetVcn &= ~(mUnitSize - 1);
  while (Runlist->NextVcn <= Runlist->TargetVcn) {
    Status = ReadRunListElement (Runlist);
    if (EFI_ERROR (Status)) {
      FreePool (Runlist->Unit.ClearTextBlock);
      return Status;
    }
  }

  Runlist->Unit.Head = Runlist->Unit.Tail = 0;
  Runlist->Unit.Cluster = AllocateZeroPool (mClusterSize);
  if (Runlist->Unit.Cluster == NULL) {
    FreePool (Runlist->Unit.ClearTextBlock);
    return EFI_OUT_OF_RESOURCES;
  }

  if (Vcn > Runlist->TargetVcn) {
    if (mClusterSize >= COMPRESSION_BLOCK) {
      BlocksPerCluster = DivU64x64Remainder (mClusterSize, COMPRESSION_BLOCK, NULL);
      Status = ReadCompressedBlock (
        Runlist,
        NULL,
        (UINTN) ((Vcn - Runlist->TargetVcn) * BlocksPerCluster)
        );
    } else {
      ClustersPerBlock = DivU64x64Remainder (COMPRESSION_BLOCK, mClusterSize, NULL);
      Status = ReadCompressedBlock (
        Runlist,
        NULL,
        (UINTN) DivU64x64Remainder (Vcn - Runlist->TargetVcn, ClustersPerBlock, NULL)
        );
    }

    if (EFI_ERROR (Status)) {
      FreePool (Runlist->Unit.ClearTextBlock);
      FreePool (Runlist->Unit.Cluster);
      return Status;
    }
  }

  if ((Offset % COMPRESSION_BLOCK) != 0) {
    Target = Runlist->TargetVcn * mClusterSize;

    Status = ReadCompressedBlock (Runlist, Runlist->Unit.ClearTextBlock, 1);
    if (EFI_ERROR (Status)) {
      FreePool (Runlist->Unit.ClearTextBlock);
      FreePool (Runlist->Unit.Cluster);
      return Status;
    }

    Runlist->Unit.SavedPosition = Target;

    Residual = (UINTN) (Offset % COMPRESSION_BLOCK);
    SpareBytes = COMPRESSION_BLOCK - Residual;
    if (SpareBytes > Length) {
      SpareBytes = Length;
    }

    CopyMem (Dest, &Runlist->Unit.ClearTextBlock[Residual], SpareBytes);
    if (SpareBytes == Length) {
      FreePool (Runlist->Unit.ClearTextBlock);
      FreePool (Runlist->Unit.Cluster);
      return EFI_SUCCESS;
    }

    Dest += SpareBytes;
    Length -= SpareBytes;
  }

  Status = ReadCompressedBlock (Runlist, Dest, Length / COMPRESSION_BLOCK);
  if (EFI_ERROR (Status)) {
    FreePool (Runlist->Unit.ClearTextBlock);
    FreePool (Runlist->Unit.Cluster);
    return Status;
  }

  Dest += (Length / COMPRESSION_BLOCK) * COMPRESSION_BLOCK;
  Length %= COMPRESSION_BLOCK;
  if (Length != 0) {
    Target = Runlist->TargetVcn * mClusterSize;

    Status = ReadCompressedBlock (Runlist, Runlist->Unit.ClearTextBlock, 1);
    if (EFI_ERROR (Status)) {
      FreePool (Runlist->Unit.Cluster);
      return Status;
    }

    Runlist->Unit.SavedPosition = Target;

    CopyMem (Dest, Runlist->Unit.ClearTextBlock, Length);
  }

  FreePool (Runlist->Unit.ClearTextBlock);
  FreePool (Runlist->Unit.Cluster);

  return EFI_SUCCESS;
}
