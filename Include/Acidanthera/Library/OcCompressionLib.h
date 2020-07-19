/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_COMPRESSION_LIB_H
#define OC_COMPRESSION_LIB_H

/**
  Maximumum compression and decompression buffer size may vary from
  0 to OC_COMPRESSION_MAX_LENGTH inclusive.
**/
#define OC_COMPRESSION_MAX_LENGTH BASE_1GB

/**
  Allow the use of extra adler32 validation.
  Not very useful as dmg has own checks.
**/
// #define OC_INFLATE_VERIFY_DATA

/**
  Compress buffer with LZSS algorithm.

  @param[out]  Dst         Destination buffer.
  @param[in]   DstLen      Destination buffer size.
  @param[in]   Src         Source buffer.
  @param[in]   SrcLen      Source buffer size.

  @return  Dst + CompressedLen on success otherwise NULL.
**/
UINT8 *
CompressLZSS (
  OUT UINT8   *Dst,
  IN  UINT32  DstLen,
  IN  UINT8   *Src,
  IN  UINT32  SrcLen
  );

/**
  Decompress buffer with LZSS algorithm.

  @param[out]  Dst         Destination buffer.
  @param[in]   DstLen      Destination buffer size.
  @param[in]   Src         Source buffer.
  @param[in]   SrcLen      Source buffer size.

  @return  DecompressedLen on success otherwise 0.
**/
UINT32
DecompressLZSS (
  OUT UINT8   *Dst,
  IN  UINT32  DstLen,
  IN  UINT8   *Src,
  IN  UINT32  SrcLen
  );

/**
  Decompress buffer with LZVN algorithm.

  @param[out]  Dst         Destination buffer.
  @param[in]   DstLen      Destination buffer size.
  @param[in]   Src         Source buffer.
  @param[in]   SrcLen      Source buffer size.

  @return  DecompressedLen on success otherwise 0.
**/
UINTN
DecompressLZVN (
  OUT UINT8        *Dst,
  IN  UINTN        DstLen,
  IN  CONST UINT8  *Src,
  IN  UINTN        SrcLen
  );

/**
  Compress buffer with ZLIB algorithm.

  @param[out]  Dst         Destination buffer.
  @param[in]   DstLen      Destination buffer size.
  @param[in]   Src         Source buffer.
  @param[in]   SrcLen      Source buffer size.

  @return  Dst + CompressedLen on success otherwise NULL.
**/
UINT8 *
CompressZLIB (
  OUT UINT8         *Dst,
  IN  UINT32        DstLen,
  IN  CONST UINT8   *Src,
  IN  UINT32        SrcLen
  );

/**
  Decompress buffer with ZLIB algorithm.

  @param[out]  Dst         Destination buffer.
  @param[in]   DstLen      Destination buffer size.
  @param[in]   Src         Source buffer.
  @param[in]   SrcLen      Source buffer size.

  @return  DecompressedLen on success otherwise 0.
**/
UINTN
DecompressZLIB (
  OUT UINT8        *Dst,
  IN  UINTN        DstLen,
  IN  CONST UINT8  *Src,
  IN  UINTN        SrcLen
  );

/**
  Decompress buffer with RLE24 algorithm and 8-bit alpha.
  This algorithm is used for encoding IT32/T8MK images in ICNS.

  @param[out]  Dst         Destination buffer.
  @param[in]   DstLen      Destination buffer size.
  @param[in]   Src         Source buffer.
  @param[in]   SrcLen      Source buffer size.
  @param[in]   Mask        Source buffer.
  @param[in]   MaskLen     Source buffer size.
  @param[in]   Premultiply Multiply source channels by alpha.

  @return  DecompressedLen on success otherwise 0.
**/
UINT32
DecompressMaskedRLE24 (
  OUT UINT8   *Dst,
  IN  UINT32  DstLen,
  IN  UINT8   *Src,
  IN  UINT32  SrcLen,
  IN  UINT8   *Mask,
  IN  UINT32  MaskLen,
  IN  BOOLEAN Premultiply
  );

/**
  Calculates Adler32 checksum.
  @param[in]   Buffer         Source buffer.
  @param[in]   BufferLen      Source buffer size.
  @return  Checksum on success otherwise 0.
**/
UINT32
Adler32 (
  IN CONST UINT8  *Buffer,
  IN UINT32       BufferLen
  );

#endif // OC_COMPRESSION_LIB_H
