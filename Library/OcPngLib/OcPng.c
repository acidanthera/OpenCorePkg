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

  if (Error != 0) {
    DEBUG ((DEBUG_INFO, "OcPngLib: Error while getting image dimensions from PNG header\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Width = W;
  *Height = H;

  return EFI_SUCCESS;
}

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

  if (Error != 0) {
    DEBUG ((DEBUG_INFO, "OcPngLib: Error while decoding PNG image\n"));
    lodepng_state_cleanup (&State);
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
  unsigned          Error;

  //
  // Init lodepng state
  //
  lodepng_state_init (&State);

  State.info_raw.colortype = LCT_RGBA;
  State.info_raw.bitdepth = BitDepth;
  State.info_png.color.colortype = LCT_RGBA;
  State.info_png.color.bitdepth = BitDepth;

  //
  // It should return 0 on success
  //
  Error = lodepng_encode ((unsigned char **) Buffer, BufferSize, RawData, Width, Height, &State);

  if (Error != 0) {
    DEBUG ((DEBUG_INFO, "OcPngLib: Error while encoding PNG image\n"));
    lodepng_state_cleanup (&State);
    return EFI_INVALID_PARAMETER;
  }

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
