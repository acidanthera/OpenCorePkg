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
#include <Library/UefiBootServicesTableLib.h>

#define XHC_HCCPARAMS_OFFSET      0x10
#define XHC_NEXT_CAPABILITY_MASK  0xFF00
#define XHC_CAPABILITY_ID_MASK    0xFF
#define XHC_USBCMD_OFFSET         0x0    ///< USB Command Register Offset
#define XHC_USBSTS_OFFSET         0x4    ///< USB Status Register Offset
#define XHC_POLL_DELAY            1000

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

    //
    // FIXME: Check whether we need PCI_IF_UHCI and PCI_IF_EHCI as well.
    //
    if (Pci.Hdr.ClassCode[0] == PCI_IF_XHCI) {
      Result = XhciReleaseOwnership (PciIo);
    }
  }

  gBS->FreePool (HandleArray);
  return Result;
}
