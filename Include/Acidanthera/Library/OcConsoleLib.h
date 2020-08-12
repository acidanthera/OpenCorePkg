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

#include <Protocol/ConsoleControl.h>
#include <Protocol/AppleFramebufferInfo.h>

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
#define OC_CONSOLE_MARK_CONTROLLED       L"MarkControlled"
#define OC_CONSOLE_MARK_UNCONTROLLED     L"MarkUncontrolled"

/**
  Configure console control protocol with given options.

  @param[in] Renderer                 Renderer to use.
  @param[in] IgnoreTextOutput         Skip console output in text mode.
  @param[in] SanitiseClearScreen      Workaround ClearScreen breaking resolution.
  @param[in] ClearScreenOnModeSwitch  Clear graphic screen when switching to text mode.
  @param[in] ReplaceTabWithSpace      Replace invisible tab characters with spaces in OutputString.
**/
VOID
OcSetupConsole (
  IN OC_CONSOLE_RENDERER   Renderer,
  IN BOOLEAN               IgnoreTextOutput,
  IN BOOLEAN               SanitiseClearScreen,
  IN BOOLEAN               ClearScreenOnModeSwitch,
  IN BOOLEAN               ReplaceTabWithSpace
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
  Parse screen resolution from string.

  @param[in]   String   Resolution in WxH@B or WxH format.
  @param[out]  Width    Parsed resolution width or 0.
  @param[out]  Height   Parsed resolution height or 0.
  @param[out]  Bpp      Parsed resolution bpp or 0.
  @param[out]  Max      Set to TRUE when String equals to Max.
**/
VOID
OcParseScreenResolution (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT UINT32              *Bpp,
  OUT BOOLEAN             *Max
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
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT BOOLEAN             *Max
  );

/**
  Set screen resolution on console handle.

  @param[in]  Width      Resolution width or 0 for Max.
  @param[in]  Height     Resolution height or 0 for Max.
  @param[in]  Bpp        Resolution bpp or 0 for automatic.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetConsoleResolution (
  IN  UINT32              Width,
  IN  UINT32              Height,
  IN  UINT32              Bpp    OPTIONAL
  );

/**
  Set console mode.

  @param[in]  Width     Resolution width or 0 for Max.
  @param[in]  Height    Resolution height or 0 for Max.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSetConsoleMode (
  IN  UINT32              Width,
  IN  UINT32              Height
  );

/**
  Ensure installed GOP protocol on ConOut handle.
**/
VOID
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

#endif // OC_CONSOLE_LIB_H
