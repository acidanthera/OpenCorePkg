/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef EFI_OS_INFO_H
#define EFI_OS_INFO_H

// EFI_OS_INFO_PROTOCOL_REVISION
#define EFI_OS_INFO_PROTOCOL_REVISION  0x00000003

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
  IN CHAR8  *OSName
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

// EFI_OS_INFO_PROTOCOL
typedef struct {
  UINTN                   Revision;           ///< Revision.
  OS_INFO_OS_NAME         OSName;             ///< Present as of Revision 1.
  OS_INFO_OS_VENDOR       OSVendor;           ///< Present as of Revision 2.
  OS_INFO_SET_VTD_ENABLED SetBootVTdEnabled;  ///< Present as of Revision 3.
  OS_INFO_GET_VTD_ENABLED GetBootVTdEnabled;  ///< Present as of Revision 3.
} EFI_OS_INFO_PROTOCOL;

// gEfiOSInfoProtocolGuid
extern EFI_GUID gEfiOSInfoProtocolGuid;

#endif // EFI_OS_INFO_H
