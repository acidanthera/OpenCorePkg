/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
    PcatPciRootBridgeDevicePath.c
    
Abstract:

    EFI PCAT PCI Root Bridge Device Path Protocol

Revision History

--*/

#include "PcatPciRootBridge.h"

//
// Static device path declarations for this driver.
//

typedef struct {
  ACPI_HID_DEVICE_PATH              AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8),
      }
    },
    EISA_PNP_ID(0x0A03),
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

EFI_STATUS
PcatRootBridgeDevicePathConstructor (
  IN EFI_DEVICE_PATH_PROTOCOL  **Protocol,
  IN UINTN                     RootBridgeNumber,
  IN BOOLEAN                   IsPciExpress
  )
/*++

Routine Description:

    Construct the device path protocol

Arguments:

    Protocol - protocol to initialize
    
Returns:

    None

--*/
{
  ACPI_HID_DEVICE_PATH  *AcpiDevicePath;
 
  *Protocol = DuplicateDevicePath((EFI_DEVICE_PATH_PROTOCOL *)(&mEfiPciRootBridgeDevicePath));

  AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)(*Protocol);
 
  AcpiDevicePath->UID = (UINT32)RootBridgeNumber;

  if (IsPciExpress) {
    //
    // ** CHANGE START **
    // Force general PCI as normal firmwares and macOS do.
    //
    // AcpiDevicePath->HID = EISA_PNP_ID (0x0A08);
    //
    // ** CHANGE END **
    //
  }

  return EFI_SUCCESS;
}

