#include <Base.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>

#include "GUI.h"
#include "BmfLib.h"

CONST BMF_CHAR *
BmfGetChar (
  IN CONST BMF_CONTEXT  *Context,
  IN UINT32             Char
  )
{
  CONST BMF_CHAR *Chars;

  UINTN          Left;
  UINTN          Right;
  UINTN          Median;

  ASSERT (Context != NULL);

  Chars = Context->Chars;
  //
  // Binary Search for the character as the list is sorted.
  //
  Left  = 0;
  //
  // As duplicates are not allowed, Right can be ceiled with Char.
  //
  Right = MIN (Context->NumChars, Char) - 1;
  while (Left <= Right) {
    //
    // This cannot wrap around due to the file size limitation.
    //
    Median = (Left + Right) / 2;
    if (Chars[Median].id == Char) {
      return &Chars[Median];
    } else if (Chars[Median].id < Char) {
      Left  = Median + 1;
    } else {
      Right = Median - 1;
    }
  }

  return NULL;
}

CONST BMF_KERNING_PAIR *
BmfGetKerningPair (
  IN CONST BMF_CONTEXT  *Context,
  IN CHAR16             Char1,
  IN CHAR16             Char2
  )
{
  CONST BMF_KERNING_PAIR *Pairs;

  UINTN                  Left;
  UINTN                  Right;
  UINTN                  Median;

  UINTN                  Index;

  ASSERT (Context != NULL);

  Pairs = Context->KerningPairs;
  //
  // Binary Search for the first character as the list is sorted.
  //
  Left  = 0;
  Right = Context->NumKerningPairs - 1;
  while (Left <= Right) {
    //
    // This cannot wrap around due to the file size limitation.
    //
    Median = (Left + Right) / 2;
    if (Pairs[Median].first == Char1) {
      //
      // Flat search for the second character as these are usually a lot less,
      // the list is sorted (relative to the first character).
      //
      if (Pairs[Median].second == Char2) {
        return &Pairs[Median];
      } else if (Pairs[Median].second < Char2) {
        for (
          Index = Median + 1;
          Index < Context->NumKerningPairs && Pairs[Index].first == Char1;
          ++Index
          ) {
          if (Pairs[Index].second == Char2) {
            return &Pairs[Index];
          }
        }
      } else {
        Index = Median;
        while (Index > 0) {
          --Index;
          if (Pairs[Index].first != Char1) {
            break;
          }

          if (Pairs[Index].second == Char2) {
            return &Pairs[Index];
          }
        }
      }

      break;
    } else if (Pairs[Median].first < Char1) {
      Left  = Median + 1;
    } else {
      Right = Median - 1;
    }
  }

  return NULL;
}

BOOLEAN
BmfContextInitialize (
  OUT BMF_CONTEXT  *Context,
  IN  CONST VOID   *FileBuffer,
  IN  UINT32       FileSize
  )
{
  BOOLEAN                Result;

  CONST BMF_HEADER       *Header;
  CONST BMF_BLOCK_HEADER *Block;
  UINTN                  Index;

  INT16                  MinY;
  UINT16                 MaxY;
  INT32                  Width;
  CONST BMF_CHAR         *Chars;
  CONST BMF_KERNING_PAIR *Pairs;

  CONST BMF_CHAR         *Char;

  ASSERT (Context    != NULL);
  ASSERT (FileBuffer != NULL);
  ASSERT (FileSize    > 0);

  Header = FileBuffer;
  //
  // Limit file size for sanity reason and to guarantee wrapping around in BS
  // is not going to happen.
  //
  if (FileSize < sizeof (*Header) || FileSize > BASE_2MB) {
    DEBUG ((DEBUG_WARN, "BMF: FileSize insane\n"));
    return FALSE;
  }

  if (Header->signature[0] != 'B'
   || Header->signature[1] != 'M'
   || Header->signature[2] != 'F'
   || Header->version      != 3) {
    DEBUG ((DEBUG_WARN, "BMF: Header insane\n"));
    return FALSE;
  }

  ZeroMem (Context, sizeof (*Context));

  FileSize -= sizeof (*Header);
  Block = (CONST BMF_BLOCK_HEADER *)(Header + 1);

  while (FileSize >= sizeof (*Block)) {
    FileSize -= sizeof (*Block);
    if (FileSize < Block->size) {
      DEBUG ((DEBUG_WARN, "BMF: Block insane %u vs %u\n", FileSize, Block->size));
      return FALSE;
    }
    FileSize -= Block->size;

    switch (Block->identifier) {
      case BMF_BLOCK_INFO_ID:
      {
        if (Block->size < sizeof (BMF_BLOCK_INFO)) {
          DEBUG ((DEBUG_WARN, "BMF: BlockInfo insane\n"));
          return FALSE;
        }

        Context->Info = (CONST BMF_BLOCK_INFO *)Block;

        DEBUG ((
          DEBUG_INFO,
          "Info->fontSize %u\n"
          "Info->bitField %u\n"
          "Info->charSet %u\n"
          "Info->stretchH %u\n"
          "Info->aa %u\n"
          "Info->paddingUp %u\n"
          "Info->paddingRight %u\n"
          "Info->paddingDown %u\n"
          "Info->paddingLeft %u\n"
          "Info->spacingHoriz %u\n"
          "Info->spacingVert %u\n"
          "Info->outline %u\n"
          "Info->fontName %a\n",
          Context->Info->fontSize,
          Context->Info->bitField,
          Context->Info->charSet,
          Context->Info->stretchH,
          Context->Info->aa,
          Context->Info->paddingUp,
          Context->Info->paddingRight,
          Context->Info->paddingDown,
          Context->Info->paddingLeft,
          Context->Info->spacingHoriz,
          Context->Info->spacingVert,
          Context->Info->outline,
          Context->Info->fontName
          ));

        /*if ((Context->Info->bitField & BMF_BLOCK_INFO_BF_UNICODE) == 0) {
          DEBUG ((DEBUG_WARN, "BMF: Only Unicode is supported\n"));
          return FALSE;
        }*/

        break;
      }

      case BMF_BLOCK_COMMON_ID:
      {
        if (Block->size != sizeof (BMF_BLOCK_COMMON)) {
          DEBUG ((DEBUG_WARN, "BMF: Block Common insane\n"));
          return FALSE;
        }

        Context->Common = (CONST BMF_BLOCK_COMMON *)(Block + 1);
        if (Context->Common->pages != 1) {
          DEBUG ((DEBUG_WARN, "BMF: Only one page is supported\n"));
          return FALSE;
        }

        break;
      }

      case BMF_BLOCK_PAGES_ID:
      {
        Context->Pages = (CONST BMF_BLOCK_PAGES *)(Block + 1);
        break;
      }

      case BMF_BLOCK_CHARS_ID:
      {
        if (Block->size % sizeof (BMF_CHAR) != 0) {
          DEBUG ((DEBUG_WARN, "BMF: Block chars size unaligned\n"));
          return FALSE;
        }

        Context->Chars    = (CONST BMF_BLOCK_CHARS *)(Block + 1);
        Context->NumChars = Block->size / sizeof (BMF_CHAR);
        break;
      }

      case BMF_BLOCK_KERNING_PAIRS_ID:
      {
        if (Block->size % sizeof (BMF_KERNING_PAIR) != 0) {
          DEBUG ((DEBUG_WARN, "BMF: Block Pairs unaligned\n"));
          return FALSE;
        }

        Context->KerningPairs    = (CONST BMF_BLOCK_KERNING_PAIRS *)(Block + 1);
        Context->NumKerningPairs = Block->size / sizeof (BMF_KERNING_PAIR);
        break;
      }

      default:
      {
        //
        // Ignore potential trailer.
        //
        FileSize = 0;
        break;
      }
    }

    Block = (CONST BMF_BLOCK_HEADER *)((UINTN)(Block + 1) + Block->size);
  }

  if (Context->Info         == NULL
   || Context->Common       == NULL
   || Context->Pages        == NULL
   || Context->Chars        == NULL
   || Context->KerningPairs == NULL
    ) {
    DEBUG ((DEBUG_WARN, "BMF: Missing block\n"));
    return FALSE;
  }

  Chars = Context->Chars;
  MinY  = MAX_INT16;
  MaxY  = 0;

  for (Index = 0; Index < Context->NumChars; ++Index) {
    if ((INT32)Chars[Index].yoffset + (INT32)Chars[Index].height <= 0
     || Chars[Index].width < 0 || Chars[Index].xadvance < 0) {
      DEBUG ((
        DEBUG_WARN,
        "BMF: Char insane\n"
        " id %u\n"
        " x %u\n"
        " y %u\n"
        " width %u\n"
        " height %u\n"
        " xoffset %d\n"
        " yoffset %d\n"
        " xadvance %d\n"
        " page %u\n"
        " chnl %u\n",
        Chars[Index].id,
        Chars[Index].x,
        Chars[Index].y,
        Chars[Index].width,
        Chars[Index].height,
        Chars[Index].xoffset,
        Chars[Index].yoffset,
        Chars[Index].xadvance,
        Chars[Index].page,
        Chars[Index].chnl
        ));
      return FALSE;
    }

    Result = OcOverflowAddS32 (
               Chars[Index].xoffset,
               Chars[Index].width,
               &Width
               );
    if (Result || Width < 0) {
      return FALSE;
    }

    Result = OcOverflowAddS32 (
               Chars[Index].xoffset,
               Chars[Index].xadvance,
               &Width
               );
    if (Result || Width < 0) {
      return FALSE;
    }

    MinY = MIN (MinY, Chars[Index].yoffset);
    MaxY = MAX (MaxY, Chars[Index].yoffset + Chars[Index].height);
    //
    // This only yields unexpected but not undefined behaviour when not met,
    // hence it is fine verifying it only DEBUG mode.
    //
    DEBUG_CODE_BEGIN ();
    if (Index > 0) {
      if (Chars[Index - 1].id > Chars[Index].id) {
        DEBUG ((DEBUG_WARN, "BMF: Character IDs are not sorted\n"));
        return FALSE;
      } else if (Chars[Index - 1].id == Chars[Index].id) {
        DEBUG ((DEBUG_WARN, "BMF: Character ID duplicate\n"));
        return FALSE;
      }
    }
    DEBUG_CODE_END ();
  }

  Context->Height  = MaxY - MinY;
  Context->OffsetY = -MinY;

  Pairs = Context->KerningPairs;
  for (Index = 0; Index < Context->NumKerningPairs; ++Index) {
    Char = BmfGetChar (Context, Pairs[Index].first);
    if (Char == NULL) {
      DEBUG ((DEBUG_WARN, "BMF: Pair char not found\n"));
      return FALSE;
    }

    Result = OcOverflowAddS32 (
               Char->xoffset + Char->width,
               Pairs[Index].amount,
               &Width
               );
    if (Result || Width < 0) {
      return FALSE;
    }

    Result = OcOverflowAddS32 (
               Char->xoffset + Char->xadvance,
               Pairs[Index].amount,
               &Width
               );
    if (Result || Width < 0) {
      return FALSE;
    }
    //
    // This only yields unexpected but not undefined behaviour when not met,
    // hence it is fine verifying it only DEBUG mode.
    //
    DEBUG_CODE_BEGIN ();
    if (Index > 0) {
      if (Pairs[Index - 1].first > Pairs[Index].first) {
        DEBUG ((DEBUG_WARN, "BMF: First Character IDs are not sorted\n"));
        return FALSE;
      }

      if (Pairs[Index - 1].first == Pairs[Index].first) {
        if (Pairs[Index - 1].second > Pairs[Index].second) {
          DEBUG ((DEBUG_WARN, "BMF: Second Character IDs are not sorted\n"));
          return FALSE;
        } else if (Pairs[Index - 1].second == Pairs[Index].second) {
          DEBUG ((DEBUG_WARN, "BMF: Second Character ID duplicate\n"));
          return FALSE;
        }
      }
    }
    DEBUG_CODE_END ();
  }
  
  return TRUE;
}

typedef struct {
  UINT16                 Width;
  UINT16                 Height;
  INT16                  OffsetY;
  CONST BMF_CHAR         *Chars[];
//CONST BMF_KERNING_PAIR *KerningPairs[];
} BMF_TEXT_INFO;

BMF_TEXT_INFO *
BmfGetTextInfo (
  IN CONST BMF_CONTEXT  *Context,
  IN CONST CHAR16       *String,
  IN UINTN              StringLen,
  IN UINTN              MaxWidth
  )
{
  BOOLEAN                Result;

  BMF_TEXT_INFO          *TextInfo;
  CONST BMF_KERNING_PAIR **InfoPairs;

  INT32                  Width;
  INT32                  CurWidth;

  CONST BMF_CHAR         *Char;
  CONST BMF_KERNING_PAIR *Pair;
  UINTN                  Index;

  ASSERT (Context != NULL);
  ASSERT (String  != NULL);

  if (StringLen == 0) {
    return NULL;
  }

  ASSERT (String[0] != 0);

  Char = BmfGetChar (Context, String[0]);
  if (Char == NULL) {
    DEBUG ((DEBUG_WARN, "BMF: Text Char not found\n"));
    return NULL;
  }

  TextInfo = AllocatePool (
               sizeof (*TextInfo)
                 + StringLen * sizeof (BMF_CHAR *)
                 + StringLen * sizeof (BMF_BLOCK_KERNING_PAIRS *)
               );
  if (TextInfo == NULL) {
    DEBUG ((DEBUG_WARN, "BMF: Out of res\n"));
    return NULL;
  }
  InfoPairs = (CONST BMF_KERNING_PAIR **)&TextInfo->Chars[StringLen];

  TextInfo->Chars[0] = Char;
  Width = Char->xadvance;

  for (Index = 1; Index < StringLen; ++Index) {
    ASSERT (String[Index] != 0);

    Char = BmfGetChar (Context, String[Index]);
    if (Char == NULL) {
      DEBUG ((DEBUG_WARN, "BMF: Text Char not found\n"));
      FreePool (TextInfo);
      return NULL;
    }
    CurWidth = Char->xadvance;

    Pair = BmfGetKerningPair (Context, String[Index - 1], String[Index]);
    if (Pair != NULL) {
      CurWidth += Pair->amount;
    }

    Result = OcOverflowAddS32 (Width, CurWidth, &Width);
    if (Result) {
      DEBUG ((DEBUG_WARN, "BMF: width overflows\n"));
      FreePool (TextInfo);
      return NULL;
    }

    /*if (Width > MaxWidth) {
      break;
    }*/

    TextInfo->Chars[Index] = Char;
    InfoPairs[Index - 1]   = Pair;
  }

  if (Index != StringLen) {
    CONST BMF_KERNING_PAIR *PeriodPair;

    Char = BmfGetChar (Context, '.');
    if (Char == NULL) {
      FreePool (TextInfo);
      return NULL;
    }

    CurWidth = 2 * (INT32)Char->xadvance + (INT32)Char->xoffset + (INT32)Char->width;

    PeriodPair = BmfGetKerningPair (Context, '.', '.');
    if (PeriodPair != NULL) {
      CurWidth += 2 * (INT32)PeriodPair->amount;
    }
  }

  Width += ((INT32)TextInfo->Chars[Index - 1]->xoffset + (INT32)TextInfo->Chars[Index - 1]->width - (INT32)TextInfo->Chars[Index - 1]->xadvance);

  InfoPairs[Index - 1] = NULL;

  if (Width > MAX_UINT16) {
    DEBUG ((DEBUG_WARN, "BMF: Width exceeds bounds\n"));
    FreePool (TextInfo);
    return NULL;
  }

  TextInfo->Width   = (UINT16)Width;
  TextInfo->Height  = Context->Height;
  TextInfo->OffsetY = Context->OffsetY;
  return TextInfo;
}

BOOLEAN
GuiGetLabel (
  OUT GUI_IMAGE               *LabelImage,
  IN  CONST GUI_FONT_CONTEXT  *Context,
  IN  CONST CHAR16            *String,
  IN  UINTN                   StringLen
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
  BMF_TEXT_INFO                 *TextInfo;
  CONST BMF_KERNING_PAIR        **InfoPairs;
  UINTN                         Index;

  UINT16                        RowIndex;
  UINT32                        SourceRowOffset;
  UINT32                        TargetRowOffset;
  INT32                         TargetCharX;
  INT32                         InitialCharX;
  INT32                         InitialWidthOffset;

  ASSERT (LabelImage != NULL);
  ASSERT (Context    != NULL);
  ASSERT (String     != NULL);

  TextInfo = BmfGetTextInfo (&Context->BmfContext, String, StringLen, 0);
  if (TextInfo == NULL) {
    DEBUG ((DEBUG_WARN, "BMF: GetTextInfo failed\n"));
    return FALSE;
  }

  Buffer = AllocateZeroPool (TextInfo->Width * TextInfo->Height * sizeof (*Buffer));
  if (Buffer == NULL) {
    DEBUG ((DEBUG_WARN, "BMF: out of res\n"));
    FreePool (TextInfo);
    return FALSE;
  }

  InfoPairs   = (CONST BMF_KERNING_PAIR **)&TextInfo->Chars[StringLen];
  TargetCharX = 0;

  if (TextInfo->Chars[0]->xoffset >= 0) {
    InitialCharX       = 0;
    InitialWidthOffset = 0;
  } else {
    InitialCharX       = -TextInfo->Chars[0]->xoffset;
    InitialWidthOffset = TextInfo->Chars[0]->xoffset;
  }

  for (Index = 0; Index < StringLen; ++Index) {
    ASSERT (TextInfo->Chars[Index]->yoffset + TextInfo->OffsetY >= 0);

    for (
      RowIndex = 0,
        SourceRowOffset = TextInfo->Chars[Index]->y * Context->FontImage.Width,
        TargetRowOffset = (TextInfo->Chars[Index]->yoffset + TextInfo->OffsetY) * TextInfo->Width;
      RowIndex < TextInfo->Chars[Index]->height;
      ++RowIndex,
        SourceRowOffset += Context->FontImage.Width,
        TargetRowOffset += TextInfo->Width
      ) {
      CopyMem (
        &Buffer[TargetRowOffset + TargetCharX + TextInfo->Chars[Index]->xoffset + InitialCharX],
        &Context->FontImage.Buffer[SourceRowOffset + TextInfo->Chars[Index]->x + InitialCharX],
        (TextInfo->Chars[Index]->width + InitialWidthOffset) * sizeof (*Buffer)
        );
    }

    TargetCharX += TextInfo->Chars[Index]->xadvance;

    if (InfoPairs[Index] != NULL) {
      TargetCharX += InfoPairs[Index]->amount;
    }

    InitialCharX       = 0;
    InitialWidthOffset = 0;
  }

  LabelImage->Width  = TextInfo->Width;
  LabelImage->Height = TextInfo->Height;
  LabelImage->Buffer = Buffer;
  FreePool (TextInfo);
  return TRUE;
}

BOOLEAN
GuiFontConstruct (
  OUT GUI_FONT_CONTEXT  *Context,
  IN  VOID              *BmpImage,
  IN  UINTN             BmpImageSize,
  IN  CONST VOID        *FileBuffer,
  IN  UINT32            FileSize
  )
{
  RETURN_STATUS Status;
  BOOLEAN       Result;

  ASSERT (Context     != NULL);
  ASSERT (BmpImage    != NULL);
  ASSERT (BmpImageSize > 0);
  ASSERT (FileBuffer  != NULL);
  ASSERT (FileSize     > 0);

  Status = GuiBmpToImage (
             &Context->FontImage,
             BmpImage,
             BmpImageSize
             );
  if (RETURN_ERROR (Status)) {
    return FALSE;
  }

  Result = BmfContextInitialize (&Context->BmfContext, FileBuffer, FileSize);
  if (!Result) {
    return FALSE;
  }
  // TODO: check file size
  return TRUE;
}

VOID
GuiFontDestruct (
  IN GUI_FONT_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->FontImage.Buffer != NULL);
  FreePool (Context->FontImage.Buffer);
}

extern unsigned char Helvetica_fnt[];
extern unsigned int Helvetica_fnt_len;

extern unsigned char Helvetica_0_png[];
extern unsigned int Helvetica_0_png_len;

BOOLEAN
GuiInitializeFontHelvetica (
  OUT GUI_FONT_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  return GuiFontConstruct (
           Context,
           Helvetica_0_png,
           Helvetica_0_png_len,
           Helvetica_fnt,
           Helvetica_fnt_len
           );
}
