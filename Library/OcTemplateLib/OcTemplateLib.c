/** @file

OcTemplateLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcTemplateLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>

#define PRIV_OC_BLOB_FIELDS(_, __) \
  OC_BLOB (CHAR8, [], {0}, _, __)
  OC_DECLARE (PRIV_OC_BLOB)

#define PRIV_OC_MAP_FIELDS(_, __) \
  OC_MAP (PRIV_OC_BLOB, PRIV_OC_BLOB, _, __)
  OC_DECLARE (PRIV_OC_MAP)

#define PRIV_OC_ARRAY_FIELDS(_, __) \
  OC_ARRAY (PRIV_OC_BLOB, _, __)
  OC_DECLARE (PRIV_OC_ARRAY)

typedef union PRIV_OC_LIST_ {
  PRIV_OC_MAP    Map;
  PRIV_OC_ARRAY  Array;
} PRIV_OC_LIST;

//
// We have to be a bit careful about this hack, so assert that type layouts match at the very least.
//
#if defined(__GNUC__) || defined(__clang__)
STATIC_ASSERT(OFFSET_OF (PRIV_OC_ARRAY, Count)      == OFFSET_OF (PRIV_OC_MAP, Count), "PRIV_OC_ARRAY vs PRIV_OC_MAP");
STATIC_ASSERT(OFFSET_OF (PRIV_OC_ARRAY, AllocCount) == OFFSET_OF (PRIV_OC_MAP, AllocCount), "PRIV_OC_ARRAY vs PRIV_OC_MAP");
STATIC_ASSERT(OFFSET_OF (PRIV_OC_ARRAY, Construct)  == OFFSET_OF (PRIV_OC_MAP, Construct), "PRIV_OC_ARRAY vs PRIV_OC_MAP");
STATIC_ASSERT(OFFSET_OF (PRIV_OC_ARRAY, Destruct)   == OFFSET_OF (PRIV_OC_MAP, Destruct), "PRIV_OC_ARRAY vs PRIV_OC_MAP");
STATIC_ASSERT(OFFSET_OF (PRIV_OC_ARRAY, Values)     == OFFSET_OF (PRIV_OC_MAP, Values), "PRIV_OC_ARRAY vs PRIV_OC_MAP");
STATIC_ASSERT(OFFSET_OF (PRIV_OC_ARRAY, ValueSize)  == OFFSET_OF (PRIV_OC_MAP, ValueSize), "PRIV_OC_ARRAY vs PRIV_OC_MAP");
#endif

VOID
OcFreePointer (
  VOID    *Pointer,
  UINT32  Size
  )
{
  VOID **Field = (VOID **) Pointer;
  if (*Field) {
    FreePool (*Field);
    *Field = NULL;
  }
}

VOID
OcZeroField (
  VOID    *Pointer,
  UINT32  Size
  )
{
  ZeroMem (Pointer, Size);
}

VOID
OcDestructEmpty (
  VOID    *Pointer,
  UINT32  Size
  )
{
  (VOID) Pointer;
  (VOID) Size;
}

STATIC
VOID
OcFreeList (
  VOID     *Pointer,
  BOOLEAN  HasKeys
  )
{
  UINT32              Index;
  PRIV_OC_LIST        *List;

  List = (PRIV_OC_LIST *) Pointer;

  for (Index = 0; Index < List->Array.Count; Index++) {
    List->Array.Destruct (List->Array.Values[Index], List->Array.ValueSize);
    FreePool (List->Array.Values[Index]);

    if (HasKeys) {
      List->Map.KeyDestruct (List->Map.Keys[Index], List->Map.KeySize);
      FreePool (List->Map.Keys[Index]);
    }
  }

  OcFreePointer (&List->Array.Values, List->Array.AllocCount * List->Array.ValueSize);
  if (HasKeys) {
    OcFreePointer (&List->Map.Keys, List->Array.AllocCount * List->Map.KeySize);
  }

  List->Array.Count = 0;
  List->Array.AllocCount = 0;
}

VOID
OcFreeMap (
  VOID    *Pointer,
  UINT32  Size
  )
{
  OcFreeList (Pointer, TRUE);
}

VOID
OcFreeArray (
  VOID    *Pointer,
  UINT32  Size
  )
{
  OcFreeList (Pointer, FALSE);
}


VOID *
OcBlobAllocate (
  VOID    *Pointer,
  UINT32  Size,
  UINT32  **OutSize  OPTIONAL
  )
{
  PRIV_OC_BLOB  *Blob;
  VOID          *DynValue;

  Blob = (PRIV_OC_BLOB *) Pointer;

  DEBUG ((DEBUG_VERBOSE, "OCTPL: Allocating %u bytes in blob %p with size %u/%u curr %p\n",
     Size, Blob, Blob->Size, Blob->MaxSize, Blob->DynValue));

  //
  // We fit into static space
  //
  if (Size <= Blob->MaxSize) {
    OcFreePointer (&Blob->DynValue, Blob->Size);
    Blob->Size = Size;
    if (OutSize != NULL) {
      *OutSize = &Blob->Size;
    }
    return Blob->Value;
  }

  //
  // We do not fit into dynamic space
  //
  if (Size > Blob->Size) {
    OcFreePointer (&Blob->DynValue, Blob->Size);
    DynValue = AllocatePool (Size);
    if (DynValue == NULL) {
      DEBUG ((DEBUG_VERBOSE, "OCTPL: Failed to fit %u bytes in OC_BLOB\n", Size));
      return NULL;
    }
    //
    // Propagate default value.
    //
    CopyMem (DynValue, Blob->Value, Blob->MaxSize);
    Blob->DynValue = DynValue;
  }

  Blob->Size = Size;
  if (OutSize != NULL) {
    *OutSize = &Blob->Size;
  }

  return Blob->DynValue;
}

BOOLEAN
OcListEntryAllocate (
  VOID            *Pointer,
  VOID            **Value,
  VOID            **Key
  )
{
  PRIV_OC_LIST       *List;
  UINT32             Count;
  UINT32             AllocCount;
  VOID               **NewValues;
  VOID               **NewKeys;

  List = (PRIV_OC_LIST *) Pointer;

  //
  // Prepare new pair.
  //
  *Value = AllocatePool (List->Array.ValueSize);
  if (*Value == NULL) {
    return FALSE;
  }

  if (Key != NULL) {
    *Key = AllocatePool (List->Map.KeySize);
    if (*Key == NULL) {
      FreePool (*Value);
      return FALSE;
    }
  }

  //
  // Initialize new pair.
  //
  List->Array.Construct (*Value, List->Array.ValueSize);
  if (Key != NULL) {
    List->Map.KeyConstruct (*Key, List->Map.KeySize);
  }

  Count = 0;
  AllocCount = 1;

  //
  // Push new entry if there is enough room.
  //
  if (List->Array.Values != NULL) {
    Count = List->Array.Count;
    AllocCount = List->Array.AllocCount;

    if (AllocCount > Count) {
      List->Array.Count++;
      List->Array.Values[Count] = *Value;
      if (Key != NULL) {
        List->Map.Keys[Count]   = *Key;
      }
      return TRUE;
    }
  }

  //
  // Allocate twice more room and destruct on failure.
  //
  AllocCount *= 2;

  NewValues = (VOID **) AllocatePool (
    sizeof (VOID *) * AllocCount
    );

  if (NewValues == NULL) {
    List->Array.Destruct (*Value, List->Array.ValueSize);
    FreePool (*Value);
    if (Key != NULL) {
      List->Map.KeyDestruct (*Key, List->Map.KeySize);
      FreePool (*Key);
    }
    return FALSE;
  }

  if (Key != NULL) {
    NewKeys = (VOID **) AllocatePool (
      sizeof (VOID *) * AllocCount
      );

    if (NewKeys == NULL) {
      List->Array.Destruct (*Value, List->Array.ValueSize);
      List->Map.KeyDestruct (*Key, List->Map.KeySize);
      FreePool (NewValues);
      FreePool (*Value);
      FreePool (*Key);
      return FALSE;
    }
  } else {
    NewKeys = NULL;
  }

  //
  // Insert and return.
  //
  if (List->Array.Values != NULL) {
    CopyMem (
      &NewValues[0],
      &List->Array.Values[0],
      sizeof (VOID *) * Count
      );

    FreePool (List->Array.Values);
  }

  if (Key != NULL && List->Map.Keys != NULL) {
    CopyMem (
      &NewKeys[0],
      &List->Map.Keys[0],
      sizeof (VOID *) * Count
      );

    FreePool (List->Map.Keys);
  }

  List->Array.Count++;
  List->Array.AllocCount = AllocCount;

  NewValues[Count]   = *Value;
  List->Array.Values = (PRIV_OC_BLOB **) NewValues;

  if (Key != NULL) {
    NewKeys[Count] = *Key;
    List->Map.Keys = (PRIV_OC_BLOB **) NewKeys;
  }

  return TRUE;
}

OC_BLOB_STRUCTORS (OC_STRING)
OC_BLOB_STRUCTORS (OC_DATA)
OC_MAP_STRUCTORS (OC_ASSOC)
