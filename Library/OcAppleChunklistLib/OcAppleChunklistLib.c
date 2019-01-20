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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAppleChunklistLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
OcAppleChunklistInitializeContext (
  IN  VOID                        *Buffer,
  IN  UINTN                       Length,
  OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context
  )
{
  APPLE_CHUNKLIST_HEADER  *ChunklistHeader;
  UINTN                   DataEnd;

  ASSERT (Buffer != NULL);
  ASSERT (Length > 0);
  ASSERT (Context != NULL);

  ChunklistHeader = (APPLE_CHUNKLIST_HEADER *) Buffer;

  if (Length < sizeof (APPLE_CHUNKLIST_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Ensure the header is compatible.
  //
  if (ChunklistHeader->Magic != APPLE_CHUNKLIST_MAGIC
    || ChunklistHeader->Length != sizeof (APPLE_CHUNKLIST_HEADER)
    || ChunklistHeader->FileVersion != APPLE_CHUNKLIST_FILE_VERSION_10
    || ChunklistHeader->ChunkMethod != APPLE_CHUNKLIST_CHUNK_METHOD_10
    || ChunklistHeader->SigMethod != APPLE_CHUNKLIST_SIG_METHOD_10) {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (Context, sizeof (OC_APPLE_CHUNKLIST_CONTEXT));
  Context->Header    = ChunklistHeader;
  Context->FileSize  = Length;

  //
  // Ensure that chunk and signature addresses are valid in the first place.
  //
  if (OcOverflowAddUN ((UINTN) Buffer, (UINTN) ChunklistHeader->ChunkOffset, (UINTN *) &Context->Chunks)
    || OcOverflowAddUN ((UINTN) Buffer, (UINTN) ChunklistHeader->SigOffset, (UINTN *) &Context->Signature)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Ensure that chunks and signature reside within Buffer.
  //
  if (OcOverflowMulAddUN (sizeof (APPLE_CHUNKLIST_CHUNK), ChunklistHeader->ChunkCount, (UINTN) Context->Chunks, &DataEnd)
    || DataEnd > (UINTN) Buffer + Length
    || OcOverflowAddUN (sizeof (APPLE_CHUNKLIST_SIG), (UINTN) Context->Signature, &DataEnd)
    || DataEnd > (UINTN) Buffer + Length) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcAppleChunklistVerifyData (
  IN OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN  VOID                       *Buffer,
  IN  UINTN                      Length
  )
{
  UINT64                 Index;
  UINTN                  RemainingLength;
  UINT8                  *BufferCurrent;
  UINT8                  ChunkHash[SHA256_DIGEST_SIZE];
  UINT64                 ChunkCount;
  APPLE_CHUNKLIST_CHUNK  *CurrentChunk;

  ASSERT (Context != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (Length > 0);
  ASSERT (Context->Header != NULL);
  ASSERT (Context->FileSize > 0);
  ASSERT (Context->Chunks != NULL);
  ASSERT (Context->Signature != NULL);

  RemainingLength = Length;
  BufferCurrent   = (UINT8 *) Buffer;
  ChunkCount      = Context->Header->ChunkCount;
  CurrentChunk    = &Context->Chunks[0];

  for (Index = 0; Index < ChunkCount; Index++) {
    //
    // Ensure length of chunk is valid.
    //
    if (RemainingLength < CurrentChunk->Length) {
      return EFI_END_OF_FILE;
    }

    //
    // Calculate checksum of data and ensure they match.
    //
    DEBUG ((DEBUG_INFO, "AppleChunklistVerifyData(): Validating chunk %lu of %lu\n",
      Index, ChunkCount));
    Sha256 (ChunkHash, BufferCurrent, CurrentChunk->Length);
    if (CompareMem (ChunkHash, CurrentChunk->Checksum, SHA256_DIGEST_SIZE) != 0) {
      return EFI_COMPROMISED_DATA;
    }

    //
    // Move to next chunk.
    //
    BufferCurrent   += CurrentChunk->Length;
    RemainingLength -= CurrentChunk->Length;
    CurrentChunk++;
  }

  return EFI_SUCCESS;
}
