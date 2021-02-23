/** @file
  Header file for AmiEfiKeycode to KeyMapDb translator.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef AIK_SELF_H
#define AIK_SELF_H

#include "AIKData.h"
#include "AIKSource.h"
#include "AIKTarget.h"
#include "AIKTranslate.h"

#include <Library/OcInputLib.h>
#include <Library/UefiLib.h>

//
// Maximum amount of keys polled at once
//
#define AIK_KEY_POLL_LIMIT (AIK_TARGET_BUFFER_SIZE)

//
// Defines key polling frequency
//
#define AIK_KEY_POLL_INTERVAL EFI_TIMER_PERIOD_MILLISECONDS(10)

typedef struct {
  //
  // Input mode
  //
  OC_INPUT_KEY_MODE     Mode;

  //
  // Remove key if it was not submitted after this value.
  //
  UINT8                 KeyForgotThreshold;

  //
  // Perform ASCII and scan code input key filtering.
  //
  BOOLEAN               KeyFiltering;

  //
  // Input sources
  //
  AIK_SOURCE            Source;

  //
  // Output targets
  //
  AIK_TARGET            Target;

  //
  // Key data
  //
  AIK_DATA              Data;

  //
  // AppleKeyMapAggregator polling event
  //
  EFI_EVENT             KeyMapDbArriveEvent;

  //
  // Keyboard input polling event
  //
  EFI_EVENT             PollKeyboardEvent;

  //
  // Indicates keyboard input polling event to avoid reentrancy if any
  //
  BOOLEAN               InPollKeyboardEvent;

  //
  // Indicates we are done for any event in case it gets fired.
  // Not really needed. Added in case of bogus firmware.
  //
  BOOLEAN               OurJobIsDone;
} AIK_SELF;

//
// This is only used in Shims, where no other way exists.
//
extern AIK_SELF  gAikSelf;

EFI_STATUS
AIKProtocolArriveInstall (
  AIK_SELF  *Keycode
  );

VOID
AIKProtocolArriveUninstall (
  AIK_SELF  *Keycode
  );

EFI_STATUS
AIKInstall (
  AIK_SELF  *Keycode
  );

VOID
AIKUninstall (
  AIK_SELF  *Keycode
  );

#endif
