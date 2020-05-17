/** @file
Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FEATURES_H
#define APPLE_FEATURES_H

//
// OEM Firmware Volume Information - Firmware Feature Bits.
// These bits are exposed via APPLE_SMBIOS_TABLE_TYPE128 FirmwareFeatures,
// and UEFI variables under gAppleVendorVariableGuid (4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14):
// UINT32 FirmwareFeatures / FirmwareFeaturesMask.
// UINT64 ExtendedFirmwareFeatures / ExtendedFirmwareFeaturesMask.
//
// nvram -x 4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:FirmwareFeatures
// nvram -x 4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:FirmwareFeaturesMask
// nvram -x 4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:ExtendedFirmwareFeatures
// nvram -x 4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:ExtendedFirmwareFeaturesMask
//
// Relevant binaries (besides firmware and boot.efi) can be found with FirmwareFeatures
// and IORegistry exposed firmware-features:
// /usr/libexec/diskmanagementd
// /usr/libexec/kextd
// /usr/sbin/bless
// /usr/sbin/kextcache
// /System/Library/CoreServices/backupd.bundle/Contents/Resources/brtool
// /System/Library/CoreServices/Software Update.app/Contents/MacOS/Software Update
// /System/Library/Extensions/AppleACPIPlatform.kext/Contents/MacOS/AppleACPIPlatform
// /System/Library/PrivateFrameworks/DiskManagement.framework/Versions/A/DiskManagement
// /System/Library/PrivateFrameworks/FindMyMac.framework/XPCServices/com.apple.HasTRB.xpc/Contents/MacOS/com.apple.HasTRB
// /System/Library/PrivateFrameworks/StorageKit.framework/Versions/A/Resources/storagekitd
// /System/Library/Extensions/AppleSMBIOS.kext/Contents/MacOS/AppleSMBIOS
// /System/Library/PreferencePanes/StartupDisk.prefPane/Contents/MacOS/StartupDisk
// /System/Library/PrivateFrameworks/Install.framework/Frameworks/DistributionKit.framework/Versions/A/DistributionKit
// /System/Library/PrivateFrameworks/Install.framework/Versions/A/Install
// /System/Library/PrivateFrameworks/OSInstaller.framework/Versions/A/OSInstaller
//
// NOTE: There is an unconfirmed belief that Thunderbolt also has a bit.
// NOTE: FirmwareFeatures enabling code is similar in all recent firmwares, T2 and not:
// VOID SetPlatformFeatures (VOID) {
//   UINT64 Mask     = 0xFF1FFF3FU; /* Model dependent */
//   UINT64 Features = 0xFC07E136U; /* Model dependent */
//   UINT32 SecurityPolicy;
//   APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL *Policy;
//   if (!EFI_ERROR (gBS->LocateProtocol(&gApplePlatformSecurityPolicyProtocolGuid, NULL, (VOID **) &Policy))
//     && (!EFI_ERROR (Policy->GetSecurityPolicy(Policy, &SecurityPolicy)))) {
//     if (SecurityPolicy & 1) Features |= 0x400U;
//     if (SecurityPolicy & 2) Features |= 0x800U;
//     Features |= 0x1000000U;
//   }
//   Features = ((*(UINT64 *)(0xFFFFFF80) != 0x4E15E2F599858AC6ULL) << 12U) | Features & 0xFFFFFFFFFFFFEFFFULL;
//   if (!EFI_ERROR (gRT->SetVariable(L"FirmwareFeaturesMask", &gAppleVendorVariableGuid, 6, sizeof (UINT32), &Mask)))
//     gRT->SetVariable(L"FirmwareFeatures", &gAppleVendorVariableGuid, 6, sizeof (UINT32), &Features);
//   if ((Features & 0x2000000U)
//     && !EFI_ERROR (gRT->SetVariable(L"ExtendedFirmwareFeaturesMask", &gAppleVendorVariableGuid, 6, sizeof (UINT64), &Mask)))
//     gRT->SetVariable(L"ExtendedFirmwareFeatures", &gAppleVendorVariableGuid, 6, sizeof (UINT64), &Features);
// }
//

//
// Supports CSM module, which allows booting legacy Windows operating systems like Windows 7.
// See: https://opensource.apple.com/source/bless/bless-166/libbless/EFI/BLSupportsLegacyMode.c.auto.html
//
#define FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE           0x00000001U // 0
//
// Supports CD/DVD Drive boot via El Torito.
//
#define FW_FEATURE_SUPPORTS_CD_DRIVE_BOOT             0x00000002U // 1
//
// Supports target disk mode, normally invoked by pressing T key at start.
// Note, AppleTamperResistantBootProtocol+6 may prohibit this.
// See: BootPicker.
//
#define FW_FEATURE_SUPPORTS_TARGET_DISK_MODE          0x00000004U // 2
#define FW_FEATURE_UNKNOWN_BIT3                       0x00000008U // 3
//
// Supports NET boot. Read by StartupDisk.prefPane.
// FW_FEATURE_SUPPORTS_WIRELESS also implies NET boot support.
//
#define FW_FEATURE_SUPPORTS_NET_BOOT                  0x00000010U // 4
//
// Supports SlingShot recovery embedded into the firmware.
//
#define FW_FEATURE_SUPPORTS_SLING_SHOT                0x00000020U // 5
#define FW_FEATURE_UNKNOWN_BIT6                       0x00000040U // 6
#define FW_FEATURE_UNKNOWN_BIT7                       0x00000080U // 7
//
// Has Wi-Fi module and supports wireless drivers in the firmware.
// Found in SlingShot.
//
#define FW_FEATURE_SUPPORTS_WIRELESS                  0x00000100U // 8
#define FW_FEATURE_UNKNOWN_BIT9                       0x00000200U // 9
//
// Bit 0 of the ApplePlatformSecurityPolicy.
// TODO: Explore, related to ApplePlatformInfoDatabaseProtocol.
// Set by AppleFirmwareFeaturesProtocol.
//
#define FW_FEATURE_PLATFORM_SECURITY_POLICY_01        0x00000400U // 10
//
// Bit 1 of the ApplePlatformSecurityPolicy.
// TODO: Explore, related to AppleTamperResistantBootProtocol.
// Set by AppleFirmwareFeaturesProtocol.
//
#define FW_FEATURE_PLATFORM_SECURITY_POLICY_02        0x00000800U // 11
//
// Supports Apple Tamper Resistant Boot, which helps FindMyMac functionality.
// Not set on Macs with T2 via detected non APPLE_EFI_FIRMWARE_GEN3_ID_MAGIC.
// See: /System/Library/PrivateFrameworks/FindMyMac.framework/XPCServices/com.apple.HasTRB.xpc/Contents/MacOS/com.apple.HasTRB.
//
#define FW_FEATURE_SUPPORTS_TRB                       0x00001000U // 12
#define FW_FEATURE_UNKNOWN_BIT13                      0x00002000U // 13
//
// Supports USB 2.0 in the firmware. Found in AppleACPIPlatform.kext.
//
#define FW_FEATURE_SUPPORTS_HIGH_SPEED_USB            0x00004000U // 14
#define FW_FEATURE_UNKNOWN_BIT15                      0x00008000U // 15
#define FW_FEATURE_UNKNOWN_BIT16                      0x00010000U // 16
//
// Instructs AppleACPIPlatform.kext not to substitute certain USB ports with other ports.
// In particular: EHC2-2 -> EHC1-3, EHC2-3 -> EHC1-4.
//
#define FW_FEATURE_DISABLE_USB_SUBSTITUTE_WORKAROUND  0x00020000U // 17
#define FW_FEATURE_UNKNOWN_BIT18                      0x00040000U // 18
//
// Supports APFS filesystem (via JumpStart or built-in driver).
// Verified by OSInstaller.framework (apfsSupportedByROM). Either bit would do.
//
#define FW_FEATURE_SUPPORTS_APFS                      0x00080000U // 19
//
// Supports APFS filesystem (unused by by any known Mac Model including Macmini7,1).
// Verified by OSInstaller.framework (apfsSupportedByROM). Either bit would do.
// Could be reserved for APFS Fusion Drive support, but unused even on them.
//
#define FW_FEATURE_SUPPORTS_APFS_EXTRA                0x00100000U // 20
#define FW_FEATURE_UNKNOWN_BIT21                      0x00200000U // 21
//
// Supports Apple Tamper Resistant Boot X. This is supposed to be activation lock.
// See: /System/Library/CoreServices/Language Chooser.app/Contents/MacOS/Language Chooser
//
#define FW_FEATURE_SUPPORTS_TRBX                      0x00400000U // 22
#define FW_FEATURE_UNKNOWN_BIT23                      0x00800000U // 23
//
// Supports platform security policy.
// Exposed in UEFI via gApplePlatformSecurityPolicyProtocolGuid GUID.
// Set by gAppleFirmwareFeaturesProtocolGuid.
//
#define FW_FEATURE_SUPPORTS_PLATFORM_SECURITY_POLICY  0x01000000U // 24
//
// Supports 64-bit ExtendedFirmwareFeatures/ExtendedFirmwareFeaturesMask variables.
// Lower 32-bits are same as FirmwareFeatures, and higher 32-bits contain new information.
//
#define FW_FEATURE_SUPPORTS_EXTENDED_FEATURES         0x02000000U // 25
#define FW_FEATURE_UNKNOWN_BIT26                      0x04000000U // 26
#define FW_FEATURE_UNKNOWN_BIT27                      0x08000000U // 27
//
// Instructs boot.efi not to adjust memory map on MacBookAir4,1 and MacBookAir4,2
// during hibernate wake.
//
#define FW_FEATURE_DISABLE_MBA_S4_WORKAROUND          0x10000000U // 28
//
// Supports Windows UEFI boot (Windows 8 and newer).
// Also known as UEFIWindowsBootCapable variable.
// DMIsPreBootEnvironmentUEFIWindowsBootCapable
//
#define FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT         0x20000000U // 29
#define FW_FEATURE_UNKNOWN_BIT30                      0x40000000U // 30
//
// Instructs boot.efi not to adjust memory map based on AcpiGlobalVariable->AcpiBootScriptTable.
// See: https://github.com/Piker-Alpha/macosxbootloader/blob/1abba11e3b792dd29c2a08c410c122efd1d19e98/src/boot/AcpiUtils.cpp#L152
//
#define FW_FEATURE_DISABLE_BOOTSCRIPT_WORKAROUND      0x80000000U // 31

//
// OEM Platform Feature Information - Platform feature bits
// These bits are exposed via APPLE_SMBIOS_TABLE_TYPE133 FirmwareFeatures:
// UINT64 PlatformFeature.
//
// Ref: https://github.com/al3xtjames/AppleSystemInfo/blob/master/src/util/dump_features.c
//
// Relevant binaries can be found with IORegistry exposed firmware-features
// and AppleSystemInfo ASI_IsPlatformFeatureEnabled function:
// /System/Library/Extensions/AppleACPIPlatform.kext/Contents/PlugIns/AppleACPIEC.kext/Contents/MacOS/AppleACPIEC
// /System/Library/Extensions/AppleSMBIOS.kext/Contents/MacOS/AppleSMBIOS
// /System/Library/PrivateFrameworks/AppleSystemInfo.framework/Versions/A/AppleSystemInfo
// /System/Library/PrivateFrameworks/NeutrinoCore.framework/Versions/A/NeutrinoCore
// /System/Library/PrivateFrameworks/PhotoLibraryPrivate.framework/Versions/A/Frameworks/PAImaging.framework/Versions/A/PAImaging
// /System/Library/CoreServices/PowerChime.app/Contents/MacOS/PowerChime
// /System/Library/SystemProfiler/SPMemoryReporter.spreporter/Contents/MacOS/SPMemoryReporter
//

#define PT_FEATURE_UNKNOWN_BIT0                0x00000001U // 0
//
// Has not replaceable system memory (RAM).
//
#define PT_FEATURE_HAS_SOLDERED_SYSTEM_MEMORY  0x00000002U // 1
//
// Has headless (connector-less) GPU present.
//
#define PT_FEATURE_HAS_HEADLESS_GPU            0x00000004U // 2
//
// Supports host power management.
//
#define PT_FEATURE_SUPPORTS_HOST_PM            0x00000008U // 3
//
// Supports Power Chime sound at boot.
//
#define PT_FEATURE_SUPPORTS_POWER_CHIME        0x00000010U // 4
#define PT_FEATURE_UNKNOWN_BIT5                0x00000020U // 5
#define PT_FEATURE_UNKNOWN_BIT6                0x00000040U // 6
#define PT_FEATURE_UNKNOWN_BIT7                0x00000080U // 7
#define PT_FEATURE_UNKNOWN_BIT8                0x00000100U // 8
#define PT_FEATURE_UNKNOWN_BIT9                0x00000200U // 9
#define PT_FEATURE_UNKNOWN_BIT10               0x00000400U // 10
#define PT_FEATURE_UNKNOWN_BIT11               0x00000800U // 11
#define PT_FEATURE_UNKNOWN_BIT12               0x00001000U // 12
#define PT_FEATURE_UNKNOWN_BIT13               0x00002000U // 13
#define PT_FEATURE_UNKNOWN_BIT14               0x00004000U // 14
#define PT_FEATURE_UNKNOWN_BIT15               0x00008000U // 15
#define PT_FEATURE_UNKNOWN_BIT16               0x00010000U // 16
#define PT_FEATURE_UNKNOWN_BIT17               0x00020000U // 17
#define PT_FEATURE_UNKNOWN_BIT18               0x00040000U // 18
#define PT_FEATURE_UNKNOWN_BIT19               0x00080000U // 19
#define PT_FEATURE_UNKNOWN_BIT20               0x00100000U // 20
#define PT_FEATURE_UNKNOWN_BIT21               0x00200000U // 21
#define PT_FEATURE_UNKNOWN_BIT22               0x00400000U // 22
#define PT_FEATURE_UNKNOWN_BIT23               0x00800000U // 23
#define PT_FEATURE_UNKNOWN_BIT24               0x01000000U // 24
#define PT_FEATURE_UNKNOWN_BIT25               0x02000000U // 25
#define PT_FEATURE_UNKNOWN_BIT26               0x04000000U // 26
#define PT_FEATURE_UNKNOWN_BIT27               0x08000000U // 27
#define PT_FEATURE_UNKNOWN_BIT28               0x10000000U // 28
#define PT_FEATURE_UNKNOWN_BIT29               0x20000000U // 29
#define PT_FEATURE_UNKNOWN_BIT30               0x40000000U // 30
#define PT_FEATURE_UNKNOWN_BIT31               0x80000000U // 31

#endif // APPLE_FEATURES_H
