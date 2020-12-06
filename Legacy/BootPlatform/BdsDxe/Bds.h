/** @file
  Head file for BDS Architectural Protocol implementation

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BDS_MODULE_H_
#define _BDS_MODULE_H_

#include <Protocol/Bds.h>

/**

  Install Boot Device Selection Protocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  BDS has finished initializing.
                        Return the dispatcher and recall BDS.Entry
  @retval  Other        Return status from AllocatePool() or gBS->InstallProtocolInterface

**/
EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                     ImageHandle,
  IN EFI_SYSTEM_TABLE               *SystemTable
  );

/**

  Service routine for BdsInstance->Entry(). Devices are connected, the
  consoles are initialized, and the boot options are tried.

  @param This            Protocol Instance structure.

**/
VOID
EFIAPI
BdsEntry (
  IN  EFI_BDS_ARCH_PROTOCOL *This
  );

#endif
