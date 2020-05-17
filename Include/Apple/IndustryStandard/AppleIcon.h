/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_ICON_H
#define APPLE_ICON_H

///
/// Header types.
///
#define APPLE_ICNS_MAGIC SIGNATURE_32 ('i', 'c', 'n', 's')

///
/// Record types.
///
#define APPLE_ICNS_IC07  SIGNATURE_32 ('i', 'c', '0', '7')
#define APPLE_ICNS_IC13  SIGNATURE_32 ('i', 'c', '1', '3')
#define APPLE_ICNS_IT32  SIGNATURE_32 ('i', 't', '3', '2')
#define APPLE_ICNS_T8MK  SIGNATURE_32 ('t', '8', 'm', 'k')

///
/// Normal disk icon dimension.
///
#define APPLE_DISK_ICON_DIMENSION 128U

#pragma pack(push, 1)

///
/// Apple .icns record, all fields are Big Endian.
///
typedef struct APPLE_ICNS_RECORD_ {
  UINT32  Type;
  UINT32  Size;
  UINT8   Data[];
} APPLE_ICNS_RECORD;

#pragma pack(pop)

#endif // APPLE_ICON_H
