/** @file
  GUIDs used for Bios ID.

Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
Portions Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef BIOS_ID_H
#define BIOS_ID_H

// EFI_BIOS_ID_GUID
#define EFI_BIOS_ID_GUID         \
  { 0xC3E36D09, 0x8294, 0x4b97,  \
    { 0xA8, 0x57, 0xD5, 0x28, 0x8F, 0xE3, 0x3E, 0x28 } }

// APPLE_ROM_INFO_GUID
#define APPLE_ROM_INFO_GUID      \
  { 0xB535ABF6, 0x967D, 0x43F2,  \
    { 0xB4, 0x94, 0xA1, 0xEB, 0x8E, 0x21, 0xA2, 0x8E } }

//
// BIOS ID string format:
//
// $(BOARD_ID)$(BOARD_REV).$(OEM_ID).$(VERSION_MAJOR).$(BUILD_TYPE)$(VERSION_MINOR).YYMMDDHHMM
//
// Example: "TRFTCRB1.86C.0008.D03.0506081529"
//
#pragma pack(1)

typedef PACKED struct {
  CHAR16  BoardId[7];               // "TRFTCRB"
  CHAR16  BoardRev;                 // "1"
  CHAR16  Dot1;                     // "."
  CHAR16  OemId[3];                 // "86C"
  CHAR16  Dot2;                     // "."
  CHAR16  VersionMajor[4];          // "0008"
  CHAR16  Dot3;                     // "."
  CHAR16  BuildType;                // "D"
  CHAR16  VersionMinor[2];          // "03"
  CHAR16  Dot4;                     // "."
  CHAR16  TimeStamp[10];            // "YYMMDDHHMM"
  CHAR16  NullTerminator;           // 0x0000
} BIOS_ID_STRING;

#define MEM_IFWIVER_START           0x7E0000
#define MEM_IFWIVER_LENGTH          0x1000

typedef PACKED struct _MANIFEST_OEM_DATA{
  UINT32  Signature;
  UINT8   FillNull[0x39];
  UINT32  IFWIVersionLen;
  UINT8   IFWIVersion[32];
}MANIFEST_OEM_DATA;

//
// A signature precedes the BIOS ID string in the FV to enable search by external tools.
//
typedef PACKED struct {
  UINT8           Signature[8];     // "$IBIOSI$"
  BIOS_ID_STRING  BiosIdString;     // "TRFTCRB1.86C.0008.D03.0506081529"
} BIOS_ID_IMAGE;

#pragma pack()

// APPLE_ROM_INFO_STRING
typedef CHAR8 *APPLE_ROM_INFO_STRING;

// gEfiBiosIdGuid
extern EFI_GUID gEfiBiosIdGuid;

// gAppleRomInfoGuid
extern EFI_GUID gAppleRomInfoGuid;

#endif // BIOS_ID_H
