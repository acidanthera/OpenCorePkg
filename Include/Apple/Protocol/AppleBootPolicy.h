/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_BOOT_POLICY_H
#define APPLE_BOOT_POLICY_H

#include <Protocol/SimpleFileSystem.h>

#define APPLE_BOOT_POLICY_PROTOCOL_REVISION  0x00000003

///
/// The GUID of the APPLE_BOOT_POLICY_PROTOCOL.
///
#define APPLE_BOOT_POLICY_PROTOCOL_GUID  \
  { 0x62257758, 0x350C, 0x4D0A,          \
    { 0xB0, 0xBD, 0xF6, 0xBE, 0x2E, 0x1E, 0x27, 0x2C } }

// BOOT_POLICY_GET_BOOT_FILE
/** Locates the bootable file of the given volume.  Prefered are the values
    blessed, though if unavailable, hard-coded names are being verified and
    used if existing.

  The blessed paths are to be determined by the HFS Driver via
  EFI_FILE_PROTOCOL.GetInfo().  The related identifier definitions are to be
  found in AppleBless.h.

  @param[in]  Device    The Device's Handle to perform the search on.
  @param[out] FilePath  A pointer to the device path pointer to set to the file
                        path of the boot file.

  @retval EFI_NOT_FOUND         A bootable file could not be found on the given
                                volume.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_SUCCESS           The operation completed successfully and the
                                BootFilePath Buffer has been filled.
  @retval other                 The status of an operation used to complete
                                this operation is returned.
**/
typedef
EFI_STATUS
(EFIAPI *BOOT_POLICY_GET_BOOT_FILE) (
  IN     EFI_HANDLE                Device,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

//
// Neither is used at this moment.
// https://opensource.apple.com/source/kext_tools/kext_tools-528.220.8/KextAudit/efi_smc.h.auto.html
//
typedef enum {
  // Boot Policy not valid retry.
  BootPolicyNotReady,

  // Boot Selected macOS.
  BootPolicyOk,

  // Boot Recovery OS, update bridgeOS.
  BootPolicyUpdate,

  // Full system reboot, boot selected macOS.
  BootPolicyReboot,

  // Version unknown boot to recovery OS to get more info.
  BootPolicyUnknown,

  // Update failed take the failure path.
  BootPolicyBridgeOSUpdateFailed,

  // Boot Recovery OS to change security policy.
  BootPolicyRecoverySecurityPolicyUpdate,

  // Valid values will be less that this version.
  BootPolicyMaxValue
} BOOT_POLICY_ACTION;

typedef
EFI_STATUS
(EFIAPI *BOOT_POLICY_GET_BOOT_FILE_EX) (
  IN  EFI_HANDLE                Device,
  IN  BOOT_POLICY_ACTION        Action,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

typedef
EFI_STATUS
(EFIAPI *BOOT_POLICY_DEVICE_PATH_TO_DIR_PATH) (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device,
  OUT EFI_HANDLE                *ApfsVolumeHandle
  );

typedef
EFI_STATUS
(EFIAPI *BOOT_POLICY_GET_APFS_RECOVERY_FILE_PATH) (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  OUT CHAR16                    **FullPathName,
  OUT VOID                      **Reserved,
  OUT EFI_FILE_PROTOCOL         **Root,
  OUT EFI_HANDLE                *DeviceHandle
  );

typedef
EFI_STATUS
(EFIAPI *BOOT_POLICY_GET_ALL_APFS_RECOVERY_FILE_PATH) (
  IN  EFI_HANDLE  Handle,
  OUT VOID        **Volumes,
  OUT UINTN       *NumberOfEntries
  );

///
/// The structure exposed by the APPLE_BOOT_POLICY_PROTOCOL.
///
typedef struct {
  UINTN                                       Revision;                    ///< The revision of the installed protocol.
  BOOT_POLICY_GET_BOOT_FILE                   GetBootFile;                 ///< Present as of Revision 1.
  BOOT_POLICY_GET_BOOT_FILE_EX                GetBootFileEx;               ///< Present as of Revision 3.
  BOOT_POLICY_DEVICE_PATH_TO_DIR_PATH         DevicePathToDirPath;         ///< Present as of Revision 3.
  BOOT_POLICY_GET_APFS_RECOVERY_FILE_PATH     GetApfsRecoveryFilePath;     ///< Present as of Revision 3.
  BOOT_POLICY_GET_ALL_APFS_RECOVERY_FILE_PATH GetAllApfsRecoveryFilePath;  ///< Present as of Revision 3.
} APPLE_BOOT_POLICY_PROTOCOL;

///
/// A global variable storing the GUID of the APPLE_BOOT_POLICY_PROTOCOL.
///
extern EFI_GUID gAppleBootPolicyProtocolGuid;

#endif // APPLE_BOOT_POLICY_H
