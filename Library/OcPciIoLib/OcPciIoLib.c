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
  EFI_TPL                          Tpl;
  UINTN                            Index;
  EFI_STATUS                       Status;
  EFI_CPU_IO2_PROTOCOL             *mCpuIo;
  UINTN                            HandleCount;
  EFI_HANDLE                       *HandleBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;

  mCpuIo = InitializeCpuIo2 ();
  if (!mCpuIo) {
    return NULL;
  }

  if (Reinstall) {
    return mCpuIo;
  } else {
    Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
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

    DEBUG ((DEBUG_INFO, "OCPIO: Fixing CpuIo2\n"));
    // Override with 64-bit MMIO compatible functions
    mCpuIo->Mem.Read  = CpuMemoryServiceRead;
    mCpuIo->Mem.Write = CpuMemoryServiceWrite;

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
  }

Free:
  gBS->RestoreTPL (Tpl);
  FreePool (HandleBuffer);
  return mCpuIo;
}
