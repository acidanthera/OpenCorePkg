/** @file
  Common variable non-volatile store routines.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_NON_VOLATILE_H_
#define _VARIABLE_NON_VOLATILE_H_

#include "Variable.h"

/**
  Get non-volatile maximum variable size.

  @return Non-volatile maximum variable size.

**/
UINTN
GetNonVolatileMaxVariableSize (
  VOID
  );

/**
  Init emulated non-volatile variable store.

  @param[out] VariableStoreBase Output pointer to emulated non-volatile variable store base.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
InitEmuNonVolatileVariableStore (
  EFI_PHYSICAL_ADDRESS  *VariableStoreBase
  );

/**
  Init real non-volatile variable store.

  @param[out] VariableStoreBase Output pointer to real non-volatile variable store base.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.
  @retval EFI_VOLUME_CORRUPTED  Variable Store or Firmware Volume for Variable Store is corrupted.

**/
EFI_STATUS
InitRealNonVolatileVariableStore (
  OUT EFI_PHYSICAL_ADDRESS  *VariableStoreBase
  );

/**
  Init non-volatile variable store.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.
  @retval EFI_VOLUME_CORRUPTED  Variable Store or Firmware Volume for Variable Store is corrupted.

**/
EFI_STATUS
InitNonVolatileVariableStore (
  VOID
  );

#endif
