/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "OcConsoleLibInternal.h"

#include <Uefi.h>
#include <Guid/AppleVariable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>

STATIC BOOLEAN          mIsDefaultFont;
STATIC OC_CONSOLE_FONT  *mConsoleFont;

STATIC UINT32  mGraphicsEfiColors[16] = {
  0x00000000,  // BLACK
  0x00000098,  // LIGHTBLUE
  0x00009800,  // LIGHTGREEN
  0x00009898,  // LIGHTCYAN
  0x00980000,  // LIGHTRED
  0x00980098,  // MAGENTA
  0x00989800,  // BROWN
  0x00bfbfbf,  // LIGHTGRAY
  0x00303030,  // DARKGRAY - BRIGHT BLACK
  0x000000ff,  // BLUE
  0x0000ff00,  // LIME
  0x0000ffff,  // CYAN
  0x00ff0000,  // RED
  0x00ff00ff,  // FUCHSIA
  0x00ffff00,  // YELLOW
  0x00ffffff   // WHITE
};

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL         *mGraphicsOutput;
STATIC UINTN                                mConsolePaddingX;
STATIC UINTN                                mConsolePaddingY;
STATIC UINTN                                mUserWidth;
STATIC UINTN                                mUserHeight;
STATIC UINTN                                mConsoleWidth;
STATIC UINTN                                mConsoleHeight;
STATIC UINTN                                mConsoleMaxPosX;
STATIC UINTN                                mConsoleMaxPosY;
STATIC BOOLEAN                              mConsoleUncontrolled;
STATIC UINTN                                mPrivateColumn; ///< At least UEFI Shell trashes Mode values.
STATIC UINTN                                mPrivateRow;    ///< At least UEFI Shell trashes Mode values.
STATIC UINT32                               mConsoleGopMode;
STATIC UINT8                                mUIScale;
STATIC UINT8                                mFontScale;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  mBackgroundColor;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  mForegroundColor;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  *mCharacterBuffer;
STATIC EFI_CONSOLE_CONTROL_SCREEN_MODE      mConsoleMode = EfiConsoleControlScreenText;

#define TGT_CHAR_WIDTH     ((UINTN)(ISO_CHAR_WIDTH) * mFontScale)
#define TGT_CHAR_HEIGHT    ((UINTN)(ISO_CHAR_HEIGHT) * mFontScale)
#define TGT_CHAR_AREA      ((TGT_CHAR_WIDTH) * (TGT_CHAR_HEIGHT))
#define TGT_PADD_WIDTH     (mConsolePaddingX)
#define TGT_PADD_HEIGHT    (mConsolePaddingY)
#define TGT_CURSOR_X       mFontScale
#define TGT_CURSOR_Y       ((TGT_CHAR_HEIGHT) - mFontScale)
#define TGT_CURSOR_WIDTH   ((TGT_CHAR_WIDTH) - mFontScale * 2)
#define TGT_CURSOR_HEIGHT  (mFontScale)

#define MIN_SUPPORTED_CONSOLE_WIDTH   (80)
#define MIN_SUPPORTED_CONSOLE_HEIGHT  (25)

/**
  @param[in]  ConsoleFont           Font to use.
  @param[in]  Char                  Char for which to find information.
  @param[out] Page                  Font page in which char (or fallback char) is found.
  @param[out] GlyphIndex            Index to glyph for char (or fallback char).
  @param[in]  UseFallback           Whether to use fallback char if char is not found.

  @retval EFI_SUCCESS               Successfully returned font data for character.
  @retval EFI_WARN_UNKNOWN_GLYPH    Returned font data for fallback character because requested
                                    character was not in font.
  @retval EFI_LOAD_ERROR            No valid font data returned. Will only occur in abnormal circumstances
                                    (namely, broken font data), or when UseFallback is FALSE.
**/
STATIC
EFI_STATUS
GetConsoleFontCharInfo (
  IN  OC_CONSOLE_FONT       *ConsoleFont,
  IN  CHAR16                Char,
  OUT OC_CONSOLE_FONT_PAGE  **Page,
  OUT UINT8                 *GlyphIndex,
  IN  BOOLEAN               UseFallback
  )
{
  EFI_STATUS  Status;
  UINT16      PageNumber;
  UINT8       PageChar;
  UINT16      PageIndex;

  ASSERT (Page != NULL);
  ASSERT (GlyphIndex != NULL);

  PageNumber = (Char >> 7) & 0x1FF;
  PageChar   = Char & 0x7F;

  *GlyphIndex = 0;

  Status = EFI_SUCCESS;
  do {
    if ((PageNumber >= ConsoleFont->PageMin) && (PageNumber < ConsoleFont->PageMax)) {
      PageIndex = PageNumber - ConsoleFont->PageMin + 1;
      if (ConsoleFont->PageOffsets != NULL) {
        PageIndex = ConsoleFont->PageOffsets[PageIndex - 1];
      }

      if (PageIndex != 0) {
        *Page = &ConsoleFont->Pages[PageIndex - 1];
        if ((PageChar >= (*Page)->CharMin) && (PageChar < (*Page)->CharMax)) {
          ASSERT ((*Page)->Glyphs != NULL);
          if ((*Page)->GlyphOffsets == NULL) {
            *GlyphIndex = PageChar - (*Page)->CharMin + 1;
          } else {
            *GlyphIndex = (*Page)->GlyphOffsets[PageChar - (*Page)->CharMin];
          }
        }
      }
    }

    if (*GlyphIndex == 0) {
      //
      // Give up and render nothing if fallback char is not present.
      //
      ASSERT (Status == EFI_SUCCESS); ///< NB EFI_WARN_... are not errors
      if (!UseFallback || (Status != EFI_SUCCESS)) {
        return EFI_LOAD_ERROR;
      }

      Status = EFI_WARN_UNKNOWN_GLYPH;

      //
      // For default font, render all unknown chars in provided page(s) as space (following the explicit
      // spaces in free console font used in XNU); in other pages and in user fonts use underscore.
      //
      STATIC_ASSERT (ISO_FONT_MAX_PAGE == 1 || ISO_FONT_MAX_PAGE == 2, "Invalid ISO_FONT_MAX_PAGE value");
      if (mIsDefaultFont && (PageNumber < ISO_FONT_MAX_PAGE)) {
        PageChar = L' ';
      } else {
        PageChar = OC_CONSOLE_FONT_FALLBACK_CHAR;
      }

      PageNumber = 0;
    }
  } while (*GlyphIndex == 0);

  return Status;
}

BOOLEAN
OcConsoleFontContainsChar (
  IN OC_CONSOLE_FONT  *ConsoleFont,
  IN CHAR16           Char
  )
{
  EFI_STATUS            Status;
  OC_CONSOLE_FONT_PAGE  *Page;
  UINT8                 GlyphIndex;

  Status = GetConsoleFontCharInfo (ConsoleFont, Char, &Page, &GlyphIndex, FALSE);

  return !EFI_ERROR (Status);
}

/**
  Render character onscreen.

  @param[in]  Char  Character code.
  @param[in]  PosX  Character X position.
  @param[in]  PosY  Character Y position.
**/
STATIC
VOID
RenderChar (
  IN CHAR16  Char,
  IN UINTN   PosX,
  IN UINTN   PosY
  )
{
  UINT32                *DstBuffer;
  UINT8                 *SrcBuffer;
  OC_CONSOLE_FONT_PAGE  *Page;
  UINT8                 Line;
  UINT32                Index;
  UINT32                Index2;
  UINT8                 Mask;
  UINT8                 FontHead;
  UINT8                 FontTail;
  UINT8                 GlyphIndex;
  BOOLEAN               LeftToRight;
  EFI_STATUS            Status;

  DstBuffer = &mCharacterBuffer[0].Raw;

  Status = GetConsoleFontCharInfo (mConsoleFont, Char, &Page, &GlyphIndex, TRUE);

  if (EFI_ERROR (Status)) {
    return;
  }

  FontHead    = Page->FontHead;
  FontTail    = Page->FontTail;
  LeftToRight = Page->LeftToRight;

  SrcBuffer = Page->Glyphs + ((GlyphIndex - 1) * (ISO_CHAR_HEIGHT - FontHead - FontTail));

  for (Line = 0; Line < FontHead; ++Line) {
    //
    // Apply scale twice, for width and height.
    //
    SetMem32 (DstBuffer, TGT_CHAR_WIDTH * mFontScale * sizeof (DstBuffer[0]), mBackgroundColor.Raw);
    DstBuffer += TGT_CHAR_WIDTH * mFontScale;
  }

  for ( ; Line < ISO_CHAR_HEIGHT - FontTail; ++Line) {
    //
    // Iterate, while single bit scans font.
    //
    for (Index = 0; Index < mFontScale; ++Index) {
      Mask = LeftToRight ? 0x80 : 1;
      do {
        for (Index2 = 0; Index2 < mFontScale; ++Index2) {
          *DstBuffer = (*SrcBuffer & Mask) ? mForegroundColor.Raw : mBackgroundColor.Raw;
          ++DstBuffer;
        }

        if (LeftToRight) {
          Mask >>= 1U;
        } else {
          Mask <<= 1U;
        }
      } while (Mask != 0);
    }

    ++SrcBuffer;
  }

  for ( ; Line < ISO_CHAR_HEIGHT; ++Line) {
    SetMem32 (DstBuffer, TGT_CHAR_WIDTH * mFontScale * sizeof (DstBuffer[0]), mBackgroundColor.Raw);
    DstBuffer += TGT_CHAR_WIDTH * mFontScale;
  }

  ASSERT (DstBuffer - &mCharacterBuffer[0].Raw == (INTN)TGT_CHAR_AREA);

  mGraphicsOutput->Blt (
                     mGraphicsOutput,
                     &mCharacterBuffer[0].Pixel,
                     EfiBltBufferToVideo,
                     0,
                     0,
                     TGT_PADD_WIDTH  + PosX * TGT_CHAR_WIDTH,
                     TGT_PADD_HEIGHT + PosY * TGT_CHAR_HEIGHT,
                     TGT_CHAR_WIDTH,
                     TGT_CHAR_HEIGHT,
                     0
                     );
}

/**
  Swap cursor visibility onscreen.

  @param[in]  Enabled  Whether cursor is visible.
  @param[in]  PosX     Character X position.
  @param[in]  PosY     Character Y position.
**/
STATIC
VOID
FlushCursor (
  IN BOOLEAN  Enabled,
  IN UINTN    PosX,
  IN UINTN    PosY
  )
{
  EFI_STATUS                           Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Colour;

  if (!Enabled || (mConsoleMode != EfiConsoleControlScreenText)) {
    return;
  }

  //
  // UEFI only has one cursor at a time. UEFI Shell edit command has a cursor and a mouse
  // pointer, which are not connected. To be able to draw both at a time UEFI Shell constantly
  // redraws both the cursor and the mouse pointer. To do that it constantly flips bg and fg
  // colours as well as cursor visibility.
  // It seems that the Shell implementation relies on an undocumented feature (is that a bug?)
  // of hiding an already drawn cursor with a space with inverted attributes.
  // This is weird but EDK II implementation seems to match the logic, and as a result we
  // track cursor visibility or easily optimise this logic.
  //
  Status = mGraphicsOutput->Blt (
                              mGraphicsOutput,
                              &Colour.Pixel,
                              EfiBltVideoToBltBuffer,
                              TGT_PADD_WIDTH  + PosX * TGT_CHAR_WIDTH  + TGT_CURSOR_X,
                              TGT_PADD_HEIGHT + PosY * TGT_CHAR_HEIGHT + TGT_CURSOR_Y,
                              0,
                              0,
                              1,
                              1,
                              0
                              );

  if (EFI_ERROR (Status)) {
    return;
  }

  mGraphicsOutput->Blt (
                     mGraphicsOutput,
                     Colour.Raw == mForegroundColor.Raw ? &mBackgroundColor.Pixel : &mForegroundColor.Pixel,
                     EfiBltVideoFill,
                     0,
                     0,
                     TGT_PADD_WIDTH  + PosX * TGT_CHAR_WIDTH  + TGT_CURSOR_X,
                     TGT_PADD_HEIGHT + PosY * TGT_CHAR_HEIGHT + TGT_CURSOR_Y,
                     TGT_CURSOR_WIDTH,
                     TGT_CURSOR_HEIGHT,
                     0
                     );
}

STATIC
VOID
RenderScroll (
  VOID
  )
{
  UINTN  Width;

  //
  // Move used screen region.
  //
  Width = (mConsoleMaxPosX + 1) * TGT_CHAR_WIDTH;
  mGraphicsOutput->Blt (
                     mGraphicsOutput,
                     NULL,
                     EfiBltVideoToVideo,
                     TGT_PADD_WIDTH,
                     TGT_PADD_HEIGHT + TGT_CHAR_HEIGHT,
                     TGT_PADD_WIDTH,
                     TGT_PADD_HEIGHT,
                     Width,
                     TGT_CHAR_HEIGHT * (mConsoleHeight - 1),
                     0
                     );

  //
  // Erase last line.
  //
  mGraphicsOutput->Blt (
                     mGraphicsOutput,
                     &mBackgroundColor.Pixel,
                     EfiBltVideoFill,
                     0,
                     0,
                     TGT_PADD_WIDTH,
                     TGT_PADD_HEIGHT + TGT_CHAR_HEIGHT * (mConsoleHeight - 1),
                     Width,
                     TGT_CHAR_HEIGHT,
                     0
                     );
}

//
// Resync - called on detected change of GOP mode and on reset.
//
STATIC
EFI_STATUS
RenderResync (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINTN                                 MaxWidth;
  UINTN                                 MaxHeight;

  Info = mGraphicsOutput->Mode->Info;

  //
  // Require space for at least 1x1 chars on the calculation below.
  //
  if (  (Info->HorizontalResolution < ISO_CHAR_WIDTH)
     || (Info->VerticalResolution   < ISO_CHAR_HEIGHT))
  {
    return EFI_LOAD_ERROR;
  }

  if (mCharacterBuffer != NULL) {
    FreePool (mCharacterBuffer);
  }

  //
  // Reset font scale and allocate for target size - may be over-allocated if we have to override below.
  //
  mFontScale       = mUIScale;
  mCharacterBuffer = AllocatePool (TGT_CHAR_AREA * sizeof (mCharacterBuffer[0]));
  if (mCharacterBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mConsoleGopMode = mGraphicsOutput->Mode->Mode;

  //
  // Override font scale to reach minimum supported text resolution, if needed and possible.
  //
  while (TRUE) {
    MaxWidth  = Info->HorizontalResolution / TGT_CHAR_WIDTH;
    MaxHeight = Info->VerticalResolution / TGT_CHAR_HEIGHT;
    if (  (MaxWidth  >= MIN_SUPPORTED_CONSOLE_WIDTH)
       && (MaxHeight >= MIN_SUPPORTED_CONSOLE_HEIGHT))
    {
      break;
    }

    if (mFontScale == 1) {
      break;
    }

    mFontScale = 1;
  }

  if ((mUserWidth == 0) || (mUserHeight == 0)) {
    mConsoleWidth  = MaxWidth;
    mConsoleHeight = MaxHeight;
  } else {
    mConsoleWidth  = MIN (MaxWidth, mUserWidth);
    mConsoleHeight = MIN (MaxHeight, mUserHeight);
  }

  mConsolePaddingX     = (Info->HorizontalResolution - (mConsoleWidth * TGT_CHAR_WIDTH)) / 2;
  mConsolePaddingY     = (Info->VerticalResolution - (mConsoleHeight * TGT_CHAR_HEIGHT)) / 2;
  mConsoleMaxPosX      = 0;
  mConsoleMaxPosY      = 0;
  mConsoleUncontrolled = FALSE;

  mPrivateColumn           = mPrivateRow = 0;
  This->Mode->CursorColumn = This->Mode->CursorRow = 0;

  //
  // Avoid rendering any console content when in graphics mode.
  //
  if (mConsoleMode == EfiConsoleControlScreenText) {
    mGraphicsOutput->Blt (
                       mGraphicsOutput,
                       &mBackgroundColor.Pixel,
                       EfiBltVideoFill,
                       0,
                       0,
                       0,
                       0,
                       Info->HorizontalResolution,
                       Info->VerticalResolution,
                       0
                       );
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextResetEx (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          ExtendedVerification,
  IN BOOLEAN                          Debug
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = OcHandleProtocolFallback (
             gST->ConsoleOutHandle,
             &gEfiGraphicsOutputProtocolGuid,
             (VOID **)&mGraphicsOutput
             );

  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    if (Debug) {
      DEBUG ((DEBUG_INFO, "OCC: ASCII Text Reset [HandleProtocolFallback] - %r\n", Status));
      return Status;
    }

    return EFI_DEVICE_ERROR;
  }

  This->Mode->MaxMode   = 1;
  This->Mode->Attribute = ARRAY_SIZE (mGraphicsEfiColors) / 2 - 1;
  mBackgroundColor.Raw  = mGraphicsEfiColors[0];
  mForegroundColor.Raw  = mGraphicsEfiColors[ARRAY_SIZE (mGraphicsEfiColors) / 2 - 1];

  Status = RenderResync (This);
  gBS->RestoreTPL (OldTpl);
  if (EFI_ERROR (Status)) {
    if (Debug) {
      DEBUG ((DEBUG_INFO, "OCC: ASCII Text Reset [RenderResync] - %r\n", Status));
      return Status;
    }

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextReset (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS  Status;

  Status = AsciiTextResetEx (This, ExtendedVerification, FALSE);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  UINTN       Index;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Do not print text in graphics mode.
  //
  if (mConsoleMode != EfiConsoleControlScreenText) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Do not print in different modes.
  //
  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // For whatever reason UEFI Shell trashes these values when executing commands like help -b.
  //
  This->Mode->CursorColumn = (INT32)mPrivateColumn;
  This->Mode->CursorRow    = (INT32)mPrivateRow;

  FlushCursor (This->Mode->CursorVisible, This->Mode->CursorColumn, This->Mode->CursorRow);

  for (Index = 0; String[Index] != '\0'; ++Index) {
    //
    // Carriage return should just move the cursor back.
    //
    if (String[Index] == CHAR_CARRIAGE_RETURN) {
      This->Mode->CursorColumn = 0;
      continue;
    }

    if (String[Index] == CHAR_BACKSPACE) {
      if ((This->Mode->CursorColumn == 0) && (This->Mode->CursorRow > 0)) {
        This->Mode->CursorRow--;
        This->Mode->CursorColumn = (INT32)(mConsoleWidth - 1);
        RenderChar (' ', This->Mode->CursorColumn, This->Mode->CursorRow);
      } else if (This->Mode->CursorColumn > 0) {
        This->Mode->CursorColumn--;
        RenderChar (' ', This->Mode->CursorColumn, This->Mode->CursorRow);
      }

      continue;
    }

    //
    // Newline should move the cursor lower.
    // In case we are out of room it should scroll instead.
    //
    if (String[Index] == CHAR_LINEFEED) {
      if ((UINTN)This->Mode->CursorRow < mConsoleHeight - 1) {
        ++This->Mode->CursorRow;
        mConsoleMaxPosY = MAX (mConsoleMaxPosY, (UINTN)This->Mode->CursorRow);
      } else {
        RenderScroll ();
      }

      continue;
    }

    //
    // Render normal symbol and decide on next cursor position.
    //
    RenderChar (String[Index] == CHAR_TAB ? L' ' : String[Index], This->Mode->CursorColumn, This->Mode->CursorRow);

    if ((UINTN)This->Mode->CursorColumn < mConsoleWidth - 1) {
      //
      // Continues on the same line.
      //
      ++This->Mode->CursorColumn;
      mConsoleMaxPosX = MAX (mConsoleMaxPosX, (UINTN)This->Mode->CursorColumn);
    } else if ((UINTN)This->Mode->CursorRow < mConsoleHeight - 1) {
      //
      // New line without scroll.
      //
      This->Mode->CursorColumn = 0;
      ++This->Mode->CursorRow;
      mConsoleMaxPosY = MAX (mConsoleMaxPosY, (UINTN)This->Mode->CursorRow);
    } else {
      //
      // New line with scroll.
      //
      RenderScroll ();
      This->Mode->CursorColumn = 0;
    }
  }

  FlushCursor (This->Mode->CursorVisible, This->Mode->CursorColumn, This->Mode->CursorRow);

  mPrivateColumn = (UINTN)This->Mode->CursorColumn;
  mPrivateRow    = (UINTN)This->Mode->CursorRow;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextTestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  if (StrCmp (String, OC_CONSOLE_MARK_UNCONTROLLED) == 0) {
    mConsoleUncontrolled = TRUE;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_DEVICE_ERROR;
    }
  }

  if (ModeNumber == 0) {
    *Columns = mConsoleWidth;
    *Rows    = mConsoleHeight;
    Status   = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextSetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            ModeNumber
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  if (ModeNumber != 0) {
    return EFI_UNSUPPORTED;
  }

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    Status = RenderResync (This);
    gBS->RestoreTPL (OldTpl);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextSetAttribute (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            Attribute
  )
{
  EFI_STATUS  Status;
  UINT32      FgColor;
  UINT32      BgColor;
  EFI_TPL     OldTpl;

  if ((Attribute & ~0x7FU) != 0) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_DEVICE_ERROR;
    }
  }

  if (Attribute != (UINTN)This->Mode->Attribute) {
    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);

    FgColor = BitFieldRead32 ((UINT32)Attribute, 0, 3);
    BgColor = BitFieldRead32 ((UINT32)Attribute, 4, 6);

    //
    // Once we change the background colour, any clear screen must cover the whole screen.
    //
    if (mGraphicsEfiColors[BgColor] != mBackgroundColor.Raw) {
      mConsoleUncontrolled = TRUE;
    }

    mForegroundColor.Raw  = mGraphicsEfiColors[FgColor];
    mBackgroundColor.Raw  = mGraphicsEfiColors[BgColor];
    This->Mode->Attribute = (UINT32)Attribute;

    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

//
// Note: This intentionally performs a partial screen clear, affecting only the
// area containing text which has been written using our renderer, unless console
// is marked uncontrolled prior to clearing.
//
STATIC
EFI_STATUS
EFIAPI
AsciiTextClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;
  UINTN       Width;
  UINTN       Height;
  EFI_TPL     OldTpl;

  //
  // Note: We stay marked uncontrolled, if staying in graphics mode.
  //
  if (mConsoleMode != EfiConsoleControlScreenText) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // No need to re-clear screen straight after resync; but note that the initial screen
  // clear after resync, although of non-zero size, is intentionally very lightweight.
  //
  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_DEVICE_ERROR;
    }
  } else {
    //
    // When controlled, we assume that only text which we rendered needs to be cleared.
    // When marked uncontrolled anyone may have put content (in particular, graphics) anywhere
    // so always clear full screen.
    //
    if (mConsoleUncontrolled) {
      mGraphicsOutput->Blt (
                         mGraphicsOutput,
                         &mBackgroundColor.Pixel,
                         EfiBltVideoFill,
                         0,
                         0,
                         0,
                         0,
                         mGraphicsOutput->Mode->Info->HorizontalResolution,
                         mGraphicsOutput->Mode->Info->VerticalResolution,
                         0
                         );

      mConsoleUncontrolled = FALSE;
    } else {
      //
      // mConsoleMaxPosX,Y coordinates are the top left coordinates of the of the greatest
      // occupied character position. Because there is a cursor, there is always at least
      // one character position occupied.
      //
      Width  = (mConsoleMaxPosX + 1) * TGT_CHAR_WIDTH;
      Height = (mConsoleMaxPosY + 1) * TGT_CHAR_HEIGHT;
      ASSERT (TGT_PADD_WIDTH  + Width <= mGraphicsOutput->Mode->Info->HorizontalResolution);
      ASSERT (TGT_PADD_HEIGHT + Height <= mGraphicsOutput->Mode->Info->VerticalResolution);

      mGraphicsOutput->Blt (
                         mGraphicsOutput,
                         &mBackgroundColor.Pixel,
                         EfiBltVideoFill,
                         0,
                         0,
                         TGT_PADD_WIDTH,
                         TGT_PADD_HEIGHT,
                         Width,
                         Height,
                         0
                         );
    }
  }

  //
  // Handle cursor.
  //
  mPrivateColumn           = mPrivateRow = 0;
  This->Mode->CursorColumn = This->Mode->CursorRow = 0;
  FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);

  //
  // After clear screen, shell may scroll through old text via page up/down buttons,
  // but it is okay to reset max x/y here anyway, as any old text is brought on via
  // full redraw.
  //
  mConsoleMaxPosX = 0;
  mConsoleMaxPosY = 0;

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextSetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            Column,
  IN UINTN                            Row
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Clamping the row here successfully works round a bug in memtest86 where it
  // does not re-read console text resolution when it changes the graphics mode.
  // If changing between text resolutions >= 80x25, the issue is only visible
  // in the position of the footer line on the text UI screen, and this fixes it.
  //
  if (Column >= mConsoleWidth) {
    Column = mConsoleWidth - 1;
  }

  if (Row >= mConsoleHeight) {
    Row = mConsoleHeight - 1;
  }

  if ((Column < mConsoleWidth) && (Row < mConsoleHeight)) {
    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
    mPrivateColumn           = Column;
    mPrivateRow              = Row;
    This->Mode->CursorColumn = (INT32)mPrivateColumn;
    This->Mode->CursorRow    = (INT32)mPrivateRow;
    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
    mConsoleMaxPosX = MAX (mConsoleMaxPosX, Column);
    mConsoleMaxPosY = MAX (mConsoleMaxPosY, Row);
    Status          = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextEnableCursor (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          Visible
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_DEVICE_ERROR;
    }
  }

  FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
  This->Mode->CursorVisible = Visible;
  FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_SIMPLE_TEXT_OUTPUT_MODE
  mAsciiTextOutputMode;

STATIC
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  mAsciiTextOutputProtocol = {
  AsciiTextReset,
  AsciiTextOutputString,
  AsciiTextTestString,
  AsciiTextQueryMode,
  AsciiTextSetMode,
  AsciiTextSetAttribute,
  AsciiTextClearScreen,
  AsciiTextSetCursorPosition,
  AsciiTextEnableCursor,
  &mAsciiTextOutputMode
};

STATIC
EFI_STATUS
EFIAPI
ConsoleControlGetMode (
  IN   EFI_CONSOLE_CONTROL_PROTOCOL     *This,
  OUT  EFI_CONSOLE_CONTROL_SCREEN_MODE  *Mode,
  OUT  BOOLEAN                          *GopUgaExists OPTIONAL,
  OUT  BOOLEAN                          *StdInLocked  OPTIONAL
  )
{
  *Mode = mConsoleMode;

  if (GopUgaExists != NULL) {
    *GopUgaExists = TRUE;
  }

  if (StdInLocked != NULL) {
    *StdInLocked = FALSE;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlSetMode (
  IN EFI_CONSOLE_CONTROL_PROTOCOL     *This,
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  )
{
  if (mConsoleMode != Mode) {
    mConsoleMode = Mode;

    //
    // If controlled, switching to graphics then back to text should change nothing.
    //
    if (  (mConsoleMode == EfiConsoleControlScreenText)
       && mConsoleUncontrolled)
    {
      gST->ConOut->ClearScreen (gST->ConOut);
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlLockStdIn (
  IN EFI_CONSOLE_CONTROL_PROTOCOL  *This,
  IN CHAR16                        *Password
  )
{
  return EFI_DEVICE_ERROR;
}

STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
  mConsoleControlProtocol = {
  ConsoleControlGetMode,
  ConsoleControlSetMode,
  ConsoleControlLockStdIn
};

EFI_STATUS
OcUseBuiltinTextOutput (
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  InitialMode,
  IN OC_STORAGE_CONTEXT               *Storage  OPTIONAL,
  IN CONST CHAR8                      *Font     OPTIONAL,
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode,
  IN UINT32                           Width,
  IN UINT32                           Height
  )
{
  EFI_STATUS                    Status;
  UINTN                         UiScaleSize;
  EFI_CONSOLE_CONTROL_PROTOCOL  OriginalConsoleControlProtocol;

  Status = EFI_NOT_FOUND;
  if ((Storage != NULL) && (Font != NULL) && (*Font != '\0')) {
    Status = OcLoadConsoleFont (Storage, Font, &mConsoleFont);
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OCC: Loading hex font %a - %r\n",
      Font,
      Status
      ));
  }

  if (EFI_ERROR (Status)) {
    mIsDefaultFont = TRUE;
    mConsoleFont   = &gDefaultConsoleFont;
  }

  UiScaleSize = sizeof (mUIScale);

  Status = gRT->GetVariable (
                  APPLE_UI_SCALE_VARIABLE_NAME,
                  &gAppleVendorVariableGuid,
                  NULL,
                  &UiScaleSize,
                  (VOID *)&mUIScale
                  );

  if (EFI_ERROR (Status) || (mUIScale != 2)) {
    mUIScale = 1;
  }

  mFontScale = mUIScale;

  DEBUG ((DEBUG_INFO, "OCC: Using builtin text renderer with %d scale\n", mUIScale));

  mUserWidth   = Width;
  mUserHeight  = Height;
  mConsoleMode = InitialMode;
  OcConsoleControlSetMode (Mode);
  Status = OcConsoleControlInstallProtocol (&mConsoleControlProtocol, &OriginalConsoleControlProtocol, NULL); ///< Produces o/p using old, uncontrolled text protocol
  if (!EFI_ERROR (Status)) {
    Status = AsciiTextResetEx (&mAsciiTextOutputProtocol, TRUE, TRUE); ///< Prepare new text protocol (sets new font size, clears screen)
    if (EFI_ERROR (Status)) {
      OcConsoleControlRestoreProtocol (&OriginalConsoleControlProtocol);
    } else {
      gST->ConOut    = &mAsciiTextOutputProtocol; ///< Install new text protocol
      gST->Hdr.CRC32 = 0;

      gBS->CalculateCrc32 (
             gST,
             gST->Hdr.HeaderSize,
             &gST->Hdr.CRC32
             );
    }
  }

  DEBUG ((DEBUG_INFO, "OCC: Setup ASCII Output - %r\n", Status));

  return Status;
}
