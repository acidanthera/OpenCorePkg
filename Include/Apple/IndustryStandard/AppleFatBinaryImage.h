/** @file
Copyright (C) 2014 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FAT_BINARY_IMAGE_H
#define APPLE_FAT_BINARY_IMAGE_H

#include <IndustryStandard/AppleMachoImage.h>

///
/// The common "Fat Binary Magic" number used to identify a Fat binary.
///
#define EFI_FAT_BINARY_SIGNATURE             0x0EF1FAB9
///
/// The common "Fat Binary Magic" number used to identify a Fat binary.
///
#define MACH_FAT_BINARY_SIGNATURE            0xCAFEBABE
///
/// The common "Fat Binary Magic" number used to identify a Fat binary.
///
#define MACH_FAT_BINARY_INVERT_SIGNATURE     0xBEBAFECA
///
/// The common "Fat Binary Magic" number used to identify a 64-bit Fat binary.
///
#define MACH_FAT_BINARY_64_SIGNATURE         0xCAFEBABF
///
/// The common "Fat Binary Magic" number used to identify a 64-bit Fat binary.
///
#define MACH_FAT_BINARY_64_INVERT_SIGNATURE  0xBFBAFECA


///
/// Definition of the the Fat architecture-specific file header.
///
typedef struct {
  ///
  /// The found CPU architecture specifier.
  ///
  MACH_CPU_TYPE    CpuType;
  ///
  /// The found CPU sub-architecture specifier.
  ///
  MACH_CPU_SUBTYPE CpuSubtype;
  ///
  /// The offset of the architecture-specific boot file.
  ///
  UINT32           Offset;
  ///
  /// The size of the architecture-specific boot file.
  ///
  UINT32           Size;
  ///
  /// The alignment as a power of 2 (necessary for the x86_64 architecture).
  ///
  UINT32           Alignment;
} MACH_FAT_ARCH;

///
/// Definition of the Fat file header
///
typedef struct {
  ///
  /// The assumed "Fat Binary Magic" number found in the file.
  ///
  UINT32          Signature;
  ///
  /// The hard-coded number of architectures within the file.
  ///
  UINT32          NumberOfFatArch;
  ///
  /// The first APPLE_FAT_ARCH child of the FAT binary.
  ///
  MACH_FAT_ARCH  FatArch[];
} MACH_FAT_HEADER;


///
/// The support for the 64-bit fat file format described here is a work in
/// progress and not yet fully supported in all the Apple Developer Tools.
///
/// When a slice is greater than 4mb or an offset to a slice is greater than 4mb
/// then the 64-bit fat file format is used.
///

///
/// Definition of the the Fat architecture-specific file header.
///
typedef struct {
  ///
  /// The found CPU architecture specifier.
  ///
  MACH_CPU_TYPE    CpuType;
  ///
  /// The found CPU sub-architecture specifier.
  ///
  MACH_CPU_SUBTYPE CpuSubtype;
  ///
  /// The offset of the architecture-specific boot file.
  ///
  UINT64           Offset;
  ///
  /// The size of the architecture-specific boot file.
  ///
  UINT64           Size;
  ///
  /// The alignment as a power of 2 (necessary for the x86_64 architecture).
  ///
  UINT32           Alignment;
  ///
  /// Reserved.
  ///
  UINT32           Reserved;
} MACH_FAT_ARCH_64;

///
/// Definition of the Fat file header
///
typedef struct {
  ///
  /// The assumed "Fat Binary Magic" number found in the file.
  ///
  UINT32            Signature;
  ///
  /// The hard-coded number of architectures within the file.
  ///
  UINT32            NumberOfFatArch;
  ///
  /// The first APPLE_FAT_ARCH child of the FAT binary.
  ///
  MACH_FAT_ARCH_64  FatArch[];
} MACH_FAT_HEADER_64;

///
/// Allow selecting a correct header based on magic
///
typedef union {
  UINT32             Signature;
  MACH_FAT_HEADER    Header32;
  MACH_FAT_HEADER_64 Header64;
} MACH_FAT_HEADER_ANY;

#endif // APPLE_FAT_BINARY_IMAGE_H
