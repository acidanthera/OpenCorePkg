/** @file
  Copyright (C) 2021, PMheart. All rights reserved.
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
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct PCI_CLASS_ENTRY_TAG {
  UINT8                               Code;             // Class, subclass or interface code
  CONST CHAR8                         *DescText;        // Description string
  CONST struct PCI_CLASS_ENTRY_TAG    *LowerLevelClass; // Subclass or interface if any
} PCI_CLASS_ENTRY;

//
// PCI subclasses.
//

//
// Pre-classcode subclass.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass00PreClassCode[] = {
  {
    PCI_CLASS_OLD_OTHER,
    "Other device",
    NULL
  },
  {
    PCI_CLASS_OLD_VGA,
    "VGA-compatible device",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Mass storage subclasses.
//

// SCSI controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass01MassStorage00[] = {
  {
    0x00,
    "SCSI controller",
    NULL
  },
  {
    0x11,
    "SCSI storage device SOP using PQI",
    NULL
  },
  {
    0x12,
    "SCSI controller SOP using PQI",
    NULL
  },
  {
    0x13,
    "SCSI storage device and controller SOP using PQI",
    NULL
  },
  {
    0x21,
    "SCSI storage device SOP using NVMe",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// ATA controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass01MassStorage05[] = {
  {
    0x20,
    "ATA controller with single stepping ADMA interface",
    NULL
  },
  {
    0x30,
    "ATA controller with continous ADMA interface",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// Serial ATA controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass01MassStorage06[] = {
  {
    0x00,
    "Serial ATA controller",
    NULL
  },
  {
    0x01,
    "Serial ATA controller using AHCI",
    NULL
  },
  {
    0x02,
    "Serial Storage Bus interface",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// Non-volatile memory subsystem interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass01MassStorage08[] = {
  {
    0x00,
    "Non-volatile memory subsystem",
    NULL
  },
  {
    0x01,
    "Non-volatile memory subsystem using NVMHCI",
    NULL
  },
  {
    0x02,
    "NVM Express I/O controller",
    NULL
  },
  {
    0x03,
    "NVM Express administrative controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// Universal Flash Storage controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass01MassStorage09[] = {
  {
    0x00,
    "Universal Flash Storage controller",
    NULL
  },
  {
    0x01,
    "Universal Flash Storage controller using UFSHCI",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

STATIC CONST PCI_CLASS_ENTRY  mPciClass01MassStorage[] = {
  {
    PCI_CLASS_MASS_STORAGE_SCSI,
    "SCSI controller",
    mPciClass01MassStorage00
  },
  {
    PCI_CLASS_MASS_STORAGE_IDE,
    "IDE controller",
    NULL
  },
  {
    PCI_CLASS_MASS_STORAGE_FLOPPY,
    "Floppy controller",
    NULL
  },
  {
    PCI_CLASS_MASS_STORAGE_IPI,
    "IPI bus controller",
    NULL
  },
  {
    PCI_CLASS_MASS_STORAGE_RAID,
    "RAID controller",
    NULL
  },
  {
    0x05,
    "ATA controller",
    mPciClass01MassStorage05
  },
  {
    0x06,
    "Serial ATA controller",
    mPciClass01MassStorage06
  },
  {
    0x07,
    "Serial Attached SCSI controller",
    NULL
  },
  {
    0x08,
    "Non-volatile memory subsystem",
    mPciClass01MassStorage08
  },
  {
    0x09,
    "Universal Flash Storage controller",
    mPciClass01MassStorage09
  },
  {
    PCI_CLASS_MASS_STORAGE_OTHER,
    "Other mass storage controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Network controller subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass02Network[] = {
  {
    PCI_CLASS_NETWORK_ETHERNET,
    "Ethernet controller",
    NULL
  },
  {
    PCI_CLASS_NETWORK_TOKENRING,
    "Token Ring controller",
    NULL
  },
  {
    PCI_CLASS_NETWORK_FDDI,
    "FDDI controller",
    NULL
  },
  {
    PCI_CLASS_NETWORK_ATM,
    "ATM controller",
    NULL
  },
  {
    PCI_CLASS_NETWORK_ISDN,
    "ISDN controller",
    NULL
  },
  {
    0x05,
    "WorldFip controller",
    NULL
  },
  {
    0x06,
    "PCMIG 2.14 Multi Computing",
    NULL
  },
  {
    0x07,
    "InfiniBand controller",
    NULL
  },
  {
    0x08,
    "Host fabric controller",
    NULL
  },
  {
    PCI_CLASS_NETWORK_OTHER,
    "Other network controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Display controller subclasses.
//

// VGA controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass03Display00[] = {
  {
    PCI_IF_VGA_VGA,
    "VGA-compatible controller",
    NULL
  },
  {
    PCI_IF_VGA_8514,
    "8514-compatible controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

STATIC CONST PCI_CLASS_ENTRY  mPciClass03Display[] = {
  {
    PCI_CLASS_DISPLAY_VGA,
    "VGA controller",
    mPciClass03Display00
  },
  {
    PCI_CLASS_DISPLAY_XGA,
    "XGA controller",
    NULL
  },
  {
    PCI_CLASS_DISPLAY_3D,
    "3D controller",
    NULL
  },
  {
    PCI_CLASS_DISPLAY_OTHER,
    "Other display controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Multimedia controller subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass04Multimedia[] = {
  {
    PCI_CLASS_MEDIA_VIDEO,
    "Video device",
    NULL
  },
  {
    PCI_CLASS_MEDIA_AUDIO,
    "Audio device",
    NULL
  },
  {
    PCI_CLASS_MEDIA_TELEPHONE,
    "Telephony device",
    NULL
  },
  {
    0x03,
    "High definition audio device",
    NULL
  },
  {
    PCI_CLASS_MEDIA_OTHER,
    "Other multimedia device",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Memory controller subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass05Memory[] = {
  {
    PCI_CLASS_MEMORY_RAM,
    "RAM controller",
    NULL
  },
  {
    PCI_CLASS_MEMORY_FLASH,
    "Flash memory controller",
    NULL
  },
  {
    PCI_CLASS_MEMORY_OTHER,
    "Other memory controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Bridge subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass06Bridge[] = {
  {
    PCI_CLASS_BRIDGE_HOST,
    "Host bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_ISA,
    "ISA bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_EISA,
    "EISA bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_MCA,
    "MCA bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_P2P,
    "PCI-to-PCI bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_PCMCIA,
    "PCMCIA bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_CARDBUS,
    "CardBus bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_RACEWAY,
    "RACEway bridge",
    NULL
  },
  {
    0x09,
    "Semi-transparent PCI-to-PCI bridge",
    NULL
  },
  {
    0x0A,
    "InfiniBand-to-PCI host bridge",
    NULL
  },
  {
    0x0B,
    "Advanced Switching to PCI host bridge",
    NULL
  },
  {
    PCI_CLASS_BRIDGE_OTHER,
    "Other bridge device",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Simple communication controller subclasses.
//

// Serial controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass07SimpleComms00[] = {
  {
    PCI_IF_GENERIC_XT,
    "Generic XT-compatible serial controller",
    NULL
  },
  {
    PCI_IF_16450,
    "16450-compatible serial controller",
    NULL
  },
  {
    PCI_IF_16550,
    "16550-compatible serial controller",
    NULL
  },
  {
    PCI_IF_16650,
    "16650-compatible serial controller",
    NULL
  },
  {
    PCI_IF_16750,
    "16750-compatible serial controller",
    NULL
  },
  {
    PCI_IF_16850,
    "16850-compatible serial controller",
    NULL
  },
  {
    PCI_IF_16950,
    "16950-compatible serial controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// Parallel controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass07SimpleComms01[] = {
  {
    PCI_IF_PARALLEL_PORT,
    "Parallel port",
    NULL
  },
  {
    PCI_IF_BI_DIR_PARALLEL_PORT,
    "Bi-directional parallel port",
    NULL
  },
  {
    PCI_IF_ECP_PARALLEL_PORT,
    "ECP parallel port",
    NULL
  },
  {
    PCI_IF_1284_CONTROLLER,
    "IEEE1284 controller",
    NULL
  },
  {
    PCI_IF_1284_DEVICE,
    "IEEE1284 device",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// Modem interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass07SimpleComms03[] = {
  {
    PCI_IF_GENERIC_MODEM,
    "Generic modem",
    NULL
  },
  {
    PCI_IF_16450_MODEM,
    "Hayes compatible modem, 16450-compatible interface",
    NULL
  },
  {
    PCI_IF_16550_MODEM,
    "Hayes compatible modem, 16550-compatible interface",
    NULL
  },
  {
    PCI_IF_16650_MODEM,
    "Hayes compatible modem, 16650-compatible interface",
    NULL
  },
  {
    PCI_IF_16750_MODEM,
    "Hayes compatible modem, 16750-compatible interface",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

STATIC CONST PCI_CLASS_ENTRY  mPciClass07SimpleComms[] = {
  {
    PCI_SUBCLASS_SERIAL,
    "Serial controller",
    mPciClass07SimpleComms00
  },
  {
    PCI_SUBCLASS_PARALLEL,
    "Parallel controller",
    mPciClass07SimpleComms01
  },
  {
    PCI_SUBCLASS_MULTIPORT_SERIAL,
    "Multiport serial controller",
    NULL
  },
  {
    PCI_SUBCLASS_MODEM,
    "Modem",
    mPciClass07SimpleComms03
  },
  {
    0x04,
    "GPIB (IEEE448.1/2) controller",
    NULL
  },
  {
    0x05,
    "Smart Card",
    NULL
  },
  {
    PCI_SUBCLASS_SCC_OTHER,
    "Other communications device",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// System peripheral subclasses.
//

// Interrupt controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass08SystemPerip00[] = {
  {
    PCI_IF_8259_PIC,
    "Generic 8259 programmable interrupt controller",
    NULL
  },
  {
    PCI_IF_ISA_PIC,
    "ISA programmable interrupt controller",
    NULL
  },
  {
    PCI_IF_EISA_PIC,
    "EISA programmable interrupt controller",
    NULL
  },
  {
    PCI_IF_APIC_CONTROLLER,
    "I/O APIC interrupt controller",
    NULL
  },
  {
    PCI_IF_APIC_CONTROLLER2,
    "I/O x2 APIC interrupt controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// DMA controller interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass08SystemPerip01[] = {
  {
    PCI_IF_8237_DMA,
    "Generic 8237 DMA controller",
    NULL
  },
  {
    PCI_IF_ISA_DMA,
    "ISA DMA controller",
    NULL
  },
  {
    PCI_IF_EISA_DMA,
    "EISA DMA controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// Timer interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass08SystemPerip02[] = {
  {
    PCI_IF_8254_TIMER,
    "Generic 8254 system timer",
    NULL
  },
  {
    PCI_IF_ISA_TIMER,
    "ISA system timer",
    NULL
  },
  {
    PCI_IF_EISA_TIMER,
    "EISA system timer",
    NULL
  },
  {
    0x04,
    "High Performance Event Timer",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// RTC interfaces.
STATIC CONST PCI_CLASS_ENTRY  mPciClass08SystemPerip03[] = {
  {
    PCI_IF_GENERIC_RTC,
    "Generic RTC controller",
    NULL
  },
  {
    PCI_IF_ISA_RTC,
    "ISA RTC controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

STATIC CONST PCI_CLASS_ENTRY  mPciClass08SystemPerip[] = {
  {
    PCI_SUBCLASS_PIC,
    "Programmable interrupt controller",
    mPciClass08SystemPerip00
  },
  {
    PCI_SUBCLASS_DMA,
    "DMA controller",
    mPciClass08SystemPerip01
  },
  {
    PCI_SUBCLASS_TIMER,
    "System timer",
    mPciClass08SystemPerip02
  },
  {
    PCI_SUBCLASS_RTC,
    "RTC controller",
    mPciClass08SystemPerip03
  },
  {
    PCI_SUBCLASS_PNP_CONTROLLER,
    "Generic PCI Hot-Plug controller",
    NULL
  },
  {
    0x05,
    "SD Host controller",
    NULL
  },
  {
    0x06,
    "IOMMU",
    NULL
  },
  {
    0x07,
    "Root Complex Event Collector",
    NULL
  },
  {
    PCI_SUBCLASS_PERIPHERAL_OTHER,
    "Other system peripheral",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Input device subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass09Input[] = {
  {
    PCI_SUBCLASS_KEYBOARD,
    "Keyboard controller",
    NULL
  },
  {
    PCI_SUBCLASS_PEN,
    "Pen controller",
    NULL
  },
  {
    PCI_SUBCLASS_MOUSE_CONTROLLER,
    "Mouse controller",
    NULL
  },
  {
    PCI_SUBCLASS_SCAN_CONTROLLER,
    "Scanner controller",
    NULL
  },
  {
    PCI_SUBCLASS_GAMEPORT,
    "Gameport controller",
    NULL
  },
  {
    PCI_SUBCLASS_INPUT_OTHER,
    "Other input controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Docking station subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass0ADocking[] = {
  {
    PCI_SUBCLASS_DOCKING_GENERIC,
    "Generic docking station",
    NULL
  },
  {
    PCI_SUBCLASS_DOCKING_OTHER,
    "Other docking station",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Processor subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass0BProcessor[] = {
  {
    PCI_SUBCLASS_PROC_386,
    "386 processor",
    NULL
  },
  {
    PCI_SUBCLASS_PROC_486,
    "486 processor",
    NULL
  },
  {
    PCI_SUBCLASS_PROC_PENTIUM,
    "Pentium processor",
    NULL
  },
  {
    PCI_SUBCLASS_PROC_ALPHA,
    "Alpha processor",
    NULL
  },
  {
    PCI_SUBCLASS_PROC_POWERPC,
    "PowerPC processor",
    NULL
  },
  {
    PCI_SUBCLASS_PROC_MIPS,
    "MIPS processor",
    NULL
  },
  {
    PCI_SUBCLASS_PROC_CO_PORC,
    "Co-processor",
    NULL
  },
  {
    0x80,
    "Other processor",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Serial bus controller subclasses.
//

// FireWire controller interfaces
STATIC CONST PCI_CLASS_ENTRY  mPciClass0CSerialBus00[] = {
  {
    PCI_IF_1394,
    "IEEE 1394 FireWire controller",
    NULL
  },
  {
    PCI_IF_1394_OPEN_HCI,
    "IEEE 1394 OHCI controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

// USB controller interfaces
STATIC CONST PCI_CLASS_ENTRY  mPciClass0CSerialBus03[] = {
  {
    PCI_IF_UHCI,
    "Universal Serial Bus UHCI controller",
    NULL
  },
  {
    PCI_IF_OHCI,
    "Universal Serial Bus OHCI controller",
    NULL
  },
  {
    0x20,
    "Universal Serial Bus EHCI controller",
    NULL
  },
  {
    0x30,
    "Universal Serial Bus xHCI controller",
    NULL
  },
  {
    0x40,
    "USB4 Host Interface controller",
    NULL
  },
  {
    PCI_IF_USB_OTHER,
    "Other Universal Serial Bus controller",
    NULL
  },
  {
    PCI_IF_USB_DEVICE,
    "Universal Serial Bus device",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

STATIC CONST PCI_CLASS_ENTRY  mPciClass0CSerialBus[] = {
  {
    PCI_CLASS_SERIAL_FIREWIRE,
    "FireWire controller",
    mPciClass0CSerialBus00
  },
  {
    PCI_CLASS_SERIAL_ACCESS_BUS,
    "ACCESS.bus",
    NULL
  },
  {
    PCI_CLASS_SERIAL_SSA,
    "SSA",
    NULL
  },
  {
    PCI_CLASS_SERIAL_USB,
    "Universal Serial Bus controller",
    mPciClass0CSerialBus03
  },
  {
    PCI_CLASS_SERIAL_FIBRECHANNEL,
    "Fibre Channel controller",
    NULL
  },
  {
    PCI_CLASS_SERIAL_SMB,
    "SMBus controller",
    NULL
  },
  {
    0x06,
    "InfiniBand controller",
    NULL
  },
  {
    0x07,
    "IPMI controller",
    NULL
  },
  {
    0x08,
    "SERCOS Interface Standard controller",
    NULL
  },
  {
    0x09,
    "CANbus controller",
    NULL
  },
  {
    0x09,
    "MIPI I3C Host Controller",
    NULL
  },
  {
    0x80,
    "Other serial bus controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Wireless subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass0DWireless[] = {
  {
    PCI_SUBCLASS_IRDA,
    "irDA-compatible controller",
    NULL
  },
  {
    PCI_SUBCLASS_IR,
    "Consumer IR controller",
    NULL
  },
  {
    PCI_SUBCLASS_RF,
    "RF controller",
    NULL
  },
  {
    0x11,
    "Bluetooth controller",
    NULL
  },
  {
    0x12,
    "Broadband controller",
    NULL
  },
  {
    0x20,
    "Ethernet 802.11a controller",
    NULL
  },
  {
    0x21,
    "Ethernet 802.11b controller",
    NULL
  },
  {
    0x40,
    "Cellular controller",
    NULL
  },
  {
    0x41,
    "Cellular controller plus Ethernet 802.11",
    NULL
  },
  {
    PCI_SUBCLASS_WIRELESS_OTHER,
    "Other wireless controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Satellite communication subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass0FSatellite[] = {
  {
    PCI_SUBCLASS_TV,
    "Satellite TV controller",
    NULL
  },
  {
    PCI_SUBCLASS_AUDIO,
    "Satellite audio controller",
    NULL
  },
  {
    PCI_SUBCLASS_VOICE,
    "Satellite voice controller",
    NULL
  },
  {
    PCI_SUBCLASS_DATA,
    "Satellite data controller",
    NULL
  },
  {
    0x80,
    "Other satellite controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Encryption/decryption subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass10EncryptionDecryption[] = {
  {
    PCI_SUBCLASS_NET_COMPUT,
    "Network and computing encryption and decryption controller",
    NULL
  },
  {
    PCI_SUBCLASS_ENTERTAINMENT,
    "Entertainment encryption and decryption controller",
    NULL
  },
  {
    PCI_SUBCLASS_SECURITY_OTHER,
    "Other encryption and decryption controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// Data acquisition and signal processing subclasses.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClass11DataAcquisition[] = {
  {
    PCI_SUBCLASS_DPIO,
    "DPIO module",
    NULL
  },
  {
    0x01,
    "Performance counters",
    NULL
  },
  {
    0x10,
    "Communications synchronization plus time and frequency test/measurement",
    NULL
  },
  {
    0x20,
    "Management card",
    NULL
  },
  {
    PCI_SUBCLASS_DPIO_OTHER,
    "Other data acquisition/signal processing controller",
    NULL
  },
  {
    0x00,
    NULL,
    NULL
  }
};

//
// PCI classes.
//
STATIC CONST PCI_CLASS_ENTRY  mPciClasses[] = {
  {
    PCI_CLASS_OLD,
    "Legacy PCI class",
    mPciClass00PreClassCode
  },
  {
    PCI_CLASS_MASS_STORAGE,
    "Mass storage controller",
    mPciClass01MassStorage
  },
  {
    PCI_CLASS_NETWORK,
    "Network controller",
    mPciClass02Network
  },
  {
    PCI_CLASS_DISPLAY,
    "Display controller",
    mPciClass03Display
  },
  {
    PCI_CLASS_MEDIA,
    "Multimedia controller",
    mPciClass04Multimedia
  },
  {
    PCI_CLASS_MEMORY_CONTROLLER,
    "Memory controller",
    mPciClass05Memory
  },
  {
    PCI_CLASS_BRIDGE,
    "Bridge device",
    mPciClass06Bridge
  },
  {
    PCI_CLASS_SCC,
    "Simple communications controller",
    mPciClass07SimpleComms
  },
  {
    PCI_CLASS_SYSTEM_PERIPHERAL,
    "Base system peripheral",
    mPciClass08SystemPerip
  },
  {
    PCI_CLASS_INPUT_DEVICE,
    "Input device",
    mPciClass09Input
  },
  {
    PCI_CLASS_DOCKING_STATION,
    "Docking station",
    mPciClass0ADocking
  },
  {
    PCI_CLASS_PROCESSOR,
    "Processor",
    mPciClass0BProcessor
  },
  {
    PCI_CLASS_SERIAL,
    "Serial bus controller",
    mPciClass0CSerialBus
  },
  {
    PCI_CLASS_WIRELESS,
    "Wireless controller",
    mPciClass0DWireless
  },
  {
    PCI_CLASS_INTELLIGENT_IO,
    "Intelligent I/O controller",
    NULL
  },
  {
    PCI_CLASS_SATELLITE,
    "Satellite communication controller",
    mPciClass0FSatellite
  },
  {
    PCI_SECURITY_CONTROLLER,
    "Encryption/decryption controller",
    mPciClass10EncryptionDecryption
  },
  {
    PCI_CLASS_DPIO,
    "Data acquisition and signal processing controller",
    mPciClass11DataAcquisition
  },
  {
    0x12,
    "Processing accelerator",
    NULL
  },
  {
    0x13,
    "Non-essential instrumentation function"
  }
};

STATIC
VOID
GetPciDeviceClassText (
  IN  UINT8        BaseClass,
  IN  UINT8        SubClass,
  IN  UINT8        Interface,
  OUT CONST CHAR8  **TextBaseClass,
  OUT CONST CHAR8  **TextSubClass
  )
{
  UINTN                  Index;
  CONST PCI_CLASS_ENTRY  *BaseClassEntry;
  CONST PCI_CLASS_ENTRY  *SubClassEntry;
  CONST PCI_CLASS_ENTRY  *InterfaceEntry;

  //
  // Get base class.
  //
  BaseClassEntry = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mPciClasses); Index++) {
    if (mPciClasses[Index].Code == BaseClass) {
      BaseClassEntry = &mPciClasses[Index];
    }
  }

  if (BaseClassEntry == NULL) {
    *TextBaseClass = "Unknown class";
    *TextSubClass  = NULL;
    return;
  }

  *TextBaseClass = BaseClassEntry->DescText;

  //
  // Get subclass if present.
  //
  if (BaseClassEntry->LowerLevelClass == NULL) {
    *TextSubClass = NULL;
    return;
  }

  Index         = 0;
  SubClassEntry = NULL;
  while (BaseClassEntry->LowerLevelClass[Index].DescText != NULL) {
    if (BaseClassEntry->LowerLevelClass[Index].Code == SubClass) {
      SubClassEntry = &BaseClassEntry->LowerLevelClass[Index];
      break;
    }

    Index++;
  }

  //
  // Use other subclass if subclass not found.
  //
  if (SubClassEntry == NULL) {
    Index = 0;
    while (BaseClassEntry->LowerLevelClass[Index].DescText != NULL) {
      if (BaseClassEntry->LowerLevelClass[Index].Code == SubClass) {
        SubClassEntry = &BaseClassEntry->LowerLevelClass[Index];
        break;
      }

      Index++;
    }
  }

  if (SubClassEntry == NULL) {
    *TextSubClass = NULL;
    return;
  }

  //
  // Get interface if present.
  //
  if (SubClassEntry->LowerLevelClass == NULL) {
    *TextSubClass = SubClassEntry->DescText;
    return;
  }

  Index          = 0;
  InterfaceEntry = NULL;
  while (SubClassEntry->LowerLevelClass[Index].DescText != NULL) {
    if (SubClassEntry->LowerLevelClass[Index].Code == Interface) {
      InterfaceEntry = &SubClassEntry->LowerLevelClass[Index];
      break;
    }

    Index++;
  }

  if (InterfaceEntry == NULL) {
    *TextSubClass = SubClassEntry->DescText;
    return;
  }

  *TextSubClass = InterfaceEntry->DescText;
}

EFI_STATUS
OcPciInfoDump (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                PciDevice;
  CONST CHAR8               *TextPciClass;
  CONST CHAR8               *TextPciSubClass;
  EFI_DEVICE_PATH_PROTOCOL  *PciDevicePath;
  CHAR16                    *TextPciDevicePath;

  CHAR8   *FileBuffer;
  UINTN   FileBufferSize;
  CHAR16  TmpFileName[32];

  ASSERT (Root != NULL);

  FileBufferSize = SIZE_1KB;
  FileBuffer     = AllocateZeroPool (FileBufferSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDM: No PCI devices found for dumping - %r\n", Status));
    FreePool (FileBuffer);
    return Status;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Read the whole PCI device in 32-bit.
    //
    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (PCI_TYPE00) / sizeof (UINT32),
                          &PciDevice
                          );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Dump PCI class text.
    //
    GetPciDeviceClassText (
      PciDevice.Hdr.ClassCode[2],
      PciDevice.Hdr.ClassCode[1],
      PciDevice.Hdr.ClassCode[0],
      &TextPciClass,
      &TextPciSubClass
      );
    if (TextPciSubClass != NULL) {
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        "%02u. %a - %a",
        Index + 1,
        TextPciClass,
        TextPciSubClass
        );
    } else {
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        "%02u. %a",
        Index + 1,
        TextPciClass
        );
    }

    //
    // Dump PCI info.
    //
    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "\n    Vendor ID: 0x%04X, Device ID: 0x%04X, RevisionID: 0x%02X, ClassCode: 0x%02X%02X%02X",
      Index + 1,
      PciDevice.Hdr.VendorId,
      PciDevice.Hdr.DeviceId,
      PciDevice.Hdr.RevisionID,
      PciDevice.Hdr.ClassCode[2],
      PciDevice.Hdr.ClassCode[1],
      PciDevice.Hdr.ClassCode[0]
      );
    //
    // Dump SubsystemVendorID and SubsystemID if the current device is not a PCI bridge.
    //
    if (!IS_PCI_BRIDGE (&PciDevice)) {
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        ", SubsystemVendorID: 0x%04X, SubsystemID: 0x%04X",
        PciDevice.Device.SubsystemVendorID,
        PciDevice.Device.SubsystemID
        );
    }

    //
    // Also dump device path if possible.
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&PciDevicePath
                    );
    if (!EFI_ERROR (Status)) {
      TextPciDevicePath = ConvertDevicePathToText (PciDevicePath, FALSE, FALSE);
      if (TextPciDevicePath != NULL) {
        OcAsciiPrintBuffer (
          &FileBuffer,
          &FileBufferSize,
          "\n    DevicePath: %s",
          TextPciDevicePath
          );

        FreePool (TextPciDevicePath);
      }
    }

    //
    // Finally, append two newlines.
    //
    OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "\n\n");
  }

  //
  // Save dumped PCI info to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"PCIInfo.txt");
    Status = OcSetFileData (Root, TmpFileName, FileBuffer, (UINT32)AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCDM: Dumped PCI info - %r\n", Status));

    FreePool (FileBuffer);
  }

  return EFI_SUCCESS;
}
