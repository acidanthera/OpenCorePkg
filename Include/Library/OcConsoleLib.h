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

/**
  Configure console control protocol with given options.

  @param[in] WrapExisting  Wrap existing protocol.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
ConfigureConsoleControl (
  IN BOOLEAN              WrapExisting
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
ParseScreenResolution (
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
ParseConsoleMode (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT BOOLEAN             *Max
  );

/**
  Set screen resolution on console handle.

  @param[in]  Width     Resolution width or 0 for Max.
  @param[in]  Height    Resolution height or 0 for Max.
  @param[in]  Bpp       Resolution bpp or 0 for automatic.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
SetConsoleResolution (
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
SetConsoleMode (
  IN  UINT32              Width,
  IN  UINT32              Height
  );

#endif // OC_CONSOLE_LIB_H
