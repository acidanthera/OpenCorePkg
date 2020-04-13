/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_APFS_LIB_H
#define OC_APFS_LIB_H

/**
  Latest known from High Sierra version 10.13.6 (17G12034).
**/
#define OC_APFS_VERSION_HIGH_SIERRA 748077012000000ULL
#define OC_APFS_DATE_HIGH_SIERRA    20200219U

/**
  Latest known APFS from High Sierra 10.14.6 (18G4032).
**/
#define OC_APFS_VERSION_MOJAVE      945275008000000ULL
#define OC_APFS_DATE_MOJAVE         20200211U

/**
  Latest known APFS from Catalina 10.15.4 (19E287).
**/
#define OC_APFS_VERSION_CATALINA    1412101001000000ULL
#define OC_APFS_DATE_CATALINA       20200306U

/**
  Default version subject to increase.
**/
#define OC_APFS_VERSION_DEFAULT     OC_APFS_VERSION_HIGH_SIERRA
#define OC_APFS_DATE_DEFAULT        20200101U

/**
  Use default version as a minimal.
**/
#define OC_APFS_VERSION_AUTO  0
#define OC_APFS_DATE_AUTO     0

/**
  Use any version, not recommended.
**/
#define OC_APFS_VERSION_ANY ((UINT64) (-1))
#define OC_APFS_DATE_ANY    ((UINT32) (-1))

/**
  Configure APFS driver loading for subsequent connections.

  @param[in] MinVersion     Minimal allowed APFS driver version to load.
  @param[in] MinDate        Minimal allowed APFS driver date to load.
  @param[in] ScanPolicy     OpenCore scan policy.
  @param[in] IgnoreVerbose  Avoid APFS driver verbose output.
**/
VOID
OcApfsConfigure (
  IN UINT64   MinVersion,
  IN UINT32   MinDate,
  IN UINT32   ScanPolicy,
  IN BOOLEAN  IgnoreVerbose
  );

/**
  Connect APFS driver to a device at handle.

  @param[in] Handle   Device handle (APFS container).

  @retval EFI_SUCCESS if the device was connected.
**/
EFI_STATUS
OcApfsConnectDevice (
  IN EFI_HANDLE  Handle
  );

/**
  Connect APFS driver to all present devices.

  @param[in] Monitor   Setup monitoring for newly connected devices.

  @retval EFI_SUCCESS if at least one device was connected.
**/
EFI_STATUS
OcApfsConnectDevices (
  IN BOOLEAN      Monitor
  );

#endif // OC_APFS_LIB_H
