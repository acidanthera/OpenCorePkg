/** @file
  Library for performing UEFI GOP Blt operations on a framebuffer.

  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OC_BLIT_LIB
#define OC_BLIT_LIB

#include <Protocol/GraphicsOutput.h>
#include <Library/OcGuardLib.h>

//
// Limit bytes per pixel to most common value for simplicity.
//
#define BYTES_PER_PIXEL sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
STATIC_ASSERT (BYTES_PER_PIXEL == sizeof (UINT32), "Non 4-byte pixels are unsupported!");

//
// Read-only structure for the frame buffer configure.
//
typedef struct OC_BLIT_CONFIGURE {
  UINT32                          PixelsPerScanLine;
  UINT32                          Width;
  UINT32                          Height;
  UINT32                          RotatedWidth;
  UINT32                          RotatedHeight;
  UINT32                          Rotation;
  UINT8                           *FrameBuffer;
  EFI_GRAPHICS_PIXEL_FORMAT       PixelFormat;
  EFI_PIXEL_BITMASK               PixelMasks;
  INT8                            PixelShl[4]; // R-G-B-Rsvd
  INT8                            PixelShr[4]; // R-G-B-Rsvd
  OC_ALIGNAS(64) UINT8            LineBuffer[];
} OC_BLIT_CONFIGURE;

/**
  Create the configuration for a video frame buffer.

  The configuration is returned in the caller provided buffer.

  @param[in] FrameBuffer       Pointer to the start of the frame buffer.
  @param[in] FrameBufferInfo   Describes the frame buffer characteristics.
                               The information in FramebufferInfo must refer to unmodified
                               (not rotated) framebuffer.
  @param[in] Rotation          Rotation scheme in degrees (must be one of 0, 90, 180, 270).
  @param[in,out] Configure     The created configuration information.
  @param[in,out] ConfigureSize Size of the configuration information.

  @retval RETURN_SUCCESS            The configuration was successful created.
  @retval RETURN_BUFFER_TOO_SMALL   The Configure is to too small. The required
                                    size is returned in ConfigureSize.
  @retval RETURN_UNSUPPORTED        The requested mode is not supported by
                                    this implementaion.
**/
RETURN_STATUS
EFIAPI
OcBlitConfigure (
  IN      VOID                                  *FrameBuffer,
  IN      EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *FrameBufferInfo,
  IN      UINT32                                Rotation,
  IN OUT  OC_BLIT_CONFIGURE                     *Configure,
  IN OUT  UINTN                                 *ConfigureSize
  );

/**
  Performs a UEFI Graphics Output Protocol Blt operation.

  @param[in]     Configure    Pointer to a configuration which was successfully
                              created by FrameBufferBltConfigure ().
  @param[in,out] BltBuffer    The data to transfer to screen.
  @param[in]     BltOperation The operation to perform.
  @param[in]     SourceX      The X coordinate of the source for BltOperation.
  @param[in]     SourceY      The Y coordinate of the source for BltOperation.
  @param[in]     DestinationX The X coordinate of the destination for
                              BltOperation.
  @param[in]     DestinationY The Y coordinate of the destination for
                              BltOperation.
  @param[in]     Width        The width of a rectangle in the blt rectangle
                              in pixels.
  @param[in]     Height       The height of a rectangle in the blt rectangle
                              in pixels.
  @param[in]     Delta        Not used for EfiBltVideoFill and
                              EfiBltVideoToVideo operation. If a Delta of 0
                              is used, the entire BltBuffer will be operated
                              on. If a subrectangle of the BltBuffer is
                              used, then Delta represents the number of
                              bytes in a row of the BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
EFIAPI
OcBlitRender (
  IN     OC_BLIT_CONFIGURE                  *Configure,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer OPTIONAL,
  IN     EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN     UINTN                              SourceX,
  IN     UINTN                              SourceY,
  IN     UINTN                              DestinationX,
  IN     UINTN                              DestinationY,
  IN     UINTN                              Width,
  IN     UINTN                              Height,
  IN     UINTN                              Delta
  );

#endif // OC_BLIT_LIB
