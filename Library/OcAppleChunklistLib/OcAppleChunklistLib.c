/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleRamDiskLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcGuardLib.h>

BOOLEAN
OcAppleChunklistInitializeContext (
     OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN OUT VOID                        *Buffer,
  IN     UINT32                      BufferSize
  )
{
  APPLE_CHUNKLIST_HEADER  *ChunklistHeader;
  UINTN                   DataEnd;
  UINT8                   *Signature;
  UINT32                  SigLength;
  UINT32                  Index;
  UINT8                   SwapValue;

  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);
  ASSERT (Context != NULL);

  ChunklistHeader = (APPLE_CHUNKLIST_HEADER *) Buffer;

  if (BufferSize < sizeof (APPLE_CHUNKLIST_HEADER)) {
    return FALSE;
  }

  //
  // Ensure the header is compatible.
  //
  if (ChunklistHeader->Magic != APPLE_CHUNKLIST_MAGIC
    || ChunklistHeader->Length != sizeof (APPLE_CHUNKLIST_HEADER)
    || ChunklistHeader->FileVersion != APPLE_CHUNKLIST_FILE_VERSION_10
    || ChunklistHeader->ChunkMethod != APPLE_CHUNKLIST_CHUNK_METHOD_10
    || ChunklistHeader->SigMethod != APPLE_CHUNKLIST_SIG_METHOD_10) {
    return FALSE;
  }

  //
  // Ensure that chunk and signature addresses are valid in the first place.
  //
  if (OcOverflowAddUN ((UINTN) Buffer, (UINTN) ChunklistHeader->ChunkOffset, (UINTN *) &Context->Chunks)
    || OcOverflowAddUN ((UINTN) Buffer, (UINTN) ChunklistHeader->SigOffset, (UINTN *) &Context->Signature)) {
    return FALSE;
  }

  //
  // Ensure that chunks and signature reside within Buffer.
  //
  if (ChunklistHeader->ChunkCount > MAX_UINTN) {
    return FALSE;
  }

  Context->ChunkCount = (UINTN)ChunklistHeader->ChunkCount;

  if (OcOverflowMulAddUN (sizeof (APPLE_CHUNKLIST_CHUNK), Context->ChunkCount, (UINTN) Context->Chunks, &DataEnd)
    || DataEnd > (UINTN) Buffer + BufferSize
    || OcOverflowAddUN (sizeof (APPLE_CHUNKLIST_SIG), (UINTN) Context->Signature, &DataEnd)
    || DataEnd != (UINTN) Buffer + BufferSize) {
    return FALSE;
  }

  //
  // Prepare signature verification data.
  //
  Sha256 (Context->Hash, (UINT8 *)ChunklistHeader, (UINTN)ChunklistHeader->SigOffset);

  Signature = Context->Signature->Signature;
  SigLength = sizeof (Context->Signature->Signature);

  for (Index = 0; Index < (SigLength / 2); ++Index) {
    SwapValue                        = Signature[Index];
    Signature[Index]                 = Signature[SigLength - Index - 1];
    Signature[SigLength - Index - 1] = SwapValue;
  }

  return TRUE;
}

BOOLEAN
OcAppleChunklistVerifySignature (
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN     CONST OC_RSA_PUBLIC_KEY     *PublicKey
  )
{
  BOOLEAN Result;

  ASSERT (Context != NULL);
  ASSERT (Context->Signature != NULL);

  Result = RsaVerifySigHashFromKey (
             PublicKey,
             Context->Signature->Signature,
             sizeof (Context->Signature->Signature),
             Context->Hash,
             sizeof (Context->Hash),
             OcSigHashTypeSha256
             );
  DEBUG_CODE (
    if (Result) {
      Context->Signature = NULL;
    }
    );

  return Result;
}

BOOLEAN
OcAppleChunklistVerifyData (
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT         *Context,
  IN     CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable
  )
{
  BOOLEAN                     Result;

  UINTN                       Index;
  UINT8                       ChunkHash[SHA256_DIGEST_SIZE];
  CONST APPLE_CHUNKLIST_CHUNK *CurrentChunk;
  UINTN                       CurrentOffset;

  UINT32                      ChunkDataSize;
  VOID                        *ChunkData;

  ASSERT (Context != NULL);
  ASSERT (Context->Chunks != NULL);
  ASSERT (ExtentTable != NULL);

  DEBUG_CODE (
    ASSERT (Context->Signature == NULL);
    );

  ChunkDataSize = 0;
  for (Index = 0; Index < Context->ChunkCount; ++Index) {
    CurrentChunk = &Context->Chunks[Index];
    if (ChunkDataSize < CurrentChunk->Length) {
      ChunkDataSize = CurrentChunk->Length;
    }
  }

  ChunkData = AllocatePool (ChunkDataSize);
  if (ChunkData == NULL) {
    return FALSE;
  }

  CurrentOffset = 0;
  for (Index = 0; Index < Context->ChunkCount; Index++) {
    CurrentChunk = &Context->Chunks[Index];

    Result = OcAppleRamDiskRead (
               ExtentTable,
               CurrentOffset,
               CurrentChunk->Length,
               ChunkData
               );
    if (!Result) {
      FreePool (ChunkData);
      return FALSE;
    }
    //
    // Calculate checksum of data and ensure they match.
    //
    DEBUG ((DEBUG_VERBOSE, "OCCL: Validating chunk %lu of %lu\n",
      (UINT64)Index + 1, (UINT64)Context->ChunkCount));
    Sha256 (ChunkHash, ChunkData, CurrentChunk->Length);
    if (CompareMem (ChunkHash, CurrentChunk->Checksum, SHA256_DIGEST_SIZE) != 0) {
      FreePool (ChunkData);
      return FALSE;
    }

    CurrentOffset += CurrentChunk->Length;
  }

  FreePool (ChunkData);
  return TRUE;
}
