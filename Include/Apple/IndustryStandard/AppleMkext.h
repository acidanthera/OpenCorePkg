/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_MKEXT_H
#define APPLE_MKEXT_H

#include <IndustryStandard/AppleMachoImage.h>

//
// Based upon XNU libkern/libkern/mkext.h.
//

//
// Magic value "MKXT" used to identify an MKEXT file.
//
#define MKEXT_MAGIC             0x4D4B5854
//
// Magic value "MKXT" used to identify an MKEXT file.
//
#define MKEXT_INVERT_MAGIC      0x54584B4D
//
// Signature "MOSX" used to identify an MKEXT file.
//
#define MKEXT_SIGNATURE         0x4D4F5358
//
// Signature "MOSX" used to identify an MKEXT file.
//
#define MKEXT_INVERT_SIGNATURE  0x58534F4D
//
// Version 1 format.
//
#define MKEXT_VERSION_V1        0x01008000
//
// Version 2 format.
//
#define MKEXT_VERSION_V2        0x02002001

//
// Common MKEXT header.
//
typedef struct {
  //
  // MKEXT magic value "MKXT".
  //
  UINT32           Magic;
  //
  // MKEXT signature "MOSX".
  //
  UINT32           Signature;
  //
  // File length.
  //
  UINT32           Length;
  //
  // Adler32 checksum from Version field to end of file.
  //
  UINT32           Adler32;
  //
  // File version.
  //
  UINT32           Version;
  //
  // Number of kernel extensions in file.
  //
  UINT32           NumKexts;
  //
  // The CPU architecture specifier.
  //
  MACH_CPU_TYPE    CpuType;
  //
  // The CPU sub-architecture specifier.
  //
  MACH_CPU_SUBTYPE CpuSubtype;
} MKEXT_CORE_HEADER;

//
//
// MKEXT v1 format.
// Version: 0x01008000
// 
// Kexts stored within an array as plist/binary pairs, with
// optional lzss compression of each plist and/or binary. V1 allows
// for fat kexts individually within the mkext or the entire mkext
// can be contained within a fat binary.
//
//

//
// MKEXT v1 kext file.
//
typedef struct {
  //
  // Offset into file where data begins.
  //
  UINT32  Offset;
  //
  // Compressed size. Zero indicates no compression.
  //
  UINT32  CompressedSize;
  //
  // Uncompressed size.
  //
  UINT32  FullSize;
  //
  // Last modified time.
  //
  UINT32  ModifiedSeconds;
} MKEXT_V1_KEXT_FILE;

//
// MKEXT v1 kext entry.
//
typedef struct {
  //
  // Info.plist entry.
  //
  MKEXT_V1_KEXT_FILE  Plist;
  //
  // Binary entry; optional.
  //
  MKEXT_V1_KEXT_FILE  Binary;
} MKEXT_V1_KEXT;

//
// MKEXT v1 header.
//
typedef struct {
  //
  // Common header.
  //
  MKEXT_CORE_HEADER   Header;
  //
  // Array of kext entries.
  //
  MKEXT_V1_KEXT       Kexts[];
} MKEXT_V1_HEADER;

//
//
// MKEXT v2 format. Introduced in XNU 10.0.
// Version: 0x02001000
// 
// Kext binaries and resources are stored together in sequence
// followed by a combined plist of all kexts at the end. This plist
// further contains offset information to respective binaries and resources.
// Zlib compression can be optionally used, with individual fat kexts
// diallowed, requiring all to be of the same single arch.
//
//

//
// MKEXT v2 file entry.
//
typedef struct {
  //
  // Compressed size. Zero indicates no compression.
  UINT32  CompressedSize;
  //
  // Uncompressed size.
  //
  UINT32  FullSize;
  //
  // Data.
  //
  UINT8   Data[];
} MKEXT_V2_FILE_ENTRY;

//
// MKEXT v2 header.
//
typedef struct {
  //
  // Common header.
  //
  MKEXT_CORE_HEADER   Header;
  //
  // Offset of plist in file.
  //
  UINT32              PlistOffset;
  //
  // Compressed size of plist. Zero indicates no compression.
  //
  UINT32              PlistCompressedSize;
  //
  // Uncompressed size of plist.
  //
  UINT32              PlistFullSize;
} MKEXT_V2_HEADER;

//
// Allow selecting correct header based on version.
//
typedef union {
  MKEXT_CORE_HEADER   Common;
  MKEXT_V1_HEADER     V1;
  MKEXT_V2_HEADER     V2;
} MKEXT_HEADER_ANY;

#endif // APPLE_MKEXT_H
