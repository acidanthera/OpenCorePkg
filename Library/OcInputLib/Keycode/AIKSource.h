/** @file
  Key provider

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef AIK_SOURCE_H
#define AIK_SOURCE_H

#include <Protocol/AmiKeycode.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <Library/OcInputLib.h>

typedef struct {
  //
  // Preserved handle of gST->ConsoleInHandle
  //
  EFI_HANDLE                         ConSplitHandler;

  //
  // Solved input protocol instances from ConSplitHandler
  // We override their ReadKey handlers and implement them
  // ourselves via polled data from one of these protocols.
  // Polled proto is prioritised as present: AMI, EX, Legacy.
  //
  AMI_EFIKEYCODE_PROTOCOL            *AmiKeycode;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     *TextInput;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TextInputEx;

  //
  // Original implementations of the protocols.
  //
  AMI_READ_EFI_KEY                   AmiReadEfikey;
  EFI_EVENT                          AmiWait;
  EFI_EVENT                          TextWait;
  EFI_INPUT_READ_KEY                 TextReadKeyStroke;
  EFI_INPUT_READ_KEY_EX              TextReadKeyStrokeEx;
  EFI_EVENT                          TextWaitEx;
} AIK_SOURCE;

EFI_STATUS
AIKSourceGrabEfiKey (
  AIK_SOURCE        *Source,
  AMI_EFI_KEY_DATA  *KeyData,
  BOOLEAN           KeyFiltering
  );

EFI_STATUS
AIKSourceInstall (
  AIK_SOURCE         *Source,
  OC_INPUT_KEY_MODE  Mode
  );

VOID
AIKSourceUninstall (
  AIK_SOURCE  *Source
  );

#endif
