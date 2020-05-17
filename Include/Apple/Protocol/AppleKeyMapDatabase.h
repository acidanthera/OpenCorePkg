/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_KEY_MAP_DATABASE_H
#define APPLE_KEY_MAP_DATABASE_H

#include <IndustryStandard/AppleHid.h>

// APPLE_KEY_MAP_DATABASE_PROTOCOL_REVISION
#define APPLE_KEY_MAP_DATABASE_PROTOCOL_REVISION  0x00010000

// APPLE_KEY_MAP_DATABASE_PROTOCOL_GUID
#define APPLE_KEY_MAP_DATABASE_PROTOCOL_GUID  \
  { 0x584B9EBE, 0x80C1, 0x4BD6,               \
    { 0x98, 0xB0, 0xA7, 0x78, 0x6E, 0xC2, 0xF2, 0xE2 } }

typedef struct APPLE_KEY_MAP_DATABASE_PROTOCOL APPLE_KEY_MAP_DATABASE_PROTOCOL;

// KEY_MAP_CREATE_KEY_STROKES_BUFFER
/** Creates a new key stroke buffer with a given number of keys allocated.
    The index within the database is returned.

  @param[in]  This          Protocol instance pointer.
  @param[in]  BufferLength  The amount of keys to reserve.
  @param[out] Index         The assigned index of the created buffer.

  @retval EFI_SUCCESS           The key stroke buffer has been created.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval other                 An error returned by a sub-operation.
**/
typedef
EFI_STATUS
(EFIAPI *KEY_MAP_CREATE_KEY_STROKES_BUFFER)(
  IN  APPLE_KEY_MAP_DATABASE_PROTOCOL  *This,
  IN  UINTN                            BufferLength,
  OUT UINTN                            *Index
  );

// KEY_MAP_REMOVE_KEY_STROKES_BUFFER
/** Removes the key stroke buffer specified by its index from the database.

  @param[in] This   Protocol instance pointer.
  @param[in] Index  The index of the key stroke buffer to remove.

  @retval EFI_SUCCESS    The key stroke buffer has been removed.
  @retval EFI_NOT_FOUND  No key stroke buffer could be found for the given
                         index.
  @retval other          An error returned by a sub-operation.
**/
typedef
EFI_STATUS
(EFIAPI *KEY_MAP_REMOVE_KEY_STROKES_BUFFER)(
  IN APPLE_KEY_MAP_DATABASE_PROTOCOL  *This,
  IN UINTN                            Index
  );

// KEY_MAP_SET_KEY_STROKES_KEYS
/** Sets the key strokes of the key stroke buffer to the given KeyCodes buffer.

  @param[in] This              Protocol instance pointer.
  @param[in] Index             The index of the key stroke buffer to edit.
  @param[in] Modifiers         The key modifiers manipulating the given keys.
  @param[in] NumberOfKeyCodes  The number of keys contained in KeyCodes.
  @param[in] KeyCodes          An array of keys to add to the specified key
                               set.

  @retval EFI_SUCCESS           The given keys were set for the specified key
                                set.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_NOT_FOUND         No key stroke buffer could be found for the
                                given index.
  @retval other                 An error returned by a sub-operation.
**/
typedef
EFI_STATUS
(EFIAPI *KEY_MAP_SET_KEY_STROKES_KEYS)(
  IN APPLE_KEY_MAP_DATABASE_PROTOCOL  *This,
  IN UINTN                            Index,
  IN APPLE_MODIFIER_MAP               Modifiers,
  IN UINTN                            NumberOfKeyCodes,
  IN APPLE_KEY_CODE                   *KeyCodes
  );

// APPLE_KEY_MAP_DATABASE_PROTOCOL
struct APPLE_KEY_MAP_DATABASE_PROTOCOL {
  UINTN                             Revision;
  KEY_MAP_CREATE_KEY_STROKES_BUFFER CreateKeyStrokesBuffer;
  KEY_MAP_REMOVE_KEY_STROKES_BUFFER RemoveKeyStrokesBuffer;
  KEY_MAP_SET_KEY_STROKES_KEYS      SetKeyStrokeBufferKeys;
};

// gAppleKeyMapDatabaseProtocolGuid
extern EFI_GUID gAppleKeyMapDatabaseProtocolGuid;

#endif // APPLE_KEY_MAP_DATABASE_H
