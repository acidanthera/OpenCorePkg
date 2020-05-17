/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_SMBIOS_LIB_H
#define APPLE_SMBIOS_LIB_H

// SmbiosInstallTables
EFI_STATUS
SmbiosInstallTables (
  VOID
  );

// SmbiosGetRecord
SMBIOS_STRUCTURE *
SmbiosGetRecord (
  IN EFI_SMBIOS_HANDLE  Handle
  );

// SmbiosAdd
VOID
SmbiosAdd (
  IN SMBIOS_STRUCTURE  *Record
  );

// SmbiosUpdateString
VOID
SmbiosUpdateString (
  IN EFI_SMBIOS_HANDLE  *Handle,
  IN UINTN              StringNumber,
  IN CHAR8              *String
  );

// SmbiosGetFirstHandle
SMBIOS_STRUCTURE *
SmbiosGetFirstHandle (
  IN     EFI_SMBIOS_TYPE    Type,
  IN OUT EFI_SMBIOS_HANDLE  *Handle
  );

#endif // APPLE_SMBIOS_LIB_H
