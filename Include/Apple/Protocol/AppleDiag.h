/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DIAG_H
#define APPLE_DIAG_H

#define APPLE_DIAG_VARIABLE_ACCESS_PROTOCOL_GUID  \
  { 0xC5CFF4F1, 0x379A, 0x54E1,                   \
    { 0x9E, 0xDD, 0x93, 0x21, 0x9C, 0x6A, 0xA4, 0xFE } }

#define APPLE_DIAG_VAULT_PROTOCOL_GUID   \
  { 0xF76761DC, 0xFF89, 0x44E4,          \
    { 0x9C, 0x0C, 0xCD, 0x0A, 0xDA, 0x4E, 0xF9, 0x83 } }

typedef
EFI_STATUS
(EFIAPI *DIAG_ACCESS_GET)(
  IN     CHAR16     *Key,
     OUT VOID       *Value,
  IN OUT UINTN      *Length
  );

typedef
EFI_STATUS
(EFIAPI *DIAG_ACCESS_SET)(
  IN     CHAR16 *Key,
  IN     VOID   *Value,
  IN OUT UINTN  *Length
  );

typedef struct {
  UINTN            Revision;
  DIAG_ACCESS_GET  GetValue;
  DIAG_ACCESS_SET  SetValue;
} APPLE_DIAG_VARIABLE_ACCESS_PROTOCOL;

// Note, both these GUIDs are also exposed as a configuration table with the same GUID and is used by AppleEFINVRAM.kext.
extern EFI_GUID gAppleDiagVariableAccessProtocolGuid;
extern EFI_GUID gAppleDiagVaultProtocolGuid;

#endif // APPLE_DIAG_ACCESS_H
