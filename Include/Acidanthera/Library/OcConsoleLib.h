/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_CONSOLE_LIB_H
#define OC_CONSOLE_LIB_H

#include <Protocol/AppleFramebufferInfo.h>
#include <Protocol/AppleEg2Info.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/OcBootManagementLib.h>
#include <Library/OcFileLib.h>

/**
  Console renderer to use.
**/
typedef enum {
  OcConsoleRendererBuiltinGraphics,
  OcConsoleRendererBuiltinText,
  OcConsoleRendererSystemGraphics,
  OcConsoleRendererSystemText,
  OcConsoleRendererSystemGeneric
} OC_CONSOLE_RENDERER;

/**
  Special commands sent to Builtin text renderer through TestString.
**/
/**
  Extension to notify OpenCore builtin renderer that any text it may have produced
  on screen is mixed with graphics which it did not control.
**/
#define OC_CONSOLE_MARK_UNCONTROLLED  L"MarkUncontrolled"

/**
  Console font.
 **/
#define ISO_CHAR_WIDTH   (8u)
#define ISO_CHAR_HEIGHT  (16u)

#define OC_CONSOLE_FONT_FALLBACK_CHAR  (L'_')

typedef struct _OC_CONSOLE_FONT_PAGE {
  UINT8      *Glyphs;
  UINT8      *GlyphOffsets;
  UINT8      CharMin;
  UINT8      CharMax;
  UINT8      FontHead : 4;
  UINT8      FontTail : 4;
  BOOLEAN    LeftToRight;
} OC_CONSOLE_FONT_PAGE;

typedef struct _OC_CONSOLE_FONT {
  OC_CONSOLE_FONT_PAGE    *Pages;
  UINT16                  *PageOffsets;
  UINT16                  PageMin;
  UINT16                  PageMax;
} OC_CONSOLE_FONT;

typedef struct _OC_CONSOLE_FONT_RANGE {
  CHAR16    Start;
  UINT16    Length;
} OC_CONSOLE_FONT_RANGE;

/**
  Free font used by XNU, plus unicode box drawing chars.
**/
extern OC_CONSOLE_FONT  gDefaultConsoleFont;

/**
  List of non-page 0 chars required by EFI.
  Refs:
    https://github.com/acidanthera/audk/blob/master/MdePkg/Include/Protocol/SimpleTextOut.h#L177-L178
    https://github.com/acidanthera/audk/blob/master/MdePkg/Include/Protocol/SimpleTextOut.h#L34-L98
**/
extern OC_CONSOLE_FONT_RANGE  gEfiRequiredUnicodeChars[];

/**
  List of all chars present in Extended Unicode range.
  Ref: https://int10h.org/oldschool-pc-fonts/fontlist/font?ibm_vga_8x16
**/
extern OC_CONSOLE_FONT_RANGE  gExtendedUnicodeChars[];

/**
  Configure console control protocol with given options.

  @param[in] InitialMode              Initial mode to use, or set max. value to use existing mode.
  @param[in] Renderer                 Renderer to use.
  @param[in] Storage                  Storage context - only required if Font parameter is used.
  @param[in] Font                     Font file to load. Use builtin font if NULL or empty string.
  @param[in] IgnoreTextOutput         Skip console output in text mode.
  @param[in] SanitiseClearScreen      Workaround ClearScreen breaking resolution.
  @param[in] ClearScreenOnModeSwitch  Clear graphic screen when switching to text mode.
  @param[in] ReplaceTabWithSpace      Replace invisible tab characters with spaces in OutputString.
  @param[in] Width                    Width to set - applies to builtin renderer only.
  @param[in] Height                   Height to set - applies to builtin renderer only.
**/
VOID
OcSetupConsole (
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  InitialMode,
  IN OC_CONSOLE_RENDERER              Renderer,
  IN OC_STORAGE_CONTEXT               *Storage  OPTIONAL,
  IN CONST CHAR8                      *Font     OPTIONAL,
  IN BOOLEAN                          IgnoreTextOutput,
  IN BOOLEAN                          SanitiseClearScreen,
  IN BOOLEAN                          ClearScreenOnModeSwitch,
  IN BOOLEAN                          ReplaceTabWithSpace,
  IN UINT32                           Width,
  IN UINT32                           Height
  );

/**
  Update console control screen mode.

  @param[in] Mode       Desired mode.

  @retval previous console control mode.
**/
EFI_CONSOLE_CONTROL_SCREEN_MODE
OcConsoleControlSetMode (
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  );

/**
  Get existing console control screen mode, default to text if no existing console control protocol.

  @retval existing console control mode.
**/
EFI_CONSOLE_CONTROL_SCREEN_MODE
OcConsoleControlGetMode (
  VOID
  );

/**
  Parse screen resolution from string.

  @param[in]   String   Resolution in WxH@B or WxH format.
  @param[out]  Width    Parsed resolution width or 0.
  @param[out]  Height   Parsed resolution height or 0.
  @param[out]  Bpp      Parsed resolution bpp or 0.
  @param[out]  Max      Set to TRUE when String equals to Max.
**/
VOID
OcParseScreenResolution (
  IN  CONST CHAR8  *String,
  OUT UINT32       *Width,
  OUT UINT32       *Height,
  OUT UINT32       *Bpp,
  OUT BOOLEAN      *Max
  );

/**
  Parse console mode from string.

  @param[in]   String   Resolution in WxH format.
  @param[out]  Width    Parsed mode width or 0.
  @param[out]  Height   Parsed mode height or 0.
  @param[out]  Max      Set to TRUE when String equals to Max.
**/
VOID
OcParseConsoleMode (
  IN  CONST CHAR8  *String,
  OUT UINT32       *Width,
  OUT UINT32       *Height,
  OUT BOOLEAN      *Max
  );

/**
  Use PAT to enable write-combining caching (burst mode) on GOP memory,
  when it is suppported but firmware has not set it up.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetGopBurstMode (
  VOID
  );

/**
  Return the bytes per pixel for the current GOP mode.

  GOP mode information does not include anything directly containing the bytes
  per pixel, but there is a defined algorithm for working out this size, even for
  non-standard pixel masks, and also a defined situation (PixelBltOnly pixel
  format) where there is no such size.

  @param[in]  Mode                GOP mode.
  @param[in]  BytesPerPixel       Bytes per pixel for the mode in use.

  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         There is no frame buffer.
  @retval EFI_INVALID_PARAMETER   Mode info parameters are outside valid ranges.
**/
EFI_STATUS
OcGopModeBytesPerPixel (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *Mode,
  OUT UINTN                              *BytesPerPixel
  );

/**
  Return the frame buffer size in use for the current GOP mode, even where
  Gop->Mode->FrameBufferSize misreports this.

  Occasional GOP implementations report a frame buffer size far larger (e.g. ~100x)
  than required for the actual mode in use.

  @param[in]  Mode                GOP mode.
  @param[in]  FrameBufferSize     Frame buffer size in use.

  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         There is no frame buffer.
  @retval EFI_INVALID_PARAMETER   Size parameters are outside valid ranges.
**/
EFI_STATUS
OcGopModeSafeFrameBufferSize (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *Mode,
  OUT UINTN                              *FrameBufferSize
  );

/**
  Set screen resolution on console handle.

  @param[in]  Width      Resolution width or 0 for Max.
  @param[in]  Height     Resolution height or 0 for Max.
  @param[in]  Bpp        Resolution bpp or 0 for automatic.
  @param[in]  Force      Force the specified resolution using
                         the OC_FORCE_RESOLUTION protocol.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetConsoleResolution (
  IN  UINT32   Width  OPTIONAL,
  IN  UINT32   Height OPTIONAL,
  IN  UINT32   Bpp    OPTIONAL,
  IN  BOOLEAN  Force
  );

/**
  Set console mode.

  @param[in]  Width     Resolution width or 0 for Max.
  @param[in]  Height    Resolution height or 0 for Max.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetConsoleMode (
  IN  UINT32  Width,
  IN  UINT32  Height
  );

/**
  Ensure installed GOP protocol on ConOut handle.
**/
EFI_STATUS
OcProvideConsoleGop (
  IN BOOLEAN  Route
  );

/**
  Perform console reconnection.
**/
VOID
OcReconnectConsole (
  VOID
  );

/**
  Use direct GOP renderer for console.

  @param[in] CacheType   Caching type, e.g. CacheWriteCombining or -1 to disable.
  @param[in] Rotation    Rotation scheme in degrees (must be one of 0, 90, 180, 270).

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcUseDirectGop (
  IN INT32  CacheType
  );

/**
  Allocate new System Table with disabled text output.

  @param[in] SystemTable     Base System Table.

  @retval non NULL  The System Table table was allocated successfully.
**/
EFI_SYSTEM_TABLE *
AllocateNullTextOutSystemTable (
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Provide UGA protocol instances on top of existing GOP instances.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcProvideUgaPassThrough (
  VOID
  );

/**
  Provide GOP protocol instances on top of existing UGA instances.

  @param[in]  ForAll  For all instances, otherwises for AppleFramebuffer-enabled only.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcProvideGopPassThrough (
  IN BOOLEAN  ForAll
  );

/**
  Install and initialise Apple Framebuffer Info protocol
  on top of GOP protocol. For EfiBoot 10.4, which can only
  use UGA, this is the only way to obtain framebuffer base
  for XNU kernel PE Boot_args.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_FRAMEBUFFER_INFO_PROTOCOL *
OcAppleFbInfoInstallProtocol (
  IN BOOLEAN  Reinstall
  );

/**
  Install and initialise Apple EG2 Info protocol
  on top of GOP protocol. For newer EfiBoot this is the way
  to obtain screen rotation angle.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_EG2_INFO_PROTOCOL *
OcAppleEg2InfoInstallProtocol (
  IN BOOLEAN  Reinstall
  );

/**
  Dump GOP info to the specified directory.

  @param[in]  Root     Directory to write CPU data.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcGopInfoDump (
  IN EFI_FILE_PROTOCOL  *Root
  );

#endif // OC_CONSOLE_LIB_H
