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

#include <IndustryStandard/AppleMkext.h>
#include <Library/OcCpuLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcXmlLib.h>
#include <Protocol/SimpleFileSystem.h>

#define PRELINK_KERNEL_IDENTIFIER "__kernel__"
#define PRELINK_KPI_IDENTIFIER_PREFIX "com.apple.kpi."

#define PRELINK_INFO_SEGMENT  "__PRELINK_INFO"
#define PRELINK_INFO_SECTION  "__info"
#define PRELINK_TEXT_SEGMENT  "__PRELINK_TEXT"
#define PRELINK_TEXT_SECTION  "__text"
#define PRELINK_STATE_SEGMENT "__PRELINK_STATE"
#define PRELINK_STATE_SECTION_KERNEL "__kernel"
#define PRELINK_STATE_SECTION_KEXTS  "__kexts"


#define PRELINK_INFO_DICTIONARY_KEY               "_PrelinkInfoDictionary"
#define PRELINK_INFO_KMOD_INFO_KEY                "_PrelinkKmodInfo"
#define PRELINK_INFO_BUNDLE_PATH_KEY              "_PrelinkBundlePath"
#define PRELINK_INFO_EXECUTABLE_RELATIVE_PATH_KEY "_PrelinkExecutableRelativePath"
#define PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY     "_PrelinkExecutableLoadAddr"
#define PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY   "_PrelinkExecutableSourceAddr"
#define PRELINK_INFO_EXECUTABLE_SIZE_KEY          "_PrelinkExecutableSize"
#define PRELINK_INFO_LINK_STATE_ADDR_KEY          "_PrelinkLinkState"
#define PRELINK_INFO_LINK_STATE_SIZE_KEY          "_PrelinkLinkStateSize"

#define INFO_BUNDLE_IDENTIFIER_KEY                "CFBundleIdentifier"
#define INFO_BUNDLE_EXECUTABLE_KEY                "CFBundleExecutable"
#define INFO_BUNDLE_LIBRARIES_KEY                 "OSBundleLibraries"
#define INFO_BUNDLE_LIBRARIES_64_KEY              "OSBundleLibraries_x86_64"
#define INFO_BUNDLE_VERSION_KEY                   "CFBundleVersion"
#define INFO_BUNDLE_COMPATIBLE_VERSION_KEY        "OSBundleCompatibleVersion"
#define INFO_BUNDLE_OS_BUNDLE_REQUIRED_KEY        "OSBundleRequired"

#define OS_BUNDLE_REQUIRED_ROOT                   "Root"
#define OS_BUNDLE_REQUIRED_SAFE_BOOT              "Safe Boot"


#define MKEXT_INFO_DICTIONARIES_KEY               "_MKEXTInfoDictionaries"
#define MKEXT_BUNDLE_PATH_KEY                     "_MKEXTBundlePath"
#define MKEXT_EXECUTABLE_RELATIVE_PATH_KEY        "_MKEXTExecutableRelativePath"
#define MKEXT_EXECUTABLE_KEY                      "_MKEXTExecutable"


#define PRELINK_INFO_INTEGER_ATTRIBUTES           "size=\"64\""
#define MKEXT_INFO_INTEGER_ATTRIBUTES             "size=\"32\""

#define KC_REGION_SEGMENT_PREFIX                  "__REGION"
#define KC_REGION0_SEGMENT                        "__REGION0"
#define KC_TEXT_SEGMENT                           "__TEXT"
#define KC_LINKEDIT_SEGMENT                       "__LINKEDIT"
#define KC_MOSCOW_SEGMENT                         "__MOSCOW101"

//
// As PageCount is UINT16, we can only index 2^16 * 4096 Bytes with one chain.
//
#define PRELINKED_KEXTS_MAX_SIZE (BIT16 * MACHO_PAGE_SIZE)

//
// Failsafe default for plist reserve allocation.
//
#define PRELINK_INFO_RESERVE_SIZE (5U * 1024U * 1024U)

//
// Size to reserve per kext for plist expansion.
// Additional properties are added to prelinked and mkext v2, this should account for those.
//
#define PLIST_EXPANSION_SIZE      512

//
// Make integral kernel version (major, minor, revision).
//
#define KERNEL_VERSION(A, B, C) ((A) * 10000 + (B) * 100 + (C))

//
// Minimum kernel versions for each release.
//
#define KERNEL_VERSION_TIGER_MIN            KERNEL_VERSION (8, 0, 0)
#define KERNEL_VERSION_LEOPARD_MIN          KERNEL_VERSION (9, 0, 0)
#define KERNEL_VERSION_SNOW_LEOPARD_MIN     KERNEL_VERSION (10, 0, 0)
#define KERNEL_VERSION_LION_MIN             KERNEL_VERSION (11, 0, 0)
#define KERNEL_VERSION_MOUNTAIN_LION_MIN    KERNEL_VERSION (12, 0, 0)
#define KERNEL_VERSION_MAVERICKS_MIN        KERNEL_VERSION (13, 0, 0)
#define KERNEL_VERSION_YOSEMITE_MIN         KERNEL_VERSION (14, 0, 0)
#define KERNEL_VERSION_EL_CAPITAN_MIN       KERNEL_VERSION (15, 0, 0)
#define KERNEL_VERSION_SIERRA_MIN           KERNEL_VERSION (16, 0, 0)
#define KERNEL_VERSION_HIGH_SIERRA_MIN      KERNEL_VERSION (17, 0, 0)
#define KERNEL_VERSION_MOJAVE_MIN           KERNEL_VERSION (18, 0, 0)
#define KERNEL_VERSION_CATALINA_MIN         KERNEL_VERSION (19, 0, 0)
#define KERNEL_VERSION_BIG_SUR_MIN          KERNEL_VERSION (20, 0, 0)

//
// Maximum kernel versions for each release.
//
#define KERNEL_VERSION_TIGER_MAX            (KERNEL_VERSION_LEOPARD_MIN - 1)
#define KERNEL_VERSION_LEOPARD_MAX          (KERNEL_VERSION_SNOW_LEOPARD_MIN - 1)
#define KERNEL_VERSION_SNOW_LEOPARD_MAX     (KERNEL_VERSION_LION_MIN - 1)
#define KERNEL_VERSION_LION_MAX             (KERNEL_VERSION_MOUNTAIN_LION_MIN - 1)
#define KERNEL_VERSION_MOUNTAIN_LION_MAX    (KERNEL_VERSION_MAVERICKS_MIN - 1)
#define KERNEL_VERSION_MAVERICKS_MAX        (KERNEL_VERSION_YOSEMITE_MIN - 1)
#define KERNEL_VERSION_YOSEMITE_MAX         (KERNEL_VERSION_EL_CAPITAN_MIN - 1)
#define KERNEL_VERSION_EL_CAPITAN_MAX       (KERNEL_VERSION_SIERRA_MIN - 1)
#define KERNEL_VERSION_SIERRA_MAX           (KERNEL_VERSION_HIGH_SIERRA_MIN - 1)
#define KERNEL_VERSION_HIGH_SIERRA_MAX      (KERNEL_VERSION_MOJAVE_MIN - 1)
#define KERNEL_VERSION_MOJAVE_MAX           (KERNEL_VERSION_CATALINA_MIN - 1)
#define KERNEL_VERSION_CATALINA_MAX         (KERNEL_VERSION_BIG_SUR_MIN - 1)

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
  // Pointer to PRELINK_STATE_SEGMENT (for 10.6.8).
  //
  MACH_SEGMENT_COMMAND_64  *PrelinkedStateSegment;
  //
  // Pointer to PRELINK_STATE_SECTION_KERNEL (for 10.6.8).
  //
  MACH_SECTION_64          *PrelinkedStateSectionKernel;
  //
  // Pointer to PRELINK_STATE_SECTION_KEXTS (for 10.6.8).
  //
  MACH_SECTION_64          *PrelinkedStateSectionKexts;
  //
  // Contents of PRELINK_STATE_SECTION_KERNEL section (for 10.6.8).
  //
  VOID                     *PrelinkedStateKernel;
  //
  // Contents of PRELINK_STATE_SECTION_KEXTS (for 10.6.8).
  //
  VOID                     *PrelinkedStateKexts;
  //
  // PRELINK_STATE_SECTION_KEXTS original load address.
  //
  UINT64                   PrelinkedStateKextsAddress;
  //
  // PRELINK_STATE_SECTION_KERNEL section size (for 10.6.8).
  //
  UINT32                   PrelinkedStateKernelSize;
  //
  // PRELINK_STATE_SECTION_KEXTS section size (for 10.6.8).
  //
  UINT32                   PrelinkedStateKextsSize;
  //
  // Pointer to KC_LINKEDIT_SEGMENT (for KC mode).
  //
  MACH_SEGMENT_COMMAND_64  *LinkEditSegment;
  //
  // Pointer to KC_REGION0_SEGMENT (for KC mode).
  //
  MACH_SEGMENT_COMMAND_64  *RegionSegment;
  //
  // Mach-O context for inner prelinkedkernel (for KC mode).
  //
  OC_MACHO_CONTEXT         InnerMachContext;
  //
  // Pointer to PRELINK_INFO_SEGMENT in the inner prelinkedkernel.
  //
  MACH_SEGMENT_COMMAND_64  *InnerInfoSegment;
  //
  // Pointer to PRELINK_INFO_SECTION in the inner prelinkedkernel.
  //
  MACH_SECTION_64          *InnerInfoSection;
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
  // Plist scratch buffer used when updating values.
  //
  CHAR8                    *KextScratchBuffer;
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
  // Used for caching all prelinked kexts.
  // I.e. this contains kernel, injected kexts, and kexts used as dependencies.
  //
  LIST_ENTRY               PrelinkedKexts;
  //
  // Used for caching prelinked kexts, which we inject.
  // This is a sublist of PrelinkedKexts.
  //
  LIST_ENTRY               InjectedKexts;
  //
  // Whether this kernel is a kernel collection (used by macOS 11.0+).
  //
  BOOLEAN                  IsKernelCollection;
  //
  // Kext segment file location for kernel collection.
  //
  UINT32                   KextsFileOffset;
  //
  // Kext segment memory location for kernel collection.
  //
  UINT64                   KextsVmAddress;
  //
  // Kext fixup chain for kernel collection.
  //
  MACH_DYLD_CHAINED_STARTS_IN_SEGMENT  *KextsFixupChains;
  //
  // Kernel virtual base.
  //
  UINT64                   VirtualBase;
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
  //
  // Pointer to KXLD state (read only, it is allocated in PrelinkedStateKexts).
  //
  CONST VOID               *KxldState;
  //
  // Pointer to KXLD state (read only, it is allocated in PrelinkedStateKexts).
  //
  UINT32                   KxldStateSize;
} PATCHER_CONTEXT;

//
// Kernel and kext patch description.
//
typedef struct {
  //
  // Comment or NULL.
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

//
// Context for cacheless boot (S/L/E).
//
typedef struct {
  //
  // Extensions directory EFI_FILE_PROTOCOL instance.
  //
  EFI_FILE_PROTOCOL     *ExtensionsDir;
  //
  // Extensions directory filename. This is freed by the caller.
  //
  CONST CHAR16          *ExtensionsDirFileName;
  //
  // List of injected kexts.
  //
  LIST_ENTRY            InjectedKexts;
  //
  // List of dependencies that need to be injected.
  //
  LIST_ENTRY            InjectedDependencies;
  //
  // List of kext patches for built-in shipping kexts.
  //
  LIST_ENTRY            PatchedKexts;
  //
  // List of built-in shipping kexts.
  //
  LIST_ENTRY            BuiltInKexts;
  //
  // Current kernel version.
  //
  UINT32                KernelVersion;
  //
  // Flag to indicate if above list is valid. List is built during the first read from SLE.
  //
  BOOLEAN               BuiltInKextsValid;
} CACHELESS_CONTEXT;

//
// Mkext context.
//
typedef struct {
  //
  // Current version of mkext. It takes a reference of user-allocated
  // memory block from pool, and grows if needed.
  //
  UINT8                    *Mkext;
  //
  // Exportable mkext size, i.e. the payload size. Also references user field.
  //
  UINT32                   MkextSize;
  //
  // Currently allocated mkext size, used for reduced rellocations.
  //
  UINT32                   MkextAllocSize;
  //
  // Mkext header.
  //
  MKEXT_HEADER_ANY         *MkextHeader;
  //
  // Version.
  //
  UINT32                    MkextVersion;
  //
  // CPU type.
  //
  BOOLEAN                   Is32Bit;
  //
  // Current number of kexts.
  //
  UINT32                    NumKexts;
  //
  // Max kexts for allocation.
  //
  UINT32                    NumMaxKexts;
  //
  // Offset of mkext plist.
  //
  UINT32                    MkextInfoOffset;
  //
  // Copy of mkext plist used for XML_DOCUMENT.
  // Freed upon context destruction.
  //
  UINT8                    *MkextInfo;
  //
  // Parsed instance of mkext plist. New entries are added here.
  //
  XML_DOCUMENT             *MkextInfoDocument;
  //
  // Array of kexts.
  //
  XML_NODE                 *MkextKexts;
  //
  // List of cached kexts, used for patching and blocking.
  //
  LIST_ENTRY               CachedKexts;
} MKEXT_CONTEXT;

//
// Kernel quirk names.
//
typedef enum {
  //
  // Apply MSR E2 patches to AppleIntelCPUPowerManagement kext.
  //
  KernelQuirkAppleCpuPmCfgLock,
  //
  // Apply MSR E2 patches to XNU kernel (XCPM).
  //
  KernelQuirkAppleXcpmCfgLock,
  //
  // Apply extra MSR patches to XNU kernel (XCPM).
  //
  KernelQuirkAppleXcpmExtraMsrs,
  //
  // Apply max MSR_IA32_PERF_CONTROL patches to XNU kernel (XCPM).
  //
  KernelQuirkAppleXcpmForceBoost,
  //
  // Apply custom AppleSMBIOS kext GUID patch for Custom UpdateSMBIOSMode.
  //
  KernelQuirkCustomSmbiosGuid1,
  KernelQuirkCustomSmbiosGuid2,
  //
  // Apply VT-d disabling patches to IOPCIFamily kext to disable IOMapper in macOS.
  //
  KernelQuirkDisableIoMapper,
  //
  // Disable AppleRTC checksum writing.
  //
  KernelQuirkDisableRtcChecksum,
  //
  // Apply dummy power management patches to AppleIntelCpuPowerManagement in macOS.
  //
  KernelQuirkDummyPowerManagement,
  //
  // Apply icon type patches to IOAHCIPort kext to force internal disk icons.
  //
  KernelQuirkExternalDiskIcons,
  //
  // Apply PCI bar size patches to IOPCIFamily kext for compatibility with select configuration.
  //
  KernelQuirkIncreasePciBarSize,
  //
  // Disable LAPIC interrupt kernel panic on AP cores.
  //
  KernelQuirkLapicKernelPanic,
  //
  // Apply kernel patches to remove kext dumping in the panic log.
  //
  KernelQuirkPanicNoKextDump,
  //
  // Disable power state change timeout kernel panic (10.15+).
  //
  KernelQuirkPowerTimeoutKernelPanic,
  //
  // Apply vendor patches to IOAHCIFamily kext to enable native features for third-party drives,
  //   such as TRIM on SSDs or hibernation support on 10.15.
  //
  KernelQuirkThirdPartyDrives,
  //
  // Apply port limit patches to AppleUSBXHCI and AppleUSBXHCIPCI kexts.
  //
  KernelQuirkXhciPortLimit1,
  KernelQuirkXhciPortLimit2,
  KernelQuirkXhciPortLimit3,

  KernelQuirkMax
} KERNEL_QUIRK_NAME;

//
// Kernel quirk patch function.
//
typedef
EFI_STATUS
(KERNEL_QUIRK_PATCH_FUNCTION)(
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  );

//
// Kernel quirk.
//
typedef struct {
  //
  // Target bundle ID. NULL for kernel.
  //
  CONST CHAR8                   *BundleId;
  //
  // Quirk patch function.
  //
  KERNEL_QUIRK_PATCH_FUNCTION   *PatchFunction;
} KERNEL_QUIRK;

/**
  Applies the specified quirk.

  @param[in]     Name           KERNEL_QUIRK_NAME specifying the quirk name.
  @param[in,out] Patcher        PATCHER_CONTEXT instance.
  @param[in]     KernelVersion  Current kernel version.

  @returns EFI_SUCCESS on success.
**/
EFI_STATUS
KernelApplyQuirk (
  IN     KERNEL_QUIRK_NAME  Name,
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  );

/**
  Read Apple kernel for target architecture (possibly decompressing)
  into pool allocated buffer. If CpuType does not exist in fat
  mkext, an error is returned.

  @param[in]      File           File handle instance.
  @param[in]      Prefer32Bit    Prefer 32-bit in case of fat binary.
  @param[out]     Is32Bit        Whether resulting kernel is 32-bit or not.
  @param[in,out]  Kernel         Resulting non-fat kernel buffer from pool.
  @param[out]     KernelSize     Actual kernel size.
  @param[out]     AllocatedSize  Allocated kernel size (AllocatedSize >= KernelSize).
  @param[in]      ReservedSize   Allocated extra size for added kernel extensions.
  @param[out]     Digest         SHA-384 digest for the original kernel, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
ReadAppleKernel (
  IN     EFI_FILE_PROTOCOL  *File,
  IN     BOOLEAN            Prefer32Bit,
     OUT BOOLEAN            *Is32Bit,
     OUT UINT8              **Kernel,
     OUT UINT32             *KernelSize,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize,
     OUT UINT8              *Digest  OPTIONAL
  );

/**
  Read mkext for target architecture (possibly decompressing)
  into pool allocated buffer. If CpuType does not exist in fat
  mkext, an error is returned.

  @param[in]     File             File handle instance.
  @param[in]     Prefer32Bit      Prefer 32-bit in case of fat binary.
  @param[in,out] Mkext            Resulting non-fat mkext buffer from pool.
  @param[out]    MkextSize        Actual mkext size.
  @param[out]    AllocatedSize    Allocated mkext size (AllocatedSize >= MkextSize).
  @param[in]     ReservedSize     Allocated extra size for added kernel extensions.
  @param[in]     NumReservedKexts Number of kext slots to reserve in mkext.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
ReadAppleMkext (
  IN     EFI_FILE_PROTOCOL  *File,
  IN     BOOLEAN            Prefer32Bit,
     OUT UINT8              **Mkext,
     OUT UINT32             *MkextSize,
     OUT UINT32             *AllocatedSize,
  IN     UINT32             ReservedSize,
  IN     UINT32             NumReservedKexts
  );

/**
  Create Apple kernel version in integer format.
  See KERNEL_VERSION on how to build it from integers.

  @param[in]  String      Apple kernel version.

  @returns Kernel version or 0.
**/
UINT32
OcParseDarwinVersion (
  IN  CONST CHAR8         *String
  );

/**
  Read Apple kernel version in integer format.

  @param[in]  Kernel      Apple kernel binary.
  @param[in]  KernelSize  Apple kernel size.

  @returns Kernel version or 0.
**/
UINT32
OcKernelReadDarwinVersion (
  IN  CONST UINT8   *Kernel,
  IN  UINT32        KernelSize
  );

/**
  Check whether min <= current <= max is true.
  When current or max are 0 they are considered infinite.

  @param[in]  CurrentVersion  Current kernel version.
  @param[in]  MinVersion      Minimal kernel version.
  @param[in]  MaxVersion      Maximum kernel version.

  @returns TRUE on match.
**/
BOOLEAN
OcMatchDarwinVersion (
  IN  UINT32  CurrentVersion OPTIONAL,
  IN  UINT32  MinVersion     OPTIONAL,
  IN  UINT32  MaxVersion     OPTIONAL
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

  @param[in,out] Context          Prelinked context.
  @param[in]     LinkedExpansion  Extra LINKEDIT size for KC required
                                  to hold DYLD chained fixups.

  @retval  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedInjectPrepare (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     UINT32             LinkedExpansion,
  IN     UINT32             ReservedExeSize
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

  @param[in,out] ReservedInfoSize  Current reserved PLIST size, updated.
  @param[in,out] ReservedExeSize   Current reserved KEXT size, updated.
  @param[in]     InfoPlistSize     Kext Info.plist size.
  @param[in]     Executable        Kext executable, optional.
  @param[in]     ExecutableSize    Kext executable size, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedReserveKextSize (
  IN OUT UINT32       *ReservedInfoSize,
  IN OUT UINT32       *ReservedExeSize,
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
  Apply kext patch to prelinked.

  @param[in,out] Context         Prelinked context.
  @param[in]     BundleId        Kext bundle ID.
  @param[in]     Patch           Patch to apply.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedContextApplyPatch (
  IN OUT PRELINKED_CONTEXT      *Context,
  IN     CONST CHAR8            *BundleId,
  IN     PATCHER_GENERIC_PATCH  *Patch
  );

/**
  Apply kext quirk to prelinked.

  @param[in,out] Context         Prelinked context.
  @param[in]     Quirk           Kext quirk to apply.
  @param[in]     KernelVersion   Current kernel version.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedContextApplyQuirk (
  IN OUT PRELINKED_CONTEXT    *Context,
  IN     KERNEL_QUIRK_NAME    Quirk,
  IN     UINT32               KernelVersion
  );

/**
  Update Mach-O header with new commands.

  @Param[in,out] Context      Prelinked context.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
KcRebuildMachHeader (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Returns the size required to store a segment's fixup chains information.

  @param[in] SegmentSize  The size, in bytes, of the segment to index.

  @retval 0      The segment is too large to index with a single structure.
  @retval other  The size, in bytes, required to store a segment's fixup chain
                 information.
**/
UINT32
KcGetSegmentFixupChainsSize (
  IN UINT32  SegmentSize
  );

/**
  Initialises a structure that stores a segments's fixup chains information.

  @param[out] SegChain      The information structure to initialise.
  @param[in]  SegChainSize  The size, in bytes, available to SegChain.
  @param[in]  VmAddress     The virtual address of the segment to index.
  @param[in]  VmSize        The virtual size of the segment to index.
**/
EFI_STATUS
KcInitKextFixupChains (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     UINT32             SegChainSize,
  IN     UINT32             ReservedSize
  );

/**
  Indexes all relocations of MachContext into the kernel described by Context.

  @param[in,out] Context      Prelinked context.
  @param[in]     MachContext  The context of the Mach-O to index. It must have
                              been prelinked by OcAppleKernelLib. The image
                              must reside in Segment.
**/
VOID
KcKextIndexFixups (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     OC_MACHO_CONTEXT   *MachContext
  );

/**
  Retrieves a KC KEXT's virtual size.

  @param[in] Context        Prelinked context.
  @param[in] SourceAddress  The virtual address within the KC image of the KEXT.

  @retval 0      An error has occured.
  @retval other  The virtual size, in bytes, of the KEXT at SourceAddress.
**/
UINT32
KcGetKextSize (
  IN PRELINKED_CONTEXT  *Context,
  IN UINT64             SourceAddress
  );

/**
  Apply the delta from KC header to the file's offsets.

  @param[in,out] Context  The context of the KEXT to rebase.
  @param[in]     Delta    The offset from KC header the KEXT starts at.

  @retval EFI_SUCCESS  The file has beem rebased successfully.
  @retval other        An error has occured.
**/
EFI_STATUS
KcKextApplyFileDelta (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Delta
  );

/**
  Update address or dyld fixup value to real address.

  @param[in] Value           Value or fixup.
  @param[in] Name            Source name, optional.

  @return real address on sucess.
**/
UINT64
KcFixupValue (
  IN UINT64             Value,
  IN CONST CHAR8        *Name OPTIONAL
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
  Initialize patcher from mkext context for kext patching.

  @param[in,out] Context         Patcher context.
  @param[in,out] Mkext           Mkext context.
  @param[in]     Name            Kext bundle identifier.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PatcherInitContextFromMkext(
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT MKEXT_CONTEXT      *Mkext,
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
  Initializes cacheless context for later modification.
  Must be freed with CachelessContextFree on success.

  @param[in,out] Context             Cacheless context.
  @param[in]     FileName            Extensions directory filename.
  @param[in]     ExtensionsDir       Extensions directory EFI_FILE_PROTOCOL. 
  @param[in]     KernelVersion       Current kernel version.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
CachelessContextInit (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *FileName,
  IN     EFI_FILE_PROTOCOL    *ExtensionsDir,
  IN     UINT32               KernelVersion
  );

/**
  Frees cacheless context.

  @param[in,out] Context             Cacheless context.

  @return  EFI_SUCCESS on success.
**/
VOID
CachelessContextFree (
  IN OUT CACHELESS_CONTEXT    *Context
  );

/**
  Add kext to cacheless context to be injected later on.

  @param[in,out] Context         Cacheless context.
  @param[in]     InfoPlist       Kext Info.plist.
  @param[in]     InfoPlistSize   Kext Info.plist size.
  @param[in]     Executable      Kext executable, optional.
  @param[in]     ExecutableSize  Kext executable size, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
CachelessContextAddKext (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR8          *InfoPlist,
  IN     UINT32               InfoPlistSize,
  IN     CONST UINT8          *Executable OPTIONAL,
  IN     UINT32               ExecutableSize OPTIONAL
  );

/**
  Add patch to cacheless context to be applied later on.

  @param[in,out] Context         Cacheless context.
  @param[in]     BundleId        Kext bundle ID.
  @param[in]     Patch           Patch to apply.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
CachelessContextAddPatch (
  IN OUT CACHELESS_CONTEXT      *Context,
  IN     CONST CHAR8            *BundleId,
  IN     PATCHER_GENERIC_PATCH  *Patch
  );

/**
  Add kernel quirk to cacheless context to be applied later on.

  @param[in,out] Context         Cacheless context.
  @param[in]     Quirk           Quirk to apply.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
CachelessContextAddQuirk (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     KERNEL_QUIRK_NAME    Quirk
  );

/**
  Creates virtual directory overlay EFI_FILE_PROTOCOL from cacheless context.

  @param[in,out] Context             Cacheless context.
  @param[out]    File                The virtual directory instance.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
CachelessContextOverlayExtensionsDir (
  IN OUT CACHELESS_CONTEXT    *Context,
     OUT EFI_FILE_PROTOCOL    **File
  );

/**
  Perform kext injection.

  @param[in,out] Context         Prelinked context.
  @param[in]     FileName        Filename of kext file to be injected.
  @param[out]    VirtualFile     Newly created virtualised EFI_FILE_PROTOCOL instance.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
CachelessContextPerformInject (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *FileName,
     OUT EFI_FILE_PROTOCOL    **VirtualFile
  );

/**
  Apply patches to built-in kexts.

  @param[in,out] Context         Prelinked context.
  @param[in]     FileName        Filename of kext file to be injected.
  @param[in]     File            EFI_FILE_PROTOCOL instance of kext file.
  @param[out]    VirtualFile     Newly created virtualised EFI_FILE_PROTOCOL instance.

  @return  EFI_SUCCESS on success. If no patches are applicable, VirtualFile will be NULL.
**/
EFI_STATUS
CachelessContextHookBuiltin (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *FileName,
  IN     EFI_FILE_PROTOCOL    *File,
     OUT EFI_FILE_PROTOCOL    **VirtualFile
  );

/**
  Decompress mkext buffer while reserving space for injected kexts later on.
  Specifying zero for OutBufferSize will calculate the size of the
  buffer required for the decompressed mkext in OutMkextSize.

  @param[in]     Buffer               Mkext buffer.
  @param[in]     BufferSize           Mkext buffer size.
  @param[in]     NumReservedKexts     Number of kext slots to reserve for injection.
  @param[in,out] OutBuffer            Output buffer. Optional if OutBufferSize is zero.
  @param[in]     OutBufferSize        Total output buffer size. Specify zero to
                                      calculate output buffer size.
  @param[in,out] OutMkextSize         Decompressed Mkext size.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextDecompress (
  IN     CONST UINT8      *Buffer,
  IN     UINT32           BufferSize,
  IN     UINT32           NumReservedKexts,
  IN OUT UINT8            *OutBuffer OPTIONAL,
  IN     UINT32           OutBufferSize OPTIONAL,
  IN OUT UINT32           *OutMkextSize
  );

/**
  Check if passed mkext is of desired CPU arch.

  @param[in] Mkext          Mkext buffer.
  @param[in] MkextSize      Mkext buffer size.
  @param[in] CpuType        Desired CPU arch.

  @return  FALSE if mismatched arch or invalid mkext.
**/
BOOLEAN
MkextCheckCpuType (
  IN UINT8            *Mkext,
  IN UINT32           MkextSize,
  IN MACH_CPU_TYPE    CpuType
  );

/**
  Construct mkext context for later modification.
  Must be freed with MkextContextFree on success.
  Note that MkextAllocSize never changes, and is to be estimated.

  Mkext buffers cannot contain any compression, and should be run
  through MkextDecompress first.

  @param[in,out] Context            Mkext context.
  @param[in,out] Mkext              Decompressed Mkext buffer.
  @param[in]     MkextSize          Decompressed Mkext buffer size.
  @param[in]     MkextAllocSize     Decompressed Mkext buffer allocated size.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextContextInit (
  IN OUT  MKEXT_CONTEXT      *Context,
  IN OUT  UINT8              *Mkext,
  IN      UINT32             MkextSize,
  IN      UINT32             MkextAllocSize
  );

/**
  Free resources consumed by mkext context.

  @param[in,out] Context    Mkext context.
**/
VOID
MkextContextFree (
  IN OUT MKEXT_CONTEXT      *Context
  );

/**
  Updated required mkext reserve size to inject this kext.

  @param[in,out] ReservedInfoSize  Current reserved PLIST size, updated.
  @param[in,out] ReservedExeSize   Current reserved KEXT size, updated.
  @param[in]     InfoPlistSize     Kext Info.plist size.
  @param[in]     Executable        Kext executable, optional.
  @param[in]     ExecutableSize    Kext executable size, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextReserveKextSize (
  IN OUT UINT32       *ReservedInfoSize,
  IN OUT UINT32       *ReservedExeSize,
  IN     UINT32       InfoPlistSize,
  IN     UINT8        *Executable,
  IN     UINT32       ExecutableSize OPTIONAL
  );

/**
  Perform mkext kext injection.

  @param[in,out] Context          Mkext context.
  @param[in]     BundlePath       Kext bundle path (e.g. /L/E/mykext.kext).
  @param[in,out] InfoPlist        Kext Info.plist.
  @param[in]     InfoPlistSize    Kext Info.plist size.
  @param[in,out] Executable       Kext executable, optional.
  @param[in]     ExecutableSize   Kext executable size, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextInjectKext (
  IN OUT MKEXT_CONTEXT      *Context,
  IN     CONST CHAR8        *BundlePath,
  IN     CONST CHAR8        *InfoPlist,
  IN     UINT32             InfoPlistSize,
  IN     UINT8              *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL
  );

/**
  Apply kext patch to mkext.

  @param[in,out] Context         Mkext context.
  @param[in]     BundleId        Kext bundle ID.
  @param[in]     Patch           Patch to apply.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextContextApplyPatch (
  IN OUT MKEXT_CONTEXT          *Context,
  IN     CONST CHAR8            *BundleId,
  IN     PATCHER_GENERIC_PATCH  *Patch
  );

/**
  Apply kext quirk to mkext.

  @param[in,out] Context         Mkext context.
  @param[in]     Quirk           Kext quirk to apply.
  @param[in]     KernelVersion   Current kernel version.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextContextApplyQuirk (
  IN OUT MKEXT_CONTEXT        *Context,
  IN     KERNEL_QUIRK_NAME    Quirk,
  IN     UINT32               KernelVersion
  );

/**
  Refresh plist and checksum after kext
  injection and/or patching.

  @param[in,out] Context    Mkext context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
MkextInjectPatchComplete (
  IN OUT MKEXT_CONTEXT      *Context
  );

#endif // OC_APPLE_KERNEL_LIB_H
