/** @file
  Support for Graphics output spliter.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "ConSplitter.h"

CHAR16  mCrLfString[3] = { CHAR_CARRIAGE_RETURN, CHAR_LINEFEED, CHAR_NULL };

/**
  Returns information for an available graphics mode that the graphics device
  and the set of active video output devices supports.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            The mode number to return information on.
  @param  SizeOfInfo            A pointer to the size, in bytes, of the Info buffer.
  @param  Info                  A pointer to callee allocated buffer that returns information about ModeNumber.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_BUFFER_TOO_SMALL  The Info buffer was too small.
  @retval EFI_DEVICE_ERROR      A hardware error occurred trying to retrieve the video mode.
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.
  @retval EFI_OUT_OF_RESOURCES  No resource available.

**/
EFI_STATUS
EFIAPI
ConSplitterGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_STATUS                      Status;
  UINTN                           Index;

  if ((This == NULL) || (Info == NULL) || (SizeOfInfo == NULL) || (ModeNumber >= This->Mode->MaxMode)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // retrieve private data
  //
  Private = GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  GraphicsOutput = NULL;

  if (Private->CurrentNumberOfGraphicsOutput == 1) {
    //
    // Find the only one GraphicsOutput.
    //
    for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
      GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
      if (GraphicsOutput != NULL) {
        break;
      }
    }
  }

  if (GraphicsOutput != NULL) {
    //
    // If only one physical GOP device exist, return its information.
    //
    Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32)ModeNumber, SizeOfInfo, Info);
    return Status;
  } else {
    //
    // If 2 more phyiscal GOP device exist or GOP protocol does not exist,
    // return GOP information (PixelFormat is PixelBltOnly) created in ConSplitterAddGraphicsOutputMode ().
    //
    *Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
    if (*Info == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    CopyMem (*Info, &Private->GraphicsOutputModeBuffer[ModeNumber], *SizeOfInfo);
  }

  return EFI_SUCCESS;
}

/**
  Set the video device into the specified mode and clears the visible portions of
  the output display to black.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            Abstraction that defines the current video mode.

  @retval EFI_SUCCESS           The graphics mode specified by ModeNumber was selected.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED       ModeNumber is not supported by this device.
  @retval EFI_OUT_OF_RESOURCES  No resource available.

**/
EFI_STATUS
EFIAPI
ConSplitterGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  EFI_STATUS                            Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA        *Private;
  UINTN                                 Index;
  EFI_STATUS                            ReturnStatus;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Mode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *PhysicalGraphicsOutput;
  UINTN                                 NumberIndex;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  EFI_UGA_DRAW_PROTOCOL                 *UgaDraw;

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  Private = GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);
  Mode    = &Private->GraphicsOutputModeBuffer[ModeNumber];

  ReturnStatus           = EFI_SUCCESS;
  GraphicsOutput         = NULL;
  PhysicalGraphicsOutput = NULL;
  //
  // return the worst status met
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
    if (GraphicsOutput != NULL) {
      PhysicalGraphicsOutput = GraphicsOutput;
      //
      // Find corresponding ModeNumber of this GraphicsOutput instance
      //
      for (NumberIndex = 0; NumberIndex < GraphicsOutput->Mode->MaxMode; NumberIndex++) {
        Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32)NumberIndex, &SizeOfInfo, &Info);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        if ((Info->HorizontalResolution == Mode->HorizontalResolution) && (Info->VerticalResolution == Mode->VerticalResolution)) {
          FreePool (Info);
          break;
        }

        FreePool (Info);
      }

      Status = GraphicsOutput->SetMode (GraphicsOutput, (UINT32)NumberIndex);
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      }
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      UgaDraw = Private->TextOutList[Index].UgaDraw;
      if (UgaDraw != NULL) {
        Status = UgaDraw->SetMode (
                            UgaDraw,
                            Mode->HorizontalResolution,
                            Mode->VerticalResolution,
                            32,
                            60
                            );
        if (EFI_ERROR (Status)) {
          ReturnStatus = Status;
        }
      }
    }
  }

  This->Mode->Mode = ModeNumber;

  if ((Private->CurrentNumberOfGraphicsOutput == 1) && (PhysicalGraphicsOutput != NULL)) {
    //
    // If only one physical GOP device exist, copy physical information to consplitter.
    //
    CopyMem (This->Mode->Info, PhysicalGraphicsOutput->Mode->Info, PhysicalGraphicsOutput->Mode->SizeOfInfo);
    This->Mode->SizeOfInfo      = PhysicalGraphicsOutput->Mode->SizeOfInfo;
    This->Mode->FrameBufferBase = PhysicalGraphicsOutput->Mode->FrameBufferBase;
    This->Mode->FrameBufferSize = PhysicalGraphicsOutput->Mode->FrameBufferSize;
  } else {
    //
    // If 2 more phyiscal GOP device exist or GOP protocol does not exist,
    // return GOP information (PixelFormat is PixelBltOnly) created in ConSplitterAddGraphicsOutputMode ().
    //
    CopyMem (This->Mode->Info, &Private->GraphicsOutputModeBuffer[ModeNumber], This->Mode->SizeOfInfo);
  }

  return ReturnStatus;
}

/**
  The following table defines actions for BltOperations.

  EfiBltVideoFill - Write data from the  BltBuffer pixel (SourceX, SourceY)
  directly to every pixel of the video display rectangle
  (DestinationX, DestinationY)
  (DestinationX + Width, DestinationY + Height).
  Only one pixel will be used from the BltBuffer. Delta is NOT used.
  EfiBltVideoToBltBuffer - Read data from the video display rectangle
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
  the BltBuffer rectangle (DestinationX, DestinationY )
  (DestinationX + Width, DestinationY + Height). If DestinationX or
  DestinationY is not zero then Delta must be set to the length in bytes
  of a row in the BltBuffer.
  EfiBltBufferToVideo - Write data from the  BltBuffer rectangle
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
  video display rectangle (DestinationX, DestinationY)
  (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
  not zero then Delta must be set to the length in bytes of a row in the
  BltBuffer.
  EfiBltVideoToVideo - Copy from the video display rectangle
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) .
  to the video display rectangle (DestinationX, DestinationY)
  (DestinationX + Width, DestinationY + Height).
  The BltBuffer and Delta  are not used in this mode.

  @param  This                    Protocol instance pointer.
  @param  BltBuffer               Buffer containing data to blit into video buffer.
                                  This buffer has a size of
                                  Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
  @param  BltOperation            Operation to perform on BlitBuffer and video
                                  memory
  @param  SourceX                 X coordinate of source for the BltBuffer.
  @param  SourceY                 Y coordinate of source for the BltBuffer.
  @param  DestinationX            X coordinate of destination for the BltBuffer.
  @param  DestinationY            Y coordinate of destination for the BltBuffer.
  @param  Width                   Width of rectangle in BltBuffer in pixels.
  @param  Height                  Hight of rectangle in BltBuffer in pixels.
  @param  Delta                   OPTIONAL.

  @retval EFI_SUCCESS             The Blt operation completed.
  @retval EFI_INVALID_PARAMETER   BltOperation is not valid.
  @retval EFI_DEVICE_ERROR        A hardware error occurred writting to the video
                                  buffer.

**/
EFI_STATUS
EFIAPI
ConSplitterGraphicsOutputBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      ReturnStatus;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL           *UgaDraw;

  if ((This == NULL) || (((UINTN)BltOperation) >= EfiGraphicsOutputBltOperationMax)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  ReturnStatus = EFI_SUCCESS;

  //
  // return the worst status met
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                                 GraphicsOutput,
                                 BltBuffer,
                                 BltOperation,
                                 SourceX,
                                 SourceY,
                                 DestinationX,
                                 DestinationY,
                                 Width,
                                 Height,
                                 Delta
                                 );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      } else if (BltOperation == EfiBltVideoToBltBuffer) {
        //
        // Only need to read the data into buffer one time
        //
        return EFI_SUCCESS;
      }
    }

    UgaDraw = Private->TextOutList[Index].UgaDraw;
    if ((UgaDraw != NULL) && FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *)BltBuffer,
                          (EFI_UGA_BLT_OPERATION)BltOperation,
                          SourceX,
                          SourceY,
                          DestinationX,
                          DestinationY,
                          Width,
                          Height,
                          Delta
                          );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      } else if (BltOperation == EfiBltVideoToBltBuffer) {
        //
        // Only need to read the data into buffer one time
        //
        return EFI_SUCCESS;
      }
    }
  }

  return ReturnStatus;
}

/**
  Return the current video mode information.

  @param  This                  The EFI_UGA_DRAW_PROTOCOL instance.
  @param  HorizontalResolution  The size of video screen in pixels in the X dimension.
  @param  VerticalResolution    The size of video screen in pixels in the Y dimension.
  @param  ColorDepth            Number of bits per pixel, currently defined to be 32.
  @param  RefreshRate           The refresh rate of the monitor in Hertz.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_NOT_STARTED       Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
ConSplitterUgaDrawGetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  OUT UINT32                 *HorizontalResolution,
  OUT UINT32                 *VerticalResolution,
  OUT UINT32                 *ColorDepth,
  OUT UINT32                 *RefreshRate
  )
{
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;

  if ((HorizontalResolution == NULL) ||
      (VerticalResolution   == NULL) ||
      (RefreshRate          == NULL) ||
      (ColorDepth           == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // retrieve private data
  //
  Private = UGA_DRAW_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  *HorizontalResolution = Private->UgaHorizontalResolution;
  *VerticalResolution   = Private->UgaVerticalResolution;
  *ColorDepth           = Private->UgaColorDepth;
  *RefreshRate          = Private->UgaRefreshRate;

  return EFI_SUCCESS;
}

/**
  Set the current video mode information.

  @param  This                 The EFI_UGA_DRAW_PROTOCOL instance.
  @param  HorizontalResolution The size of video screen in pixels in the X dimension.
  @param  VerticalResolution   The size of video screen in pixels in the Y dimension.
  @param  ColorDepth           Number of bits per pixel, currently defined to be 32.
  @param  RefreshRate          The refresh rate of the monitor in Hertz.

  @retval EFI_SUCCESS          Mode information returned.
  @retval EFI_NOT_STARTED      Video display is not initialized. Call SetMode ()
  @retval EFI_OUT_OF_RESOURCES Out of resources.

**/
EFI_STATUS
EFIAPI
ConSplitterUgaDrawSetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN UINT32                  HorizontalResolution,
  IN UINT32                  VerticalResolution,
  IN UINT32                  ColorDepth,
  IN UINT32                  RefreshRate
  )
{
  EFI_STATUS                            Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA        *Private;
  UINTN                                 Index;
  EFI_STATUS                            ReturnStatus;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  UINTN                                 NumberIndex;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  EFI_UGA_DRAW_PROTOCOL                 *UgaDraw;

  Private = UGA_DRAW_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  ReturnStatus = EFI_SUCCESS;

  //
  // Update the Mode data
  //
  Private->UgaHorizontalResolution = HorizontalResolution;
  Private->UgaVerticalResolution   = VerticalResolution;
  Private->UgaColorDepth           = ColorDepth;
  Private->UgaRefreshRate          = RefreshRate;

  //
  // return the worst status met
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
    if (GraphicsOutput != NULL) {
      //
      // Find corresponding ModeNumber of this GraphicsOutput instance
      //
      for (NumberIndex = 0; NumberIndex < GraphicsOutput->Mode->MaxMode; NumberIndex++) {
        Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32)NumberIndex, &SizeOfInfo, &Info);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        if ((Info->HorizontalResolution == HorizontalResolution) && (Info->VerticalResolution == VerticalResolution)) {
          FreePool (Info);
          break;
        }

        FreePool (Info);
      }

      Status = GraphicsOutput->SetMode (GraphicsOutput, (UINT32)NumberIndex);
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      }
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      UgaDraw = Private->TextOutList[Index].UgaDraw;
      if (UgaDraw != NULL) {
        Status = UgaDraw->SetMode (
                            UgaDraw,
                            HorizontalResolution,
                            VerticalResolution,
                            ColorDepth,
                            RefreshRate
                            );
        if (EFI_ERROR (Status)) {
          ReturnStatus = Status;
        }
      }
    }
  }

  return ReturnStatus;
}

/**
  Blt a rectangle of pixels on the graphics screen.

  The following table defines actions for BltOperations.

  EfiUgaVideoFill:
    Write data from the  BltBuffer pixel (SourceX, SourceY)
    directly to every pixel of the video display rectangle
    (DestinationX, DestinationY)
    (DestinationX + Width, DestinationY + Height).
    Only one pixel will be used from the BltBuffer. Delta is NOT used.
  EfiUgaVideoToBltBuffer:
    Read data from the video display rectangle
    (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
    the BltBuffer rectangle (DestinationX, DestinationY )
    (DestinationX + Width, DestinationY + Height). If DestinationX or
    DestinationY is not zero then Delta must be set to the length in bytes
    of a row in the BltBuffer.
  EfiUgaBltBufferToVideo:
    Write data from the  BltBuffer rectangle
    (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
    video display rectangle (DestinationX, DestinationY)
    (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
    not zero then Delta must be set to the length in bytes of a row in the
    BltBuffer.
  EfiUgaVideoToVideo:
    Copy from the video display rectangle
    (SourceX, SourceY) (SourceX + Width, SourceY + Height) .
    to the video display rectangle (DestinationX, DestinationY)
    (DestinationX + Width, DestinationY + Height).
    The BltBuffer and Delta  are not used in this mode.

  @param  This           Protocol instance pointer.
  @param  BltBuffer      Buffer containing data to blit into video buffer. This
                         buffer has a size of Width*Height*sizeof(EFI_UGA_PIXEL)
  @param  BltOperation   Operation to perform on BlitBuffer and video memory
  @param  SourceX        X coordinate of source for the BltBuffer.
  @param  SourceY        Y coordinate of source for the BltBuffer.
  @param  DestinationX   X coordinate of destination for the BltBuffer.
  @param  DestinationY   Y coordinate of destination for the BltBuffer.
  @param  Width          Width of rectangle in BltBuffer in pixels.
  @param  Height         Hight of rectangle in BltBuffer in pixels.
  @param  Delta          OPTIONAL

  @retval EFI_SUCCESS            The Blt operation completed.
  @retval EFI_INVALID_PARAMETER  BltOperation is not valid.
  @retval EFI_DEVICE_ERROR       A hardware error occurred writting to the video buffer.

**/
EFI_STATUS
EFIAPI
ConSplitterUgaDrawBlt (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  EFI_UGA_PIXEL          *BltBuffer  OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION  BltOperation,
  IN  UINTN                  SourceX,
  IN  UINTN                  SourceY,
  IN  UINTN                  DestinationX,
  IN  UINTN                  DestinationY,
  IN  UINTN                  Width,
  IN  UINTN                  Height,
  IN  UINTN                  Delta         OPTIONAL
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;

  Private = UGA_DRAW_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  ReturnStatus = EFI_SUCCESS;
  //
  // return the worst status met
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                                 GraphicsOutput,
                                 (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BltBuffer,
                                 (EFI_GRAPHICS_OUTPUT_BLT_OPERATION)BltOperation,
                                 SourceX,
                                 SourceY,
                                 DestinationX,
                                 DestinationY,
                                 Width,
                                 Height,
                                 Delta
                                 );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      } else if (BltOperation == EfiUgaVideoToBltBuffer) {
        //
        // Only need to read the data into buffer one time
        //
        return EFI_SUCCESS;
      }
    }

    if ((Private->TextOutList[Index].UgaDraw != NULL) && FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = Private->TextOutList[Index].UgaDraw->Blt (
                                                      Private->TextOutList[Index].UgaDraw,
                                                      BltBuffer,
                                                      BltOperation,
                                                      SourceX,
                                                      SourceY,
                                                      DestinationX,
                                                      DestinationY,
                                                      Width,
                                                      Height,
                                                      Delta
                                                      );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      } else if (BltOperation == EfiUgaVideoToBltBuffer) {
        //
        // Only need to read the data into buffer one time
        //
        return EFI_SUCCESS;
      }
    }
  }

  return ReturnStatus;
}

/**
  Sets the output device(s) to a specified mode.

  @param  Private                 Text Out Splitter pointer.
  @param  ModeNumber              The mode number to set.

**/
VOID
TextOutSetMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  UINTN                           ModeNumber
  )
{
  //
  // No need to do extra check here as whether (Column, Row) is valid has
  // been checked in ConSplitterTextOutSetCursorPosition. And (0, 0) should
  // always be supported.
  //
  Private->TextOutMode.Mode          = (INT32)ModeNumber;
  Private->TextOutMode.CursorColumn  = 0;
  Private->TextOutMode.CursorRow     = 0;
  Private->TextOutMode.CursorVisible = TRUE;

  return;
}
