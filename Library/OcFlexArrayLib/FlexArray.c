/** @file
  Auto-resizing array.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcGuardLib.h>

#define INITIAL_NUM_ITEMS (8)

OC_FLEX_ARRAY *
OcFlexArrayInit (
  IN     CONST UINTN                        ItemSize,
  IN     CONST OC_FLEX_ARRAY_FREE_ITEM      FreeItem   OPTIONAL
  )
{
  OC_FLEX_ARRAY   *FlexArray;

  ASSERT (ItemSize > 0);

  FlexArray = AllocateZeroPool (sizeof (OC_FLEX_ARRAY));
  if (FlexArray != NULL) {
    FlexArray->ItemSize = ItemSize;
    FlexArray->FreeItem = FreeItem;
  }
  return FlexArray;
}

STATIC
VOID *
InternalFlexArrayAddItem (
  IN OUT       OC_FLEX_ARRAY                *FlexArray
  )
{
  VOID    *TmpBuffer;
  UINTN   NewSize;
  VOID    *Item;

  ASSERT (FlexArray != NULL);

  if (FlexArray->Items == NULL) {
    FlexArray->AllocatedCount = INITIAL_NUM_ITEMS;
    if (OcOverflowMulUN (FlexArray->AllocatedCount, FlexArray->ItemSize, &NewSize)) {
      return NULL;
    }
    FlexArray->Count = 1;
    FlexArray->Items = AllocatePool (NewSize);
    if (FlexArray->Items == NULL) {
      return NULL;
    }
  } else {
    ASSERT (FlexArray->Count > 0);
    ASSERT (FlexArray->AllocatedCount > 0);
    ASSERT (FlexArray->Count <= FlexArray->AllocatedCount);
    ++(FlexArray->Count);
    if (FlexArray->Count > FlexArray->AllocatedCount) {
      if (OcOverflowMulUN (FlexArray->AllocatedCount * FlexArray->ItemSize, 2, &NewSize)) {
        return NULL;
      }
      TmpBuffer = ReallocatePool (FlexArray->AllocatedCount * FlexArray->ItemSize, NewSize, FlexArray->Items);
      if (TmpBuffer == NULL) {
        return NULL;
      }
      FlexArray->Items = TmpBuffer;
      FlexArray->AllocatedCount = FlexArray->AllocatedCount * 2;
    }
  }

  Item = OcFlexArrayItemAt (FlexArray, FlexArray->Count - 1);

  return Item;
}

VOID *
OcFlexArrayAddItem (
  IN OUT       OC_FLEX_ARRAY                *FlexArray
  )
{
  VOID    *Item;

  ASSERT (FlexArray != NULL);
  
  Item = InternalFlexArrayAddItem (FlexArray);

  if (Item != NULL) {
    ZeroMem (Item, FlexArray->ItemSize);
  }

  return Item;
}

VOID *
OcFlexArrayInsertItem (
  IN OUT       OC_FLEX_ARRAY                *FlexArray,
  IN     CONST UINTN                        InsertIndex
  )
{
  VOID    *Item;
  VOID    *Dest;

  ASSERT (FlexArray != NULL);
  ASSERT (InsertIndex <= FlexArray->Count);

  if (InsertIndex == FlexArray->Count) {
    return OcFlexArrayAddItem (FlexArray);
  }

  Item = InternalFlexArrayAddItem (FlexArray);

  if (Item == NULL) {
    return Item;
  }

  Item = OcFlexArrayItemAt (FlexArray, InsertIndex);
  Dest = OcFlexArrayItemAt (FlexArray, InsertIndex + 1);
  CopyMem (Dest, Item, (FlexArray->Count - InsertIndex) * FlexArray->ItemSize);

  ZeroMem (Item, FlexArray->ItemSize);

  return Item;
}

VOID *
OcFlexArrayItemAt (
  IN     CONST OC_FLEX_ARRAY                *FlexArray,
  IN     CONST UINTN                        Index
  )
{
  if (Index >= FlexArray->Count || FlexArray->Items == NULL) {
    ASSERT (FALSE);
    return NULL;
  }
  
  return ((UINT8 *) FlexArray->Items) + Index * FlexArray->ItemSize;
}

VOID 
OcFlexArrayFree (
  IN           OC_FLEX_ARRAY                **FlexArray
  )
{
  UINTN Index;

  ASSERT (FlexArray != NULL);

  if (*FlexArray != NULL) {
    if ((*FlexArray)->Items != NULL) {
      if ((*FlexArray)->FreeItem != NULL) {
        for (Index = 0; Index < (*FlexArray)->Count; Index++) {
          (*FlexArray)->FreeItem (OcFlexArrayItemAt (*FlexArray, Index));
        }
      }
      FreePool ((*FlexArray)->Items);
    }
    FreePool (*FlexArray);
    *FlexArray = NULL;
  }
}

VOID
OcFlexArrayDiscardItem (
  IN OUT       OC_FLEX_ARRAY       *FlexArray,
  IN     CONST BOOLEAN             FreeItem
  )
{
  ASSERT (FlexArray != NULL);
  ASSERT (FlexArray->Items != NULL);
  ASSERT (FlexArray->Count > 0);

  if (FreeItem && FlexArray->FreeItem != NULL) {
    FlexArray->FreeItem (OcFlexArrayItemAt (FlexArray, FlexArray->Count - 1));
  }
  
  --FlexArray->Count;
}

VOID
OcFlexArrayFreeContainer (
  IN           OC_FLEX_ARRAY                **FlexArray,
  IN           VOID                         **Items,
  IN           UINTN                        *Count
  )
{
  if (FlexArray == NULL || *FlexArray == NULL) {
    ASSERT (FALSE);
    *Items = NULL;
    *Count = 0;
  } else {
    *Items = (*FlexArray)->Items;
    *Count = (*FlexArray)->Count;
    FreePool (*FlexArray);
    *FlexArray = NULL;
  }
}

VOID
OcFlexArrayFreePointerItem (
  IN VOID *Item
  )
{
  ASSERT (Item != NULL);
  if (*(VOID **)Item != NULL) {
    FreePool (*(VOID **)Item);
    *(VOID **)Item = NULL;
  }
}
