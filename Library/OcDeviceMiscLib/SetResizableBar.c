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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDeviceMiscLib.h>

#include "PciExtInternal.h"

STATIC
EFI_STATUS
LocatePciCapability (
  IN  EFI_PCI_IO_PROTOCOL *PciIo,
  IN  UINT16              CapId,
  OUT UINT32              *Offset
  )
{
  EFI_STATUS  Status;
  UINT32      CapabilityPtr;
  UINT32      CapabilityEntry;
  UINT16      CapabilityID;

  CapabilityPtr = EFI_PCIE_CAPABILITY_BASE_OFFSET;

  while (CapabilityPtr != 0) {
    //
    // Mask it to DWORD alignment per PCI spec
    //
    CapabilityPtr &= 0xFFC;
    Status = PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint32,
      CapabilityPtr,
      1,
      &CapabilityEntry
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCDM: Capability I/O error - %r\n", Status));
      return EFI_DEVICE_ERROR;
    }

    if (CapabilityEntry == MAX_UINT32) {
      DEBUG ((DEBUG_INFO, "OCDM: Read from disabled device\n"));
      return EFI_INVALID_PARAMETER;
    }

    CapabilityID = (UINT16) CapabilityEntry;

    if (CapabilityID == CapId) {
      DEBUG ((DEBUG_VERBOSE, "OCDM: Found CAP 0x%X at 0x%X\n", CapabilityID, CapabilityPtr));
      *Offset = CapabilityPtr;
      return EFI_SUCCESS;
    }

    CapabilityPtr = (CapabilityEntry >> 20) & 0xFFF;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
SetResizableBarOnDevice (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN PCI_BAR_SIZE         Size,
  IN BOOLEAN              Increase
  )
{
  EFI_STATUS                                               Status;
  UINT32                                                   ResizableBarOffset;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY    Entries[PCI_MAX_BAR];
  PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_CONTROL  ResizableBarControl;
  UINT32                                                   Offset;
  UINT32                                                   Index;
  UINT32                                                   ResizableBarNumber;
  UINT64                                                   Capabilities;
  UINT64                                                   NewCapabilities;
  UINT32                                                   OldBar[PCI_MAX_BAR];
  UINT32                                                   NewBar[PCI_MAX_BAR];
  INTN                                                     Bit;
  BOOLEAN                                                  ChangedBars;

  ChangedBars = FALSE;

  Status = LocatePciCapability (
    PciIo,
    PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID,
    &ResizableBarOffset
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: RBAR is unsupported by device - %r\n", Status));
    return EFI_UNSUPPORTED;
  }

  ResizableBarControl.Uint32 = 0;
  Offset = ResizableBarOffset + sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER)
    + sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_CAPABILITY);
  Status = PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint8,
    Offset,
    sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_CONTROL),
    &ResizableBarControl
    );

  DEBUG ((
    DEBUG_INFO,
    "OCDM: RBAR control is %X, total %u - %r\n",
    ResizableBarControl.Uint32,
    MIN (ResizableBarControl.Bits.ResizableBarNumber, PCI_MAX_BAR),
    Status
    ));

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  ResizableBarNumber = MIN (ResizableBarControl.Bits.ResizableBarNumber, PCI_MAX_BAR);

  Status = PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint8,
    ResizableBarOffset + sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER),
    sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY) * ResizableBarNumber,
    (VOID *) Entries
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: RBAR caps cannot be read - %r\n", Status));
    return EFI_UNSUPPORTED;
  }

  Status = PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    OFFSET_OF (PCI_TYPE00, Device.Bar),
    PCI_MAX_BAR,
    (VOID *) OldBar
    );
  if (EFI_ERROR (Status)) {
    ZeroMem (OldBar, sizeof (OldBar));
  }

  DEBUG ((
    DEBUG_INFO,
    "OCDM: Old BAR %08X %08X %08X %08X %08X %08X - %r\n",
    OldBar[0],
    OldBar[1],
    OldBar[2],
    OldBar[3],
    OldBar[4],
    OldBar[5],
    Status
    ));

  for (Index = 0; Index < ResizableBarNumber; Index++) {
    //
    // When the bit of Capabilities Set, indicates that the Function supports
    // operating with the BAR sized to (2^Bit) MB.
    // Example:
    // Bit 0 is set: supports operating with the BAR sized to 1 MB
    // Bit 1 is set: supports operating with the BAR sized to 2 MB
    // Bit n is set: supports operating with the BAR sized to (2^n) MB
    //
    // Reference values for RX 6900 with two resizable BARs.
    // Disabled values:
    //   Resizeable Bar Capability [1]
    //     ResizableBarCapability         0007F000
    //     ResizableBarControl            0840
    //   Resizeable Bar Capability [2]
    //     ResizableBarCapability         00001FE0
    //     ResizableBarControl            0102
    // Enabled values:
    //   Resizeable Bar Capability [1]
    //     ResizableBarCapability         0007F000
    //     ResizableBarControl            0E40
    //   Resizeable Bar Capability [2]
    //     ResizableBarCapability         00001FE0
    //     ResizableBarControl            0802
    //
    NewCapabilities = Capabilities = LShiftU64 (Entries[Index].ResizableBarControl.Bits.BarSizeCapability, 28)
      | Entries[Index].ResizableBarCapability.Bits.BarSizeCapability;

    //
    // Restrict supported BARs to specified value.
    //
    NewCapabilities &= PCI_BAR_CAP_LIMIT (Size);

    //
    // Disable bits higher than current as we are not allowed to increase bar size
    // more than we already have.
    //
    if (!Increase) {
      NewCapabilities &= PCI_BAR_CAP_LIMIT (Entries[Index].ResizableBarControl.Bits.BarSize);
    }

    //
    // If requested BAR size is too low, choose the lowest available BAR size.
    //
    if (NewCapabilities == 0
      && Entries[Index].ResizableBarControl.Bits.BarSize > (UINT32) Size) {
      Bit = LowBitSet64 (Capabilities);
    } else {
      Bit = HighBitSet64 (NewCapabilities);
    }

    DEBUG ((
      DEBUG_INFO,
      "OCDM: RBAR %u/%u supports 0x%Lx, sizing %u inc %d results setting from %u to %d\n",
      Index + 1,
      ResizableBarNumber,
      Capabilities,
      Size,
      Increase,
      Entries[Index].ResizableBarControl.Bits.BarSize,
      (INT32) Bit
      ));

    //
    // If we have no supported configuration, just skip.
    //
    if (Bit < 0 || Entries[Index].ResizableBarControl.Bits.BarSize == (UINT32) Bit) {
      continue;
    }

    Offset = ResizableBarOffset + sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER)
            + Index * sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY)
            + OFFSET_OF (PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY, ResizableBarControl);

    Entries[Index].ResizableBarControl.Bits.BarSize = (UINT32) Bit;
    PciIo->Pci.Write (
      PciIo,
      EfiPciIoWidthUint32,
      Offset,
      1,
      &Entries[Index].ResizableBarControl.Uint32
      );

    ChangedBars = TRUE;
  }

  if (ChangedBars) {
    DEBUG_CODE_BEGIN ();

    Status = PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint32,
      OFFSET_OF (PCI_TYPE00, Device.Bar),
      PCI_MAX_BAR,
      (VOID *) NewBar
      );
    if (EFI_ERROR (Status)) {
      ZeroMem (NewBar, sizeof (NewBar));
    }

    DEBUG ((
      DEBUG_INFO,
      "OCDM: New BAR %08X %08X %08X %08X %08X %08X - %r\n",
      NewBar[0],
      NewBar[1],
      NewBar[2],
      NewBar[3],
      NewBar[4],
      NewBar[5],
      Status
      ));

    DEBUG_CODE_END ();

    //
    // PCI BARs are reset after resizing, so we must restore them. This follows the spec:
    //   After writing the BAR Size field, the contents of the corresponding BAR are undefined.
    //   To ensure that it contains a valid address after resizing the BAR, system software must
    //   reprogram the BAR, and Set the Memory Space Enable bit (unless the resource is not allocated).
    // TODO: We do not bother touching `Memory Space Enable` bit but strictly we should.
    //
    if (!IsZeroBuffer (OldBar, sizeof (OldBar))) {
      Status = PciIo->Pci.Write (
        PciIo,
        EfiPciIoWidthUint32,
        OFFSET_OF (PCI_TYPE00, Device.Bar),
        PCI_MAX_BAR,
        (VOID *) OldBar
        );
      DEBUG ((DEBUG_INFO, "OCDM: Reprogrammed BARs to original - %r\n", Status));
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ResizeGpuBars (
  IN PCI_BAR_SIZE  Size,
  IN BOOLEAN       Increase
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  EFI_HANDLE           *HandleBuffer;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_CLASSCODE        ClassCode;
  BOOLEAN              HasSuccess;

  ASSERT (Size < PciBarTotal);

  HasSuccess = FALSE;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: No PCI devices for RBAR support - %r\n", Status));
    return Status;
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

    DEBUG ((DEBUG_VERBOSE, "OCDM: PCI device %u/%u has class %X\n", Index+1, HandleCount, ClassCode));

    if (ClassCode.BaseCode != PCI_CLASS_DISPLAY) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCDM: Setting RBAR to %u inc %d on %u/%u\n",
      Size,
      Increase,
      Index+1,
      HandleCount
      ));
    Status = SetResizableBarOnDevice (PciIo, Size, Increase);
    if (!EFI_ERROR (Status)) {
      HasSuccess = TRUE;
    }
  }

  if (HasSuccess) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}
