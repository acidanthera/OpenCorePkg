/** @file
  Copyright (C) 2023, xCuri0. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_PCI_IO_LIB_H
#define OC_PCI_IO_LIB_H

#include <Protocol/CpuIo2.h>

/**
  The user Entry Point for PciIo.

  This function fixes PciIo related protocols.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_CPU_IO2_PROTOCOL *
OcPciIoInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_PCI_IO_LIB_H
