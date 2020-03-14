/** @file
Implementation for EFI_HII_FONT_PROTOCOL.


Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "HiiDatabase.h"

EFI_GRAPHICS_OUTPUT_BLT_PIXEL        mHiiEfiColors[16] = {
  //
  // B     G     R
  //
  {0x00, 0x00, 0x00, 0x00},  // BLACK
  {0x98, 0x00, 0x00, 0x00},  // BLUE
  {0x00, 0x98, 0x00, 0x00},  // GREEN
  {0x98, 0x98, 0x00, 0x00},  // CYAN
  {0x00, 0x00, 0x98, 0x00},  // RED
  {0x98, 0x00, 0x98, 0x00},  // MAGENTA
  {0x00, 0x98, 0x98, 0x00},  // BROWN
  {0x98, 0x98, 0x98, 0x00},  // LIGHTGRAY
  {0x30, 0x30, 0x30, 0x00},  // DARKGRAY - BRIGHT BLACK
  {0xff, 0x00, 0x00, 0x00},  // LIGHTBLUE
  {0x00, 0xff, 0x00, 0x00},  // LIGHTGREEN
  {0xff, 0xff, 0x00, 0x00},  // LIGHTCYAN
  {0x00, 0x00, 0xff, 0x00},  // LIGHTRED
  {0xff, 0x00, 0xff, 0x00},  // LIGHTMAGENTA
  {0x00, 0xff, 0xff, 0x00},  // YELLOW
  {0xff, 0xff, 0xff, 0x00},  // WHITE
};


/**
  Insert a character cell information to the list specified by GlyphInfoList.

  This is a internal function.

  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphInfoList           HII_GLYPH_INFO list head.
  @param  Cell                    Incoming character cell information.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
NewCell (
  IN  CHAR16                         CharValue,
  IN  LIST_ENTRY                     *GlyphInfoList,
  IN  EFI_HII_GLYPH_INFO             *Cell
  )
{
  HII_GLYPH_INFO           *GlyphInfo;

  ASSERT (Cell != NULL && GlyphInfoList != NULL);

  GlyphInfo = (HII_GLYPH_INFO *) AllocateZeroPool (sizeof (HII_GLYPH_INFO));
  if (GlyphInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // GlyphInfoList stores a list of default character cell information, each is
  // identified by "CharId".
  //
  GlyphInfo->Signature = HII_GLYPH_INFO_SIGNATURE;
  GlyphInfo->CharId    = CharValue;
  if (Cell->AdvanceX == 0) {
    Cell->AdvanceX = Cell->Width;
  }
  CopyMem (&GlyphInfo->Cell, Cell, sizeof (EFI_HII_GLYPH_INFO));
  InsertTailList (GlyphInfoList, &GlyphInfo->Entry);

  return EFI_SUCCESS;
}


/**
  Get a character cell information from the list specified by GlyphInfoList.

  This is a internal function.

  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphInfoList           HII_GLYPH_INFO list head.
  @param  Cell                    Buffer which stores output character cell
                                  information.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_NOT_FOUND           The character info specified by CharValue does
                                  not exist.

**/
EFI_STATUS
GetCell (
  IN  CHAR16                         CharValue,
  IN  LIST_ENTRY                     *GlyphInfoList,
  OUT EFI_HII_GLYPH_INFO             *Cell
  )
{
  HII_GLYPH_INFO           *GlyphInfo;
  LIST_ENTRY               *Link;

  ASSERT (Cell != NULL && GlyphInfoList != NULL);

  //
  // Since the EFI_HII_GIBT_DEFAULTS block won't increment CharValueCurrent,
  // the value of "CharId" of a default character cell which is used for a
  // EFI_HII_GIBT_GLYPH_DEFAULT or EFI_HII_GIBT_GLYPHS_DEFAULT should be
  // less or equal to the value of "CharValueCurrent" of this default block.
  //
  // For instance, if the CharId of a GlyphInfoList is {1, 3, 7}, a default glyph
  // with CharValue equals "7" uses the GlyphInfo with CharId = 7;
  // a default glyph with CharValue equals "6" uses the GlyphInfo with CharId = 3.
  //
  for (Link = GlyphInfoList->BackLink; Link != GlyphInfoList; Link = Link->BackLink) {
    GlyphInfo = CR (Link, HII_GLYPH_INFO, Entry, HII_GLYPH_INFO_SIGNATURE);
    if (GlyphInfo->CharId <= CharValue) {
      CopyMem (Cell, &GlyphInfo->Cell, sizeof (EFI_HII_GLYPH_INFO));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Convert the glyph for a single character into a bitmap.

  This is a internal function.

  @param  Private                 HII database driver private data.
  @param  Char                    Character to retrieve.
  @param  StringInfo              Points to the string font and color information
                                  or NULL  if the string should use the default
                                  system font and color.
  @param  GlyphBuffer             Buffer to store the retrieved bitmap data.
  @param  Cell                    Points to EFI_HII_GLYPH_INFO structure.
  @param  Attributes              If not NULL, output the glyph attributes if any.

  @retval EFI_SUCCESS             Glyph bitmap outputted.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate the output buffer GlyphBuffer.
  @retval EFI_NOT_FOUND           The glyph was unknown can not be found.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.

**/
EFI_STATUS
GetGlyphBuffer (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  IN  CHAR16                         Char,
  IN  EFI_FONT_INFO                  *StringInfo,
  OUT UINT8                          **GlyphBuffer,
  OUT EFI_HII_GLYPH_INFO             *Cell,
  OUT UINT8                          *Attributes OPTIONAL
  )
{
  HII_DATABASE_RECORD                *Node;
  LIST_ENTRY                         *Link;
  HII_SIMPLE_FONT_PACKAGE_INSTANCE   *SimpleFont;
  LIST_ENTRY                         *Link1;
  UINT16                             Index;
  EFI_NARROW_GLYPH                   Narrow;
  EFI_WIDE_GLYPH                     Wide;
  HII_GLOBAL_FONT_INFO               *GlobalFont;
  UINTN                              HeaderSize;
  EFI_NARROW_GLYPH                   *NarrowPtr;
  EFI_WIDE_GLYPH                     *WidePtr;

  if (GlyphBuffer == NULL || Cell == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Private == NULL || Private->Signature != HII_DATABASE_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Cell, sizeof (EFI_HII_GLYPH_INFO));

  //
  // If StringInfo is not NULL, it must point to an existing EFI_FONT_INFO rather
  // than system default font and color.
  // If NULL, try to find the character in simplified font packages since
  // default system font is the fixed font (narrow or wide glyph).
  //
  if (StringInfo != NULL) {
    if(!IsFontInfoExisted (Private, StringInfo, NULL, NULL, &GlobalFont)) {
      return EFI_INVALID_PARAMETER;
    }
    if (Attributes != NULL) {
      *Attributes = PROPORTIONAL_GLYPH;
    }
    return FindGlyphBlock (GlobalFont->FontPackage, Char, GlyphBuffer, Cell, NULL);
  } else {
    HeaderSize = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR);

    for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
      Node = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
      for (Link1 = Node->PackageList->SimpleFontPkgHdr.ForwardLink;
           Link1 != &Node->PackageList->SimpleFontPkgHdr;
           Link1 = Link1->ForwardLink
          ) {
        SimpleFont = CR (Link1, HII_SIMPLE_FONT_PACKAGE_INSTANCE, SimpleFontEntry, HII_S_FONT_PACKAGE_SIGNATURE);
        //
        // Search the narrow glyph array
        //
        NarrowPtr = (EFI_NARROW_GLYPH *) ((UINT8 *) (SimpleFont->SimpleFontPkgHdr) + HeaderSize);
        for (Index = 0; Index < SimpleFont->SimpleFontPkgHdr->NumberOfNarrowGlyphs; Index++) {
          CopyMem (&Narrow, NarrowPtr + Index,sizeof (EFI_NARROW_GLYPH));
          if (Narrow.UnicodeWeight == Char) {
            *GlyphBuffer = (UINT8 *) AllocateZeroPool (EFI_GLYPH_HEIGHT);
            if (*GlyphBuffer == NULL) {
              return EFI_OUT_OF_RESOURCES;
            }
            Cell->Width    = EFI_GLYPH_WIDTH;
            Cell->Height   = EFI_GLYPH_HEIGHT;
            Cell->AdvanceX = Cell->Width;
            CopyMem (*GlyphBuffer, Narrow.GlyphCol1, Cell->Height);
            if (Attributes != NULL) {
              *Attributes = (UINT8) (Narrow.Attributes | NARROW_GLYPH);
            }
            return EFI_SUCCESS;
          }
        }
        //
        // Search the wide glyph array
        //
        WidePtr = (EFI_WIDE_GLYPH *) (NarrowPtr + SimpleFont->SimpleFontPkgHdr->NumberOfNarrowGlyphs);
        for (Index = 0; Index < SimpleFont->SimpleFontPkgHdr->NumberOfWideGlyphs; Index++) {
          CopyMem (&Wide, WidePtr + Index, sizeof (EFI_WIDE_GLYPH));
          if (Wide.UnicodeWeight == Char) {
            *GlyphBuffer    = (UINT8 *) AllocateZeroPool (EFI_GLYPH_HEIGHT * 2);
            if (*GlyphBuffer == NULL) {
              return EFI_OUT_OF_RESOURCES;
            }
            Cell->Width    = EFI_GLYPH_WIDTH * 2;
            Cell->Height   = EFI_GLYPH_HEIGHT;
            Cell->AdvanceX = Cell->Width;
            CopyMem (*GlyphBuffer, Wide.GlyphCol1, EFI_GLYPH_HEIGHT);
            CopyMem (*GlyphBuffer + EFI_GLYPH_HEIGHT, Wide.GlyphCol2, EFI_GLYPH_HEIGHT);
            if (Attributes != NULL) {
              *Attributes = (UINT8) (Wide.Attributes | EFI_GLYPH_WIDE);
            }
            return EFI_SUCCESS;
          }
        }
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Convert bitmap data of the glyph to blt structure.

  This is a internal function.

  @param GlyphBuffer     Buffer points to bitmap data of glyph.
  @param  Foreground     The color of the "on" pixels in the glyph in the
                         bitmap.
  @param  Background     The color of the "off" pixels in the glyph in the
                         bitmap.
  @param  ImageWidth     Width of the whole image in pixels.
  @param  RowWidth       The width of the text on the line, in pixels.
  @param  RowHeight      The height of the line, in pixels.
  @param  Transparent    If TRUE, the Background color is ignored and all
                         "off" pixels in the character's drawn will use the
                         pixel value from BltBuffer.
  @param  Origin         On input, points to the origin of the to be
                         displayed character, on output, points to the
                         next glyph's origin.

**/
VOID
NarrowGlyphToBlt (
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINT16                        ImageWidth,
  IN     UINTN                         RowWidth,
  IN     UINTN                         RowHeight,
  IN     BOOLEAN                       Transparent,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Origin
  )
{
  UINT8                                Xpos;
  UINT8                                Ypos;
  UINT8                                Height;
  UINT8                                Width;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *Buffer;

  ASSERT (GlyphBuffer != NULL && Origin != NULL && *Origin != NULL);

  Height = EFI_GLYPH_HEIGHT;
  Width  = EFI_GLYPH_WIDTH;

  //
  // Move position to the left-top corner of char.
  //
  Buffer = *Origin - EFI_GLYPH_HEIGHT * ImageWidth;

  //
  // Char may be partially displayed when CLIP_X or CLIP_Y is not set.
  //
  if (RowHeight < Height) {
    Height = (UINT8) RowHeight;
  }
  if (RowWidth < Width) {
    Width = (UINT8) RowWidth;
  }

  for (Ypos = 0; Ypos < Height; Ypos++) {
    for (Xpos = 0; Xpos < Width; Xpos++) {
      if ((GlyphBuffer[Ypos] & (1 << (EFI_GLYPH_WIDTH - Xpos - 1))) != 0) {
        Buffer[Ypos * ImageWidth + Xpos] = Foreground;
      } else {
        if (!Transparent) {
          Buffer[Ypos * ImageWidth + Xpos] = Background;
        }
      }
    }
  }

  *Origin = *Origin + EFI_GLYPH_WIDTH;
}


/**
  Convert bitmap data of the glyph to blt structure.

  This is a internal function.

  @param  GlyphBuffer             Buffer points to bitmap data of glyph.
  @param  Foreground              The color of the "on" pixels in the glyph in the
                                  bitmap.
  @param  Background              The color of the "off" pixels in the glyph in the
                                  bitmap.
  @param  ImageWidth              Width of the whole image in pixels.
  @param  BaseLine                BaseLine in the line.
  @param  RowWidth                The width of the text on the line, in pixels.
  @param  RowHeight               The height of the line, in pixels.
  @param  Transparent             If TRUE, the Background color is ignored and all
                                  "off" pixels in the character's drawn will use the
                                  pixel value from BltBuffer.
  @param  Cell                    Points to EFI_HII_GLYPH_INFO structure.
  @param  Attributes              The attribute of incoming glyph in GlyphBuffer.
  @param  Origin                  On input, points to the origin of the to be
                                  displayed character, on output, points to the
                                  next glyph's origin.


**/
VOID
GlyphToBlt (
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINT16                        ImageWidth,
  IN     UINT16                        BaseLine,
  IN     UINTN                         RowWidth,
  IN     UINTN                         RowHeight,
  IN     BOOLEAN                       Transparent,
  IN     CONST EFI_HII_GLYPH_INFO      *Cell,
  IN     UINT8                         Attributes,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Origin
  )
{
  UINT16                                Xpos;
  UINT16                                Ypos;
  UINT8                                 Data;
  UINT16                                Index;
  UINT16                                YposOffset;
  UINTN                                 OffsetY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer;

  ASSERT (Origin != NULL && *Origin != NULL && Cell != NULL);

  //
  // Only adjust origin position if char has no bitmap.
  //
  if (GlyphBuffer == NULL) {
    *Origin = *Origin + Cell->AdvanceX;
    return;
  }
  //
  // Move position to the left-top corner of char.
  //
  BltBuffer  = *Origin + Cell->OffsetX - (Cell->OffsetY + Cell->Height) * ImageWidth;
  YposOffset = (UINT16) (BaseLine - (Cell->OffsetY + Cell->Height));

  //
  // Since non-spacing key will be printed OR'd with the previous glyph, don't
  // write 0.
  //
  if ((Attributes & EFI_GLYPH_NON_SPACING) == EFI_GLYPH_NON_SPACING) {
    Transparent = TRUE;
  }

  //
  // The glyph's upper left hand corner pixel is the most significant bit of the
  // first bitmap byte.
  //
  for (Ypos = 0; Ypos < Cell->Height && (((UINT32) Ypos + YposOffset) < RowHeight); Ypos++) {
    OffsetY = BITMAP_LEN_1_BIT (Cell->Width, Ypos);

    //
    // All bits in these bytes are meaningful.
    //
    for (Xpos = 0; Xpos < Cell->Width / 8; Xpos++) {
      Data  = *(GlyphBuffer + OffsetY + Xpos);
      for (Index = 0; Index < 8 && (((UINT32) Xpos * 8 + Index + Cell->OffsetX) < RowWidth); Index++) {
        if ((Data & (1 << (8 - Index - 1))) != 0) {
          BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Foreground;
        } else {
          if (!Transparent) {
            BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Background;
          }
        }
      }
    }

    if (Cell->Width % 8 != 0) {
      //
      // There are some padding bits in this byte. Ignore them.
      //
      Data  = *(GlyphBuffer + OffsetY + Xpos);
      for (Index = 0; Index < Cell->Width % 8 && (((UINT32) Xpos * 8 + Index + Cell->OffsetX) < RowWidth); Index++) {
        if ((Data & (1 << (8 - Index - 1))) != 0) {
          BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Foreground;
        } else {
          if (!Transparent) {
            BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Background;
          }
        }
      }
    } // end of if (Width % 8...)

  } // end of for (Ypos=0...)

  *Origin = *Origin + Cell->AdvanceX;
}


/**
  Convert bitmap data of the glyph to blt structure.

  This is a internal function.

  @param  GlyphBuffer             Buffer points to bitmap data of glyph.
  @param  Foreground              The color of the "on" pixels in the glyph in the
                                  bitmap.
  @param  Background              The color of the "off" pixels in the glyph in the
                                  bitmap.
  @param  ImageWidth              Width of the whole image in pixels.
  @param  BaseLine                BaseLine in the line.
  @param  RowWidth                The width of the text on the line, in pixels.
  @param  RowHeight               The height of the line, in pixels.
  @param  Transparent             If TRUE, the Background color is ignored and all
                                  "off" pixels in the character's drawn will use the
                                  pixel value from BltBuffer.
  @param  Cell                    Points to EFI_HII_GLYPH_INFO structure.
  @param  Attributes              The attribute of incoming glyph in GlyphBuffer.
  @param  Origin                  On input, points to the origin of the to be
                                  displayed character, on output, points to the
                                  next glyph's origin.

  @return Points to the address of next origin node in BltBuffer.

**/
VOID
GlyphToImage (
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINT16                        ImageWidth,
  IN     UINT16                        BaseLine,
  IN     UINTN                         RowWidth,
  IN     UINTN                         RowHeight,
  IN     BOOLEAN                       Transparent,
  IN     CONST EFI_HII_GLYPH_INFO      *Cell,
  IN     UINT8                         Attributes,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Origin
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *Buffer;

  ASSERT (Origin != NULL && *Origin != NULL && Cell != NULL);

  Buffer = *Origin;

  if ((Attributes & EFI_GLYPH_NON_SPACING) == EFI_GLYPH_NON_SPACING) {
    //
    // This character is a non-spacing key, print it OR'd with the previous glyph.
    // without advancing cursor.
    //
    Buffer -= Cell->AdvanceX;
    GlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      BaseLine,
      RowWidth,
      RowHeight,
      Transparent,
      Cell,
      Attributes,
      &Buffer
      );

  } else if ((Attributes & EFI_GLYPH_WIDE) == EFI_GLYPH_WIDE) {
    //
    // This character is wide glyph, i.e. 16 pixels * 19 pixels.
    // Draw it as two narrow glyphs.
    //
    NarrowGlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      RowWidth,
      RowHeight,
      Transparent,
      Origin
      );

    NarrowGlyphToBlt (
      GlyphBuffer + EFI_GLYPH_HEIGHT,
      Foreground,
      Background,
      ImageWidth,
      RowWidth,
      RowHeight,
      Transparent,
      Origin
      );

  } else if ((Attributes & NARROW_GLYPH) == NARROW_GLYPH) {
    //
    // This character is narrow glyph, i.e. 8 pixels * 19 pixels.
    //
    NarrowGlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      RowWidth,
      RowHeight,
      Transparent,
      Origin
      );

  } else if ((Attributes & PROPORTIONAL_GLYPH) == PROPORTIONAL_GLYPH) {
    //
    // This character is proportional glyph, i.e. Cell->Width * Cell->Height pixels.
    //
    GlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      BaseLine,
      RowWidth,
      RowHeight,
      Transparent,
      Cell,
      Attributes,
      Origin
      );
  }
}


/**
  Write the output parameters of FindGlyphBlock().

  This is a internal function.

  @param  BufferIn                Buffer which stores the bitmap data of the found
                                  block.
  @param  BufferLen               Length of BufferIn.
  @param  InputCell               Buffer which stores cell information of the
                                  encoded bitmap.
  @param  GlyphBuffer             Output the corresponding bitmap data of the found
                                  block. It is the caller's responsibility to free
                                  this buffer.
  @param  Cell                    Output cell information of the encoded bitmap.
  @param  GlyphBufferLen          If not NULL, output the length of GlyphBuffer.

  @retval EFI_SUCCESS             The operation is performed successfully.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
WriteOutputParam (
  IN  UINT8                          *BufferIn,
  IN  UINTN                          BufferLen,
  IN  EFI_HII_GLYPH_INFO             *InputCell,
  OUT UINT8                          **GlyphBuffer, OPTIONAL
  OUT EFI_HII_GLYPH_INFO             *Cell, OPTIONAL
  OUT UINTN                          *GlyphBufferLen OPTIONAL
  )
{
  if (BufferIn == NULL || InputCell == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Cell != NULL) {
    CopyMem (Cell, InputCell, sizeof (EFI_HII_GLYPH_INFO));
  }

  if (GlyphBuffer != NULL && BufferLen > 0) {
    *GlyphBuffer = (UINT8 *) AllocateZeroPool (BufferLen);
    if (*GlyphBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (*GlyphBuffer, BufferIn, BufferLen);
  }

  if (GlyphBufferLen != NULL) {
    *GlyphBufferLen = BufferLen;
  }

  return EFI_SUCCESS;
}


/**
  Parse all glyph blocks to find a glyph block specified by CharValue.
  If CharValue = (CHAR16) (-1), collect all default character cell information
  within this font package and backup its information.

  @param  FontPackage             Hii string package instance.
  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphBuffer             Output the corresponding bitmap data of the found
                                  block. It is the caller's responsibility to free
                                  this buffer.
  @param  Cell                    Output cell information of the encoded bitmap.
  @param  GlyphBufferLen          If not NULL, output the length of GlyphBuffer.

  @retval EFI_SUCCESS             The bitmap data is retrieved successfully.
  @retval EFI_NOT_FOUND           The specified CharValue does not exist in current
                                  database.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
FindGlyphBlock (
  IN  HII_FONT_PACKAGE_INSTANCE      *FontPackage,
  IN  CHAR16                         CharValue,
  OUT UINT8                          **GlyphBuffer, OPTIONAL
  OUT EFI_HII_GLYPH_INFO             *Cell, OPTIONAL
  OUT UINTN                          *GlyphBufferLen OPTIONAL
  )
{
  EFI_STATUS                          Status;
  UINT8                               *BlockPtr;
  UINT16                              CharCurrent;
  UINT16                              Length16;
  UINT32                              Length32;
  EFI_HII_GIBT_GLYPHS_BLOCK           Glyphs;
  UINTN                               BufferLen;
  UINT16                              Index;
  EFI_HII_GLYPH_INFO                  DefaultCell;
  EFI_HII_GLYPH_INFO                  LocalCell;
  INT16                               MinOffsetY;
  UINT16                              BaseLine;

  ASSERT (FontPackage != NULL);
  ASSERT (FontPackage->Signature == HII_FONT_PACKAGE_SIGNATURE);
  BaseLine  = 0;
  MinOffsetY = 0;

  if (CharValue == (CHAR16) (-1)) {
    //
    // Collect the cell information specified in font package fixed header.
    // Use CharValue =0 to represent this particular cell.
    //
    Status = NewCell (
               0,
               &FontPackage->GlyphInfoList,
               (EFI_HII_GLYPH_INFO *) ((UINT8 *) FontPackage->FontPkgHdr + 3 * sizeof (UINT32))
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem (
      &LocalCell,
      (UINT8 *) FontPackage->FontPkgHdr + 3 * sizeof (UINT32),
      sizeof (EFI_HII_GLYPH_INFO)
      );
  }

  BlockPtr    = FontPackage->GlyphBlock;
  CharCurrent = 1;
  BufferLen   = 0;

  while (*BlockPtr != EFI_HII_GIBT_END) {
    switch (*BlockPtr) {
    case EFI_HII_GIBT_DEFAULTS:
      //
      // Collect all default character cell information specified by
      // EFI_HII_GIBT_DEFAULTS.
      //
      if (CharValue == (CHAR16) (-1)) {
        Status = NewCell (
                   CharCurrent,
                   &FontPackage->GlyphInfoList,
                   (EFI_HII_GLYPH_INFO *) (BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK))
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
        CopyMem (
          &LocalCell,
          BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK),
          sizeof (EFI_HII_GLYPH_INFO)
          );
        if (BaseLine < LocalCell.Height + LocalCell.OffsetY) {
          BaseLine = (UINT16) (LocalCell.Height + LocalCell.OffsetY);
        }
        if (MinOffsetY > LocalCell.OffsetY) {
          MinOffsetY = LocalCell.OffsetY;
        }
      }
      BlockPtr += sizeof (EFI_HII_GIBT_DEFAULTS_BLOCK);
      break;

    case EFI_HII_GIBT_DUPLICATE:
      if (CharCurrent == CharValue) {
        CopyMem (&CharValue, BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK), sizeof (CHAR16));
        CharCurrent = 1;
        BlockPtr    = FontPackage->GlyphBlock;
        continue;
      }
      CharCurrent++;
      BlockPtr += sizeof (EFI_HII_GIBT_DUPLICATE_BLOCK);
      break;

    case EFI_HII_GIBT_EXT1:
      BlockPtr += *(UINT8*)((UINTN)BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK) + sizeof (UINT8));
      break;
    case EFI_HII_GIBT_EXT2:
      CopyMem (
        &Length16,
        (UINT8*)((UINTN)BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK) + sizeof (UINT8)),
        sizeof (UINT16)
        );
      BlockPtr += Length16;
      break;
    case EFI_HII_GIBT_EXT4:
      CopyMem (
        &Length32,
        (UINT8*)((UINTN)BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK) + sizeof (UINT8)),
        sizeof (UINT32)
        );
      BlockPtr += Length32;
      break;

    case EFI_HII_GIBT_GLYPH:
      CopyMem (
        &LocalCell,
        BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK),
        sizeof (EFI_HII_GLYPH_INFO)
        );
      if (CharValue == (CHAR16) (-1)) {
        if (BaseLine < LocalCell.Height + LocalCell.OffsetY) {
          BaseLine = (UINT16) (LocalCell.Height + LocalCell.OffsetY);
        }
        if (MinOffsetY > LocalCell.OffsetY) {
          MinOffsetY = LocalCell.OffsetY;
        }
      }
      BufferLen = BITMAP_LEN_1_BIT (LocalCell.Width, LocalCell.Height);
      if (CharCurrent == CharValue) {
        return WriteOutputParam (
                 (UINT8*)((UINTN)BlockPtr + sizeof (EFI_HII_GIBT_GLYPH_BLOCK) - sizeof (UINT8)),
                 BufferLen,
                 &LocalCell,
                 GlyphBuffer,
                 Cell,
                 GlyphBufferLen
                 );
      }
      CharCurrent++;
      BlockPtr += sizeof (EFI_HII_GIBT_GLYPH_BLOCK) - sizeof (UINT8) + BufferLen;
      break;

    case EFI_HII_GIBT_GLYPHS:
      BlockPtr += sizeof (EFI_HII_GLYPH_BLOCK);
      CopyMem (&Glyphs.Cell, BlockPtr, sizeof (EFI_HII_GLYPH_INFO));
      BlockPtr += sizeof (EFI_HII_GLYPH_INFO);
      CopyMem (&Glyphs.Count, BlockPtr, sizeof (UINT16));
      BlockPtr += sizeof (UINT16);

      if (CharValue == (CHAR16) (-1)) {
        if (BaseLine < Glyphs.Cell.Height + Glyphs.Cell.OffsetY) {
          BaseLine = (UINT16) (Glyphs.Cell.Height + Glyphs.Cell.OffsetY);
        }
        if (MinOffsetY > Glyphs.Cell.OffsetY) {
          MinOffsetY = Glyphs.Cell.OffsetY;
        }
      }

      BufferLen = BITMAP_LEN_1_BIT (Glyphs.Cell.Width, Glyphs.Cell.Height);
      for (Index = 0; Index < Glyphs.Count; Index++) {
        if (CharCurrent + Index == CharValue) {
          return WriteOutputParam (
                   BlockPtr,
                   BufferLen,
                   &Glyphs.Cell,
                   GlyphBuffer,
                   Cell,
                   GlyphBufferLen
                   );
        }
        BlockPtr += BufferLen;
      }
      CharCurrent = (UINT16) (CharCurrent + Glyphs.Count);
      break;

    case EFI_HII_GIBT_GLYPH_DEFAULT:
      Status = GetCell (CharCurrent, &FontPackage->GlyphInfoList, &DefaultCell);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      if (CharValue == (CHAR16) (-1)) {
        if (BaseLine < DefaultCell.Height + DefaultCell.OffsetY) {
          BaseLine = (UINT16) (DefaultCell.Height + DefaultCell.OffsetY);
        }
        if (MinOffsetY > DefaultCell.OffsetY) {
          MinOffsetY = DefaultCell.OffsetY;
        }
      }
      BufferLen = BITMAP_LEN_1_BIT (DefaultCell.Width, DefaultCell.Height);

      if (CharCurrent == CharValue) {
        return WriteOutputParam (
                 BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK),
                 BufferLen,
                 &DefaultCell,
                 GlyphBuffer,
                 Cell,
                 GlyphBufferLen
                 );
      }
      CharCurrent++;
      BlockPtr += sizeof (EFI_HII_GLYPH_BLOCK) + BufferLen;
      break;

    case EFI_HII_GIBT_GLYPHS_DEFAULT:
      CopyMem (&Length16, BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK), sizeof (UINT16));
      Status = GetCell (CharCurrent, &FontPackage->GlyphInfoList, &DefaultCell);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      if (CharValue == (CHAR16) (-1)) {
        if (BaseLine < DefaultCell.Height + DefaultCell.OffsetY) {
          BaseLine = (UINT16) (DefaultCell.Height + DefaultCell.OffsetY);
        }
        if (MinOffsetY > DefaultCell.OffsetY) {
          MinOffsetY = DefaultCell.OffsetY;
        }
      }
      BufferLen = BITMAP_LEN_1_BIT (DefaultCell.Width, DefaultCell.Height);
      BlockPtr += sizeof (EFI_HII_GIBT_GLYPHS_DEFAULT_BLOCK) - sizeof (UINT8);
      for (Index = 0; Index < Length16; Index++) {
        if (CharCurrent + Index == CharValue) {
          return WriteOutputParam (
                   BlockPtr,
                   BufferLen,
                   &DefaultCell,
                   GlyphBuffer,
                   Cell,
                   GlyphBufferLen
                   );
        }
        BlockPtr += BufferLen;
      }
      CharCurrent = (UINT16) (CharCurrent + Length16);
      break;

    case EFI_HII_GIBT_SKIP1:
      CharCurrent = (UINT16) (CharCurrent + (UINT16) (*(BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK))));
      BlockPtr    += sizeof (EFI_HII_GIBT_SKIP1_BLOCK);
      break;
    case EFI_HII_GIBT_SKIP2:
      CopyMem (&Length16, BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK), sizeof (UINT16));
      CharCurrent = (UINT16) (CharCurrent + Length16);
      BlockPtr    += sizeof (EFI_HII_GIBT_SKIP2_BLOCK);
      break;
    default:
      ASSERT (FALSE);
      break;
    }

    if (CharValue < CharCurrent) {
      return EFI_NOT_FOUND;
    }
  }

  if (CharValue == (CHAR16) (-1)) {
    FontPackage->BaseLine = BaseLine;
    FontPackage->Height   = (UINT16) (BaseLine - MinOffsetY);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


/**
  Copy a Font Name to a new created EFI_FONT_INFO structure.

  This is a internal function.

  @param  FontName                NULL-terminated string.
  @param  FontInfo                a new EFI_FONT_INFO which stores the FontName.
                                  It's caller's responsibility to free this buffer.

  @retval EFI_SUCCESS             FontInfo is allocated and copied with FontName.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
SaveFontName (
  IN  EFI_STRING                       FontName,
  OUT EFI_FONT_INFO                    **FontInfo
  )
{
  UINTN         FontInfoLen;
  UINTN         NameSize;

  ASSERT (FontName != NULL && FontInfo != NULL);

  NameSize = StrSize (FontName);
  FontInfoLen = sizeof (EFI_FONT_INFO) - sizeof (CHAR16) + NameSize;
  *FontInfo = (EFI_FONT_INFO *) AllocateZeroPool (FontInfoLen);
  if (*FontInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrCpyS ((*FontInfo)->FontName, NameSize / sizeof (CHAR16), FontName);
  return EFI_SUCCESS;
}


/**
  Retrieve system default font and color.

  @param  Private                 HII database driver private data.
  @param  FontInfo                Points to system default font output-related
                                  information. It's caller's responsibility to free
                                  this buffer.
  @param  FontInfoSize            If not NULL, output the size of buffer FontInfo.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.

**/
EFI_STATUS
GetSystemFont (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  OUT EFI_FONT_DISPLAY_INFO          **FontInfo,
  OUT UINTN                          *FontInfoSize OPTIONAL
  )
{
  EFI_FONT_DISPLAY_INFO              *Info;
  UINTN                              InfoSize;
  UINTN                              NameSize;

  if (Private == NULL || Private->Signature != HII_DATABASE_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
  if (FontInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The standard font always has the name "sysdefault".
  //
  NameSize = StrSize (L"sysdefault");
  InfoSize = sizeof (EFI_FONT_DISPLAY_INFO) - sizeof (CHAR16) + NameSize;
  Info = (EFI_FONT_DISPLAY_INFO *) AllocateZeroPool (InfoSize);
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Info->ForegroundColor    = mHiiEfiColors[Private->Attribute & 0x0f];
  ASSERT ((Private->Attribute >> 4) < 8);
  Info->BackgroundColor    = mHiiEfiColors[Private->Attribute >> 4];
  Info->FontInfoMask       = EFI_FONT_INFO_SYS_FONT | EFI_FONT_INFO_SYS_SIZE | EFI_FONT_INFO_SYS_STYLE;
  Info->FontInfo.FontStyle = 0;
  Info->FontInfo.FontSize  = EFI_GLYPH_HEIGHT;
  StrCpyS (Info->FontInfo.FontName, NameSize / sizeof (CHAR16), L"sysdefault");

  *FontInfo = Info;
  if (FontInfoSize != NULL) {
    *FontInfoSize = InfoSize;
  }
  return EFI_SUCCESS;
}


/**
  Check whether EFI_FONT_DISPLAY_INFO points to system default font and color or
  returns the system default according to the optional inputs.

  This is a internal function.

  @param  Private                 HII database driver private data.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.
  @param  SystemInfo              If not NULL, points to system default font and color.

  @param  SystemInfoLen           If not NULL, output the length of default system
                                  info.

  @retval TRUE                    Yes, it points to system default.
  @retval FALSE                   No.

**/
BOOLEAN
IsSystemFontInfo (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  IN  EFI_FONT_DISPLAY_INFO          *StringInfo,
  OUT EFI_FONT_DISPLAY_INFO          **SystemInfo, OPTIONAL
  OUT UINTN                          *SystemInfoLen OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_FONT_DISPLAY_INFO               *SystemDefault;
  UINTN                               DefaultLen;
  BOOLEAN                             Flag;

  ASSERT (Private != NULL && Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);

  if (StringInfo == NULL && SystemInfo == NULL) {
    return TRUE;
  }

  SystemDefault = NULL;
  DefaultLen    = 0;

  Status = GetSystemFont (Private, &SystemDefault, &DefaultLen);
  ASSERT_EFI_ERROR (Status);
  ASSERT ((SystemDefault != NULL) && (DefaultLen != 0));

  //
  // Record the system default info.
  //
  if (SystemInfo != NULL) {
    *SystemInfo = SystemDefault;
  }

  if (SystemInfoLen != NULL) {
    *SystemInfoLen = DefaultLen;
  }

  if (StringInfo == NULL) {
    return TRUE;
  }

  Flag = FALSE;
  //
  // Check the FontInfoMask to see whether it is retrieving system info.
  //
  if ((StringInfo->FontInfoMask & (EFI_FONT_INFO_SYS_FONT | EFI_FONT_INFO_ANY_FONT)) == 0) {
    if (StrCmp (StringInfo->FontInfo.FontName, SystemDefault->FontInfo.FontName) != 0) {
      goto Exit;
    }
  }
  if ((StringInfo->FontInfoMask & (EFI_FONT_INFO_SYS_SIZE | EFI_FONT_INFO_ANY_SIZE)) == 0) {
    if (StringInfo->FontInfo.FontSize != SystemDefault->FontInfo.FontSize) {
      goto Exit;
    }
  }
  if ((StringInfo->FontInfoMask & (EFI_FONT_INFO_SYS_STYLE | EFI_FONT_INFO_ANY_STYLE)) == 0) {
    if (StringInfo->FontInfo.FontStyle != SystemDefault->FontInfo.FontStyle) {
      goto Exit;
    }
  }
  if ((StringInfo->FontInfoMask & EFI_FONT_INFO_SYS_FORE_COLOR) == 0) {
    if (CompareMem (
          &StringInfo->ForegroundColor,
          &SystemDefault->ForegroundColor,
          sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
          ) != 0) {
      goto Exit;
    }
  }
  if ((StringInfo->FontInfoMask & EFI_FONT_INFO_SYS_BACK_COLOR) == 0) {
    if (CompareMem (
          &StringInfo->BackgroundColor,
          &SystemDefault->BackgroundColor,
          sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
          ) != 0) {
      goto Exit;
    }
  }

  Flag = TRUE;

Exit:
  if (SystemInfo == NULL) {
    if (SystemDefault != NULL) {
      FreePool (SystemDefault);
    }
  }
  return Flag;
}


/**
  This function checks whether EFI_FONT_INFO exists in current database. If
  FontInfoMask is specified, check what options can be used to make a match.
  Note that the masks relate to where the system default should be supplied
  are ignored by this function.

  @param  Private                 Hii database private structure.
  @param  FontInfo                Points to EFI_FONT_INFO structure.
  @param  FontInfoMask            If not NULL, describes what options can be used
                                  to make a match between the font requested and
                                  the font available. The caller must guarantee
                                  this mask is valid.
  @param  FontHandle              On entry, Points to the font handle returned by a
                                  previous  call to GetFontInfo() or NULL to start
                                  with the first font.
  @param  GlobalFontInfo          If not NULL, output the corresponding global font
                                  info.

  @retval TRUE                    Existed
  @retval FALSE                   Not existed

**/
BOOLEAN
IsFontInfoExisted (
  IN  HII_DATABASE_PRIVATE_DATA *Private,
  IN  EFI_FONT_INFO             *FontInfo,
  IN  EFI_FONT_INFO_MASK        *FontInfoMask,   OPTIONAL
  IN  EFI_FONT_HANDLE           FontHandle,      OPTIONAL
  OUT HII_GLOBAL_FONT_INFO      **GlobalFontInfo OPTIONAL
  )
{
  HII_GLOBAL_FONT_INFO          *GlobalFont;
  HII_GLOBAL_FONT_INFO          *GlobalFontBackup1;
  HII_GLOBAL_FONT_INFO          *GlobalFontBackup2;
  LIST_ENTRY                    *Link;
  EFI_FONT_INFO_MASK            Mask;
  BOOLEAN                       Matched;
  BOOLEAN                       VagueMatched1;
  BOOLEAN                       VagueMatched2;

  ASSERT (Private != NULL && Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);
  ASSERT (FontInfo != NULL);

  //
  // Matched flag represents an exactly match; VagueMatched1 represents a RESIZE
  // or RESTYLE match; VagueMatched2 represents a RESIZE | RESTYLE match.
  //
  Matched           = FALSE;
  VagueMatched1     = FALSE;
  VagueMatched2     = FALSE;

  Mask              = 0;
  GlobalFontBackup1 = NULL;
  GlobalFontBackup2 = NULL;

  // The process of where the system default should be supplied instead of
  // the specified font info beyonds this function's scope.
  //
  if (FontInfoMask != NULL) {
    Mask = *FontInfoMask & (~SYS_FONT_INFO_MASK);
  }

  //
  // If not NULL, FontHandle points to the next node of the last searched font
  // node by previous call.
  //
  if (FontHandle == NULL) {
    Link = Private->FontInfoList.ForwardLink;
  } else {
    Link = (LIST_ENTRY     *) FontHandle;
  }

  for (; Link != &Private->FontInfoList; Link = Link->ForwardLink) {
    GlobalFont = CR (Link, HII_GLOBAL_FONT_INFO, Entry, HII_GLOBAL_FONT_INFO_SIGNATURE);
    if (FontInfoMask == NULL) {
      if (CompareMem (GlobalFont->FontInfo, FontInfo, GlobalFont->FontInfoSize) == 0) {
        if (GlobalFontInfo != NULL) {
          *GlobalFontInfo = GlobalFont;
        }
        return TRUE;
      }
    } else {
      //
      // Check which options could be used to make a match.
      //
      switch (Mask) {
      case EFI_FONT_INFO_ANY_FONT:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle &&
            GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_STYLE:
        if (GlobalFont->FontInfo->FontSize == FontInfo->FontSize) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_SIZE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_SIZE | EFI_FONT_INFO_ANY_STYLE:
        Matched   = TRUE;
        break;
      //
      // If EFI_FONT_INFO_RESTYLE is specified, then the system may attempt to
      // remove some of the specified styles to meet the style requested.
      //
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_RESTYLE:
        if (GlobalFont->FontInfo->FontSize == FontInfo->FontSize) {
          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            Matched           = TRUE;
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      //
      // If EFI_FONT_INFO_RESIZE is specified, then the system may attempt to
      // stretch or shrink a font to meet the size requested.
      //
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_RESIZE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_RESTYLE | EFI_FONT_INFO_RESIZE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          } else {
            VagueMatched2     = TRUE;
            GlobalFontBackup2 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_STYLE | EFI_FONT_INFO_RESIZE:
        if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
          Matched           = TRUE;
        } else {
          VagueMatched1     = TRUE;
          GlobalFontBackup1 = GlobalFont;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_SIZE | EFI_FONT_INFO_RESTYLE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          Matched           = TRUE;
        } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
          VagueMatched1     = TRUE;
          GlobalFontBackup1 = GlobalFont;
        }
        break;
      case EFI_FONT_INFO_ANY_STYLE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_STYLE | EFI_FONT_INFO_ANY_SIZE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_STYLE | EFI_FONT_INFO_RESIZE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_ANY_SIZE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontStyle  == FontInfo->FontStyle) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_SIZE | EFI_FONT_INFO_RESTYLE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            Matched           = TRUE;
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_RESTYLE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {

          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            Matched           = TRUE;
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_RESIZE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontStyle  == FontInfo->FontStyle) {

          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_RESIZE | EFI_FONT_INFO_RESTYLE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
              Matched           = TRUE;
            } else {
              VagueMatched1     = TRUE;
              GlobalFontBackup1 = GlobalFont;
            }
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
              VagueMatched1     = TRUE;
              GlobalFontBackup1 = GlobalFont;
            } else {
              VagueMatched2     = TRUE;
              GlobalFontBackup2 = GlobalFont;
            }
          }
        }
        break;
      default:
        break;
      }

      if (Matched) {
        if (GlobalFontInfo != NULL) {
          *GlobalFontInfo = GlobalFont;
        }
        return TRUE;
      }
    }
  }

  if (VagueMatched1) {
    if (GlobalFontInfo != NULL) {
      *GlobalFontInfo = GlobalFontBackup1;
    }
    return TRUE;
  } else if (VagueMatched2) {
    if (GlobalFontInfo != NULL) {
      *GlobalFontInfo = GlobalFontBackup2;
    }
    return TRUE;
  }

  return FALSE;
}


/**
  Check whether the unicode represents a line break or not.

  This is a internal function. Please see Section 27.2.6 of the UEFI Specification
  for a description of the supported string format.

  @param  Char                    Unicode character

  @retval 0                       Yes, it forces a line break.
  @retval 1                       Yes, it presents a line break opportunity
  @retval 2                       Yes, it requires a line break happen before and after it.
  @retval -1                      No, it is not a link break.

**/
INT8
IsLineBreak (
  IN  CHAR16    Char
  )
{
  switch (Char) {
    //
    // Mandatory line break characters, which force a line-break
    //
    case 0x000A:
    case 0x000C:
    case 0x000D:
    case 0x2028:
    case 0x2029:
      return 0;
    //
    // Space characters, which is taken as a line-break opportunity
    //
    case 0x0020:
    case 0x1680:
    case 0x2000:
    case 0x2001:
    case 0x2002:
    case 0x2003:
    case 0x2004:
    case 0x2005:
    case 0x2006:
    case 0x2008:
    case 0x2009:
    case 0x200A:
    case 0x205F:
    //
    // In-Word Break Opportunities
    //
    case 0x200B:
      return 1;
    //
    // A space which is not a line-break opportunity
    //
    case 0x00A0:
    case 0x202F:
    //
    // A hyphen which is not a line-break opportunity
    //
    case 0x2011:
      return -1;
    //
    // Hyphen characters which describe line break opportunities after the character
    //
    case 0x058A:
    case 0x2010:
    case 0x2012:
    case 0x2013:
    case 0x0F0B:
    case 0x1361:
    case 0x17D5:
      return 1;
    //
    // A hyphen which describes line break opportunities before and after them, but not between a pair of them
    //
    case 0x2014:
      return 2;
  }
  return -1;
}


/**
  Renders a string to a bitmap or to the display.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Flags                   Describes how the string is to be drawn.
  @param  String                  Points to the null-terminated string to be
                                  displayed.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.  If NULL, then the
                                  string will be output in the default system font
                                  and color.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels   wide
                                  and Height pixels high. The string will be drawn
                                  onto this image and
                                  EFI_HII_OUT_FLAG_CLIP is implied. If this points
                                  to a NULL on entry, then a              buffer
                                  will be allocated to hold the generated image and
                                  the pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX                    Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  BltY                    Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  RowInfoArray            If this is non-NULL on entry, then on exit, this
                                  will point to an allocated buffer    containing
                                  row information and RowInfoArraySize will be
                                  updated to contain the        number of elements.
                                  This array describes the characters which were at
                                  least partially drawn and the heights of the
                                  rows. It is the caller's responsibility to free
                                  this buffer.
  @param  RowInfoArraySize        If this is non-NULL on entry, then on exit it
                                  contains the number of elements in RowInfoArray.
  @param  ColumnInfoArray         If this is non-NULL, then on return it will be
                                  filled with the horizontal offset for each
                                  character in the string on the row where it is
                                  displayed. Non-printing characters will     have
                                  the offset ~0. The caller is responsible to
                                  allocate a buffer large enough so that    there
                                  is one entry for each character in the string,
                                  not including the null-terminator. It is possible
                                  when character display is normalized that some
                                  character cells overlap.

  @retval EFI_SUCCESS             The string was successfully rendered.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for
                                  RowInfoArray or Blt.
  @retval EFI_INVALID_PARAMETER   The String or Blt was NULL.
  @retval EFI_INVALID_PARAMETER Flags were invalid combination..

**/
EFI_STATUS
EFIAPI
HiiStringToImage (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  CONST EFI_STRING               String,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  UINT8                               **GlyphBuf;
  EFI_HII_GLYPH_INFO                  *Cell;
  UINT8                               *Attributes;
  EFI_IMAGE_OUTPUT                    *Image;
  EFI_STRING                          StringPtr;
  EFI_STRING                          StringTmp;
  EFI_HII_ROW_INFO                    *RowInfo;
  UINTN                               LineWidth;
  UINTN                               LineHeight;
  UINTN                               LineOffset;
  UINTN                               LastLineHeight;
  UINTN                               BaseLineOffset;
  UINT16                              MaxRowNum;
  UINT16                              RowIndex;
  UINTN                               Index;
  UINTN                               NextIndex;
  UINTN                               Index1;
  EFI_FONT_DISPLAY_INFO               *StringInfoOut;
  EFI_FONT_DISPLAY_INFO               *SystemDefault;
  EFI_FONT_HANDLE                     FontHandle;
  EFI_STRING                          StringIn;
  EFI_STRING                          StringIn2;
  UINT16                              Height;
  UINT16                              BaseLine;
  EFI_FONT_INFO                       *FontInfo;
  BOOLEAN                             SysFontFlag;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       Background;
  BOOLEAN                             Transparent;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BufferPtr;
  UINTN                               RowInfoSize;
  BOOLEAN                             LineBreak;
  UINTN                               StrLength;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *RowBufferPtr;
  HII_GLOBAL_FONT_INFO                *GlobalFont;
  UINT32                              PreInitBkgnd;

  //
  // Check incoming parameters.
  //

  if (This == NULL || String == NULL || Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (*Blt == NULL) {
    //
    // These two flag cannot be used if Blt is NULL upon entry.
    //
    if ((Flags & EFI_HII_OUT_FLAG_TRANSPARENT) == EFI_HII_OUT_FLAG_TRANSPARENT) {
      return EFI_INVALID_PARAMETER;
    }
    if ((Flags & EFI_HII_OUT_FLAG_CLIP) == EFI_HII_OUT_FLAG_CLIP) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // These two flags require that EFI_HII_OUT_FLAG_CLIP be also set.
  //
  if ((Flags & (EFI_HII_OUT_FLAG_CLIP | EFI_HII_OUT_FLAG_CLIP_CLEAN_X)) ==  EFI_HII_OUT_FLAG_CLIP_CLEAN_X) {
    return EFI_INVALID_PARAMETER;
  }
  if ((Flags & (EFI_HII_OUT_FLAG_CLIP | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y)) ==  EFI_HII_OUT_FLAG_CLIP_CLEAN_Y) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // This flag cannot be used with EFI_HII_OUT_FLAG_CLEAN_X.
  //
  if ((Flags & (EFI_HII_OUT_FLAG_WRAP | EFI_HII_OUT_FLAG_CLIP_CLEAN_X)) ==  (EFI_HII_OUT_FLAG_WRAP | EFI_HII_OUT_FLAG_CLIP_CLEAN_X)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Blt == NULL) {
    //
    // Create a new bitmap and draw the string onto this image.
    //
    Image = AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    if (Image == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Image->Width  = 800;
    Image->Height = 600;
    Image->Image.Bitmap = AllocateZeroPool (Image->Width * Image->Height *sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (Image->Image.Bitmap == NULL) {
      FreePool (Image);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Other flags are not permitted when Blt is NULL.
    //
    Flags &= EFI_HII_OUT_FLAG_WRAP | EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_IGNORE_LINE_BREAK;
    *Blt = Image;
  }

  StrLength = StrLen(String);
  GlyphBuf = (UINT8 **) AllocateZeroPool (StrLength * sizeof (UINT8 *));
  ASSERT (GlyphBuf != NULL);
  Cell = (EFI_HII_GLYPH_INFO *) AllocateZeroPool (StrLength * sizeof (EFI_HII_GLYPH_INFO));
  ASSERT (Cell != NULL);
  Attributes = (UINT8 *) AllocateZeroPool (StrLength * sizeof (UINT8));
  ASSERT (Attributes != NULL);

  RowInfo       = NULL;
  Status        = EFI_SUCCESS;
  StringIn2     = NULL;
  SystemDefault = NULL;
  StringIn      = NULL;

  //
  // Calculate the string output information, including specified color and font .
  // If StringInfo does not points to system font info, it must indicate an existing
  // EFI_FONT_INFO.
  //
  StringInfoOut = NULL;
  FontHandle    = NULL;
  Private       = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  SysFontFlag   = IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfo, &SystemDefault, NULL);

  if (SysFontFlag) {
    ASSERT (SystemDefault != NULL);
    FontInfo   = NULL;
    Height     = SystemDefault->FontInfo.FontSize;
    BaseLine   = SystemDefault->FontInfo.FontSize;
    Foreground = SystemDefault->ForegroundColor;
    Background = SystemDefault->BackgroundColor;

  } else {
    //
    //  StringInfo must not be NULL if it is not system info.
    //
    ASSERT (StringInfo != NULL);
    Status = HiiGetFontInfo (This, &FontHandle, (EFI_FONT_DISPLAY_INFO *) StringInfo, &StringInfoOut, NULL);
    if (Status == EFI_NOT_FOUND) {
      //
      // The specified EFI_FONT_DISPLAY_INFO does not exist in current database.
      // Use the system font instead. Still use the color specified by StringInfo.
      //
      SysFontFlag = TRUE;
      FontInfo    = NULL;
      Height      = SystemDefault->FontInfo.FontSize;
      BaseLine    = SystemDefault->FontInfo.FontSize;
      Foreground  = ((EFI_FONT_DISPLAY_INFO *) StringInfo)->ForegroundColor;
      Background  = ((EFI_FONT_DISPLAY_INFO *) StringInfo)->BackgroundColor;

    } else if (Status == EFI_SUCCESS) {
      FontInfo   = &StringInfoOut->FontInfo;
      IsFontInfoExisted (Private, FontInfo, NULL, NULL, &GlobalFont);
      Height     = GlobalFont->FontPackage->Height;
      BaseLine   = GlobalFont->FontPackage->BaseLine;
      Foreground = StringInfoOut->ForegroundColor;
      Background = StringInfoOut->BackgroundColor;
    } else {
      goto Exit;
    }
  }

  //
  // Use the maximum height of font as the base line.
  // And, use the maximum height as line height.
  //
  LineHeight     = Height;
  LastLineHeight = Height;
  BaseLineOffset = Height - BaseLine;

  //
  // Parse the string to be displayed to drop some ignored characters.
  //

  StringPtr = String;

  //
  // Ignore line-break characters only. Hyphens or dash character will be displayed
  // without line-break opportunity.
  //
  if ((Flags & EFI_HII_IGNORE_LINE_BREAK) == EFI_HII_IGNORE_LINE_BREAK) {
    StringIn = AllocateZeroPool (StrSize (StringPtr));
    if (StringIn == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    StringTmp = StringIn;
    while (*StringPtr != 0) {
      if (IsLineBreak (*StringPtr) == 0) {
        StringPtr++;
      } else {
        *StringTmp++ = *StringPtr++;
      }
    }
    *StringTmp = 0;
    StringPtr  = StringIn;
  }
  //
  // If EFI_HII_IGNORE_IF_NO_GLYPH is set, then characters which have no glyphs
  // are not drawn. Otherwise they are replaced with Unicode character 0xFFFD.
  //
  StringIn2  = AllocateZeroPool (StrSize (StringPtr));
  if (StringIn2 == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  Index     = 0;
  StringTmp = StringIn2;
  StrLength = StrLen(StringPtr);
  while (*StringPtr != 0 && Index < StrLength) {
    if (IsLineBreak (*StringPtr) == 0) {
      *StringTmp++ = *StringPtr++;
      Index++;
      continue;
    }

    Status = GetGlyphBuffer (Private, *StringPtr, FontInfo, &GlyphBuf[Index], &Cell[Index], &Attributes[Index]);
    if (Status == EFI_NOT_FOUND) {
      if ((Flags & EFI_HII_IGNORE_IF_NO_GLYPH) == EFI_HII_IGNORE_IF_NO_GLYPH) {
        GlyphBuf[Index] = NULL;
        ZeroMem (&Cell[Index], sizeof (Cell[Index]));
        Status = EFI_SUCCESS;
      } else {
        //
        // Unicode 0xFFFD must exist in current hii database if this flag is not set.
        //
        Status = GetGlyphBuffer (
                   Private,
                   REPLACE_UNKNOWN_GLYPH,
                   FontInfo,
                   &GlyphBuf[Index],
                   &Cell[Index],
                   &Attributes[Index]
                   );
        if (EFI_ERROR (Status)) {
          Status = EFI_INVALID_PARAMETER;
        }
      }
    }

    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    *StringTmp++ = *StringPtr++;
    Index++;
  }
  *StringTmp = 0;
  StringPtr  = StringIn2;

  //
  // Draw the string according to the specified EFI_HII_OUT_FLAGS and Blt.
  // If Blt is not NULL, then EFI_HII_OUT_FLAG_CLIP is implied, render this string
  // to an existing image (bitmap or screen depending on flags) pointed by "*Blt".
  // Otherwise render this string to a new allocated image and output it.
  //
  Image     = *Blt;
  BufferPtr = Image->Image.Bitmap + Image->Width * BltY + BltX;
  if (Image->Height < BltY) {
    //
    // the top edge of the image should be in Image resolution scope.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  MaxRowNum = (UINT16) ((Image->Height - BltY) / Height);
  if ((Image->Height - BltY) % Height != 0) {
    LastLineHeight = (Image->Height - BltY) % Height;
    MaxRowNum++;
  }

  RowInfo = (EFI_HII_ROW_INFO *) AllocateZeroPool (MaxRowNum * sizeof (EFI_HII_ROW_INFO));
  if (RowInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Format the glyph buffer according to flags.
  //
  Transparent = (BOOLEAN) ((Flags & EFI_HII_OUT_FLAG_TRANSPARENT) == EFI_HII_OUT_FLAG_TRANSPARENT ? TRUE : FALSE);

  for (RowIndex = 0, Index = 0; RowIndex < MaxRowNum && StringPtr[Index] != 0; ) {
    LineWidth      = 0;
    LineBreak      = FALSE;

    //
    // Clip the final row if the row's bottom-most on pixel cannot fit when
    // EFI_HII_OUT_FLAG_CLEAN_Y is set.
    //
    if (RowIndex == MaxRowNum - 1) {
      if ((Flags & EFI_HII_OUT_FLAG_CLIP_CLEAN_Y) == EFI_HII_OUT_FLAG_CLIP_CLEAN_Y && LastLineHeight < LineHeight ) {
        //
        // Don't draw at all if the row's bottom-most on pixel cannot fit.
        //
        break;
      }
      LineHeight = LastLineHeight;
    }

    //
    // Calculate how many characters there are in a row.
    //
    RowInfo[RowIndex].StartIndex = Index;

    while (LineWidth + BltX < Image->Width && StringPtr[Index] != 0) {
      if ((Flags & EFI_HII_IGNORE_LINE_BREAK) == 0 &&
           IsLineBreak (StringPtr[Index]) == 0) {
        //
        // It forces a line break that ends this row.
        //
        Index++;
        LineBreak = TRUE;
        break;
      }

      //
      // If the glyph of the character is existing, then accumulate the actual printed width
      //
      LineWidth += (UINTN) Cell[Index].AdvanceX;

      Index++;
    }

    //
    // Record index of next char.
    //
    NextIndex = Index;
    //
    // Return to the previous char.
    //
    Index--;
    if (LineBreak && Index > 0 ) {
      //
      // Return the previous non line break char.
      //
      Index --;
    }

    //
    // If this character is the last character of a row, we need not
    // draw its (AdvanceX - Width - OffsetX) for next character.
    //
    LineWidth -= (Cell[Index].AdvanceX - Cell[Index].Width - Cell[Index].OffsetX);

    //
    // Clip the right-most character if cannot fit when EFI_HII_OUT_FLAG_CLEAN_X is set.
    //
    if (LineWidth + BltX <= Image->Width ||
      (LineWidth + BltX > Image->Width && (Flags & EFI_HII_OUT_FLAG_CLIP_CLEAN_X) == 0)) {
      //
      // Record right-most character in RowInfo even if it is partially displayed.
      //
      RowInfo[RowIndex].EndIndex       = Index;
      RowInfo[RowIndex].LineWidth      = LineWidth;
      RowInfo[RowIndex].LineHeight     = LineHeight;
      RowInfo[RowIndex].BaselineOffset = BaseLineOffset;
    } else {
      //
      // When EFI_HII_OUT_FLAG_CLEAN_X is set, it will not draw a character
      // if its right-most on pixel cannot fit.
      //
      if (Index > RowInfo[RowIndex].StartIndex) {
        //
        // Don't draw the last char on this row. And, don't draw the second last char (AdvanceX - Width - OffsetX).
        //
        LineWidth -= (Cell[Index].Width + Cell[Index].OffsetX);
        LineWidth -= (Cell[Index - 1].AdvanceX - Cell[Index - 1].Width - Cell[Index - 1].OffsetX);
        RowInfo[RowIndex].EndIndex       = Index - 1;
        RowInfo[RowIndex].LineWidth      = LineWidth;
        RowInfo[RowIndex].LineHeight     = LineHeight;
        RowInfo[RowIndex].BaselineOffset = BaseLineOffset;
      } else {
        //
        // There is no enough column to draw any character, so set current line width to zero.
        // And go to draw Next line if LineBreak is set.
        //
        RowInfo[RowIndex].LineWidth      = 0;
        goto NextLine;
      }
    }

    //
    // EFI_HII_OUT_FLAG_WRAP will wrap the text at the right-most line-break
    // opportunity prior to a character whose right-most extent would exceed Width.
    // Search the right-most line-break opportunity here.
    //
    if ((Flags & EFI_HII_OUT_FLAG_WRAP) == EFI_HII_OUT_FLAG_WRAP &&
        (RowInfo[RowIndex].LineWidth + BltX > Image->Width || StringPtr[NextIndex] != 0) &&
        !LineBreak) {
      if ((Flags & EFI_HII_IGNORE_LINE_BREAK) == 0) {
        LineWidth = RowInfo[RowIndex].LineWidth;
        for (Index1 = RowInfo[RowIndex].EndIndex; Index1 >= RowInfo[RowIndex].StartIndex; Index1--) {
          if (Index1 == RowInfo[RowIndex].EndIndex) {
            LineWidth -= (Cell[Index1].Width + Cell[Index1].OffsetX);
          } else {
            LineWidth -= Cell[Index1].AdvanceX;
          }
          if (IsLineBreak (StringPtr[Index1]) > 0) {
            LineBreak = TRUE;
            if (Index1 > RowInfo[RowIndex].StartIndex) {
              RowInfo[RowIndex].EndIndex = Index1 - 1;
            }
            //
            // relocate to the character after the right-most line break opportunity of this line
            //
            NextIndex = Index1 + 1;
            break;
          }
          //
          // If don't find a line break opportunity from EndIndex to StartIndex,
          // then jump out.
          //
          if (Index1 == RowInfo[RowIndex].StartIndex)
            break;
        }

        //
        // Update LineWidth to the real width
        //
        if (IsLineBreak (StringPtr[Index1]) > 0) {
          if (Index1 == RowInfo[RowIndex].StartIndex) {
            LineWidth = 0;
          } else {
            LineWidth -= (Cell[Index1 - 1].AdvanceX - Cell[Index1 - 1].Width - Cell[Index1 - 1].OffsetX);
          }
          RowInfo[RowIndex].LineWidth = LineWidth;
        }
      }
      //
      // If no line-break opportunity can be found, then the text will
      // behave as if EFI_HII_OUT_FLAG_CLEAN_X is set.
      //
      if (!LineBreak) {
        LineWidth = RowInfo[RowIndex].LineWidth;
        Index1    = RowInfo[RowIndex].EndIndex;
        if (LineWidth + BltX > Image->Width) {
          if (Index1 > RowInfo[RowIndex].StartIndex) {
            //
            // Don't draw the last char on this row. And, don't draw the second last char (AdvanceX - Width - OffsetX).
            //
            LineWidth -= (Cell[Index1].Width + Cell[Index1].OffsetX);
            LineWidth -= (Cell[Index1 - 1].AdvanceX - Cell[Index1 - 1].Width - Cell[Index1 - 1].OffsetX);
            RowInfo[RowIndex].EndIndex       = Index1 - 1;
            RowInfo[RowIndex].LineWidth      = LineWidth;
          } else {
            //
            // There is no enough column to draw any character, so set current line width to zero.
            // And go to draw Next line if LineBreak is set.
            //
            RowInfo[RowIndex].LineWidth = 0;
            goto NextLine;
          }
        }
      }
    }

    //
    // LineWidth can't exceed Image width.
    //
    if (RowInfo[RowIndex].LineWidth + BltX > Image->Width) {
      RowInfo[RowIndex].LineWidth = Image->Width - BltX;
    }

    //
    // Draw it to screen or existing bitmap depending on whether
    // EFI_HII_DIRECT_TO_SCREEN is set.
    //
    LineOffset = 0;
    if ((Flags & EFI_HII_DIRECT_TO_SCREEN) == EFI_HII_DIRECT_TO_SCREEN) {
      BltBuffer = NULL;
      if (RowInfo[RowIndex].LineWidth != 0) {
        BltBuffer = AllocatePool (RowInfo[RowIndex].LineWidth * RowInfo[RowIndex].LineHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        if (BltBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Exit;
        }
        //
        // Initialize the background color.
        //
        PreInitBkgnd = Background.Blue | Background.Green << 8 | Background.Red << 16;
        SetMem32 (BltBuffer,RowInfo[RowIndex].LineWidth * RowInfo[RowIndex].LineHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL),PreInitBkgnd);
        //
        // Set BufferPtr to Origin by adding baseline to the starting position.
        //
        BufferPtr = BltBuffer + BaseLine * RowInfo[RowIndex].LineWidth;
      }
      for (Index1 = RowInfo[RowIndex].StartIndex; Index1 <= RowInfo[RowIndex].EndIndex; Index1++) {
        if (RowInfo[RowIndex].LineWidth > 0 && RowInfo[RowIndex].LineWidth > LineOffset) {
          //
          // Only BLT these character which have corresponding glyph in font database.
          //
          GlyphToImage (
            GlyphBuf[Index1],
            Foreground,
            Background,
            (UINT16) RowInfo[RowIndex].LineWidth,
            BaseLine,
            RowInfo[RowIndex].LineWidth - LineOffset,
            RowInfo[RowIndex].LineHeight,
            Transparent,
            &Cell[Index1],
            Attributes[Index1],
            &BufferPtr
          );
        }
        if (ColumnInfoArray != NULL) {
          if ((GlyphBuf[Index1] == NULL && Cell[Index1].AdvanceX == 0)
              || RowInfo[RowIndex].LineWidth == 0) {
            *ColumnInfoArray = (UINTN) ~0;
          } else {
            *ColumnInfoArray = LineOffset + Cell[Index1].OffsetX + BltX;
          }
          ColumnInfoArray++;
        }
        LineOffset += Cell[Index1].AdvanceX;
      }

      if (BltBuffer != NULL) {
        Status = Image->Image.Screen->Blt (
                                        Image->Image.Screen,
                                        BltBuffer,
                                        EfiBltBufferToVideo,
                                        0,
                                        0,
                                        BltX,
                                        BltY,
                                        RowInfo[RowIndex].LineWidth,
                                        RowInfo[RowIndex].LineHeight,
                                        0
                                        );
        if (EFI_ERROR (Status)) {
          FreePool (BltBuffer);
          goto Exit;
        }

        FreePool (BltBuffer);
      }
    } else {
      //
      // Save the starting position for calculate the starting position of next row.
      //
      RowBufferPtr = BufferPtr;
      //
      // Set BufferPtr to Origin by adding baseline to the starting position.
      //
      BufferPtr = BufferPtr + BaseLine * Image->Width;
      for (Index1 = RowInfo[RowIndex].StartIndex; Index1 <= RowInfo[RowIndex].EndIndex; Index1++) {
        if (RowInfo[RowIndex].LineWidth > 0 && RowInfo[RowIndex].LineWidth > LineOffset) {
          //
          // Only BLT these character which have corresponding glyph in font database.
          //
          GlyphToImage (
            GlyphBuf[Index1],
            Foreground,
            Background,
            Image->Width,
            BaseLine,
            RowInfo[RowIndex].LineWidth - LineOffset,
            RowInfo[RowIndex].LineHeight,
            Transparent,
            &Cell[Index1],
            Attributes[Index1],
            &BufferPtr
          );
        }
        if (ColumnInfoArray != NULL) {
          if ((GlyphBuf[Index1] == NULL && Cell[Index1].AdvanceX == 0)
              || RowInfo[RowIndex].LineWidth == 0) {
            *ColumnInfoArray = (UINTN) ~0;
          } else {
            *ColumnInfoArray = LineOffset + Cell[Index1].OffsetX + BltX;
          }
          ColumnInfoArray++;
        }
        LineOffset += Cell[Index1].AdvanceX;
      }

      //
      // Jump to starting position of next row.
      //
      if (RowIndex == 0) {
        BufferPtr = RowBufferPtr - BltX + LineHeight * Image->Width;
      } else {
        BufferPtr = RowBufferPtr + LineHeight * Image->Width;
      }
    }

NextLine:
    //
    // Recalculate the start point of Y axis to draw multi-lines with the order of top-to-down
    //
    BltY += RowInfo[RowIndex].LineHeight;

    RowIndex++;
    Index = NextIndex;

    if (!LineBreak) {
      //
      // If there is not a mandatory line break or line break opportunity, only render one line to image
      //
      break;
    }
  }

  //
  // Write output parameters.
  //
  RowInfoSize = RowIndex * sizeof (EFI_HII_ROW_INFO);
  if (RowInfoArray != NULL) {
    if (RowInfoSize > 0) {
      *RowInfoArray = AllocateZeroPool (RowInfoSize);
      if (*RowInfoArray == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
      CopyMem (*RowInfoArray, RowInfo, RowInfoSize);
    } else {
      *RowInfoArray = NULL;
    }
  }
  if (RowInfoArraySize != NULL) {
    *RowInfoArraySize = RowIndex;
  }

  Status = EFI_SUCCESS;

Exit:

  for (Index = 0; Index < StrLength; Index++) {
    if (GlyphBuf[Index] != NULL) {
      FreePool (GlyphBuf[Index]);
    }
  }
  if (StringIn != NULL) {
    FreePool (StringIn);
  }
  if (StringIn2 != NULL) {
    FreePool (StringIn2);
  }
  if (StringInfoOut != NULL) {
    FreePool (StringInfoOut);
  }
  if (RowInfo != NULL) {
    FreePool (RowInfo);
  }
  if (SystemDefault != NULL) {
    FreePool (SystemDefault);
  }
  if (GlyphBuf != NULL) {
    FreePool (GlyphBuf);
  }
  if (Cell != NULL) {
    FreePool (Cell);
  }
  if (Attributes != NULL) {
    FreePool (Attributes);
  }

  return Status;
}


/**
  Render a string to a bitmap or the screen containing the contents of the specified string.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Flags                   Describes how the string is to be drawn.
  @param  PackageList             The package list in the HII database to search
                                  for the specified string.
  @param  StringId                The string's id, which is unique within
                                  PackageList.
  @param  Language                Points to the language for the retrieved string.
                                  If NULL, then the current system language is
                                  used.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.  If NULL, then the
                                  string will be output in the default system font
                                  and color.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels   wide
                                  and Height pixels high. The string will be drawn
                                  onto this image and
                                  EFI_HII_OUT_FLAG_CLIP is implied. If this points
                                  to a NULL on entry, then a              buffer
                                  will be allocated to hold the generated image and
                                  the pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX                    Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  BltY                    Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  RowInfoArray            If this is non-NULL on entry, then on exit, this
                                  will point to an allocated buffer    containing
                                  row information and RowInfoArraySize will be
                                  updated to contain the        number of elements.
                                  This array describes the characters which were at
                                  least partially drawn and the heights of the
                                  rows. It is the caller's responsibility to free
                                  this buffer.
  @param  RowInfoArraySize        If this is non-NULL on entry, then on exit it
                                  contains the number of elements in RowInfoArray.
  @param  ColumnInfoArray         If this is non-NULL, then on return it will be
                                  filled with the horizontal offset for each
                                  character in the string on the row where it is
                                  displayed. Non-printing characters will     have
                                  the offset ~0. The caller is responsible to
                                  allocate a buffer large enough so that    there
                                  is one entry for each character in the string,
                                  not including the null-terminator. It is possible
                                  when character display is normalized that some
                                  character cells overlap.

  @retval EFI_SUCCESS            The string was successfully rendered.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for
                                 RowInfoArray or Blt.
  @retval EFI_INVALID_PARAMETER  The Blt or PackageList was NULL.
  @retval EFI_INVALID_PARAMETER  Flags were invalid combination.
  @retval EFI_NOT_FOUND          The specified PackageList is not in the Database or the string id is not
                                 in the specified PackageList.

**/
EFI_STATUS
EFIAPI
HiiStringIdToImage (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_STRING_ID                  StringId,
  IN  CONST CHAR8*                   Language,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_HII_STRING_PROTOCOL             *HiiString;
  EFI_STRING                          String;
  UINTN                               StringSize;
  UINTN                               FontLen;
  UINTN                               NameSize;
  EFI_FONT_INFO                       *StringFontInfo;
  EFI_FONT_DISPLAY_INFO               *NewStringInfo;
  CHAR8                               TempSupportedLanguages;
  CHAR8                               *SupportedLanguages;
  UINTN                               SupportedLanguagesSize;
  CHAR8                               *CurrentLanguage;
  CHAR8                               *BestLanguage;

  if (This == NULL || PackageList == NULL || Blt == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  //
  // Initialize string pointers to be NULL
  //
  SupportedLanguages = NULL;
  CurrentLanguage    = NULL;
  BestLanguage       = NULL;
  String             = NULL;
  StringFontInfo     = NULL;
  NewStringInfo      = NULL;

  //
  // Get the string to be displayed.
  //
  Private   = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  HiiString = &Private->HiiString;

  //
  // Get the size of supported language.
  //
  SupportedLanguagesSize = 0;
  Status = HiiString->GetLanguages (
                        HiiString,
                        PackageList,
                        &TempSupportedLanguages,
                        &SupportedLanguagesSize
                        );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  SupportedLanguages = AllocatePool (SupportedLanguagesSize);
  if (SupportedLanguages == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HiiString->GetLanguages (
                        HiiString,
                        PackageList,
                        SupportedLanguages,
                        &SupportedLanguagesSize
                        );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (Language == NULL) {
    Language = "";
  }
  GetEfiGlobalVariable2 (L"PlatformLang", (VOID**)&CurrentLanguage, NULL);
  BestLanguage = GetBestLanguage (
                   SupportedLanguages,
                   FALSE,
                   Language,
                   (CurrentLanguage == NULL) ? CurrentLanguage : "",
                   (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang),
                   NULL
                   );
  if (BestLanguage == NULL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  StringSize = MAX_STRING_LENGTH;
  String = (EFI_STRING) AllocateZeroPool (StringSize);
  if (String == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = HiiString->GetString (
                        HiiString,
                        BestLanguage,
                        PackageList,
                        StringId,
                        String,
                        &StringSize,
                        &StringFontInfo
                        );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (String);
    String = (EFI_STRING) AllocateZeroPool (StringSize);
    if (String == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    Status = HiiString->GetString (
                          HiiString,
                          BestLanguage,
                          PackageList,
                          StringId,
                          String,
                          &StringSize,
                          NULL
                          );
  }

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // When StringInfo specifies that string will be output in the system default font and color,
  // use particular stringfontinfo described in string package instead if exists.
  // StringFontInfo equals NULL means system default font attaches with the string block.
  //
  if (StringFontInfo != NULL && IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfo, NULL, NULL)) {
    NameSize = StrSize (StringFontInfo->FontName);
    FontLen = sizeof (EFI_FONT_DISPLAY_INFO) - sizeof (CHAR16) + NameSize;
    NewStringInfo = AllocateZeroPool (FontLen);
    if (NewStringInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    NewStringInfo->FontInfoMask       = EFI_FONT_INFO_SYS_FORE_COLOR | EFI_FONT_INFO_SYS_BACK_COLOR;
    NewStringInfo->FontInfo.FontStyle = StringFontInfo->FontStyle;
    NewStringInfo->FontInfo.FontSize  = StringFontInfo->FontSize;
    StrCpyS (NewStringInfo->FontInfo.FontName, NameSize / sizeof (CHAR16), StringFontInfo->FontName);

    Status = HiiStringToImage (
               This,
               Flags,
               String,
               NewStringInfo,
               Blt,
               BltX,
               BltY,
               RowInfoArray,
               RowInfoArraySize,
               ColumnInfoArray
               );
    goto Exit;
  }

  Status = HiiStringToImage (
           This,
           Flags,
           String,
           StringInfo,
           Blt,
           BltX,
           BltY,
           RowInfoArray,
           RowInfoArraySize,
           ColumnInfoArray
           );

Exit:
  if (SupportedLanguages != NULL) {
    FreePool (SupportedLanguages);
  }
  if (CurrentLanguage != NULL) {
    FreePool (CurrentLanguage);
  }
  if (BestLanguage != NULL) {
    FreePool (BestLanguage);
  }
  if (String != NULL) {
    FreePool (String);
  }
  if (StringFontInfo != NULL) {
    FreePool (StringFontInfo);
  }
  if (NewStringInfo != NULL) {
    FreePool (NewStringInfo);
  }

  return Status;
}


/**
  Convert the glyph for a single character into a bitmap.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Char                    Character to retrieve.
  @param  StringInfo              Points to the string font and color information
                                  or NULL if the string should use the default
                                  system font and color.
  @param  Blt                     Thus must point to a NULL on entry. A buffer will
                                  be allocated to hold the output and the pointer
                                  updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  Baseline                Number of pixels from the bottom of the bitmap to
                                  the baseline.

  @retval EFI_SUCCESS             Glyph bitmap created.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate the output buffer Blt.
  @retval EFI_WARN_UNKNOWN_GLYPH  The glyph was unknown and was replaced with the
                                  glyph for Unicode character 0xFFFD.
  @retval EFI_INVALID_PARAMETER   Blt is NULL or *Blt is not NULL.

**/
EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  CHAR16                         Char,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo,
  OUT EFI_IMAGE_OUTPUT               **Blt,
  OUT UINTN                          *Baseline OPTIONAL
  )
{
  EFI_STATUS                         Status;
  HII_DATABASE_PRIVATE_DATA          *Private;
  EFI_IMAGE_OUTPUT                   *Image;
  UINT8                              *GlyphBuffer;
  EFI_FONT_DISPLAY_INFO              *SystemDefault;
  EFI_FONT_DISPLAY_INFO              *StringInfoOut;
  BOOLEAN                            Default;
  EFI_FONT_HANDLE                    FontHandle;
  EFI_STRING                         String;
  EFI_HII_GLYPH_INFO                 Cell;
  EFI_FONT_INFO                      *FontInfo;
  UINT8                              Attributes;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer;
  UINT16                             BaseLine;

  if (This == NULL || Blt == NULL || *Blt != NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  Default       = FALSE;
  Image         = NULL;
  SystemDefault = NULL;
  FontHandle    = NULL;
  String        = NULL;
  GlyphBuffer   = NULL;
  StringInfoOut = NULL;
  FontInfo      = NULL;

  ZeroMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  ZeroMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  Default = IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfo, &SystemDefault, NULL);

  if (!Default) {
    //
    // Find out a EFI_FONT_DISPLAY_INFO which could display the character in
    // the specified color and font.
    //
    String = (EFI_STRING) AllocateZeroPool (sizeof (CHAR16) * 2);
    if (String == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    *String = Char;
    *(String + 1) = 0;

    Status = HiiGetFontInfo (This, &FontHandle, StringInfo, &StringInfoOut, String);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
    ASSERT (StringInfoOut != NULL);
    FontInfo   = &StringInfoOut->FontInfo;
    Foreground = StringInfoOut->ForegroundColor;
    Background = StringInfoOut->BackgroundColor;
  } else {
    ASSERT (SystemDefault != NULL);
    Foreground = SystemDefault->ForegroundColor;
    Background = SystemDefault->BackgroundColor;
  }

  Status = GetGlyphBuffer (Private, Char, FontInfo, &GlyphBuffer, &Cell, &Attributes);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Image = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  if (Image == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  Image->Width   = Cell.Width;
  Image->Height  = Cell.Height;

  if (Image->Width * Image->Height > 0) {
    Image->Image.Bitmap = AllocateZeroPool (Image->Width * Image->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (Image->Image.Bitmap == NULL) {
      FreePool (Image);
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    //
    // Set BaseLine to the char height.
    //
    BaseLine  = (UINT16) (Cell.Height + Cell.OffsetY);
    //
    // Set BltBuffer to the position of Origin.
    //
    BltBuffer = Image->Image.Bitmap + (Cell.Height + Cell.OffsetY) * Image->Width - Cell.OffsetX;
    GlyphToImage (
      GlyphBuffer,
      Foreground,
      Background,
      Image->Width,
      BaseLine,
      Cell.Width + Cell.OffsetX,
      BaseLine - Cell.OffsetY,
      FALSE,
      &Cell,
      Attributes,
      &BltBuffer
      );
  }

  *Blt = Image;
  if (Baseline != NULL) {
    *Baseline = Cell.OffsetY;
  }

  Status = EFI_SUCCESS;

Exit:

  if (Status == EFI_NOT_FOUND) {
    //
    // Glyph is unknown and replaced with the glyph for unicode character 0xFFFD
    //
    if (Char != REPLACE_UNKNOWN_GLYPH) {
      Status = HiiGetGlyph (This, REPLACE_UNKNOWN_GLYPH, StringInfo, Blt, Baseline);
      if (!EFI_ERROR (Status)) {
        Status = EFI_WARN_UNKNOWN_GLYPH;
      }
    } else {
      Status = EFI_WARN_UNKNOWN_GLYPH;
    }
  }

  if (SystemDefault != NULL) {
   FreePool (SystemDefault);
  }
  if (StringInfoOut != NULL) {
    FreePool (StringInfoOut);
  }
  if (String != NULL) {
    FreePool (String);
  }
  if (GlyphBuffer != NULL) {
    FreePool (GlyphBuffer);
  }

  return Status;
}


/**
  This function iterates through fonts which match the specified font, using
  the specified criteria. If String is non-NULL, then all of the characters in
  the string must exist in order for a candidate font to be returned.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  FontHandle              On entry, points to the font handle returned by a
                                   previous call to GetFontInfo() or NULL to start
                                  with the  first font. On return, points to the
                                  returned font handle or points to NULL if there
                                  are no more matching fonts.
  @param  StringInfoIn            Upon entry, points to the font to return information
                                  about. If NULL, then the information about the system
                                  default font will be returned.
  @param  StringInfoOut           Upon return, contains the matching font's information.
                                  If NULL, then no information is returned. This buffer
                                  is allocated with a call to the Boot Service AllocatePool().
                                  It is the caller's responsibility to call the Boot
                                  Service FreePool() when the caller no longer requires
                                  the contents of StringInfoOut.
  @param  String                  Points to the string which will be tested to
                                  determine  if all characters are available. If
                                  NULL, then any font  is acceptable.

  @retval EFI_SUCCESS             Matching font returned successfully.
  @retval EFI_NOT_FOUND           No matching font was found.
  @retval EFI_INVALID_PARAMETER  StringInfoIn->FontInfoMask is an invalid combination.
  @retval EFI_OUT_OF_RESOURCES    There were insufficient resources to complete the
                                  request.

**/
EFI_STATUS
EFIAPI
HiiGetFontInfo (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  OUT   EFI_FONT_HANDLE          *FontHandle,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfoIn, OPTIONAL
  OUT       EFI_FONT_DISPLAY_INFO    **StringInfoOut,
  IN  CONST EFI_STRING               String OPTIONAL
  )
{
  HII_DATABASE_PRIVATE_DATA          *Private;
  EFI_STATUS                         Status;
  EFI_FONT_DISPLAY_INFO              *SystemDefault;
  EFI_FONT_DISPLAY_INFO              InfoOut;
  UINTN                              StringInfoOutLen;
  EFI_FONT_INFO                      *FontInfo;
  HII_GLOBAL_FONT_INFO               *GlobalFont;
  EFI_STRING                         StringIn;
  EFI_FONT_HANDLE                    LocalFontHandle;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringInfoOutLen = 0;
  FontInfo        = NULL;
  SystemDefault   = NULL;
  LocalFontHandle = NULL;
  if (FontHandle != NULL) {
    LocalFontHandle = *FontHandle;
  }

  Private = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  //
  // Already searched to the end of the whole list, return directly.
  //
  if (LocalFontHandle == &Private->FontInfoList) {
    LocalFontHandle = NULL;
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  //
  // Get default system display info, if StringInfoIn points to
  // system display info, return it directly.
  //
  if (IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfoIn, &SystemDefault, &StringInfoOutLen)) {
    //
    // System font is the first node. When handle is not NULL, system font can not
    // be found any more.
    //
    if (LocalFontHandle == NULL) {
      if (StringInfoOut != NULL) {
        *StringInfoOut = AllocateCopyPool (StringInfoOutLen, SystemDefault);
        if (*StringInfoOut == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          LocalFontHandle = NULL;
          goto Exit;
        }
      }

      LocalFontHandle = Private->FontInfoList.ForwardLink;
      Status = EFI_SUCCESS;
      goto Exit;
    } else {
      LocalFontHandle = NULL;
      Status = EFI_NOT_FOUND;
      goto Exit;
    }
  }

  //
  // StringInfoIn must not be NULL if it is not system default font info.
  //
  ASSERT (StringInfoIn != NULL);
  //
  // Check the font information mask to make sure it is valid.
  //
  if (((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_SYS_FONT  | EFI_FONT_INFO_ANY_FONT))  ==
       (EFI_FONT_INFO_SYS_FONT | EFI_FONT_INFO_ANY_FONT))   ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_SYS_SIZE  | EFI_FONT_INFO_ANY_SIZE))  ==
       (EFI_FONT_INFO_SYS_SIZE | EFI_FONT_INFO_ANY_SIZE))   ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_SYS_STYLE | EFI_FONT_INFO_ANY_STYLE)) ==
       (EFI_FONT_INFO_SYS_STYLE | EFI_FONT_INFO_ANY_STYLE)) ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_RESIZE    | EFI_FONT_INFO_ANY_SIZE))  ==
       (EFI_FONT_INFO_RESIZE | EFI_FONT_INFO_ANY_SIZE))     ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_RESTYLE   | EFI_FONT_INFO_ANY_STYLE)) ==
       (EFI_FONT_INFO_RESTYLE | EFI_FONT_INFO_ANY_STYLE))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse the font information mask to find a matching font.
  //

  CopyMem (&InfoOut, (EFI_FONT_DISPLAY_INFO *) StringInfoIn, sizeof (EFI_FONT_DISPLAY_INFO));

  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_FONT) == EFI_FONT_INFO_SYS_FONT) {
    Status = SaveFontName (SystemDefault->FontInfo.FontName, &FontInfo);
  } else {
    Status = SaveFontName (((EFI_FONT_DISPLAY_INFO *) StringInfoIn)->FontInfo.FontName, &FontInfo);
  }
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_SIZE) == EFI_FONT_INFO_SYS_SIZE) {
    InfoOut.FontInfo.FontSize = SystemDefault->FontInfo.FontSize;
  }
  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_STYLE) == EFI_FONT_INFO_SYS_STYLE) {
    InfoOut.FontInfo.FontStyle = SystemDefault->FontInfo.FontStyle;
  }
  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_FORE_COLOR) == EFI_FONT_INFO_SYS_FORE_COLOR) {
    InfoOut.ForegroundColor = SystemDefault->ForegroundColor;
  }
  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_BACK_COLOR) == EFI_FONT_INFO_SYS_BACK_COLOR) {
    InfoOut.BackgroundColor = SystemDefault->BackgroundColor;
  }

  ASSERT (FontInfo != NULL);
  FontInfo->FontSize  = InfoOut.FontInfo.FontSize;
  FontInfo->FontStyle = InfoOut.FontInfo.FontStyle;

  if (IsFontInfoExisted (Private, FontInfo, &InfoOut.FontInfoMask, LocalFontHandle, &GlobalFont)) {
    //
    // Test to guarantee all characters are available in the found font.
    //
    if (String != NULL) {
      StringIn = String;
      while (*StringIn != 0) {
        Status = FindGlyphBlock (GlobalFont->FontPackage, *StringIn, NULL, NULL, NULL);
        if (EFI_ERROR (Status)) {
          LocalFontHandle = NULL;
          goto Exit;
        }
        StringIn++;
      }
    }
    //
    // Write to output parameter
    //
    if (StringInfoOut != NULL) {
      StringInfoOutLen = sizeof (EFI_FONT_DISPLAY_INFO) - sizeof (EFI_FONT_INFO) + GlobalFont->FontInfoSize;
      *StringInfoOut   = (EFI_FONT_DISPLAY_INFO *) AllocateZeroPool (StringInfoOutLen);
      if (*StringInfoOut == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        LocalFontHandle = NULL;
        goto Exit;
      }

      CopyMem (*StringInfoOut, &InfoOut, sizeof (EFI_FONT_DISPLAY_INFO));
      CopyMem (&(*StringInfoOut)->FontInfo, GlobalFont->FontInfo, GlobalFont->FontInfoSize);
    }

    LocalFontHandle = GlobalFont->Entry.ForwardLink;
    Status = EFI_SUCCESS;
    goto Exit;
  }

  Status = EFI_NOT_FOUND;

Exit:

  if (FontHandle != NULL) {
    *FontHandle = LocalFontHandle;
  }

  if (SystemDefault != NULL) {
   FreePool (SystemDefault);
  }
  if (FontInfo != NULL) {
   FreePool (FontInfo);
  }
  return Status;
}


