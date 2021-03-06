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

#ifndef OC_AFTER_BOOT_COMPAT_LIB_H
#define OC_AFTER_BOOT_COMPAT_LIB_H

#include <Library/OcCryptoLib.h>
#include <Library/OcCpuLib.h>

/**
  Booter patch structure.
**/
typedef struct OC_BOOTER_PATCH_ {
  //
  // Comment from user.
  //
  CHAR8        *Comment;
  //
  // Target architecture.
  //
  CONST CHAR8  *Arch;
  //
  // Find bytes.
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
  // Limit replacement size to this value or 0, which assumes booter file size.
  //
  UINT32       Limit;
  //
  // Identifier for bootloader type. (e.g. Apple for boot.efi, or file name suffix for other possible booters)
  //
  CONST CHAR8  *Identifier;
} OC_BOOTER_PATCH;

/**
  Apple Boot Compatibility layer configuration.
**/
typedef struct OC_ABC_SETTINGS_ {
  ///
  /// Protect from boot.efi from defragmenting runtime memory. This fixes UEFI runtime services
  /// (date and time, NVRAM, power control, etc.) support on many types of firmware.
  /// Needed basically by everyone that uses SMM implementation of variable services.
  ///
  BOOLEAN  AvoidRuntimeDefrag;
  ///
  /// Setup virtual memory mapping after SetVirtualAddresses call. This fixes crashes in many types
  /// of firmware at early boot as they accidentally access virtual addresses after ExitBootServices.
  ///
  BOOLEAN  SetupVirtualMap;
  ///
  /// Provide custom Apple KASLR slide calculation for firmware with polluted low memory ranges.
  /// This also ensures that slide= argument is never passed to the operating system.
  ///
  BOOLEAN  ProvideCustomSlide;
  ///
  /// Provide max KASLR slide for firmware with polluted higher memory ranges.
  ///
  UINT8    ProvideMaxSlide;
  ///
  /// Remove runtime flag from MMIO areas and prevent virtual address assignment for known
  /// MMIO regions. This may improve the amount of slides available, but may not work on
  /// unknown configurations.
  ///
  BOOLEAN  DevirtualiseMmio;
  ///
  /// Disable passing -s to operating system through key presses, to simulate T2 Mac behaviour.
  /// Ref: https://support.apple.com/HT201573
  ///
  BOOLEAN  DisableSingleUser;
  ///
  /// Discard UEFI memory map after waking from hibernation and preserve the original mapping.
  ///
  BOOLEAN  DiscardHibernateMap;
  ///
  /// Try to patch Apple bootloader to have KASLR enabled even in SafeMode.
  ///
  BOOLEAN  EnableSafeModeSlide;
  ///
  /// Try to relocate memory to different address space when KASLR is disabled
  /// and lower memory is unavailable.
  ///
  BOOLEAN  AllowRelocationBlock;
  ///
  /// Attempt to protect certain memory regions from being incorrectly mapped:
  /// - CSM region could get used by the kernel due to being BS data,
  ///   which caused caused wake issues on older firmware.
  /// - MMIO regions can be marked as reserved memory and be thus unmapped,
  ///   which caused boot failures when accessing NVRAM.
  ///
  BOOLEAN  ProtectMemoryRegions;
  ///
  /// Rebuild memory map to be compatible with Apple kernel.
  /// - Apply memory attributes and split RT entries into code and data.
  /// - Reduce memory map entries through grouping to fit into 4KB.
  ///
  BOOLEAN  RebuildAppleMemoryMap;
  ///
  /// Ensure that ExitBootServices call succeeds even with outdated MemoryMap key.
  ///
  BOOLEAN  ForceExitBootServices;
  ///
  /// Disable NVRAM variable write support to protect from malware or to prevent
  /// buggy NVRAM implementations cause system issues.
  ///
  BOOLEAN  DisableVariableWrite;
  ///
  /// Protect secure boot variables.
  ///
  BOOLEAN  ProtectSecureBoot;
  ///
  /// Permit writing to executable memory in UEFI runtime services. Fixes crashes
  /// on many types of APTIO V firmware.
  ///
  BOOLEAN  EnableWriteUnprotector;
  ///
  /// Signal OSInfo protocol that every loaded non-macOS OS is macOS.
  /// Works around disabled IGPU in Windows and Linux on Apple laptops.
  ///
  BOOLEAN  SignalAppleOS;
  ///
  /// Provide OpenCore boot-signature when loading macOS.
  /// This resolves the ability to wake from hibernate on Mac EFI, which
  /// checks that the hibernation signature matches the SHA-1 hash of the
  /// EFI image it launches prior to exposing the image key.
  ///
  BOOLEAN  ForceBooterSignature;
  ///
  /// Booter signature for ForceBooterSignature.
  ///
  UINT8    BooterSignature[SHA1_DIGEST_SIZE];
  ///
  /// CoreImage may update and restore GetMemoryMap during loading (see InsertImageRecord)
  /// as it needs this for segment splitting. Unfortunately it assumes nobody else
  /// changes GetMemoryMap, and thus restores to its own CoreGetMemoryMap instead of
  /// the previous value. Fix it here.
  /// To make it worse VMware also replaces GetMemoryMap pointer in MacMisc, which CoreDxe
  /// effectively trashes when we load drivers. As a result without this hack VMware Fusion
  /// may show "Your Mac OS guest might run unreliably with more than one virtual core."
  /// message when running OpenCore.
  ///
  BOOLEAN  ProtectUefiServices;
  ///
  /// Fix OpenRuntime permissions in the memory map and memory attributes.
  ///
  BOOLEAN  SyncRuntimePermissions;
  ///
  /// List of physical addresses to not be devirtualised by DevirtualiseMmio.
  ///
  EFI_PHYSICAL_ADDRESS *MmioWhitelist;
  ///
  /// Size of list of physical addresses to not be devirtualised by DevirtualiseMmio.
  ///
  UINTN                MmioWhitelistSize;
  ///
  /// List of booter patches.
  ///
  OC_BOOTER_PATCH      *BooterPatches;
  ///
  /// Size of list of booter patches.
  ///
  UINT32               BooterPatchesSize;
  ///
  /// List of NULL-terminated handlers for TPL_APPLICATION execution within ExitBootServices.
  ///
  EFI_EVENT_NOTIFY     *ExitBootServicesHandlers;
  ///
  /// List of handler contexts for ExitBootServicesHandlers.
  ///
  VOID                 **ExitBootServicesHandlerContexts;
} OC_ABC_SETTINGS;

/**
  Initialize Apple Boot Compatibility layer. This layer is needed on partially
  incompatible firmware to prevent boot failure and UEFI services breakage.

  @param[in]  Settings  Compatibility layer configuration.
  @param[in]  CpuInfo   CPU information.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcAbcInitialize (
  IN OC_ABC_SETTINGS  *Settings,
  IN OC_CPU_INFO      *CpuInfo
  );

#endif // OC_AFTER_BOOT_COMPAT_LIB_H
