/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
    PcatPciRootBridgeIo.c
    
Abstract:

    EFI PC AT PCI Root Bridge Io Protocol

Revision History

--*/

#include "PcatPciRootBridge.h"

BOOLEAN                  mPciOptionRomTableInstalled = FALSE;
EFI_PCI_OPTION_ROM_TABLE mPciOptionRomTable          = {0, NULL};

EFI_STATUS
EFIAPI
PcatRootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  )
{
  return gCpuIo->Io.Read (
                      gCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      UserAddress,
                      Count,
                      UserBuffer
                      );
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoIoWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  )
{
  return gCpuIo->Io.Write (
                      gCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      UserAddress,
                      Count,
                      UserBuffer
                      );

}

EFI_STATUS
PcatRootBridgeIoGetIoPortMapping (
  OUT EFI_PHYSICAL_ADDRESS  *IoPortMapping,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryPortMapping
  )
/*++

  Get the IO Port Mapping.  For IA-32 it is always 0.
  
--*/
{
  *IoPortMapping = 0;
  *MemoryPortMapping = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
PcatRootBridgeIoPciRW (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN BOOLEAN                                Write,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  )
{
  PCI_CONFIG_ACCESS_CF8             Pci;
  PCI_CONFIG_ACCESS_CF8             PciAligned;
  UINT32                            InStride;
  UINT32                            OutStride;
  UINTN                             PciData;
  UINTN                             PciDataStride;
  PCAT_PCI_ROOT_BRIDGE_INSTANCE     *PrivateData;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS  PciAddress;
  UINT64                            PciExpressRegAddr;
  BOOLEAN                           UsePciExpressAccess;

  if ((UINT32)Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((Width & 0x03) >= EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  InStride    = 1 << (Width & 0x03);
  OutStride   = InStride;
  if (Width >= EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    InStride = 0;
  }

  if (Width >= EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    OutStride = 0;
  }

  UsePciExpressAccess = FALSE;

  CopyMem (&PciAddress, &UserAddress, sizeof(UINT64));

  if (PciAddress.ExtendedRegister > 0xFF) {
    //
    // Check PciExpressBaseAddress
    //
    if ((PrivateData->PciExpressBaseAddress == 0) ||
        (PrivateData->PciExpressBaseAddress >= MAX_ADDRESS)) {
      return EFI_UNSUPPORTED;
    } else {
      UsePciExpressAccess = TRUE;
    }
  } else {
    if (PciAddress.ExtendedRegister != 0) {
      Pci.Bits.Reg = PciAddress.ExtendedRegister & 0xFF;
    } else {
      Pci.Bits.Reg = PciAddress.Register;
    }
    //
    // Note: We can also use PciExpress access here, if wanted.
    //
  }

  if (!UsePciExpressAccess) {
    Pci.Bits.Func     = PciAddress.Function;
    Pci.Bits.Dev      = PciAddress.Device;
    Pci.Bits.Bus      = PciAddress.Bus;
    Pci.Bits.Reserved = 0;
    Pci.Bits.Enable   = 1;

    //
    // PCI Config access are all 32-bit alligned, but by accessing the
    //  CONFIG_DATA_REGISTER (0xcfc) with different widths more cycle types
    //  are possible on PCI.
    //
    // To read a byte of PCI config space you load 0xcf8 and 
    //  read 0xcfc, 0xcfd, 0xcfe, 0xcff
    //
    PciDataStride = Pci.Bits.Reg & 0x03;

    while (Count) {
      PciAligned = Pci;
      PciAligned.Bits.Reg &= 0xfc;
      PciData = (UINTN)PrivateData->PciData + PciDataStride;
      EfiAcquireLock(&PrivateData->PciLock);
      This->Io.Write (This, EfiPciWidthUint32, PrivateData->PciAddress, 1, &PciAligned);
      if (Write) {
        This->Io.Write (This, Width, PciData, 1, UserBuffer);
      } else {
        This->Io.Read (This, Width, PciData, 1, UserBuffer);
      }
      EfiReleaseLock(&PrivateData->PciLock);
      UserBuffer = ((UINT8 *)UserBuffer) + OutStride;
      PciDataStride = (PciDataStride + InStride) % 4;
      Pci.Bits.Reg += InStride;
      Count -= 1;
    }
  } else {
    //
    // Access PCI-Express space by using memory mapped method.
    //
    PciExpressRegAddr = (PrivateData->PciExpressBaseAddress) |
                        (PciAddress.Bus      << 20) |
                        (PciAddress.Device   << 15) |
                        (PciAddress.Function << 12);
    if (PciAddress.ExtendedRegister != 0) {
      PciExpressRegAddr += PciAddress.ExtendedRegister;
    } else {
      PciExpressRegAddr += PciAddress.Register;
    }
    while (Count) {
      if (Write) {
        This->Mem.Write (This, Width, (UINTN) PciExpressRegAddr, 1, UserBuffer);
      } else {
        This->Mem.Read (This, Width, (UINTN) PciExpressRegAddr, 1, UserBuffer);
      }

      UserBuffer = ((UINT8 *) UserBuffer) + OutStride;
      PciExpressRegAddr += InStride;
      Count -= 1;
    }
  }
  
  return EFI_SUCCESS;
}

