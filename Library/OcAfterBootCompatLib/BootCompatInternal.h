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

#include <IndustryStandard/AppleBootArgs.h>

#include <Library/OcAfterBootCompatLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMemoryLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/OcFirmwareRuntime.h>

//
// The kernel is normally allocated at base 0x100000 + slide address.
//
// For slide=0x1~0x7F the kernel is allocated from
// 0x100000 + 0x200000 till 0x100000 + 0xFE00000.
//
// For slide = 0x80~0xFF on Sandy Bridge or Ivy Bridge CPUs from
// 0x100000 + 0x20200000 till 0x100000? + 0x30000000??.
//
// For slide = 0x80~0xFF on Other CPUs from
// 0x100000 + 0x10000000 till 0x100000 + 0x1FE00000.
//

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
  Kernel static vaddr mapping base.
**/
#define KERNEL_STATIC_VADDR      ((UINT64) 0xFFFFFF8000000000ULL)

/**
  Kernel __HIB segment virtual address.
**/
#define KERNEL_HIB_VADDR         ((UINTN) (0xFFFFFF8000100000ULL & MAX_UINTN))

/**
  Kernel __TEXT segment virtual address.
**/
#define KERNEL_TEXT_VADDR        ((UINTN) (0xFFFFFF8000200000ULL & MAX_UINTN))

/**
  Kernel physical base address.
**/
#define KERNEL_BASE_PADDR        ((UINT32) (KERNEL_HIB_VADDR & MAX_UINT32))

/**
  Kernel physical base address.
**/
#define KERNEL_TEXT_PADDR        ((UINT32) (KERNEL_TEXT_VADDR & MAX_UINT32))

/**
  Slide offset per slide entry
**/
#define SLIDE_GRANULARITY        ((UINTN) SIZE_2MB)

/**
  Total possible number of KASLR slide offsets.
**/
#define TOTAL_SLIDE_NUM          ((UINTN) 0x100)

/**
  Slide errate number to skip range from.
**/
#define SLIDE_ERRATA_NUM         ((UINTN) 0x80)

/**
  Sandy/Ivy skip slide range for Intel HD graphics.
**/
#define SLIDE_ERRATA_SKIP_RANGE  ((UINTN) 0x10200000)

/**
  Assume the kernel is roughly 128 MBs.
  And the recovery introduced with Big Sur has roughly 200 MBs.
  See 11.0b10 EB.MM.AKMR function (EfiBoot.MemoryMap.AllocateKernelMemoryRecovery),
  it has 0xC119 pages requested. This value is likely calculated from KC size.
**/
#define ESTIMATED_KERNEL_SIZE    ((UINTN) (200 * SIZE_1MB))

/**
  Assume call gate (normally a little over 100 bytes) can be up to 256 bytes.
  It is allocated in its own page and is relocatable.

  WARNING: Keep this in sync with RelocationCallGate assembly!
**/
#define ESTIMATED_CALL_GATE_SIZE 256

/**
  Size of jump from call gate inserted before Call Gate to jump to our code.
**/
#define CALL_GATE_JUMP_SIZE      (sizeof (CALL_GATE_JUMP))

/**
  Command used to perform an absolute 64-bit jump from Call Gate to our code.
**/
#pragma pack(push,1)
typedef struct CALL_GATE_JUMP_ {
  UINT16  Command;
  UINT32  Argument;
  UINT64  Address;
} CALL_GATE_JUMP;
STATIC_ASSERT (sizeof (CALL_GATE_JUMP) == 14, "Invalid CALL_GATE_JUMP size");
#pragma pack(pop)

/**
  Kernel call gate prototype.
**/
typedef
UINTN
(EFIAPI *KERNEL_CALL_GATE) (
  IN UINTN    Args,
  IN UINTN    EntryPoint
  );

/**
  Relocation call gate prototype.
**/
typedef
UINTN
(EFIAPI *RELOCATION_CALL_GATE) (
  IN UINTN                  QWordCount,
  IN UINTN                  EntryPoint,
  IN EFI_PHYSICAL_ADDRESS   Source,
  IN UINTN                  Args
  );

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
  /// Original page deallocator. We override it to fix memory
  /// attributes table as it is updated after page dealloc.
  ///
  EFI_FREE_PAGES              FreePages;
  ///
  /// Original memory map function. We override it to make
  /// memory map shrinking and CSM region protection.
  ///
  EFI_GET_MEMORY_MAP          GetMemoryMap;
  ///
  /// Original pool allocator. We override it to fix memory
  /// attributes table as it is updated after pool alloc.
  ///
  EFI_ALLOCATE_POOL           AllocatePool;
  ///
  /// Original pool deallocator. We override it to fix memory
  /// attributes table as it is updated after pool dealloc.
  ///
  EFI_FREE_POOL               FreePool;
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
} UEFI_SERVICES_POINTERS;

/**
  UEFI services override internal state.
**/
typedef struct SERVICES_OVERRIDE_STATE_ {
  ///
  /// GetVariable arrival event.
  ///
  EFI_EVENT                     GetVariableEvent;
  ///
  /// Firmware runtime protocol instance.
  ///
  OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime;
  ///
  /// Kernel call gate is an assembly function that takes boot arguments (rcx)
  /// and kernel entry point (rdx) and jumps to the kernel (pstart) in 32-bit mode.
  ///
  /// It is only used for normal booting, so for general interception we do not
  /// use call gate but rather patch the kernel entry point. However, when it comes
  /// to booting with relocation block (it does not support hibernation) we need
  /// to update kernel entry point with the relocation block offset and that can
  /// only be done in the call gate as it will otherwise jump to lower memory.
  ///
  EFI_PHYSICAL_ADDRESS          KernelCallGate;
  ///
  /// Last descriptor size obtained from GetMemoryMap.
  ///
  UINTN                         MemoryMapDescriptorSize;
  ///
  /// Amount of nested boot.efi detected.
  ///
  UINTN                         AppleBootNestedCount;
  ///
  /// TRUE if we are doing boot.efi hibernate wake.
  ///
  BOOLEAN                       AppleHibernateWake;
  ///
  /// TRUE if we are using custom KASLR slide (via boot arg).
  ///
  BOOLEAN                       AppleCustomSlide;
  ///
  /// TRUE if we are done reporting MMIO cleanup.
  ///
  BOOLEAN                       ReportedMmio;
  ///
  /// TRUE if we are waiting for performance memory allocation.
  ///
  BOOLEAN                       AwaitingPerfAlloc;
} SERVICES_OVERRIDE_STATE;

/**
  Apple kernel support internal state..
**/
typedef struct KERNEL_SUPPORT_STATE_ {
  ///
  /// Custom kernel UEFI System Table.
  ///
  EFI_PHYSICAL_ADDRESS     SysTableRtArea;
  ///
  /// Custom kernel UEFI System Table size in bytes.
  ///
  UINTN                    SysTableRtAreaSize;
  ///
  /// Physical configuration table location.
  ///
  EFI_CONFIGURATION_TABLE  *ConfigurationTable;
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
  ///
  /// Relocation block is a scratch buffer allocated in lower 4GB to be used for
  /// loading the kernel and related structures by EfiBoot on firmware where the
  /// lower memory region is otherwise occupied by (assumed) non-runtime data.
  /// Relocation block can be used when:
  /// - no better slide exists (all the memory is used)
  /// - slide=0 is forced (by an argument or safe mode)
  /// - KASLR (slide) is unsupported (macOS 10.7 or older)
  /// Right before kernel startup the relocation block is copied back to lower
  /// addresses. Similarly all the other addresses pointing to relocation block
  /// are also carefully adjusted.
  ///
  EFI_PHYSICAL_ADDRESS     RelocationBlock;
  ///
  /// Real amount of memory used in the relocation block.
  /// This value should match ksize in XNU BootArgs.
  ///
  UINTN                    RelocationBlockUsed;
} KERNEL_SUPPORT_STATE;

/**
  Apple booter KASLR slide support internal state.
**/
typedef struct SLIDE_SUPPORT_STATE_ {
  ///
  /// Memory map analysis status determining slide usage.
  ///
  BOOLEAN                  HasMemoryMapAnalysis;
  ///
  /// TRUE if we are running on Intel Sandy or Ivy bridge.
  ///
  BOOLEAN                  HasSandyOrIvy;
  ///
  /// TRUE if CsrActiveConfig was set.
  ///
  BOOLEAN                  HasCsrActiveConfig;
  ///
  /// TRUE if BootArgs was set.
  ///
  BOOLEAN                  HasBootArgs;
  ///
  /// Read or assumed csr-arctive-config variable value.
  ///
  UINT32                   CsrActiveConfig;
  ///
  /// Max slide value provided.
  ///
  UINT8                    ProvideMaxSlide;
  ///
  /// Valid slides to choose from when using custom slide.
  ///
  UINT8                    ValidSlides[TOTAL_SLIDE_NUM];
  ///
  /// Number of entries in ValidSlides.
  ///
  UINT32                   ValidSlideCount;
  ///
  /// Apple kernel boot arguments read from boot-args variable and then
  /// modified with an additional slide parameter in case custom slide is used.
  ///
  CHAR8                    BootArgs[BOOT_LINE_LENGTH];
  ///
  /// BootArgs data size.
  ///
  UINTN                    BootArgsSize;
  ///
  /// Estimated size for kernel itself, device tree, memory map, and rt pages.
  ///
  UINTN                    EstimatedKernelArea;
} SLIDE_SUPPORT_STATE;

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
  ///
  /// Apple booter KASLR slide support internal state.
  ///
  SLIDE_SUPPORT_STATE      SlideSupport;
  ///
  /// CPU information.
  ///
  OC_CPU_INFO              *CpuInfo;
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
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat
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
  Patch kernel entry point with KernelJump to later land in AppleMapPrepareKernelState.

  @param[in,out]  BootCompat          Boot compatibility context.
  @param[in]      CallGate            Kernel call gate address.
**/
VOID
AppleMapPrepareKernelJump (
  IN OUT BOOT_COMPAT_CONTEXT    *BootCompat,
  IN     EFI_PHYSICAL_ADDRESS   CallGate
  );

/**
  Prepare memory state and perform virtual address translation.

  @param[in,out]  BootCompat          Boot compatibility context.
  @param[in]      MemoryMapSize       SetVirtualAddresses memory map size argument.
  @param[in]      DescriptorSize      SetVirtualAddresses descriptor size argument.
  @param[in]      DescriptorVersion   SetVirtualAddresses descriptor version argument.
  @param[in]      MemoryMap           SetVirtualAddresses memory map argument.
**/
EFI_STATUS
AppleMapPrepareMemState (
  IN OUT BOOT_COMPAT_CONTEXT    *BootCompat,
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN     UINT32                 DescriptorVersion,
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryMap
  );

/**
  Prepare environment for Apple kernel bootloader in boot or wake cases.
  This callback arrives when boot.efi jumps to kernel call gate.
  Should transfer control to kernel call gate + CALL_GATE_JUMP_SIZE
  with the same arguments.

  @param[in]  Args         Case-specific kernel argument handle.
  @param[in]  EntryPoint   Case-specific kernel entry point.

  @returns Case-specific value if any.
**/
UINTN
EFIAPI
AppleMapPrepareKernelState (
  IN UINTN    Args,
  IN UINTN    EntryPoint
  );

/**
  Patch boot.efi to support random and passed slide values in safe mode.

  @param[in,out]  ImageBase  Apple booter image base.
  @param[in]      ImageSize  Apple booter image size.
**/
VOID
AppleSlideUnlockForSafeMode (
  IN OUT UINT8  *ImageBase,
  IN     UINTN  ImageSize
  );

/**
  Primary custom KASLR support handler. This gets called on every
  UEFI RuntimeServices GetVariable call and thus is useful to
  perform KASLR slide injection through boot-args.

  @param[in,out]  BootCompat       Boot compatibility context.
  @param[in]      GetVariable      Original UEFI GetVariable service.
  @param[in]      GetMemoryMap     Unmodified GetMemoryMap pointer, optional.
  @param[in]      FilterMap        GetMemoryMap result filter, optional.
  @param[in]      FilterMapContext FilterMap context, optional.
  @param[in]      VariableName     GetVariable variable name argument.
  @param[in]      VendorGuid       GetVariable vendor GUID argument.
  @param[out]     Attributes       GetVariable attributes argument.
  @param[in,out]  DataSize         GetVariable data size argument.
  @param[out]     Data             GetVariable data argument.

  @retval GetVariable status code.
**/
EFI_STATUS
AppleSlideGetVariable (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN     EFI_GET_VARIABLE      GetVariable,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap  OPTIONAL,
  IN     OC_MEMORY_FILTER      FilterMap     OPTIONAL,
  IN     VOID                  *FilterMapContext  OPTIONAL,
  IN     CHAR16                *VariableName,
  IN     EFI_GUID              *VendorGuid,
     OUT UINT32                *Attributes   OPTIONAL,
  IN OUT UINTN                 *DataSize,
     OUT VOID                  *Data
  );

/**
  Ensures that the original csr-active-config is passed to the kernel,
  and removes customised slide value for security reasons.

  @param[in,out]  BootCompat    Boot compatibility context.
  @param[in,out]  BootArgs      Apple kernel boot arguments.
**/
VOID
AppleSlideRestore (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN OUT OC_BOOT_ARGUMENTS     *BootArgs
  );

/**
  Get calculated relocation block size for booting with slide=0
  (e.g. Safe Mode) or without KASLR (older macOS) when it is
  otherwise impossible.

  @param[in,out]  BootCompat    Boot compatibility context.

  @returns Size of the relocation block (maximum).
  @retval 0 otherwise.
**/
UINTN
AppleSlideGetRelocationSize (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat
  );

/**
  Allocate memory from a relocation block when zero slide is unavailable.
  EfiLoaderData at address.

  @param[in,out]  BootCompat     Boot compatibility context.
  @param[in]      GetMemoryMap   Unmodified GetMemoryMap pointer, optional.
  @param[in]      AllocatePages  Unmodified AllocatePages pointer.
  @param[in]      NumberOfPages  Number of pages to allocate.
  @param[in,out]  Memory         Memory address to allocate, may be updated.

  @retval EFI_SUCCESS on success.
  @retval EFI_UNSUPPORTED when zero slide is available.
**/
EFI_STATUS
AppleRelocationAllocatePages (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap,
  IN     EFI_ALLOCATE_PAGES    AllocatePages,
  IN     UINTN                 NumberOfPages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  );

/**
  Release relocation block if present.

  @param[in,out]  BootCompat     Boot compatibility context.

  @retval EFI_SUCCESS on success.
  @retval EFI_UNSUPPORTED when zero slide is available.
**/
EFI_STATUS
AppleRelocationRelease (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat
  );

/**
  Transitions to virtual memory for the relocation block.

  @param[in,out]  BootCompat    Boot compatibility context.
  @param[in,out]  BootArgs      Apple kernel boot arguments.
**/
EFI_STATUS
AppleRelocationVirtualize (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN OUT OC_BOOT_ARGUMENTS    *BA
  );

/**
  Transition from relocation block address space to normal low
  memory address space in the relevant XNU areas.

  @param[in,out]  BootCompat    Boot compatibility context.
  @param[in,out]  BootArgs      Apple kernel boot arguments.
**/
VOID
AppleRelocationRebase (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN OUT OC_BOOT_ARGUMENTS    *BA
  );

/**
  Boot Apple Kernel through relocation block.

  @param[in,out] BootCompat   Boot compatibility context.
  @param[in]     Args         Case-specific kernel argument handle.
  @param[in]     CallGate     Kernel call gate address.
  @param[in]     EntryPoint   Case-specific kernel entry point.

  @returns Case-specific value if any.
**/
UINTN
AppleRelocationCallGate (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat,
  IN     KERNEL_CALL_GATE     CallGate,
  IN     UINTN                Args,
  IN     UINTN                EntryPoint
  );

#endif // BOOT_COMPAT_INTERNAL_H
