/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiLdr.c

Abstract:

Revision History:

--*/

#ifndef _DUET_EFI_LOADER_H_
#define _DUET_EFI_LOADER_H_

#include "Uefi.h"
#include "EfiLdrHandoff.h"

#include <Protocol/LoadedImage.h>
#include <IndustryStandard/PeImage.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#define INT15_E820_AddressRangeMemory   1
#define INT15_E820_AddressRangeReserved 2
#define INT15_E820_AddressRangeACPI     3
#define INT15_E820_AddressRangeNVS      4

#define EFI_FIRMWARE_BASE_ADDRESS  0x00200000

#define EFI_DECOMPRESSED_BUFFER_ADDRESS 0x00600000

#define EFI_MAX_MEMORY_DESCRIPTORS 64

#define LOADED_IMAGE_SIGNATURE     SIGNATURE_32('l','d','r','i')

typedef struct {
  UINTN                       Signature;
  CHAR16                      *Name;          // Displayable name
  UINTN                       Type;

  BOOLEAN                     Started;        // If entrypoint has been called
  VOID                        *StartImageContext;

  EFI_IMAGE_ENTRY_POINT       EntryPoint;     // The image's entry point
  EFI_LOADED_IMAGE_PROTOCOL   Info;           // loaded image protocol

  // 
  EFI_PHYSICAL_ADDRESS        ImageBasePage;  // Location in memory
  UINTN                       NoPages;        // Number of pages 
  UINT8                       *ImageBase;     // As a char pointer
  UINT8                       *ImageEof;      // End of memory image

  // relocate info
  UINT8                       *ImageAdjust;   // Bias for reloc calculations
  UINTN                       StackAddress;
  UINT8                       *FixupData;     //  Original fixup data
} EFILDR_LOADED_IMAGE;

#pragma pack(4)
typedef struct {          
  UINT64       BaseAddress;
  UINT64       Length;
  UINT32       Type;
} BIOS_MEMORY_MAP_ENTRY;
#pragma pack()

typedef struct {          
  UINT32                MemoryMapSize;
  BIOS_MEMORY_MAP_ENTRY MemoryMapEntry[1];
} BIOS_MEMORY_MAP;

typedef
VOID
(EFIAPI * EFI_MAIN_ENTRYPOINT) (
    IN EFILDRHANDOFF  *Handoff
    );

#endif //_DUET_EFI_LOADER_H_
