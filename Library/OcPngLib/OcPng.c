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
#include <Library/OcPngLib.h>
#include "lodepng.h"

EFI_STATUS
OcGetPngDims (
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
  State.decoder.ignore_crc                  = TRUE;
  State.decoder.zlibsettings.ignore_adler32 = TRUE;
  State.decoder.zlibsettings.ignore_nlen    = TRUE;

  //
  // Reads header and resets other parameters in state->info_png
  //
  Error = lodepng_inspect (&W, &H, &State, Buffer, Size);

  lodepng_state_cleanup (&State);

  if (Error != 0) {
    DEBUG ((DEBUG_INFO, "OCPNG: Error while getting image dimensions from PNG header\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Width  = (UINT32) W;
  *Height = (UINT32) H;

  return EFI_SUCCESS;
}

EFI_STATUS
OcDecodePng (
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
  State.info_raw.colortype                  = LCT_RGBA;
  State.info_raw.bitdepth                   = 8;
  State.decoder.ignore_crc                  = TRUE;
  State.decoder.zlibsettings.ignore_adler32 = TRUE;
  State.decoder.zlibsettings.ignore_nlen    = TRUE;

  //
  // It should return 0 on success
  //
  Error = lodepng_decode ((unsigned char **) RawData, &W, &H, &State, Buffer, Size);

  if (Error != 0) {
    DEBUG ((DEBUG_INFO, "OCPNG: Error while decoding PNG image - %u\n", Error));
    lodepng_state_cleanup (&State);
    return EFI_INVALID_PARAMETER;
  }

  *Width  = (UINT32) W;
  *Height = (UINT32) H;

  if (HasAlphaType != NULL) {
    //
    // Check alpha layer existence
    //
    *HasAlphaType = lodepng_is_alpha_type (&State.info_png.color) != 0;
  }

  //
  // Cleanup state
  //
  lodepng_state_cleanup (&State);

  return EFI_SUCCESS;
}

EFI_STATUS
OcEncodePng (
  IN  VOID    *RawData,
  IN  UINT32  Width,
  IN  UINT32  Height,
  OUT VOID    **Buffer,
  OUT UINTN   *BufferSize
  )
{
  unsigned          Error;

  //
  // It should return 0 on success
  //
  Error = lodepng_encode32 ((unsigned char **) Buffer, BufferSize, RawData, Width, Height);

  if (Error != 0) {
    DEBUG ((DEBUG_INFO, "OCPNG: Error while encoding PNG image\n"));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
