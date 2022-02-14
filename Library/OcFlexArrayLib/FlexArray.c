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

VOID
OcFlexArrayFreePointerItem (
  IN  VOID  *Item
  )
{
  ASSERT (Item != NULL);

  if (*(VOID **) Item != NULL) {
    FreePool (*(VOID **) Item);
    *(VOID **) Item = NULL;
  }
}

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

  DEBUG ((OC_TRACE_FLEX, "FLEX: Init %p %u\n", FlexArray, ItemSize));

  return FlexArray;
}

/**
  Retrieve the Index-th item in a flex array.

  @param[in]  FlexArray  A pointer to the flex array.
  @param[in]  Index      The Index-th item to be retrieved in FlexArray.

  @return  The Index-th item in FlexArray.
**/
STATIC
VOID *
InternalFlexArrayItemAt (
  IN     CONST OC_FLEX_ARRAY                *FlexArray,
  IN     CONST UINTN                        Index
  )
{
  VOID  *Item;

  ASSERT (FlexArray != NULL);
  ASSERT (Index < FlexArray->Count);
  ASSERT (FlexArray->Items != NULL);

  Item = ((UINT8 *) FlexArray->Items) + Index * FlexArray->ItemSize;

  return Item;
}

/**
  Add an item in a flex array.

  @param[in,out]  FlexArray  A pointer to the flex array.

  @return  The added item.
**/
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
    ASSERT (FlexArray->AllocatedCount > 0);
    ASSERT (FlexArray->Count <= FlexArray->AllocatedCount);
    ++(FlexArray->Count);
    if (FlexArray->Count > FlexArray->AllocatedCount) {
      if (OcOverflowMulUN (FlexArray->AllocatedCount * FlexArray->ItemSize, 2, &NewSize)) {
        return NULL;
      }
      TmpBuffer = ReallocatePool (
        FlexArray->AllocatedCount * FlexArray->ItemSize,
        NewSize,
        FlexArray->Items
        );
      if (TmpBuffer == NULL) {
        return NULL;
      }
      FlexArray->Items = TmpBuffer;
      FlexArray->AllocatedCount = FlexArray->AllocatedCount * 2;
    }
  }

  Item = InternalFlexArrayItemAt (FlexArray, FlexArray->Count - 1);

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

  DEBUG ((OC_TRACE_FLEX, "FLEX: Add %p %p\n", FlexArray, Item));

  return Item;
}

VOID *
OcFlexArrayInsertItem (
  IN OUT  OC_FLEX_ARRAY                *FlexArray,
  IN      CONST UINTN                  InsertIndex
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

  Item = InternalFlexArrayItemAt (FlexArray, InsertIndex);
  Dest = InternalFlexArrayItemAt (FlexArray, InsertIndex + 1);
  CopyMem (Dest, Item, (FlexArray->Count - InsertIndex) * FlexArray->ItemSize);

  ZeroMem (Item, FlexArray->ItemSize);

  DEBUG ((OC_TRACE_FLEX, "FLEX: Insert %p %p\n", FlexArray, Item));

  return Item;
}

VOID *
OcFlexArrayItemAt (
  IN     CONST OC_FLEX_ARRAY                *FlexArray,
  IN     CONST UINTN                        Index
  )
{
  VOID *Item;

  //
  // Repeat these checks here and in internal ItemAt for easier debugging if they fail.
  //
  ASSERT (FlexArray != NULL);
  ASSERT (Index < FlexArray->Count);
  ASSERT (FlexArray->Items != NULL);

  Item = InternalFlexArrayItemAt (FlexArray, Index);

  DEBUG ((OC_TRACE_FLEX, "FLEX: At %p %p\n", FlexArray, Item));

  return Item;
}

VOID 
OcFlexArrayFree (
  IN           OC_FLEX_ARRAY                **FlexArray
  )
{
  UINTN Index;

  DEBUG ((OC_TRACE_FLEX, "FLEX: Free %p\n", FlexArray));

  ASSERT (FlexArray != NULL);

  if (*FlexArray != NULL) {
    if ((*FlexArray)->Items != NULL) {
      if ((*FlexArray)->FreeItem != NULL) {
        for (Index = 0; Index < (*FlexArray)->Count; Index++) {
          (*FlexArray)->FreeItem (InternalFlexArrayItemAt (*FlexArray, Index));
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
  IN OUT  OC_FLEX_ARRAY       *FlexArray,
  IN      CONST BOOLEAN       FreeItem
  )
{
  DEBUG ((OC_TRACE_FLEX, "FLEX: Discard %p %u\n", FlexArray, FreeItem));

  ASSERT (FlexArray != NULL);
  ASSERT (FlexArray->Items != NULL);
  ASSERT (FlexArray->Count > 0);

  if (FreeItem && FlexArray->FreeItem != NULL) {
    FlexArray->FreeItem (InternalFlexArrayItemAt (FlexArray, FlexArray->Count - 1));
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
  DEBUG ((OC_TRACE_FLEX, "FLEX: FreeContainer %p\n", FlexArray));

  if (FlexArray == NULL || *FlexArray == NULL) {
    ASSERT (FALSE);
    *Items = NULL;
    *Count = 0;
  } else {
    *Items = (*FlexArray)->Items;
    *Count = (*FlexArray)->Count;
    if (*Count == 0 && *Items != NULL) {
      FreePool (*Items);
      *Items = NULL;
    }
    FreePool (*FlexArray);
    *FlexArray = NULL;
  }
}
