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
  UINT32               ClassCode;
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
      EfiPciIoWidthUint32,
      OFFSET_OF (PCI_DEVICE_INDEPENDENT_REGION, RevisionID),
      1,
      &ClassCode
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    ClassCode >>= 16U; ///< Drop revision and minor codes.
    if (ClassCode == (PCI_CLASS_BRIDGE << 8 | PCI_CLASS_BRIDGE_ISA)) {
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0xF0 /* RCBA */, 1, &Rcba);
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
      if ((Rcba & 0xFFFFC000) == 0) {
        continue;
      }

      //
      // Disabled access. Try to enable.
      //
      if ((Rcba & BIT0) == 0) {
        Rcba |= BIT0;
        PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0xF0 /* RCBA */, 1, &Rcba);
      }

      Rcba &= 0xFFFFC000;

      Hptc = MmioRead32 (Rcba + 0x3404);

      DEBUG ((
        DEBUG_INFO,
        "OCDM: Discovered HPTC register with 0x%X value\n",
        Hptc
        ));

      if ((Hptc & BIT7) == 0) {
        MmioWrite32 (Rcba + 0x3404, Hptc | BIT7);
        Hptc = MmioRead32 (Rcba + 0x3404);

        DEBUG ((
          DEBUG_INFO,
          "OCDM: Updated HPTC register with HPET has 0x%X value\n",
          Hptc
          ));
      }
    }
  }
}
