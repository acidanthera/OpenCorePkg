/** @file
  Copyright (C) 2013, dmazar. All rights reserved.
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BootCompatInternal.h"

#include <Guid/OcVariable.h>
#include <IndustryStandard/AppleHibernate.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDeviceTreeLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC CONST UINT8 mAsmRelocationCallGate[] = {
  #include "RelocationCallGate.h"
};

EFI_STATUS
AppleRelocationAllocatePages (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap,
  IN     EFI_ALLOCATE_PAGES    AllocatePages,
  IN     UINTN                 NumberOfPages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS  Status;
  UINTN       EssentialSize;

  EssentialSize = AppleSlideGetRelocationSize (BootCompat);
  if (EssentialSize == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Operating systems up to macOS 10.15 allocate starting with TEXT segment (2MB).
  // macOS 11.0 allocates starting with HIB segment (1MB), but it does not support
  // this quirk anyway due to AllocatePages in AllocateAddress mode Address pointer
  // no longer being reread after the allocation in EfiBoot.
  //
  if (*Memory == KERNEL_TEXT_PADDR && BootCompat->KernelState.RelocationBlock == 0) {
    BootCompat->KernelState.RelocationBlock = BASE_4GB;
    Status = OcAllocatePagesFromTop (
      EfiLoaderData,
      EFI_SIZE_TO_PAGES (EssentialSize),
      &BootCompat->KernelState.RelocationBlock, 
      GetMemoryMap,
      AllocatePages,
      NULL
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCABC: Relocation block (0x%Lx) allocation failure - %r\n",
        EssentialSize,
        Status
        ));
      return Status;
    }
    BootCompat->KernelState.RelocationBlockUsed = 0;
  }

  //
  // Not our allocation.
  //
  if (BootCompat->KernelState.RelocationBlock == 0
    || *Memory < KERNEL_BASE_PADDR
    || *Memory >= KERNEL_BASE_PADDR + EssentialSize) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Track actually occupied memory.
  //
  EssentialSize = (UINTN) (*Memory - KERNEL_BASE_PADDR + EFI_PAGES_TO_SIZE (NumberOfPages));
  if (EssentialSize > BootCompat->KernelState.RelocationBlockUsed) {
    BootCompat->KernelState.RelocationBlockUsed = EssentialSize;
  }

  //
  // Assume that EfiBoot does not try to reallocate memory.
  //
  *Memory = *Memory - KERNEL_BASE_PADDR + BootCompat->KernelState.RelocationBlock;

  return EFI_SUCCESS;
}

EFI_STATUS
AppleRelocationRelease (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat
  )
{
  EFI_STATUS  Status;
  UINTN       EssentialSize;

  EssentialSize = AppleSlideGetRelocationSize (BootCompat);
  if (EssentialSize == 0) {
    return EFI_UNSUPPORTED;
  }

  if (BootCompat->KernelState.RelocationBlock == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->FreePages (
    BootCompat->KernelState.RelocationBlock,
    EFI_SIZE_TO_PAGES (EssentialSize)
    );

  BootCompat->KernelState.RelocationBlock = 0;
  BootCompat->KernelState.RelocationBlockUsed = 0;

  return Status;
}

EFI_STATUS
AppleRelocationVirtualize (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN OUT OC_BOOT_ARGUMENTS    *BA
  )
{
  EFI_STATUS               Status;
  UINTN                    MemoryMapSize;
  UINTN                    DescriptorSize;
  UINT32                   DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR    *MemoryMap;
  EFI_PHYSICAL_ADDRESS     KernelRTAddress;
  UINTN                    NumEntries;
  UINTN                    Index;
  EFI_MEMORY_DESCRIPTOR    *Desc;
  UINTN                    BlockSize;
  UINT8                    *KernelRTBlock;

  MemoryMapSize     = *BA->MemoryMapSize;
  DescriptorSize    = *BA->MemoryMapDescriptorSize;
  DescriptorVersion = *BA->MemoryMapDescriptorVersion;
  MemoryMap         = (EFI_MEMORY_DESCRIPTOR *)(UINTN) *BA->MemoryMap;
  KernelRTAddress   = EFI_PAGES_TO_SIZE (*BA->RuntimeServicesPG)
    - (UINT32) (BootCompat->KernelState.RelocationBlock - KERNEL_BASE_PADDR);

  //
  // (1) Assign virtual addresses to all runtime blocks (but reserved).
  //
  Desc = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;

  for (Index = 0; Index < NumEntries; ++Index) {
    BlockSize = EFI_PAGES_TO_SIZE ((UINTN) Desc->NumberOfPages);

    if (Desc->Type == EfiReservedMemoryType) {
      Desc->Attribute &= ~EFI_MEMORY_RUNTIME;
    } else if ((Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      Desc->VirtualStart = KernelRTAddress + KERNEL_STATIC_VADDR;
      KernelRTAddress += BlockSize;
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  //
  // (2) Transition to virtual memory.
  //
  Status = gRT->SetVirtualAddressMap (
    MemoryMapSize,
    DescriptorSize,
    DescriptorVersion,
    MemoryMap
    );

  //
  // (3) Perform quick dirty defragmentation similarly to EfiBoot to make vaddr = paddr
  // for critical areas like EFI_SYSTEM_TABLE.
  //
  Desc = MemoryMap;

  for (Index = 0; Index < NumEntries; ++Index) {
    if (Desc->Type == EfiRuntimeServicesCode || Desc->Type == EfiRuntimeServicesData) {
      //
      // Get physical address from statically mapped virtual.
      //
      KernelRTBlock = (UINT8 *)(UINTN) (Desc->VirtualStart & (BASE_1GB - 1));
      BlockSize = EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages);
      CopyMem (
        KernelRTBlock + (BootCompat->KernelState.RelocationBlock - KERNEL_BASE_PADDR),
        (VOID*)(UINTN) Desc->PhysicalStart,
        BlockSize
        );

      ZeroMem ((VOID*)(UINTN) Desc->PhysicalStart, BlockSize);

      //
      // (4) Sync changes to EFI_SYSTEM_TABLE location with boot args.
      //
      if (Desc->PhysicalStart <= *BA->SystemTableP
        && *BA->SystemTableP <= LAST_DESCRIPTOR_ADDR (Desc)) {
        *BA->SystemTableP = (UINT32)((UINTN) KernelRTBlock
          + (*BA->SystemTableP - Desc->PhysicalStart)
          + (BootCompat->KernelState.RelocationBlock - KERNEL_BASE_PADDR));
      }

      //
      // Mark old RT block in MemMap as free memory and remove RT attribute.
      //
      Desc->Type = EfiConventionalMemory;
      Desc->Attribute = Desc->Attribute & (~EFI_MEMORY_RUNTIME);
    }

    Desc = NEXT_MEMORY_DESCRIPTOR(Desc, DescriptorSize);
  }

  return Status;
}

VOID
AppleRelocationRebase (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN OUT OC_BOOT_ARGUMENTS    *BA
  )
{
  EFI_STATUS                 Status;
  DTEntry                    MemMap;
  CHAR8                      *PropName;
  DTMemMapEntry              *PropValue;
  OpaqueDTPropertyIterator   OPropIter;
  DTPropertyIterator         PropIter;
  UINT32                     RelocDiff;

  PropIter = &OPropIter;

  RelocDiff = (UINT32) (BootCompat->KernelState.RelocationBlock - KERNEL_BASE_PADDR);

  DTInit ((DTEntry)(UINTN) *BA->DeviceTreeP, BA->DeviceTreeLength);
  Status = DTLookupEntry (NULL, "/chosen/memory-map", &MemMap);

  if (!EFI_ERROR (Status)) {
    Status = DTCreatePropertyIterator (MemMap, &OPropIter);
    if (!EFI_ERROR (Status)) {
      while (!EFI_ERROR (DTIterateProperties (PropIter, &PropName))) {
        //
        // /chosen/memory-map props have DTMemMapEntry values (address, length).
        // We need to correct the addresses in matching types.
        //

        //
        // Filter entries with different size right away.
        //
        if (PropIter->CurrentProperty->Length != sizeof (DTMemMapEntry)) {
          continue;
        }

        //
        // Filter enteries out of the relocation range.
        //
        PropValue = (DTMemMapEntry*)((UINT8 *) PropIter->CurrentProperty + sizeof (DTProperty));
        if (PropValue->Address < BootCompat->KernelState.RelocationBlock
          || PropValue->Address >= BootCompat->KernelState.RelocationBlock + BootCompat->KernelState.RelocationBlockUsed) {
          continue;
        }

        //
        // Patch the addresses up.
        //
        PropValue->Address -= RelocDiff;
      }
    }
  }

  *BA->MemoryMap         -= RelocDiff;
  *BA->KernelAddrP       -= RelocDiff;
  *BA->SystemTableP      -= RelocDiff;
  *BA->RuntimeServicesPG -= EFI_SIZE_TO_PAGES (RelocDiff);
  //
  // Note, this one does not seem to be used by XNU but we set it anyway.
  //
  *BA->RuntimeServicesV  = EFI_PAGES_TO_SIZE (*BA->RuntimeServicesPG) + KERNEL_STATIC_VADDR;
  *BA->DeviceTreeP       -= RelocDiff;
}

UINTN
AppleRelocationCallGate (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN     KERNEL_CALL_GATE     CallGate,
  IN     UINTN                Args,
  IN     UINTN                EntryPoint
  )
{
  UINT8                  *Payload;
  RELOCATION_CALL_GATE   ReloGate;

  //
  // Shift kernel arguments back.
  //
  Args -= (UINTN) (BootCompat->KernelState.RelocationBlock - KERNEL_BASE_PADDR);

  //
  // Provide copying payload that will not be overwritten.
  //
  Payload  = (VOID*)(UINTN) CallGate;
  Payload += ESTIMATED_CALL_GATE_SIZE;
  CopyMem (Payload, mAsmRelocationCallGate, sizeof (mAsmRelocationCallGate));

  //
  // Transition to payload.
  //
  ReloGate = (RELOCATION_CALL_GATE)(UINTN) Payload;
  return ReloGate (
    BootCompat->KernelState.RelocationBlockUsed / sizeof (UINT64),
    EntryPoint,
    BootCompat->KernelState.RelocationBlock,
    Args
    );
}
