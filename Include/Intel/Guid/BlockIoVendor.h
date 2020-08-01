/** @file
  Guid for unrecognized EDD 3.0 device.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BLOCKIO_VENDOR_H__
#define __BLOCKIO_VENDOR_H__

//
// Guid is to specifiy the unrecognized EDD 3.0 device.
//
#define BLOCKIO_VENDOR_GUID \
  { 0xCF31FAC5, 0xC24E, 0x11D2, \
    {0x85, 0xF3, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }}

typedef struct {
  VENDOR_DEVICE_PATH              DevicePath;
  UINT8                           LegacyDriveLetter;
} BLOCKIO_VENDOR_DEVICE_PATH;

extern GUID gBlockIoVendorGuid;

#endif
