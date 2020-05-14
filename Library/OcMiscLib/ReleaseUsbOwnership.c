/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

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

#define XHC_HCCPARAMS_OFFSET      0x10
#define XHC_NEXT_CAPABILITY_MASK  0xFF00
#define XHC_CAPABILITY_ID_MASK    0xFF
#define XHC_USBCMD_OFFSET         0x0    ///< USB Command Register Offset
#define XHC_USBSTS_OFFSET         0x4    ///< USB Status Register Offset
#define XHC_POLL_DELAY            1000

#define EHC_BAR_INDEX             0x0
#define EHC_HCCPARAMS_OFFSET      0x8
#define EHC_USBCMD_OFFSET         0x0    ///< USB Command Register Offset
#define EHC_USBSTS_OFFSET         0x4    ///< USB Status Register Offset
#define EHC_USBINT_OFFSET         0x8    ///< USB Interrupt Enable Register

/**
  Release XHCI USB controllers ownership.

  @param[in]  PciIo  PCI I/O protocol for the device.

  @retval EFI_NOT_FOUND  No XHCI controllers had ownership incorrectly set.
  @retval EFI_SUCCESS    Corrected XHCI controllers ownership to OS.
**/
STATIC
EFI_STATUS
XhciReleaseOwnership (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo
  )
{
  EFI_STATUS  Status;

  UINT32  HcCapParams;
  UINT32  ExtendCap;
  UINT32  Value;
  INT32   TimeOut;

  //
  // XHCI controller, then disable legacy support, if enabled.
  //

  Status = PciIo->Mem.Read (
    PciIo,
    EfiPciIoWidthUint32,
    XHC_USBCMD_OFFSET,
    XHC_HCCPARAMS_OFFSET,
    1,
    &HcCapParams
    );

  ExtendCap = EFI_ERROR (Status) ? 0 : ((HcCapParams >> 14U) & 0x3FFFCU);

  while (ExtendCap) {
    Status = PciIo->Mem.Read (
      PciIo,
      EfiPciIoWidthUint32,
      XHC_USBCMD_OFFSET,
      ExtendCap,
      1,
      &Value
      );

    if (EFI_ERROR (Status)) {
      break;
    }

    if ((Value & XHC_CAPABILITY_ID_MASK) == 1) {
      //
      // Do nothing if BIOS ownership is cleared.
      //
      if (!(Value & BIT16)) {
        break;
      }

      Value |= BIT24;

      PciIo->Mem.Write (
        PciIo,
        EfiPciIoWidthUint32,
        XHC_USBCMD_OFFSET,
        ExtendCap,
        1,
        &Value
        );

      TimeOut = 40;
      while (TimeOut--) {
        gBS->Stall (500);

        Status = PciIo->Mem.Read (
          PciIo,
          EfiPciIoWidthUint32,
          XHC_USBCMD_OFFSET,
          ExtendCap,
          1,
          &Value
          );

        if (EFI_ERROR(Status) || !(Value & BIT16)) {
          break;
        }
      }

      //
      // Disable all SMI in USBLEGCTLSTS
      //
      Status = PciIo->Mem.Read (
        PciIo,
        EfiPciIoWidthUint32,
        XHC_USBCMD_OFFSET,
        ExtendCap + 4,
        1,
        &Value
        );

      if (EFI_ERROR(Status)) {
        break;
      }

      Value &= 0x1F1FEEU;
      Value |= 0xE0000000U;

      PciIo->Mem.Write (
        PciIo,
        EfiPciIoWidthUint32,
        XHC_USBCMD_OFFSET,
        ExtendCap + 4,
        1,
        &Value
        );

      //
      // Clear all ownership
      //
      Status = PciIo->Mem.Read (
        PciIo,
        EfiPciIoWidthUint32,
        XHC_USBCMD_OFFSET,
        ExtendCap,
        1,
        &Value
        );

      if (EFI_ERROR(Status)) {
        break;
      }

      Value &= ~(BIT24 | BIT16);
      PciIo->Mem.Write (
        PciIo,
        EfiPciIoWidthUint32,
        XHC_USBCMD_OFFSET,
        ExtendCap,
        1,
        &Value
        );

      break;
    }

    if (!(Value & XHC_NEXT_CAPABILITY_MASK)) {
      break;
    }

    ExtendCap += ((Value >> 6U) & 0x3FCU);
  }

  return Status;
}

/**
 Release EHCI USB controllers ownership.

 @param[in]  PciIo  PCI I/O protocol for the device.

 @retval EFI_NOT_FOUND  No EHCI controllers had ownership incorrectly set.
 @retval EFI_SUCCESS    Corrected EHCI controllers ownership to OS.
 **/
STATIC
EFI_STATUS
EhciReleaseOwnership (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo
  )
{
  EFI_STATUS       Status;
  UINT32           Value;
  UINT32           Base;
  UINT32           OpAddr;
  UINT32           ExtendCap;
  UINT32           UsbCmd;
  UINT32           UsbLegSup;
  UINT32           UsbLegCtlSts;
  UINTN            IsOsOwned;
  UINTN            IsBiosOwned;
  BOOLEAN          IsOwnershipConflict;
  UINT32           HcCapParams;
  INT32            TimeOut;

  Value = 0x0002;

  PciIo->Pci.Write (
    PciIo,
    EfiPciIoWidthUint16,
    0x04,
    1,
    &Value
    );

  Base = 0;

  Status = PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    0x10,
    1,
    &Base
    );

  if (MmioRead8 (Base) < 0x0C) {
    //
    // Config space too small: no legacy implementation.
    //
    return EFI_NOT_FOUND;
  }

  //
  // Operational Registers = capaddr + offset (8bit CAPLENGTH in Capability Registers + offset 0).
  //
  OpAddr = Base + MmioRead8 (Base);

  Status = PciIo->Mem.Read (
    PciIo,
    EfiPciIoWidthUint32,
    EHC_BAR_INDEX,
    EHC_HCCPARAMS_OFFSET,
    1,
    &HcCapParams
    );

  ExtendCap = (HcCapParams >> 8U) & 0xFFU;

  //
  // Read PCI Config 32bit USBLEGSUP (eecp+0).
  //
  Status = PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap,
    1,
    &UsbLegSup
    );

  IsBiosOwned = (UsbLegSup & BIT16) != 0;
  if (!IsBiosOwned) {
    //
    // No BIOS ownership, ignore.
    //
    return EFI_NOT_FOUND;
  }

  //
  // Read PCI Config 32bit USBLEGCTLSTS (eecp+4).
  //
  PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap + 0x4,
    1,
    &UsbLegCtlSts
    );

  //
  // Disable the SMI in USBLEGCTLSTS firstly.
  //
  UsbLegCtlSts &= 0xFFFF0000U;
  PciIo->Pci.Write (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap + 0x4,
    1,
    &UsbLegCtlSts
    );

  UsbCmd  = MmioRead32 (OpAddr + EHC_USBCMD_OFFSET);

  //
  // Clear registers to default.
  //
  UsbCmd = UsbCmd & 0xFFFFFF00U;
  MmioWrite32 (OpAddr + EHC_USBCMD_OFFSET, UsbCmd);
  MmioWrite32 (OpAddr + EHC_USBINT_OFFSET, 0);
  MmioWrite32 (OpAddr + EHC_USBSTS_OFFSET, 0x1000);

  Value = 1;
  PciIo->Pci.Write (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap,
    1,
    &Value
    );

  //
  // Read 32bit USBLEGSUP (eecp+0).
  //
  PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap,
    1,
    &UsbLegSup
    );

  IsBiosOwned = (UsbLegSup & BIT16) != 0;
  IsOsOwned   = (UsbLegSup & BIT24) != 0;

  //
  // Read 32bit USBLEGCTLSTS (eecp+4).
  //
  PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap + 0x4,
    1,
    &UsbLegCtlSts
    );

  //
  // Get EHCI Ownership from legacy bios.
  //
  PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap,
    1,
    &UsbLegSup
    );

  IsOwnershipConflict = IsBiosOwned && IsOsOwned;

  if (IsOwnershipConflict) {
    //
    // EHCI - Ownership conflict - attempting soft reset.
    //
    Value = 0;
    PciIo->Pci.Write (
      PciIo,
      EfiPciIoWidthUint8,
      ExtendCap + 3,
      1,
      &Value
      );

    TimeOut = 40;
    while (TimeOut--) {
      gBS->Stall (500);

      PciIo->Pci.Read (
        PciIo,
        EfiPciIoWidthUint32,
        ExtendCap,
        1,
        &Value
        );

      if ((Value & BIT24) == 0x0) {
        break;
      }
    }
  }

  PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap,
    1,
    &Value
    );

  Value |= BIT24;
  PciIo->Pci.Write (
    PciIo,
    EfiPciIoWidthUint32,
    ExtendCap,
    1,
    &Value
    );

  TimeOut = 40;
  while (TimeOut--) {
    gBS->Stall (500);

    PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint32,
      ExtendCap,
      1,
      &Value
      );

    if ((Value & BIT16) == 0x0) {
      break;
    }
  }

  IsOwnershipConflict = (Value & BIT16) != 0x0;
  if (IsOwnershipConflict) {
    //
    // Soft reset has failed. Assume SMI being ignored and do hard reset.
    //
    Value = 0;
    PciIo->Pci.Write (
      PciIo,
      EfiPciIoWidthUint8,
      ExtendCap + 2,
      1,
      &Value
      );

    TimeOut = 40;
    while (TimeOut--) {
      gBS->Stall (500);

      PciIo->Pci.Read (
        PciIo,
        EfiPciIoWidthUint32,
        ExtendCap,
        1,
        &Value
        );

      if ((Value & BIT16) == 0x0) {
        break;
      }
    }

    //
    // Disable further SMI events.
    //
    PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint32,
      ExtendCap + 0x4,
      1,
      &UsbLegCtlSts
      );

    UsbLegCtlSts &= 0xFFFF0000U;
    PciIo->Pci.Write (
      PciIo,
      EfiPciIoWidthUint32,
      ExtendCap + 0x4,
      1,
      &UsbLegCtlSts
      );
  }

  if (Value & BIT16) {
    //
    // EHCI controller unable to take control from BIOS.
    //
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/**
 Release UHCI USB controllers ownership.

 @param[in]  PciIo  PCI I/O protocol for the device.

 @retval EFI_NOT_FOUND  No UHCI controllers had ownership incorrectly set.
 @retval EFI_SUCCESS    Corrected UHCI controllers ownership to OS.
 **/
STATIC
EFI_STATUS
UhciReleaseOwnership (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo
  )
{
  EFI_STATUS  Status;
  UINT32      Base;
  UINT32      PortBase;
  UINT16      Command;

  Base = 0;

  Status = PciIo->Pci.Read(
    PciIo,
    EfiPciIoWidthUint32,
    0x20,
    1,
    &Base
    );

  PortBase = (Base >> 5) & 0x07ff;

  Command = 0x8f00;

  Status = PciIo->Pci.Write (
    PciIo,
    EfiPciIoWidthUint16,
    0xC0,
    1,
    &Command
    );

  if (PortBase != 0 && (PortBase & BIT0) == 0) {
    IoWrite16 (PortBase, 0x0002);
    gBS->Stall (500);
    IoWrite16 (PortBase + 4, 0);
    gBS->Stall (500);
    IoWrite16 (PortBase, 0);
  }

  return Status;
}

EFI_STATUS
ReleaseUsbOwnership (
  VOID
  )
{
  EFI_STATUS            Result;
  EFI_STATUS            Status;
  EFI_HANDLE            *HandleArray;
  UINTN                 HandleArrayCount;
  UINTN                 Index;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  PCI_TYPE00            Pci;

  Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiPciIoProtocolGuid,
                    NULL,
                    &HandleArrayCount,
                    &HandleArray
                    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Result = EFI_UNSUPPORTED;

  for (Index = 0; Index < HandleArrayCount; ++Index) {
    Status = gBS->HandleProtocol (
      HandleArray[Index],
      &gEfiPciIoProtocolGuid,
      (VOID **) &PciIo
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (
      PciIo,
      EfiPciIoWidthUint32,
      0,
      sizeof (Pci) / sizeof (UINT32),
      &Pci
      );

    if (EFI_ERROR (Status)
      || Pci.Hdr.ClassCode[1] != PCI_CLASS_SERIAL_USB
      || Pci.Hdr.ClassCode[2] != PCI_CLASS_SERIAL) {
      continue;
    }

    if (Pci.Hdr.ClassCode[0] == PCI_IF_XHCI) {
      Result = XhciReleaseOwnership (PciIo);
    } else if (Pci.Hdr.ClassCode[0] == PCI_IF_EHCI) {
      Result = EhciReleaseOwnership (PciIo);
    } else if (Pci.Hdr.ClassCode[0] == PCI_IF_UHCI) {
      Result = UhciReleaseOwnership (PciIo);
    }
  }

  gBS->FreePool (HandleArray);
  return Result;
}
