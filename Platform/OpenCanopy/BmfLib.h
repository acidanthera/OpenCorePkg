/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef BMF_LIB_H
#define BMF_LIB_H

#include "BmfFile.h"
#include "OpenCanopy.h"

typedef struct {
  CONST BMF_BLOCK_INFO          *Info;
  CONST BMF_BLOCK_COMMON        *Common;
  CONST BMF_BLOCK_PAGES         *Pages;
  CONST BMF_BLOCK_CHARS         *Chars;
  CONST BMF_BLOCK_KERNING_PAIRS *KerningPairs;
  UINT32                        NumChars;
  UINT32                        NumKerningPairs;
  UINT16                        Height;
} BMF_CONTEXT;

typedef struct {
  GUI_IMAGE   FontImage;
  BMF_CONTEXT BmfContext;
  VOID        *KerningData;
  UINT8       Scale;
} GUI_FONT_CONTEXT;

BOOLEAN
GuiFontConstruct (
  OUT GUI_FONT_CONTEXT  *Context,
  IN  VOID              *FontImage,
  IN  UINTN             FontImageSize,
  IN  VOID              *FileBuffer,
  IN  UINT32            FileSize,
  IN  UINT8             Scale
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
  IN  UINTN                   StringLen,
  IN  BOOLEAN                 Inverted
  );

#endif // BMF_LIB_H
