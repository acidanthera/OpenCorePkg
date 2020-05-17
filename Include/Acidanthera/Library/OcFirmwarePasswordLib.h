/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_FIRMWARE_PASSWORD_LIB_H
#define OC_FIRMWARE_PASSWORD_LIB_H

// OcFirmwarePasswordInstallProtocol
/// Install the Apple Firmware Password protocol
///
/// @param[in] ImageHandle      The firmware allocated handle for the EFI image.
/// @param[in] SystemTable      A pointer to the EFI System Table.
///
/// @retval EFI_SUCCESS         The entry point is executed successfully.

EFI_STATUS
OcFirmwarePasswordInstallProtocol (
  IN  EFI_HANDLE ImageHandle,
  IN  EFI_SYSTEM_TABLE *SystemTable
  );

#endif // OC_FIRMWARE_PASSWORD_LIB_H
