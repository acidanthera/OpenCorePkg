/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/AppleCompressedBinaryImage.h>
#include <IndustryStandard/AppleFatBinaryImage.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>

//
// Pick a reasonable maximum to fit.
//
#define KERNEL_HEADER_SIZE (EFI_PAGE_SIZE*2)

STATIC SHA384_CONTEXT mKernelDigestContext;
STATIC UINT32         mKernelDigestPosition;
STATIC BOOLEAN        mNeedKernelDigest;

STATIC
EFI_STATUS
ReplaceBuffer (
  IN     UINT32  TargetSize,
  IN OUT UINT8   **Buffer,
     OUT UINT32  *AllocatedSize,
  IN     UINT32  ReservedSize
  )
{
  UINT8  *TmpBuffer;

  if (OcOverflowAddU32 (TargetSize, ReservedSize, &TargetSize)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*AllocatedSize >= TargetSize) {
    return EFI_SUCCESS;
  }

  TmpBuffer = AllocatePool (TargetSize);
  if (TmpBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FreePool (*Buffer);
  *Buffer = TmpBuffer;
  *AllocatedSize = TargetSize;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
KernelGetFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      RemainingSize;
  UINT32      ChunkSize;

  //
  // Calculate hash for the prefix.
  //
  if (mNeedKernelDigest && Position > mKernelDigestPosition) {
    RemainingSize = Position - mKernelDigestPosition;

    while (RemainingSize > 0) {
      ChunkSize = MIN (RemainingSize, Size);
      Status = GetFileData (
        File,
        mKernelDigestPosition,
        ChunkSize,
        Buffer
        );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Sha384Update (&mKernelDigestContext, Buffer, ChunkSize);
      mKernelDigestPosition += ChunkSize;
      RemainingSize -= ChunkSize;
    }
  }

  Status = GetFileData (
    File,
    Position,
    Size,
    Buffer
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Calculate hash for the suffix.
  //
  if (mNeedKernelDigest && Position >= mKernelDigestPosition) {
    RemainingSize = Position + Size - mKernelDigestPosition;
    Sha384Update (
      &mKernelDigestContext,
      Buffer + (Position - mKernelDigestPosition),
      RemainingSize
      );
    mKernelDigestPosition += RemainingSize;
  }

  return Status;
}

STATIC
UINT32
ParseFatArchitecture (
  IN OUT UINT8             **Buffer,
  IN OUT UINT32            *Offset
  )
{
  BOOLEAN           SwapBytes;
  MACH_FAT_HEADER   *FatHeader;
  UINT32            NumberOfFatArch;
  MACH_CPU_TYPE     CpuType;
  UINT32            TmpSize;
  UINT32            Index;
  UINT32            Size;

  FatHeader       = (MACH_FAT_HEADER *)*Buffer;
  SwapBytes       = FatHeader->Signature == MACH_FAT_BINARY_INVERT_SIGNATURE;
  NumberOfFatArch = FatHeader->NumberOfFatArch;
  if (SwapBytes) {
    NumberOfFatArch = SwapBytes32 (NumberOfFatArch);
  }

  if (OcOverflowMulAddU32 (NumberOfFatArch, sizeof (MACH_FAT_ARCH), sizeof (MACH_FAT_HEADER), &TmpSize)
    || TmpSize > KERNEL_HEADER_SIZE) {
    DEBUG ((DEBUG_INFO, "OCAK: Fat kernel invalid arch count %u\n", NumberOfFatArch));
    return 0;
  }

  //
  // TODO: Currently there are no kernels with MachCpuSubtypeX8664H, but we should support them. 
  //
  for (Index = 0; Index < NumberOfFatArch; Index++) {
    CpuType = FatHeader->FatArch[Index].CpuType;
    if (SwapBytes) {
      CpuType = SwapBytes32 (CpuType);
    }
    if (CpuType == MachCpuTypeX8664) {
      *Offset = FatHeader->FatArch[Index].Offset;
      Size   = FatHeader->FatArch[Index].Size;
      if (SwapBytes) {
        *Offset = SwapBytes32 (*Offset);
        Size    = SwapBytes32 (Size);
      }

      if (*Offset == 0) {
        DEBUG ((DEBUG_INFO, "OCAK: Fat kernel has 0 offset\n"));
        return 0;
      }

      if (OcOverflowAddU32 (*Offset, Size, &TmpSize)) {
        DEBUG ((DEBUG_INFO, "OCAK: Fat kernel invalid size %u\n", Size));
        return 0;
      }

      return Size;
    }
  }

  DEBUG ((DEBUG_INFO, "OCAK: Fat kernel has no x86_64 arch\n"));
  return 0;
}

STATIC
UINT32
ParseCompressedHeader (
  IN     EFI_FILE_PROTOCOL  *File,
  IN OUT UINT8              **Buffer,
  IN     UINT32             Offset,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize
  )
{
  EFI_STATUS        Status;

  UINT32            KernelSize;
  MACH_COMP_HEADER  *CompHeader;
  UINT8             *CompressedBuffer;
  UINT32            CompressionType;
  UINT32            CompressedSize;
  UINT32            DecompressedSize;
  UINT32            DecompressedHash;

  CompHeader       = (MACH_COMP_HEADER *)*Buffer;
  CompressionType  = CompHeader->Compression;
  CompressedSize   = SwapBytes32 (CompHeader->Compressed);
  DecompressedSize = SwapBytes32 (CompHeader->Decompressed);
  DecompressedHash = SwapBytes32 (CompHeader->Hash);

  KernelSize = 0;

  if (CompressedSize > OC_COMPRESSION_MAX_LENGTH
    || CompressedSize == 0
    || DecompressedSize > OC_COMPRESSION_MAX_LENGTH
    || DecompressedSize < KERNEL_HEADER_SIZE) {
    DEBUG ((DEBUG_INFO, "OCAK: Comp kernel invalid comp %u or decomp %u at %08X\n", CompressedSize, DecompressedSize, Offset));
    return KernelSize;
  }

  Status = ReplaceBuffer (DecompressedSize, Buffer, AllocatedSize, ReservedSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Decomp kernel (%u bytes) cannot be allocated at %08X\n", DecompressedSize, Offset));
    return KernelSize;
  }

  CompressedBuffer = AllocatePool (CompressedSize);
  if (CompressedBuffer == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: Comp kernel (%u bytes) cannot be allocated at %08X\n", CompressedSize, Offset));
    return KernelSize;
  }

  Status = KernelGetFileData (File, Offset + sizeof (MACH_COMP_HEADER), CompressedSize, CompressedBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Comp kernel (%u bytes) cannot be read at %08X\n", CompressedSize, Offset));
    FreePool (CompressedBuffer);
    return KernelSize;
  }

  if (CompressionType == MACH_COMPRESSED_BINARY_INVERT_LZVN) {
    KernelSize = (UINT32)DecompressLZVN (*Buffer, DecompressedSize, CompressedBuffer, CompressedSize);
  } else if (CompressionType == MACH_COMPRESSED_BINARY_INVERT_LZSS) {
    KernelSize = (UINT32)DecompressLZSS (*Buffer, DecompressedSize, CompressedBuffer, CompressedSize);
  }

  if (KernelSize != DecompressedSize) {
    KernelSize = 0;
  }

  //
  // TODO: implement adler32 hash verification.
  //
  (VOID) DecompressedHash;

  FreePool (CompressedBuffer);

  return KernelSize;
}

STATIC
EFI_STATUS
ReadAppleKernelImage (
  IN     EFI_FILE_PROTOCOL  *File,
  IN OUT UINT8              **Buffer,
     OUT UINT32             *KernelSize,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize,
  IN     UINT32             Offset
  )
{
  EFI_STATUS        Status;
  UINT32            *MagicPtr;
  BOOLEAN           ForbidFat;
  BOOLEAN           Compressed;

  Status = KernelGetFileData (File, Offset, KERNEL_HEADER_SIZE, *Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Do not allow FAT architectures with Offset > 0 (recursion).
  //
  ForbidFat  = Offset > 0;
  Compressed = FALSE;

  while (TRUE) {
    if (!OC_TYPE_ALIGNED (UINT32 , *Buffer)) {
      DEBUG ((DEBUG_INFO, "OCAK: Misaligned kernel header %p at %08X\n", *Buffer, Offset));
      return EFI_INVALID_PARAMETER;
    }
    MagicPtr = (UINT32 *)* Buffer;

    switch (*MagicPtr) {
      case MACH_HEADER_64_SIGNATURE:
        DEBUG ((DEBUG_VERBOSE, "OCAK: Found Mach-O compressed %d offset %u size %u\n", Compressed, Offset, *KernelSize));

        //
        // This is just a valid (formerly) compressed image.
        //
        if (Compressed) {
          return EFI_SUCCESS;
        }

        //
        // This is an uncompressed image, just fully read it.
        //

        if (Offset == 0) {
          //
          // Figure out size for a non fat image.
          //
          Status = GetFileSize (File, KernelSize);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "OCAK: Kernel size cannot be determined - %r\n", Status));
            return EFI_OUT_OF_RESOURCES;
          }

          DEBUG ((DEBUG_VERBOSE, "OCAK: Determined kernel size is %u bytes\n", *KernelSize));
        }

        Status = ReplaceBuffer (*KernelSize, Buffer, AllocatedSize, ReservedSize);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCAK: Kernel (%u bytes) cannot be allocated at %08X\n", *KernelSize, Offset));
          return Status;
        }

        Status = KernelGetFileData (File, Offset, *KernelSize, *Buffer);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCAK: Kernel (%u bytes) cannot be read at %08X\n", *KernelSize, Offset));
        }

        return Status;
      case MACH_FAT_BINARY_SIGNATURE:
      case MACH_FAT_BINARY_INVERT_SIGNATURE:
      {
        if (ForbidFat) {
          DEBUG ((DEBUG_INFO, "Fat kernel recursion %p at %08X\n", MagicPtr, Offset));
          return EFI_INVALID_PARAMETER;
        }

        *KernelSize = ParseFatArchitecture (Buffer, &Offset);
        if (*KernelSize != 0) {
          return ReadAppleKernelImage (File, Buffer, KernelSize, AllocatedSize, ReservedSize, Offset);
        }
        return EFI_INVALID_PARAMETER;
      }
      case MACH_COMPRESSED_BINARY_INVERT_SIGNATURE:
      {
        if (Compressed) {
          DEBUG ((DEBUG_INFO, "Compression recursion %p at %08X\n", MagicPtr, Offset));
          return EFI_INVALID_PARAMETER;
        }

        //
        // No FAT or Comp is allowed after compressed.
        //
        ForbidFat = Compressed = TRUE;

        //
        // Loop into updated image in Buffer.
        //
        *KernelSize = ParseCompressedHeader (File, Buffer, Offset, AllocatedSize, ReservedSize);
        if (*KernelSize != 0) {
          DEBUG ((DEBUG_VERBOSE, "OCAK: Compressed result has %08X magic\n", *(UINT32 *) Buffer));
          continue;
        }
        return EFI_INVALID_PARAMETER;
      }
      default:
        DEBUG ((Offset > 0 ? DEBUG_INFO : DEBUG_VERBOSE, "OCAK: Invalid kernel magic %08X at %08X\n", *MagicPtr, Offset));
        return EFI_INVALID_PARAMETER;
    }
  }
}

EFI_STATUS
ReadAppleKernel (
  IN     EFI_FILE_PROTOCOL  *File,
     OUT UINT8              **Kernel,
     OUT UINT32             *KernelSize,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize,
     OUT UINT8              *Digest  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      FullSize;
  UINT8       *Remainder;

  *KernelSize    = 0;
  *AllocatedSize = KERNEL_HEADER_SIZE;
  *Kernel        = AllocatePool (*AllocatedSize);

  if (*Kernel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  mNeedKernelDigest = Digest != NULL;
  if (mNeedKernelDigest) {
    mKernelDigestPosition = 0;
    Sha384Init (&mKernelDigestContext);
  }

  Status = ReadAppleKernelImage (
    File,
    Kernel,
    KernelSize,
    AllocatedSize,
    ReservedSize,
    0
    );

  if (EFI_ERROR (Status)) {
    FreePool (*Kernel);
    mNeedKernelDigest = FALSE;
    return Status;
  }

  if (mNeedKernelDigest) {
    Status = GetFileSize (File, &FullSize);
    if (EFI_ERROR (Status)) {
      mNeedKernelDigest = FALSE;
      return Status;
    }

    if (FullSize > mKernelDigestPosition) {
      Remainder = AllocatePool (FullSize - mKernelDigestPosition);
      if (Remainder == NULL) {
        mNeedKernelDigest = FALSE;
        return EFI_OUT_OF_RESOURCES;
      }

      Status = KernelGetFileData (
        File,
        mKernelDigestPosition,
        FullSize - mKernelDigestPosition,
        Remainder
        );
      mNeedKernelDigest = FALSE;
      FreePool (Remainder);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ASSERT (FullSize == mKernelDigestPosition);
    }

    Sha384Final (&mKernelDigestContext, Digest);
  }

  return EFI_SUCCESS;
}
