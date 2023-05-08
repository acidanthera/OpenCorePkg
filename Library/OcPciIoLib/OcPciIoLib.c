/** @file
   PciIo overrides

   Copyright (c) 2023, xCuri0. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include "OcPciIoU.h"
#include <Library/OcPciIoLib.h>

EFI_PCI_IO_PROTOCOL *
OcPciIoInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_TPL                          tpl;
  EFI_STATUS                       Status = 0;
  EFI_CPU_IO2_PROTOCOL             *mCpuIo;
  UINTN                            HandleCount;
  EFI_HANDLE                       *HandleBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;
  UINTN                            Index;

  if (Reinstall) {
    // TODO: if we need it
  } else {
    DEBUG ((DEBUG_INFO, "OCPIO: Init CpuIo2\n"));
    mCpuIo = InitializeCpuIo2 ();
    DEBUG ((DEBUG_INFO, "OCPIO: Fixing CpuIo2\n"));

    tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    if (EFI_ERROR (Status)) {
      return NULL;
    }

    // Override with 64-bit MMIO compatible functions
    mCpuIo->Mem.Read  = CpuMemoryServiceRead;
    mCpuIo->Mem.Write = CpuMemoryServiceWrite;

    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );

    for (Index = 0; Index < HandleCount; ++Index) {
      DEBUG ((DEBUG_INFO, "OCPIO: Fixing PciRootBridgeIo %d\n", Index));
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiPciRootBridgeIoProtocolGuid,
                      (VOID **)&PciRootBridgeIo
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      // Override with 64-bit MMIO compatible functions
      PciRootBridgeIo->Mem.Read  = RootBridgeIoMemRead;
      PciRootBridgeIo->Mem.Write = RootBridgeIoMemWrite;
    }

    FreePool (HandleBuffer);

    gBS->RestoreTPL (tpl);

    return NULL;
  }

  return NULL;
}
