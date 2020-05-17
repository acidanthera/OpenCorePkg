/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  DxeInit.c

Abstract:

Revision History:

**/

#include "DxeIpl.h"

#include "LegacyTable.h"
#include "HobGeneration.h"

/*
--------------------------------------------------------
 Memory Map: (XX=32,64)
--------------------------------------------------------
0x0
        IVT
0x400
        BDA
0x500

0x7C00
        BootSector
0x10000
        EfiLdr (relocate by efiXX.COM)
0x15000
        Efivar.bin (Load by StartXX.COM)
0x20000
        StartXX.COM (E820 table, Temporary GDT, Temporary IDT)
0x21000
        EfiXX.COM (Temporary Interrupt Handler)
0x22000
        EfiLdr.efi + DxeIpl.Z + DxeMain.Z + BFV.Z
0x86000
        MemoryFreeUnder1M (For legacy driver DMA)
0x90000
        Temporary 4G PageTable for X64 (6 page)
0x9F800
        EBDA
0xA0000
        VGA
0xC0000
        OPROM
0xE0000
        FIRMEWARE
0x100000 (1M)
        Temporary Stack (1M)
0x200000

MemoryAbove1MB.PhysicalStart <-----------------------------------------------------+
        ...                                                                        |
        ...                                                                        |
                        <- Phit.EfiMemoryBottom -------------------+               |
        HOB                                                        |               |
                        <- Phit.EfiFreeMemoryBottom                |               |
                                                                   |     MemoryFreeAbove1MB.ResourceLength
                        <- Phit.EfiFreeMemoryTop ------+           |               |
        MemoryDescriptor (For ACPINVS, ACPIReclaim)    |    4M = CONSUMED_MEMORY   |
                                                       |           |               |
        Permament 4G PageTable for IA32 or      MemoryAllocation   |               |
        Permament 64G PageTable for X64                |           |               |
                        <------------------------------+           |               |
        Permament Stack (0x20 Pages = 128K)                        |               |
                        <- Phit.EfiMemoryTop ----------+-----------+---------------+
        NvFV (64K)                                                                 |
                                                                                 MMIO
        FtwFV (128K)                                                               |  
                        <----------------------------------------------------------+<---------+
        DxeCore                                                                    |          |
                                                                                DxeCore       |
        DxeIpl                                                                     |   Allocated in EfiLdr
                        <----------------------------------------------------------+          |
        BFV                                                                      MMIO         |
                        <- Top of Free Memory reported by E820 --------------------+<---------+
        ACPINVS        or
        ACPIReclaim    or
        Reserved
                        <- Memory Top on RealMemory

0x100000000 (4G)

MemoryFreeAbove4G.Physicalstart <--------------------------------------------------+
                                                                                   |
                                                                                   |
                                                                  MemoryFreeAbove4GB.ResourceLength
                                                                                   |
                                                                                   |
                                <--------------------------------------------------+
*/

VOID
EnterDxeMain (
  IN VOID *StackTop,
  IN VOID *DxeCoreEntryPoint,
  IN VOID *Hob,
  IN VOID *PageTable
  );

VOID
DxeInit (
  IN EFILDRHANDOFF  *Handoff
  )
/*++

  Routine Description:

    This is the entry point after this code has been loaded into memory. 

Arguments:


Returns:

    Calls into EFI Firmware

--*/
{
  VOID                  *StackTop;
  VOID                  *StackBottom;
  VOID                  *PageTableBase;
  VOID                  *MemoryTopOnDescriptor;
  VOID                  *MemoryDescriptor;
  VOID                  *NvStorageBase;
  EFILDRHANDOFF         HandoffCopy;

  CopyMem ((VOID*) &HandoffCopy, (VOID*) Handoff, sizeof (EFILDRHANDOFF));
  Handoff = &HandoffCopy;

  //
  // Hob Generation Guild line:
  //   * Don't report FV as physical memory
  //   * MemoryAllocation Hob should only cover physical memory
  //   * Use ResourceDescriptor Hob to report physical memory or Firmware Device and they shouldn't be overlapped
  PrepareHobCpu ();

  //
  // 1. BFV
  //
  PrepareHobBfv (Handoff->BfvBase, Handoff->BfvSize);

  //
  // 2. Updates Memory information, and get the top free address under 4GB
  //
  MemoryTopOnDescriptor = PrepareHobMemory (Handoff->MemDescCount, Handoff->MemDesc);
  
  //
  // 3. Put [NV], [Stack], [PageTable], [MemDesc], [HOB] just below the [top free address under 4GB]
  //
  
  //   3.1 NV data
  NvStorageBase = PrepareHobNvStorage (MemoryTopOnDescriptor);
  //   3.2 Stack
  StackTop = NvStorageBase;
  StackBottom = PrepareHobStack (StackTop);
  //   3.3 Page Table
  PageTableBase = PreparePageTable (StackBottom, gHob->Cpu.SizeOfMemorySpace);
  //   3.4 MemDesc (will be used in PlatformBds)
  MemoryDescriptor = PrepareHobMemoryDescriptor (PageTableBase, Handoff->MemDescCount, Handoff->MemDesc);
  //   3.5 Copy the Hob itself to EfiMemoryBottom, and update the PHIT Hob
  PrepareHobPhit (StackTop, MemoryDescriptor);

  //
  // 4. Register the memory occupied by DxeCore and DxeIpl together as DxeCore
  //
  PrepareHobDxeCore (
    Handoff->DxeCoreEntryPoint,
    (EFI_PHYSICAL_ADDRESS)(UINTN)Handoff->DxeCoreImageBase,
    (UINTN)Handoff->DxeIplImageBase + (UINTN)Handoff->DxeIplImageSize - (UINTN)Handoff->DxeCoreImageBase
    );

  PrepareHobLegacyTable (gHob);
  
  CompleteHobGeneration ();

  EnterDxeMain (StackTop, Handoff->DxeCoreEntryPoint, gHob, PageTableBase);
 
  //
  // Should never get here
  //
  CpuDeadLoop ();
}

EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFILDRHANDOFF  *Handoff
  )
{
  DxeInit(Handoff);
  return EFI_SUCCESS;
}
