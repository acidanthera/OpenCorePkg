/** @file
   PciIo overrides

   Copyright (c) 2023, xCuri0. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include "OcPciIoU.h"
#include <Library/OcPciIoLib.h>

EFI_CPU_IO2_PROTOCOL *
OcPciIoInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_TPL                          tpl;
  UINTN                            Index;
  EFI_STATUS                       Status  = 0;
  EFI_CPU_IO2_PROTOCOL             *mCpuIo = NULL;
  UINTN                            HandleCount;
  EFI_HANDLE                       *HandleBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;

  if (Reinstall) {
    return NULL;
  } else {
    mCpuIo = InitializeCpuIo2 ();

    tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    if (!mCpuIo) {
      return NULL;
    }

    DEBUG ((DEBUG_INFO, "OCPIO: Fixing CpuIo2\n"));
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

    if (EFI_ERROR (Status)) {
      mCpuIo = NULL;
      goto Free;
    }

    for (Index = 0; Index < HandleCount; ++Index) {
      DEBUG ((DEBUG_INFO, "OCPIO: Fixing PciRootBridgeIo %d\n", Index));
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiPciRootBridgeIoProtocolGuid,
                      (VOID **)&PciRootBridgeIo
                      );
      if (EFI_ERROR (Status)) {
        mCpuIo = NULL;
        goto Free;
      }

      // Override with 64-bit MMIO compatible functions
      PciRootBridgeIo->Mem.Read  = RootBridgeIoMemRead;
      PciRootBridgeIo->Mem.Write = RootBridgeIoMemWrite;
    }

    gBS->RestoreTPL (tpl);
  }

Free:
  FreePool (HandleBuffer);
  return mCpuIo;
}
