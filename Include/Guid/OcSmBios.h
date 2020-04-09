/** @file
  OpenCore custom SMBIOS GUID identifiers.

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_SMBIOS_H
#define OC_SMBIOS_H

///
/// This GUID is used for storing SMBIOS data when the firmware overwrites SMBIOS data at original
/// GUID at ExitBootServices, like it happens on some Dell computers.
/// Found by David Passmore. Guid matches syscl's implementation in Clover.
/// See: https://sourceforge.net/p/cloverefiboot/tickets/203/#c070
///
#define OC_CUSTOM_SMBIOS_TABLE_GUID  \
  { 0xEB9D2D35, 0x2D88, 0x11D3,      \
    { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D } }

///
/// For macOS it is currently irrelevant whether SMBIOSv3 GUID is used.
/// However, Windows tools (e.g. Dell Updater) may still use this GUID,
/// and therefore will see the custom SMBIOS we crafted.
/// This is undesired when using custom SMBIOS GUID namespace to make
/// SMBIOS specific to macOS.
///
#define OC_CUSTOM_SMBIOS3_TABLE_GUID \
  { 0xF2FD1545, 0x9794, 0x4A2C,      \
    { 0x99, 0x2E, 0xE5, 0xBB, 0xCF, 0x20, 0xE3, 0x94 } }

///
/// Exported GUID identifiers.
///
extern EFI_GUID       gOcCustomSmbiosTableGuid;
extern EFI_GUID       gOcCustomSmbios3TableGuid;

#endif // OC_SMBIOS_GUID_H
