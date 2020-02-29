/** @file

OcPngLib - library with PNG decoder functions

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_PNG_LIB_H
#define OC_PNG_LIB_H

/**
  Retrieves PNG image dimensions

  @param  Buffer            Buffer with desired png image
  @param  Size              Size of input image
  @param  Width             Image width at output
  @param  Height            Image height at output

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to init state.
  @return EFI_INVALID_PARAMETER Passed wrong parameter
**/
EFI_STATUS
GetPngDims (
  IN  VOID     *Buffer,
  IN  UINTN    Size,
  OUT UINT32   *Width,
  OUT UINT32   *Height
  );

/**
  Decodes PNG image into raw pixel buffer

  @param  Buffer                 Buffer with desired png image
  @param  Size                   Size of input image
  @param  RawData                Output buffer with raw data
  @param  Width                  Image width at output
  @param  Height                 Image height at output
  @param  HasAlphaType           Returns 1 if alpha layer present, optional param
                                 Set NULL, if not used

  @return EFI_SUCCESS            The function completed successfully.
  @return EFI_OUT_OF_RESOURCES   There are not enough resources to init state.
  @return EFI_INVALID_PARAMETER  Passed wrong parameter
**/
EFI_STATUS
DecodePng (
  IN   VOID     *Buffer,
  IN   UINTN    Size,
  OUT  VOID     **RawData,
  OUT  UINT32   *Width,
  OUT  UINT32   *Height,
  OUT  BOOLEAN  *HasAlphaType OPTIONAL
  );

/**
  Encodes raw pixel buffer into PNG image data

  @param  RawData               RawData from png image
  @param  Width                 Image width
  @param  Height                Image height
  @param  Buffer                Output buffer
  @param  BufferSize            Output size

  @return EFI_SUCCESS  The function completed successfully.
  @return EFI_INVALID_PARAMETER  Passed wrong parameter
**/
EFI_STATUS
EncodePng (
  IN  VOID    *RawData,
  IN  UINT32  Width,
  IN  UINT32  Height,
  OUT VOID    **Buffer,
  OUT UINTN   *BufferSize
  );

#endif
