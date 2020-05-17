/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_BLESS_H
#define APPLE_BLESS_H

//
// These GUIDs allow to retrieve parsed finderInfo fields out of HFSPlusVolumeHeader
// via HFSPlus.efi driver. The fields are reads from the following structure in Big Endian:
//
// struct HFSPlusVolumeFinderInfo {
//   uint32_t blessedSystemFolderID; // for OpenFirmware systems
//   uint32_t blessedSystemFileID;   // for EFI systems
//   uint32_t openWindowFolderID;    // deprecated, first link in linked list of folders to open at mount
//   uint32_t blessedAlternateOSID;  // currently used for FV2 recovery, inaccessible from UEFI
//   uint32_t unused;                // formerly PowerTalk Inbox
//   uint32_t blessedOSXFolderID;    // currently used for normal recovery
//   uint64_t volumeID;
// };
//
// References:
// https://opensource.apple.com/source/hfs/hfs-407.30.1/core/hfs_format.h.auto.html
// https://opensource.apple.com/source/bless/bless-166/handleInfo.c.auto.html
// https://opensource.apple.com/source/bless/bless-166/libbless/FinderInfo/BLGetVolumeFinderInfo.c.auto.html
// https://opensource.apple.com/source/bless/bless-166/libbless/HFS/BLLookupFileIDOnMount.c.auto.html
//
// TODO: Implement this in an opensource HFSPlus EFI driver.
//

// APPLE_BLESSED_SYSTEM_FILE_INFO_GUID
/// InformationType GUID used to get the blessed file's path.
#define APPLE_BLESSED_SYSTEM_FILE_INFO_GUID  \
  { 0xCA7E4814, 0x2ADC, 0x4ADD,              \
    { 0xAB, 0xFF, 0x73, 0x4E, 0x3C, 0xFE, 0x13, 0xF3 } }

// APPLE_BLESSED_SYSTEM_FOLDER_INFO_GUID
/// InformationType GUID exposed used to get the blessed folder's path.
#define APPLE_BLESSED_SYSTEM_FOLDER_INFO_GUID  \
  { 0x7BD1F02D, 0x9C2F, 0x4581,                \
    { 0xBF, 0x12, 0xD5, 0x4a, 0xBA, 0x0D, 0x98, 0xD6 } }

// APPLE_BLESSED_OSX_FOLDER_INFO_GUID
/// InformationType GUID exposed used to get alternative blessed file or folder's path.
#define APPLE_BLESSED_OSX_FOLDER_INFO_GUID  \
  { 0x893CA450, 0x5F5E, 0x48BA,             \
    { 0x85, 0x8F, 0x08, 0xC4, 0x5D, 0x80, 0x23, 0x18 } }

// gAppleBlessedSystemFileInfoGuid
/// A global variable storing the GUID of the APPLE_BLESSED_SYSTEM_FILE_INFO_GUID.
extern EFI_GUID gAppleBlessedSystemFileInfoGuid;

// gAppleBlessedSystemFolderInfoGuid
/// A global variable storing the GUID of the APPLE_BLESSED_SYSTEM_FOLDER_INFO_GUID.
extern EFI_GUID gAppleBlessedSystemFolderInfoGuid;

// gAppleBlessedOsxFolderInfoGuid
/// A global variable storing the GUID of the APPLE_BLESSED_OSX_FOLDER_INFO_GUID.
extern EFI_GUID gAppleBlessedOsxFolderInfoGuid;

#endif // APPLE_BLESS_H
