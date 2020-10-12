/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FILE_H
#define APPLE_FILE_H

//
// ASCII content stored in legacy firmware.
//
#define APPLE_FIRMWARE_INFO_FILE_GUID  \
  { 0x95C8C131, 0x4467, 0x4447,        \
    { 0x8A, 0x71, 0xF0, 0x87, 0xAF, 0xCA, 0x07, 0xA5 } }

// gAppleFirmwareInfoFileGuid
extern EFI_GUID gAppleFirmwareInfoFileGuid;

#define APPLE_SLING_SHOT_FILE_GUID  \
  { 0xD5B366C7, 0xDB85, 0x455F,   \
    { 0xB5, 0x0B, 0x90, 0x0A, 0x69, 0x4E, 0x4C, 0x8C } }

extern EFI_GUID gAppleSlingShotFileGuid;

#define APPLE_BOOT_PICKER_FILE_GUID  \
  { 0xE1628C66, 0x2A2D, 0x4DC5,      \
    { 0xBD, 0x41, 0xB2, 0x0F, 0x35, 0x38, 0xAA, 0xF7 } }

extern EFI_GUID gAppleBootPickerFileGuid;

#define APPLE_PASSWORD_UI_FILE_GUID  \
  { 0x9EBA2D25, 0xBBE3, 0x4AC2,      \
    { 0xA2, 0xC6, 0xC8, 0x7F, 0x44, 0xA1, 0x27, 0x8C } }

extern EFI_GUID gApplePasswordUIFileGuid;

#define APPLE_UTDMUI_APP_FILE_GUID  \
  { 0xD3231048, 0xB7D7, 0x46FC,     \
    { 0x80, 0xF8, 0x2F, 0x7B, 0x22, 0x95, 0x86, 0xC5 } }

extern EFI_GUID gAppleUTDMUIAppFileGuid;

#define APPLE_LEGACY_LOAD_APP_FILE_GUID  \
  { 0x2B0585EB, 0xD8B8, 0x49A9,          \
    { 0x8B, 0x8C, 0xE2, 0x1B, 0x01, 0xAE, 0xF2, 0xB7 } }

extern EFI_GUID gAppleLegacyLoadAppFileGuid;

///
/// 05984E1A-D8BB-5D8A-A8E6-90E6FB2AB7DA
///
#define APPLE_ALERT_UI_FILE_GUID \
  { 0x05984E1A, 0xD8BB, 0x5D8A,  \
    { 0xA8, 0xE6, 0x90, 0xE6, 0xFB, 0x2A, 0xB7, 0xDA } }

extern EFI_GUID gAppleAlertUiFileGuid;

///
/// 4CF484CD-135F-4FDC-BAFB-1AA104B48D36
///
#define APPLE_HFS_PLUS_DXE_FILE_GUID  \
  { 0x4CF484CD, 0x135F, 0x4FDC,   \
    { 0xBA, 0xFB, 0x1A, 0xA1, 0x04, 0xB4, 0x8D, 0x36 } }

extern EFI_GUID gAppleHfsPlusDxeFileGuid;

///
/// AE4C11C8-1D6C-F24E-A183-E1CA36D1A8A9
///
#define APPLE_HFS_PLUS_FILE_GUID  \
  { 0xAE4C11C8, 0x1D6C, 0xF24E,   \
    { 0xA1, 0x83, 0xE1, 0xCA, 0x36, 0xD1, 0xA8, 0xA9 } }

extern EFI_GUID gAppleHfsPlusFileGuid;

///
/// 44883EC1-C77C-1749-B73D-30C7B468B556
///
#define APPLE_EX_FAT_DXE_FILE_GUID \
  { 0x44883EC1, 0xC77C, 0x1749,    \
    { 0xB7, 0x3D, 0x30, 0xC7, 0xB4, 0x68, 0xB5, 0x56 } }

extern EFI_GUID gAppleExFatDxeFileGuid;

///
/// 3730EC36-868D-4DF6-88CF-30B791272F5C
///
#define APPLE_APFS_FILE_GUID    \
  { 0x3730EC36, 0x868D, 0x4DF6, \
    { 0x88, 0xCF, 0x30, 0xB7, 0x91, 0x27, 0x2F, 0x5C } }

extern EFI_GUID gAppleApfsFileGuid;


#endif // APPLE_FILE_H
