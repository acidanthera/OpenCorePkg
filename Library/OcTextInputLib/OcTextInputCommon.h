/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_TEXT_INPUT_COMMON_H
#define OC_TEXT_INPUT_COMMON_H

#include <Uefi.h>
#include <Protocol/SimpleTextInEx.h>

//
// Control character table for mapping scan codes to shift states
//
STATIC CONST UINT16  mOctiControlCharTable[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00 - 0x07
  0x08, 0x09, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00,  // 0x08 - 0x0F
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 - 0x17
  0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00   // 0x18 - 0x1F
};

/**
  Process key data and handle control characters.

  @param[in,out] KeyData  Key data to process

  @retval EFI_SUCCESS     Key data processed successfully
  @retval EFI_NOT_FOUND   Key data does not need processing
**/
STATIC
EFI_STATUS
OctiProcessKeyData (
  IN OUT EFI_KEY_DATA  *KeyData
  )
{
  UINT16  ScanCode;
  UINT16  UnicodeChar;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ScanCode    = KeyData->Key.ScanCode;
  UnicodeChar = KeyData->Key.UnicodeChar;

  //
  // Handle control characters
  //
  if ((ScanCode < ARRAY_SIZE (mOctiControlCharTable)) && (mOctiControlCharTable[ScanCode] != 0)) {
    KeyData->Key.UnicodeChar = mOctiControlCharTable[ScanCode];
    DEBUG ((DEBUG_VERBOSE, "OCTI: Mapped scan code 0x%X to control character 0x%X\n", ScanCode, KeyData->Key.UnicodeChar));
    return EFI_SUCCESS;
  }

  //
  // Handle standard printable characters
  //
  if ((UnicodeChar >= 0x20) && (UnicodeChar <= 0x7E)) {
    DEBUG ((DEBUG_VERBOSE, "OCTI: Standard printable character 0x%X\n", UnicodeChar));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_VERBOSE, "OCTI: Unhandled key - ScanCode: 0x%X, UnicodeChar: 0x%X\n", ScanCode, UnicodeChar));
  return EFI_NOT_FOUND;
}

#endif // OC_TEXT_INPUT_COMMON_H
