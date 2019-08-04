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
  /// Protect from boot.efi from defragmenting runtime memory and setup virtual memory
  /// early mapping. This fixes NVRAM support on many firmwares.
  ///
  BOOLEAN  SetupAppleMap;
  ///
  /// Provide custom Apple KASLR slide calculation for firmwares with polluted low memory ranges.
  ///
  BOOLEAN  SetupAppleSlide;
  ///
  /// Discard UEFI memory map after waking from hibernation and preserve the original mapping.
  ///
  BOOLEAN  DiscardAppleS4Map;
  ///
  /// Try to patch Apple bootloader to have KASLR enabled even in SafeMode.
  ///
  BOOLEAN  EnableAppleSmSlide;
  ///
  /// Attempt to protect certain CSM memory regions from being used by the kernel .
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
