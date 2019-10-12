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

#ifndef OC_APPLE_BOOT_COMPAT_LIB_H
#define OC_APPLE_BOOT_COMPAT_LIB_H

/**
  Apple Boot Compatibility layer configuration.
**/
typedef struct OC_ABC_SETTINGS_ {
  ///
  /// Protect from boot.efi from defragmenting runtime memory. This fixes UEFI runtime services
  /// (date and time, NVRAM, power control, etc.) support on many firmwares.
  /// Needed basically by everyone that uses SMM implementation of variable services.
  ///
  BOOLEAN  AvoidRuntimeDefrag;
  ///
  /// Setup virtual memory mapping after SetVirtualAddresses call. This fixes crashes in many
  /// firmwares at early boot as they accidentally access virtual addresses after ExitBootServices.
  ///
  BOOLEAN  SetupVirtualMap;
  ///
  /// Provide custom Apple KASLR slide calculation for firmwares with polluted low memory ranges.
  /// This also ensures that slide= argument is never passed to the operating system.
  ///
  BOOLEAN  ProvideCustomSlide;
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
  /// Attempt to protect certain CSM memory regions from being used by the kernel.
  /// On older firmwares this caused wake issues.
  ///
  BOOLEAN  ProtectCsmRegion;
  ///
  /// Attempt to reduce memory map entries through grouping to fit into Apple kernel.
  ///
  BOOLEAN  ShrinkMemoryMap;
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
  /// Permit writing to executable memory in UEFI runtime services. Fixes crashes
  /// on many APTIO V firmwares.
  ///
  BOOLEAN  EnableWriteUnprotector;
  ///
  /// List of physical addresses to not be devirtualised by DevirtualiseMmio.
  ///
  EFI_PHYSICAL_ADDRESS *MmioWhitelist;
  ///
  /// Size of list of physical addresses to not be devirtualised by DevirtualiseMmio.
  ///
  UINTN                MmioWhitelistSize;
} OC_ABC_SETTINGS;

/**
  Initialize Apple Boot Compatibility layer. This layer is needed on partially
  incompatible firmwares to prevent boot failure and UEFI services breakage.

  @param[in]  Settings  Compatibility layer configuration.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcAbcInitialize (
  IN OC_ABC_SETTINGS  *Settings
  );

#endif // OC_APPLE_BOOT_COMPAT_LIB_H
