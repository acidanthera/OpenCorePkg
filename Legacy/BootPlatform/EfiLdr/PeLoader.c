/*++

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PeLoader.c

Abstract:

Revision History:

--*/
#include "EfiLdr.h"
#include "Support.h"

EFI_STATUS
EfiLdrPeCoffLoadPeRelocate (
  IN EFILDR_LOADED_IMAGE      *Image,
  IN EFI_IMAGE_DATA_DIRECTORY *RelocDir,
  IN UINTN                     Adjust,
  IN UINTN                    *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor
  );

EFI_STATUS
EfiLdrPeCoffImageRead (
  IN VOID                 *FHand,
  IN UINTN                Offset,
  IN OUT UINTN            ReadSize,
  OUT VOID                *Buffer
  );

VOID *
EfiLdrPeCoffImageAddress (
  IN EFILDR_LOADED_IMAGE     *Image,
  IN UINTN                   Address
  );

EFI_STATUS
EfiLdrPeCoffSetImageType (
  IN OUT EFILDR_LOADED_IMAGE      *Image,
  IN UINTN                        ImageType
  );

EFI_STATUS
EfiLdrPeCoffCheckImageMachineType (
  IN UINT16           MachineType
  );

EFI_STATUS
EfiLdrGetPeImageInfo (
  IN  VOID                    *FHand,
  OUT UINT64                  *ImageBase,
  OUT UINT32                  *ImageSize
  )
{
  EFI_STATUS                        Status;
  EFI_IMAGE_DOS_HEADER              DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION   PeHdr;

  ZeroMem (&DosHdr, sizeof(DosHdr));
  ZeroMem (&PeHdr, sizeof(PeHdr));

  //
  // Read image headers
  //

  EfiLdrPeCoffImageRead (FHand, 0, sizeof(DosHdr), &DosHdr);
  if (DosHdr.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }

  EfiLdrPeCoffImageRead (FHand, DosHdr.e_lfanew, sizeof(PeHdr), &PeHdr);

  if (PeHdr.Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }
    
  //
  // Verify machine type
  //

  Status = EfiLdrPeCoffCheckImageMachineType (PeHdr.Pe32.FileHeader.Machine);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    *ImageBase = (UINT32)PeHdr.Pe32.OptionalHeader.ImageBase;
  } else if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    *ImageBase = PeHdr.Pe32Plus.OptionalHeader.ImageBase;
  } else {
    return EFI_UNSUPPORTED;
  }
  
  *ImageSize = PeHdr.Pe32.OptionalHeader.SizeOfImage;

  return EFI_SUCCESS;
}

EFI_STATUS
EfiLdrPeCoffLoadPeImage (
  IN VOID                     *FHand,
  IN EFILDR_LOADED_IMAGE      *Image,
  IN UINTN                    *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor
  )
{
  EFI_IMAGE_DOS_HEADER            DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION PeHdr;
  EFI_IMAGE_SECTION_HEADER        *FirstSection;
  EFI_IMAGE_SECTION_HEADER        *Section;
  UINTN                           Index;
  EFI_STATUS                      Status;
  UINT8                           *Base;
  UINT8                           *End;
  EFI_IMAGE_DATA_DIRECTORY        *DirectoryEntry;
  UINTN                           DirCount;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY TempDebugEntry;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *DebugEntry;
  UINTN                           CodeViewSize;
  UINTN                           CodeViewOffset;
  UINTN                           CodeViewFileOffset;
  UINTN                           OptionalHeaderSize;
  UINTN                           PeHeaderSize;
  UINT32                          NumberOfRvaAndSizes;
  EFI_IMAGE_DATA_DIRECTORY        *DataDirectory;
  UINT64                          ImageBase;

  ZeroMem (&DosHdr, sizeof(DosHdr));
  ZeroMem (&PeHdr, sizeof(PeHdr));

  //
  // Read image headers
  //

  EfiLdrPeCoffImageRead (FHand, 0, sizeof(DosHdr), &DosHdr);
  if (DosHdr.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }

  EfiLdrPeCoffImageRead (FHand, DosHdr.e_lfanew, sizeof(PeHdr), &PeHdr);

  if (PeHdr.Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }
    
  //
  // Set the image subsystem type
  //

  Status = EfiLdrPeCoffSetImageType (Image, PeHdr.Pe32.OptionalHeader.Subsystem);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Verify machine type
  //

  Status = EfiLdrPeCoffCheckImageMachineType (PeHdr.Pe32.FileHeader.Machine);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Compute the amount of memory needed to load the image and 
  // allocate it.  This will include all sections plus the codeview debug info.
  // Since the codeview info is actually outside of the image, we calculate
  // its size seperately and add it to the total.
  //
  // Memory starts off as data
  //

  CodeViewSize       = 0;
  CodeViewFileOffset = 0;
  if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(PeHdr.Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
  } else if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(PeHdr.Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
  } else {
    return EFI_UNSUPPORTED;
  }
  for (DirCount = 0; 
       (DirCount < DirectoryEntry->Size / sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)) && (CodeViewSize == 0);
       DirCount++) {
    Status = EfiLdrPeCoffImageRead (
               FHand, 
               DirectoryEntry->VirtualAddress + DirCount * sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY),
               sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY),
               &TempDebugEntry
               );
    if (!EFI_ERROR (Status)) {
      if (TempDebugEntry.Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
        CodeViewSize = TempDebugEntry.SizeOfData;
        CodeViewFileOffset = TempDebugEntry.FileOffset;
      }
    }
  }
    
  CodeViewOffset = PeHdr.Pe32.OptionalHeader.SizeOfImage + PeHdr.Pe32.OptionalHeader.SectionAlignment;
  Image->NoPages = EFI_SIZE_TO_PAGES (CodeViewOffset + CodeViewSize);

  //
  // Compute the amount of memory needed to load the image and 
  // allocate it.  Memory starts off as data
  //

  Image->ImageBasePage = (EFI_PHYSICAL_ADDRESS)(UINTN)FindSpace (Image->NoPages, NumberOfMemoryMapEntries, EfiMemoryDescriptor, EfiRuntimeServicesCode, EFI_MEMORY_WB);
  if (Image->ImageBasePage == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (EFI_ERROR(Status)) {
    return Status;
  }

  Image->Info.ImageBase = (VOID *)(UINTN)Image->ImageBasePage;
  Image->Info.ImageSize = (Image->NoPages << EFI_PAGE_SHIFT) - 1;
  Image->ImageBase      = (UINT8 *)(UINTN)Image->ImageBasePage;
  Image->ImageEof       = Image->ImageBase + Image->Info.ImageSize;
  Image->ImageAdjust    = Image->ImageBase;

  //
  // Copy the Image header to the base location
  //
  Status = EfiLdrPeCoffImageRead (
             FHand, 
             0, 
             PeHdr.Pe32.OptionalHeader.SizeOfHeaders, 
             Image->ImageBase
             );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Load each directory of the image into memory... 
  //  Save the address of the Debug directory for later
  //
  if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    NumberOfRvaAndSizes = PeHdr.Pe32.OptionalHeader.NumberOfRvaAndSizes;
    DataDirectory = PeHdr.Pe32.OptionalHeader.DataDirectory;
  } else {
    NumberOfRvaAndSizes = PeHdr.Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
    DataDirectory = PeHdr.Pe32Plus.OptionalHeader.DataDirectory;
  }
  DebugEntry = NULL;
  for (Index = 0; Index < NumberOfRvaAndSizes; Index++) {
    if ((DataDirectory[Index].VirtualAddress != 0) && (DataDirectory[Index].Size != 0)) {
      Status = EfiLdrPeCoffImageRead (
                 FHand,
                 DataDirectory[Index].VirtualAddress,
                 DataDirectory[Index].Size,
                 Image->ImageBase + DataDirectory[Index].VirtualAddress
                 );
      if (EFI_ERROR(Status)) {
        return Status;
      }
      if (Index == EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
        DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (Image->ImageBase + DataDirectory[Index].VirtualAddress);
      }
    }
  }

  //
  // Load each section of the image
  //

  // BUGBUG: change this to use the in memory copy
  if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    OptionalHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER32);
    PeHeaderSize       = sizeof(EFI_IMAGE_NT_HEADERS32);
  } else {
    OptionalHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER64);
    PeHeaderSize       = sizeof(EFI_IMAGE_NT_HEADERS64);
  }
  FirstSection = (EFI_IMAGE_SECTION_HEADER *) (
                   Image->ImageBase +
                   DosHdr.e_lfanew + 
                   PeHeaderSize + 
                   PeHdr.Pe32.FileHeader.SizeOfOptionalHeader - 
                   OptionalHeaderSize
                   );

  Section = FirstSection;
  for (Index=0; Index < PeHdr.Pe32.FileHeader.NumberOfSections; Index += 1) {

    //
    // Compute sections address
    //

    Base = EfiLdrPeCoffImageAddress (Image, (UINTN)Section->VirtualAddress);
    End = EfiLdrPeCoffImageAddress (Image, (UINTN)(Section->VirtualAddress + Section->Misc.VirtualSize));
        
    if (EFI_ERROR(Status) || !Base || !End) {
      return EFI_LOAD_ERROR;
    }

    //
    // Read the section
    //
 
    if (Section->SizeOfRawData) {
      Status = EfiLdrPeCoffImageRead (FHand, Section->PointerToRawData, Section->SizeOfRawData, Base);
      if (EFI_ERROR(Status)) {
        return Status;
      }
    }

    //
    // If raw size is less then virt size, zero fill the remaining
    //

    if (Section->SizeOfRawData < Section->Misc.VirtualSize) {
      ZeroMem (
        Base + Section->SizeOfRawData, 
        Section->Misc.VirtualSize - Section->SizeOfRawData
        );
    }

    //
    // Next Section
    //

    Section += 1;
  }

  //
  // Copy in CodeView information if it exists
  //
  if (CodeViewSize != 0) {
    Status = EfiLdrPeCoffImageRead (FHand, CodeViewFileOffset, CodeViewSize, Image->ImageBase + CodeViewOffset);
    DebugEntry->RVA = (UINT32) (CodeViewOffset);
  }

  //
  // Apply relocations only if needed
  //
  if (PeHdr.Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    ImageBase = (UINT64)PeHdr.Pe32.OptionalHeader.ImageBase;
  } else {
    ImageBase = PeHdr.Pe32Plus.OptionalHeader.ImageBase;
  }
  if ((UINTN)(Image->ImageBase) != (UINTN) (ImageBase)) {
    Status = EfiLdrPeCoffLoadPeRelocate (
               Image,
               &DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC],
               (UINTN) Image->ImageBase - (UINTN)ImageBase,
               NumberOfMemoryMapEntries,
               EfiMemoryDescriptor
               );

    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // Use exported EFI specific interface if present, else use the image's entry point
  //
  Image->EntryPoint = (EFI_IMAGE_ENTRY_POINT)(UINTN)
                        (EfiLdrPeCoffImageAddress(
                           Image, 
                           PeHdr.Pe32.OptionalHeader.AddressOfEntryPoint
                           ));

  return Status;
}

EFI_STATUS
EfiLdrPeCoffLoadPeRelocate (
  IN EFILDR_LOADED_IMAGE      *Image,
  IN EFI_IMAGE_DATA_DIRECTORY *RelocDir,
  IN UINTN                     Adjust,
  IN UINTN                    *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor
  )
{
  EFI_IMAGE_BASE_RELOCATION   *RelocBase;
  EFI_IMAGE_BASE_RELOCATION   *RelocBaseEnd;
  UINT16                      *Reloc;
  UINT16                      *RelocEnd;
  UINT8                       *Fixup;
  UINT8                       *FixupBase;
  UINT16                      *F16;
  UINT32                      *F32;
  UINT64                      *F64;
  UINT8                       *FixupData;
  UINTN                       NoFixupPages;

  //
  // Find the relocation block
  //

  RelocBase = EfiLdrPeCoffImageAddress (Image, RelocDir->VirtualAddress);
  RelocBaseEnd = EfiLdrPeCoffImageAddress (Image, RelocDir->VirtualAddress + RelocDir->Size);
  if (!RelocBase || !RelocBaseEnd) {
    return EFI_LOAD_ERROR;
  }

  NoFixupPages = EFI_SIZE_TO_PAGES (RelocDir->Size / sizeof(UINT16) * sizeof(UINTN));
  Image->FixupData = (UINT8*) FindSpace (NoFixupPages, NumberOfMemoryMapEntries, EfiMemoryDescriptor, EfiRuntimeServicesData, EFI_MEMORY_WB);
  if (Image->FixupData == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Run the whole relocation block
  //

  FixupData = Image->FixupData;
  while (RelocBase < RelocBaseEnd) {
           
    Reloc = (UINT16 *) ((UINT8 *) RelocBase + sizeof(EFI_IMAGE_BASE_RELOCATION));
    RelocEnd = (UINT16 *) ((UINT8 *) RelocBase + RelocBase->SizeOfBlock);
    FixupBase = EfiLdrPeCoffImageAddress (Image, RelocBase->VirtualAddress);
    if ((UINT8 *) RelocEnd < Image->ImageBase || (UINT8 *) RelocEnd > Image->ImageEof) {
      return EFI_LOAD_ERROR;
    }

    //
    // Run this relocation record
    //

    while (Reloc < RelocEnd) {

      Fixup = FixupBase + (*Reloc & 0xFFF);
      switch ((*Reloc) >> 12) {

      case EFI_IMAGE_REL_BASED_ABSOLUTE:
        break;

      case EFI_IMAGE_REL_BASED_HIGH:
        F16 = (UINT16 *) Fixup;
        *F16  = (UINT16) (*F16 + (UINT16)(((UINT32)Adjust) >> 16));
        if (FixupData != NULL) {
          *(UINT16 *) FixupData = *F16;
          FixupData = FixupData + sizeof(UINT16);
        }
        break;

      case EFI_IMAGE_REL_BASED_LOW:
        F16 = (UINT16 *) Fixup;
        *F16 = (UINT16) (*F16 + (UINT16) Adjust);
        if (FixupData != NULL) {
          *(UINT16 *) FixupData = *F16;
          FixupData = FixupData + sizeof(UINT16);
        }
        break;

      case EFI_IMAGE_REL_BASED_HIGHLOW:
        F32 = (UINT32 *) Fixup;
        *F32 = *F32 + (UINT32) Adjust;
        if (FixupData != NULL) {
          FixupData = ALIGN_POINTER(FixupData, sizeof(UINT32));
          *(UINT32 *) FixupData = *F32;
          FixupData = FixupData + sizeof(UINT32);
        }
        break;

      case EFI_IMAGE_REL_BASED_DIR64:
        F64 = (UINT64 *) Fixup;
        *F64 = *F64 + (UINT64) Adjust;
        if (FixupData != NULL) {
          FixupData = ALIGN_POINTER(FixupData, sizeof(UINT64));
          *(UINT64 *) FixupData = *F64;
          FixupData = FixupData + sizeof(UINT64);
        }
        break;

      case EFI_IMAGE_REL_BASED_HIGHADJ:
        CpuDeadLoop();                 // BUGBUG: not done
        break;

      default:
        CpuDeadLoop();
        return EFI_LOAD_ERROR;
      }

      // Next reloc record
      Reloc += 1;
    }

    // next reloc block
    RelocBase = (EFI_IMAGE_BASE_RELOCATION *) RelocEnd;
  }

  //
  // Add Fixup data to whole Image (assume Fixup data just below the image), so that there is no hole in the descriptor.
  // Because only NoPages or ImageBasePage will be used in EfiLoader(), we update these 2 fields.
  //
  Image->NoPages += NoFixupPages;
  Image->ImageBasePage -= (NoFixupPages << EFI_PAGE_SHIFT);

  return EFI_SUCCESS;
}

EFI_STATUS
EfiLdrPeCoffImageRead (
  IN VOID                 *FHand,
  IN UINTN                Offset,
  IN OUT UINTN            ReadSize,
  OUT VOID                *Buffer
  )
{
  CopyMem (Buffer, (VOID *)((UINTN)FHand + Offset), ReadSize);

  return EFI_SUCCESS;
}

VOID *
EfiLdrPeCoffImageAddress (
  IN EFILDR_LOADED_IMAGE     *Image,
  IN UINTN                   Address
  )
{
  UINT8        *FixedAddress;

  FixedAddress = Image->ImageAdjust + Address;

  if ((FixedAddress < Image->ImageBase) || (FixedAddress > Image->ImageEof)) {
    FixedAddress = NULL;
  }

  return FixedAddress;
}


EFI_STATUS
EfiLdrPeCoffSetImageType (
  IN OUT EFILDR_LOADED_IMAGE      *Image,
  IN UINTN                        ImageType
  )
{
  EFI_MEMORY_TYPE                 CodeType;
  EFI_MEMORY_TYPE                 DataType;

  switch (ImageType) {
  case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
    CodeType = EfiLoaderCode;
    DataType = EfiLoaderData;
    break;

  case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
    CodeType = EfiBootServicesCode;
    DataType = EfiBootServicesData;
    break;

  case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
    CodeType = EfiRuntimeServicesCode;
    DataType = EfiRuntimeServicesData;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  Image->Type = ImageType;
  Image->Info.ImageCodeType = CodeType;    
  Image->Info.ImageDataType = DataType;

  return EFI_SUCCESS;
}

EFI_STATUS
EfiLdrPeCoffCheckImageMachineType (
  IN UINT16           MachineType
  )
{
  EFI_STATUS          Status;

  Status = EFI_UNSUPPORTED;

#ifdef MDE_CPU_IA32
  if (MachineType == EFI_IMAGE_MACHINE_IA32) {
    Status = EFI_SUCCESS;
  }
#endif

#ifdef MDE_CPU_X64
  if (MachineType == EFI_IMAGE_MACHINE_X64) {
    Status = EFI_SUCCESS;
  }
#endif

  return Status;
}
