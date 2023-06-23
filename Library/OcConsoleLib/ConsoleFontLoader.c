/** @file
  Load console font from standard format .hex font file - currently supports 8x16 fonts only.

  Copyright (c) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OcConsoleLibInternal.h"

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcStorageLib.h>

#define HEX_LINE_DATA_OFFSET  ((sizeof(CHAR16) * 2) + 1)
#define HEX_LINE_LENGTH       (HEX_LINE_DATA_OFFSET + (ISO_CHAR_HEIGHT * sizeof(UINT8) * 2))

#define HEX_FILE_ERROR_PREFIX  "OCC: Hex font line "

//
// Re-use exactly the structure we want, but with a UINTN in the pointer during
// construction of the data. (Avoid making this a union, as it is not relevant
// to the consumer of a font.)
//
#define PTR_AS_UINTN(ptr)  (*((UINTN *) &ptr))

//
// Parse exactly Length upper or lower case hex digits with no prefix to UINTN.
// Note: AsciiStrHexToUintn and friends allow prefix and surrounding space and
// are auto-terminating (i.e. no specified length), none of which we want here.
//
STATIC
EFI_STATUS
AsciiHexToUintn (
  IN  CHAR8  *Str,
  OUT UINTN  *Data,
  IN  UINTN  Length
  )
{
  UINT8  Char;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Length > sizeof (UINTN) * 2) {
    return EFI_UNSUPPORTED;
  }

  *Data = 0;
  while (Length-- > 0) {
    Char = (UINT8)(*Str++);
    if ((Char >= '0') && (Char <= '9')) {
      Char -= '0';
    } else if ((Char >= 'A') && (Char <= 'F')) {
      Char -= 'A';
      Char += 10;
    } else if ((Char >= 'a') && (Char <= 'f')) {
      Char -= 'a';
      Char += 10;
    } else {
      *Data = 0;
      return EFI_INVALID_PARAMETER;
    }

    *Data = (*Data << 4) + Char;
  }

  return EFI_SUCCESS;
}

//
// Line length already checked.
//
STATIC
EFI_STATUS
ParseHexFontLine (
  UINTN   LineNumber,
  CHAR8   *CurrentLine,
  CHAR16  *Char,
  UINT8   *LineGlyphData
  )
{
  EFI_STATUS  Status;
  UINTN       Data;
  UINT8       Line;

  if (CurrentLine[4] != ':') {
    DEBUG ((DEBUG_WARN, "%a%u illegal line\n", HEX_FILE_ERROR_PREFIX, LineNumber));
    return EFI_UNSUPPORTED;
  }

  Status = AsciiHexToUintn (CurrentLine, &Data, 4);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a%u cannot parse character code\n", HEX_FILE_ERROR_PREFIX, LineNumber));
    return EFI_UNSUPPORTED;
  }

  *Char = (CHAR16)Data;

  for (Line = 0; Line < ISO_CHAR_HEIGHT; Line++) {
    Status = AsciiHexToUintn (&CurrentLine[HEX_LINE_DATA_OFFSET + (Line * 2)], &Data, 2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a%u cannot parse bitmap data\n", HEX_FILE_ERROR_PREFIX, LineNumber));
      break;
    }

    LineGlyphData[Line] = (UINT8)Data;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcLoadConsoleFont (
  IN  OC_STORAGE_CONTEXT  *Storage,
  IN  CONST CHAR8         *FontName,
  OUT OC_CONSOLE_FONT     **Font
  )
{
  EFI_STATUS            Status;
  CHAR16                Path[OC_STORAGE_SAFE_PATH_MAX];
  UINT8                 Line;
  UINTN                 Pass;
  CHAR8                 *HexFontFile;
  CHAR8                 *CurrentLine;
  CHAR8                 *NextLine;
  CHAR8                 *TrimmedLineEnd;
  BOOLEAN               Finished;
  UINTN                 LineNumber;
  OC_FLEX_ARRAY         *FlexFontPages;
  UINT16                FontPageMin;
  UINT16                FontPageMax;
  UINT16                Char;
  UINT8                 LineGlyphData[ISO_CHAR_HEIGHT];
  UINT16                PageNumber;
  UINT8                 PageChar;
  UINT16                CurrentPageNumber;
  OC_CONSOLE_FONT_PAGE  *FontPage;
  UINT8                 PageCharMin;
  UINT8                 PageCharMax;
  UINT8                 FontPageHead;
  UINT8                 FontPageTail;
  UINT8                 GlyphHead;
  UINT8                 GlyphTail;
  UINT16                PageSparseCount;
  UINT8                 GlyphSparseCount;
  BOOLEAN               FoundNonZero;
  UINT32                GlyphOffsetsSize;
  UINT32                GlyphDataSize;
  UINTN                 FontSize;
  UINTN                 FontHeaderSize;
  UINTN                 FontPagesSize;
  UINTN                 FontPageOffsetsSize;
  OC_CONSOLE_FONT_PAGE  *FontPages;
  UINT16                *FontPageOffsets;
  UINT8                 *GlyphOffsets;
  UINT8                 *GlyphData;
  UINT16                *SavedFontPageOffsets;
  UINT8                 *SavedGlyphOffsets;
  UINT8                 *SavedGlyphData;
  VOID                  *SavedEnd;

  ASSERT (Font != NULL);
  *Font = NULL;

  Status = OcUnicodeSafeSPrint (
             Path,
             sizeof (Path),
             OPEN_CORE_FONT_PATH L"\\%a.hex",
             FontName
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCC: Cannot fit %a\n", FontName));
    return EFI_OUT_OF_RESOURCES;
  }

  HexFontFile = OcStorageReadFileUnicode (Storage, Path, NULL);
  if (HexFontFile == NULL) {
    return EFI_NOT_FOUND;
  }

  FlexFontPages = OcFlexArrayInit (sizeof (OC_CONSOLE_FONT_PAGE), NULL);

  if (FlexFontPages == NULL) {
    FreePool (HexFontFile);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Pass = 1; Pass <= 2; Pass++) {
    FontPageMin       = 0;
    FontPageMax       = 0;
    CurrentPageNumber = MAX_UINT16;
    PageSparseCount   = 0;
    GlyphDataSize     = 0;
    GlyphOffsetsSize  = 0;

    NextLine   = HexFontFile;
    LineNumber = 0;

    while (TRUE) {
      CurrentLine = NextLine;
      ++LineNumber;

      //
      // Left trim.
      //
      while (*CurrentLine != '\n' && OcIsSpace (*CurrentLine)) {
        ++CurrentLine;
      }

      Finished = (*CurrentLine == '\0');

      if (!Finished) {
        NextLine = AsciiStrStr (CurrentLine, "\n");
        if (NextLine == NULL) {
          NextLine = &CurrentLine[AsciiStrLen (CurrentLine)];
        } else {
          ++NextLine;
        }

        if (*CurrentLine == '#') {
          continue; ///< Comment, on separate line only.
        }

        //
        // Right trim.
        //
        TrimmedLineEnd = NextLine - 1;
        while (TrimmedLineEnd >= CurrentLine && OcIsSpace (*TrimmedLineEnd)) {
          --TrimmedLineEnd;
        }

        ++TrimmedLineEnd;

        if (TrimmedLineEnd == CurrentLine) {
          continue; ///< Empty line.
        }

        if ((TrimmedLineEnd - CurrentLine) != HEX_LINE_LENGTH) {
          DEBUG ((DEBUG_WARN, "%a%u illegal line length\n", HEX_FILE_ERROR_PREFIX, LineNumber));
          Status = EFI_UNSUPPORTED;
          break;
        }

        Status = ParseHexFontLine (LineNumber, CurrentLine, &Char, LineGlyphData);
        if (EFI_ERROR (Status)) {
          break;
        }

        PageNumber = (Char >> 7) & 0x1FF;
        PageChar   = Char & 0x7F;
      }

      if (Finished || (PageNumber != CurrentPageNumber)) {
        //
        // Output previous page.
        //
        if (CurrentPageNumber != MAX_UINT16) {
          if (Pass == 1) {
            FontPage = OcFlexArrayAddItem (FlexFontPages);
            if (FontPage == NULL) {
              Status = EFI_OUT_OF_RESOURCES;
              break;
            }

            FontPage->CharMin = PageCharMin;
            FontPage->CharMax = PageCharMax;
            if (FontPageHead == ISO_CHAR_HEIGHT) {
              ASSERT (FontPageTail == ISO_CHAR_HEIGHT);
              FontPage->FontHead = ISO_CHAR_HEIGHT - (ISO_CHAR_HEIGHT / 2);
              FontPage->FontTail = (ISO_CHAR_HEIGHT / 2);
            } else {
              FontPage->FontHead = FontPageHead;
              FontPage->FontTail = FontPageTail;
            }

            FontPage->Glyphs                      = NULL;
            PTR_AS_UINTN (FontPage->GlyphOffsets) = GlyphSparseCount;
            FontPage->LeftToRight                 = TRUE;

            GlyphDataSize += (ISO_CHAR_HEIGHT - FontPageHead - FontPageTail) * GlyphSparseCount;
            if (GlyphSparseCount < PageCharMax - PageCharMin) {
              GlyphOffsetsSize += (PageCharMax - PageCharMin);
            }
          } else {
            if (GlyphSparseCount < PageCharMax - PageCharMin) {
              GlyphOffsets += (PageCharMax - PageCharMin);
            }
          }
        }

        if (Finished) {
          break;
        }

        CurrentPageNumber = PageNumber;

        if (FontPageMax == 0) {
          FontPageMin = CurrentPageNumber;
          FontPageMax = CurrentPageNumber + 1;
        } else if (CurrentPageNumber >= FontPageMax) {
          FontPageMax = CurrentPageNumber + 1;
        } else {
          DEBUG ((DEBUG_WARN, "%a%u hex file must be sorted #%u\n", HEX_FILE_ERROR_PREFIX, LineNumber, 1));
          Status = EFI_UNSUPPORTED;
          break;
        }

        ++PageSparseCount;

        PageCharMin = PageChar;
        if (Pass == 1) {
          FontPageHead = ISO_CHAR_HEIGHT;
          FontPageTail = ISO_CHAR_HEIGHT;
        } else {
          FontPage = FontPages;
          ++FontPages;

          ASSERT (FontPage->CharMin == PageCharMin);
          ASSERT (FontPage->CharMax > FontPage->CharMin);
          FontPageHead = FontPage->FontHead;
          FontPageTail = FontPage->FontTail;

          if (FontPageOffsets != NULL) {
            ASSERT ((UINT8 *)&FontPageOffsets[CurrentPageNumber - FontPageMin] < SavedGlyphOffsets);
            FontPageOffsets[CurrentPageNumber - FontPageMin] = PageSparseCount; ///< stores sparse index + 1; zeroes mean no page.
          }

          FontPage->Glyphs = GlyphData;
          //
          // Only use GlyphOffsets table when chars in page are not continuous.
          //
          FontPage->GlyphOffsets =
            ((INTN)PTR_AS_UINTN (FontPage->GlyphOffsets) == (FontPage->CharMax - FontPage->CharMin))
              ? NULL
              : GlyphOffsets;
        }

        PageCharMax      = PageCharMin + 1;
        GlyphSparseCount = 0;
      } else if (PageChar >= PageCharMax) {
        PageCharMax = PageChar + 1;
      } else {
        DEBUG ((DEBUG_WARN, "%a%u hex file must be sorted #%u\n", HEX_FILE_ERROR_PREFIX, LineNumber, 2));
        Status = EFI_UNSUPPORTED;
        break;
      }

      ++GlyphSparseCount;

      if (Pass == 1) {
        GlyphHead    = ISO_CHAR_HEIGHT;
        GlyphTail    = ISO_CHAR_HEIGHT;
        FoundNonZero = FALSE;
        for (Line = 0; Line < ISO_CHAR_HEIGHT; Line++) {
          if (LineGlyphData[Line] != 0) {
            if (!FoundNonZero) {
              GlyphHead    = Line;
              FoundNonZero = TRUE;
            }

            GlyphTail = ISO_CHAR_HEIGHT - Line - 1;
          }
        }

        if (GlyphHead < FontPageHead) {
          FontPageHead = GlyphHead;
        }

        if (GlyphTail < FontPageTail) {
          FontPageTail = GlyphTail;
        }
      } else {
        if (FontPage->GlyphOffsets != NULL) {
          FontPage->GlyphOffsets[PageChar - PageCharMin] = GlyphSparseCount; ///< stores sparse index + 1; zeroes mean no glyph.
        }

        CopyMem (GlyphData, &LineGlyphData[FontPageHead], ISO_CHAR_HEIGHT - FontPageHead - FontPageTail);
        GlyphData += ISO_CHAR_HEIGHT - FontPageHead - FontPageTail;
      }
    }

    ASSERT ((FlexFontPages != NULL) == (Pass == 1));
    ASSERT ((*Font != NULL) == (Pass == 2));

    if (EFI_ERROR (Status)) {
      ASSERT (Pass == 1);
      break;
    }

    if (Pass == 1) {
      FontHeaderSize      = sizeof (OC_CONSOLE_FONT);
      FontPagesSize       = PageSparseCount * sizeof (OC_CONSOLE_FONT_PAGE);
      FontPageOffsetsSize =
        ((FontPageMax - FontPageMin) == PageSparseCount)
          ? 0
          : ((FontPageMax - FontPageMin) * sizeof (UINT16));

      FontSize =
        FontHeaderSize +
        FontPagesSize +
        FontPageOffsetsSize +
        GlyphOffsetsSize +
        GlyphDataSize;

      //
      // Arranged in decreasing alignment requirement, so no need for padding.
      //
      *Font = AllocateZeroPool (FontSize);
      if (*Font == NULL) {
        OcFlexArrayFree (&FlexFontPages);
        return EFI_OUT_OF_RESOURCES;
      }

      DEBUG ((DEBUG_INFO, "OCC: Hex font allocated size %u\n", FontSize));

      FontPages            = (VOID *)((UINT8 *)(*Font) + FontHeaderSize);
      SavedFontPageOffsets = FontPageOffsets = (VOID *)((UINT8 *)FontPages + FontPagesSize);
      SavedGlyphOffsets    = GlyphOffsets = (VOID *)((UINT8 *)FontPageOffsets + FontPageOffsetsSize);
      SavedGlyphData       = GlyphData = (VOID *)((UINT8 *)GlyphOffsets + GlyphOffsetsSize);
      SavedEnd             = (UINT8 *)GlyphData + GlyphDataSize;

      if (FontPageOffsetsSize == 0) {
        FontPageOffsets = NULL;
      }

      if (GlyphOffsetsSize == 0) {
        GlyphOffsets = NULL;
      }

      (*Font)->PageMin     = FontPageMin;
      (*Font)->PageMax     = FontPageMax;
      (*Font)->Pages       = FontPages;
      (*Font)->PageOffsets = FontPageOffsets;

      //
      // Move flex array items to their permanent home.
      //
      ASSERT (FontPagesSize == FlexFontPages->ItemSize * FlexFontPages->Count);
      CopyMem (FontPages, FlexFontPages->Items, FlexFontPages->ItemSize * FlexFontPages->Count);

      OcFlexArrayFree (&FlexFontPages);
    } else {
      ASSERT ((UINT8 *)FontPages == (UINT8 *)SavedFontPageOffsets);
      ASSERT (GlyphOffsets == NULL ? (UINT8 *)SavedGlyphOffsets == (UINT8 *)SavedFontPageOffsets : (UINT8 *)GlyphOffsets == (UINT8 *)SavedGlyphData);
      ASSERT (GlyphData == SavedEnd);
    }
  }

  FreePool (HexFontFile);

  if (!EFI_ERROR (Status)) {
    if (!OcConsoleFontContainsChar (*Font, OC_CONSOLE_FONT_FALLBACK_CHAR)) {
      Status = EFI_UNSUPPORTED;
      DEBUG ((DEBUG_WARN, "OCC: Hex font does not contain fallback character '%c' - %r\n", OC_CONSOLE_FONT_FALLBACK_CHAR, Status));
    }
  }

  if (EFI_ERROR (Status)) {
    if (FlexFontPages != NULL) {
      OcFlexArrayFree (&FlexFontPages);
    }

    if (*Font != NULL) {
      FreePool (*Font);
    }
  }

  return Status;
}
