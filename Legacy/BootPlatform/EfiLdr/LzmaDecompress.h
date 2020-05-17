/** @file
  LZMA Decompress Library header file

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LZMADECOMPRESS_H__
#define __LZMADECOMPRESS_H__

/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.GetInfo().
  
  @param Source           The source buffer containing the compressed data.
  @param SourceSize       The size of source buffer
  @param DestinationSize  The size of destination buffer.
  @param ScratchSize      The size of scratch buffer.

  @retval RETURN_SUCCESS           - The size of destination buffer and the size of scratch buffer are successfully retrieved.
  @retval RETURN_INVALID_PARAMETER - The source data is corrupted
**/
RETURN_STATUS
EFIAPI
LzmaUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

/**
  Decompresses a Lzma compressed source buffer.

  Extracts decompressed data to its original form.
  If the compressed source data specified by Source is successfully decompressed 
  into Destination, then RETURN_SUCCESS is returned.  If the compressed source data 
  specified by Source is not in a valid compressed data format,
  then RETURN_INVALID_PARAMETER is returned.

  @param  Source      The source buffer containing the compressed data.
  @param  SourceSize  The size of source buffer.
  @param  Destination The destination buffer to store the decompressed data
  @param  Scratch     A temporary scratch buffer that is used to perform the decompression.
                      This is an optional parameter that may be NULL if the 
                      required scratch buffer size is 0.
                     
  @retval  RETURN_SUCCESS Decompression completed successfully, and 
                          the uncompressed buffer is returned in Destination.
  @retval  RETURN_INVALID_PARAMETER 
                          The source buffer specified by Source is corrupted 
                          (not in a valid compressed format).
**/
RETURN_STATUS
EFIAPI
LzmaUefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );
  
#endif // __LZMADECOMPRESS_H__

