/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_MAC_EFI_SPEC_H
#define APPLE_MAC_EFI_SPEC_H

// EFI Revision information

#define APPLE_EFI_FIRMWARE_REVISION  0x010000
#define APPLE_EFI_FIRMWARE_VENDOR    L"Apple"

//
// Magic ID found at 0xFFFFFF80 address in newer firmwares like MBP12,1.
//
#define APPLE_EFI_FIRMWARE_GEN2_ID_MAGIC  \
  { 0xA1, 0x7B, 0xE5, 0x98, 0x31, 0x22, 0x4F, 0x4E }

//
// Magic ID similar to APPLE_EFI_FIRMWARE_ID_MAGIC in T2-based firmwares.
//
#define APPLE_EFI_FIRMWARE_GEN3_ID_MAGIC  \
  { 0xC6, 0x8A, 0x85, 0x99, 0xF5, 0xE2, 0x15, 0x4E }

// APPLE_BOOTER_FILE_NAME
#define APPLE_BOOTER_FILE_NAME  L"boot.efi"

// APPLE_BOOTLOADER_FILE_PATH
#define APPLE_BOOTER_DEFAULT_FILE_NAME  L"\\System\\Library\\CoreServices\\boot.efi"

// BOOT_EFI_FILE_NAME
#define APPLE_BOOTER_ROOT_FILE_NAME  L"\\boot.efi"

// Apple EFI File location to boot from on removable media devices

// APPLE_REMOVABLE_MEDIA_FILE_NAME_IA32
#define APPLE_REMOVABLE_MEDIA_FILE_NAME_IA32  \
  L"\\EFI\\APPLE\\IA32\\BOOT.EFI"

// APPLE_REMOVABLE_MEDIA_FILE_NAME_X64
#define APPLE_REMOVABLE_MEDIA_FILE_NAME_X64  \
  L"\\EFI\\APPLE\\X64\\BOOT.EFI"

// APPLE_REMOVABLE_MEDIA_FILE_NAME_ARM
#define APPLE_REMOVABLE_MEDIA_FILE_NAME_ARM  \
  L"\\EFI\\APPLE\\ARM\\BOOT.EFI"

// APPLE_REMOVABLE_MEDIA_FILE_NAME_AARCH64
#define APPLE_REMOVABLE_MEDIA_FILE_NAME_AARCH64  \
  L"\\EFI\\APPLE\\AARCH64\\BOOT.EFI"

// APPLE_SYSTEM_VERSION_FILE_NAME
#define APPLE_SYSTEM_VERSION_FILE_NAME  \
  L"\\System\\Library\\CoreServices\\SystemVersion.plist"

#endif // APPLE_MAC_EFI_SPEC_H
