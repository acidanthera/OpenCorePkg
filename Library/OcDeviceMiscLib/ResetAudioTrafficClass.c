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
ResetAudioTrafficClass (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  EFI_HANDLE           *HandleBuffer;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_CLASSCODE        ClassCode;
  UINT8                TrafficClass;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: No PCI devices for TCSEL reset - %r\n", Status));
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

    if (ClassCode.BaseCode == PCI_CLASS_MEDIA &&
      (ClassCode.SubClassCode == PCI_CLASS_MEDIA_AUDIO ||
      ClassCode.SubClassCode == PCI_CLASS_MEDIA_HDA)) {
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_MEDIA_TCSEL_OFFSET, 1, &TrafficClass);
      if (EFI_ERROR (Status)) {
        continue;
      }

      DEBUG ((
        DEBUG_INFO,
        "OCDM: Discovered audio device at %u/%u with TCSEL %X\n",
        (UINT32) (Index + 1),
        (UINT32) HandleCount,
        TrafficClass
        ));

      //
      // Update Traffic Class Select Register to TC0.
      // This is required for AppleHDA to output audio on some machines.
      // See Intel I/O Controller Hub 9 (ICH9) Family Datasheet for more details.
      //
      if ((TrafficClass & TCSEL_CLASS_MASK) != 0) {
        TrafficClass &= ~TCSEL_CLASS_MASK;
        PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, PCI_MEDIA_TCSEL_OFFSET, 1, &TrafficClass);
      }
    }
  }
}
