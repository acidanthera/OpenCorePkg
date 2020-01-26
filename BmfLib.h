#ifndef BMF_LIB_H
#define BMF_LIB_H

#include "BmfFile.h"

typedef struct {
  CONST BMF_BLOCK_INFO          *Info;
  CONST BMF_BLOCK_COMMON        *Common;
  CONST BMF_BLOCK_PAGES         *Pages;
  CONST BMF_BLOCK_CHARS         *Chars;
  CONST BMF_BLOCK_KERNING_PAIRS *KerningPairs;
  UINT32                        NumChars;
  UINT32                        NumKerningPairs;
  UINT16                        Height;
  INT16                         OffsetY;
} BMF_CONTEXT;

typedef struct {
  GUI_IMAGE   FontImage;
  BMF_CONTEXT BmfContext;
} GUI_FONT_CONTEXT;

BOOLEAN
GuiInitializeFontHelvetica (
  OUT GUI_FONT_CONTEXT  *Context
  );

VOID
GuiFontDestruct (
  IN GUI_FONT_CONTEXT  *Context
  );

BOOLEAN
GuiGetLabel (
  OUT GUI_IMAGE               *LabelImage,
  IN  CONST GUI_FONT_CONTEXT  *Context,
  IN  CONST CHAR16            *String,
  IN  UINTN                   StringLen
  );

#endif // BMF_LIB_H
