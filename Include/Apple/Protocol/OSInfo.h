/** @file
  EFI OS Info protocol.

  Copyright (c) 2014-2017, mhaeuser. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef EFI_OS_INFO_H
#define EFI_OS_INFO_H

// EFI_OS_INFO_PROTOCOL_REVISION
#define EFI_OS_INFO_PROTOCOL_REVISION_NAME       0x00000001
#define EFI_OS_INFO_PROTOCOL_REVISION_VENDOR     0x00000002
#define EFI_OS_INFO_PROTOCOL_REVISION_VTD        0x00000003
#define EFI_OS_INFO_PROTOCOL_REVISION_FORCE_VTD  0x00000004
#define EFI_OS_INFO_PROTOCOL_REVISION            0x00000004

// EFI_OS_INFO_PROTOCOL_GUID
#define EFI_OS_INFO_PROTOCOL_GUID  \
  { 0xC5C5DA95, 0x7D5C, 0x45E6,    \
    { 0xB2, 0xF1, 0x3F, 0xD5, 0x2B, 0xB1, 0x00, 0x77 } }

#define EFI_OS_INFO_APPLE_VENDOR_NAME  "Apple Inc."

// OS_INFO_OS_NAME
typedef
VOID
(EFIAPI *OS_INFO_OS_NAME)(
  IN CHAR8  *OSName
  );

// OS_INFO_OS_VENDOR
typedef
VOID
(EFIAPI *OS_INFO_OS_VENDOR)(
  IN CHAR8  *OSVendor
  );

// OS_INFO_SET_VTD_ENABLED
typedef
VOID
(EFIAPI *OS_INFO_SET_VTD_ENABLED)(
  IN UINTN  *BootVTdEnabled
  );

// OS_INFO_GET_VTD_ENABLED
typedef
VOID
(EFIAPI *OS_INFO_GET_VTD_ENABLED)(
  OUT UINTN  *BootVTdEnabled
  );

// OS_INFO_FORCE_VTD_ENABLED
typedef
VOID
(EFIAPI *OS_INFO_FORCE_VTD_ENABLED)(
  IN UINTN  *BootVTdEnabled
  );

// EFI_OS_INFO_PROTOCOL
typedef struct {
  UINTN                        Revision;            ///< Revision.
  OS_INFO_OS_NAME              OSName;              ///< Present as of Revision 1.
  OS_INFO_OS_VENDOR            OSVendor;            ///< Present as of Revision 2.
  OS_INFO_SET_VTD_ENABLED      SetBootVTdEnabled;   ///< Present as of Revision 3.
  OS_INFO_GET_VTD_ENABLED      GetBootVTdEnabled;   ///< Present as of Revision 3.
  OS_INFO_FORCE_VTD_ENABLED    ForceBootVTdEnabled; ///< Present as of Revision 4.
} EFI_OS_INFO_PROTOCOL;

// gEfiOSInfoProtocolGuid
extern EFI_GUID  gEfiOSInfoProtocolGuid;

#endif // EFI_OS_INFO_H
