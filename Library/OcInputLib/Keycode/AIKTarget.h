/** @file
  Key consumer

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef AIK_TARGET_H
#define AIK_TARGET_H

#include <IndustryStandard/AppleHid.h>
#include <Protocol/AppleKeyMapDatabase.h>
#include <Protocol/AmiKeycode.h>

//
// Maximum amount of keys reported to Apple protocols
//
#define AIK_TARGET_BUFFER_SIZE 6

//
// Known values for key repeat (single key hold):
// VMware - 2, APTIO V - 3 or 4
// Known values for different keys (quick press one after other):
// VMware - 6+, APTIO V - 10+
// Known values for simultaneous keys (dual key hold):
// VMware - 2, APTIO V - 1
//

typedef struct {
  //
  // Apple output protocol we submit data to.
  //
  APPLE_KEY_MAP_DATABASE_PROTOCOL    *KeyMapDb;

  //
  // Apple output buffer index
  //
  UINTN                              KeyMapDbIndex;

  //
  // Apple modifier map (previously reported)
  //
  APPLE_MODIFIER_MAP                 Modifiers;

  //
  // Previously reported Apple modifiers timestamp
  //
  UINT64                             ModifierCounter;

  //
  // Previously reported Apple active keys
  //
  APPLE_KEY_CODE                     Keys[AIK_TARGET_BUFFER_SIZE];

  //
  // Previously reported Apple key timestamps
  //
  UINT64                             KeyCounters[AIK_TARGET_BUFFER_SIZE];

  //
  // Amount of active keys previously reported
  //
  UINTN                              NumberOfKeys;

  //
  // Timestamp counter incremented every refresh
  //
  UINT64                             Counter;

  //
  // Remove key if it was not submitted after this value.
  //
  UINT8                              KeyForgotThreshold;
} AIK_TARGET;

EFI_STATUS
AIKTargetInstall (
  IN OUT AIK_TARGET  *Target,
  IN     UINT8       KeyForgotThreshold
  );

VOID
AIKTargetUninstall (
  IN OUT AIK_TARGET  *Target
  );

UINT64
AIKTargetRefresh (
  IN OUT AIK_TARGET  *Target
  );

VOID
AIKTargetWriteEntry (
  IN OUT AIK_TARGET        *Target,
  IN     AMI_EFI_KEY_DATA  *KeyData
  );

VOID
AIKTargetSubmit (
  IN OUT AIK_TARGET        *Target
  );

#endif
