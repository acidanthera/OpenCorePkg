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

#include "HobGeneration.h"
#include "VirtualMemory.h"

//
// Create 2M-page table
// PML4 (47:39)
// PDPTE (38:30)
// PDE (29:21)
//

#define EFI_2M_PAGE_BITS_NUM    21
#define EFI_MAX_ENTRY_BITS_NUM  9

#define EFI_PAGE_SIZE_4K        0x1000
#define EFI_PAGE_SIZE_2M        (1 << EFI_2M_PAGE_BITS_NUM)

#ifndef MIN
  #define MIN(a, b)               ((a) < (b) ? (a) : (b))
#endif
#define ENTRY_NUM(x)            ((UINTN)1 << (x))

UINT8 gPML4BitsNum;
UINT8 gPDPTEBitsNum;
UINT8 gPDEBitsNum;

UINTN gPageNum2M;
UINTN gPageNum4K;

VOID
EnableNullPointerProtection (
  UINT8                       *PageTable
  )
{
  X64_PAGE_TABLE_ENTRY_4K *PageTableEntry4KB;

  PageTableEntry4KB = (X64_PAGE_TABLE_ENTRY_4K *) (PageTable + gPageNum2M * EFI_PAGE_SIZE_4K);
  //
  // Fill in the Page Table entries
  // Mark 0~4K as not present
  //
  PageTableEntry4KB->Bits.Present = 0;

  return ;
}

VOID
X64Create4KPageTables (
  UINT8                       *PageTable
  )
/*++
Routine Description:
  Create 4K-Page-Table for the low 2M memory.
  This will change the previously created 2M-Page-Table-Entry.
--*/
{
  UINT64                                        PageAddress;
  UINTN                                         PTEIndex;
  X64_PAGE_DIRECTORY_ENTRY_4K                   *PageDirectoryEntry4KB;
  X64_PAGE_TABLE_ENTRY_4K                       *PageTableEntry4KB;

  //
  //  Page Table structure 4 level 4K.
  //
  //                   PageMapLevel4Entry        : bits 47-39
  //                   PageDirectoryPointerEntry : bits 38-30
  //  Page Table 4K  : PageDirectoryEntry4K      : bits 29-21
  //                   PageTableEntry            : bits 20-12
  //

  PageTableEntry4KB = (X64_PAGE_TABLE_ENTRY_4K *)(PageTable + gPageNum2M * EFI_PAGE_SIZE_4K);

  PageDirectoryEntry4KB = (X64_PAGE_DIRECTORY_ENTRY_4K *) (PageTable + 2 * EFI_PAGE_SIZE_4K);
  PageDirectoryEntry4KB->Uint64 = (UINT64)(UINTN)PageTableEntry4KB;
  PageDirectoryEntry4KB->Bits.ReadWrite = 1;
  PageDirectoryEntry4KB->Bits.Present = 1;
  PageDirectoryEntry4KB->Bits.MustBeZero = 0;

  for (PTEIndex = 0, PageAddress = 0; 
       PTEIndex < ENTRY_NUM (EFI_MAX_ENTRY_BITS_NUM); 
       PTEIndex++, PageTableEntry4KB++, PageAddress += EFI_PAGE_SIZE_4K
      ) {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry4KB->Uint64 = (UINT64)PageAddress;
    PageTableEntry4KB->Bits.ReadWrite = 1;
    PageTableEntry4KB->Bits.Present = 1;
  }

  return ;
}

VOID
X64Create2MPageTables (
  UINT8 *PageTable
  )
{
  UINT64                                        PageAddress;
  UINT8                                         *TempPageTable;
  UINTN                                         PML4Index;
  UINTN                                         PDPTEIndex;
  UINTN                                         PDEIndex;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageMapLevel4Entry;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageDirectoryPointerEntry;
  X64_PAGE_TABLE_ENTRY_2M                       *PageDirectoryEntry2MB;

  TempPageTable = PageTable;
  PageAddress   = 0;

  //
  //  Page Table structure 3 level 2MB.
  //
  //                   PageMapLevel4Entry        : bits 47-39
  //                   PageDirectoryPointerEntry : bits 38-30
  //  Page Table 2MB : PageDirectoryEntry2M      : bits 29-21
  //

  PageMapLevel4Entry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)TempPageTable;

  for (PML4Index = 0; PML4Index < ENTRY_NUM (gPML4BitsNum); PML4Index++, PageMapLevel4Entry++) {
    //
    // Each PML4 entry points to a page of Page Directory Pointer entires.
    //  
    TempPageTable += EFI_PAGE_SIZE_4K;
    PageDirectoryPointerEntry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)TempPageTable;

    //
    // Make a PML4 Entry
    //
    PageMapLevel4Entry->Uint64 = (UINT64)(UINTN)(TempPageTable);
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    for (PDPTEIndex = 0; PDPTEIndex < ENTRY_NUM (gPDPTEBitsNum); PDPTEIndex++, PageDirectoryPointerEntry++) {
      //
      // Each Directory Pointer entries points to a page of Page Directory entires.
      //       
      TempPageTable += EFI_PAGE_SIZE_4K;
      PageDirectoryEntry2MB = (X64_PAGE_TABLE_ENTRY_2M *)TempPageTable;

      //
      // Fill in a Page Directory Pointer Entries
      //
      PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN)(TempPageTable);
      PageDirectoryPointerEntry->Bits.ReadWrite = 1;
      PageDirectoryPointerEntry->Bits.Present = 1;

      for (PDEIndex = 0; PDEIndex < ENTRY_NUM (gPDEBitsNum); PDEIndex++, PageDirectoryEntry2MB++) {
        //
        // Fill in the Page Directory entries
        //
        PageDirectoryEntry2MB->Uint64 = (UINT64)PageAddress;
        PageDirectoryEntry2MB->Bits.ReadWrite = 1;
        PageDirectoryEntry2MB->Bits.Present = 1;
        PageDirectoryEntry2MB->Bits.MustBe1 = 1;

        PageAddress += EFI_PAGE_SIZE_2M;
      }
    }
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
  VOID  *PageNumberBase;

  //
  // ** CHANGE START **
  // For AMD hardware with 48 bit wide space the table is rather HUGE.
  // Patch by nms42.
  //
  if (SizeOfMemorySpace > 40) {
    SizeOfMemorySpace = 40;
  }
  //
  // ** CHANGE END **
  //

  SizeOfMemorySpace -= EFI_2M_PAGE_BITS_NUM;
  gPDEBitsNum        = (UINT8) MIN (SizeOfMemorySpace, EFI_MAX_ENTRY_BITS_NUM);
  SizeOfMemorySpace  = (UINT8) (SizeOfMemorySpace - gPDEBitsNum);
  gPDPTEBitsNum      = (UINT8) MIN (SizeOfMemorySpace, EFI_MAX_ENTRY_BITS_NUM);
  SizeOfMemorySpace  = (UINT8) (SizeOfMemorySpace - gPDPTEBitsNum);
  gPML4BitsNum       = SizeOfMemorySpace;
  if (gPML4BitsNum > EFI_MAX_ENTRY_BITS_NUM) {
    return NULL;
  }

  //
  // Suppose we have:
  // 2MPage:
  //   Entry:       PML4   ->     PDPTE     ->     PDE    -> Page
  //   EntryNum:     a              b               c
  // then
  //   Occupy4KPage: 1              a              a*b
  // 
  // 2M 4KPage:
  //   Entry:       PTE    ->     Page
  //   EntryNum:    512
  // then
  //   Occupy4KPage: 1
  //   

  gPageNum2M = 1 + ENTRY_NUM (gPML4BitsNum) + ENTRY_NUM (gPML4BitsNum + gPDPTEBitsNum);
  gPageNum4K = 1;


  PageNumberBase = (VOID *)((UINTN)PageNumberTop - (gPageNum2M + gPageNum4K) * EFI_PAGE_SIZE_4K);
  ZeroMem (PageNumberBase, (gPageNum2M + gPageNum4K) * EFI_PAGE_SIZE_4K);

  X64Create2MPageTables (PageNumberBase);
  X64Create4KPageTables (PageNumberBase);
  //
  // Not enable NULL Pointer Protection if using INTx call
  //
//  EnableNullPointerProtection (PageNumberBase);

  return PageNumberBase;
}
