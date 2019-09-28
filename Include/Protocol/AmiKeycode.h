/** @file
  Header file for AMI EfiKeycode protocol definitions.

Copyright (c) 2016, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef AMI_KEYCODE_H
#define AMI_KEYCODE_H

// 0ADFB62D-FF74-484C-8944-F85C4BEA87A8
#define AMI_EFIKEYCODE_PROTOCOL_GUID \
    { 0x0ADFB62D, 0xFF74, 0x484C, { 0x89, 0x44, 0xF8, 0x5C, 0x4B, 0xEA, 0x87, 0xA8 } }

extern EFI_GUID gAmiEfiKeycodeProtocolGuid;

typedef struct _AMI_EFIKEYCODE_PROTOCOL AMI_EFIKEYCODE_PROTOCOL;

#ifndef KEY_STATE_EXPOSED
#define KEY_STATE_EXPOSED   0x40
#endif

typedef struct {
  EFI_INPUT_KEY   Key;
  EFI_KEY_STATE   KeyState;
  EFI_KEY         EfiKey;
  UINT8           EfiKeyIsValid;
  UINT8           PS2ScanCode;
  UINT8           PS2ScanCodeIsValid;
} AMI_EFI_KEY_DATA;

typedef EFI_STATUS (EFIAPI *AMI_READ_EFI_KEY) (
  IN  AMI_EFIKEYCODE_PROTOCOL  *This,
  OUT AMI_EFI_KEY_DATA          *KeyData
  );

typedef
EFI_STATUS
(EFIAPI *AMI_RESET_EX) (
  IN AMI_EFIKEYCODE_PROTOCOL   *This,
  IN BOOLEAN                   ExtendedVerification
  );

struct _AMI_EFIKEYCODE_PROTOCOL {
  AMI_RESET_EX                      Reset;
  AMI_READ_EFI_KEY                  ReadEfikey;
  EFI_EVENT                         WaitForKeyEx;
  EFI_SET_STATE                     SetState;
  EFI_REGISTER_KEYSTROKE_NOTIFY     RegisterKeyNotify;
  EFI_UNREGISTER_KEYSTROKE_NOTIFY   UnregisterKeyNotify;
};

#endif // AMI_KEYCODE_H
