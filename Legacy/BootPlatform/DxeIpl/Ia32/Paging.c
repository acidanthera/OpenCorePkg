/** @file

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  Paging.c

Abstract:

Revision History:

**/

#include "DxeIpl.h"
#include "HobGeneration.h"
#include "VirtualMemory.h"

#define EFI_PAGE_SIZE_4K      0x1000
#define EFI_PAGE_SIZE_4M      0x400000

//
// Create 4G 4M-page table
// PDE (31:22)   :  1024 entries
//
#define EFI_MAX_ENTRY_NUM     1024

#define EFI_PDE_ENTRY_NUM     EFI_MAX_ENTRY_NUM

#define EFI_PDE_PAGE_NUM      1

#define EFI_PAGE_NUMBER_4M    (EFI_PDE_PAGE_NUM)

//
// Create 4M 4K-page table
// PTE (21:12)   :  1024 entries
//
#define EFI_PTE_ENTRY_NUM     EFI_MAX_ENTRY_NUM
#define EFI_PTE_PAGE_NUM      1

#define EFI_PAGE_NUMBER_4K    (EFI_PTE_PAGE_NUM)

#define EFI_PAGE_NUMBER       (EFI_PAGE_NUMBER_4M + EFI_PAGE_NUMBER_4K)

VOID
EnableNullPointerProtection (
  UINT8 *PageTable
  )
{
  IA32_PAGE_TABLE_ENTRY_4K                      *PageTableEntry4KB;

  PageTableEntry4KB = (IA32_PAGE_TABLE_ENTRY_4K *)((UINTN)PageTable + EFI_PAGE_NUMBER_4M * EFI_PAGE_SIZE_4K);

  //
  // Fill in the Page Table entries
  // Mark 0~4K as not present
  //
  PageTableEntry4KB->Bits.Present = 0;

  return ;
}

VOID
Ia32Create4KPageTables (
  UINT8 *PageTable
  )
{
  UINT64                                        PageAddress;
  UINTN                                         PTEIndex;
  IA32_PAGE_DIRECTORY_ENTRY_4K                  *PageDirectoryEntry4KB;
  IA32_PAGE_TABLE_ENTRY_4K                      *PageTableEntry4KB;

  PageAddress = 0;

  //
  //  Page Table structure 2 level 4K.
  //
  //  Page Table 4K  : PageDirectoryEntry4K      : bits 31-22
  //                   PageTableEntry            : bits 21-12
  //

  PageTableEntry4KB = (IA32_PAGE_TABLE_ENTRY_4K *)((UINTN)PageTable + EFI_PAGE_NUMBER_4M * EFI_PAGE_SIZE_4K);
  PageDirectoryEntry4KB = (IA32_PAGE_DIRECTORY_ENTRY_4K *)((UINTN)PageTable);

  PageDirectoryEntry4KB->Uint32 = (UINT32)(UINTN)PageTableEntry4KB;
  PageDirectoryEntry4KB->Bits.ReadWrite = 0;
  PageDirectoryEntry4KB->Bits.Present = 1;
  PageDirectoryEntry4KB->Bits.MustBeZero = 1;

  for (PTEIndex = 0; PTEIndex < EFI_PTE_ENTRY_NUM; PTEIndex++, PageTableEntry4KB++) {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry4KB->Uint32 = (UINT32)PageAddress;
    PageTableEntry4KB->Bits.ReadWrite = 1;
    PageTableEntry4KB->Bits.Present = 1;

    PageAddress += EFI_PAGE_SIZE_4K;
  }

  return ;
}

VOID
Ia32Create4MPageTables (
  UINT8 *PageTable
  )
{
  UINT32                                        PageAddress;
  UINT8                                         *TempPageTable;
  UINTN                                         PDEIndex;
  IA32_PAGE_TABLE_ENTRY_4M                      *PageDirectoryEntry4MB;

  TempPageTable = PageTable;

  PageAddress = 0;

  //
  //  Page Table structure 1 level 4MB.
  //
  //  Page Table 4MB : PageDirectoryEntry4M      : bits 31-22
  //

  PageDirectoryEntry4MB = (IA32_PAGE_TABLE_ENTRY_4M *)TempPageTable;

  for (PDEIndex = 0; PDEIndex < EFI_PDE_ENTRY_NUM; PDEIndex++, PageDirectoryEntry4MB++) {
    //
    // Fill in the Page Directory entries
    //
    PageDirectoryEntry4MB->Uint32 = (UINT32)PageAddress;
    PageDirectoryEntry4MB->Bits.ReadWrite = 1;
    PageDirectoryEntry4MB->Bits.Present = 1;
    PageDirectoryEntry4MB->Bits.MustBe1 = 1;

    PageAddress += EFI_PAGE_SIZE_4M;
  }

  return ;
}

VOID *
PreparePageTable (
  VOID  *PageNumberTop,
  UINT8 SizeOfMemorySpace 
  )
/*++
Description:
  Generate pagetable below PageNumberTop, 
  and return the bottom address of pagetable for putting other things later.
--*/
{
  VOID *PageNumberBase;

  PageNumberBase = (VOID *)((UINTN)PageNumberTop - EFI_PAGE_NUMBER * EFI_PAGE_SIZE_4K);
  ZeroMem (PageNumberBase, EFI_PAGE_NUMBER * EFI_PAGE_SIZE_4K);

  Ia32Create4MPageTables (PageNumberBase);
  Ia32Create4KPageTables (PageNumberBase);
  //
  // Not enable NULL Pointer Protection if using INTX call
  //
//  EnableNullPointerProtection (PageNumberBase);

  return PageNumberBase;
}
