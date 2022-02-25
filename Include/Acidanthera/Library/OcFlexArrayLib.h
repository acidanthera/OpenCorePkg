/** @file
  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_FLEX_ARRAY_LIB_H
#define OC_FLEX_ARRAY_LIB_H

#if !defined(OC_TRACE_FLEX)
#define OC_TRACE_FLEX DEBUG_VERBOSE
#endif

#include <Uefi.h>
#include <Library/OcStringLib.h>
#include <Protocol/ApplePlatformInfoDatabase.h>

/**
  Forward declaration of OC_FLEX_ARRAY structure.
**/
typedef struct OC_FLEX_ARRAY_ OC_FLEX_ARRAY;

/**
  Free any allocated memory pointed to by flex array item.

  @param[in]      Item          Item to free.
**/
typedef
VOID
(* OC_FLEX_ARRAY_FREE_ITEM) (
  IN  VOID  *Item
  );

/**
  Utility method to free memory pointed to by flex array
  item when the item is a single pointer, or when the only
  allocated memory is pointed to by a pointer which is the
  first element.

  @param[in]      Item          Flex array item to free.
**/
VOID
OcFlexArrayFreePointerItem (
  IN  VOID  *Item
  );

/**
  Initialize flex array.

  @param[in]      ItemSize      Size of each item in the array.
  @param[in]      FreeItem      Method to free one item, called once per item by
                                OcFlexArrayFree; may be NULL if there is nothing
                                pointed to by pool items which needs freeing.

  @retval Non-NULL              Flex array was created.
  @retval NULL                  Out of memory.
**/
OC_FLEX_ARRAY *
OcFlexArrayInit (
  IN     CONST UINTN                         ItemSize,
  IN     CONST OC_FLEX_ARRAY_FREE_ITEM       FreeItem   OPTIONAL
  );

/**
  Add new item to flex array, resizing if necessary. New item memory is zeroed.

  @param[in,out]  FlexArray     Flex array to modify.

  @retval Non-NULL              Address of item created.
  @retval NULL                  Out of memory, in which case FlexArray->Items will be set
                                to NULL, but FlexArray itself will still be allocated.
**/
VOID *
OcFlexArrayAddItem (
  IN OUT       OC_FLEX_ARRAY       *FlexArray
  );

/**
  Insert new item at position in flex array, resizing array if necessary. New item memory is zeroed.

  @param[in,out]  FlexArray     Flex array to modify.
  @param[in]      InsertIndex   Index at which to insert; must be less than or equal to current item count.

  @retval Non-NULL              Address of item created.
  @retval NULL                  Out of memory, in which case FlexArray->Items will be set
                                to NULL, but FlexArray itself will still be allocated.
**/
VOID *
OcFlexArrayInsertItem (
  IN OUT  OC_FLEX_ARRAY                *FlexArray,
  IN      CONST UINTN                  InsertIndex
  );

/**
  Return item at index in array.

  @param[in,out]  FlexArray     Flex array to access.
  @param[in,out]  Index         Index of item to return.

  @retval Non-NULL              Item.
  @retval NULL                  Out-of-range item requested. This also ASSERTs.
**/
VOID *
OcFlexArrayItemAt (
  IN     CONST OC_FLEX_ARRAY       *FlexArray,
  IN     CONST UINTN               Index
  );

/**
  Free flex array.

  @param[in,out]  FlexArray     Flex array to free.
**/
VOID
OcFlexArrayFree (
  IN OUT       OC_FLEX_ARRAY      **FlexArray
  );

/**
  Discard last item on flex array, optionally calling item free method.

  @param[in,out]  FlexArray     Flex array to modify.
  @param[in]      FreeItem      Whether to call item free method.
**/
VOID
OcFlexArrayDiscardItem (
  IN OUT  OC_FLEX_ARRAY       *FlexArray,
  IN      CONST BOOLEAN       FreeItem
  );

/**
  Free dynamic container object, but do not free and
  pass back out allocated items with item count.

  @param[in,out]  FlexArray     Flex array to free.
  @param[in,out]  Items         Pool allocated flex array item list.
  @param[in,out]  Count         Item list count.
**/
VOID
OcFlexArrayFreeContainer (
  IN OUT       OC_FLEX_ARRAY      **FlexArray,
  IN OUT       VOID               **Items,
  IN OUT       UINTN              *Count
  );

/*
  Flex array.
*/
struct OC_FLEX_ARRAY_ {
  //
  // Allocated array.
  //
  VOID                            *Items;
  //
  // Item size.
  //
  UINTN                           ItemSize;
  //
  // Current used count.
  //
  UINTN                           Count;
  //
  // Current allocated count.
  //
  UINTN                           AllocatedCount;
  //
  // Optional method to free memory pointed to from item.
  //
  OC_FLEX_ARRAY_FREE_ITEM      FreeItem;
};

/*
  Forward declaration of OC_STRING_BUFFER structure.
*/
typedef struct OC_STRING_BUFFER_ OC_STRING_BUFFER;

/**
  Initialize string buffer.

  @retval Non-NULL                  Buffer was created.
  @retval NULL                      Out of memory.
**/
OC_STRING_BUFFER *
OcAsciiStringBufferInit (
  VOID
  );

/**
  Append new string to buffer, resizing if necessary.

  @param[in,out]  StringBuffer  Buffer to modify.
  @param[in]      AppendString  String to append. If NULL, nothing will be modified.

  @retval EFI_SUCCESS           String was appended.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_UNSUPPORTED       Internal error.
**/
EFI_STATUS
OcAsciiStringBufferAppend (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST CHAR8         *AppendString    OPTIONAL
  );

/**
  Append new substring to buffer, resizing if necessary.

  @param[in,out]  StringBuffer  Buffer to modify.
  @param[in]      AppendString  String to append. If NULL, nothing will be modified.
  @param[in]      Length        Maxiumum length of substring to use. 0 means use 0 characters.
                                Use MAX_UINTN for all chars. Ignored if AppendString is NULL.

  @retval EFI_SUCCESS           String was appended.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_UNSUPPORTED       Internal error.
**/
EFI_STATUS
OcAsciiStringBufferAppendN (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST CHAR8         *AppendString,   OPTIONAL
  IN      CONST UINTN         Length
  );


/**
  Safely print to string buffer.

  @param[in,out]  StringBuffer  Buffer to modify.
  @param[in]      FormatString  A Null-terminated ASCII format string.
  @param[in]      ...           Variable argument list whose contents are accessed based on the
                                format string specified by FormatString.

  @retval EFI_SUCCESS           When data was printed to string buffer.
  @retval EFI_OUT_OF_RESOURCES  Out of memory increasing string buffer sufficiently to print to.
**/
EFI_STATUS
EFIAPI
OcAsciiStringBufferSPrint (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST CHAR8         *FormatString,
  ...
  );

/**
  Free string buffer memory and return pointer to pool allocated resultant string.
  Note that if no data was ever appended to the string then the return value
  will be NULL, not a zero length string.

  @param[in,out]  StringBuffer          StringBuffer to free.

  @retval Non-NULL                      Pointer to pool allocated accumulated string.
  @retval NULL                          No data was ever added to the string.
**/
CHAR8 *
OcAsciiStringBufferFreeContainer (
  IN OUT  OC_STRING_BUFFER    **StringBuffer
  );

/**
  Free string buffer memory; free and discard any allocated resultant string.

  @param[in,out]  StringBuffer          StringBuffer to free.

  @retval Non-NULL                      Pointer to pool allocated accumulated string.
  @retval NULL                          No data was ever added to the string.
**/
VOID
OcAsciiStringBufferFree (
  IN OUT  OC_STRING_BUFFER    **StringBuffer
  );

/*
  String buffer.
*/
struct OC_STRING_BUFFER_ {
  CHAR8                           *String;
  UINTN                           StringLength;
  UINTN                           BufferSize;
};

/**
  Split string by delimiter.

  @param[in]  String     A Null-terminated string. 
  @param[in]  Delim      Delimiter to search in String.
  @param[in]  IsUnicode  Are option names and values Unicode or ASCII?

  @return  A flex array containing splitted string.
**/
OC_FLEX_ARRAY *
OcStringSplit (
  IN  CONST VOID     *String,
  IN  CONST CHAR16   Delim,
  IN  CONST BOOLEAN  IsUnicode
  );

#endif // OC_FLEX_ARRAY_LIB_H
