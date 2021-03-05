/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_KEY_MAP_AGGREGATOR_H
#define APPLE_KEY_MAP_AGGREGATOR_H

#include <IndustryStandard/AppleHid.h>

// APPLE_KEY_MAP_AGGREGATOR_PROTOCOL_REVISION
#define APPLE_KEY_MAP_AGGREGATOR_PROTOCOL_REVISION  0x00010000

// APPLE_KEY_MAP_AGGREGATOR_PROTOCOL_GUID
#define APPLE_KEY_MAP_AGGREGATOR_PROTOCOL_GUID            \
  { 0x5B213447, 0x6E73, 0x4901,                           \
    { 0xA4, 0xF1, 0xB8, 0x64, 0xF3, 0xB7, 0xA1, 0x72 } }

typedef
struct APPLE_KEY_MAP_AGGREGATOR_PROTOCOL
APPLE_KEY_MAP_AGGREGATOR_PROTOCOL;

// KEY_MAP_GET_KEY_STROKES
/** Returns all pressed keys and key modifiers into the appropiate buffers.

  @param[in]     This              A pointer to the protocol instance.
  @param[out]    Modifiers         The modifiers manipulating the given keys.
  @param[in,out] NumberOfKeyCodes  On input the number of keys allocated.
                                   On output the number of keys returned into
                                   KeyCodes.
  @param[out]    KeyCodes          A Pointer to a caller-allocated buffer in
                                   which the pressed keys get returned.

  @retval EFI_SUCCESS              The pressed keys have been returned into
                                   KeyCodes.
  @retval EFI_BUFFER_TOO_SMALL     The memory required to return the value exceeds
                                   the size of the allocated Buffer.
                                   The required number of keys to allocate to
                                   complete the operation has been returned into
                                   NumberOfKeyCodes.
  @retval other                    An error returned by a sub-operation.
**/
typedef
EFI_STATUS
(EFIAPI *KEY_MAP_GET_KEY_STROKES)(
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *This,
     OUT APPLE_MODIFIER_MAP                 *Modifiers,
  IN OUT UINTN                              *NumberOfKeyCodes,
     OUT APPLE_KEY_CODE                     *KeyCodes OPTIONAL
  );

// KEY_MAP_CONTAINS_KEY_STROKES
/** Returns whether or not a list of keys and their modifiers are part of the
    database of pressed keys.

  @param[in]     This              Protocol instance pointer.
  @param[in]     Modifiers         The modifiers manipulating the given keys.
  @param[in]     NumberOfKeyCodes  The number of keys present in KeyCodes.
  @param[in,out] KeyCodes          The list of keys to check for.  The
                                   children may be sorted in the process.
  @param[in]     ExactMatch        Specifies whether Modifiers and KeyCodes
                                   should be exact matches or just contained.

  @retval EFI_SUCCESS    The queried keys are part of the database.
  @retval EFI_NOT_FOUND  The queried keys could not be found.
**/
typedef
EFI_STATUS
(EFIAPI *KEY_MAP_CONTAINS_KEY_STROKES)(
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *This,
  IN     APPLE_MODIFIER_MAP                 Modifiers,
  IN     UINTN                              NumberOfKeyCodes,
  IN OUT APPLE_KEY_CODE                     *KeyCodes,
  IN     BOOLEAN                            ExactMatch
  );

// APPLE_KEY_MAP_AGGREGATOR_PROTOCOL
struct APPLE_KEY_MAP_AGGREGATOR_PROTOCOL {
  UINT64                       Revision;
  KEY_MAP_GET_KEY_STROKES      GetKeyStrokes;
  KEY_MAP_CONTAINS_KEY_STROKES ContainsKeyStrokes;
};

// gAppleKeyMapAggregatorProtocolGuid
extern EFI_GUID gAppleKeyMapAggregatorProtocolGuid;

#endif // APPLE_KEY_MAP_AGGREGATOR_H
