/** @file
  OVMF hardware reset implementation.

  Copyright (c) 2021-2022, vit9696, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_DIRECT_RESET_LIB_H
#define OC_DIRECT_RESET_LIB_H

#include <Uefi.h>

/**
  Perform cold reboot directly bypassing UEFI services. Does not return.
  Supposed to work in any modern physical or virtual environment.
**/
VOID
DirectResetCold (
  VOID
  );

#endif // OC_DIRECT_RESET_LIB_H
