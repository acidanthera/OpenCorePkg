/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <IndustryStandard/AppleIcon.h>
#include <IndustryStandard/AppleDiskLabel.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MtrrLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcPngLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OpenCanopy.h"
#include "GuiIo.h"
#include "GuiApp.h"
#include "Views/BootPicker.h"
#include "Blending.h"

typedef struct {
  UINT32 X;
  UINT32 Y;
  UINT32 Width;
  UINT32 Height;
} GUI_DRAW_REQUEST;

//
// I/O contexts
//
STATIC GUI_OUTPUT_CONTEXT                         *mOutputContext  = NULL;
GLOBAL_REMOVE_IF_UNREFERENCED GUI_POINTER_CONTEXT *mPointerContext = NULL;
GLOBAL_REMOVE_IF_UNREFERENCED GUI_KEY_CONTEXT     *mKeyContext     = NULL;
//
// Screen buffer information
//
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL *mScreenBuffer     = NULL;
STATIC UINT32                        mScreenBufferDelta = 0;
//
// Frame timing information (60 FPS)
//
STATIC UINT64                        mDeltaTscTarget    = 0;
STATIC UINT64                        mStartTsc          = 0;
//
// Drawing rectangles information
//
STATIC UINT8                         mNumValidDrawReqs  = 0;
STATIC GUI_DRAW_REQUEST              mDrawRequests[6]   = { { 0 } };

STATIC UINT32                        mPointerOldDrawBaseX  = 0;
STATIC UINT32                        mPointerOldDrawBaseY  = 0;
STATIC UINT32                        mPointerOldDrawWidth  = 0;
STATIC UINT32                        mPointerOldDrawHeight = 0;

#define PIXEL_TO_UINT32(Pixel)  \
  ((UINT32) SIGNATURE_32 ((Pixel)->Blue, (Pixel)->Green, (Pixel)->Red, (Pixel)->Reserved))

BOOLEAN
GuiClipChildBounds (
  IN     INT64   ChildOffset,
  IN     UINT32  ChildLength,
  IN OUT UINT32  *ReqOffset,
  IN OUT UINT32  *ReqLength
  )
{
  INT64  OffsetDelta;
  UINT32 NewOffset;
  UINT32 NewLength;

  ASSERT (ReqOffset != NULL);
  ASSERT (ReqLength != NULL);

  NewOffset = *ReqOffset;
  NewLength = *ReqLength;

  OffsetDelta = NewOffset - ChildOffset;

  if (OffsetDelta >= 0) {
    //
    // The requested offset starts within or past the child.
    //
    if (ChildLength <= OffsetDelta) {
      //
      // The requested offset starts past the child.
      //
      return FALSE;
    }
    //
    // The requested offset starts within the child.
    //
    NewOffset = (UINT32) OffsetDelta;
    NewLength = MIN (NewLength, (UINT32) (ChildLength - OffsetDelta));
  } else {
    //
    // The requested offset starts before the child.
    //
    if (NewLength <= -OffsetDelta) {
      //
      // The requested offset ends before the child.
      //
      return FALSE;
    }
    //
    // The requested offset ends within the child.
    //
    NewOffset = 0;
    NewLength = MIN ((UINT32) (NewLength + OffsetDelta), ChildLength);
  }

  ASSERT (ChildOffset + ChildLength > 0);
  ASSERT (NewLength > 0);
  ASSERT (NewOffset + NewLength <= ChildLength);

  *ReqOffset = NewOffset;
  *ReqLength = NewLength;

  return TRUE;
}

VOID
GuiObjDrawDelegate (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     UINT32                  OffsetX,
  IN     UINT32                  OffsetY,
  IN     UINT32                  Width,
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
  )
{
  BOOLEAN       Result;

  UINTN         Index;
  GUI_OBJ_CHILD *Child;

  UINT32        ChildDrawOffsetX;
  UINT32        ChildDrawOffsetY;
  UINT32        ChildDrawWidth;
  UINT32        ChildDrawHeight;
  UINT8         ChildOpacity;

  ASSERT (This != NULL);
  ASSERT (OffsetX < This->Width);
  ASSERT (OffsetY < This->Height);
  ASSERT (Width > 0);
  ASSERT (Height > 0);
  ASSERT (Width <= This->Width);
  ASSERT (Height <= This->Height);
  ASSERT (DrawContext != NULL);

  for (Index = 0; Index < This->NumChildren; ++Index) {
    Child = This->Children[Index];

    ChildDrawOffsetX = OffsetX;
    ChildDrawWidth   = Width;
    Result = GuiClipChildBounds (
               Child->Obj.OffsetX,
               Child->Obj.Width,
               &ChildDrawOffsetX,
               &ChildDrawWidth
               );
    if (!Result) {
      continue;
    }

    ChildDrawOffsetY = OffsetY;
    ChildDrawHeight  = Height;
    Result = GuiClipChildBounds (
               Child->Obj.OffsetY,
               Child->Obj.Height,
               &ChildDrawOffsetY,
               &ChildDrawHeight
               );
    if (!Result) {
      continue;
    }

    if (Opacity == 0xFF) {
      ChildOpacity = Child->Obj.Opacity;
    } else if (Child->Obj.Opacity == 0xFF) {
      ChildOpacity = Opacity;
    } else {
      ChildOpacity = RGB_APPLY_OPACITY (Child->Obj.Opacity, Opacity);
    }

    ASSERT (ChildDrawOffsetX + ChildDrawWidth <= Child->Obj.Width);
    ASSERT (ChildDrawOffsetY + ChildDrawHeight <= Child->Obj.Height);
    ASSERT (ChildDrawWidth > 0);
    ASSERT (ChildDrawHeight > 0);
    ASSERT (Child->Obj.Draw != NULL);
    Child->Obj.Draw (
                 &Child->Obj,
                 DrawContext,
                 Context,
                 BaseX + Child->Obj.OffsetX,
                 BaseY + Child->Obj.OffsetY,
                 ChildDrawOffsetX,
                 ChildDrawOffsetY,
                 ChildDrawWidth,
                 ChildDrawHeight,
                 ChildOpacity
                 );
  }
}

GUI_OBJ *
GuiObjDelegatePtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  UINTN         Index;
  GUI_OBJ       *Obj;
  GUI_OBJ_CHILD *Child;

  ASSERT (This != NULL);
  ASSERT (Event->Pos.Pos.X >= BaseX);
  ASSERT (Event->Pos.Pos.Y >= BaseY);
  ASSERT (This->Width  > Event->Pos.Pos.X - BaseX);
  ASSERT (This->Height > Event->Pos.Pos.Y - BaseY);
  ASSERT (DrawContext != NULL);
  //
  // Pointer event propagation is backwards due to forwards draw order.
  //
  for (Index = This->NumChildren; Index > 0; --Index) {
    Child = This->Children[Index - 1];
    if (Event->Pos.Pos.X - BaseX  < Child->Obj.OffsetX
     || Event->Pos.Pos.X - BaseX >= Child->Obj.OffsetX + Child->Obj.Width
     || Event->Pos.Pos.Y - BaseY  < Child->Obj.OffsetY
     || Event->Pos.Pos.Y - BaseY >= Child->Obj.OffsetY + Child->Obj.Height) {
      continue;
    }

    ASSERT (Child->Obj.PtrEvent != NULL);
    Obj = Child->Obj.PtrEvent (
                       &Child->Obj,
                       DrawContext,
                       Context,
                       BaseX + Child->Obj.OffsetX,
                       BaseY + Child->Obj.OffsetY,
                       Event
                       );
    if (Obj != NULL) {
      return Obj;
    }
  }

  return NULL;
}

VOID
GuiDrawToBufferFill (
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Colour,
  IN OUT GUI_DRAWING_CONTEXT                  *DrawContext,
  IN     UINT32                               PosX,
  IN     UINT32                               PosY,
  IN     UINT32                               Width,
  IN     UINT32                               Height
  )
{
  UINT32 RowIndex;
  UINT32 TargetRowOffset;

  ASSERT (Colour != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Width > 0);
  ASSERT (Height > 0);
  //
  // Screen cropping happens in GuiRequestDrawCrop().
  //
  ASSERT (DrawContext->Screen.Width  >= PosX);
  ASSERT (DrawContext->Screen.Height >= PosY);
  ASSERT (PosX + Width <= DrawContext->Screen.Width);
  ASSERT (PosY + Height <= DrawContext->Screen.Height);
  //
  // Iterate over each row of the request.
  //
  for (
    RowIndex = 0,
      TargetRowOffset = PosY * DrawContext->Screen.Width;
    RowIndex < Height;
    ++RowIndex,
      TargetRowOffset += DrawContext->Screen.Width
    ) {
    //
    // Populate the row pixel-by-pixel with Source's (0,0).
    //
    SetMem32 (
      &mScreenBuffer[TargetRowOffset + PosX],
      Width * sizeof (UINT32),
      PIXEL_TO_UINT32 (Colour)
      );
  }

  //
  // TODO: Support opaque fill?
  //

#if 0
  //
  // Iterate over each row of the request.
  //
  for (
    RowIndex = 0,
      TargetRowOffset = PosY * DrawContext->Screen.Width;
    RowIndex < Height;
    ++RowIndex,
      TargetRowOffset += DrawContext->Screen.Width
    ) {
    //
    // Blend the row pixel-by-pixel with Source's (0,0).
    //
    for (
      TargetColumnOffset = PosY;
      TargetColumnOffset < PosY + Width;
      ++TargetColumnOffset
      ) {
      TargetPixel = &mScreenBuffer[TargetRowOffset + TargetColumnOffset];
      GuiBlendPixel (TargetPixel, &Image->Buffer[0], Opacity);
    }
  }
#endif
}

VOID
GuiDrawToBuffer (
  IN     CONST GUI_IMAGE      *Image,
  IN     UINT8                Opacity,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height
  )
{
  UINT32                              PosX;
  UINT32                              PosY;

  UINT32                              RowIndex;
  UINT32                              SourceRowOffset;
  UINT32                              TargetRowOffset;
  UINT32                              SourceColumnOffset;
  UINT32                              TargetColumnOffset;
  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *SourcePixel;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *TargetPixel;

  ASSERT (Image != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (BaseX + OffsetX >= 0);
  ASSERT (BaseY + OffsetY >= 0);
  ASSERT (Width > 0);
  ASSERT (Height > 0);
  ASSERT (BaseX + OffsetX + Width >= 0);
  ASSERT (BaseY + OffsetY + Height >= 0);
  ASSERT (BaseX + OffsetX + Width <= MAX_UINT32);
  ASSERT (BaseY + OffsetY + Height <= MAX_UINT32);

  PosX = (UINT32) (BaseX + OffsetX);
  PosY = (UINT32) (BaseY + OffsetY);
  //
  // Screen cropping happens in GuiRequestDrawCrop().
  //
  ASSERT (DrawContext->Screen.Width  >= PosX);
  ASSERT (DrawContext->Screen.Height >= PosY);
  ASSERT (PosX + Width <= DrawContext->Screen.Width);
  ASSERT (PosY + Height <= DrawContext->Screen.Height);

  if (Opacity == 0) {
    return;
  }

  ASSERT (Image->Width  > OffsetX);
  ASSERT (Image->Height > OffsetY);
  //
  // Only crop to the image's dimensions when not using fill-drawing.
  //
  Width  = MIN (Width,  Image->Width  - OffsetX);
  Height = MIN (Height, Image->Height - OffsetY);
  if (Width == 0 || Height == 0) {
    return;
  }

  ASSERT (Image->Buffer != NULL);

  if (Opacity == 0xFF) {
    //
    // Iterate over each row of the request.
    //
    for (
      RowIndex = 0,
        SourceRowOffset = OffsetY * Image->Width,
        TargetRowOffset = PosY * DrawContext->Screen.Width;
      RowIndex < Height;
      ++RowIndex,
        SourceRowOffset += Image->Width,
        TargetRowOffset += DrawContext->Screen.Width
      ) {
      //
      // Blend the row pixel-by-pixel.
      //
      for (
        TargetColumnOffset = PosX, SourceColumnOffset = OffsetX;
        TargetColumnOffset < PosX + Width;
        ++TargetColumnOffset, ++SourceColumnOffset
        ) {
        TargetPixel = &mScreenBuffer[TargetRowOffset + TargetColumnOffset];
        SourcePixel = &Image->Buffer[SourceRowOffset + SourceColumnOffset];
        GuiBlendPixelSolid (TargetPixel, SourcePixel);
      }
    }
  } else {
    //
    // Iterate over each row of the request.
    //
    for (
      RowIndex = 0,
        SourceRowOffset = OffsetY * Image->Width,
        TargetRowOffset = PosY * DrawContext->Screen.Width;
      RowIndex < Height;
      ++RowIndex,
        SourceRowOffset += Image->Width,
        TargetRowOffset += DrawContext->Screen.Width
      ) {
      //
      // Blend the row pixel-by-pixel.
      //
      for (
        TargetColumnOffset = PosX, SourceColumnOffset = OffsetX;
        TargetColumnOffset < PosX + Width;
        ++TargetColumnOffset, ++SourceColumnOffset
        ) {
        TargetPixel = &mScreenBuffer[TargetRowOffset + TargetColumnOffset];
        SourcePixel = &Image->Buffer[SourceRowOffset + SourceColumnOffset];
        GuiBlendPixelOpaque (TargetPixel, SourcePixel, Opacity);
      }
    }
  }
}

VOID
GuiRequestDraw (
  IN UINT32  PosX,
  IN UINT32  PosY,
  IN UINT32  Width,
  IN UINT32  Height
  )
{
  UINTN  Index;

  UINT32 ThisArea;
  UINT32 ThisMaxXPlus1;
  UINT32 ThisMaxYPlus1;

  UINT32 ReqMaxXPlus1;
  UINT32 ReqMaxYPlus1;
  UINT32 ReqArea;

  UINT32 CombX;
  UINT32 CombY;
  UINT32 CombWidth;
  UINT32 CombHeight;
  UINT32 CombArea;

  ThisMaxXPlus1 = PosX + Width;
  ThisMaxYPlus1 = PosY + Height;

  ThisArea = Width * Height;

  for (Index = 0; Index < mNumValidDrawReqs; ++Index) {
    //
    // Calculate several dimensions to determine whether to merge the two
    // draw requests for improved flushing performance.
    //
    ReqMaxXPlus1 = mDrawRequests[Index].X + mDrawRequests[Index].Width;
    ReqMaxYPlus1 = mDrawRequests[Index].Y + mDrawRequests[Index].Height;
    ReqArea = mDrawRequests[Index].Width * mDrawRequests[Index].Height;

    if (mDrawRequests[Index].X < PosX) {
      CombX = mDrawRequests[Index].X;
    } else {
      CombX = PosX;
    }

    if (ReqMaxXPlus1 > ThisMaxXPlus1) {
      CombWidth = ReqMaxXPlus1;
    } else {
      CombWidth = ThisMaxXPlus1;
    }
    CombWidth -= CombX;

    if (mDrawRequests[Index].Y < PosY) {
      CombY = mDrawRequests[Index].Y;
    } else {
      CombY = PosY;
    }

    if (ReqMaxYPlus1 > ThisMaxYPlus1) {
      CombHeight = ReqMaxYPlus1;
    } else {
      CombHeight = ThisMaxYPlus1;
    }
    CombHeight -= CombY;

    CombArea = CombWidth * CombHeight;
    //
    // Two requests are merged when the overarching rectangle is not bigger than
    // the two separate rectangles (not accounting for the overlap, as it would
    // be drawn twice).
    //
    // TODO: Profile a good constant factor?
    //
    if (ThisArea + ReqArea >= CombArea) {
      mDrawRequests[Index].X = CombX;
      mDrawRequests[Index].Y = CombY;
      mDrawRequests[Index].Width = CombWidth;
      mDrawRequests[Index].Height = CombHeight;
      return;
    }
  }

  if (mNumValidDrawReqs >= ARRAY_SIZE (mDrawRequests)) {
    ASSERT (FALSE);
    return;
  }

  mDrawRequests[mNumValidDrawReqs].X = PosX;
  mDrawRequests[mNumValidDrawReqs].Y = PosY;
  mDrawRequests[mNumValidDrawReqs].Width = Width;
  mDrawRequests[mNumValidDrawReqs].Height = Height;
  ++mNumValidDrawReqs;
}

VOID
GuiRequestDrawCrop (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                X,
  IN     INT64                Y,
  IN     UINT32               Width,
  IN     UINT32               Height
  )
{
  UINT32 PosX;
  UINT32 PosY;
  INT64  EffWidth;
  INT64  EffHeight;

  ASSERT (DrawContext != NULL);

  EffWidth  = Width;
  EffHeight = Height;
  //
  // Only draw the onscreen parts.
  //
  if (X >= 0) {
    PosX = (UINT32)X;
  } else {
    EffWidth += X;
    PosX      = 0;
  }

  if (Y >= 0) {
    PosY = (UINT32)Y;
  } else {
    EffHeight += Y;
    PosY       = 0;
  }

  EffWidth  = MIN (EffWidth,  (INT64) DrawContext->Screen.Width  - PosX);
  EffHeight = MIN (EffHeight, (INT64) DrawContext->Screen.Height - PosY);

  if (EffWidth <= 0 || EffHeight <= 0) {
    return;
  }

  GuiRequestDraw (PosX, PosY, (UINT32) EffWidth, (UINT32) EffHeight);
}

VOID
GuiOverlayPointer (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  CONST GUI_IMAGE  *CursorImage;
  UINT32           MaxWidth;
  UINT32           MaxHeight;
  GUI_PTR_POSITION PointerPos;

  INT64            BaseX;
  INT64            BaseY;
  UINT32           ImageOffsetX;
  UINT32           ImageOffsetY;
  UINT32           DrawBaseX;
  UINT32           DrawBaseY;

  ASSERT (DrawContext != NULL);

  ASSERT (DrawContext->GetCursorImage != NULL);
  CursorImage = DrawContext->GetCursorImage (DrawContext->GuiContext);
  ASSERT (CursorImage != NULL);
  //
  // Poll the current cursor position late to reduce input lag.
  //
  GuiPointerGetPosition (mPointerContext, &PointerPos);

  ASSERT (PointerPos.Pos.X < DrawContext->Screen.Width);
  ASSERT (PointerPos.Pos.Y < DrawContext->Screen.Height);

  //
  // Unconditionally draw the cursor to increase frametime consistency and
  // prevent situational hiding.
  //
  // The original area of the cursor is restored at the beginning of the main
  // drawing loop.
  //

  //
  // Draw the new cursor at the new position.
  //

  BaseX = (INT64) PointerPos.Pos.X - BOOT_CURSOR_OFFSET * DrawContext->Scale;
  MaxWidth = DrawContext->Screen.Width;
  if (BaseX < 0) {
    ImageOffsetX = (UINT32) -BaseX;
    DrawBaseX    = 0;
  } else {
    ImageOffsetX = 0;
    DrawBaseX    = (UINT32) BaseX;
    MaxWidth    -= DrawBaseX;
  }

  BaseY = (INT64) PointerPos.Pos.Y - BOOT_CURSOR_OFFSET * DrawContext->Scale;
  MaxHeight = DrawContext->Screen.Height;
  if (BaseY < 0) {
    ImageOffsetY = (UINT32) -BaseY;
    DrawBaseY    = 0;
  } else {
    ImageOffsetY = 0;
    DrawBaseY    = (UINT32) BaseY;
    MaxHeight   -= DrawBaseY;
  }

  GuiDrawToBuffer (
    CursorImage,
    DrawContext->CursorOpacity,
    DrawContext,
    BaseX,
    BaseY,
    ImageOffsetX,
    ImageOffsetY,
    MaxWidth,
    MaxHeight
    );
  //
  // Queue a draw request for the newly drawn cursor.
  //
  GuiRequestDraw (
    DrawBaseX,
    DrawBaseY,
    MaxWidth,
    MaxHeight
    );

  mPointerOldDrawBaseX  = DrawBaseX;
  mPointerOldDrawBaseY  = DrawBaseY;
  mPointerOldDrawWidth  = MaxWidth;
  mPointerOldDrawHeight = MaxHeight;
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
STATIC
UINT64
InternalCpuDelayTsc (
  IN UINT64  Delay
  )
{
  UINT64  Ticks;
  UINT64  Tsc;

  //
  // The target timer count is calculated here
  //
  Ticks = AsmReadTsc () + Delay;

  //
  // Wait until time out
  // Timer wrap-arounds are NOT handled correctly by this function.
  // Thus, this function must be called within 10 years of reset since
  // Intel guarantees a minimum of 10 years before the TSC wraps.
  //
  while ((Tsc = AsmReadTsc ()) < Ticks) {
    CpuPause ();
  }

  return Tsc;
}

VOID
GuiFlushScreen (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  UINTN   Index;

  UINT64  EndTsc;
  UINT64  DeltaTsc;

  ASSERT (DrawContext != NULL);
  ASSERT (DrawContext->Screen.OffsetX == 0);
  ASSERT (DrawContext->Screen.OffsetY == 0);
  ASSERT (DrawContext->Screen.Draw != NULL);
  for (Index = 0; Index < mNumValidDrawReqs; ++Index) {
    DrawContext->Screen.Draw (
      &DrawContext->Screen,
      DrawContext,
      DrawContext->GuiContext,
      0,
      0,
      mDrawRequests[Index].X,
      mDrawRequests[Index].Y,
      mDrawRequests[Index].Width,
      mDrawRequests[Index].Height,
      DrawContext->Screen.Opacity
      );
  }
  //
  // Raise the TPL to not interrupt timing or flushing.
  //
  EndTsc   = AsmReadTsc ();
  DeltaTsc = EndTsc - mStartTsc;
  if (DeltaTsc < mDeltaTscTarget) {
    EndTsc = InternalCpuDelayTsc (mDeltaTscTarget - DeltaTsc);
  }

  if (mPointerContext != NULL) {
    GuiOverlayPointer (DrawContext);
  }

  for (Index = 0; Index < mNumValidDrawReqs; ++Index) {
    GuiOutputBlt (
      mOutputContext,
      mScreenBuffer,
      EfiBltBufferToVideo,
      mDrawRequests[Index].X,
      mDrawRequests[Index].Y,
      mDrawRequests[Index].X,
      mDrawRequests[Index].Y,
      mDrawRequests[Index].Width,
      mDrawRequests[Index].Height,
      mScreenBufferDelta
      );
  }

  mNumValidDrawReqs = 0;
  //
  // Explicitly include BLT time in the timing calculation.
  // FIXME: GOP takes inconsistently long depending on dimensions.
  //
  mStartTsc = EndTsc;
}

VOID
GuiRedrawAndFlushScreen (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  ASSERT (DrawContext != NULL);

  mStartTsc = AsmReadTsc ();

  GuiRequestDraw (0, 0, DrawContext->Screen.Width, DrawContext->Screen.Height);
  GuiFlushScreen (DrawContext);
}

EFI_STATUS
GuiLibConstruct (
  IN BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN INT32                    CursorOffsetX,
  IN INT32                    CursorOffsetY
  )
{
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *OutputInfo;
  INT64                                      CursorX;
  INT64                                      CursorY;

  mOutputContext = GuiOutputConstruct (GuiContext->Scale);
  if (mOutputContext == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to initialise output\n"));
    return EFI_UNSUPPORTED;
  }

  OutputInfo = GuiOutputGetInfo (mOutputContext);
  ASSERT (OutputInfo != NULL);

  if ((GuiContext->PickerContext->PickerAttributes & OC_ATTR_USE_POINTER_CONTROL) != 0) {
    CursorX = (INT64) CursorOffsetX + OutputInfo->HorizontalResolution / 2;
    if (CursorX < 0) {
      CursorX = 0;
    } else if (CursorX > OutputInfo->HorizontalResolution - 1) {
      CursorX = OutputInfo->HorizontalResolution - 1;
    }
    CursorY = (INT64) CursorOffsetY + OutputInfo->VerticalResolution / 2;
    if (CursorY < 0) {
      CursorY = 0;
    } else if (CursorY > OutputInfo->VerticalResolution - 1) {
      CursorY = OutputInfo->VerticalResolution - 1;
    }

    mPointerContext = GuiPointerConstruct (
      (UINT32) CursorX,
      (UINT32) CursorY,
      OutputInfo->HorizontalResolution,
      OutputInfo->VerticalResolution,
      GuiContext->Scale
      );
    if (mPointerContext == NULL) {
      DEBUG ((DEBUG_WARN, "OCUI: Failed to initialise pointer\n"));
    }
  }

  mKeyContext = GuiKeyConstruct (GuiContext->PickerContext);
  if (mKeyContext == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to initialise key input\n"));
  }

  if (mPointerContext == NULL && mKeyContext == NULL) {
    GuiLibDestruct ();
    return EFI_UNSUPPORTED;
  }

  mScreenBufferDelta = OutputInfo->HorizontalResolution * sizeof (*mScreenBuffer);
  mScreenBuffer      = AllocatePool (OutputInfo->VerticalResolution * mScreenBufferDelta);
  if (mScreenBuffer == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: GUI alloc failure\n"));
    GuiLibDestruct ();
    return EFI_OUT_OF_RESOURCES;
  }

  MtrrSetMemoryAttribute (
    (EFI_PHYSICAL_ADDRESS)(UINTN) mScreenBuffer,
    mScreenBufferDelta * OutputInfo->VerticalResolution,
    CacheWriteBack
    );

  mDeltaTscTarget =  DivU64x32 (OcGetTSCFrequency (), 60);

  return EFI_SUCCESS;
}

VOID
GuiLibDestruct (
  VOID
  )
{
  if (mOutputContext != NULL) {
    GuiOutputDestruct (mOutputContext);
    mOutputContext = NULL;
  }

  if (mPointerContext != NULL) {
    GuiPointerDestruct (mPointerContext);
    mPointerContext = NULL;
  }

  if (mKeyContext != NULL) {
    GuiKeyDestruct (mKeyContext);
    mKeyContext = NULL;
  }
}

VOID
GuiViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  CONST GUI_VIEW_CONTEXT   *ViewContext
  )
{
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *OutputInfo;

  ASSERT (DrawContext != NULL);

  OutputInfo = GuiOutputGetInfo (mOutputContext);
  ASSERT (OutputInfo != NULL);

  DrawContext->CursorOpacity = 0xFF;
  DrawContext->Scale         = GuiContext->Scale;

  DrawContext->Screen.OffsetX     = 0;
  DrawContext->Screen.OffsetY     = 0;
  DrawContext->Screen.Width       = OutputInfo->HorizontalResolution;
  DrawContext->Screen.Height      = OutputInfo->VerticalResolution;
  DrawContext->Screen.Opacity     = 0xFF;
  DrawContext->Screen.Draw        = ViewContext->Draw;
  DrawContext->Screen.KeyEvent    = ViewContext->KeyEvent;
  DrawContext->Screen.PtrEvent    = ViewContext->PtrEvent;
  DrawContext->Screen.NumChildren = ViewContext->NumChildren;
  DrawContext->Screen.Children    = ViewContext->Children;

  DrawContext->GetCursorImage = ViewContext->GetCursorImage;
  DrawContext->ExitLoop       = ViewContext->ExitLoop;
  DrawContext->GuiContext     = GuiContext;
  InitializeListHead (&DrawContext->Animations);
}

VOID
GuiViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  OUT    BOOT_PICKER_GUI_CONTEXT *GuiContext
  )
{
  GUI_PTR_POSITION                           CursorPosition;
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *OutputInfo;

  if (mPointerContext != NULL) {
    OutputInfo = GuiOutputGetInfo (mOutputContext);
    GuiPointerGetPosition (mPointerContext, &CursorPosition);
    GuiContext->CursorOffsetX = (INT32) ((INT64) CursorPosition.Pos.X - OutputInfo->HorizontalResolution / 2);
    GuiContext->CursorOffsetY = (INT32) ((INT64) CursorPosition.Pos.Y - OutputInfo->VerticalResolution / 2);
  }

  ZeroMem (DrawContext, sizeof (*DrawContext));
}

VOID
GuiGetBaseCoords (
  IN  GUI_OBJ              *This,
  IN  GUI_DRAWING_CONTEXT  *DrawContext,
  OUT INT64                *BaseX,
  OUT INT64                *BaseY
  )
{
  GUI_OBJ       *Obj;
  GUI_OBJ_CHILD *ChildObj;
  INT64         X;
  INT64         Y;
  UINT32        Index;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (DrawContext->Screen.OffsetX == 0);
  ASSERT (DrawContext->Screen.OffsetY == 0);
  ASSERT (BaseX != NULL);
  ASSERT (BaseY != NULL);

  X   = 0;
  Y   = 0;
  Obj = This;
  do {
    X += Obj->OffsetX;
    Y += Obj->OffsetY;

    ChildObj = BASE_CR (Obj, GUI_OBJ_CHILD, Obj);
    Obj      = ChildObj->Parent;

    DEBUG_CODE_BEGIN ();
    if (Obj != NULL) {
      for (Index = 0; Index < Obj->NumChildren; ++Index) {
        if (Obj->Children[Index] == ChildObj) {
          break;
        }
      }
      ASSERT (Index != Obj->NumChildren);
    }
    DEBUG_CODE_END ();
  } while (Obj != NULL);

  DEBUG_CODE_BEGIN ();
  for (Index = 0; Index < DrawContext->Screen.NumChildren; ++Index) {
    if (DrawContext->Screen.Children[Index] == ChildObj) {
      break;
    }
  }
  ASSERT (Index != DrawContext->Screen.NumChildren);
  DEBUG_CODE_END ();

  *BaseX = X;
  *BaseY = Y;
}

VOID
GuiDrawLoop (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  BOOLEAN             Result;

  GUI_KEY_EVENT        KeyEvent;
  GUI_PTR_EVENT        PointerEvent;
  GUI_OBJ              *TempObject;
  GUI_OBJ              *HoldObject;
  INT64                HoldObjBaseX;
  INT64                HoldObjBaseY;
  CONST LIST_ENTRY     *AnimEntry;
  GUI_ANIMATION        *Animation;
  UINT64               LoopStartTsc;
  UINT64               LastTsc;
  UINT64               NewLastTsc;

  ASSERT (DrawContext != NULL);

  mNumValidDrawReqs      = 0;
  DrawContext->FrameTime = 0;
  HoldObject             = NULL;
  //
  // Clear previous inputs.
  //
  if (mPointerContext != NULL) {
    GuiPointerReset (mPointerContext);
  }
  GuiKeyReset (mKeyContext);

  //
  // Pointer state will be implicitly initialised on the first call in the loop.
  //

  //
  // Main drawing loop, time and derieve sub-frequencies as required.
  //
  LastTsc = LoopStartTsc = mStartTsc = AsmReadTsc ();
  do {
    if (mPointerContext != NULL) {
      //
      // Restore the rectangle previously covered by the cursor.
      // The new cursor is drawn right before flushing the screen.
      //
      GuiRequestDraw (
        mPointerOldDrawBaseX,
        mPointerOldDrawBaseY,
        mPointerOldDrawWidth,
        mPointerOldDrawHeight
        );
      //
      // Process pointer events.
      //
      Result = GuiPointerGetEvent (mPointerContext, &PointerEvent);
      if (Result) {
        if (PointerEvent.Type == GuiPointerPrimaryUp) {
          //
          // 'Button down' must have caught and set an interaction object, but
          // it may be NULL for objects that solely delegate pointer events.
          //
          if (HoldObject != NULL) {
            GuiGetBaseCoords (
              HoldObject,
              DrawContext,
              &HoldObjBaseX,
              &HoldObjBaseY
              );
            HoldObject->PtrEvent (
              HoldObject,
              DrawContext,
              DrawContext->GuiContext,
              HoldObjBaseX,
              HoldObjBaseY,
              &PointerEvent
              );
            HoldObject = NULL;
          }
        } else {
          //
          // HoldObject == NULL cannot be tested here as double-click may arrive
          // before button up.
          //
          ASSERT (PointerEvent.Type != GuiPointerPrimaryUp);
          TempObject = DrawContext->Screen.PtrEvent (
            &DrawContext->Screen,
            DrawContext,
            DrawContext->GuiContext,
            0,
            0,
            &PointerEvent
            );
          if (PointerEvent.Type == GuiPointerPrimaryDown) {
            HoldObject = TempObject;
          }
        }
        //
        // If detected pointer press then disable menu timeout
        //
        if (DrawContext->TimeOutSeconds > 0) {
          //
          // Voice only unrelated key press.
          //
          if (!DrawContext->GuiContext->ReadyToBoot
            && DrawContext->GuiContext->PickerContext->PickerAudioAssist) {
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileAbortTimeout,
              FALSE
              );
          }

          DrawContext->TimeOutSeconds = 0;
        }
      } else if (HoldObject != NULL) {
        //
        // If there are no events to process, update the cursor position with
        // the interaction object for visual effects.
        //
        PointerEvent.Type = GuiPointerPrimaryDown;
        GuiPointerGetPosition (mPointerContext, &PointerEvent.Pos);
        GuiGetBaseCoords (
          HoldObject,
          DrawContext,
          &HoldObjBaseX,
          &HoldObjBaseY
          );
        HoldObject->PtrEvent (
          HoldObject,
          DrawContext,
          DrawContext->GuiContext,
          HoldObjBaseX,
          HoldObjBaseY,
          &PointerEvent
          );
      }
    }

    if (mKeyContext != NULL) {
      //
      // Process key events. Only allow one key at a time for now.
      //
      Result = GuiKeyGetEvent (mKeyContext, &KeyEvent);
      if (Result) {
        DrawContext->Screen.KeyEvent (
          &DrawContext->Screen,
          DrawContext,
          DrawContext->GuiContext,
          &KeyEvent
          );
        //
        // If detected key press then disable menu timeout
        //
        if (DrawContext->TimeOutSeconds > 0) {
          //
          // Voice only unrelated key press.
          //
          if (!DrawContext->GuiContext->ReadyToBoot
            && DrawContext->GuiContext->PickerContext->PickerAudioAssist) {
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileAbortTimeout,
              FALSE
              );
          }

          DrawContext->TimeOutSeconds = 0;
        }
      }
    }
    //
    // Process queued animations.
    //
    AnimEntry = GetFirstNode (&DrawContext->Animations);
    while (!IsNull (&DrawContext->Animations, AnimEntry)) {
      Animation = BASE_CR (AnimEntry, GUI_ANIMATION, Link);
      Result = Animation->Animate (Animation->Context, DrawContext, DrawContext->FrameTime);

      AnimEntry = GetNextNode (&DrawContext->Animations, AnimEntry);

      if (Result) {
        RemoveEntryList (&Animation->Link);
        InitializeListHead (&Animation->Link);
      }
    }
    ++DrawContext->FrameTime;
    //
    // Flush the changes performed in this refresh iteration.
    //
    GuiFlushScreen (DrawContext);

    NewLastTsc = AsmReadTsc ();

    if (DrawContext->GuiContext->AudioPlaybackTimeout >= 0
      && DrawContext->GuiContext->PickerContext->PickerAudioAssist) {
      DrawContext->GuiContext->AudioPlaybackTimeout -= (INT32) (DivU64x32 (
        GetTimeInNanoSecond (NewLastTsc - LastTsc),
        1000000
        ));
      if (DrawContext->GuiContext->AudioPlaybackTimeout <= 0) {
        switch (DrawContext->GuiContext->VoAction) {
          case CanopyVoSelectedEntry:
          {
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileSelected,
              FALSE
              );
            DrawContext->GuiContext->PickerContext->PlayAudioEntry (
              DrawContext->GuiContext->PickerContext,
              DrawContext->GuiContext->BootEntry
              );
            break;
          }

          case CanopyVoFocusPassword:
          {
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileEnterPassword,
              TRUE
              );
            break;
          }

          case CanopyVoFocusShutDown:
          {
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileSelected,
              FALSE
              );
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileShutDown,
              TRUE
              );
            break;
          }

          case CanopyVoFocusRestart:
          {
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileSelected,
              FALSE
              );
            DrawContext->GuiContext->PickerContext->PlayAudioFile (
              DrawContext->GuiContext->PickerContext,
              OcVoiceOverAudioFileRestart,
              TRUE
              );
            break;
          }

          default:
          {
            ASSERT (FALSE);
            break;
          }
        }
        //
        // Avoid playing twice if we reach precisely 0.
        //
        DrawContext->GuiContext->AudioPlaybackTimeout = -1;
      }
    }

    //
    // Exit early if reach timer timeout and timer isn't disabled due to key event
    //
    if (DrawContext->TimeOutSeconds > 0
      && GetTimeInNanoSecond (NewLastTsc - LoopStartTsc) >= DrawContext->TimeOutSeconds * 1000000000ULL) {
      if (DrawContext->GuiContext->PickerContext->PickerAudioAssist) {
        DrawContext->GuiContext->PickerContext->PlayAudioFile (
          DrawContext->GuiContext->PickerContext,
          OcVoiceOverAudioFileTimeout,
          FALSE
          );
      }
      DrawContext->GuiContext->ReadyToBoot = TRUE;
      break;
    }

    LastTsc = NewLastTsc;
  } while (!DrawContext->ExitLoop (DrawContext, DrawContext->GuiContext));
}

VOID
GuiClearScreen (
  IN OUT GUI_DRAWING_CONTEXT           *DrawContext,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Pixel
  )
{
  GuiOutputBlt (
    mOutputContext,
    Pixel,
    EfiBltVideoFill,
    0,
    0,
    0,
    0,
    DrawContext->Screen.Width,
    DrawContext->Screen.Height,
    0
    );
}

/// A sine approximation via a third-order approx.
/// @param x    Angle (with 2^15 units/circle)
/// @return     Sine value (Q12)
STATIC
INT32
isin_S3 (
  IN INT32  x
  )
{
  //
  // S(x) = x * ( (3<<p) - (x*x>>r) ) >> s
  // n : Q-pos for quarter circle             13
  // A : Q-pos for output                     12
  // p : Q-pos for parentheses intermediate   15
  // r = 2n-p                                 11
  // s = A-1-p-n                              17
  //
  STATIC CONST INT32 n = 13;
  STATIC CONST INT32 p = 15;
  STATIC CONST INT32 r = 11;
  STATIC CONST INT32 s = 17;

  x = x << (30 - n); // shift to full s32 range (Q13->Q30)

  if ((x ^ (x << 1)) < 0) // test for quadrant 1 or 2
    x = (1 << 31) - x;

  x = x >> (30 - n);

  return x * ((3 << p) - (x * x >> r)) >> s;
}

UINT32
GuiGetInterpolatedValue (
  IN CONST GUI_INTERPOLATION  *Interpol,
  IN       UINT64             CurrentTime
  )
{
  INT32  AnimTime;
  UINT32 DeltaTime;

  ASSERT (Interpol != NULL);
  ASSERT (Interpol->Duration > 0);

  STATIC CONST UINT32 InterpolFpTimeFactor = 1U << 12U;

  if (CurrentTime <= Interpol->StartTime) {
    return Interpol->StartValue;
  }

  DeltaTime = (UINT32)(CurrentTime - Interpol->StartTime);

  if (DeltaTime >= Interpol->Duration) {
    return Interpol->EndValue;
  }

  AnimTime = (INT32) DivU64x64Remainder ((UINT64) InterpolFpTimeFactor * DeltaTime, Interpol->Duration, NULL);
  if (Interpol->Type == GuiInterpolTypeSmooth) {
    //
    // One InterpolFpTimeFactor unit corresponds to 45 degrees in the unit circle. Divide
    // the time by two because the integral of sin from 0 to Pi is equal to 2,
    // i.e. double speed.
    //
    AnimTime = isin_S3 (4 * AnimTime / 2);
    //
    // FP-square to further smoothen the animation.
    //
    AnimTime = (AnimTime * AnimTime) / InterpolFpTimeFactor;
  } else {
    ASSERT (Interpol->Type == GuiInterpolTypeLinear);
  }

  return (Interpol->EndValue * AnimTime
    + (Interpol->StartValue * (InterpolFpTimeFactor - AnimTime)))
    / InterpolFpTimeFactor;
}
