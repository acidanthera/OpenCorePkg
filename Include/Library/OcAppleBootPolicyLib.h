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

#ifndef OC_APPLE_BOOT_POLICY_LIB_H
#define OC_APPLE_BOOT_POLICY_LIB_H

#include <Protocol/AppleBootPolicy.h>

/**
  Install and initialise Apple Boot policy protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_BOOT_POLICY_PROTOCOL *
OcAppleBootPolicyInstallProtocol (
  IN BOOLEAN  Reinstall
  );

EFI_STATUS
OcGetBooterFromPredefinedPathList (
  IN  EFI_HANDLE                Device,
  IN  EFI_FILE_PROTOCOL         *Root,
  IN  CONST CHAR16              **PredefinedPaths,
  IN  UINTN                     NumPredefinedPaths,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath  OPTIONAL,
  IN  CHAR16                    *Prefix       OPTIONAL
  );

/**
  Locates the bootable file of the given volume. Prefered are the values
  blessed, though if unavailable, hard-coded names are being verified and used
  if existing.

  The blessed paths are to be determined by the HFS Driver via
  EFI_FILE_PROTOCOL.GetInfo(). The related identifier definitions are to be
  found in AppleBless.h.

  @param[in]  Device              Thed device's Handle to perform the search on.
  @param[in]  PredefinedPaths     An array of file paths to scan for if no file
                                  was blessed.
  @param[in]  NumPredefinedPaths  The number of paths in PredefinedPaths.
  @param[out] FilePath            A pointer to the device path pointer to set to
                                  the file path of the boot file.

  @retval EFI_NOT_FOUND         A bootable file could not be found on the given
                                volume.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_SUCCESS           The operation completed successfully and the
                                BootFilePath Buffer has been filled.
  @retval other                 The status of an operation used to complete
                                this operation is returned.
**/
EFI_STATUS
OcBootPolicyGetBootFile (
  IN     EFI_HANDLE                Device,
  IN     CONST CHAR16              **PredefinedPaths,
  IN     UINTN                     NumPredefinedPaths,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

/**
  Please refer to OcBootPolicyGetBootFile. This function extends the
  functionality by supporting the APFS Preboot scheme.

  @param[in]  Device              Thed device's Handle to perform the search on.
  @param[in]  PredefinedPaths     An array of file paths to scan for if no file
                                  was blessed.
  @param[in]  NumPredefinedPaths  The number of paths in PredefinedPaths.
  @param[out] FilePath            A pointer to the device path pointer to set to
                                  the file path of the boot file.

  @retval EFI_NOT_FOUND         A bootable file could not be found on the given
                                volume.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_SUCCESS           The operation completed successfully and the
                                BootFilePath Buffer has been filled.
  @retval other                 The status of an operation used to complete
                                this operation is returned.
**/
EFI_STATUS
OcBootPolicyGetBootFileEx (
  IN  EFI_HANDLE                      Device,
  IN  CONST CHAR16                    **PredefinedPaths,
  IN  UINTN                           NumPredefinedPaths,
  OUT EFI_DEVICE_PATH_PROTOCOL        **FilePath
  );

/**
  Retrieves information about DevicePath.

  @param[in]  DevicePath          The device path to describe.
  @param[out] BootPathName        A pointer into which the folder portion of
                                  DevicePath is returned.
  @param[out] Device              A pointer into which the device handle of
                                  DevicePath is returned.

  @retval EFI_SUCCESS  The operation has been completed successfully.
  @retval other        DevicePath is not a valid file path.
**/
EFI_STATUS
OcBootPolicyDevicePathToDirPath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT CHAR16                    **BootPathName,
  OUT EFI_HANDLE                *Device
  );

/**
  Retrieves path info for the APFS Recovery volume associated with the APFS
  volume given by DevicePath.

  @param[in]  DevicePath          The device path to the APFS recovery volume.
  @param[in]  PathName            The file path to open.
  @param[in]  PredefinedPaths     An array of file paths to scan for if no file
                                  was blessed.
  @param[in]  NumPredefinedPaths  The number of paths in PredefinedPaths.
  @param[out] FullPathName        A pointer into which the full file path for
                                  DevicePath and PathName is returned.
  @param[out] Root                A pointer into which the opened Recovery
                                  volume root protocol is returned.
  @param[out] DeviceHandle        A pointer into which the device handle of
                                  DevicePath is returned.

  @retval EFI_SUCCESS    The operation has been completed successfully.
  @retval EFI_NOT_FOUND  The file path could not be found.
  @retval other          An unexpected error has occured.
**/
EFI_STATUS
OcBootPolicyGetApfsRecoveryFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONST CHAR16              *PathName,
  IN  CONST CHAR16              **PredefinedPaths,
  IN  UINTN                     NumPredefinedPaths,
  OUT CHAR16                    **FullPathName,
  OUT EFI_FILE_PROTOCOL         **Root,
  OUT EFI_HANDLE                *DeviceHandle
  );

/*
  Retrieves all valid APFS Recovery instances on the system.
  The APFS Recovery instance associated with the APFS volume referenced by
  Handle is moved to index 0, if present.

  @param[in]  Handle           If given, the Recovery instance associated with
                               this handle is added first.
  @param[out] Volumes          A pointer the APFS Recovery instances info is
                               returned into.
  @param[out] NumberOfEntries  A pointer into which the number of entries in
                               Volumes is returned.

  @retval EFI_SUCCESS    The operation has been completed successfully.
  @retval EFI_NOT_FOUND  No valid APFS Recovery instance could be found.
  @retval other          An unexpected error has occured.
*/
EFI_STATUS
OcBootPolicyGetAllApfsRecoveryFilePath (
  IN  EFI_HANDLE  Handle OPTIONAL,
  OUT VOID        **Volumes,
  OUT UINTN       *NumberOfEntries
  );

extern CONST CHAR16 *gAppleBootPolicyPredefinedPaths[];

///
/// All Apple Boot Policy predefined booter paths.
///
extern CONST UINTN gAppleBootPolicyNumPredefinedPaths;

///
/// Core Apple Boot Policy predefined booter paths.
///
extern CONST UINTN gAppleBootPolicyCoreNumPredefinedPaths;

#endif // OC_APPLE_BOOT_POLICY_LIB_H
