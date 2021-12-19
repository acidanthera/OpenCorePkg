/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/Pci.h>

#include <Protocol/PciIo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDeviceMiscLib.h>

#include "PciExtInternal.h"

VOID
ActivateHpetSupport (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  EFI_HANDLE           *HandleBuffer;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_CLASSCODE        ClassCode;
  UINT32               Rcba;
  UINT32               Hptc;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: No PCI devices for HPET support - %r\n", Status));
    return;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiPciIoProtocolGuid,
      (VOID **) &PciIo
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint8,
      PCI_CLASSCODE_OFFSET,
      sizeof (PCI_CLASSCODE) / sizeof (UINT8),
      &ClassCode
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (ClassCode.BaseCode == PCI_CLASS_BRIDGE && ClassCode.SubClassCode == PCI_CLASS_BRIDGE_ISA) {
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, PCI_BRIDGE_RCBA_OFFSET, 1, &Rcba);
      if (EFI_ERROR (Status)) {
        continue;
      }

      DEBUG ((
        DEBUG_INFO,
        "OCDM: Discovered RCBA device at %u/%u at 0x%X\n",
        (UINT32) (Index + 1),
        (UINT32) HandleCount,
        Rcba
        ));

      //
      // Disabled completely. Ignore.
      //
      if ((Rcba & PCI_BRIDGE_RCBA_ADDRESS_MASK) == 0) {
        continue;
      }

      //
      // Disabled access. Try to enable.
      //
      if ((Rcba & PCI_BRIDGE_RCBA_ACCESS_ENABLE) == 0) {
        Rcba |= PCI_BRIDGE_RCBA_ACCESS_ENABLE;
        PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, PCI_BRIDGE_RCBA_OFFSET, 1, &Rcba);
      }

      Rcba &= PCI_BRIDGE_RCBA_ADDRESS_MASK;

      Hptc = MmioRead32 (Rcba + RCBA_HTPC_REGISTER);

      DEBUG ((
        DEBUG_INFO,
        "OCDM: Discovered HPTC register with 0x%X value\n",
        Hptc
        ));

      if ((Hptc & RCBA_HTPC_HPET_ENABLE) == 0) {
        MmioWrite32 (Rcba + RCBA_HTPC_REGISTER, Hptc | RCBA_HTPC_HPET_ENABLE);
        Hptc = MmioRead32 (Rcba + RCBA_HTPC_REGISTER);

        DEBUG ((
          DEBUG_INFO,
          "OCDM: Updated HPTC register with HPET has 0x%X value\n",
          Hptc
          ));
      }
    }
  }
}
