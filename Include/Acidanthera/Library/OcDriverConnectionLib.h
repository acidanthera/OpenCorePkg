/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DRIVER_CONNECTION_LIB_H
#define OC_DRIVER_CONNECTION_LIB_H

/**
  Registers given PriorityDrivers to highest priority during connecting controllers.
  Does this by installing custom EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL
  or by overriding existing EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL.GetDriver.

  @param[in]  PriorityDrivers  NULL-terminated list of drivers to prioritise.

  @retval  EFI_SUCCESS on successful override or installation.
**/
EFI_STATUS
OcRegisterDriversToHighestPriority (
  IN EFI_HANDLE  *PriorityDrivers
  );

/**
  Unblocks all partition handles without a File System protocol attached from
  driver connection, if applicable.
**/
VOID
OcUnblockUnmountedPartitions (
  VOID
  );

/**
  Disconnect effectively all drivers attached at handle.

  @param[in] Controller    Handle to disconnect the drivers from.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDisconnectDriversOnHandle (
  IN EFI_HANDLE  Controller
  );

/**
  Connect effectively all drivers to effectively all handles.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcConnectDrivers (
  VOID
  );

#endif // OC_DRIVER_CONNECTION_LIB_H
