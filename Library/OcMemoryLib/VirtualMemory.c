/** @file
  Copyright (C) 2011, dmazar. All rights reserved.
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMemoryLib.h>

PAGE_MAP_AND_DIRECTORY_POINTER  *
OcGetCurrentPageTable (
  OUT UINTN                           *Flags  OPTIONAL
  )
{
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable;
  UINTN                           Cr3;

  Cr3 = AsmReadCr3 ();

  PageTable = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN) (Cr3 & CR3_ADDR_MASK);

  if (Flags != NULL) {
    *Flags = Cr3 & (CR3_FLAG_PWT | CR3_FLAG_PCD);
  }

  return PageTable;
}

STATIC
BOOLEAN
DisablePageTableWriteProtection (
  VOID
  )
{
  UINTN  Cr0;

  Cr0 = AsmReadCr0 ();

  if ((Cr0 & CR0_WP) != 0) {
    AsmWriteCr0 (Cr0 & ~CR0_WP);
    return TRUE;
  }

  return FALSE;
}

STATIC
VOID
EnablePageTableWriteProtection (
  VOID
  )
{
  UINTN  Cr0;

  Cr0 = AsmReadCr0 ();
  AsmWriteCr0 (Cr0 | CR0_WP);
}

EFI_STATUS
OcGetPhysicalAddress (
  IN  PAGE_MAP_AND_DIRECTORY_POINTER   *PageTable  OPTIONAL,
  IN  EFI_VIRTUAL_ADDRESS              VirtualAddr,
  OUT EFI_PHYSICAL_ADDRESS             *PhysicalAddr
  )
{
  EFI_PHYSICAL_ADDRESS            Start;
  VIRTUAL_ADDR                    VA;
  VIRTUAL_ADDR                    VAStart;
  VIRTUAL_ADDR                    VAEnd;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PML4;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PDPE;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PDE;
  PAGE_TABLE_4K_ENTRY             *PTE4K;
  PAGE_TABLE_2M_ENTRY             *PTE2M;
  PAGE_TABLE_1G_ENTRY             *PTE1G;

  if (PageTable == NULL) {
    PageTable = OcGetCurrentPageTable (NULL);
  }

  VA.Uint64 = (UINT64) VirtualAddr;

  //
  // PML4
  //
  PML4 = PageTable;
  PML4 += VA.Pg4K.PML4Offset;
  VAStart.Uint64 = 0;
  VAStart.Pg4K.PML4Offset = VA.Pg4K.PML4Offset;
  VA_FIX_SIGN_EXTEND (VAStart);
  VAEnd.Uint64 = ~(UINT64) 0;
  VAEnd.Pg4K.PML4Offset = VA.Pg4K.PML4Offset;
  VA_FIX_SIGN_EXTEND (VAEnd);

  if (!PML4->Bits.Present) {
    return EFI_NO_MAPPING;
  }

  //
  // PDPE
  //
  PDPE = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN)(PML4->Uint64 & PAGING_4K_ADDRESS_MASK_64);
  PDPE += VA.Pg4K.PDPOffset;
  VAStart.Pg4K.PDPOffset = VA.Pg4K.PDPOffset;
  VAEnd.Pg4K.PDPOffset = VA.Pg4K.PDPOffset;

  if (!PDPE->Bits.Present) {
    return EFI_NO_MAPPING;
  }

  if (PDPE->Bits.MustBeZero & 0x1) {
    //
    // 1GB PDPE
    //
    PTE1G = (PAGE_TABLE_1G_ENTRY *) PDPE;
    Start = PTE1G->Uint64 & PAGING_1G_ADDRESS_MASK_64;
    *PhysicalAddr = Start + VA.Pg1G.PhysPgOffset;
    return EFI_SUCCESS;
  }

  //
  // PDE
  //
  PDE = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN)(PDPE->Uint64 & PAGING_4K_ADDRESS_MASK_64);
  PDE += VA.Pg4K.PDOffset;
  VAStart.Pg4K.PDOffset = VA.Pg4K.PDOffset;
  VAEnd.Pg4K.PDOffset = VA.Pg4K.PDOffset;

  if (!PDE->Bits.Present) {
    return EFI_NO_MAPPING;
  }

  if (PDE->Bits.MustBeZero & 0x1) {
    //
    // 2MB PDE
    //
    PTE2M = (PAGE_TABLE_2M_ENTRY *) PDE;
    Start = PTE2M->Uint64 & PAGING_2M_ADDRESS_MASK_64;
    *PhysicalAddr = Start + VA.Pg2M.PhysPgOffset;
    return EFI_SUCCESS;
  }

  //
  // PTE
  //
  PTE4K = (PAGE_TABLE_4K_ENTRY *)(UINTN)(PDE->Uint64 & PAGING_4K_ADDRESS_MASK_64);
  PTE4K += VA.Pg4K.PTOffset;
  VAStart.Pg4K.PTOffset = VA.Pg4K.PTOffset;
  VAEnd.Pg4K.PTOffset = VA.Pg4K.PTOffset;

  if (!PTE4K->Bits.Present) {
    return EFI_NO_MAPPING;
  }

  Start = PTE4K->Uint64 & PAGING_4K_ADDRESS_MASK_64;
  *PhysicalAddr = Start + VA.Pg4K.PhysPgOffset;

  return EFI_SUCCESS;
}

EFI_STATUS
VmAllocateMemoryPool (
  OUT OC_VMEM_CONTEXT     *Context,
  IN  UINTN               NumPages,
  IN  EFI_GET_MEMORY_MAP  GetMemoryMap  OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    Addr;

  Addr = BASE_4GB;
  Status = OcAllocatePagesFromTop (
    EfiBootServicesData,
    NumPages,
    &Addr,
    GetMemoryMap,
    NULL,
    NULL
    );

  if (!EFI_ERROR (Status)) {
    Context->MemoryPool = (UINT8 *)(UINTN)Addr;
    Context->FreePages  = NumPages;
  }

  return Status;
}

VOID *
VmAllocatePages (
  IN OUT OC_VMEM_CONTEXT  *Context,
  IN     UINTN            NumPages
  )
{
  VOID   *AllocatedPages;

  AllocatedPages = NULL;

  if (Context->FreePages >= NumPages) {
    AllocatedPages = Context->MemoryPool;
    Context->MemoryPool += EFI_PAGES_TO_SIZE (NumPages);
    Context->FreePages  -= NumPages;
  }

  return AllocatedPages;
}

EFI_STATUS
VmMapVirtualPage (
  IN OUT OC_VMEM_CONTEXT                 *Context,
  IN OUT PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable  OPTIONAL,
  IN     EFI_VIRTUAL_ADDRESS             VirtualAddr,
  IN     EFI_PHYSICAL_ADDRESS            PhysicalAddr
  )
{
  EFI_PHYSICAL_ADDRESS            Start;
  VIRTUAL_ADDR                    VA;
  VIRTUAL_ADDR                    VAStart;
  VIRTUAL_ADDR                    VAEnd;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PML4;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PDPE;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PDE;
  PAGE_TABLE_4K_ENTRY             *PTE4K;
  PAGE_TABLE_4K_ENTRY             *PTE4KTmp;
  PAGE_TABLE_2M_ENTRY             *PTE2M;
  PAGE_TABLE_1G_ENTRY             *PTE1G;
  UINTN                           Index;
  BOOLEAN                         WriteProtected;

  if (PageTable == NULL) {
    PageTable = OcGetCurrentPageTable (NULL);
  }

  WriteProtected = DisablePageTableWriteProtection ();

  VA.Uint64 = (UINT64) VirtualAddr;

  //
  // PML4
  //
  PML4 = PageTable;
  PML4 += VA.Pg4K.PML4Offset;

  //
  // There is a problem if our PML4 points to the same table as first PML4 entry
  // since we may mess the mapping of first virtual region (happens in VBox and probably DUET).
  // Check for this on first call and if true, just clear our PML4 - we'll rebuild at a later step.
  //
  if (PML4 != PageTable && PML4->Bits.Present
    && PageTable->Bits.PageTableBaseAddress == PML4->Bits.PageTableBaseAddress) {
    PML4->Uint64 = 0;
  }

  VAStart.Uint64 = 0;
  VAStart.Pg4K.PML4Offset = VA.Pg4K.PML4Offset;
  VA_FIX_SIGN_EXTEND (VAStart);
  VAEnd.Uint64 = ~(UINT64) 0;
  VAEnd.Pg4K.PML4Offset = VA.Pg4K.PML4Offset;
  VA_FIX_SIGN_EXTEND (VAEnd);

  if (!PML4->Bits.Present) {
    PDPE = (PAGE_MAP_AND_DIRECTORY_POINTER *) VmAllocatePages (Context, 1);

    if (PDPE == NULL) {
      if (WriteProtected) {
        EnablePageTableWriteProtection ();
      }
      return EFI_NO_MAPPING;
    }

    ZeroMem (PDPE, EFI_PAGE_SIZE);

    //
    // Init this whole 512 GB region with 512 1GB entry pages to map
    // the first 512 GB physical space.
    //
    PTE1G = (PAGE_TABLE_1G_ENTRY *) PDPE;
    Start = 0;
    for (Index = 0; Index < 512; ++Index) {
      PTE1G->Uint64 = Start & PAGING_1G_ADDRESS_MASK_64;
      PTE1G->Bits.ReadWrite = 1;
      PTE1G->Bits.Present = 1;
      PTE1G->Bits.MustBe1 = 1;
      PTE1G++;
      Start += BASE_1GB;
    }

    //
    // Put it to PML4.
    //
    PML4->Uint64 = ((UINT64)(UINTN) PDPE) & PAGING_4K_ADDRESS_MASK_64;
    PML4->Bits.ReadWrite = 1;
    PML4->Bits.Present = 1;
  }

  //
  // PDPE
  //
  PDPE = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN)(PML4->Uint64 & PAGING_4K_ADDRESS_MASK_64);
  PDPE += VA.Pg4K.PDPOffset;
  VAStart.Pg4K.PDPOffset = VA.Pg4K.PDPOffset;
  VAEnd.Pg4K.PDPOffset = VA.Pg4K.PDPOffset;

  if (!PDPE->Bits.Present || (PDPE->Bits.MustBeZero & 0x1)) {
    PDE = (PAGE_MAP_AND_DIRECTORY_POINTER *) VmAllocatePages(Context, 1);

    if (PDE == NULL) {
      if (WriteProtected) {
        EnablePageTableWriteProtection ();
      }
      return EFI_NO_MAPPING;
    }

    ZeroMem (PDE, EFI_PAGE_SIZE);

    if (PDPE->Bits.MustBeZero & 0x1) {
      //
      // This is 1 GB page. Init new PDE array to get the same
      // mapping but with 2MB pages.
      //
      PTE2M = (PAGE_TABLE_2M_ENTRY *) PDE;
      Start = PDPE->Uint64 & PAGING_1G_ADDRESS_MASK_64;

      for (Index = 0; Index < 512; ++Index) {
        PTE2M->Uint64 = Start & PAGING_2M_ADDRESS_MASK_64;
        PTE2M->Bits.ReadWrite = 1;
        PTE2M->Bits.Present = 1;
        PTE2M->Bits.MustBe1 = 1;
        PTE2M++;
        Start += BASE_2MB;
      }
    }

    //
    // Put it to PDPE.
    //
    PDPE->Uint64 = ((UINT64)(UINTN) PDE) & PAGING_4K_ADDRESS_MASK_64;
    PDPE->Bits.ReadWrite = 1;
    PDPE->Bits.Present = 1;
  }

  //
  // PDE
  //
  PDE = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN)(PDPE->Uint64 & PAGING_4K_ADDRESS_MASK_64);
  PDE += VA.Pg4K.PDOffset;
  VAStart.Pg4K.PDOffset = VA.Pg4K.PDOffset;
  VAEnd.Pg4K.PDOffset = VA.Pg4K.PDOffset;

  if (!PDE->Bits.Present || (PDE->Bits.MustBeZero & 0x1)) {
    PTE4K = (PAGE_TABLE_4K_ENTRY *) VmAllocatePages (Context, 1);

    if (PTE4K == NULL) {
      if (WriteProtected) {
        EnablePageTableWriteProtection ();
      }
      return EFI_NO_MAPPING;
    }

    ZeroMem (PTE4K, EFI_PAGE_SIZE);

    if (PDE->Bits.MustBeZero & 0x1) {
      //
      // This is 2 MB page. Init new PTE array to get the same
      // mapping but with 4KB pages.
      //
      PTE4KTmp = (PAGE_TABLE_4K_ENTRY *)PTE4K;
      Start = PDE->Uint64 & PAGING_2M_ADDRESS_MASK_64;

      for (Index = 0; Index < 512; ++Index) {
        PTE4KTmp->Uint64 = Start & PAGING_4K_ADDRESS_MASK_64;
        PTE4KTmp->Bits.ReadWrite = 1;
        PTE4KTmp->Bits.Present = 1;
        PTE4KTmp++;
        Start += BASE_4KB;
      }
    }

    //
    // Put it to PDE.
    //
    PDE->Uint64 = ((UINT64)(UINTN) PTE4K) & PAGING_4K_ADDRESS_MASK_64;
    PDE->Bits.ReadWrite = 1;
    PDE->Bits.Present = 1;
  }

  //
  // PTE
  //
  PTE4K = (PAGE_TABLE_4K_ENTRY *)(UINTN)(PDE->Uint64 & PAGING_4K_ADDRESS_MASK_64);
  PTE4K += VA.Pg4K.PTOffset;
  VAStart.Pg4K.PTOffset = VA.Pg4K.PTOffset;
  VAEnd.Pg4K.PTOffset = VA.Pg4K.PTOffset;

  //
  // Put it to PTE.
  //
  PTE4K->Uint64 = ((UINT64) PhysicalAddr) & PAGING_4K_ADDRESS_MASK_64;
  PTE4K->Bits.ReadWrite = 1;
  PTE4K->Bits.Present = 1;

  if (WriteProtected) {
    EnablePageTableWriteProtection ();
  }

  return EFI_SUCCESS;
}

EFI_STATUS
VmMapVirtualPages (
  IN OUT OC_VMEM_CONTEXT                 *Context,
  IN OUT PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable  OPTIONAL,
  IN     EFI_VIRTUAL_ADDRESS             VirtualAddr,
  IN     UINT64                          NumPages,
  IN     EFI_PHYSICAL_ADDRESS            PhysicalAddr
  )
{
  EFI_STATUS  Status;

  if (PageTable == NULL) {
    PageTable = OcGetCurrentPageTable (NULL);
  }

  Status = EFI_SUCCESS;

  while (NumPages > 0 && !EFI_ERROR (Status)) {
    Status = VmMapVirtualPage (
      Context,
      PageTable,
      VirtualAddr,
      PhysicalAddr
      );

    VirtualAddr  += EFI_PAGE_SIZE;
    PhysicalAddr += EFI_PAGE_SIZE;
    NumPages--;
  }

  return Status;
}

VOID
VmFlushCaches (
  VOID
  )
{
  //
  // Simply reload CR3 register.
  //
  AsmWriteCr3 (AsmReadCr3 ());
}
