/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciPowerManagement.c
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#include "PciBus.h"

EFI_STATUS
EFIAPI
ResetPowerManagementFeature (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/*++

Routine Description:

  This function is intended to turn off PWE assertion and
  put the device to D0 state if the device supports
  PCI Power Management.

Arguments:

Returns:
  
  None

--*/
{
  EFI_STATUS  Status;
  UINT8       PowerManagementRegBlock;
  UINT16      PowerManagementCSR;

  PowerManagementRegBlock = 0;

  Status = LocateCapabilityRegBlock (
            PciIoDevice,
            EFI_PCI_CAPABILITY_ID_PMI,
            &PowerManagementRegBlock,
            NULL
            );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Turn off the PWE assertion and put the device into D0 State
  //

  //
  // Read PMCSR
  //
  Status = PciIoDevice->PciIo.Pci.Read (
                                    &PciIoDevice->PciIo,
                                    EfiPciIoWidthUint16,
                                    PowerManagementRegBlock + 4,
                                    1,
                                    &PowerManagementCSR
                                    );
  if (!EFI_ERROR (Status)) {
    //
    // Clear PME_Status bit
    //
    PowerManagementCSR |= BIT15;
    //
    // Clear PME_En bit. PowerState = D0.
    //
    PowerManagementCSR &= ~(BIT8 | BIT1 | BIT0);

    //
    // Write PMCSR
    //
    Status = PciIoDevice->PciIo.Pci.Write (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint16,
                                      PowerManagementRegBlock + 4,
                                      1,
                                      &PowerManagementCSR
                                      );
  }
  return Status;
}
