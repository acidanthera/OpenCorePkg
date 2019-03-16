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

#include <Library/OcMachoLib.h>
#include <Library/OcXmlLib.h>
#include <Protocol/SimpleFileSystem.h>

#define PRELINKED_ALIGN(x) ALIGN_VALUE((x), 4096)

#define PRELINK_INFO_SEGMENT "__PRELINK_INFO"
#define PRELINK_INFO_SECTION "__info"

#define PRELINK_INFO_DICTIONARY_KEY "_PrelinkInfoDictionary"
#define PRELINK_INFO_KMOD_INFO_KEY                "_PrelinkKmodInfo"
#define PRELINK_INFO_BUNDLE_PATH_KEY              "_PrelinkBundlePath"
#define PRELINK_INFO_EXECUTABLE_RELATIVE_PATH_KEY "_PrelinkExecutableRelativePath"
#define PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY     "_PrelinkExecutableLoadAddr"
#define PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY   "_PrelinkExecutableSourceAddr"
#define PRELINK_INFO_EXECUTABLE_SIZE_KEY          "_PrelinkExecutableSize"

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
  // Mach-O context for prelinkedkernel.
  //
  OC_MACHO_CONTEXT         PrelinkedMachContext;
  //
  // Mach-O header for prelinkedkernel.
  //
  MACH_HEADER_64           *PrelinkedMachHeader;
  //
  // Pointer to PRELINK_INFO_SEGMENT.
  //
  MACH_SEGMENT_COMMAND_64  *PlistInfoSegment;
  //
  // Pointer to PRELINK_INFO_SECTION.
  //
  MACH_SECTION_64          *PlistInfoSection;
  //
  // Copy of prelinkedkernel PRELINK_INFO_SECTION used for XML_DOCUMENT.
  // Freed upon context destruction.
  //
  CHAR8                    *PlistInfo;
  //
  // Parsed instance of PlistInfo. New entries are added here.
  //
  XML_DOCUMENT             *PlistInfoDocument;
  //
  // Reference for PRELINK_INFO_DICTIONARY_KEY in PlistDocument.
  // This reference is used for quick path during kext injection.
  //
  XML_NODE                 *KextList;
} PRELINKED_CONTEXT;

/**
  Read Apple kernel for target architecture (possibly decompressing)
  into pool allocated buffer.

  @param[in]      File           File handle instance.
  @param[in, out] Kernel         Resulting non-fat kernel buffer from pool.
  @param[out]     KernelSize     Actual kernel size.
  @param[out]     AllocatedSize  Allocated kernel size (AllocatedSize >= KernelSize).
  @param[in]      ReservedSize   Allocated extra size for added kernel extensions.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
ReadAppleKernel (
  IN     EFI_FILE_PROTOCOL  *File,
  IN OUT UINT8              **Kernel,
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
  Drop current plist entry, required for kext injection.

  @param[in,out] Context  Prelinked context.
**/
VOID
PrelinkedDropPlistInfo (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Insert current plist entry after kext injection.

  @param[in,out] Context  Prelinked context.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedInsertPlistInfo (
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Perform kext injection.

  @param[in,out] Context         Prelinked context.
  @param[in]     BundlePath      Kext bundle path (e.g. /L/E/mykext.kext).
  @param[in,out] InfoPlist       Kext Info.plist, overwritten.
  @param[in]     InfoPlistSize   Kext Info.plist size.
  @param[out]    NewInfoPlist    Injected NewInfoPlist, must be freed after PrelinkedInsertPlistInfo.
  @param[in,out] ExecutablePath  Kext executable path (e.g. Contents/MacOS/mykext), optional.
  @param[in,out] Executable      Kext executable, overwritten, optional.
  @param[in]     ExecutableSize  Kext executable, optional.

  @return  EFI_SUCCESS on success.
**/
EFI_STATUS
PrelinkedInjectKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     CONST CHAR8        *BundlePath,
  IN OUT CHAR8              *InfoPlist,
  IN     UINT32             InfoPlistSize,
     OUT CHAR8              **NewInfoPlist,
  IN     CONST CHAR8        *ExecutablePath OPTIONAL,
  IN OUT UINT8              *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL
  );

#endif // OC_APPLE_KERNEL_LIB_H

