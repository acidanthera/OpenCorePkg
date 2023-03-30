/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include <string.h>
#include <stdlib.h>

#ifdef WIN32
  #include <malloc.h>
#endif // WIN32

//
// Limits single pool allocation size to 512MB by default.
// Use SetPoolAllocationSizeLimit to change this limit.
//
UINTN  mPoolAllocationSizeLimit = BASE_512MB;

UINTN  mPoolAllocations;
UINTN  mPageAllocations;

UINT64  mPoolAllocationMask = MAX_UINT64;
UINTN   mPoolAllocationIndex;
UINT64  mPageAllocationMask = MAX_UINT64;
UINTN   mPageAllocationIndex;

VOID
SetPoolAllocationSizeLimit (
  UINTN  AllocationSize
  )
{
  mPoolAllocationSizeLimit = AllocationSize;
}

VOID *
EFIAPI
CopyMem (
  OUT VOID        *DestinationBuffer,
  IN  CONST VOID  *SourceBuffer,
  IN  UINTN       Length
  )
{
  ASSERT (DestinationBuffer != NULL);
  ASSERT (SourceBuffer != NULL);

  return memmove (DestinationBuffer, SourceBuffer, Length);
}

VOID *
EFIAPI
SetMem (
  OUT VOID   *Buffer,
  IN  UINTN  Length,
  IN  UINT8  Value
  )
{
  ASSERT (Buffer != NULL);

  return memset (Buffer, Value, Length);
}

VOID *
EFIAPI
ZeroMem (
  OUT VOID   *Buffer,
  IN  UINTN  Length
  )
{
  ASSERT (Buffer != NULL);

  return memset (Buffer, 0, Length);
}

BOOLEAN
EFIAPI
IsZeroBuffer (
  IN CONST VOID  *Buffer,
  IN UINTN       Length
  )
{
  UINTN  Index;
  UINT8  *Walker;

  Walker = (UINT8 *)Buffer;

  for (Index = 0; Index < Length; ++Index) {
    if (Walker[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
EFIAPI
IsZeroGuid (
  IN CONST GUID  *Guid
  )
{
  UINT64  LowPartOfGuid;
  UINT64  HighPartOfGuid;

  LowPartOfGuid  = ReadUnaligned64 ((CONST UINT64 *)Guid);
  HighPartOfGuid = ReadUnaligned64 ((CONST UINT64 *)Guid + 1);

  return (BOOLEAN)(LowPartOfGuid == 0 && HighPartOfGuid == 0);
}

INTN
EFIAPI
CompareMem (
  IN  CONST VOID  *DestinationBuffer,
  IN  CONST VOID  *SourceBuffer,
  IN  UINTN       Length
  )
{
  ASSERT (DestinationBuffer != NULL);
  ASSERT (SourceBuffer != NULL);

  return memcmp (DestinationBuffer, SourceBuffer, Length);
}

VOID *
EFIAPI
ScanMem16 (
  IN  CONST VOID  *Buffer,
  IN  UINTN       Length,
  IN  UINT16      Value
  )
{
  UINT16  *Walker;
  UINTN   Index;

  Walker = (UINT16 *)Buffer;

  for (Index = 0; Index < Length; ++Index) {
    if (Walker[Index] == Value) {
      return Walker;
    }
  }

  return NULL;
}

VOID *
EFIAPI
AllocatePool (
  IN  UINTN  AllocationSize
  )
{
  VOID   *Buffer;
  UINTN  RequestedAllocationSize;

  Buffer                  = NULL;
  RequestedAllocationSize = 0;

  if (((mPoolAllocationMask & (1ULL << mPoolAllocationIndex)) != 0) && (AllocationSize + 7ULL > AllocationSize)) {
    //
    // UEFI guarantees 8-byte alignment.
    //
    RequestedAllocationSize = (AllocationSize + 7ULL) & ~7ULL;
    //
    // Check that we have not gone beyond the single allocation size limit
    //
    if (RequestedAllocationSize <= mPoolAllocationSizeLimit) {
      Buffer = malloc (RequestedAllocationSize);
    } else {
      DEBUG ((
        DEBUG_POOL,
        "UMEM: Requested allocation size %u exceeds the pool allocation limit %u \n",
        (UINT32)RequestedAllocationSize,
        (UINT32)mPoolAllocationSizeLimit
        ));
    }
  }

  ++mPoolAllocationIndex;
  mPoolAllocationIndex &= 63ULL;

  DEBUG ((
    DEBUG_POOL,
    "UMEM: Allocating pool %u at 0x%p\n",
    (UINT32)AllocationSize,
    Buffer
    ));

  ASSERT (((UINTN)Buffer & 7ULL) == 0);

  if (Buffer != NULL) {
    ++mPoolAllocations;
  }

  return Buffer;
}

VOID *
EFIAPI
AllocateCopyPool (
  IN  UINTN       AllocationSize,
  IN  CONST VOID  *Buffer
  )
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);

  Memory = AllocatePool (AllocationSize);

  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }

  return Memory;
}

VOID *
EFIAPI
AllocateZeroPool (
  IN  UINTN  AllocationSize
  )
{
  VOID  *Memory;

  Memory = AllocatePool (AllocationSize);

  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }

  return Memory;
}

VOID *
ReallocatePool (
  IN  UINTN  OldSize,
  IN  UINTN  NewSize,
  IN  VOID   *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;

  NewBuffer = AllocateZeroPool (NewSize);

  if ((NewBuffer != NULL) && (OldBuffer != NULL)) {
    CopyMem (NewBuffer, OldBuffer, MIN (OldSize, NewSize));
    FreePool (OldBuffer);
  }

  return NewBuffer;
}

VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  VOID   *Memory;
  UINTN  RequestedAllocationSize;

  Memory                  = NULL;
  RequestedAllocationSize = Pages * EFI_PAGE_SIZE;

  if (((mPageAllocationMask & (1ULL << mPageAllocationIndex)) != 0) &&
      ((Pages != 0) && (RequestedAllocationSize / Pages == EFI_PAGE_SIZE)))
  {
    //
    // Check that we have not gone beyond the single allocation size limit
    //
    if (RequestedAllocationSize <= mPoolAllocationSizeLimit) {
 #ifdef _WIN32
      Memory = _aligned_malloc (RequestedAllocationSize, EFI_PAGE_SIZE);
 #else // !_WIN32
      Memory = NULL;
      INTN  RetVal;

      RetVal = posix_memalign (&Memory, EFI_PAGE_SIZE, RequestedAllocationSize);
      if (RetVal != 0) {
        DEBUG ((DEBUG_ERROR, "posix_memalign returns error %d\n", RetVal));
        Memory = NULL;
      }

 #endif // _WIN32
    }
  }

  ++mPageAllocationIndex;
  mPageAllocationIndex &= 63U;

  DEBUG ((
    DEBUG_PAGE,
    "UMEM: Allocating %u pages at 0x%p\n",
    (UINT32)Pages,
    Memory
    ));

  if (Memory != NULL) {
    mPageAllocations += Pages;
  }

  return Memory;
}

VOID
EFIAPI
FreePool (
  IN VOID  *Buffer
  )
{
  ASSERT (Buffer != NULL);

  DEBUG ((
    DEBUG_POOL,
    "UMEM: Deallocating pool 0x%p\n",
    Buffer
    ));

  //
  // Check that we are freeing buffer produced by our AllocatePool implementation
  //
  if (mPoolAllocations == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "UMEM: Requested buffer to free allocated not by AllocatePool implementations \n"
      ));
    abort ();
  }

  --mPoolAllocations;

  free (Buffer);
}

VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  UINTN  BytesToFree;

  ASSERT (Buffer != NULL);

  DEBUG ((
    DEBUG_PAGE,
    "UMEM: Deallocating %u pages at 0x%p\n",
    (UINT32)Pages,
    Buffer
    ));

  //
  // Check that requested pages count to free not exceeds total
  // allocated pages count
  //
  if (Pages > mPageAllocations) {
    DEBUG ((
      DEBUG_ERROR,
      "UMEM: Requested pages count %u to free exceeds total allocated pages %u\n",
      (UINT32)Pages,
      (UINT32)mPageAllocations
      ));
    abort ();
  }

  BytesToFree = Pages * EFI_PAGE_SIZE;
  if ((Pages != 0) && (BytesToFree / Pages == EFI_PAGE_SIZE)) {
    mPageAllocations -= Pages;
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "UMEM: Passed pages count %u proceeds unsigned integer overflow during BytesToFree multiplication\n",
      (UINT32)Pages
      ));
    abort ();
  }

 #ifdef _WIN32
  _aligned_free (Buffer);
 #else
  free (Buffer);
 #endif
}

GUID *
EFIAPI
CopyGuid (
  OUT GUID        *DestinationGuid,
  IN  CONST GUID  *SourceGuid
  )
{
  ASSERT (DestinationGuid != NULL);
  ASSERT (SourceGuid != NULL);

  CopyMem (DestinationGuid, SourceGuid, sizeof (GUID));

  return DestinationGuid;
}

BOOLEAN
EFIAPI
CompareGuid (
  IN  CONST GUID  *Guid1,
  IN  CONST GUID  *Guid2
  )
{
  ASSERT (Guid1 != NULL);
  ASSERT (Guid2 != NULL);

  return CompareMem (Guid1, Guid2, sizeof (GUID)) == 0;
}

UINT16
EFIAPI
ReadUnaligned16 (
  IN CONST UINT16  *Buffer
  )
{
  UINT16  Value;

  CopyMem (&Value, Buffer, sizeof (UINT16));

  return Value;
}

UINT16
EFIAPI
WriteUnaligned16 (
  OUT UINT16  *Buffer,
  IN  UINT16  Value
  )
{
  ASSERT (Buffer != NULL);

  CopyMem (Buffer, &Value, sizeof (UINT16));

  return Value;
}

UINT32
EFIAPI
ReadUnaligned24 (
  IN CONST UINT32  *Buffer
  )
{
  UINT32  Value;

  Value = ReadUnaligned32 (Buffer) & 0xFFFFFFU;

  return Value;
}

UINT32
EFIAPI
ReadUnaligned32 (
  IN CONST UINT32  *Buffer
  )
{
  UINT32  Value;

  ASSERT (Buffer != NULL);

  CopyMem (&Value, Buffer, sizeof (UINT32));

  return Value;
}

UINT64
EFIAPI
ReadUnaligned64 (
  IN CONST UINT64  *Buffer
  )
{
  UINT64  Value;

  ASSERT (Buffer != NULL);

  CopyMem (&Value, Buffer, sizeof (UINT64));

  return Value;
}

UINT32
EFIAPI
WriteUnaligned32 (
  OUT UINT32  *Buffer,
  IN  UINT32  Value
  )
{
  ASSERT (Buffer != NULL);

  CopyMem (Buffer, &Value, sizeof (UINT32));

  return Value;
}

UINT64
EFIAPI
WriteUnaligned64 (
  OUT UINT64  *Buffer,
  IN  UINT64  Value
  )
{
  ASSERT (Buffer != NULL);

  CopyMem (Buffer, &Value, sizeof (UINT64));

  return Value;
}
