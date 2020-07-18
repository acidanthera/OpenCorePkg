/** @file
Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_COMPRESSED_BINARY_IMAGE_H
#define APPLE_COMPRESSED_BINARY_IMAGE_H

#include <IndustryStandard/AppleMachoImage.h>

///
/// The common 'comp' number used to identify a compressed binary.
///
#define MACH_COMPRESSED_BINARY_INVERT_SIGNATURE  0x706D6F63
///
/// The common 'lzvn' number used to identify a compressed binary with LZVN.
///
#define MACH_COMPRESSED_BINARY_INVERT_LZVN       0x6E767A6C
///
/// The common 'lzss' number used to identify a compressed binary with LZSS.
///
#define MACH_COMPRESSED_BINARY_INVERT_LZSS       0x73737A6C

///
/// Definition of the the compressed header structure.
///
typedef struct {
  ///
  /// The assumed 'comp' type identification number found in the file.
  ///
  UINT32  Signature;
  ///
  /// The assumed 'lzvn' or 'lzss' compression identification number found in the file.
  ///
  UINT32  Compression;
  ///
  /// Adler32 hash of the decompressed data.
  ///
  UINT32  Hash;
  ///
  /// Decompressed data size.
  ///
  UINT32  Decompressed;
  ///
  /// Compressed data size.
  ///
  UINT32  Compressed;
  ///
  /// Compression file version.
  ///
  UINT32  Version;
  ///
  /// All zero padding.
  ///
  UINT32  Padding[90];
} MACH_COMP_HEADER;

#endif // APPLE_COMPRESSED_BINARY_IMAGE_H
