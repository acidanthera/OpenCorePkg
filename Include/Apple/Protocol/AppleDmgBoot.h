/** @file
  Apple Dmg Boot protocol.

Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DMG_BOOT_PROTOCOL_H
#define APPLE_DMG_BOOT_PROTOCOL_H

/**
  Relevant SMC keys used for debugging and T2 reporting:
  EFMU - Event Failed Mount UUID: boot volume UUID (network / disk / real).
  EFMV - Event Failed Mount Bridge Version GUID: from BridgeVersion.bin.
  EFMS - Event Failed Mount Status: 0 on failure
  EFBS - Event Failed Boot Policy: read from T2.
  TODO: Explore & Describe them properly.
**/

/**
  Failed boot poolicy variable in gAppleStartupManagerVariableGuid.
**/
#define APPLE_FAILED_BOOT_POLICY_VARIABLE L"AppleFailedBootPolicy"

/**
  Failed boot volume variable in gAppleStartupManagerVariableGuid (equal to EFMU).
**/
#define APPLE_FAILED_BOOT_VOLUME_UUID_VARIABLE L"AppleFailedBootVolumeUUID"

/**
  Set into EFMU / AppleFailedBootVolumeUUID for cmd-opt-R
  68D7AFF4-8079-4281-9A1E-A04A51FB12E0
**/
#define APPLE_RECOVERY_BOOT_NETWORK_GUID \
  { 0x68D7AFF4, 0x8079, 0x4281,          \
    { 0x9A, 0x1E, 0xA0, 0x4A, 0x51, 0xFB, 0x12, 0xE0 } }

/**
  Set into EFMU / AppleFailedBootVolumeUUID for cmd-R
  AF677042-9346-11E7-9F13-7200002BCC50
**/
#define APPLE_RECOVERY_BOOT_DISK_GUID    \
  { 0xAF677042, 0x9346, 0x11E7,          \
    { 0x9F, 0x13, 0x72, 0x00, 0x00, 0x2B, 0xCC, 0x50 } }

/**
  Apple DMG boot protocol GUID.
  85290934-28DC-4DF5-919A-60E28B1B9449
**/
#define APPLE_DMG_BOOT_PROTOCOL_GUID     \
  { 0x85290934, 0x28DC, 0x4DF5,          \
    { 0x91, 0x9A, 0x60, 0xE2, 0x8B, 0x1B, 0x94, 0x49 } }

/**
  Apple DMG boot protocol revision.
**/
#define APPLE_DMG_BOOT_PROTOCOL_REVISION 0x20000

/**
  Apple DMG boot protocol forward declaration.
**/
typedef struct APPLE_DMG_BOOT_PROTOCOL_ APPLE_DMG_BOOT_PROTOCOL;

/**
  Prints a debug message to the debug output device at the specified error level.

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
**/
typedef
VOID
(EFIAPI *APPLE_DMG_BOOT_DEBUG_PRINT) (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  );

/**
  Callback type used for prestart and poststart invocation.

  @param  Context     Opaque context.
**/
typedef
VOID
(EFIAPI *APPLE_DMG_BOOT_CALLBACK) (
  IN  VOID         *Context
  );

/**
  Boot from specified dmg with full chunklist verification.
  Dmg is located by replacing appending L"dmg".
  Chunklist is located by replacing appending L"chunklist".
  After verificaction completion a RamDisk is created with a
  SingleFile dmg file on it and UnverifiedBoot is called.

  @param[in]  This               Instance of this protocol.
  @param[in]  ParentImageHandle  Parent handle to boot from (local filesystem handle).
  @param[in]  BaseDmgPath        Path to dmg file without dmg suffix (e.g. L"\\BaseSystem.").
  @param[in]  PreStartCallback   Callback invoked prior to starting booter.
  @param[in]  PostStartCallback  Callback invoked after starting booter.
  @param[in]  CallbackContext    Callback context for PreStartCallback and PostStartCallback.
  @param[in]  DebugPrintFunction Debug printing function, builtin is used if NULL.

  @retval EFI_SUCCESS           Dmg booted and returned success.
  @retval EFI_INVALID_PARAMETER ParentImageHandle or DevicePath are NULL.
  @retval EFI_INVALID_PARAMETER Dmg is not valid.
  @retval EFI_UNSUPPORTED       Dmg is less than 512 bytes.
  @retval EFI_NOT_FOUND         Dmg was not found at this device path.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation error happened.
  @retval EFI_ACCESS_DENIED     Security violation.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DMG_BOOT_VERIFIED_BOOT) (
  IN APPLE_DMG_BOOT_PROTOCOL    *This,
  IN EFI_HANDLE                 ParentImageHandle,
  IN CONST CHAR16               *BaseDmgPath,
  IN APPLE_DMG_BOOT_CALLBACK    PreStartCallback OPTIONAL,
  IN APPLE_DMG_BOOT_CALLBACK    PostStartCallback OPTIONAL,
  IN VOID                       *CallbackContext OPTIONAL,
  IN APPLE_DMG_BOOT_DEBUG_PRINT DebugPrintFunction OPTIONAL
  );

/**
  Boot from specified dmg without any kind of verification (including chunklist).

  @param[in]  This               Instance of this protocol.
  @param[in]  ParentImageHandle  Parent handle to boot from.
  @param[in]  DevicePath         Path to dmg file.
  @param[in]  PreStartCallback   Callback invoked prior to starting booter.
  @param[in]  PostStartCallback  Callback invoked after starting booter.
  @param[in]  CallbackContext    Callback context for PreStartCallback and PostStartCallback.
  @param[in]  DebugPrintFunction Debug printing function, builtin is used if NULL.

  @retval EFI_SUCCESS           Dmg booted and returned success.
  @retval EFI_INVALID_PARAMETER ParentImageHandle or DevicePath are NULL.
  @retval EFI_INVALID_PARAMETER Dmg is not valid.
  @retval EFI_UNSUPPORTED       Dmg is less than 512 bytes.
  @retval EFI_NOT_FOUND         Dmg was not found at this device path.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation error happened.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DMG_BOOT_UNVERIFIED_BOOT) (
  IN APPLE_DMG_BOOT_PROTOCOL    *This,
  IN EFI_HANDLE                 ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN APPLE_DMG_BOOT_CALLBACK    PreStartCallback OPTIONAL,
  IN APPLE_DMG_BOOT_CALLBACK    PostStartCallback OPTIONAL,
  IN VOID                       *CallbackContext OPTIONAL,
  IN APPLE_DMG_BOOT_DEBUG_PRINT DebugPrintFunction OPTIONAL
  );

/**
  Dmg boot mode.
**/
typedef enum {
  RecoveryModeDefault,
  RecoveryModeDisk,
  RecoveryModeNetwork
} APPLE_DMG_BOOT_MODE;

/**
  Sets dmg boot mode for the dmg.
  When non default mode is set, boot failure will get
  APPLE_RECOVERY_BOOT_DISK_GUID or APPLE_RECOVERY_BOOT_DISK_GUID
  insted of UUID obtained from device path.

  @param[in]  This               Instance of this protocol.
  @param[in]  Mode               Desired boot mode.

  @retval EFI_SUCCESS           Boot mode was set.
  @retval EFI_INVALID_PARAMETER Boot mode is unsupported.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DMG_BOOT_SET_MODE) (
  IN APPLE_DMG_BOOT_PROTOCOL    *This,
  IN APPLE_DMG_BOOT_MODE        Mode
  );

/**
  Apple disk imge protocol.
**/
struct APPLE_DMG_BOOT_PROTOCOL_ {
  UINT32                          Revision;
  APPLE_DMG_BOOT_VERIFIED_BOOT    VerifiedDmgBoot;
  APPLE_DMG_BOOT_UNVERIFIED_BOOT  UnverifiedDmgBoot;
  APPLE_DMG_BOOT_SET_MODE         SetDmgBootMode;
};

extern EFI_GUID gAppleDiskImageProtocolGuid;

extern EFI_GUID gAppleRecoveryBootNetworkGuid;

extern EFI_GUID gAppleRecoveryBootDiskGuid;

#endif // APPLE_DISK_IMAGE_PROTOCOL_H
