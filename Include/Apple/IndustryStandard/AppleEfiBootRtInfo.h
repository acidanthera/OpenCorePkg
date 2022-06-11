/** @file
  Copyright (C) 2022, Marvin Häuser. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_EFI_BOOT_RT_INFO_H
#define APPLE_EFI_BOOT_RT_INFO_H

///
/// The state value for when EfiBootRt has not been launched.
///
#define APPLE_EFI_BOOT_RT_STATE_NONE  SIGNATURE_32 ('b', 'r', '\0', '\0')

///
/// The state value for when EfiBootRt has been launched.
///
#define APPLE_EFI_BOOT_RT_STATE_LAUNCHED  SIGNATURE_32 ('b', 'r', 'H', 'i')

///
/// Arguments to the actual kernel call gate.
///
typedef struct {
  ///
  /// The address of the kernel entry point.
  ///
  UINTN    EntryPoint;
  ///
  /// The kernel argument handle.
  ///
  UINTN    Args;
} APPLE_EFI_BOOT_RT_KCG_ARGS;

/*
  The kernel call gate transfers control to the Apple XNU kernel.

  @param[in]  SystemTable   A pointer to the EFI System Table.
  @param[in]  KcgArguments  Arguments to the kernel call gate.

  @retval EFI_ABORTED  The kernel could not be started.
  @retval other        On success, this function does not return.
*/
typedef
EFI_STATUS
(EFIAPI *APPLE_EFI_BOOT_RT_KCG)(
  IN EFI_SYSTEM_TABLE                  *SystemTable,
  IN CONST APPLE_EFI_BOOT_RT_KCG_ARGS  *KcgArguments
  );

///
/// Apple EfiBootRt communication structure.
///
typedef struct {
  ///
  /// The image buffer of EfiBootRt.
  ///
  VOID                     *ImageBase;
  ///
  /// The size, in pages, of ImageBase.
  ///
  UINT64                   ImageNumPages;
  ///
  /// The current state of EfiBootRt.
  ///
  CONST UINT32             *State;
  ///
  /// The kernel call gate function.
  ///
  APPLE_EFI_BOOT_RT_KCG    KernelCallGate;
  ///
  /// The EfiBootRt build info string.
  ///
  CONST CHAR8              *BuildInfo;
  ///
  /// The EfiBootRt version string.
  ///
  CONST CHAR8              *VersionStr;
} APPLE_EFI_BOOT_RT_INFO;

#endif // APPLE_EFI_BOOT_RT_INFO_H
