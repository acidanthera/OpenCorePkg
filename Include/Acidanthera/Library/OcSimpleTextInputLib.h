/** @file
  SimpleTextInputEx protocol implementation on top of AppleEvent and SimpleTextInput protocols.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_SIMPLE_TEXT_INPUT_LIB_H
#define OC_SIMPLE_TEXT_INPUT_LIB_H

#include <Protocol/SimpleTextInEx.h>

/**
  Install and initialise SimpleTextInEx protocol.

  @retval installed or located protocol or NULL.
**/
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *
OcSimpleTextInputExInstallProtocol (
  VOID
  );

#endif // OC_SIMPLE_TEXT_INPUT_LIB_H
