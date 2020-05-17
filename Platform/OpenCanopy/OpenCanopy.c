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

typedef struct {
  UINT32 MinX;
  UINT32 MinY;
  UINT32 MaxX;
  UINT32 MaxY;
} GUI_DRAW_REQUEST;

//
// Variables to assign the picked volume automatically once menu times out
//
extern BOOT_PICKER_GUI_CONTEXT mGuiContext;
extern GUI_VOLUME_PICKER mBootPicker;

//
// I/O contexts
//
STATIC GUI_OUTPUT_CONTEXT            *mOutputContext    = NULL;
STATIC GUI_POINTER_CONTEXT           *mPointerContext   = NULL;
STATIC GUI_KEY_CONTEXT               *mKeyContext       = NULL;
//
// Screen buffer information
//
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL *mScreenBuffer     = NULL;
STATIC UINT32                        mScreenBufferDelta = 0;
STATIC GUI_SCREEN_CURSOR             mScreenViewCursor  = { 0, 0 };
//
// Frame timing information (60 FPS)
//
STATIC UINT64                        mDeltaTscTarget    = 0;
STATIC UINT64                        mStartTsc          = 0;
//
// Drawing rectangles information
//
STATIC UINT8                         mNumValidDrawReqs  = 0;
STATIC GUI_DRAW_REQUEST              mDrawRequests[4]   = { { 0 } };
//
// Disk label palette.
//
STATIC
CONST UINT8
gAppleDiskLabelImagePalette[256] = {
  [0x00] = 255,
  [0xf6] = 238,
  [0xf7] = 221,
  [0x2a] = 204,
  [0xf8] = 187,
  [0xf9] = 170,
  [0x55] = 153,
  [0xfa] = 136,
  [0xfb] = 119,
  [0x80] = 102,
  [0xfc] = 85,
  [0xfd] = 68,
  [0xab] = 51,
  [0xfe] = 34,
  [0xff] = 17,
  [0xd6] = 0
};

BOOLEAN
GuiClipChildBounds (
  IN     INT64   ChildOffset,
  IN     UINT32  ChildLength,
  IN OUT UINT32  *ReqOffset,
  IN OUT UINT32  *ReqLength
  )
{
  UINT32 PosChildOffset;
  UINT32 OffsetDelta;
  UINT32 NewOffset;
  UINT32 NewLength;

  ASSERT (ReqOffset != NULL);
  ASSERT (ReqLength != NULL);

  if (ChildLength == 0) {
    return FALSE;
  }

  if (ChildOffset >= 0) {
    PosChildOffset = (UINT32)ChildOffset;
  } else {
    if ((INT64)ChildLength - (-ChildOffset) <= 0) {
      return FALSE;
    }

    PosChildOffset = 0;
    ChildLength    = (UINT32)(ChildLength - (-ChildOffset));
  }

  NewOffset = *ReqOffset;
  NewLength = *ReqLength;

  if (NewOffset >= PosChildOffset) {
    //
    // The requested offset starts within or past the child.
    //
    OffsetDelta = (NewOffset - PosChildOffset);
    if (ChildLength <= OffsetDelta) {
      //
      // The requested offset starts past the child.
      //
      return FALSE;
    }
    //
    // The requested offset starts within the child.
    //
    NewOffset -= PosChildOffset;
  } else {
    //
    // The requested offset ends within or before the child.
    //
    OffsetDelta = (PosChildOffset - NewOffset);
    if (NewLength <= OffsetDelta) {
      //
      // The requested offset ends before the child.
      //
      return FALSE;
    }
    //
    // The requested offset ends within the child.
    //
    NewOffset  = 0;
    NewLength -= OffsetDelta;
  }

  if (ChildOffset < 0) {
    NewOffset = (UINT32)(NewOffset + (-ChildOffset));
  }

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
  IN     BOOLEAN                 RequestDraw
  )
{
  BOOLEAN       Result;

  LIST_ENTRY    *ChildEntry;
  GUI_OBJ_CHILD *Child;

  UINT32        ChildDrawOffsetX;
  UINT32        ChildDrawOffsetY;
  UINT32        ChildDrawWidth;
  UINT32        ChildDrawHeight;

  ASSERT (This != NULL);
  ASSERT (This->Width  > OffsetX);
  ASSERT (This->Height > OffsetY);
  ASSERT (DrawContext != NULL);

  for (
    ChildEntry = GetPreviousNode (&This->Children, &This->Children);
    !IsNull (&This->Children, ChildEntry);
    ChildEntry = GetPreviousNode (&This->Children, ChildEntry)
    ) {
    Child = BASE_CR (ChildEntry, GUI_OBJ_CHILD, Link);

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
                 RequestDraw
                 );
  }
}

GUI_OBJ *
GuiObjDelegatePtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     GUI_PTR_EVENT           Event,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     INT64                   OffsetX,
  IN     INT64                   OffsetY
  )
{
  GUI_OBJ       *Obj;
  LIST_ENTRY    *ChildEntry;
  GUI_OBJ_CHILD *Child;

  ASSERT (This != NULL);
  ASSERT (This->Width  > OffsetX);
  ASSERT (This->Height > OffsetY);
  ASSERT (DrawContext != NULL);

  for (
    ChildEntry = GetFirstNode (&This->Children);
    !IsNull (&This->Children, ChildEntry);
    ChildEntry = GetNextNode (&This->Children, ChildEntry)
    ) {
    Child = BASE_CR (ChildEntry, GUI_OBJ_CHILD, Link);
    if (OffsetX  < Child->Obj.OffsetX
     || OffsetX >= Child->Obj.OffsetX + Child->Obj.Width
     || OffsetY  < Child->Obj.OffsetY
     || OffsetY >= Child->Obj.OffsetY + Child->Obj.Height) {
      continue;
    }

    ASSERT (Child->Obj.PtrEvent != NULL);
    Obj = Child->Obj.PtrEvent (
                       &Child->Obj,
                       DrawContext,
                       Context,
                       Event,
                       BaseX   + Child->Obj.OffsetX,
                       BaseY   + Child->Obj.OffsetY,
                       OffsetX - Child->Obj.OffsetX,
                       OffsetY - Child->Obj.OffsetY
                       );
    if (Obj != NULL) {
      return Obj;
    }
  }

  return NULL;
}

#define RGB_APPLY_OPACITY(Rgba, Opacity)  \
  (((Rgba) * (Opacity)) / 0xFF)

#define RGB_ALPHA_BLEND(Back, Front, InvFrontOpacity)  \
  ((Front) + RGB_APPLY_OPACITY (InvFrontOpacity, Back))

VOID
GuiBlendPixel (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel,
  IN     UINT8                                Opacity
  )
{
  UINT8                               CombOpacity;
  UINT8                               InvFrontOpacity;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       OpacFrontPixel;
  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FinalFrontPixel;
  //
  // FIXME: Optimise with SIMD or such.
  // qt_blend_argb32_on_argb32 in QT
  //
  ASSERT (BackPixel != NULL);
  ASSERT (FrontPixel != NULL);

  if (FrontPixel->Reserved == 0) {
    return;
  }

  if (FrontPixel->Reserved == 0xFF) {
    if (Opacity == 0xFF) {
      BackPixel->Blue     = FrontPixel->Blue;
      BackPixel->Green    = FrontPixel->Green;
      BackPixel->Red      = FrontPixel->Red;
      BackPixel->Reserved = FrontPixel->Reserved;
      return;
    }

    CombOpacity = Opacity;
  } else {
    CombOpacity = RGB_APPLY_OPACITY (FrontPixel->Reserved, Opacity);
  }

  if (CombOpacity == 0) {
    return;
  } else if (CombOpacity == FrontPixel->Reserved) {
    FinalFrontPixel = FrontPixel;
  } else {
    OpacFrontPixel.Reserved = CombOpacity;
    OpacFrontPixel.Blue     = RGB_APPLY_OPACITY (FrontPixel->Blue,  Opacity);
    OpacFrontPixel.Green    = RGB_APPLY_OPACITY (FrontPixel->Green, Opacity);
    OpacFrontPixel.Red      = RGB_APPLY_OPACITY (FrontPixel->Red,   Opacity);

    FinalFrontPixel = &OpacFrontPixel;
  }

  InvFrontOpacity = (0xFF - CombOpacity);

  BackPixel->Blue = RGB_ALPHA_BLEND (
                      BackPixel->Blue,
                      FinalFrontPixel->Blue,
                      InvFrontOpacity
                      );
  BackPixel->Green = RGB_ALPHA_BLEND (
                       BackPixel->Green,
                       FinalFrontPixel->Green,
                       InvFrontOpacity
                       );
  BackPixel->Red = RGB_ALPHA_BLEND (
                     BackPixel->Red,
                     FinalFrontPixel->Red,
                     InvFrontOpacity
                     );

  if (BackPixel->Reserved != 0xFF) {
    BackPixel->Reserved = RGB_ALPHA_BLEND (
                            BackPixel->Reserved,
                            CombOpacity,
                            InvFrontOpacity
                            );
  }
}

VOID
GuiDrawToBuffer (
  IN     CONST GUI_IMAGE      *Image,
  IN     UINT8                Opacity,
  IN     BOOLEAN              Fill,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  )
{
  UINT32                              PosBaseX;
  UINT32                              PosBaseY;
  UINT32                              PosOffsetX;
  UINT32                              PosOffsetY;

  UINT32                              RowIndex;
  UINT32                              SourceRowOffset;
  UINT32                              TargetRowOffset;
  UINT32                              SourceColumnOffset;
  UINT32                              TargetColumnOffset;
  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *SourcePixel;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *TargetPixel;
  GUI_DRAW_REQUEST                    ThisReq;
  UINTN                               Index;

  UINT32                              ThisArea;

  UINT32                              ReqWidth;
  UINT32                              ReqHeight;
  UINT32                              ReqArea;

  UINT32                              CombMinX;
  UINT32                              CombMaxX;
  UINT32                              CombMinY;
  UINT32                              CombMaxY;
  UINT32                              CombWidth;
  UINT32                              CombHeight;
  UINT32                              CombArea;

  UINT32                              OverMinX;
  UINT32                              OverMaxX;
  UINT32                              OverMinY;
  UINT32                              OverMaxY;
  UINT32                              OverArea;
  UINT32                              OverWidth;
  UINT32                              OverHeight;

  UINT32                              ActualArea;

  ASSERT (Image != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (DrawContext->Screen != NULL);
  ASSERT (BaseX + OffsetX >= 0);
  ASSERT (BaseY + OffsetY >= 0);
  //
  // Only draw the onscreen parts.
  //
  if (BaseX >= 0) {
    PosBaseX   = (UINT32)BaseX;
    PosOffsetX = OffsetX;
  } else {
    ASSERT (BaseX + OffsetX + Width >= 0);

    PosBaseX = 0;

    if (OffsetX - (-BaseX) >= 0) {
      PosOffsetX = (UINT32)(OffsetX - (-BaseX));
    } else {
      PosOffsetX = 0;
      Width      = (UINT32)(Width - (-(OffsetX - (-BaseX))));
    }
  }

  if (BaseY >= 0) {
    PosBaseY   = (UINT32)BaseY;
    PosOffsetY = OffsetY;
  } else {
    ASSERT (BaseY + OffsetY + Height >= 0);

    PosBaseY = 0;

    if (OffsetY - (-BaseY) >= 0) {
      PosOffsetY = (UINT32)(OffsetY - (-BaseY));
    } else {
      PosOffsetY = 0;
      Height     = (UINT32)(Height - (-(OffsetY - (-BaseY))));
    }
  }

  if (!Fill) {
    ASSERT (Image->Width  > OffsetX);
    ASSERT (Image->Height > OffsetY);
    //
    // Only crop to the image's dimensions when not using fill-drawing.
    //
    Width  = MIN (Width,  Image->Width  - OffsetX);
    Height = MIN (Height, Image->Height - OffsetY);
  }
  //
  // Crop to the screen's dimensions.
  //
  ASSERT (DrawContext->Screen->Width  >= PosBaseX + PosOffsetX);
  ASSERT (DrawContext->Screen->Height >= PosBaseY + PosOffsetY);
  Width  = MIN (Width,  DrawContext->Screen->Width  - (PosBaseX + PosOffsetX));
  Height = MIN (Height, DrawContext->Screen->Height - (PosBaseY + PosOffsetY));

  if (Width == 0 || Height == 0) {
    return;
  }

  ASSERT (Image->Buffer != NULL);

  if (!Fill) {
    //
    // Iterate over each row of the request.
    //
    for (
      RowIndex = 0,
        SourceRowOffset = OffsetY * Image->Width,
        TargetRowOffset = (PosBaseY + PosOffsetY) * DrawContext->Screen->Width;
      RowIndex < Height;
      ++RowIndex,
        SourceRowOffset += Image->Width,
        TargetRowOffset += DrawContext->Screen->Width
      ) {
      //
      // Blend the row pixel-by-pixel.
      //
      for (
        TargetColumnOffset = PosOffsetX, SourceColumnOffset = OffsetX;
        TargetColumnOffset < PosOffsetX + Width;
        ++TargetColumnOffset, ++SourceColumnOffset
        ) {
        TargetPixel = &mScreenBuffer[TargetRowOffset + PosBaseX + TargetColumnOffset];
        SourcePixel = &Image->Buffer[SourceRowOffset + SourceColumnOffset];
        GuiBlendPixel (TargetPixel, SourcePixel, Opacity);
      }
    }
  } else {
    //
    // Iterate over each row of the request.
    //
    for (
      RowIndex = 0,
        TargetRowOffset = (PosBaseY + PosOffsetY) * DrawContext->Screen->Width;
      RowIndex < Height;
      ++RowIndex,
        TargetRowOffset += DrawContext->Screen->Width
      ) {
      //
      // Blend the row pixel-by-pixel with Source's (0,0).
      //
      for (
        TargetColumnOffset = PosOffsetX;
        TargetColumnOffset < PosOffsetX + Width;
        ++TargetColumnOffset
        ) {
        TargetPixel = &mScreenBuffer[TargetRowOffset + PosBaseX + TargetColumnOffset];
        GuiBlendPixel (TargetPixel, &Image->Buffer[0], Opacity);
      }
    }
  }

  if (RequestDraw) {
    //
    // Update the coordinates of the smallest rectangle covering all changes.
    //
    ThisReq.MinX = PosBaseX + PosOffsetX;
    ThisReq.MinY = PosBaseY + PosOffsetY;
    ThisReq.MaxX = PosBaseX + PosOffsetX + Width  - 1;
    ThisReq.MaxY = PosBaseY + PosOffsetY + Height - 1;

    ThisArea = Width * Height;

    for (Index = 0; Index < mNumValidDrawReqs; ++Index) {
      //
      // Calculate several dimensions to determine whether to merge the two
      // draw requests for improved flushing performance.
      //
      ReqWidth  = mDrawRequests[Index].MaxX - mDrawRequests[Index].MinX + 1;
      ReqHeight = mDrawRequests[Index].MaxY - mDrawRequests[Index].MinY + 1;
      ReqArea   = ReqWidth * ReqHeight;

      if (mDrawRequests[Index].MinX < ThisReq.MinX) {
        CombMinX = mDrawRequests[Index].MinX;
        OverMinX = ThisReq.MinX;
      } else {
        CombMinX = ThisReq.MinX;
        OverMinX = mDrawRequests[Index].MinX;
      }

      if (mDrawRequests[Index].MaxX > ThisReq.MaxX) {
        CombMaxX = mDrawRequests[Index].MaxX;
        OverMaxX = ThisReq.MaxX;
      } else {
        CombMaxX = ThisReq.MaxX;
        OverMaxX = mDrawRequests[Index].MaxX;
      }

      if (mDrawRequests[Index].MinY < ThisReq.MinY) {
        CombMinY = mDrawRequests[Index].MinY;
        OverMinY = ThisReq.MinY;
      } else {
        CombMinY = ThisReq.MinY;
        OverMinY = mDrawRequests[Index].MinY;
      }

      if (mDrawRequests[Index].MaxY > ThisReq.MaxY) {
        CombMaxY = mDrawRequests[Index].MaxY;
        OverMaxY = ThisReq.MaxY;
      } else {
        CombMaxY = ThisReq.MaxY;
        OverMaxY = mDrawRequests[Index].MaxY;
      }

      CombWidth  = CombMaxX - CombMinX + 1;
      CombHeight = CombMaxY - CombMinY + 1;
      CombArea   = CombWidth * CombHeight;

      OverArea = 0;
      if (OverMinX <= OverMaxX && OverMinY <= OverMaxY) {
        OverWidth  = OverMaxX - OverMinX + 1;
        OverHeight = OverMaxY - OverMinY + 1;
        OverArea   = OverWidth * OverHeight;
      }

      ActualArea = ThisArea + ReqArea - OverArea;
      //
      // Two requests are merged when their combined actual draw area is at
      // least 3/4 of the area needed to draw both at once.
      //
      if (4 * ActualArea >= 3 * CombArea) {
        mDrawRequests[Index].MinX = CombMinX;
        mDrawRequests[Index].MaxX = CombMaxX;
        mDrawRequests[Index].MinY = CombMinY;
        mDrawRequests[Index].MaxY = CombMaxY;
        return;
      }
    }

    if (mNumValidDrawReqs >= ARRAY_SIZE (mDrawRequests)) {
      ASSERT (FALSE);
      return;
    }

    CopyMem (&mDrawRequests[mNumValidDrawReqs], &ThisReq, sizeof (ThisReq));
    ++mNumValidDrawReqs;
  }
}

VOID
GuiDrawScreen (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                X,
  IN     INT64                Y,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  )
{
  UINT32 PosX;
  UINT32 PosY;

  ASSERT (DrawContext != NULL);
  ASSERT (DrawContext->Screen != NULL);
  //
  // Only draw the onscreen parts.
  //
  if (X >= 0) {
    PosX = (UINT32)X;
  } else {
    if (X + Width <= 0) {
      return;
    }

    Width = (UINT32)(Width - (-X));
    PosX  = 0;
  }

  if (Y >= 0) {
    PosY = (UINT32)Y;
  } else {
    if (Y + Height <= 0) {
      return;
    }

    Height = (UINT32)(Height - (-Y));
    PosY  = 0;
  }

  if (PosX >= DrawContext->Screen->Width
   || PosY >= DrawContext->Screen->Height) {
    return;
  }

  Width  = MIN (Width,  DrawContext->Screen->Width  - PosX);
  Height = MIN (Height, DrawContext->Screen->Height - PosY);

  if (Width == 0 || Height == 0) {
    return;
  }

  ASSERT (DrawContext->Screen->OffsetX == 0);
  ASSERT (DrawContext->Screen->OffsetY == 0);
  ASSERT (DrawContext->Screen->Draw != NULL);
  DrawContext->Screen->Draw (
                         DrawContext->Screen,
                         DrawContext,
                         DrawContext->GuiContext,
                         0,
                         0,
                         PosX,
                         PosY,
                         Width,
                         Height,
                         RequestDraw
                         );
}

VOID
GuiRedrawObject (
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     BOOLEAN              RequestDraw
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);

  GuiDrawScreen (
    DrawContext,
    BaseX,
    BaseY,
    This->Width,
    This->Height,
    RequestDraw
    );
}

VOID
GuiRedrawPointer (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  STATIC UINT32          CursorOldX      = 0;
  STATIC UINT32          CursorOldY      = 0;
  STATIC UINT32          CursorOldWidth  = 0;
  STATIC UINT32          CursorOldHeight = 0;
  STATIC CONST GUI_IMAGE *CursorOldImage = NULL;

  CONST GUI_IMAGE *CursorImage;
  BOOLEAN         RequestDraw;
  UINT32          MinX;
  UINT32          DeltaX;
  UINT32          MinY;
  UINT32          DeltaY;

  ASSERT (DrawContext != NULL);

  ASSERT (DrawContext->GetCursorImage != NULL);
  CursorImage = DrawContext->GetCursorImage (
                               &mScreenViewCursor,
                               DrawContext->GuiContext
                               );
  ASSERT (CursorImage != NULL);

  RequestDraw = FALSE;

  if (mScreenViewCursor.X != CursorOldX || mScreenViewCursor.Y != CursorOldY) {
    //
    // Redraw the cursor when it has been moved.
    //
    RequestDraw = TRUE;
  } else if (CursorImage != CursorOldImage) {
    //
    // Redraw the cursor if its image has changed.
    //
    RequestDraw = TRUE;
  } else if (mNumValidDrawReqs == 0) {
    //
    // Redraw the cursor if nothing else is drawn to always invoke GOP for a
    // more consistent framerate.
    //
    RequestDraw = TRUE;
  }
  //
  // Always drawing the cursor to the buffer increases consistency and is less
  // error-prone to situational hiding.
  //
  // Restore the rectangle previously covered by the cursor.
  // Cover the area of the new cursor too and do not request a draw of the new
  // cursor to not need to merge the requests later.
  //
  if (CursorOldX < mScreenViewCursor.X) {
    MinX   = CursorOldX;
    DeltaX = mScreenViewCursor.X - CursorOldX;
  } else {
    MinX   = mScreenViewCursor.X;
    DeltaX = CursorOldX - mScreenViewCursor.X;
  }

  if (CursorOldY < mScreenViewCursor.Y) {
    MinY   = CursorOldY;
    DeltaY = mScreenViewCursor.Y - CursorOldY;
  } else {
    MinY   = mScreenViewCursor.Y;
    DeltaY = CursorOldY - mScreenViewCursor.Y;
  }

  GuiDrawScreen (
    DrawContext,
    MinX,
    MinY,
    MAX (CursorOldWidth,  CursorImage->Width)  + DeltaX,
    MAX (CursorOldHeight, CursorImage->Height) + DeltaY,
    RequestDraw
    );
  GuiDrawToBuffer (
    CursorImage,
    0xFF,
    FALSE,
    DrawContext,
    mScreenViewCursor.X,
    mScreenViewCursor.Y,
    0,
    0,
    CursorImage->Width,
    CursorImage->Height,
    FALSE
    );

  if (RequestDraw) {
    CursorOldX      = mScreenViewCursor.X;
    CursorOldY      = mScreenViewCursor.Y;
    CursorOldWidth  = CursorImage->Width;
    CursorOldHeight = CursorImage->Height;
    CursorOldImage  = CursorImage;
  } else {
    ASSERT (CursorOldX      == mScreenViewCursor.X);
    ASSERT (CursorOldY      == mScreenViewCursor.Y);
    ASSERT (CursorOldWidth  == CursorImage->Width);
    ASSERT (CursorOldHeight == CursorImage->Height);
    ASSERT (CursorOldImage  == CursorImage);
  }
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
  EFI_TPL OldTpl;

  UINTN   NumValidDrawReqs;
  UINTN   Index;

  UINT64  EndTsc;
  UINT64  DeltaTsc;

  BOOLEAN Interrupts;

  ASSERT (DrawContext != NULL);
  ASSERT (DrawContext->Screen != NULL);

  GuiRedrawPointer (DrawContext);

  NumValidDrawReqs = mNumValidDrawReqs;
  ASSERT (NumValidDrawReqs <= ARRAY_SIZE (mDrawRequests));

  mNumValidDrawReqs = 0;

  for (Index = 0; Index < NumValidDrawReqs; ++Index) {
    ASSERT (mDrawRequests[Index].MaxX >= mDrawRequests[Index].MinX);
    ASSERT (mDrawRequests[Index].MaxY >= mDrawRequests[Index].MinY);
    //
    // Set MaxX/Y to Width and Height as the requests are invalidated anyway.
    //
    mDrawRequests[Index].MaxX -= mDrawRequests[Index].MinX - 1;
    mDrawRequests[Index].MaxY -= mDrawRequests[Index].MinY - 1;
  }
  //
  // Raise the TPL to not interrupt timing or flushing.
  //
  OldTpl     = gBS->RaiseTPL (TPL_NOTIFY);
  Interrupts = SaveAndDisableInterrupts ();

  EndTsc   = AsmReadTsc ();
  DeltaTsc = EndTsc - mStartTsc;
  if (DeltaTsc < mDeltaTscTarget) {
    EndTsc = InternalCpuDelayTsc (mDeltaTscTarget - DeltaTsc);
  }

  for (Index = 0; Index < NumValidDrawReqs; ++Index) {
    //
    // Due to above's loop, MaxX/Y correspond to Width and Height here.
    //
    GuiOutputBlt (
      mOutputContext,
      mScreenBuffer,
      EfiBltBufferToVideo,
      mDrawRequests[Index].MinX,
      mDrawRequests[Index].MinY,
      mDrawRequests[Index].MinX,
      mDrawRequests[Index].MinY,
      mDrawRequests[Index].MaxX,
      mDrawRequests[Index].MaxY,
      mScreenBufferDelta
      );
  }

  if (Interrupts) {
    EnableInterrupts ();
  }
  gBS->RestoreTPL (OldTpl);
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
  ASSERT (DrawContext->Screen != NULL);

  mStartTsc = AsmReadTsc ();

  GuiRedrawObject (DrawContext->Screen, DrawContext, 0, 0, TRUE);
  GuiFlushScreen (DrawContext);
}

EFI_STATUS
GuiLibConstruct (
  IN OC_PICKER_CONTEXT  *PickerContet,
  IN UINT32             CursorDefaultX,
  IN UINT32             CursorDefaultY
  )
{
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *OutputInfo;

  mOutputContext = GuiOutputConstruct ();
  if (mOutputContext == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to initialise output\n"));
    return EFI_UNSUPPORTED;
  }

  OutputInfo = GuiOutputGetInfo (mOutputContext);
  ASSERT (OutputInfo != NULL);

  CursorDefaultX = MIN (CursorDefaultX, OutputInfo->HorizontalResolution - 1);
  CursorDefaultY = MIN (CursorDefaultY, OutputInfo->VerticalResolution   - 1);

  mPointerContext = GuiPointerConstruct (
    PickerContet,
    CursorDefaultX,
    CursorDefaultY,
    OutputInfo->HorizontalResolution,
    OutputInfo->VerticalResolution
    );
  if (mPointerContext == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to initialise pointer\n"));
  }

  mKeyContext = GuiKeyConstruct (PickerContet);
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

  mScreenViewCursor.X = CursorDefaultX;
  mScreenViewCursor.Y = CursorDefaultY;

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
  OUT    GUI_DRAWING_CONTEXT     *DrawContext,
  IN OUT GUI_OBJ                 *Screen,
  IN     GUI_CURSOR_GET_IMAGE    GetCursorImage,
  IN     GUI_EXIT_LOOP           ExitLoop,
  IN     BOOT_PICKER_GUI_CONTEXT *GuiContext
  )
{
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *OutputInfo;

  ASSERT (DrawContext != NULL);
  ASSERT (Screen != NULL);
  ASSERT (GetCursorImage != NULL);
  ASSERT (ExitLoop != NULL);

  OutputInfo = GuiOutputGetInfo (mOutputContext);
  ASSERT (OutputInfo != NULL);

  Screen->Width  = OutputInfo->HorizontalResolution;
  Screen->Height = OutputInfo->VerticalResolution;

  DrawContext->Screen         = Screen;
  DrawContext->GetCursorImage = GetCursorImage;
  DrawContext->ExitLoop       = ExitLoop;
  DrawContext->GuiContext     = GuiContext;
  InitializeListHead (&DrawContext->Animations);
}

VOID
GuiViewDeinitialize (
  IN OUT    GUI_DRAWING_CONTEXT   *DrawContext
  )
{
  ZeroMem (DrawContext, sizeof (*DrawContext));
}

CONST GUI_SCREEN_CURSOR *
GuiViewCurrentCursor (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  return &mScreenViewCursor;
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

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (DrawContext->Screen->OffsetX == 0);
  ASSERT (DrawContext->Screen->OffsetY == 0);
  ASSERT (BaseX != NULL);
  ASSERT (BaseY != NULL);

  X   = 0;
  Y   = 0;
  Obj = This;
  while (Obj != DrawContext->Screen) {
    X += Obj->OffsetX;
    Y += Obj->OffsetY;

    ChildObj = BASE_CR (Obj, GUI_OBJ_CHILD, Obj);
    Obj      = ChildObj->Parent;
    ASSERT (Obj != NULL);
    ASSERT (IsNodeInList (&Obj->Children, &ChildObj->Link));
  }

  *BaseX = X;
  *BaseY = Y;
}

VOID
GuiDrawLoop (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     UINT32               TimeOutSeconds
  )
{
  EFI_STATUS          Status;
  BOOLEAN             Result;

  INTN                InputKey;
  BOOLEAN             Modifier;
  GUI_POINTER_STATE   PointerState;
  GUI_OBJ             *HoldObject;
  INT64               HoldObjBaseX;
  INT64               HoldObjBaseY;
  CONST LIST_ENTRY    *AnimEntry;
  CONST GUI_ANIMATION *Animation;
  UINT64              LoopStartTsc;

  ASSERT (DrawContext != NULL);

  mNumValidDrawReqs = 0;
  HoldObject        = NULL;

  GuiRedrawAndFlushScreen (DrawContext);
  //
  // Clear previous inputs.
  //
  GuiPointerReset (mPointerContext);
  GuiKeyReset (mKeyContext);
  //
  // Main drawing loop, time and derieve sub-frequencies as required.
  //
  LoopStartTsc = mStartTsc = AsmReadTsc ();
  do {
    if (mPointerContext != NULL) {
      //
      // Process pointer events.
      //
      Status = GuiPointerGetState (mPointerContext, &PointerState);
      if (!EFI_ERROR (Status)) {
        mScreenViewCursor.X = PointerState.X;
        mScreenViewCursor.Y = PointerState.Y;

        if (HoldObject == NULL && PointerState.PrimaryDown) {
          HoldObject = GuiObjDelegatePtrEvent (
                          DrawContext->Screen,
                          DrawContext,
                          DrawContext->GuiContext,
                          GuiPointerPrimaryDown,
                          0,
                          0,
                          PointerState.X,
                          PointerState.Y
                          );
        }
      }

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
                      !PointerState.PrimaryDown ? GuiPointerPrimaryUp : GuiPointerPrimaryHold,
                      HoldObjBaseX,
                      HoldObjBaseY,
                      (INT64)PointerState.X - HoldObjBaseX,
                      (INT64)PointerState.Y - HoldObjBaseY
                      );
        if (!PointerState.PrimaryDown) {
          HoldObject = NULL;
        }
      }
    }

    if (mKeyContext != NULL) {
      //
      // Process key events. Only allow one key at a time for now.
      //
      Status = GuiKeyRead (mKeyContext, &InputKey, &Modifier);
      if (!EFI_ERROR (Status)) {
        ASSERT (DrawContext->Screen->KeyEvent != NULL);
        DrawContext->Screen->KeyEvent (
                               DrawContext->Screen,
                               DrawContext,
                               DrawContext->GuiContext,
                               0,
                               0,
                               InputKey,
                               Modifier
                               );
        //
        // If detected key press then disable menu timeout
        //
        TimeOutSeconds = 0;
      }
    }

    STATIC UINT64 FrameTime = 0;
    //
    // Process queued animations.
    //
    AnimEntry = GetFirstNode (&DrawContext->Animations);
    while (!IsNull (&DrawContext->Animations, AnimEntry)) {
      Animation = BASE_CR (AnimEntry, GUI_ANIMATION, Link);
      Result = Animation->Animate (Animation->Context, DrawContext, FrameTime);

      AnimEntry = GetNextNode (&DrawContext->Animations, AnimEntry);

      if (Result) {
        RemoveEntryList (&Animation->Link);
      }
    }
    ++FrameTime;
    //
    // Flush the changes performed in this refresh iteration.
    //
    GuiFlushScreen (DrawContext);

    //
    // Exit early if reach timer timeout and timer isn't disabled due to key event
    //
    if (TimeOutSeconds > 0
      && GetTimeInNanoSecond (AsmReadTsc () - LoopStartTsc) >= TimeOutSeconds * 1000000000ULL) {
      //
      // FIXME: There should be view function or alike.
      //
      mGuiContext.BootEntry = mBootPicker.SelectedEntry->Context;
      break;
    }
  } while (!DrawContext->ExitLoop (DrawContext->GuiContext));
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
    DrawContext->Screen->Width,
    DrawContext->Screen->Height,
    0
    );
}

EFI_STATUS
GuiIcnsToImageIcon (
  OUT GUI_IMAGE  *Image,
  IN  VOID       *IcnsImage,
  IN  UINT32     IcnsImageSize,
  IN  UINT8      Scale,
  IN  UINT32     MatchWidth,
  IN  UINT32     MatchHeight,
  IN  BOOLEAN    AllowLess
  )
{
  EFI_STATUS         Status;
  UINT32             Offset;
  UINT32             RecordLength;
  UINT32             ImageSize;
  UINT32             DecodedBytes;
  APPLE_ICNS_RECORD  *Record;
  APPLE_ICNS_RECORD  *RecordIT32;
  APPLE_ICNS_RECORD  *RecordT8MK;

  ASSERT (Scale == 1 || Scale == 2);

  //
  // We do not need to support 'it32' 128x128 icon format,
  // because Finder automatically converts the icons to PNG-based
  // when assigning volume icon.
  //

  if (IcnsImageSize < sizeof (APPLE_ICNS_RECORD)*2) {
    return EFI_INVALID_PARAMETER;
  }

  Record = IcnsImage;
  if (Record->Type != APPLE_ICNS_MAGIC || SwapBytes32 (Record->Size) != IcnsImageSize) {
    return EFI_SECURITY_VIOLATION;
  }

  RecordIT32 = NULL;
  RecordT8MK = NULL;

  Offset  = sizeof (APPLE_ICNS_RECORD);
  while (Offset < IcnsImageSize - sizeof (APPLE_ICNS_RECORD)) {
    Record       = (APPLE_ICNS_RECORD *) ((UINT8 *) IcnsImage + Offset);
    RecordLength = SwapBytes32 (Record->Size);

    //
    // 1. Record smaller than its header and 1 32-bit word is invalid.
    //    32-bit is required by some entries like IT32 (see below).
    // 2. Record overflowing UINT32 is invalid.
    // 3. Record larger than file size is invalid.
    //
    if (RecordLength < sizeof (APPLE_ICNS_RECORD) + sizeof (UINT32)
      || OcOverflowAddU32 (Offset, RecordLength, &Offset)
      || Offset > IcnsImageSize) {
      return EFI_SECURITY_VIOLATION;
    }

    if ((Scale == 1 && Record->Type == APPLE_ICNS_IC07)
      || (Scale == 2 && Record->Type == APPLE_ICNS_IC13)) {
      Status = GuiPngToImage (
        Image,
        Record->Data,
        RecordLength - sizeof (APPLE_ICNS_RECORD),
        TRUE
        );

      if (!EFI_ERROR (Status) && MatchWidth > 0 && MatchHeight > 0) {
        if (AllowLess
          ? (Image->Width >  MatchWidth * Scale || Image->Height >  MatchWidth * Scale
          || Image->Width == 0 || Image->Height == 0)
          : (Image->Width != MatchWidth * Scale || Image->Height != MatchHeight * Scale)) {
          FreePool (Image->Buffer);
          DEBUG ((
            DEBUG_INFO,
            "OCUI: Expected %dx%d, actual %dx%d, allow less: %d\n",
             MatchWidth * Scale,
             MatchHeight * Scale,
             Image->Width,
             Image->Height,
             AllowLess
            ));
          Status = EFI_UNSUPPORTED;
        }
      }

      return Status;
    }

    if (Scale == 1) {
      if (Record->Type == APPLE_ICNS_IT32) {
        RecordIT32 = Record;
      } else if (Record->Type == APPLE_ICNS_T8MK) {
        RecordT8MK = Record;
      }

      if (RecordT8MK != NULL && RecordIT32 != NULL) {
        Image->Width  = MatchWidth;
        Image->Height = MatchHeight;
        ImageSize     = (MatchWidth * MatchHeight) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
        Image->Buffer = AllocateZeroPool (ImageSize);

        if (Image->Buffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // We have to add an additional UINT32 for IT32, since it has a reserved field.
        //
        DecodedBytes = DecompressMaskedRLE24 (
          (UINT8 *) Image->Buffer,
          ImageSize,
          RecordIT32->Data + sizeof (UINT32),
          SwapBytes32 (RecordIT32->Size) - sizeof (APPLE_ICNS_RECORD) - sizeof (UINT32),
          RecordT8MK->Data,
          SwapBytes32 (RecordT8MK->Size) - sizeof (APPLE_ICNS_RECORD),
          TRUE
          );

        if (DecodedBytes != ImageSize) {
          FreePool (Image->Buffer);
          return EFI_UNSUPPORTED;
        }

        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
GuiLabelToImage (
  OUT GUI_IMAGE *Image,
  IN  VOID      *RawData,
  IN  UINT32    DataLength,
  IN  UINT8     Scale,
  IN  BOOLEAN   Inverted
  )
{
  APPLE_DISK_LABEL  *Label;
  UINT32            PixelIdx;

  ASSERT (RawData != NULL);
  ASSERT (Scale == 1 || Scale == 2);

  if (DataLength < sizeof (APPLE_DISK_LABEL)) {
    return EFI_INVALID_PARAMETER;
  }

  Label = RawData;
  Image->Width  = SwapBytes16 (Label->Width);
  Image->Height = SwapBytes16 (Label->Height);

  if (Image->Width > APPLE_DISK_LABEL_MAX_WIDTH * Scale
    || Image->Height > APPLE_DISK_LABEL_MAX_HEIGHT * Scale
    || DataLength != sizeof (APPLE_DISK_LABEL) + Image->Width * Image->Height) {
    DEBUG ((DEBUG_INFO, "OCUI: Invalid label has %dx%d dims at %u size\n", Image->Width, Image->Height, DataLength));
    return EFI_SECURITY_VIOLATION;
  }

  Image->Buffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocatePool (
    Image->Width * Image->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    );

  if (Image->Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Inverted) {
    for (PixelIdx = 0; PixelIdx < Image->Width * Image->Height; PixelIdx++) {
      Image->Buffer[PixelIdx].Blue     = 0;
      Image->Buffer[PixelIdx].Green    = 0;
      Image->Buffer[PixelIdx].Red      = 0;
      Image->Buffer[PixelIdx].Reserved = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
    }
  } else {
    for (PixelIdx = 0; PixelIdx < Image->Width * Image->Height; PixelIdx++) {
      Image->Buffer[PixelIdx].Blue     = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
      Image->Buffer[PixelIdx].Green    = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
      Image->Buffer[PixelIdx].Red      = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
      Image->Buffer[PixelIdx].Reserved = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GuiPngToImage (
  OUT GUI_IMAGE  *Image,
  IN  VOID       *ImageData,
  IN  UINTN      ImageDataSize,
  IN  BOOLEAN    PremultiplyAlpha
  )
{
  EFI_STATUS                       Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BufferWalker;
  UINTN                            Index;
  UINT8                            TmpChannel;

  Status = DecodePng (
    ImageData,
    ImageDataSize,
    (VOID **) &Image->Buffer,
    &Image->Width,
    &Image->Height,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCUI: DecodePNG - %r\n", Status));
    return Status;
  }

  if (PremultiplyAlpha) {
    BufferWalker = Image->Buffer;
    for (Index = 0; Index < (UINTN) Image->Width * Image->Height; ++Index) {
      TmpChannel             = (UINT8) ((BufferWalker->Blue * BufferWalker->Reserved) / 0xFF);
      BufferWalker->Blue     = (UINT8) ((BufferWalker->Red * BufferWalker->Reserved) / 0xFF);
      BufferWalker->Green    = (UINT8) ((BufferWalker->Green * BufferWalker->Reserved) / 0xFF);
      BufferWalker->Red      = TmpChannel;
      ++BufferWalker;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GuiCreateHighlightedImage (
  OUT GUI_IMAGE                            *SelectedImage,
  IN  CONST GUI_IMAGE                      *SourceImage,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL PremulPixel;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
  UINT32                        ColumnOffset;
  BOOLEAN                       OneSet;
  UINT32                        FirstUnsetX;
  UINT32                        IndexY;
  UINT32                        RowOffset;

  ASSERT (SelectedImage != NULL);
  ASSERT (SourceImage != NULL);
  ASSERT (SourceImage->Buffer != NULL);
  ASSERT (HighlightPixel != NULL);
  //
  // The multiplication cannot wrap around because the original allocation sane.
  //
  Buffer = AllocateCopyPool (
             SourceImage->Width * SourceImage->Height * sizeof (*SourceImage->Buffer),
             SourceImage->Buffer
             );
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PremulPixel.Blue     = (UINT8)((HighlightPixel->Blue  * HighlightPixel->Reserved) / 0xFF);
  PremulPixel.Green    = (UINT8)((HighlightPixel->Green * HighlightPixel->Reserved) / 0xFF);
  PremulPixel.Red      = (UINT8)((HighlightPixel->Red   * HighlightPixel->Reserved) / 0xFF);
  PremulPixel.Reserved = HighlightPixel->Reserved;

  for (
    IndexY = 0, RowOffset = 0;
    IndexY < SourceImage->Height;
    ++IndexY, RowOffset += SourceImage->Width
    ) {
    FirstUnsetX = 0;
    OneSet      = FALSE;

    for (ColumnOffset = 0; ColumnOffset < SourceImage->Width; ++ColumnOffset) {
      if (SourceImage->Buffer[RowOffset + ColumnOffset].Reserved != 0) {
        OneSet = TRUE;
        GuiBlendPixel (
          &Buffer[RowOffset + ColumnOffset],
          &PremulPixel,
          0xFF
          );
        if (FirstUnsetX != 0) {
          //
          // Set all fully transparent pixels between two not fully transparent
          // pixels to the highlighter pixel.
          //
          while (FirstUnsetX < ColumnOffset) {
            CopyMem (
              &Buffer[RowOffset + FirstUnsetX],
              &PremulPixel,
              sizeof (*Buffer)
              );
            ++FirstUnsetX;
          }

          FirstUnsetX = 0;
        }
      } else if (FirstUnsetX == 0 && OneSet) {
        FirstUnsetX = ColumnOffset;
      }
    }
  }

  SelectedImage->Width  = SourceImage->Width;
  SelectedImage->Height = SourceImage->Height;
  SelectedImage->Buffer = Buffer;
  return EFI_SUCCESS;
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
