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

#ifndef CONSOLE_GOP_INTERNAL_H
#define CONSOLE_GOP_INTERNAL_H

#include <Protocol/GraphicsOutput.h>
#include <Library/OcBlitLib.h>

typedef struct CONSOLE_GOP_CONTEXT {
  ///
  /// Original EFI_BOOT_SERVICES HandleProtocol to hook and override protocols.
  ///
  EFI_HANDLE_PROTOCOL OriginalHandleProtocol;
  ///
  /// Graphics output protocol specific to console.
  ///
  EFI_GRAPHICS_OUTPUT_PROTOCOL *ConsoleGop;
  ///
  /// Direct framebuffer context.
  ///
  OC_BLIT_CONFIGURE  *FramebufferContext;
  ///
  /// Direct framebuffer context size in pages.
  ///
  UINTN              FramebufferContextPageCount;
  ///
  /// Framebuffer memory cache policy.
  ///
  INT32              CachePolicy;
  ///
  /// Framebuffer rotation angle.
  ///
  UINT32             Rotation;
  ///
  /// Original EFI_GRAPHICS_OUTPUT_PROTOCOL SetMode function.
  ///
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   OriginalGopSetMode;
  ///
  /// Original EFI_GRAPHICS_OUTPUT_PROTOCOL SetMode function.
  ///
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE OriginalGopQueryMode;
  ///
  /// Original (not rotated) mode information.
  ///
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    OriginalModeInfo;
  ///
  /// Original (not rotated) linear frame buffer.
  ///
  EFI_PHYSICAL_ADDRESS                    OriginalFrameBufferBase;
  ///
  /// Original (not rotated) linear frame buffer size.
  ///
  UINTN                                   OriginalFrameBufferSize;
  ///
  /// Ours (rotated) mode informaation.
  ///
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    CustomModeInfo;
} CONSOLE_GOP_CONTEXT;

CONST CONSOLE_GOP_CONTEXT *
InternalGetDirectGopContext (
  VOID
  );

#endif // CONSOLE_GOP_INTERNAL_H
