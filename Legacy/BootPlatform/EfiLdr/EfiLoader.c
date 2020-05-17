/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiLoader.c

Abstract:

Revision History:

--*/

#include "EfiLdr.h"
#include "Support.h"
#include "PeLoader.h"
#include "LzmaDecompress.h"

#include <Library/SerialPortLib.h>

EFILDR_LOADED_IMAGE    DxeCoreImage;
EFILDR_LOADED_IMAGE    DxeIplImage;

VOID
SystemHang (
  CHAR8        *Message
  )
{
  CpuDeadLoop();
}

VOID
EfiLoader (
  UINT32    BiosMemoryMapBaseAddress
  )
{
  BIOS_MEMORY_MAP       *BiosMemoryMap;    
  EFILDR_IMAGE          *EFILDRImage;
  EFI_MEMORY_DESCRIPTOR EfiMemoryDescriptor[EFI_MAX_MEMORY_DESCRIPTORS];
  EFI_STATUS            Status;
  UINTN                 NumberOfMemoryMapEntries;
  UINT32                DestinationSize;
  UINT32                ScratchSize;
  UINTN                 BfvPageNumber;
  UINTN                 BfvBase;
  EFI_MAIN_ENTRYPOINT   EfiMainEntrypoint;
  EFILDRHANDOFF         Handoff;

#if 0
  SerialPortInitialize ();
  SerialPortWrite ((UINT8*) "EfiLdr\r\n", AsciiStrLen ("EfiLdr\r\n"));
#endif

  //
  // Add all EfiConventionalMemory descriptors to the table.  If there are partial pages, then
  // round the start address up to the next page, and round the length down to a page boundary.
  //
  BiosMemoryMap = (BIOS_MEMORY_MAP *) (UINTN) BiosMemoryMapBaseAddress;
  NumberOfMemoryMapEntries = 0;
  GenMemoryMap (&NumberOfMemoryMapEntries, EfiMemoryDescriptor, BiosMemoryMap);

  //
  // Get information on where the image is in memory
  //
  EFILDRImage  = (EFILDR_IMAGE *)(UINTN)(EFILDR_HEADER_ADDRESS + sizeof(EFILDR_HEADER));


  //
  // Point to the 4th image (Bfv)
  //
  EFILDRImage += 3;

  //
  // Decompress the image
  //
  Status = LzmaUefiDecompressGetInfo (
             (VOID *)(UINTN)(EFILDR_HEADER_ADDRESS + EFILDRImage->Offset),
             EFILDRImage->Length,
             &DestinationSize, 
             &ScratchSize
             );

  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to get decompress information for BFV!\n");
  }
  
  Status =  LzmaUefiDecompress (
    (VOID *)(UINTN)(EFILDR_HEADER_ADDRESS + EFILDRImage->Offset),
    EFILDRImage->Length,
    (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS, 
    (VOID *)(UINTN)((EFI_DECOMPRESSED_BUFFER_ADDRESS + DestinationSize + 0x1000) & 0xfffff000)
    );
  

  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to decompress BFV!\n");
  }

  BfvPageNumber = EFI_SIZE_TO_PAGES (DestinationSize);
  BfvBase = (UINTN) FindSpace (BfvPageNumber, &NumberOfMemoryMapEntries, EfiMemoryDescriptor, EfiRuntimeServicesData, EFI_MEMORY_WB);
  if (BfvBase == 0) {
    SystemHang ("Failed to find free space to hold decompressed BFV\n");
  }
  ZeroMem ((VOID *)(UINTN)BfvBase, BfvPageNumber * EFI_PAGE_SIZE);
  CopyMem ((VOID *)(UINTN)BfvBase, (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS, DestinationSize);

  //
  // Point to the 2nd image (DxeIpl)
  //
    
  EFILDRImage -= 2;

  //
  // Decompress the image
  //
  Status = LzmaUefiDecompressGetInfo (
             (VOID *)(UINTN)(EFILDR_HEADER_ADDRESS + EFILDRImage->Offset),
             EFILDRImage->Length,
             &DestinationSize, 
             &ScratchSize
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to get decompress information for DxeIpl!\n");
  }

  Status = LzmaUefiDecompress (
             (VOID *)(UINTN)(EFILDR_HEADER_ADDRESS + EFILDRImage->Offset),
             EFILDRImage->Length,
             (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS,
             (VOID *)(UINTN)((EFI_DECOMPRESSED_BUFFER_ADDRESS + DestinationSize + 0x1000) & 0xfffff000)
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to decompress DxeIpl image\n");
  }

  //
  // Load and relocate the EFI PE/COFF Firmware Image 
  //
  Status = EfiLdrPeCoffLoadPeImage (
             (VOID *)(UINTN)(EFI_DECOMPRESSED_BUFFER_ADDRESS), 
             &DxeIplImage, 
             &NumberOfMemoryMapEntries, 
             EfiMemoryDescriptor
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to load and relocate DxeIpl PE image!\n");
  }

  //
  // Point to the 3rd image (DxeMain)
  //
  EFILDRImage++;

  //
  // Decompress the image
  //
  Status = LzmaUefiDecompressGetInfo (

             (VOID *)(UINTN)(EFILDR_HEADER_ADDRESS + EFILDRImage->Offset),
             EFILDRImage->Length,
             &DestinationSize, 
             &ScratchSize
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to get decompress information for DxeMain FV image!\n");
  }

  Status = LzmaUefiDecompress (
             (VOID *)(UINTN)(EFILDR_HEADER_ADDRESS + EFILDRImage->Offset),
              EFILDRImage->Length,
             (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS,
             (VOID *)(UINTN)((EFI_DECOMPRESSED_BUFFER_ADDRESS + DestinationSize + 0x1000) & 0xfffff000)
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to decompress DxeMain FV image!\n");
  }

  //
  // Load and relocate the EFI PE/COFF Firmware Image 
  //
  Status = EfiLdrPeCoffLoadPeImage (
             (VOID *)(UINTN)(EFI_DECOMPRESSED_BUFFER_ADDRESS), 
             &DxeCoreImage, 
             &NumberOfMemoryMapEntries, 
             EfiMemoryDescriptor
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to load/relocate DxeMain!\n");
  }

  //
  // Jump to EFI Firmware
  //

  if (DxeIplImage.EntryPoint != NULL) {

    Handoff.MemDescCount      = NumberOfMemoryMapEntries;
    Handoff.MemDesc           = EfiMemoryDescriptor;
    Handoff.BfvBase           = (VOID *)(UINTN)BfvBase;
    Handoff.BfvSize           = BfvPageNumber * EFI_PAGE_SIZE;
    Handoff.DxeIplImageBase   = (VOID *)(UINTN)DxeIplImage.ImageBasePage;
    Handoff.DxeIplImageSize   = DxeIplImage.NoPages * EFI_PAGE_SIZE;
    Handoff.DxeCoreImageBase  = (VOID *)(UINTN)DxeCoreImage.ImageBasePage;
    Handoff.DxeCoreImageSize  = DxeCoreImage.NoPages * EFI_PAGE_SIZE;
    Handoff.DxeCoreEntryPoint = (VOID *)(UINTN)DxeCoreImage.EntryPoint;

    EfiMainEntrypoint = (EFI_MAIN_ENTRYPOINT)(UINTN) DxeIplImage.EntryPoint;
    EfiMainEntrypoint (&Handoff);
  }

  //
  // There was a problem loading the image, so HALT the system.
  //

  SystemHang ("Failed to jump to DxeIpl!\n");
}

EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  UINT32    BiosMemoryMapBaseAddress
  )
{
  EfiLoader(BiosMemoryMapBaseAddress);
  return EFI_SUCCESS;
}

