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

#ifndef OC_APPLE_KERNEL_LIB_H
#define OC_APPLE_KERNEL_LIB_H

#include <Library/OcCpuLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcXmlLib.h>
#include <Protocol/SimpleFileSystem.h>

#define PRELINK_KERNEL_IDENTIFIER "__kernel__"
#define PRELINK_KPI_IDENTIFIER_PREFIX "com.apple.kpi."

#define PRELINK_INFO_SEGMENT "__PRELINK_INFO"
#define PRELINK_INFO_SECTION "__info"
#define PRELINK_TEXT_SEGMENT "__PRELINK_TEXT"
#define PRELINK_TEXT_SECTION "__text"

#define PRELINK_INFO_DICTIONARY_KEY               "_PrelinkInfoDictionary"
#define PRELINK_INFO_KMOD_INFO_KEY                "_PrelinkKmodInfo"
#define PRELINK_INFO_BUNDLE_PATH_KEY              "_PrelinkBundlePath"
#define PRELINK_INFO_EXECUTABLE_RELATIVE_PATH_KEY "_PrelinkExecutableRelativePath"
#define PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY     "_PrelinkExecutableLoadAddr"
#define PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY   "_PrelinkExecutableSourceAddr"
#define PRELINK_INFO_EXECUTABLE_SIZE_KEY          "_PrelinkExecutableSize"

#define INFO_BUNDLE_IDENTIFIER_KEY                "CFBundleIdentifier"
#define INFO_BUNDLE_EXECUTABLE_KEY                "CFBundleExecutable"
#define INFO_BUNDLE_LIBRARIES_KEY                 "OSBundleLibraries"
#define INFO_BUNDLE_LIBRARIES_64_KEY              "OSBundleLibraries_x86_64"
#define INFO_BUNDLE_VERSION_KEY                   "CFBundleVersion"
#define INFO_BUNDLE_COMPATIBLE_VERSION_KEY        "OSBundleCompatibleVersion"


#define PRELINK_INFO_INTEGER_ATTRIBUTES           "size=\"64\""

//
// Failsafe default for plist reserve allocation.
//
#define PRELINK_INFO_RESERVE_SIZE (5U * 1024U * 1024U)

//
// Prelinked context used for kernel modification.
//
typedef struct {
  //
  // Current version of prelinkedkernel. It takes a reference of user-allocated
  // memory block from pool, and grows if needed.
  //
  UINT8                    *Prelinked;
  //
  // Exportable prelinkedkernel size, i.e. the payload size. Also references user field.
  //
  UINT32                   PrelinkedSize;
  //
  // Currently allocated prelinkedkernel size, used for reduced rellocations.
  //
  UINT32                   PrelinkedAllocSize;
  //
  // Current last virtual address (kext source files and plist are put here).
  //
  UINT64                   PrelinkedLastAddress;
  //
  // Current last virtual load address (kexts are loaded here after kernel startup).
  //
  UINT64                   PrelinkedLastLoadAddress;
  //
  // Mach-O context for prelinkedkernel.
  //
  OC_MACHO_CONTEXT         PrelinkedMachContext;
  //
  // Pointer to PRELINK_INFO_SEGMENT.
  //
  MACH_SEGMENT_COMMAND_64  *PrelinkedInfoSegment;
  //
  // Pointer to PRELINK_INFO_SECTION.
  //
  MACH_SECTION_64          *PrelinkedInfoSection;
  //
  // Pointer to PRELINK_TEXT_SEGMENT.
  //
  MACH_SEGMENT_COMMAND_64  *PrelinkedTextSegment;
  //
  // Pointer to PRELINK_TEXT_SECTION.
  //
  MACH_SECTION_64          *PrelinkedTextSection;
  //
  // Copy of prelinkedkernel PRELINK_INFO_SECTION used for XML_DOCUMENT.
  // Freed upon context destruction.
  //
  CHAR8                    *PrelinkedInfo;
  //
  // Parsed instance of PlistInfo. New entries are added here.
  //
  XML_DOCUMENT             *PrelinkedInfoDocument;
  //
  // Reference for PRELINK_INFO_DICTIONARY_KEY in PlistDocument.
  // This reference is used for quick path during kext injection.
  //
  XML_NODE                 *KextList;
  //
  // Buffers allocated from pool for internal needs.
  //
  VOID                     **PooledBuffers;
  //
  // Currently used pooled buffers.
  //
  UINT32                   PooledBuffersCount;
  //
  // Currently allocated pooled buffers. PooledBuffersAllocCount >= PooledBuffersCount.
  //
  UINT32                   PooledBuffersAllocCount;
  VOID                     *LinkBuffer;
  UINT32                   LinkBufferSize;
  //
  // Used for caching prelinked kexts.
  //
  LIST_ENTRY               PrelinkedKexts;
} PRELINKED_CONTEXT;

//
// Kernel and kext patching context.
//
typedef struct {
  //
  // Mach-O context for patched binary.
  //
  OC_MACHO_CONTEXT         MachContext;
  //
  // Virtual base to subtract to obtain file offset.
  //
  UINT64                   VirtualBase;
  //
  // Virtual kmod_info_t address.
  //
  UINT64                   VirtualKmod;
} PATCHER_CONTEXT;

//
// Kernel and kext patch description.
//
typedef struct {
  //
  // Comment or NULL (0 base is used then).
  //
  CONST CHAR8  *Comment;
  //
  // Symbol base or NULL (0 base is used then).
  //
  CONST CHAR8  *Base;
  //
  // Find bytes or NULL (data is written to base then).
  //
  CONST UINT8  *Find;
  //
  // Replace bytes.
  //
  CONST UINT8  *Replace;
  //
  // Find mask or NULL.
  //
  CONST UINT8  *Mask;
  //
  // Replace mask or NULL.
  //
  CONST UINT8  *ReplaceMask;
  //
  // Patch size.
  //
  UINT32       Size;
  //
  // Replace count or 0 for all.
  //
  UINT32       Count;
  //
  // Skip count or 0 to start from 1 match.
  //
  UINT32       Skip;
  //
  // Limit replacement size to this value or 0, which assumes table size.
  //
  UINT32       Limit;
} PATCHER_GENERIC_PATCH;

/**
  Read Apple kernel for target architecture (possibly decompressing)
  into pool allocated buffer.

  @param[in]      File           File handle instance.
  @param[in,out]  Kernel         Resulting non-fat kernel buffer from pool.
  @param[out]     KernelSize     Actual kernel size.
  @param[out]     AllocatedSize  Allocated kernel size (AllocatedSize >= KernelSize).
  @param[in]      ReservedSize   Allocated extra size for added kernel extensions.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
ReadAppleKernel (
  IN     EFI_FILE_PROTOCOL  *File,
     OUT UINT8              **Kernel,
     OUT UINT32             *KernelSize,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize
  );

/**
  Construct prelinked context for later modification.
  Must be freed with PrelinkedContextFree on success.
  Note, that PrelinkedAllocSize never changes, and is to be estimated.

  @param[in,out] Context             Prelinked context.
  @param[in,out] Prelinked           Unpacked prelinked buffer (Mach-O image).
  @param[in]     PrelinkedSize       Unpacked prelinked buffer size.
  @param[in]     PrelinkedAllocSize  Unpacked prelinked buffer allocated size.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedContextInit (
  IN OUT  PRELINKED_CONTEXT  *Context,
  IN OUT  UINT8              *Prelinked,
  IN      UINT32             PrelinkedSize,
  IN      UINT32             PrelinkedAllocSize
  );

/**
  Free resources consumed by prelinked context.

  @param[in,out] Context  Prelinked context.
**/
VOID
PrelinkedContextFree (
  IN OUT  PRELINKED_CONTEXT  *Context
  );

/**
  Insert pool-allocated buffer dependency with the same lifetime as
  prelinked context, so it gets freed with PrelinkedContextFree.

  @param[in,out] Context          Prelinked context.
  @param[in]     Buffer           Pool allocated buffer.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedDependencyInsert (
  IN OUT  PRELINKED_CONTEXT  *Context,
  IN      VOID               *Buffer
  );

/**
  Drop current plist entry, required for kext injection.
  Ensure that prelinked text can grow with new kexts.

  @param[in,out] Context  Prelinked context.
**/
EFI_STATUS
PrelinkedInjectPrepare (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Insert current plist entry after kext injection.

  @param[in,out] Context  Prelinked context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedInjectComplete (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Updated required reserve size to inject this kext.

  @param[in,out] ReservedSize    Current reserved size, updated.
  @param[in]     InfoPlistSize   Kext Info.plist size.
  @param[in]     Executable      Kext executable, optional.
  @param[in]     ExecutableSize  Kext executable size, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedReserveKextSize (
  IN OUT UINT32       *ReservedSize,
  IN     UINT32       InfoPlistSize,
  IN     UINT8        *Executable OPTIONAL,
  IN     UINT32       ExecutableSize OPTIONAL
  );

/**
  Perform kext injection.

  @param[in,out] Context         Prelinked context.
  @param[in]     BundlePath      Kext bundle path (e.g. /L/E/mykext.kext).
  @param[in,out] InfoPlist       Kext Info.plist.
  @param[in]     InfoPlistSize   Kext Info.plist size.
  @param[in,out] ExecutablePath  Kext executable path (e.g. Contents/MacOS/mykext), optional.
  @param[in,out] Executable      Kext executable, optional.
  @param[in]     ExecutableSize  Kext executable size, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedInjectKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     CONST CHAR8        *BundlePath,
  IN     CONST CHAR8        *InfoPlist,
  IN     UINT32             InfoPlistSize,
  IN     CONST CHAR8        *ExecutablePath OPTIONAL,
  IN OUT CONST UINT8        *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL
  );

/**
  Initialize patcher from prelinked context for kext patching.

  @param[in,out] Context         Patcher context.
  @param[in,out] Prelinked       Prelinked context.
  @param[in]     Name            Kext bundle identifier.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatcherInitContextFromPrelinked (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Name
  );

/**
  Initialize patcher from buffer for e.g. kernel patching.

  @param[in,out] Context         Patcher context.
  @param[in,out] Buffer          Kernel buffer (could be prelinked).
  @param[in]     BufferSize      Kernel buffer size.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatcherInitContextFromBuffer (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT UINT8              *Buffer,
  IN     UINT32             BufferSize
  );

/**
  Get local symbol address.

  @param[in,out] Context         Patcher context.
  @param[in]     Name            Symbol name.
  @param[in,out] Address         Returned symbol address in file.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatcherGetSymbolAddress (
  IN OUT PATCHER_CONTEXT    *Context,
  IN     CONST CHAR8        *Name,
  IN OUT UINT8              **Address
  );

/**
  Apply generic patch.

  @param[in,out] Context         Patcher context.
  @param[in]     Patch           Patch description.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatcherApplyGenericPatch (
  IN OUT PATCHER_CONTEXT        *Context,
  IN     PATCHER_GENERIC_PATCH  *Patch
  );

/**
  Block kext from loading.

  @param[in,out] Context         Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatcherBlockKext (
  IN OUT PATCHER_CONTEXT        *Context
  );

/**
  Apply MSR E2 patches to AppleIntelCPUPowerManagement kext.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchAppleCpuPmCfgLock (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply MSR E2 patches to XNU kernel (XCPM).

  @param Patcher  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchAppleXcpmCfgLock (
  IN OUT PATCHER_CONTEXT  *Patcher
  );

/**
  Apply extra MSR patches to XNU kernel (XCPM).

  @param Patcher  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchAppleXcpmExtraMsrs (
  IN OUT PATCHER_CONTEXT  *Patcher
  );

/**
  Apply max MSR_IA32_PERF_CONTROL patches to XNU kernel (XCPM).

  @param Patcher  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchAppleXcpmForceBoost (
  IN OUT PATCHER_CONTEXT   *Patcher
  );

/**
  Apply port limit patches to AppleUSBXHCI and AppleUSBXHCIPCI kexts.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchUsbXhciPortLimit (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply vendor patches to IOAHCIFamily kext to enable native features for third-party drives,
  such as TRIM on SSDs or hibernation support on 10.15.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchThirdPartyDriveSupport (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply icon type patches to IOAHCIPort kext to force internal disk icons.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchForceInternalDiskIcons (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply VT-d disabling patches to IOPCIFamily kext to disable IOMapper in macOS.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchAppleIoMapperSupport (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply dummy power management patches to AppleIntelCpuPowerManagement in macOS.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchDummyPowerManagement (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply PCI bar size patches to IOPCIFamily kext for compatibility with select configuration.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchIncreasePciBarSize (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply modification to CPUID 1.

  @param Patcher  Patcher context.
  @param CpuInfo  CPU information.
  @param Data     4 32-bit integers with CPUID data.
  @param DataMask 4 32-bit integers with CPUID enabled overrides data.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchKernelCpuId (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           *Data,
  IN     UINT32           *DataMask
  );

/**
  Apply custom AppleSMBIOS kext GUID patch for Custom UpdateSMBIOSMode.

  @param Context  Prelinked kernel context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchCustomSmbiosGuid (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Apply kernel patches to remove kext dumping in the panic log.

  @param Patcher  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchPanicKextDump (
  IN OUT PATCHER_CONTEXT  *Patcher
  );

/**
  Disable LAPIC interrupt kernel panic on AP cores.

  @param Patcher  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchLapicKernelPanic (
  IN OUT PATCHER_CONTEXT  *Patcher
  );

/**
  Disable power state change timeout kernel panic (10.15+).

  @param Patcher  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchPowerStateTimeout (
  IN OUT PATCHER_CONTEXT   *Patcher
  );

/**
  Disable AppleRTC checksum writing.

  @param Context  Patcher context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatchAppleRtcChecksum (
  IN OUT PRELINKED_CONTEXT  *Context
  );

#endif // OC_APPLE_KERNEL_LIB_H
