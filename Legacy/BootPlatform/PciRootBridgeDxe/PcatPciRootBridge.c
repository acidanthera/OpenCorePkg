/*++

Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PcatPciRootBridge.c
    
Abstract:

    EFI PC-AT PCI Root Bridge Controller

--*/

#include "PcatPciRootBridge.h"
#include "DeviceIo.h"

EFI_CPU_IO2_PROTOCOL *gCpuIo;


typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  union {
    PCI_CARDBUS_CONTROL_REGISTER   CardBridge;
    PCI_BRIDGE_CONTROL_REGISTER    P2PBridge;
  } Bridge;
} PCI_TYPE02;

EFI_STATUS
EFIAPI
InitializePcatPciRootBridge (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
/*++

Routine Description:
  Initializes the PCI Root Bridge Controller

Arguments:
  ImageHandle -
  SystemTable -
    
Returns:
    None

--*/
{
  EFI_STATUS                     Status;
  PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  UINTN                          PciSegmentIndex;
  UINTN                          PciRootBridgeIndex;
  UINTN                          PrimaryBusIndex;
  UINTN                          NumberOfPciRootBridges;
  UINTN                          NumberOfPciDevices;
  UINTN                          Device;
  UINTN                          Function;
  UINT16                         VendorId;
  PCI_TYPE02                     PciConfigurationHeader;
  UINT64                         Address;
  UINT64                         Value;
  UINT64                         Base;
  UINT64                         Limit;

  //
  // Initialize gCpuIo now since the chipset init code requires it.
  //
  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **)&gCpuIo);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize variables required to search all PCI segments for PCI devices
  //
  PciSegmentIndex        = 0;
  PciRootBridgeIndex     = 0;
  NumberOfPciRootBridges = 0;
  PrimaryBusIndex        = 0;

  while (PciSegmentIndex <= PCI_MAX_SEGMENT) {

    PrivateData = NULL;
    Status = gBS->AllocatePool(
                    EfiBootServicesData,
                    sizeof (PCAT_PCI_ROOT_BRIDGE_INSTANCE),
                    (VOID **)&PrivateData
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    ZeroMem (PrivateData, sizeof (PCAT_PCI_ROOT_BRIDGE_INSTANCE));

    //
    // Initialize the signature of the private data structure
    //
    PrivateData->Signature  = PCAT_PCI_ROOT_BRIDGE_SIGNATURE;
    PrivateData->Handle     = NULL;
    PrivateData->DevicePath = NULL;
    InitializeListHead (&PrivateData->MapInfo);

    //
    // Initialize the PCI root bridge number and the bus range for that root bridge
    //
    PrivateData->RootBridgeNumber = (UINT32)PciRootBridgeIndex;
    PrivateData->PrimaryBus       = (UINT32)PrimaryBusIndex;
    PrivateData->SubordinateBus   = (UINT32)PrimaryBusIndex;

    PrivateData->IoBase      = 0xffffffff;
    PrivateData->MemBase     = 0xffffffff;
    PrivateData->Mem32Base   = 0xffffffffffffffffULL;
    PrivateData->Pmem32Base  = 0xffffffffffffffffULL;
    PrivateData->Mem64Base   = 0xffffffffffffffffULL;
    PrivateData->Pmem64Base  = 0xffffffffffffffffULL;

    //
    // The default mechanism for performing PCI Configuration cycles is to 
    // use the I/O ports at 0xCF8 and 0xCFC.  This is only used for IA-32.
    // IPF uses SAL calls to perform PCI COnfiguration cycles
    //
    PrivateData->PciAddress  = 0xCF8;
    PrivateData->PciData     = 0xCFC;

    //
    // Get the physical I/O base for performing PCI I/O cycles
    // For IA-32, this is always 0, because IA-32 has IN and OUT instructions
    // For IPF, a SAL call is made to retrieve the base address for PCI I/O cycles
    //
    Status = PcatRootBridgeIoGetIoPortMapping (
               &PrivateData->PhysicalIoBase, 
               &PrivateData->PhysicalMemoryBase
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Get PCI Express Base Address
    //
    PrivateData->PciExpressBaseAddress = GetPciExpressBaseAddressForRootBridge (PciSegmentIndex, PciRootBridgeIndex);

    //
    // Create a lock for performing PCI Configuration cycles
    //
    EfiInitializeLock (&PrivateData->PciLock, TPL_HIGH_LEVEL);

    //
    // Initialize the attributes for this PCI root bridge
    //
    PrivateData->Attributes  = 0;

    //
    // Build the EFI Device Path Protocol instance for this PCI Root Bridge
    //
    Status = PcatRootBridgeDevicePathConstructor (&PrivateData->DevicePath, PciRootBridgeIndex, (BOOLEAN)((PrivateData->PciExpressBaseAddress != 0) ? TRUE : FALSE));
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Build the PCI Root Bridge I/O Protocol instance for this PCI Root Bridge
    //
    Status = PcatRootBridgeIoConstructor (&PrivateData->Io, PciSegmentIndex);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    
    //
    // Scan all the PCI devices on the primary bus of the PCI root bridge
    //
    for (Device = 0, NumberOfPciDevices = 0; Device <= PCI_MAX_DEVICE; Device++) {
    
      for (Function = 0; Function <= PCI_MAX_FUNC; Function++) {

        //
        // Compute the PCI configuration address of the PCI device to probe
        //
        Address = EFI_PCI_ADDRESS (PrimaryBusIndex, Device, Function, 0);

        //
        // Read the Vendor ID from the PCI Configuration Header
        //
        Status = PrivateData->Io.Pci.Read (
                                       &PrivateData->Io, 
                                       EfiPciWidthUint16, 
                                       Address, 
                                       sizeof (VendorId) / sizeof (UINT16), 
                                       &VendorId
                                       );
        if ((EFI_ERROR (Status)) || ((VendorId == 0xffff) && (Function == 0))) {
          //
          // If the PCI Configuration Read fails, or a PCI device does not exist, then 
          // skip this entire PCI device
          //
          break;
        }
        if (VendorId == 0xffff) {
          //
          // If PCI function != 0, VendorId == 0xFFFF, we continue to search PCI function.
          //
          continue;
        }

        //
        // Read the entire PCI Configuration Header
        //
        Status = PrivateData->Io.Pci.Read (
                                       &PrivateData->Io, 
                                       EfiPciWidthUint16, 
                                       Address, 
                                       sizeof (PciConfigurationHeader) / sizeof (UINT16), 
                                       &PciConfigurationHeader
                                       );
        if (EFI_ERROR (Status)) {
          //
          // If the entire PCI Configuration Header can not be read, then skip this entire PCI device
          //
          break;
        }


        //
        // Increment the number of PCI device found on the primary bus of the PCI root bridge
        //
        NumberOfPciDevices++;

        //
        // Look for devices with the VGA Palette Snoop enabled in the COMMAND register of the PCI Config Header
        //
        if (PciConfigurationHeader.Hdr.Command & 0x20) {
          PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
        }

        //
        // If the device is a PCI-PCI Bridge, then look at the Subordinate Bus Number
        //
        if (IS_PCI_BRIDGE(&PciConfigurationHeader)) {
          //
          // Get the Bus range that the PPB is decoding
          //
          if (PciConfigurationHeader.Bridge.P2PBridge.SubordinateBus > PrivateData->SubordinateBus) {
            //
            // If the suborinate bus number of the PCI-PCI bridge is greater than the PCI root bridge's
            // current subordinate bus number, then update the PCI root bridge's subordinate bus number 
            //
            PrivateData->SubordinateBus = PciConfigurationHeader.Bridge.P2PBridge.SubordinateBus;
          }

          //
          // Get the I/O range that the PPB is decoding
          //
          Value = PciConfigurationHeader.Bridge.P2PBridge.IoBase & 0x0f;
          Base  = ((UINT32)PciConfigurationHeader.Bridge.P2PBridge.IoBase & 0xf0) << 8;
          Limit = (((UINT32)PciConfigurationHeader.Bridge.P2PBridge.IoLimit & 0xf0) << 8) | 0x0fff;
          if (Value == 0x01) {
            Base  |= ((UINT32)PciConfigurationHeader.Bridge.P2PBridge.IoBaseUpper16 << 16);
            Limit |= ((UINT32)PciConfigurationHeader.Bridge.P2PBridge.IoLimitUpper16 << 16);
          }
          if (Base < Limit) {
            if (PrivateData->IoBase > Base) {
              PrivateData->IoBase = Base;
            }
            if (PrivateData->IoLimit < Limit) {
              PrivateData->IoLimit = Limit;
            }
          }

          //
          // Get the Memory range that the PPB is decoding
          //
          Base  = ((UINT32)PciConfigurationHeader.Bridge.P2PBridge.MemoryBase & 0xfff0) << 16;
          Limit = (((UINT32)PciConfigurationHeader.Bridge.P2PBridge.MemoryLimit & 0xfff0) << 16) | 0xfffff;
          if (Base < Limit) {
            if (PrivateData->MemBase > Base) {
              PrivateData->MemBase = Base;
            }
            if (PrivateData->MemLimit < Limit) {
              PrivateData->MemLimit = Limit;
            }
            if (PrivateData->Mem32Base > Base) {
              PrivateData->Mem32Base = Base;
            }
            if (PrivateData->Mem32Limit < Limit) {
              PrivateData->Mem32Limit = Limit;
            }
          }

          //
          // Get the Prefetchable Memory range that the PPB is decoding
          //
          Value = PciConfigurationHeader.Bridge.P2PBridge.PrefetchableMemoryBase & 0x0f;
          Base  = ((UINT32)PciConfigurationHeader.Bridge.P2PBridge.PrefetchableMemoryBase & 0xfff0) << 16;
          Limit = (((UINT32)PciConfigurationHeader.Bridge.P2PBridge.PrefetchableMemoryLimit & 0xfff0) << 16) | 0xffffff;
          if (Value == 0x01) {
            Base  |= LShiftU64((UINT64)PciConfigurationHeader.Bridge.P2PBridge.PrefetchableBaseUpper32,32);
            Limit |= LShiftU64((UINT64)PciConfigurationHeader.Bridge.P2PBridge.PrefetchableLimitUpper32,32);
          }
          if (Base < Limit) {
            if (PrivateData->MemBase > Base) {
              PrivateData->MemBase = Base;
            }
            if (PrivateData->MemLimit < Limit) {
              PrivateData->MemLimit = Limit;
            }
            if (Value == 0x00) {
              if (PrivateData->Pmem32Base > Base) {
                PrivateData->Pmem32Base = Base;
              }
              if (PrivateData->Pmem32Limit < Limit) {
                PrivateData->Pmem32Limit = Limit;
              }
            }
            if (Value == 0x01) {
              if (PrivateData->Pmem64Base > Base) {
                PrivateData->Pmem64Base = Base;
              }
              if (PrivateData->Pmem64Limit < Limit) {
                PrivateData->Pmem64Limit = Limit;
              }
            }
          }

          //
          // Look at the PPB Configuration for legacy decoding attributes
          //
          if (PciConfigurationHeader.Bridge.P2PBridge.BridgeControl & 0x04) {
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;
          }
          if (PciConfigurationHeader.Bridge.P2PBridge.BridgeControl & 0x08) {
            //
            // ** CHANGE **
            // We do not set EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO.
            //
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
          }

        } else if (IS_CARDBUS_BRIDGE (&PciConfigurationHeader)) {
          //
          // ** CHANGE START **
          // Added CardBridge support.
          //

          //
          // Get the Bus range that the PPB is decoding
          //
          if (PciConfigurationHeader.Bridge.CardBridge.SubordinateBusNumber > PrivateData->SubordinateBus) {
            //
            // If the suborinate bus number of the PCI-PCI bridge is greater than the PCI root bridge's
            // current subordinate bus number, then update the PCI root bridge's subordinate bus number 
            //
            PrivateData->SubordinateBus = PciConfigurationHeader.Bridge.CardBridge.SubordinateBusNumber;
          }
          
          //
          // Get the I/O range that the PPB is decoding
          //
          Base  = PciConfigurationHeader.Bridge.CardBridge.IoBase0;
          Limit = PciConfigurationHeader.Bridge.CardBridge.IoLimit0;
          if (Base < Limit) {
            if (PrivateData->IoBase > Base) {
              PrivateData->IoBase = Base;
            }
            if (PrivateData->IoLimit < Limit) {
              PrivateData->IoLimit = Limit;
            }
          }
          
          //
          // Get the Memory range that the PPB is decoding
          //
          Base  = PciConfigurationHeader.Bridge.CardBridge.MemoryBase0;
          Limit = PciConfigurationHeader.Bridge.CardBridge.MemoryLimit0;
          if (Base < Limit) {
            if (PrivateData->MemBase > Base) {
              PrivateData->MemBase = Base;
            }
            if (PrivateData->MemLimit < Limit) {
              PrivateData->MemLimit = Limit;
            }
            if (PrivateData->Mem32Base > Base) {
              PrivateData->Mem32Base = Base;
            }
            if (PrivateData->Mem32Limit < Limit) {
              PrivateData->Mem32Limit = Limit;
            }
          }
          //
          // ** CHANGE END **
          //
        } else {
          //
          // Parse the BARs of the PCI device to determine what I/O Ranges,
          // Memory Ranges, and Prefetchable Memory Ranges the device is decoding
          //
          if ((PciConfigurationHeader.Hdr.HeaderType & HEADER_LAYOUT_CODE) == HEADER_TYPE_DEVICE) {
            Status = PcatPciRootBridgeParseBars (
                       PrivateData, 
                       PciConfigurationHeader.Hdr.Command,
                       PrimaryBusIndex, 
                       Device, 
                       Function
                       );
          }

          //
          // See if the PCI device is an IDE controller
          //
          if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x01 &&
              PciConfigurationHeader.Hdr.ClassCode[1] == 0x01    ) {
            if (PciConfigurationHeader.Hdr.ClassCode[0] & 0x80) {
              PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
              PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
            }
            if (PciConfigurationHeader.Hdr.ClassCode[0] & 0x01) {
              PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
            }
            if (PciConfigurationHeader.Hdr.ClassCode[0] & 0x04) {
              PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
            }
          }

          //
          // See if the PCI device is a legacy VGA controller
          //
          if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x00 &&
              PciConfigurationHeader.Hdr.ClassCode[1] == 0x01    ) {
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
          }

          //
          // See if the PCI device is a standard VGA controller
          //
          if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x03 &&
              PciConfigurationHeader.Hdr.ClassCode[1] == 0x00    ) {
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
            PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
          }

          //
          // See if the PCI Device is a PCI - ISA or PCI - EISA 
          // or ISA_POSITIVIE_DECODE Bridge device
          //
          if (PciConfigurationHeader.Hdr.ClassCode[2] == 0x06) {
            if (PciConfigurationHeader.Hdr.ClassCode[1] == 0x01 ||
                PciConfigurationHeader.Hdr.ClassCode[1] == 0x02 || 
                PciConfigurationHeader.Hdr.ClassCode[1] == 0x80 ) {
              PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
              PrivateData->Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;

              if (PrivateData->MemBase > 0xa0000) {
                PrivateData->MemBase = 0xa0000;
              }
              if (PrivateData->MemLimit < 0xbffff) {
               PrivateData->MemLimit = 0xbffff;
             }
            }
          }
        }

        //
        // If this device is not a multi function device, then skip the rest of this PCI device
        //
        if (Function == 0 && !(PciConfigurationHeader.Hdr.HeaderType & HEADER_TYPE_MULTI_FUNCTION)) {
          break;
        }
      }
    }

    //
    // After scanning all the PCI devices on the PCI root bridge's primary bus, update the 
    // Primary Bus Number for the next PCI root bridge to be this PCI root bridge's subordinate
    // bus number + 1.
    //
    PrimaryBusIndex = PrivateData->SubordinateBus + 1;

    //
    // If at least one PCI device was found on the primary bus of this PCI root bridge, then the PCI root bridge
    // exists.
    //
    if (NumberOfPciDevices > 0) {

      //
      // Adjust the I/O range used for bounds checking for the legacy decoding attributed
      //
      if (PrivateData->Attributes & 0x7f) {
        PrivateData->IoBase = 0;
        if (PrivateData->IoLimit < 0xffff) {
          PrivateData->IoLimit = 0xffff;
        }
      }

      //
      // Adjust the Memory range used for bounds checking for the legacy decoding attributed
      //
      if (PrivateData->Attributes & EFI_PCI_ATTRIBUTE_VGA_MEMORY) {
        if (PrivateData->MemBase > 0xa0000) {
          PrivateData->MemBase = 0xa0000;
        }
        if (PrivateData->MemLimit < 0xbffff) {
          PrivateData->MemLimit = 0xbffff;
        }
      }

      //
      // Build ACPI descriptors for the resources on the PCI Root Bridge
      //
      Status = ConstructConfiguration(PrivateData);
      ASSERT_EFI_ERROR (Status);

      //
      // Create the handle for this PCI Root Bridge 
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                     &PrivateData->Handle,              
                     &gEfiDevicePathProtocolGuid,
                     PrivateData->DevicePath,
                     &gEfiPciRootBridgeIoProtocolGuid,
                     &PrivateData->Io,
                     NULL
                     );
      ASSERT_EFI_ERROR (Status);

      //
      // Contruct DeviceIoProtocol
      //
      Status = DeviceIoConstructor (
                 PrivateData->Handle,
                 &PrivateData->Io,
                 PrivateData->DevicePath,
                 (UINT16)PrivateData->PrimaryBus,
                 (UINT16)PrivateData->SubordinateBus
                 );
      ASSERT_EFI_ERROR (Status);
      //
      // ** CHANGE **
      // Option ROM support is removed. Patch by nms42.
      //
      //
      // Increment the index for the next PCI Root Bridge
      //
      PciRootBridgeIndex++;

    } else {

      //
      // If no PCI Root Bridges were found on the current PCI segment, then exit
      //
      if (NumberOfPciRootBridges == 0) {
        Status = EFI_SUCCESS;
        goto Done;
      }

    }

    //
    // If the PrimaryBusIndex is greater than the maximum allowable PCI bus number, then
    // the PCI Segment Number is incremented, and the next segment is searched starting at Bus #0
    // Otherwise, the search is continued on the next PCI Root Bridge
    //
    if (PrimaryBusIndex > PCI_MAX_BUS) {
      PciSegmentIndex++;
      NumberOfPciRootBridges = 0;
      PrimaryBusIndex = 0;
    } else {
      NumberOfPciRootBridges++;
    }

  }

  return EFI_SUCCESS;

Done:
  //
  // Clean up memory allocated for the PCI Root Bridge that was searched but not created.
  //
  if (PrivateData) {
    if (PrivateData->DevicePath) {
      gBS->FreePool(PrivateData->DevicePath);
    }
    gBS->FreePool (PrivateData);
  }

  //
  // If no PCI Root Bridges were discovered, then return the error condition from scanning the
  // first PCI Root Bridge
  //
  if (PciRootBridgeIndex == 0) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS 
ConstructConfiguration(
  IN OUT PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
 
{
  EFI_STATUS                         Status;
  UINT8                              NumConfig;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Configuration;
  EFI_ACPI_END_TAG_DESCRIPTOR        *ConfigurationEnd;

  NumConfig = 0;
  PrivateData->Configuration = NULL;

  if (PrivateData->SubordinateBus >= PrivateData->PrimaryBus) {
    NumConfig++;
  }
  if (PrivateData->IoLimit >= PrivateData->IoBase) {
    NumConfig++;
  }
  if (PrivateData->Mem32Limit >= PrivateData->Mem32Base) {
    NumConfig++;
  }
  if (PrivateData->Pmem32Limit >= PrivateData->Pmem32Base) {
    NumConfig++;
  }
  if (PrivateData->Mem64Limit >= PrivateData->Mem64Base) {
    NumConfig++;
  }
  if (PrivateData->Pmem64Limit >= PrivateData->Pmem64Base) {
    NumConfig++;
  }

  if ( NumConfig == 0 ) {

    //
    // If there is no resource request
    //
    Status = gBS->AllocatePool (
                    EfiBootServicesData, 
                    sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR),
                    (VOID **)&PrivateData->Configuration
                    );
    if (EFI_ERROR (Status )) {
      return Status;
    }

    Configuration = PrivateData->Configuration;
    
    ZeroMem (
      Configuration, 
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR) 
      );
    
    Configuration->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len  = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration++;

    ConfigurationEnd       = (EFI_ACPI_END_TAG_DESCRIPTOR *)(Configuration);
    ConfigurationEnd->Desc = ACPI_END_TAG_DESCRIPTOR;
    ConfigurationEnd->Checksum = 0;
  }

  //
  // If there is at least one type of resource request,
  // allocate a acpi resource node 
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData, 
                  sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR),
                  (VOID **)&PrivateData->Configuration
                  );
  if (EFI_ERROR (Status )) {
    return Status;
  }
  
  Configuration = PrivateData->Configuration;

  ZeroMem (
    Configuration, 
    sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)
    );

  if (PrivateData->SubordinateBus >= PrivateData->PrimaryBus) {
    Configuration->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len          = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration->ResType      = ACPI_ADDRESS_SPACE_TYPE_BUS;
    Configuration->SpecificFlag = 0; 
    Configuration->AddrRangeMin = PrivateData->PrimaryBus;
    Configuration->AddrRangeMax = PrivateData->SubordinateBus;
    Configuration->AddrLen      = Configuration->AddrRangeMax - Configuration->AddrRangeMin + 1;
    Configuration++;
  }
  //
  // Deal with io aperture
  //
  if (PrivateData->IoLimit >= PrivateData->IoBase) {
    Configuration->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len          = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration->ResType      = ACPI_ADDRESS_SPACE_TYPE_IO;
    Configuration->SpecificFlag = 1; //non ISA range
    Configuration->AddrRangeMin = PrivateData->IoBase;
    Configuration->AddrRangeMax = PrivateData->IoLimit;
    Configuration->AddrLen      = Configuration->AddrRangeMax - Configuration->AddrRangeMin + 1;
    Configuration++;
  }

  //
  // Deal with mem32 aperture
  //
  if (PrivateData->Mem32Limit >= PrivateData->Mem32Base) {
    Configuration->Desc                 = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len                  = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
    Configuration->SpecificFlag         = 0; //Nonprefechable
    Configuration->AddrSpaceGranularity = 32; //32 bit
    Configuration->AddrRangeMin         = PrivateData->Mem32Base;
    Configuration->AddrRangeMax         = PrivateData->Mem32Limit;
    Configuration->AddrLen              = Configuration->AddrRangeMax - Configuration->AddrRangeMin + 1;
    Configuration++;
  } 

  //
  // Deal with Pmem32 aperture
  //
  if (PrivateData->Pmem32Limit >= PrivateData->Pmem32Base) {
    Configuration->Desc                 = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len                  = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
    Configuration->SpecificFlag         = 0x6; //prefechable
    Configuration->AddrSpaceGranularity = 32; //32 bit
    Configuration->AddrRangeMin         = PrivateData->Pmem32Base;
    Configuration->AddrRangeMax         = PrivateData->Pmem32Limit;
    Configuration->AddrLen              = Configuration->AddrRangeMax - Configuration->AddrRangeMin + 1;
    Configuration++;
  }

  //
  // Deal with mem64 aperture
  //
  if (PrivateData->Mem64Limit >= PrivateData->Mem64Base) {
    Configuration->Desc                 = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len                  = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
    Configuration->SpecificFlag         = 0; //nonprefechable
    Configuration->AddrSpaceGranularity = 64; //32 bit
    Configuration->AddrRangeMin         = PrivateData->Mem64Base;
    Configuration->AddrRangeMax         = PrivateData->Mem64Limit;
    Configuration->AddrLen              = Configuration->AddrRangeMax - Configuration->AddrRangeMin + 1;
    Configuration++;
  }

  //
  // Deal with Pmem64 aperture
  //
  if (PrivateData->Pmem64Limit >= PrivateData->Pmem64Base) {
    Configuration->Desc                 = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Configuration->Len                  = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
    Configuration->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
    Configuration->SpecificFlag         = 0x06; //prefechable
    Configuration->AddrSpaceGranularity = 64; //32 bit
    Configuration->AddrRangeMin         = PrivateData->Pmem64Base;
    Configuration->AddrRangeMax         = PrivateData->Pmem64Limit;
    Configuration->AddrLen              = Configuration->AddrRangeMax - Configuration->AddrRangeMin + 1;
    Configuration++;
  }

  //
  // put the checksum
  //
  ConfigurationEnd           = (EFI_ACPI_END_TAG_DESCRIPTOR *)(Configuration);
  ConfigurationEnd->Desc     = ACPI_END_TAG_DESCRIPTOR;
  ConfigurationEnd->Checksum = 0;

  return EFI_SUCCESS;
}

EFI_STATUS 
PcatPciRootBridgeBarExisted (
  IN  PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData,
  IN  UINT64                         Address,
  OUT UINT32                         *OriginalValue,
  OUT UINT32                         *Value
  ) 
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT32      AllOnes;
  EFI_TPL     OldTpl;

  //
  // Preserve the original value
  //
  PrivateData->Io.Pci.Read (
                                 &PrivateData->Io, 
                                 EfiPciWidthUint32, 
                                 Address, 
                                 1, 
                                 OriginalValue
                                 );

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  AllOnes = 0xffffffff;

  PrivateData->Io.Pci.Write (
                                 &PrivateData->Io, 
                                 EfiPciWidthUint32, 
                                 Address, 
                                 1, 
                                 &AllOnes
                                 );
  PrivateData->Io.Pci.Read (
                                 &PrivateData->Io, 
                                 EfiPciWidthUint32, 
                                 Address, 
                                 1, 
                                 Value
                                 );

  //
  //Write back the original value
  //
  PrivateData->Io.Pci.Write (
                                 &PrivateData->Io, 
                                 EfiPciWidthUint32, 
                                 Address, 
                                 1, 
                                 OriginalValue
                                 );
  //
  // Restore TPL to its original level
  //
  gBS->RestoreTPL (OldTpl);

  if ( *Value == 0 ) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
PcatPciRootBridgeParseBars (
  IN PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData,
  IN UINT16                         Command,
  IN UINTN                          Bus,
  IN UINTN                          Device,
  IN UINTN                          Function
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS  Status;
  UINT64      Address;
  UINT32      OriginalValue;
  UINT32      Value;
  UINT32      OriginalUpperValue;
  UINT32      UpperValue;
  UINT64      Mask;
  UINTN       Offset;
  UINT64      Base;
  UINT64      Length;
  UINT64      Limit;

  for (Offset = 0x10; Offset < 0x28; Offset += 4) {
    Address = EFI_PCI_ADDRESS (Bus, Device, Function, Offset);
    Status = PcatPciRootBridgeBarExisted (
               PrivateData,
               Address,
               &OriginalValue,
               &Value
               );

    if (!EFI_ERROR (Status )) {
      if ( Value & 0x01 ) { 
        if (Command & 0x0001) {
          //
          //Device I/Os
          //
          Mask = 0xfffffffc;
          Base = OriginalValue & Mask;
          Length = ((~(Value & Mask)) & Mask) + 0x04;
          if (!(Value & 0xFFFF0000)){
            Length &= 0x0000FFFF;
          }
          Limit = Base + Length - 1;

          if (Base < Limit) {
            if (PrivateData->IoBase > Base) {
              PrivateData->IoBase = (UINT32)Base;
            }
            if (PrivateData->IoLimit < Limit) {
              PrivateData->IoLimit = (UINT32)Limit;
            }
          }
        }
   
      } else {

        if (Command & 0x0002) {

          Mask = 0xfffffff0;
          Base = OriginalValue & Mask;
          Length = Value & Mask;
 
          if ((Value & 0x07) != 0x04) {
            Length = ((~Length) + 1) & 0xffffffff;
          } else {
            Offset += 4; 
            Address = EFI_PCI_ADDRESS (Bus, Device, Function, Offset);

            Status = PcatPciRootBridgeBarExisted (
                       PrivateData,
                       Address,
                       &OriginalUpperValue,
                       &UpperValue
                       );

            Base   = Base | LShiftU64((UINT64)OriginalUpperValue,32);
            Length = Length | LShiftU64((UINT64)UpperValue,32);
            Length = (~Length) + 1;
          }

          Limit = Base + Length - 1;

          if (Base < Limit) {
            if (PrivateData->MemBase > Base) {
              PrivateData->MemBase = Base;
            }
            if (PrivateData->MemLimit < Limit) {
              PrivateData->MemLimit = Limit;
            }

            switch (Value &0x07) {
            case 0x00: ////memory space; anywhere in 32 bit address space
              if (Value & 0x08) {
                if (PrivateData->Pmem32Base > Base) {
                  PrivateData->Pmem32Base = Base;
                }
                if (PrivateData->Pmem32Limit < Limit) {
                  PrivateData->Pmem32Limit = Limit;
                }
              } else {
                if (PrivateData->Mem32Base > Base) {
                  PrivateData->Mem32Base = Base;
                }
                if (PrivateData->Mem32Limit < Limit) {
                  PrivateData->Mem32Limit = Limit;
                }
              }
              break;
            case 0x04: //memory space; anywhere in 64 bit address space
              if (Value & 0x08) {
                if (PrivateData->Pmem64Base > Base) {
                  PrivateData->Pmem64Base = Base;
                }
                if (PrivateData->Pmem64Limit < Limit) {
                  PrivateData->Pmem64Limit = Limit;
                }
              } else {
                if (PrivateData->Mem64Base > Base) {
                  PrivateData->Mem64Base = Base;
                }
                if (PrivateData->Mem64Limit < Limit) {
                  PrivateData->Mem64Limit = Limit;
                }
              }
              break;
            }
          }
        }
      }
    }
  }
  return EFI_SUCCESS;
}

UINT64
GetPciExpressBaseAddressForRootBridge (
  IN UINTN    HostBridgeNumber,
  IN UINTN    RootBridgeNumber
  )
/*++

Routine Description:
  This routine is to get PciExpress Base Address for this RootBridge

Arguments:
  HostBridgeNumber - The number of HostBridge
  RootBridgeNumber - The number of RootBridge
    
Returns:
  UINT64 - PciExpressBaseAddress for this HostBridge and RootBridge

--*/
{
  EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION *PciExpressBaseAddressInfo;
  UINTN                                    BufferSize;
  UINT32                                   Index;
  UINT32                                   Number;
  EFI_PEI_HOB_POINTERS                     GuidHob;

  //
  // Get PciExpressAddressInfo Hob
  //
  PciExpressBaseAddressInfo = NULL;
  BufferSize                = 0;
  GuidHob.Raw = GetFirstGuidHob (&gEfiPciExpressBaseAddressGuid);
  if (GuidHob.Raw != NULL) {
    PciExpressBaseAddressInfo = GET_GUID_HOB_DATA (GuidHob.Guid);
    BufferSize                = GET_GUID_HOB_DATA_SIZE (GuidHob.Guid);
  } else {
    return 0;
  }

  //
  // Search the PciExpress Base Address in the Hob for current RootBridge
  //
  Number = (UINT32)(BufferSize / sizeof(EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION));
  for (Index = 0; Index < Number; Index++) {
    if ((PciExpressBaseAddressInfo[Index].HostBridgeNumber == HostBridgeNumber) &&
        (PciExpressBaseAddressInfo[Index].RootBridgeNumber == RootBridgeNumber)) {
      return PciExpressBaseAddressInfo[Index].PciExpressBaseAddress;
    }
  }

  //
  // Do not find the PciExpress Base Address in the Hob
  //
  return 0;
}

