/** @file
  SimpleTextInputEx protocol implementation.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_SIMPLE_TEXT_INPUT_LIB_H
#define OC_SIMPLE_TEXT_INPUT_LIB_H

#include <Protocol/SimpleTextInEx.h>

/**
  Install and initialise SimpleTextInputEx protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *
OcSimpleTextInputExInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_SIMPLE_TEXT_INPUT_LIB_H
