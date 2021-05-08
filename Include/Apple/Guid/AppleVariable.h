/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_VARIABLE_H
#define APPLE_VARIABLE_H

#include <Uefi.h>

///
/// The Apple Vendor Variable-vendor GUID.
///
#define APPLE_VENDOR_VARIABLE_GUID  \
  { 0x4D1EDE05, 0x38C7, 0x4A6A,     \
    { 0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x8C, 0x14 } }

///
/// The Apple boot Variable-vendor GUID.
///
#define APPLE_BOOT_VARIABLE_GUID  \
  { 0x7C436110, 0xAB2A, 0x4BBB,   \
    { 0xA8, 0x80, 0xFE, 0x41, 0x99, 0x5C, 0x9F, 0x82 } }

///
/// The Apple Core Storage Variable-vendor GUID.
///
#define APPLE_CORE_STORAGE_VARIABLE_GUID  \
  { 0x8D63D4FE, 0xBD3C, 0x4AAD,           \
    { 0x88, 0x1D, 0x86, 0xFD, 0x97, 0x4B, 0xC1, 0xDF } }

///
/// The Apple TRB Variable-vendor GUID.
///
#define APPLE_TAMPER_RESISTANT_BOOT_VARIABLE_GUID  \
  { 0x5D62B28D, 0x6ED2, 0x40B4,                    \
    { 0xA5, 0x60, 0x6C, 0xD7, 0x9B, 0x93, 0xD3, 0x66 } }

///
/// The Apple TRB EFI User GUID.
///
#define APPLE_TAMPER_RESISTANT_BOOT_EFI_USER_GUID  \
  { 0x4E8023FF, 0xA79A, 0x47D1,                    \
    { 0xA3, 0x42, 0x75, 0x24, 0xCF, 0xC9, 0x6D, 0xC4 } }

///
/// The Apple WiFi Network Variable-vendor GUID.
///
#define APPLE_WIRELESS_NETWORK_VARIABLE_GUID  \
  { 0x36C28AB5, 0x6566, 0x4C50,               \
    { 0x9E, 0xBD, 0xCB, 0xB9, 0x20, 0xF8, 0x38, 0x43 } }

///
/// The Apple Personalization Variable-vendor GUID.
///
#define APPLE_PERSONALIZATION_VARIABLE_GUID  \
  { 0xFA4CE28D, 0xB62F, 0x4C99,              \
    { 0x9C, 0xC3, 0x68, 0x15, 0x68, 0x6E, 0x30, 0xF9 } }

///
/// The Apple TRB Secure Variable-vendor GUID.
///
#define APPLE_TAMPER_RESISTANT_BOOT_SECURE_VARIABLE_GUID  \
  { 0xF68DA75E, 0x1B55, 0x4E70,                           \
    { 0xB4, 0x1B, 0xA7, 0xB7, 0xA5, 0xB7, 0x58, 0xEA } }

///
/// The Apple Netboot Variable-vendor GUID.
///
#define APPLE_NETBOOT_VARIABLE_GUID  \
  { 0x37BCBEC7, 0xA645, 0x4215,      \
    { 0x97, 0x9E, 0xF5, 0xAE, 0x4D, 0x11, 0x5F, 0x13 } }

///
/// The Apple Security-vendor GUID.
///
#define APPLE_SECURITY_VARIABLE_GUID \
  { 0x7870DBED, 0x151D, 0x63FE,      \
    { 0xF5, 0x88, 0x7C, 0x69, 0x94, 0x1C, 0xD0, 0x7B } }

///
/// The Apple Secure Boot Variable-vendor GUID.
///
#define APPLE_SECURE_BOOT_VARIABLE_GUID  \
  { 0x94B73556, 0x2197, 0x4702,          \
    { 0x82, 0xA8, 0x3E, 0x13, 0x37, 0xDA, 0xFB, 0xFB } }

///
/// The Apple Startup Manager Variable-vendor GUID.
///
#define APPLE_STARTUP_MANAGER_VARIABLE_GUID  \
  { 0x5EEB160F, 0x45FB, 0x4CE9,              \
    { 0xB4, 0xE3, 0x61, 0x03, 0x59, 0xAB, 0xF6, 0xF8 } }

///
/// The Apple Boot Variable Backup-vendor GUID.
///
#define APPLE_BACKUP_BOOT_VARIABLE_GUID      \
  { 0xA5CE328C, 0x769D, 0x11E9,              \
    { 0x94, 0xC7, 0x8C, 0x85, 0x90, 0x6B, 0xAC, 0x48 } }

///
/// User interface scale variable
/// UINT8: 1 or 2
/// gAppleVendorVariableGuid
///
#define APPLE_UI_SCALE_VARIABLE_NAME  L"UIScale"

///
/// User interface scale variable.
/// UINT32: RGBA
/// gAppleVendorVariableGuid
///
#define APPLE_DEFAULT_BACKGROUND_COLOR_VARIABLE_NAME  L"DefaultBackgroundColor"

typedef enum {
  ApplePickerEntryReasonUnknown           = 0, ///< Unknown
  ApplePickerEntryReasonManufacturingMode = 1, ///< IR Remote
  ApplePickerEntryReasonNvram             = 2, ///< NVRAM
  ApplePickerEntryReasonLeftOptKeyPress   = 3, ///< Left Option
  ApplePickerEntryReasonRightOptKeyPress  = 4, ///< Right Option
  ApplePickerEntryReasonBootDeviceTimeout = 5, ///< BDS Timeout
} APPLE_PICKER_ENTRY_REASON;

///
/// BootPicker startup mode.
/// UINT32: APPLE_PICKER_ENTRY_REASON.
/// gAppleVendorVariableGuid
///
#define APPLE_PICKER_ENTRY_REASON_VARIABLE_NAME  L"PickerEntryReason"

#define APPLE_SYSTEM_AUDIO_VOLUME_MUTED        BIT7
#define APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK  0x7FU

///
/// System audio volume.
/// UINT8: APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK | APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK
/// gAppleBootVariableGuid
///
#define APPLE_SYSTEM_AUDIO_VOLUME_VARIABLE_NAME  L"SystemAudioVolume"

///
/// System audio volume backup, restored by AppleEFIRuntime.kext at next reboot.
/// UINT8: APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK | APPLE_SYSTEM_AUDIO_VOLUME_VOLUME_MASK
/// gAppleBootVariableGuid
///
#define APPLE_SYSTEM_AUDIO_VOLUME_SAVED_VARIABLE_NAME  L"SystemAudioVolumeSaved"

///
/// System audio volume in decibels, created by AppleHDA.kext.
/// UINT8: SystemAudioVolume'
/// gAppleBootVariableGuid
///
#define APPLE_SYSTEM_AUDIO_VOLUME_SAVED_VARIABLE_DB_NAME  L"SystemAudioVolumeDB"

///
/// System language.
/// CHAR8[] String starting with language code (e.g. ru-RU:252).
/// gAppleBootVariableGuid
///
#define APPLE_PREV_LANG_KBD_VARIABLE_NAME   L"prev-lang:kbd"

///
/// Performance record data enable.
/// UINT8: Any value to enable performance gathering (one time).
/// gAppleBootVariableGuid
///
#define APPLE_EFI_BOOT_PERF_VARIABLE_NAME           L"efiboot-perf-record"

///
/// Performance record data address.
/// UINT32: Physical address to EfiACPIReclaimMemory containing perf data.
/// gAppleBootVariableGuid
///
#define APPLE_EFI_BOOT_PERF_ADDRESS_VARIABLE_NAME   L"efiboot-perf-record-data"

///
/// Performance record data size.
/// UINT32: Size of performance record data, usually 4096 bytes.
/// gAppleBootVariableGuid
///
#define APPLE_EFI_BOOT_PERF_SIZE_VARIABLE_NAME      L"efiboot-perf-record-data-size"

///
/// Hibernation wake log (mirror for APPLE_RTC_WAKE_LOG_ADDR).
/// UINT8[4]: Normally printed like 0x%02X 0x%02X % 3d 0x%02X.
/// gAppleBootVariableGuid
///
#define APPLE_WAKE_FAILURE_VARIABLE_NAME  L"wake-failure"

///
/// Force enable UEFI Windows support.
/// CHAR8: '1'.
/// gAppleBootVariableGuid
///
#define APPLE_UEFI_WINDOWS_BOOT_CAPABLE_VARIABLE_NAME  L"UEFIWindowsBootCapable"

///
/// Will install UEFI Windows on next reboot.
/// CHAR8: '1'.
/// gAppleBootVariableGuid
///
#define APPLE_INSTALL_WINDOWS_UEFI_VARIABLE_NAME L"InstallWindowsUEFI"

///
/// 7-bit packed panic information.
/// Data blob from with indices in the range of [0000, 0400).
/// gAppleBootVariableGuid
///
#define APPLE_PANIC_INFO_NO_VARIABLE_NAME L"AAPL,PanicInfo%04x"

///
/// Set display rotation angle.
/// UINT32: 0, 90, 180, 270.
/// gAppleBootVariableGuid
///
#define APPLE_FORCE_DISPLAY_ROTATION_VARIABLE_NAME L"ForceDisplayRotationInEFI"

///
/// BootCampt device path.
/// UEFI Device Path.
/// gAppleBootVariableGuid
///
#define APPLE_BOOT_CAMP_HD_VARIABLE_NAME L"BootCampHD"

///
/// Boot recovery once on the next restart in the specified mode in ASCII
/// (not null-terminated):
/// - locked              --- standard reboot (boot-feature-usage BIT6)
/// - guest               --- triggered by EfiBoot to perform FV2 "guest" recovery (boot-feature-usage BIT7)
/// - fde-recovery        --- triggered by EfiBoot to perform FV2 recovery
/// - fde-password-reset  --- triggered by EfiBoot to perform FV2 password reset
/// - unused              --- seems to be unused by the firmware, but used often by the users
/// - secure-boot         --- triggered by EfiBoot to perform recovery after SB failure
/// gAppleBootVariableGuid
///
#define APPLE_RECOVERY_BOOT_MODE_VARIABLE_NAME L"recovery-boot-mode"

///
/// Startup sound configuration variable.
/// UINT8: 00 (for unmuted, default) or 01 (for muted).
/// gAppleBootVariableGuid
///
#define APPLE_STARTUP_MUTE_VARIABLE_NAME L"StartupMute"

///
/// Recovery initiator device path. In general EfiBoot device path that called
/// reboot to recovery.
/// gAppleVendorVariableGuid
///
#define APPLE_RECOVERY_BOOT_INITIATOR_VARIABLE_NAME L"RecoveryBootInitiator"

///
/// A global variable storing the GUID of the APPLE_VENDOR EFI variable scope.
///
extern EFI_GUID gAppleVendorVariableGuid;

///
/// A global variable storing the GUID of the APPLE_BOOT EFI variable scope.
/// AKA gAppleEFINVRAMGuid
///
extern EFI_GUID gAppleBootVariableGuid;

///
/// A global variable storing the GUID of the APPLE_CORE_STORAGE EFI variable
/// scope.
///
extern EFI_GUID gAppleCoreStorageVariableGuid;

///
/// A global variable storing the GUID of the APPLE_TAMPER_RESISTANT_BOOT EFI
/// variable scope.
/// AKA gAppleEFINVRAMTRBStagingCommandGuid.
///
extern EFI_GUID gAppleTamperResistantBootVariableGuid;

///
/// A global variable storing the GUID of the APPLE_WIRELESS_NETWORK EFI
/// variable scope.
///
extern EFI_GUID gAppleWirelessNetworkVariableGuid;

///
/// A global variable storing the GUID of the APPLE_PERSONALIZATION EFI
/// variable scope.
///
extern EFI_GUID gApplePersonalizationVariableGuid;

///
/// A global variable storing the GUID of
/// the APPLE_TAMPER_RESISTANT_BOOT_SECURE_VARIABLE_GUID EFI variable scope.
/// AKA gAppleEFINVRAMTRBSecureVariableGuid.
///
extern EFI_GUID gAppleTamperResistantBootSecureVariableGuid;

///
/// A global variable storing the GUID of
/// the APPLE_TAMPER_RESISTANT_BOOT_EFI_USER_VARIABLE_GUID EFI variable scope.
///
extern EFI_GUID gAppleTamperResistantBootEfiUserVariableGuid;

///
/// A global variable storing the GUID of the APPLE_NETBOOT EFI variable
/// scope.
///
extern EFI_GUID gAppleNetbootVariableGuid;

///
/// A global variable storing the GUID of the APPLE_SECURITY EFI variable
/// scope.
///
extern EFI_GUID gAppleSecurityVariableGuid;

///
/// A global variable storing the GUID of the APPLE_SECURE_BOOT_VARIABLE_GUID
/// EFI variable  scope.
///
extern EFI_GUID gAppleSecureBootVariableGuid;

///
/// A global variable storing the GUID of the APPLE_STARTUP_MANAGER variable scope.
///
extern EFI_GUID gAppleStartupManagerVariableGuid;

///
/// A global variable storing the GUID of the APPLE_BACKUP_BOOT variable scope.
///
extern EFI_GUID gAppleBackupBootVariableGuid;

#endif // APPLE_VARIABLE_H
