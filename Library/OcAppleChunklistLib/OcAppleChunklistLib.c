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
#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
OcAppleChunklistInitializeContext(
    IN  VOID *Buffer,
    IN  UINTN Length,
    OUT OC_APPLE_CHUNKLIST_CONTEXT *Context) {

    // Create variables.
    APPLE_CHUNKLIST_HEADER *ChunklistHeader = NULL;

    // Check if parameters are valid.
    if (!Buffer || !Length || !Context)
        return EFI_INVALID_PARAMETER;
    if (Length < sizeof(APPLE_CHUNKLIST_HEADER))
        return EFI_INVALID_PARAMETER;

    // Get header and ensure it is valid.
    ChunklistHeader = (APPLE_CHUNKLIST_HEADER*)Buffer;
    if ((ChunklistHeader->Magic != APPLE_CHUNKLIST_MAGIC) ||
        (ChunklistHeader->Length != sizeof(APPLE_CHUNKLIST_HEADER)) ||
        (ChunklistHeader->FileVersion != APPLE_CHUNKLIST_FILE_VERSION_10) ||
        (ChunklistHeader->ChunkMethod != APPLE_CHUNKLIST_CHUNK_METHOD_10) ||
        (ChunklistHeader->SigMethod != APPLE_CHUNKLIST_SIG_METHOD_10))
        return EFI_UNSUPPORTED;

    // Get chunklist data.
    ZeroMem(Context, sizeof(OC_APPLE_CHUNKLIST_CONTEXT));
    Context->Header = ChunklistHeader;
    Context->FileSize = Length;
    Context->Chunks = (APPLE_CHUNKLIST_CHUNK*)(((UINT8*)Buffer) + ReadUnaligned64(&(ChunklistHeader->ChunkOffset)));
    Context->Signature = (APPLE_CHUNKLIST_SIG*)(((UINT8*)Buffer) + ReadUnaligned64(&(ChunklistHeader->SigOffset)));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcAppleChunklistVerifyData(
    IN OC_APPLE_CHUNKLIST_CONTEXT *Context,
    IN VOID *Buffer,
    IN UINTN Length) {

    // Create variables.
    UINT8 *BufferCurrent = NULL;
    UINTN RemainingLength = 0;
    APPLE_CHUNKLIST_CHUNK *Chunks;
    UINT64 ChunkCount;
    UINT8 ChunkHash[SHA256_DIGEST_SIZE];

    // Check if parameters are valid.
    if (!Context || !Buffer || !Length)
        return EFI_INVALID_PARAMETER;

    // Get chunklist info.
    Chunks = Context->Chunks;
    ChunkCount = ReadUnaligned64(&(Context->Header->ChunkCount));

    // Hash each chunk and validate checksums are the same.
    BufferCurrent = (UINT8*)Buffer;
    RemainingLength = Length;
    for (UINT64 i = 0; i < ChunkCount; i++) {
        // Ensure length of chunk is valid.
        if (Chunks[i].Length > RemainingLength)
            return EFI_END_OF_FILE;

        // Calculate checksum of data and ensure they match.
        DEBUG((DEBUG_INFO, "AppleChunklistVerifyData(): Validating chunk %lu of %lu\n", i, ChunkCount));
        Sha256(ChunkHash, BufferCurrent, Chunks[i].Length);
        if (CompareMem(ChunkHash, Chunks[i].Checksum, SHA256_DIGEST_SIZE))
            return EFI_COMPROMISED_DATA;

        // Move to next chunk.
        BufferCurrent += Chunks[i].Length;
        RemainingLength -= Chunks[i].Length;
    }

    // Success.
    return EFI_SUCCESS;
}
