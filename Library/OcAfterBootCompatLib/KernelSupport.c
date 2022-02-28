/** @file
  Copyright (C) 2013, dmazar. All rights reserved.
  Copyright (C) 2019, vit9696. All rights reserved.

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

/**
  Protect RT data from boot.efi relocation by marking them MemMapIO.
  See more details in the function definition.

  @param[in,out]  RtReloc           Relocation entry list to store entry types.
  @param[in]      MemoryMapSize     Memory map size.
  @param[in]      DescriptorSize    Memory map descriptor size.
  @param[in,out]  MemoryMap         MemoryMap to protect entries in.
  @param[in]      SysTableArea      Special address that should not be protected.
**/
STATIC
VOID
ProtectRtMemoryFromRelocation (
  IN OUT RT_RELOC_PROTECT_DATA  *RtReloc,
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     EFI_PHYSICAL_ADDRESS   SysTableArea,
  IN     UINTN                  SysTableAreaSize
  )
{
  //
  // We protect RT data & code from relocation by marking them MemMapIO except EFI_SYSTEM_TABLE area.
  //
  // This fixes NVRAM issues on some boards where access to NVRAM after boot services is possible
  // only in SMM mode. RT driver passes data to SMM handler through previously negotiated buffer
  // and this buffer must not be relocated.
  // Explained and examined in detail by CodeRush and night199uk:
  // https://web.archive.org/web/20141025080709/http://www.projectosx.com/forum/lofiversion/index.php/t3298.html
  //
  // Starting with APTIO V for NVRAM to work not only RT data but RT code too can no longer be moved
  // due to the use of commbuffers. This, however, creates a memory protection issue, because
  // XNU maps RT data as RW and code as RX, and AMI appears use global variables in some RT drivers.
  // For this reason we shim (most?) affected RT services via wrapers that unset the WP bit during
  // the UEFI call and set it back on return in a separate driver.
  // Explained in detail by Download-Fritz and vit9696:
  // http://www.insanelymac.com/forum/topic/331381-aptiomemoryfix (first 2 links in particular).
  //
  // EFI_SYSTEM_TABLE is passed directly through kernel boot arguments, and thus goes through static
  // mapping (ml_static_ptovirt) in efi_set_tables_64 call. This mapping works as PHYS | CONST = VIRT.
  // To avoid kernel accessing unmapped virtual address we let boot.efi relocate the page with
  // EFI_SYSTEM_TABLE area. While technically it is possible to let the original page to be relocated,
  // we pick a safer root by using a private copy.
  //
  // The primary downside of this approach is that boot.efi will still reserve the contiguous memory
  // for runtime services after the kernel: efiRuntimeServicesPageCount pages starting from
  // efiRuntimeServicesPageStart within kaddr ~ ksize range. However, unlike Macs, which have reserved
  // gaps only for ACPI NVS, MemMapIO and similar regions, with this approach almost no physical memory
  // in efiRuntimeServicesPageStart area is used at all. This memory is never reclaimed by XNU, which
  // marks it as allocated in i386_vm_init. Expirements show that at least 85 MBs (Z170) are used for
  // this process. On server systems the issue is much worse due to many devices in place.
  // Ideally boot.efi should only count RT code and RT data pages, but it is not easy to change.
  //

  UINTN                   NumEntries;
  UINTN                   Index;
  EFI_MEMORY_DESCRIPTOR   *Desc;
  RT_RELOC_PROTECT_INFO   *RelocInfo;

  Desc                = MemoryMap;
  RtReloc->NumEntries = 0;
  RelocInfo           = &RtReloc->RelocInfo[0];
  NumEntries          = MemoryMapSize / DescriptorSize;

  for (Index = 0; Index < NumEntries; ++Index) {
    if ((Desc->Attribute & EFI_MEMORY_RUNTIME) != 0
      && Desc->NumberOfPages > 0
      && (Desc->Type == EfiRuntimeServicesCode || Desc->Type == EfiRuntimeServicesData)
      && !AREA_WITHIN_DESCRIPTOR (Desc, SysTableArea, SysTableAreaSize)) {

      if (RtReloc->NumEntries == ARRAY_SIZE (RtReloc->RelocInfo)) {
        RUNTIME_DEBUG ((
          DEBUG_ERROR,
          "OCABC: Cannot save mem type for entry: %Lx (type 0x%x)\n",
          (UINT64) Desc->PhysicalStart,
          (UINT32) Desc->Type
          ));
        return;
      }

      RelocInfo->PhysicalStart = Desc->PhysicalStart;
      RelocInfo->PhysicalEnd   = LAST_DESCRIPTOR_ADDR (Desc);
      RelocInfo->Type          = Desc->Type;
      Desc->Type               = EfiMemoryMappedIO;
      ++RelocInfo;
      ++RtReloc->NumEntries;
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }
}

/**
  Copy RT flagged areas to separate memmap, define virtual to physical address mapping,
  and call SetVirtualAddressMap() only with that partial memmap.

  @param[in,out]  KernelState       Kernel support state.
  @param[in]      MemoryMapSize     Memory map size.
  @param[in]      DescriptorSize    Memory map descriptor size.
  @param[in]      DescriptorVersion Memor map descriptor version.
  @param[in,out]  MemoryMap         Complete memory map with all entries.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
PerformRtMemoryVirtualMapping (
  IN OUT KERNEL_SUPPORT_STATE   *KernelState,
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN     UINT32                 DescriptorVersion,
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  //
  // About partial memmap:
  // Some UEFIs are converting pointers to virtual addresses even if they do not
  // point to regions with RT flag. This means that those UEFIs are using
  // Desc->VirtualStart even for non-RT regions. Linux had issues with this:
  // http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=7cb00b72876ea2451eb79d468da0e8fb9134aa8a
  // They are doing it Windows way now - copying RT descriptors to separate
  // mem map and passing that stripped map to SetVirtualAddressMap().
  // We'll do the same, although it seems that just assigning
  // VirtualStart = PhysicalStart for non-RT areas also does the job.
  //
  // About virtual to physical mappings:
  // Also adds virtual to physical address mappings for RT areas. This is needed since
  // SetVirtualAddressMap() does not work on my Aptio without that. Probably because some driver
  // has a bug and is trying to access new virtual addresses during the change.
  // Linux and Windows are doing the same thing and problem is
  // not visible there.
  //

  UINTN                           NumEntries;
  UINTN                           Index;
  EFI_MEMORY_DESCRIPTOR           *Desc;
  EFI_MEMORY_DESCRIPTOR           *VirtualDesc;
  EFI_STATUS                      Status;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable;

  Desc                       = MemoryMap;
  NumEntries                 = MemoryMapSize / DescriptorSize;
  VirtualDesc                = KernelState->VmMap;
  KernelState->VmMapSize     = 0;
  KernelState->VmMapDescSize = DescriptorSize;

  //
  // Get current VM page table.
  //
  PageTable = OcGetCurrentPageTable (NULL);

  for (Index = 0; Index < NumEntries; ++Index) {
    //
    // Legacy note. Some UEFIs end up with "reserved" area with EFI_MEMORY_RUNTIME flag set when
    // Intel HD3000 or HD4000 is used. For example, on GA-H81N-D2H there is a single 1 GB descriptor:
    // 000000009F800000-00000000DF9FFFFF 0000000000040200 8000000000000000
    //
    // All known boot.efi starting from at least 10.5.8 properly handle this flag and do not assign
    // virtual addresses to reserved descriptors. However, our legacy code had a bug, and did not
    // check for EfiReservedMemoryType. Therefore it replaced such entries by EfiMemoryMappedIO
    // to "prevent" boot.efi relocations.
    //
    // The relevant discussion and the original fix can be found here:
    // http://web.archive.org/web/20141111124211/http://www.projectosx.com:80/forum/lofiversion/index.php/t2428-450.html
    // https://sourceforge.net/p/cloverefiboot/code/605/
    //
    // The correct approach is to properly handle EfiReservedMemoryType with EFI_MEMORY_RUNTIME
    // attribute set, and not mess with the memory map passed to boot.efi. As done here.
    //
    if (Desc->Type != EfiReservedMemoryType && (Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      //
      // Check if there is enough space in virtual map.
      //
      if (KernelState->VmMapSize + DescriptorSize > sizeof (KernelState->VmMap)) {
        RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: Too many RT entries to memory map\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Copy region with EFI_MEMORY_RUNTIME flag to virtual map.
      //
      CopyMem (VirtualDesc, Desc, DescriptorSize);

      //
      // Define virtual to physical mapping.
      //
      Status = VmMapVirtualPages (
        &KernelState->VmContext,
        PageTable,
        Desc->VirtualStart,
        Desc->NumberOfPages,
        Desc->PhysicalStart
        );
      if (EFI_ERROR (Status)) {
        RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: RT mapping failure - %r\n", Status));
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Proceed to next virtual map slot.
      //
      VirtualDesc             = NEXT_MEMORY_DESCRIPTOR (VirtualDesc, DescriptorSize);
      KernelState->VmMapSize += DescriptorSize;
    }

    //
    // Proceed to next original map slot.
    //
    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  VmFlushCaches ();

  Status = gRT->SetVirtualAddressMap (
    KernelState->VmMapSize,
    DescriptorSize,
    DescriptorVersion,
    KernelState->VmMap
    );

  return Status;
}

/**
  Revert RT data protected types to let XNU kernel kernel properly map data.

  @param[in]      RtReloc           Relocated entry list with entry types.
  @param[in]      MemoryMapSize     Memory map size.
  @param[in]      DescriptorSize    Memory map descriptor size.
  @param[in,out]  MemoryMap         MemoryMap to restore protected entries in.
**/
STATIC
VOID
RestoreProtectedRtMemoryTypes (
  IN     RT_RELOC_PROTECT_DATA  *RtReloc,
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  UINTN                  Index;
  UINTN                  Index2;
  UINTN                  NumEntriesLeft;
  UINTN                  NumEntries;
  EFI_PHYSICAL_ADDRESS   PhysicalStart;
  EFI_PHYSICAL_ADDRESS   PhysicalEnd;
  EFI_MEMORY_DESCRIPTOR  *Desc;

  NumEntriesLeft = RtReloc->NumEntries;
  NumEntries     = MemoryMapSize / DescriptorSize;
  Desc           = MemoryMap;

  for (Index = 0; Index < NumEntries && NumEntriesLeft > 0; ++Index) {
    PhysicalStart = Desc->PhysicalStart;
    PhysicalEnd   = LAST_DESCRIPTOR_ADDR (Desc);

    for (Index2 = 0; Index2 < RtReloc->NumEntries; ++Index2) {
      //
      // PhysicalStart match is enough, but just in case.
      // Some types of firmware, such as on the Lenovo ThinkPad X240, have unusual reserved areas.
      // For example, 0000000000000000-FFFFFFFFFFFFFFFF 0000000000000000 0000000000000000.
      // Execute exact comparisons as fuzzy matching often results in errors.
      //
      if (PhysicalStart == RtReloc->RelocInfo[Index2].PhysicalStart
        && PhysicalEnd == RtReloc->RelocInfo[Index2].PhysicalEnd)  {
        Desc->Type = RtReloc->RelocInfo[Index2].Type;
        --NumEntriesLeft;
        break;
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  if (NumEntriesLeft > 0) {
    RUNTIME_DEBUG ((
      DEBUG_ERROR,
      "OCABC: Failed to restore %u entries out of %u\n",
      (UINT32) NumEntriesLeft,
      (UINT32) RtReloc->NumEntries
      ));
  }
}

/**
  Prepare environment for normal booting. Called when boot.efi jumps to kernel.

  @param[in,out]  BootCompat    Boot compatibility context.
  @param[in,out]  BootArgs      Apple kernel boot arguments.
**/
STATIC
VOID
AppleMapPrepareForBooting (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN OUT VOID                 *BootArgs
  )
{
  EFI_STATUS              Status;
  DTEntry                 Chosen;
  CHAR8                   *ArgsStr;
  UINT32                  ArgsSize;
  OC_BOOT_ARGUMENTS       BA;
  UINTN                   MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  UINTN                   DescriptorSize;

  OcParseBootArgs (&BA, BootArgs);

  if (BootCompat->Settings.ProvideCustomSlide) {
    //
    // Restore the variables we tampered with to support custom slides.
    //
    AppleSlideRestore (BootCompat, &BA);
  }

  if (BootCompat->Settings.DisableSingleUser) {
    //
    // First, there is a BootArgs entry for XNU.
    //
    OcRemoveArgumentFromCmd (BA.CommandLine, "-s");
  }

  if (BootCompat->Settings.DisableSingleUser
    || BootCompat->Settings.ForceBooterSignature) {
    DTInit ((VOID *)(UINTN) *BA.DeviceTreeP, BA.DeviceTreeLength);
    Status = DTLookupEntry (NULL, "/chosen", &Chosen);
    if (!EFI_ERROR (Status)) {
      if (BootCompat->Settings.DisableSingleUser) {
        //
        // Second, there is a DT entry.
        //
        Status = DTGetProperty (Chosen, "boot-args", (VOID **) &ArgsStr, &ArgsSize);
        if (!EFI_ERROR (Status) && ArgsSize > 0) {
          OcRemoveArgumentFromCmd (ArgsStr, "-s");
        }
      }

      if (BootCompat->Settings.ForceBooterSignature) {
        Status = DTGetProperty (Chosen, "boot-signature", (VOID **) &ArgsStr, &ArgsSize);
        if (!EFI_ERROR (Status) && ArgsSize == SHA1_DIGEST_SIZE) {
          CopyMem (ArgsStr, BootCompat->Settings.BooterSignature, ArgsSize);
        }
      }
    }
  }

  if (BootCompat->KernelState.RelocationBlock != 0) {
    //
    // When using Relocation Block EfiBoot will not virtualize the addresses since they
    // cannot be mapped 1:1 due to any region from the relocation block being outside
    // of static XNU vaddr to paddr mapping. This causes a clean early exit in their
    // SetVirtualAddressMap calling routine avoiding gRT->SetVirtualAddressMap.
    //
    // For this reason we need to perform it ourselves right here before we restored
    // runtime memory protections as we also need to defragment EFI_SYSTEM_TABLE memory
    // to be accessible from XNU.
    //
    AppleRelocationVirtualize (BootCompat, &BA);
  }

  if (BootCompat->Settings.AvoidRuntimeDefrag) {
    MemoryMapSize  = *BA.MemoryMapSize;
    MemoryMap      = (EFI_MEMORY_DESCRIPTOR *)(UINTN) (*BA.MemoryMap);
    DescriptorSize = *BA.MemoryMapDescriptorSize;

    //
    // We must restore EfiRuntimeServicesCode memory area types, because otherwise
    // RuntimeServices won't be mapped.
    //
    RestoreProtectedRtMemoryTypes (
      &BootCompat->RtReloc,
      MemoryMapSize,
      DescriptorSize,
      MemoryMap
      );

    //
    // On native Macs due to EfiBoot defragmentation it is guaranteed that
    // VADDR % BASE_1GB == PADDR. macOS 11 started to rely on this in
    // acpi_count_enabled_logical_processors, which needs to access MADT (APIC)
    // ACPI table, and does that through ConfigurationTables.
    //
    // The simplest approach is to just copy the table, so that it is accessible
    // at both actual mapping and 1:1 defragmented mapping. This should be safe,
    // as the memory for 1:1 defragmented mapping is reserved by EfiBoot in the
    // first place and is otherwise stolen anyway.
    //
    if (BootCompat->KernelState.ConfigurationTable != NULL) {
      CopyMem (
        (VOID*) ((UINTN) BA.SystemTable->ConfigurationTable & (BASE_1GB - 1)),
        BootCompat->KernelState.ConfigurationTable,
        sizeof (*BootCompat->KernelState.ConfigurationTable) * BA.SystemTable->NumberOfTableEntries
        );
    }
  }

  if (BootCompat->KernelState.RelocationBlock != 0) {
    AppleRelocationRebase (BootCompat, &BA);
  }
}

/**
  Prepare environment for hibernate wake. Called when boot.efi jumps to kernel.

  @param[in,out]  BootCompat       Boot compatibility context.
  @param[in,out]  ImageHeaderPage  Apple hibernate image page number.
**/
STATIC
VOID
AppleMapPrepareForHibernateWake (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN     UINTN                ImageHeaderPage
  )
{
  IOHibernateImageHeader  *ImageHeader;
  IOHibernateHandoff      *Handoff;

  ImageHeader = (IOHibernateImageHeader *) EFI_PAGES_TO_SIZE (ImageHeaderPage);

  //
  // Legacy note. In legacy implementations systemTableOffset was unconditionally overwritten
  // with a wrong address due to ImageHeader->runtimePages not being converted from pages to bytes.
  // Fortunately systemTableOffset was unused when kIOHibernateHandoffTypeMemoryMap is unspecified.
  // systemTableOffset is calculated properly by boot.efi itself starting from 10.6.8 at least,
  // and thus this assignment was useless in the first place.
  //

  //
  // At this step we have two routes.
  //
  // 1. Remove newly generated memory map from hibernate image to let XNU use the original mapping.
  //    This is known to work well on most systems primarily because Windows requires UEFI firmware
  //    to preserve physical memory consistency at S4 wake. "On a UEFI platform, firmware runtime memory
  //    must be consistent across S4 sleep state transitions, in both size and location.", see:
  //    https://docs.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-uefi#hibernation-state-s4-transition-requirements
  // 2. Recover memory map just as we do for normal booting. This created issues on some types of firmware
  //    resulting in unusual memory maps after S4 wake. In other cases, this should not immediately
  //    break things. XNU will entirely remove efiRuntimeServicesPageStart/efiRuntimeServicesPageSize
  //    mapping and our new memory map entries will unconditionally overwrite previous ones. In case
  //    no physical memory changes happened, this should work fine.
  //
  Handoff = (IOHibernateHandoff *) EFI_PAGES_TO_SIZE ((UINTN) ImageHeader->handoffPages);
  while (Handoff->type != kIOHibernateHandoffTypeEnd) {
    if (Handoff->type == kIOHibernateHandoffTypeMemoryMap) {
      if (BootCompat->Settings.DiscardHibernateMap) {
        //
        // Route 1. Discard the new memory map here, and let XNU use what it had.
        // It is unknown whether there are still examples of firmware that need this.
        //
        Handoff->type = kIOHibernateHandoffType;
      } else {
        //
        // Route 2. Recovery memory protection types just as normal boot.
        //

        if (BootCompat->KernelState.VmMapDescSize == 0) {
          RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: Saved descriptor size cannot be 0\n"));
          return;
        }

        //
        // TODO: If we try to work on hibernation support with relocation block
        // We will need to add a call similar to AppleRelocationVirtualize here.
        //

        if (BootCompat->Settings.AvoidRuntimeDefrag) {
          //
          // I think we should not be there, but ideally all quirks are relatively independent.
          //
          RestoreProtectedRtMemoryTypes (
            &BootCompat->RtReloc,
            Handoff->bytecount,
            BootCompat->KernelState.VmMapDescSize,
            (EFI_MEMORY_DESCRIPTOR *)(UINTN) Handoff->data
            );
        }
      }

      break;
    }

    Handoff = (IOHibernateHandoff *) ((UINTN) Handoff + sizeof(Handoff) + Handoff->bytecount);
  }

  //
  // TODO: To support hibernation with relocation block we will need to add a call similar
  // to AppleRelocationRebase here.
  //
}

VOID
AppleMapPrepareMemoryPool (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat
  )
{
  EFI_STATUS  Status;

  if (!BootCompat->Settings.SetupVirtualMap
    || BootCompat->KernelState.VmContext.MemoryPool != NULL) {
    return;
  }

  Status = VmAllocateMemoryPool (
    &BootCompat->KernelState.VmContext,
    OC_DEFAULT_VMEM_PAGE_COUNT,
    BootCompat->ServicePtrs.GetMemoryMap
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCABC: Memory pool allocation failure - %r\n", Status));
  }
}

VOID
AppleMapPrepareBooterState (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN OUT EFI_LOADED_IMAGE      *LoadedImage,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap  OPTIONAL
  )
{
  EFI_STATUS            Status;

  //
  // Allocate memory pool if needed.
  //
  AppleMapPrepareMemoryPool (
    BootCompat
    );

  if (BootCompat->Settings.AvoidRuntimeDefrag) {
    if (BootCompat->KernelState.SysTableRtArea == 0) {
      //
      // Allocate RT data pages for copy of UEFI system table for kernel.
      // This one also has to be 32-bit due to XNU BootArgs structure.
      // The reason for this allocation to be required is because XNU uses static
      // mapping for directly passed pointers (see ProtectRtMemoryFromRelocation).
      //
      BootCompat->KernelState.SysTableRtArea     = BASE_4GB;
      BootCompat->KernelState.SysTableRtAreaSize = gST->Hdr.HeaderSize;
      Status = OcAllocatePagesFromTop (
        EfiRuntimeServicesData,
        EFI_SIZE_TO_PAGES (gST->Hdr.HeaderSize),
        &BootCompat->KernelState.SysTableRtArea,
        GetMemoryMap,
        NULL,
        NULL
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "OCABC: Failed to allocate system table memory - %r\n",
          Status
          ));
        BootCompat->KernelState.SysTableRtArea = 0;
        return;
      }

      //
      // Copy UEFI system table to the new location.
      //
      CopyMem (
        (VOID *)(UINTN) BootCompat->KernelState.SysTableRtArea,
        gST,
        gST->Hdr.HeaderSize
        );
      //
      // Remember physical configuration table location.
      //
      BootCompat->KernelState.ConfigurationTable = gST->ConfigurationTable;
    }

    //
    // Assign loaded image with custom system table.
    //
    LoadedImage->SystemTable =
      (EFI_SYSTEM_TABLE *)(UINTN) BootCompat->KernelState.SysTableRtArea;
  }
}

VOID
AppleMapPrepareKernelJump (
  IN OUT BOOT_COMPAT_CONTEXT    *BootCompat,
  IN     EFI_PHYSICAL_ADDRESS   CallGate
  )
{
  CALL_GATE_JUMP           *CallGateJump;

  //
  // There is no reason to patch the kernel when we do not need it.
  //
  if (!BootCompat->Settings.AvoidRuntimeDefrag
    && !BootCompat->Settings.DiscardHibernateMap
    && !BootCompat->Settings.AllowRelocationBlock
    && !BootCompat->Settings.DisableSingleUser
    && !BootCompat->Settings.ForceBooterSignature) {
    return;
  }

#ifndef MDE_CPU_X64
  RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: Kernel trampolines are unsupported for non-X64\n"));
  CpuDeadLoop ();
#endif

  //
  // Check whether we have address and abort if not.
  //
  if (CallGate == 0) {
    RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: Failed to find call gate address\n"));
    return;
  }

  CallGateJump = (VOID *)(UINTN) CallGate;

  //
  // Move call gate jump bytes front.
  //
  CopyMem (
    CallGateJump + 1,
    CallGateJump,
    ESTIMATED_CALL_GATE_SIZE
    );

  CallGateJump->Command  = 0x25FF;
  CallGateJump->Argument = 0x0;
  CallGateJump->Address  = (UINTN) AppleMapPrepareKernelState;
}

EFI_STATUS
AppleMapPrepareMemState (
  IN OUT BOOT_COMPAT_CONTEXT    *BootCompat,
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN     UINT32                 DescriptorVersion,
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  EFI_STATUS         Status;

  //
  // Protect RT areas from relocation by marking then MemMapIO.
  //
  if (BootCompat->Settings.AvoidRuntimeDefrag) {
    ProtectRtMemoryFromRelocation (
      &BootCompat->RtReloc,
      MemoryMapSize,
      DescriptorSize,
      MemoryMap,
      BootCompat->KernelState.SysTableRtArea,
      BootCompat->KernelState.SysTableRtAreaSize
      );
  }

  //
  // Virtualize RT services with all needed fixes.
  //
  if (BootCompat->Settings.SetupVirtualMap) {
    Status = PerformRtMemoryVirtualMapping (
      &BootCompat->KernelState,
      MemoryMapSize,
      DescriptorSize,
      DescriptorVersion,
      MemoryMap
      );
  } else {
    Status = gRT->SetVirtualAddressMap (
      MemoryMapSize,
      DescriptorSize,
      DescriptorVersion,
      MemoryMap
      );
  }

  //
  // Copy now virtualized UEFI system table for boot.efi to hand it to the kernel.
  //
  if (BootCompat->Settings.AvoidRuntimeDefrag) {
    CopyMem (
      (VOID *)(UINTN) BootCompat->KernelState.SysTableRtArea,
      gST,
      gST->Hdr.HeaderSize
      );
  }

  return Status;
}

UINTN
EFIAPI
AppleMapPrepareKernelState (
  IN UINTN    Args,
  IN UINTN    EntryPoint
  )
{
  BOOT_COMPAT_CONTEXT    *BootCompatContext;
  KERNEL_CALL_GATE       CallGate;

  BootCompatContext = GetBootCompatContext ();

  if (BootCompatContext->ServiceState.AppleHibernateWake) {
    AppleMapPrepareForHibernateWake (
      BootCompatContext,
      Args
      );
  } else {
    AppleMapPrepareForBooting (
      BootCompatContext,
      (VOID *) Args
      );
  }

  CallGate = (KERNEL_CALL_GATE)(UINTN) (
    BootCompatContext->ServiceState.KernelCallGate + CALL_GATE_JUMP_SIZE
    );

  if (BootCompatContext->KernelState.RelocationBlock != 0) {
    return AppleRelocationCallGate (
      BootCompatContext,
      CallGate,
      Args,
      EntryPoint
      );
  }

  return CallGate (Args, EntryPoint);
}
