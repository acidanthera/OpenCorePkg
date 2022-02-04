/** @file
  OcBlitLib - Library to perform blt operations on a frame buffer.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BLIT_INTERNAL_H
#define BLIT_INTERNAL_H

#include <Library/OcBlitLib.h>

/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters at 0 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within BltBuffer.
  @param[in]  SourceY       Y location within BltBuffer.
  @param[in]  DestinationX  X location within video.
  @param[in]  DestinationY  Y location within video.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibBufferToVideo0 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters at -90 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within BltBuffer.
  @param[in]  SourceY       Y location within BltBuffer.
  @param[in]  DestinationX  X location within video.
  @param[in]  DestinationY  Y location within video.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibBufferToVideo90 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters at 180 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within BltBuffer.
  @param[in]  SourceY       Y location within BltBuffer.
  @param[in]  DestinationX  X location within video.
  @param[in]  DestinationY  Y location within video.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibBufferToVideo180 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters at 270 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within BltBuffer.
  @param[in]  SourceY       Y location within BltBuffer.
  @param[in]  DestinationX  X location within video.
  @param[in]  DestinationY  Y location within video.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibBufferToVideo270 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters at 0 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[out] BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within video.
  @param[in]  SourceY       Y location within video.
  @param[in]  DestinationX  X location within BltBuffer.
  @param[in]  DestinationY  Y location within BltBuffer.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibVideoToBuffer0 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters at 90 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[out] BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within video.
  @param[in]  SourceY       Y location within video.
  @param[in]  DestinationX  X location within BltBuffer.
  @param[in]  DestinationY  Y location within BltBuffer.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibVideoToBuffer90 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters at 180 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[out] BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within video.
  @param[in]  SourceY       Y location within video.
  @param[in]  DestinationX  X location within BltBuffer.
  @param[in]  DestinationY  Y location within BltBuffer.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibVideoToBuffer180 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  );

/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters at 270 degree rotation.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[out] BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within video.
  @param[in]  SourceY       Y location within video.
  @param[in]  DestinationX  X location within BltBuffer.
  @param[in]  DestinationY  Y location within BltBuffer.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  DeltaPixels   Number of pixels in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
BlitLibVideoToBuffer270 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  );

#endif // BLIT_INTERNAL_H
