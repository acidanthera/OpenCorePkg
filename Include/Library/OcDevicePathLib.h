/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DEVICE_PATH_LIB_H_
#define OC_DEVICE_PATH_LIB_H_

// AppendFileNameDevicePath
/**

  @param[in] DevicePath  The device path which to append the file path.
  @param[in] FileName    The file name to append to the device path.

  @retval EFI_SUCCESS            The defaults were initialized successfully.
  @retval EFI_INVALID_PARAMETER  The parameters passed were invalid.
  @retval EFI_OUT_OF_RESOURCES   The system ran out of memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
AppendFileNameDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CHAR16                    *FileName
  );

// DevicePathToText
/** Converts a device path structure to its string representative.

  @param[in] StorageDevicePath  The device path to convert to unicode string.
  @param[in] DisplayOnly        If DisplayOnly is TRUE, then the shorter text
                                representation of the display node is used,
                                where applicable.  If DisplayOnly is FALSE,
                                then the longer text representation of the
                                display node is used.
  @param[in] AllowShortcuts     If AllowShortcuts is TRUE, then the shortcut
                                forms of text representation for a device node
                                can be used, where applicable.

  @retval EFI_SUCCESS            The defaults were initialized successfully.
  @retval EFI_INVALID_PARAMETER  The parameters passed were invalid.
  @retval EFI_OUT_OF_RESOURCES   The system ran out of memory.
**/
CHAR16 *
DevicePathToText (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN BOOLEAN                   DisplayOnly,
  IN BOOLEAN                   AllowShortcuts
  );

// FindDevicePathNodeWithType
/** Locate the node inside the device path specified by Type an SubType values.

  @param[in] DevicePath  The device path used in the search.
  @param[in] Type        The Type field of the device path node specified by Node.
  @param[in] SubType     The SubType field of the device path node specified by Node.

  @return  Returned is the first Device Path Node with the given type.
**/
EFI_DEVICE_PATH_PROTOCOL *
FindDevicePathNodeWithType (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT8                     Type,
  IN UINT8                     SubType OPTIONAL
  );

// IsDevicePathEqual
/** 

  @param[in] DevicePath1 The first device path protocol to compare.
  @param[in] DevicePath2 The second device path protocol to compare.

  @retval TRUE         The device paths matched
  @retval FALSE        The device paths were different
**/
BOOLEAN
EFIAPI
IsDevicePathEqual (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath1,
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath2
  );

// IsDeviceChild
/** 

  @param[in] ParentPath  The parent device path protocol to check against.
  @param[in] ChildPath   The device path protocol of the child device to compare.

  @retval TRUE         The child device path contains the parent device path.
  @retval FALSE        The device paths were different
**/
BOOLEAN
EFIAPI
IsDeviceChild (
  IN  EFI_DEVICE_PATH_PROTOCOL      *ParentPath,
  IN  EFI_DEVICE_PATH_PROTOCOL      *ChildPath,
  IN  UINT8                         EndPathType
  );

VOID
DebugPrintDevicePath (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

#endif // OC_DEVICE_PATH_LIB_H_
