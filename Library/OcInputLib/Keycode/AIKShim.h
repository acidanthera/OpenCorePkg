/** @file
  Key shimming code

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef AIK_SHIM_H
#define AIK_SHIM_H

#include <Protocol/AmiKeycode.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

EFI_STATUS
EFIAPI
AIKShimAmiKeycodeReadEfikey (
  IN  AMI_EFIKEYCODE_PROTOCOL  *This,
  OUT AMI_EFI_KEY_DATA         *KeyData
  );

EFI_STATUS
EFIAPI
AIKShimTextInputReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  );

EFI_STATUS
EFIAPI
AIKShimTextInputReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  );

VOID
EFIAPI
AIKShimWaitForKeyHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

#endif

