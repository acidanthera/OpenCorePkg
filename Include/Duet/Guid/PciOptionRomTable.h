/** @file
  GUID and data structure used to describe the list of PCI Option ROMs present in a system.
  
Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __PCI_OPTION_ROM_TABLE_GUID_H_
#define __PCI_OPTION_ROM_TABLE_GUID_H_

#define EFI_PCI_OPTION_ROM_TABLE_GUID \
  { 0x7462660f, 0x1cbd, 0x48da, {0xad, 0x11, 0x91, 0x71, 0x79, 0x13, 0x83, 0x1c } }

extern EFI_GUID gEfiPciOptionRomTableGuid;

typedef struct {
  EFI_PHYSICAL_ADDRESS   RomAddress; 
  EFI_MEMORY_TYPE        MemoryType;
  UINT32                 RomLength; 
  UINT32                 Seg; 
  UINT8                  Bus; 
  UINT8                  Dev; 
  UINT8                  Func; 
  BOOLEAN                ExecutedLegacyBiosImage; 
  BOOLEAN                DontLoadEfiRom;
} EFI_PCI_OPTION_ROM_DESCRIPTOR;

typedef struct {
  UINT64                         PciOptionRomCount;
  EFI_PCI_OPTION_ROM_DESCRIPTOR   *PciOptionRomDescriptors;
} EFI_PCI_OPTION_ROM_TABLE;

#endif // __PCI_OPTION_ROM_TABLE_GUID_H_

