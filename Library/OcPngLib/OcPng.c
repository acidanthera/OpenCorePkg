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
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include "lodepng.h"

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
  IN  VOID    *Buffer,
  IN  UINTN   Size,
  OUT UINT32  *Width,
  OUT UINT32  *Height
  )
{
  LodePNGState  State;
  unsigned      Error;
  unsigned      W;
  unsigned      H;

  //
  // Init state
  //
  lodepng_state_init (&State);

  //
  // Reads header and resets other parameters in state->info_png
  //
  Error = lodepng_inspect (&W, &H, &State, Buffer, Size);

  lodepng_state_cleanup (&State);

  if (Error) {
    DEBUG ((DEBUG_INFO, "OcPngLib: Error while getting image dimensions from PNG header\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Width = W;
  *Height = H;

  return EFI_SUCCESS;
}

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
  IN   VOID    *Buffer,
  IN   UINTN   Size,
  OUT  VOID    **RawData,
  OUT  UINT32  *Width,
  OUT  UINT32  *Height,
  OUT  BOOLEAN *HasAlphaType OPTIONAL
  )
{
  LodePNGState      State;
  unsigned          Error;
  unsigned          W;
  unsigned          H;

  //
  // Init lodepng state
  //
  lodepng_state_init (&State);

  //
  // It should return 0 on success
  //
  Error = lodepng_decode ((unsigned char **) RawData, &W, &H, &State, Buffer, Size);

  if (Error) {
    lodepng_state_cleanup (&State);
    DEBUG ((DEBUG_INFO, "OcPngLib: Error while decoding PNG image\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Width = W;
  *Height = H;

  if (HasAlphaType != NULL) {
    //
    // Check alpha layer existence
    //
    *HasAlphaType = lodepng_is_alpha_type (&State.info_png.color) == 1;
  }

  //
  // Cleanup state
  //
  lodepng_state_cleanup (&State);

  return EFI_SUCCESS;

}

/**
  Encodes raw pixel buffer into PNG image data

  @param  RawData            RawData from png image
  @param  Width                 Image width
  @param  Height               Image height
  @param  BitDepth          BitDept, 8 or 16
  @param  Buffer              Output buffer
  @param  BufferSize      Output size


  @return EFI_SUCCESS  The function completed successfully.
  @return EFI_INVALID_PARAMETER  Passed wrong parameter
**/
EFI_STATUS
EncodePng (
  IN  VOID    *RawData,
  IN  UINT32  Width,
  IN  UINT32  Height,
  IN  UINT8   BitDepth,
  OUT VOID    **Buffer,
  OUT UINTN   *BufferSize
  )
{
  LodePNGState      State;
  UINTN             Size;
  unsigned          Error;
  unsigned          W;
  unsigned          H;
  
  
  //
  // Init lodepng state
  //
  lodepng_state_init (&State);
  
  State.info_raw.colortype = LCT_RGBA;
  State.info_raw.bitdepth = BitDepth;
  State.info_png.color.colortype = LCT_RGBA;
  State.info_png.color.bitdepth = BitDepth;
  W = Width;
  H = Height;
  Size = 0;
  
  //
  // It should return 0 on success
  //
  Error = lodepng_encode ((unsigned char **) Buffer, &Size, RawData, W, H, &State);

  if (Error) {
    DEBUG ((DEBUG_INFO, "OcPngLib: Error while Encoding PNG image\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  *BufferSize = Size;
  
  //
  // Cleanup state
  //
  lodepng_state_cleanup (&State);

  return EFI_SUCCESS;

}

/**
  Frees image buffer

  @param  Buffer                 Buffer with desired png image
**/
VOID
FreePng (
  IN VOID  *Buffer
  )
{
  lodepng_free (Buffer);
}
