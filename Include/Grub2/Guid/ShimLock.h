/** @file
  GRUB2 shim GUID values.

  Copyright (c) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef __SHIM_GUID_H
#define __SHIM_GUID_H

#include <Uefi.h>

///
/// Shim lock protocol GUID.
///
#define SHIM_LOCK_GUID                  \
  { 0x605DAB50, 0xE046, 0x4300,         \
    { 0xAB, 0xB6, 0x3D, 0xD8, 0x10, 0xDD, 0x8B, 0x23 }}

///
/// Exported GUID identifiers.
///
extern EFI_GUID  gShimLockGuid;

#endif // __SHIM_GUID_H
