/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAppleChunklistLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcGuardLib.h>

BOOLEAN
OcAppleChunklistInitializeContext (
  OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN  VOID                        *Buffer,
  IN  UINT32                      BufferSize
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

  Context->ChunkCount = ChunklistHeader->ChunkCount;

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
  if (OcOverflowMulAddUN (sizeof (APPLE_CHUNKLIST_CHUNK), ChunklistHeader->ChunkCount, (UINTN) Context->Chunks, &DataEnd)
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
  IN     RSA_PUBLIC_KEY              *PublicKey
  )
{
  BOOLEAN Result;

  ASSERT (Context != NULL);
  ASSERT (Context->Signature != NULL);

  Result = RsaVerify (
             PublicKey,
             Context->Signature->Signature,
             Context->Hash
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
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN     VOID                        *Buffer,
  IN     UINTN                       BufferSize
  )
{
  UINT64                      Index;
  UINTN                       RemainingLength;
  UINT8                       *BufferCurrent;
  UINT8                       ChunkHash[SHA256_DIGEST_SIZE];
  UINT64                      ChunkCount;
  CONST APPLE_CHUNKLIST_CHUNK *CurrentChunk;

  ASSERT (Context != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);
  ASSERT (Context->Chunks != NULL);

  DEBUG_CODE (
    ASSERT (Context->Signature == NULL);
    );

  RemainingLength = BufferSize;
  BufferCurrent   = (UINT8 *) Buffer;
  ChunkCount      = Context->ChunkCount;
  CurrentChunk    = &Context->Chunks[0];

  for (Index = 0; Index < ChunkCount; Index++) {
    //
    // Ensure length of chunk is valid.
    //
    if (RemainingLength < CurrentChunk->Length) {
      return FALSE;
    }

    //
    // Calculate checksum of data and ensure they match.
    //
    DEBUG ((DEBUG_VERBOSE, "AppleChunklistVerifyData(): Validating chunk %lu of %lu\n",
      Index, ChunkCount));
    Sha256 (ChunkHash, BufferCurrent, CurrentChunk->Length);
    if (CompareMem (ChunkHash, CurrentChunk->Checksum, SHA256_DIGEST_SIZE) != 0) {
      return FALSE;
    }

    //
    // Move to next chunk.
    //
    BufferCurrent   += CurrentChunk->Length;
    RemainingLength -= CurrentChunk->Length;
    CurrentChunk++;
  }

  return TRUE;
}
