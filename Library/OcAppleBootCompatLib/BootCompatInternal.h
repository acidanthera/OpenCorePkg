/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef BOOT_COMPAT_INTERNAL_H
#define BOOT_COMPAT_INTERNAL_H

#include <Uefi.h>
#include <Library/OcAppleBootCompatLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMemoryLib.h>
#include <Protocol/LoadedImage.h>

#ifdef MDE_CPU_X64
#include "X64/ContextSwitch.h"
#else
#error "Unsupported architecture!"
#endif

/**
  Maximum number of supported runtime reloc protection areas.
  Currently hardocded for simplicity.
**/
#define RT_RELOC_PROTECT_MAX_NUM ((UINTN) 64)

/**
  Runtime descriptor number to virtualise.
  Currently hardocded for simplicity.
**/
#define RT_DESC_ENTRY_NUM        ((UINTN) 64)

/**
  Kernel __HIB segment virtual address.
**/
#define KERNEL_HIB_VADDR       ((UINTN)0xFFFFFF8000100000ULL)

/**
  Kernel __TEXT segment virtual address.
**/
#define KERNEL_TEXT_VADDR      ((UINTN)0xFFFFFF8000200000ULL)
/**
  Slide offset per slide entry
**/
#define SLIDE_GRANULARITY      ((UINTN)0x200000)

/**
  Total possible number of KASLR slide offsets.
**/
#define TOTAL_SLIDE_NUM        256

/**
  Get last descriptor address.
  It is assumed that the descriptor contains pages.
**/
#define LAST_DESCRIPTOR_ADDR(Desc) \
  ((Desc)->PhysicalStart + (EFI_PAGES_TO_SIZE ((UINTN) (Desc)->NumberOfPages) - 1))

/**
  Check if area is within the specified descriptor.
  It is assumed that the descriptor contains pages and AreaSize is not 0.
**/
#define AREA_WITHIN_DESCRIPTOR(Desc, Area, AreaSize) \
  ((Area) >= (Desc)->PhysicalStart && ((Area) + ((AreaSize) - 1)) <= LAST_DESCRIPTOR_ADDR (Desc))

/**
  Preserved relocation entry.
**/
typedef struct RT_RELOC_PROTECT_INFO_ {
  ///
  /// Physical address of descriptor start.
  ///
  EFI_PHYSICAL_ADDRESS   PhysicalStart;
  ///
  /// Physical address of descriptor end.
  ///
  EFI_PHYSICAL_ADDRESS   PhysicalEnd;
  ///
  /// Descriptor original memory type.
  ///
  EFI_MEMORY_TYPE        Type;
} RT_RELOC_PROTECT_INFO;

/**
  Preserved relocation entry list.
**/
typedef struct RT_RELOC_PROTECT_DATA_ {
  ///
  /// Number of currently used methods in the table.
  ///
  UINTN                  NumEntries;
  ///
  /// Reloc entries fitted.
  ///
  RT_RELOC_PROTECT_INFO  RelocInfo[RT_RELOC_PROTECT_MAX_NUM];
} RT_RELOC_PROTECT_DATA;

/**
  UEFI Boot & Runtime Services original pointers.
**/
typedef struct UEFI_SERVICES_POINTERS_ {
  ///
  /// Original page allocator. We override it to obtain
  /// the location macOS kernel and hibernation images.
  ///
  EFI_ALLOCATE_PAGES          AllocatePages;
  ///
  /// Original memory map function. We override it to make
  /// memory map shrinking and CSM region protection.
  ///
  EFI_GET_MEMORY_MAP          GetMemoryMap;
  ///
  /// Original exit boot services function. We override it
  /// to ensure we always succeed exiting boot services.
  ///
  EFI_EXIT_BOOT_SERVICES      ExitBootServices;
  ///
  /// Image starting routine. We override to catch boot.efi
  /// loading and enable the rest of functions.
  ///
  EFI_IMAGE_START             StartImage;
  ///
  /// Original get variable function. We override it to alter
  /// boot.efi boot arguments for custom KASLR slide.
  ///
  EFI_GET_VARIABLE            GetVariable;
  ///
  /// Original virtual address mapping function. We override
  /// it to perform runtime area protection to prevent boot.efi
  /// defragmentation and setup virtual memory for firmwares
  /// accessing it after exit boot services.
  ///
  EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
} UEFI_SERVICES_POINTERS;

/**
  UEFI services override internal state.
**/
typedef struct SERVICES_OVERRIDE_STATE_ {
  ///
  /// GetVariable arrival event.
  ///
  EFI_EVENT             GetVariableEvent;
  ///
  /// Minimum address allocated by AlocatePages.
  ///
  EFI_PHYSICAL_ADDRESS  MinAllocatedAddr;
  ///
  /// Maximum address allocated by AlocatePages.
  ///
  EFI_PHYSICAL_ADDRESS  MaxAllocatedAddr;
  ///
  /// Apple hibernate image address allocated by AlocatePages.
  ///
  EFI_PHYSICAL_ADDRESS  HibernateImageAddress;
  ///
  /// Last descriptor size obtained from GetMemoryMap.
  ///
  UINTN                 MemoryMapDescriptorSize;
  ///
  /// Amount of nested boot.efi detected.
  ///
  UINTN                 AppleBootNestedCount;
  ///
  /// TRUE if we are doing boot.efi hibernate wake.
  ///
  BOOLEAN               AppleHibernateWake;
  ///
  /// TRUE if we are using custom KASLR slide (via boot arg).
  ///
  BOOLEAN               AppleCustomSlide;
} SERVICES_OVERRIDE_STATE;

/**
  Apple kernel support internal state..
**/
typedef struct KERNEL_SUPPORT_STATE_ {
  ///
  /// Assembly support internal state.
  ///
  ASM_SUPPORT_STATE        AsmState;
  ///
  /// Kernel jump trampoline.
  ///
  ASM_KERNEL_JUMP          KernelJump;
  ///
  /// Original kernel memory.
  ///
  UINT8                    KernelOrg[sizeof (ASM_KERNEL_JUMP)];
  ///
  /// Custom kernel UEFI System Table.
  ///
  EFI_PHYSICAL_ADDRESS     SysTableRtArea;
  ///
  /// Custom kernel UEFI System Table size in bytes.
  ///
  UINTN                    SysTableRtAreaSize;
  ///
  /// Virtual memory mapper context.
  ///
  OC_VMEM_CONTEXT          VmContext;
  ///
  /// Virtual memory map containing partial memory map with runtime areas only.
  /// Actual number of entries may be less than RT_DESC_ENTRY_NUM due to DescriptorSize
  /// being potentially bigger than sizeof (EFI_MEMORY_DESCRIPTOR).
  ///
  EFI_MEMORY_DESCRIPTOR    VmMap[RT_DESC_ENTRY_NUM];
  ///
  /// Virtual memory map size in bytes.
  ///
  UINTN                    VmMapSize;
  ///
  /// Virtual memory map descriptor size in bytes.
  ///
  UINTN                    VmMapDescSize;
} KERNEL_SUPPORT_STATE;

/**
  Apple Boot Compatibility context.
**/
typedef struct BOOT_COMPAT_CONTEXT_ {
  ///
  /// Apple Coot Compatibility settings.
  ///
  OC_ABC_SETTINGS          Settings;
  ///
  /// Runtime relocations.
  ///
  RT_RELOC_PROTECT_DATA    RtReloc;
  ///
  /// UEFI Boot & Runtime Services original pointers.
  ///
  UEFI_SERVICES_POINTERS   ServicePtrs;
  ///
  /// UEFI services override internal state.
  ///
  SERVICES_OVERRIDE_STATE  ServiceState;
  ///
  /// Apple kernel support internal state.
  ///
  KERNEL_SUPPORT_STATE     KernelState;
} BOOT_COMPAT_CONTEXT;

/**
  Obtain Apple Boot Compatibility context. This function must only
  be called from wrapped services, where passing context arguments
  is not possible.

  @retval  Apple Boot Compatibility context (not null).
**/
BOOT_COMPAT_CONTEXT *
GetBootCompatContext (
  VOID
  );

/**
  Install UEFI services overrides as necessary.

  @param[in,out]  BootCompat    Boot compatibility context.
**/
VOID
InstallServiceOverrides (
  IN OUT BOOT_COMPAT_CONTEXT  *ServicePtrs
  );

/**
  Prepare virtual memory management environment for later usage.

  @param[in,out]  BootCompat    Boot compatibility context.
**/
VOID
AppleMapPrepareMemoryPool (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat
  );

/**
  Prepare environment for Apple UEFI bootloader. See more details inside.

  @param[in,out]  BootCompat    Boot compatibility context.
  @param[in,out]  LoadedImage   UEFI loaded image protocol instance.
  @param[in]      GetMemoryMap  Unmodified GetMemoryMap pointer, optional.
**/
VOID
AppleMapPrepareBooterState (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN OUT EFI_LOADED_IMAGE      *LoadedImage,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap  OPTIONAL
  );

/**
  Save UEFI environment state in implementation specific way.

  @param[in,out]  AsmState      Assembly state to update, can be preserved.
  @param[out]     KernelJump    Kernel jump trampoline to fill.
**/
VOID
AppleMapPlatformSaveState (
  IN OUT ASM_SUPPORT_STATE  *AsmState,
     OUT ASM_KERNEL_JUMP    *KernelJump
  );

/**
  Patch kernel entry point with KernelJump to later land in AppleMapPrepareKernelState.

  @param[in,out]  BootCompat          Boot compatibility context.
  @param[in]      ImageAddress        Kernel or hibernation image address.
  @param[in]      AppleHibernateWake  TRUE when ImageAddress points to hibernation image.
**/
VOID
AppleMapPrepareKernelJump (
  IN OUT BOOT_COMPAT_CONTEXT    *BootCompat,
  IN     UINTN                  ImageAddress,
  IN     BOOLEAN                AppleHibernateWake
  );

/**
  Patch kernel entry point with KernelJump to later land in AppleMapPrepareKernelState.

  @param[in,out]  BootCompat          Boot compatibility context.
  @param[in]      ImageAddress        Kernel or hibernation image address.
  @param[in]      AppleHibernateWake  TRUE when ImageAddress points to hibernation image.
**/
EFI_STATUS
AppleMapPrepareVmState (
  IN OUT BOOT_COMPAT_CONTEXT    *BootCompat,
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN     UINT32                 DescriptorVersion,
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryMap
  );

/**
  Prepare environment for Apple kernel bootloader in boot or wake cases.
  This callback arrives when boot.efi jumps to kernel.

  @param[in]  Args     Case-specific kernel argument handle.
  @param[in]  ModeX64  Debug flag about kernel context type, TRUE when X64.

  @retval  Args must be returned with the necessary modifications if any.
**/
UINTN
EFIAPI
AppleMapPrepareKernelState (
  IN UINTN    Args,
  IN BOOLEAN  ModeX64
  );

#endif // BOOT_COMPAT_INTERNAL_H
