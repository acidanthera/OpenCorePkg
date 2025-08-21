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

/**
  Process key data and handle control characters.

  This function is provided by OcTextInputLib for use by standalone drivers.
  It handles control character mapping and key processing logic.

  @param[in,out] KeyData  Key data to process

  @retval EFI_SUCCESS     Key data processed successfully
  @retval EFI_NOT_FOUND   Key data does not need processing
**/
EFI_STATUS
OctiProcessKeyData (
  IN OUT EFI_KEY_DATA  *KeyData
  );

#endif // OC_TEXT_INPUT_COMMON_H
