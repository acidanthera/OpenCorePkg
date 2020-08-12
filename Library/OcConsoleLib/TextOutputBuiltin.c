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

/*
 * ISO Latin-1 Font
 *
 * Copyright (c) 2000
 * Ka-Ping Yee <ping@lfw.org>
 *
 * This font may be freely used for any purpose.
 */

#define ISO_CHAR_MIN    0x21
#define ISO_CHAR_MAX    0x7E
#define ISO_CHAR_WIDTH  8
#define ISO_CHAR_HEIGHT 16

STATIC UINT8 mIsoFontData[(ISO_CHAR_MAX - ISO_CHAR_MIN + 1)*(ISO_CHAR_HEIGHT - 2)] = {
/*  33 */ 0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,
/*  34 */ 0x00,0x6c,0x6c,0x36,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*  35 */ 0x00,0x00,0x36,0x36,0x7f,0x36,0x36,0x7f,0x36,0x36,0x00,0x00,0x00,0x00,
/*  36 */ 0x08,0x08,0x3e,0x6b,0x0b,0x0b,0x3e,0x68,0x68,0x6b,0x3e,0x08,0x08,0x00,
/*  37 */ 0x00,0x00,0x33,0x13,0x18,0x08,0x0c,0x04,0x06,0x32,0x33,0x00,0x00,0x00,
/*  38 */ 0x00,0x1c,0x36,0x36,0x1c,0x6c,0x3e,0x33,0x33,0x7b,0xce,0x00,0x00,0x00,
/*  39 */ 0x00,0x18,0x18,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*  40 */ 0x00,0x30,0x18,0x18,0x0c,0x0c,0x0c,0x0c,0x0c,0x18,0x18,0x30,0x00,0x00,
/*  41 */ 0x00,0x0c,0x18,0x18,0x30,0x30,0x30,0x30,0x30,0x18,0x18,0x0c,0x00,0x00,
/*  42 */ 0x00,0x00,0x00,0x00,0x36,0x1c,0x7f,0x1c,0x36,0x00,0x00,0x00,0x00,0x00,
/*  43 */ 0x00,0x00,0x00,0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00,0x00,0x00,0x00,
/*  44 */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x0c,0x00,0x00,
/*  45 */ 0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*  46 */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,
/*  47 */ 0x00,0x60,0x20,0x30,0x10,0x18,0x08,0x0c,0x04,0x06,0x02,0x03,0x00,0x00,
/*  48 */ 0x00,0x3e,0x63,0x63,0x63,0x6b,0x6b,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,
/*  49 */ 0x00,0x18,0x1e,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,
/*  50 */ 0x00,0x3e,0x63,0x60,0x60,0x30,0x18,0x0c,0x06,0x03,0x7f,0x00,0x00,0x00,
/*  51 */ 0x00,0x3e,0x63,0x60,0x60,0x3c,0x60,0x60,0x60,0x63,0x3e,0x00,0x00,0x00,
/*  52 */ 0x00,0x30,0x38,0x3c,0x36,0x33,0x7f,0x30,0x30,0x30,0x30,0x00,0x00,0x00,
/*  53 */ 0x00,0x7f,0x03,0x03,0x3f,0x60,0x60,0x60,0x60,0x63,0x3e,0x00,0x00,0x00,
/*  54 */ 0x00,0x3c,0x06,0x03,0x03,0x3f,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,
/*  55 */ 0x00,0x7f,0x60,0x30,0x30,0x18,0x18,0x18,0x0c,0x0c,0x0c,0x00,0x00,0x00,
/*  56 */ 0x00,0x3e,0x63,0x63,0x63,0x3e,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,
/*  57 */ 0x00,0x3e,0x63,0x63,0x63,0x7e,0x60,0x60,0x60,0x30,0x1e,0x00,0x00,0x00,
/*  58 */ 0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,
/*  59 */ 0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x0c,0x00,0x00,
/*  60 */ 0x00,0x60,0x30,0x18,0x0c,0x06,0x06,0x0c,0x18,0x30,0x60,0x00,0x00,0x00,
/*  61 */ 0x00,0x00,0x00,0x00,0x7e,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,
/*  62 */ 0x00,0x06,0x0c,0x18,0x30,0x60,0x60,0x30,0x18,0x0c,0x06,0x00,0x00,0x00,
/*  63 */ 0x00,0x3e,0x63,0x60,0x30,0x30,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,
/*  64 */ 0x00,0x3c,0x66,0x73,0x7b,0x6b,0x6b,0x7b,0x33,0x06,0x3c,0x00,0x00,0x00,
/*  65 */ 0x00,0x3e,0x63,0x63,0x63,0x7f,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,
/*  66 */ 0x00,0x3f,0x63,0x63,0x63,0x3f,0x63,0x63,0x63,0x63,0x3f,0x00,0x00,0x00,
/*  67 */ 0x00,0x3c,0x66,0x03,0x03,0x03,0x03,0x03,0x03,0x66,0x3c,0x00,0x00,0x00,
/*  68 */ 0x00,0x1f,0x33,0x63,0x63,0x63,0x63,0x63,0x63,0x33,0x1f,0x00,0x00,0x00,
/*  69 */ 0x00,0x7f,0x03,0x03,0x03,0x3f,0x03,0x03,0x03,0x03,0x7f,0x00,0x00,0x00,
/*  70 */ 0x00,0x7f,0x03,0x03,0x03,0x3f,0x03,0x03,0x03,0x03,0x03,0x00,0x00,0x00,
/*  71 */ 0x00,0x3c,0x66,0x03,0x03,0x03,0x73,0x63,0x63,0x66,0x7c,0x00,0x00,0x00,
/*  72 */ 0x00,0x63,0x63,0x63,0x63,0x7f,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,
/*  73 */ 0x00,0x3c,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,
/*  74 */ 0x00,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x33,0x1e,0x00,0x00,0x00,
/*  75 */ 0x00,0x63,0x33,0x1b,0x0f,0x07,0x07,0x0f,0x1b,0x33,0x63,0x00,0x00,0x00,
/*  76 */ 0x00,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x7f,0x00,0x00,0x00,
/*  77 */ 0x00,0x63,0x63,0x77,0x7f,0x7f,0x6b,0x6b,0x63,0x63,0x63,0x00,0x00,0x00,
/*  78 */ 0x00,0x63,0x63,0x67,0x6f,0x6f,0x7b,0x7b,0x73,0x63,0x63,0x00,0x00,0x00,
/*  79 */ 0x00,0x3e,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,
/*  80 */ 0x00,0x3f,0x63,0x63,0x63,0x63,0x3f,0x03,0x03,0x03,0x03,0x00,0x00,0x00,
/*  81 */ 0x00,0x3e,0x63,0x63,0x63,0x63,0x63,0x63,0x6f,0x7b,0x3e,0x30,0x60,0x00,
/*  82 */ 0x00,0x3f,0x63,0x63,0x63,0x63,0x3f,0x1b,0x33,0x63,0x63,0x00,0x00,0x00,
/*  83 */ 0x00,0x3e,0x63,0x03,0x03,0x0e,0x38,0x60,0x60,0x63,0x3e,0x00,0x00,0x00,
/*  84 */ 0x00,0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,
/*  85 */ 0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,
/*  86 */ 0x00,0x63,0x63,0x63,0x63,0x63,0x36,0x36,0x1c,0x1c,0x08,0x00,0x00,0x00,
/*  87 */ 0x00,0x63,0x63,0x6b,0x6b,0x6b,0x6b,0x7f,0x36,0x36,0x36,0x00,0x00,0x00,
/*  88 */ 0x00,0x63,0x63,0x36,0x36,0x1c,0x1c,0x36,0x36,0x63,0x63,0x00,0x00,0x00,
/*  89 */ 0x00,0xc3,0xc3,0x66,0x66,0x3c,0x3c,0x18,0x18,0x18,0x18,0x00,0x00,0x00,
/*  90 */ 0x00,0x7f,0x30,0x30,0x18,0x18,0x0c,0x0c,0x06,0x06,0x7f,0x00,0x00,0x00,
/*  91 */ 0x00,0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00,0x00,0x00,
/*  92 */ 0x00,0x03,0x02,0x06,0x04,0x0c,0x08,0x18,0x10,0x30,0x20,0x60,0x00,0x00,
/*  93 */ 0x00,0x3c,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3c,0x00,0x00,0x00,
/*  94 */ 0x08,0x1c,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*  95 */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
/*  96 */ 0x00,0x0c,0x0c,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*  97 */ 0x00,0x00,0x00,0x00,0x3e,0x60,0x7e,0x63,0x63,0x73,0x6e,0x00,0x00,0x00,
/*  98 */ 0x00,0x03,0x03,0x03,0x3b,0x67,0x63,0x63,0x63,0x67,0x3b,0x00,0x00,0x00,
/*  99 */ 0x00,0x00,0x00,0x00,0x3e,0x63,0x03,0x03,0x03,0x63,0x3e,0x00,0x00,0x00,
/* 100 */ 0x00,0x60,0x60,0x60,0x6e,0x73,0x63,0x63,0x63,0x73,0x6e,0x00,0x00,0x00,
/* 101 */ 0x00,0x00,0x00,0x00,0x3e,0x63,0x63,0x7f,0x03,0x63,0x3e,0x00,0x00,0x00,
/* 102 */ 0x00,0x3c,0x66,0x06,0x1f,0x06,0x06,0x06,0x06,0x06,0x06,0x00,0x00,0x00,
/* 103 */ 0x00,0x00,0x00,0x00,0x6e,0x73,0x63,0x63,0x63,0x73,0x6e,0x60,0x63,0x3e,
/* 104 */ 0x00,0x03,0x03,0x03,0x3b,0x67,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,
/* 105 */ 0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x38,0x00,0x00,0x00,
/* 106 */ 0x00,0x30,0x30,0x00,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x33,0x1e,
/* 107 */ 0x00,0x03,0x03,0x03,0x63,0x33,0x1b,0x0f,0x1f,0x33,0x63,0x00,0x00,0x00,
/* 108 */ 0x00,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x38,0x00,0x00,0x00,
/* 109 */ 0x00,0x00,0x00,0x00,0x35,0x6b,0x6b,0x6b,0x6b,0x6b,0x6b,0x00,0x00,0x00,
/* 110 */ 0x00,0x00,0x00,0x00,0x3b,0x67,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,
/* 111 */ 0x00,0x00,0x00,0x00,0x3e,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,
/* 112 */ 0x00,0x00,0x00,0x00,0x3b,0x67,0x63,0x63,0x63,0x67,0x3b,0x03,0x03,0x03,
/* 113 */ 0x00,0x00,0x00,0x00,0x6e,0x73,0x63,0x63,0x63,0x73,0x6e,0x60,0xe0,0x60,
/* 114 */ 0x00,0x00,0x00,0x00,0x3b,0x67,0x03,0x03,0x03,0x03,0x03,0x00,0x00,0x00,
/* 115 */ 0x00,0x00,0x00,0x00,0x3e,0x63,0x0e,0x38,0x60,0x63,0x3e,0x00,0x00,0x00,
/* 116 */ 0x00,0x00,0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x0c,0x0c,0x38,0x00,0x00,0x00,
/* 117 */ 0x00,0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x73,0x6e,0x00,0x00,0x00,
/* 118 */ 0x00,0x00,0x00,0x00,0x63,0x63,0x36,0x36,0x1c,0x1c,0x08,0x00,0x00,0x00,
/* 119 */ 0x00,0x00,0x00,0x00,0x63,0x6b,0x6b,0x6b,0x3e,0x36,0x36,0x00,0x00,0x00,
/* 120 */ 0x00,0x00,0x00,0x00,0x63,0x36,0x1c,0x1c,0x1c,0x36,0x63,0x00,0x00,0x00,
/* 121 */ 0x00,0x00,0x00,0x00,0x63,0x63,0x36,0x36,0x1c,0x1c,0x0c,0x0c,0x06,0x03,
/* 122 */ 0x00,0x00,0x00,0x00,0x7f,0x60,0x30,0x18,0x0c,0x06,0x7f,0x00,0x00,0x00,
/* 123 */ 0x00,0x70,0x18,0x18,0x18,0x18,0x0e,0x18,0x18,0x18,0x18,0x70,0x00,0x00,
/* 124 */ 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,
/* 125 */ 0x00,0x0e,0x18,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0x0e,0x00,0x00,
/* 126 */ 0x00,0x00,0x00,0x00,0x00,0x6e,0x3b,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

STATIC UINT32 mGraphicsEfiColors[16] = {
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

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL *mGraphicsOutput;
STATIC UINTN  mConsoleWidth;
STATIC UINTN  mConsoleHeight;
STATIC UINTN  mConsoleMaxPosX;
STATIC UINTN  mConsoleMaxPosY;
STATIC UINTN  mPrivateColumn; ///< At least UEFI Shell trashes Mode values.
STATIC UINTN  mPrivateRow;    ///< At least UEFI Shell trashes Mode values.
STATIC UINT32 mConsoleGopMode;
STATIC UINT8  mFontScale;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION mBackgroundColor;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION mForegroundColor;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *mCharacterBuffer;
STATIC EFI_CONSOLE_CONTROL_SCREEN_MODE     mConsoleMode = EfiConsoleControlScreenText;

#define SCR_PADD           1
#define TGT_CHAR_WIDTH     ((UINTN)(ISO_CHAR_WIDTH) * mFontScale)
#define TGT_CHAR_HEIGHT    ((UINTN)(ISO_CHAR_HEIGHT) * mFontScale)
#define TGT_CHAR_AREA      ((TGT_CHAR_WIDTH) * (TGT_CHAR_HEIGHT))
#define TGT_PADD_WIDTH     ((TGT_CHAR_WIDTH) * (SCR_PADD))
#define TGT_PADD_HEIGHT    ((ISO_CHAR_HEIGHT) * (SCR_PADD))
#define TGT_CURSOR_X       mFontScale
#define TGT_CURSOR_Y       ((TGT_CHAR_HEIGHT) - mFontScale)
#define TGT_CURSOR_WIDTH   ((TGT_CHAR_WIDTH) - mFontScale*2)
#define TGT_CURSOR_HEIGHT  (mFontScale)

/**
  Render character onscreen.

  @param[in]  Char  Character code.
  @param[in]  PosX  Character X position.
  @param[in]  PosY  Character Y position.
**/
STATIC
VOID
RenderChar (
  IN CHAR16   Char,
  IN UINTN    PosX,
  IN UINTN    PosY
  )
{
  UINT32  *DstBuffer;
  UINT8   *SrcBuffer;
  UINT32  Line;
  UINT32  Index;
  UINT32  Index2;
  UINT8   Mask;

  DstBuffer = &mCharacterBuffer[0].Raw;

  if ((Char >= 0 && Char < ISO_CHAR_MIN) || Char == ' ' || Char == CHAR_TAB || Char == 0x7F) {
    SetMem32 (DstBuffer, TGT_CHAR_AREA * sizeof (DstBuffer[0]), mBackgroundColor.Raw);
  } else {

    if (Char < 0 || Char > ISO_CHAR_MAX) {
      Char = L'_';
    }

    SrcBuffer = mIsoFontData + ((Char - ISO_CHAR_MIN) * (ISO_CHAR_HEIGHT - 2));

    SetMem32 (DstBuffer, TGT_CHAR_WIDTH * mFontScale * sizeof (DstBuffer[0]), mBackgroundColor.Raw);
    DstBuffer += TGT_CHAR_WIDTH * mFontScale;

    for (Line = 0; Line < ISO_CHAR_HEIGHT - 2; ++Line) {
      //
      // Iterate, while the single bit drops to the right.
      //
      for (Index = 0; Index < mFontScale; ++Index) {
        Mask = 1;
        do {
          for (Index2 = 0; Index2 < mFontScale; ++Index2) {
            *DstBuffer = (*SrcBuffer & Mask) ? mForegroundColor.Raw : mBackgroundColor.Raw;
            ++DstBuffer;
          }
          Mask <<= 1U;
        } while (Mask != 0);
      }
      ++SrcBuffer;
    }

    SetMem32 (DstBuffer, TGT_CHAR_WIDTH * mFontScale * sizeof (DstBuffer[0]), mBackgroundColor.Raw);
  }

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

  if (!Enabled) {
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
  //
  // Move data.
  //
  mGraphicsOutput->Blt (
    mGraphicsOutput,
    NULL,
    EfiBltVideoToVideo,
    0,
    TGT_PADD_HEIGHT + TGT_CHAR_HEIGHT,
    0,
    TGT_PADD_HEIGHT,
    mGraphicsOutput->Mode->Info->HorizontalResolution,
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
    0,
    TGT_PADD_HEIGHT + TGT_CHAR_HEIGHT * (mConsoleHeight - 1),
    mGraphicsOutput->Mode->Info->HorizontalResolution,
    TGT_CHAR_HEIGHT,
    0
    );
}

STATIC
EFI_STATUS
RenderResync (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
  )
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  Info = mGraphicsOutput->Mode->Info;

  if (Info->HorizontalResolution < TGT_CHAR_WIDTH * 3
    || Info->VerticalResolution < TGT_CHAR_HEIGHT * 3) {
    return EFI_DEVICE_ERROR;
  }

  if (mCharacterBuffer != NULL) {
    FreePool (mCharacterBuffer);
  }

  mCharacterBuffer = AllocatePool (TGT_CHAR_AREA * sizeof (mCharacterBuffer[0]));
  if (mCharacterBuffer == NULL) {
    return EFI_DEVICE_ERROR;
  }

  mConsoleGopMode          = mGraphicsOutput->Mode->Mode;
  mConsoleWidth            = (Info->HorizontalResolution / TGT_CHAR_WIDTH)  - 2 * SCR_PADD;
  mConsoleHeight           = (Info->VerticalResolution   / TGT_CHAR_HEIGHT) - 2 * SCR_PADD;
  mConsoleMaxPosX          = 0;
  mConsoleMaxPosY          = 0;

  mPrivateColumn = mPrivateRow = 0;
  This->Mode->CursorColumn = This->Mode->CursorRow = 0;

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
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextReset (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN BOOLEAN                         ExtendedVerification
  )
{
  EFI_STATUS                            Status;
  EFI_TPL                               OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = OcHandleProtocolFallback (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &mGraphicsOutput
    );

  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return EFI_DEVICE_ERROR;
  }

  This->Mode->MaxMode      = 1;
  This->Mode->Attribute    = ARRAY_SIZE (mGraphicsEfiColors) / 2 - 1;
  mBackgroundColor.Raw     = mGraphicsEfiColors[0];
  mForegroundColor.Raw     = mGraphicsEfiColors[ARRAY_SIZE (mGraphicsEfiColors) / 2 - 1];

  Status = RenderResync (This);
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
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
      return Status;
    }
  }

  //
  // For whatever reason UEFI Shell trashes these values when executing commands like help -b.
  //
  This->Mode->CursorColumn = (INT32) mPrivateColumn;
  This->Mode->CursorRow    = (INT32) mPrivateRow;

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
      if (This->Mode->CursorColumn == 0 && This->Mode->CursorRow > 0) {
        This->Mode->CursorRow--;
        This->Mode->CursorColumn = (INT32) (mConsoleWidth - 1);
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
      if ((UINTN) This->Mode->CursorRow < mConsoleHeight - 1) {
        ++This->Mode->CursorRow;
        mConsoleMaxPosY = MAX (mConsoleMaxPosY, (UINTN) This->Mode->CursorRow);
      } else {
        RenderScroll ();
      }
      continue;
    }

    //
    // Render normal symbol and decide on next cursor position.
    //
    RenderChar (String[Index], This->Mode->CursorColumn, This->Mode->CursorRow);
    if ((UINTN) This->Mode->CursorColumn < mConsoleWidth - 1) {
      //
      // Continues on the same line.
      //
      ++This->Mode->CursorColumn;
      mConsoleMaxPosX = MAX (mConsoleMaxPosX, (UINTN) This->Mode->CursorColumn);
    } else if ((UINTN) This->Mode->CursorRow < mConsoleHeight - 1) {
      //
      // New line without scroll.
      //
      This->Mode->CursorColumn = 0;
      ++This->Mode->CursorRow;
      mConsoleMaxPosY = MAX (mConsoleMaxPosY, (UINTN) This->Mode->CursorRow);
    } else {
      //
      // New line with scroll.
      //
      RenderScroll ();
      This->Mode->CursorColumn = 0;
    }
  }

  FlushCursor (This->Mode->CursorVisible, This->Mode->CursorColumn, This->Mode->CursorRow);

  mPrivateColumn = (UINTN) This->Mode->CursorColumn;
  mPrivateRow    = (UINTN) This->Mode->CursorRow;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextTestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
  )
{
  if (StrCmp (String, OC_CONSOLE_MARK_UNCONTROLLED) == 0) {
    mConsoleMaxPosX = mGraphicsOutput->Mode->Info->HorizontalResolution / TGT_CHAR_WIDTH;
    mConsoleMaxPosY = mGraphicsOutput->Mode->Info->VerticalResolution   / TGT_CHAR_HEIGHT;
  } else if (StrCmp (String, OC_CONSOLE_MARK_CONTROLLED) == 0) {
    mConsoleMaxPosX = 0;
    mConsoleMaxPosX = 0;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           ModeNumber,
  OUT UINTN                           *Columns,
  OUT UINTN                           *Rows
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return Status;
    }
  }

  if (ModeNumber == 0) {
    *Columns = mConsoleWidth;
    *Rows    = mConsoleHeight;
    Status   = EFI_SUCCESS;
  } else {
    Status   = EFI_UNSUPPORTED;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextSetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN UINTN                           ModeNumber
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
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextSetAttribute (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN UINTN                           Attribute
  )
{
  EFI_STATUS  Status;
  UINT32      FgColor;
  UINT32      BgColor;
  EFI_TPL     OldTpl;

  if ((Attribute & ~0x7FU) != 0) {
    return EFI_UNSUPPORTED;
  }

  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return Status;
    }
  }

  if (Attribute != (UINTN) This->Mode->Attribute) {
    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);

    FgColor = BitFieldRead32 ((UINT32) Attribute, 0, 3);
    BgColor = BitFieldRead32 ((UINT32) Attribute, 4, 6);

    //
    // Once we change the background we should redraw everything.
    //
    if (mGraphicsEfiColors[BgColor] != mBackgroundColor.Raw) {
      mConsoleMaxPosX = mConsoleWidth + SCR_PADD;
      mConsoleMaxPosY = mConsoleHeight + SCR_PADD;
    }

    mForegroundColor.Raw  = mGraphicsEfiColors[FgColor];
    mBackgroundColor.Raw  = mGraphicsEfiColors[BgColor];
    This->Mode->Attribute = (UINT32) Attribute;

    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
  )
{
  EFI_STATUS  Status;
  UINTN       Width;
  UINTN       Height;
  EFI_TPL     OldTpl;

  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return Status;
    }
  }

  //
  // X coordinate points to the right most coordinate of the last printed
  // character, but after this character we may also have cursor.
  // Y coordinate points to the top most coordinate of the last printed row.
  //
  Width  = TGT_PADD_WIDTH  + (mConsoleMaxPosX + 1) * TGT_CHAR_WIDTH;
  Height = TGT_PADD_HEIGHT + (mConsoleMaxPosY + 1) * TGT_CHAR_HEIGHT;

  mGraphicsOutput->Blt (
    mGraphicsOutput,
    &mBackgroundColor.Pixel,
    EfiBltVideoFill,
    0,
    0,
    0,
    0,
    MIN (Width, mGraphicsOutput->Mode->Info->HorizontalResolution),
    MIN (Height, mGraphicsOutput->Mode->Info->VerticalResolution),
    0
    );

  //
  // Handle cursor.
  //
  mPrivateColumn = mPrivateRow = 0;
  This->Mode->CursorColumn  = This->Mode->CursorRow = 0;
  FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);

  //
  // We do not reset max here, as we may still scroll (e.g. in shell via page buttons).
  //

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AsciiTextSetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN UINTN                           Column,
  IN UINTN                           Row
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return Status;
    }
  }

  if (Column < mConsoleWidth && Row < mConsoleHeight) {
    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
    mPrivateColumn = Column;
    mPrivateRow    = Row;
    This->Mode->CursorColumn = (INT32) mPrivateColumn;
    This->Mode->CursorRow    = (INT32) mPrivateRow;
    FlushCursor (This->Mode->CursorVisible, mPrivateColumn, mPrivateRow);
    mConsoleMaxPosX = MAX (mConsoleMaxPosX, Column);
    mConsoleMaxPosY = MAX (mConsoleMaxPosY, Row);
    Status = EFI_SUCCESS;
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
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN BOOLEAN                         Visible
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (mConsoleGopMode != mGraphicsOutput->Mode->Mode) {
    Status = RenderResync (This);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (OldTpl);
      return Status;
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
  IN   EFI_CONSOLE_CONTROL_PROTOCOL    *This,
  OUT  EFI_CONSOLE_CONTROL_SCREEN_MODE *Mode,
  OUT  BOOLEAN                         *GopUgaExists OPTIONAL,
  OUT  BOOLEAN                         *StdInLocked  OPTIONAL
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
  mConsoleMode = Mode;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlLockStdIn (
  IN EFI_CONSOLE_CONTROL_PROTOCOL *This,
  IN CHAR16                       *Password
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

VOID
ConsoleControlInstall (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  Status = OcHandleProtocolFallback (
    &gST->ConsoleOutHandle,
    &gEfiConsoleControlProtocolGuid,
    (VOID *) &ConsoleControl
    );

  if (!EFI_ERROR (Status)) {
    ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenGraphics);

    CopyMem (
      ConsoleControl,
      &mConsoleControlProtocol,
      sizeof (mConsoleControlProtocol)
      );
  }

  gBS->InstallMultipleProtocolInterfaces (
    &gST->ConsoleOutHandle,
    &gEfiConsoleControlProtocolGuid,
    &mConsoleControlProtocol,
    NULL
    );
}

VOID
OcUseBuiltinTextOutput (
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  )
{
  EFI_STATUS  Status;
  UINTN       UiScaleSize;

  UiScaleSize = sizeof (mFontScale);

  Status = gRT->GetVariable (
    APPLE_UI_SCALE_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    NULL,
    &UiScaleSize,
    (VOID *) &mFontScale
    );

  if (EFI_ERROR (Status) || mFontScale != 2) {
    mFontScale = 1;
  }

  DEBUG ((DEBUG_INFO, "OCC: Using builtin text renderer with %d scale\n", mFontScale));

  Status = AsciiTextReset (&mAsciiTextOutputProtocol, TRUE);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Cannot setup ASCII output - %r\n", Status));
    return;
  }

  OcConsoleControlSetMode (Mode);
  OcConsoleControlInstallProtocol (&mConsoleControlProtocol, NULL, NULL);

  gST->ConOut = &mAsciiTextOutputProtocol;
  gST->Hdr.CRC32 = 0;

  gBS->CalculateCrc32 (
    gST,
    gST->Hdr.HeaderSize,
    &gST->Hdr.CRC32
    );
}
