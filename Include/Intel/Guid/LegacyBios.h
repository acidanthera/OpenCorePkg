/** @file
  Defines a Tag GUID used to mark a UEFI legacy BIOS thunk driver based
  on legacy BIOS services and legacy option ROM. This Tag GUID must be installed on
  the ImageHandle of any module that follows the EFI Driver Model and uses
  the Int86() or FarCall() services of the Legacy Bios Protocol to produce
  a standard UEFI I/O Protocol.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LEGACY_BIOS_H_
#define _LEGACY_BIOS_H_

///
/// The Global ID for the Legacy BIOS GUID that must be installed onto the ImageHandle
/// of any module follows the EFI Driver Model and uses the Int86() or FarCall()
/// services of the Legacy BIOS Protocol to produce a standard UEFI I/O Protocol.
///
#define EFI_LEGACY_BIOS_GUID \
  { \
    0x2e3044ac, 0x879f, 0x490f, {0x97, 0x60, 0xbb, 0xdf, 0xaf, 0x69, 0x5f, 0x50 } \
  }

extern EFI_GUID  gEfiLegacyBiosGuid;

#endif
