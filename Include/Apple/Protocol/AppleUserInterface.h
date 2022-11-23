/** @file
  Apple User Interface Protocol.

  Copyright (C) 2022, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_H
#define APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_H

#include <Protocol/GraphicsOutput.h>

/**
  Apple User Interface protocol GUID.
**/
#define APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_GUID \
  { 0x691E1AF0, 0x8673, 0x4C98,           \
    { 0xA9, 0xB3, 0x04, 0x26, 0x20, 0xE9, 0x14, 0x98 }}

// Version in MacPro5,1 144.0.0.0.0 firmware
#define APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_REVISION  0x00000004

typedef struct APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_ APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL;

/**
  Connect user interface protocol to current GOP on gST->ConsoleOutHandle.

  @retval EFI_SUCCESS           GOP interface on required handle was found and stored.
**/
typedef
EFI_STATUS
(EFIAPI *USER_INTERFACE_CONNECT_GOP)(
  VOID
  );

/**
  On version 4 of the protocol, always returns EFI_SUCCESS.

  @retval EFI_SUCCESS           Success.
**/
typedef
EFI_STATUS
(EFIAPI *USER_INTERFACE_RETURN_SUCCESS)(
  VOID
  );

/**
  Allocate draw buffer and associated info buffers, and return containing info block.

  @param[out] DrawBufferInfo    Draw buffer info.
  @param[in]  BackgroundColor   Background color.
**/
typedef
EFI_STATUS
(EFIAPI *USER_INTERFACE_CREATE_DRAW_BUFFER)(
  OUT VOID    *DrawBufferInfo,
  IN UINT32   BackgroundColor
  );

/**
  Free draw buffer info block and associated allocations.

  @param[in]  DrawBufferInfo    Draw buffer info.
**/
typedef
VOID
(EFIAPI *USER_INTERFACE_FREE_DRAW_BUFFER)(
  IN VOID     *DrawBufferInfo
  );

/**
  If Color is not NULL, combine specified RGBA components into Color.

  @param[out] Color  Combined color.
  @param[in]  Red    Color red value.
  @param[in]  Green  Color green value.
  @param[in]  Blue   Color blue value.
  @param[in]  Alpha  Color alpha value.
**/
typedef
VOID
(EFIAPI *USER_INTERFACE_COMBINE_RGBA)(
  OUT UINT32 *Colour OPTIONAL,
  IN UINT8 Red,
  IN UINT8 Green,
  IN UINT8 Blue,
  IN UINT8 Alpha
  );

/**
  Method not yet mapped.
**/
typedef
VOID
(EFIAPI *USER_INTERFACE_UNMAPPED)(
  VOID
  );

struct APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_ {
  UINTN                                Revision;
  USER_INTERFACE_CONNECT_GOP           ConnectGop;
  USER_INTERFACE_RETURN_SUCCESS        ReturnSuccess;
  USER_INTERFACE_UNMAPPED              _Method_18h;
  USER_INTERFACE_UNMAPPED              _Method_20h;
  USER_INTERFACE_UNMAPPED              _Method_28h;
  USER_INTERFACE_UNMAPPED              _Method_30h;
  USER_INTERFACE_UNMAPPED              _Method_38h;
  USER_INTERFACE_UNMAPPED              _Method_40h;
  USER_INTERFACE_UNMAPPED              _Method_48h;
  USER_INTERFACE_UNMAPPED              _Method_50h;
  USER_INTERFACE_UNMAPPED              _Method_58h;
  USER_INTERFACE_UNMAPPED              _Method_60h;
  USER_INTERFACE_UNMAPPED              _Method_68h;
  USER_INTERFACE_UNMAPPED              _Method_70h;
  USER_INTERFACE_UNMAPPED              _Method_78h;
  USER_INTERFACE_UNMAPPED              _Method_80h;
  USER_INTERFACE_UNMAPPED              _Method_88h;
  USER_INTERFACE_UNMAPPED              _Method_90h;
  USER_INTERFACE_UNMAPPED              _Method_98h;
  USER_INTERFACE_UNMAPPED              _Method_A0h;
  USER_INTERFACE_UNMAPPED              _Method_A8h;
  USER_INTERFACE_UNMAPPED              _Method_B0h;
  USER_INTERFACE_UNMAPPED              _Method_B8h;
  USER_INTERFACE_UNMAPPED              _Method_C0h;
  USER_INTERFACE_UNMAPPED              _Method_C8h;
  USER_INTERFACE_UNMAPPED              _Method_D0h;
  USER_INTERFACE_UNMAPPED              _Method_D8h;
  USER_INTERFACE_UNMAPPED              _Method_E0h;
  USER_INTERFACE_UNMAPPED              _Method_E8h;
  USER_INTERFACE_UNMAPPED              _Method_F0h;
  USER_INTERFACE_UNMAPPED              _Method_F8h;
  USER_INTERFACE_UNMAPPED              _Method_100h;
  USER_INTERFACE_UNMAPPED              _Method_108h;
  USER_INTERFACE_UNMAPPED              _Method_110h;
  USER_INTERFACE_UNMAPPED              _Method_118h;
  USER_INTERFACE_UNMAPPED              _Method_120h;
  USER_INTERFACE_CREATE_DRAW_BUFFER    CreateDrawBuffer;
  USER_INTERFACE_FREE_DRAW_BUFFER      FreeDrawBuffer;
  USER_INTERFACE_UNMAPPED              _Method_138h;
  USER_INTERFACE_UNMAPPED              _Method_140h;
  USER_INTERFACE_UNMAPPED              _Method_148h;
  USER_INTERFACE_UNMAPPED              _Method_150h;
  USER_INTERFACE_UNMAPPED              _Method_158h;
  USER_INTERFACE_UNMAPPED              _Method_160h;
  USER_INTERFACE_UNMAPPED              _Method_168h;
  USER_INTERFACE_UNMAPPED              _Method_170h;
  USER_INTERFACE_UNMAPPED              _Method_178h;
  USER_INTERFACE_UNMAPPED              _Method_180h;
  USER_INTERFACE_UNMAPPED              _Method_188h;
  USER_INTERFACE_UNMAPPED              _Method_190h;
  USER_INTERFACE_UNMAPPED              _Method_198h;
  USER_INTERFACE_UNMAPPED              _Method_1A0h;
  USER_INTERFACE_UNMAPPED              _Method_1A8h;
  USER_INTERFACE_UNMAPPED              _Method_1B0h;
  USER_INTERFACE_UNMAPPED              _Method_1B8h;
  USER_INTERFACE_UNMAPPED              _Method_1C0h;
  USER_INTERFACE_UNMAPPED              _Method_1C8h;
  USER_INTERFACE_UNMAPPED              _Method_1D0h;
  USER_INTERFACE_UNMAPPED              _Method_1D8h;
  USER_INTERFACE_UNMAPPED              _Method_1E0h;
  USER_INTERFACE_UNMAPPED              _Method_1E8h;
  USER_INTERFACE_UNMAPPED              _Method_1F0h;
  USER_INTERFACE_UNMAPPED              _Method_1F8h;
  USER_INTERFACE_UNMAPPED              _Method_200h;
  USER_INTERFACE_UNMAPPED              _Method_208h;
  USER_INTERFACE_UNMAPPED              _Method_210h;
  USER_INTERFACE_UNMAPPED              _Method_218h;
  USER_INTERFACE_UNMAPPED              _Method_220h;
  USER_INTERFACE_UNMAPPED              _Method_228h;
  USER_INTERFACE_UNMAPPED              _Method_230h;
  USER_INTERFACE_UNMAPPED              _Method_238h;
  USER_INTERFACE_UNMAPPED              _Method_240h;
  USER_INTERFACE_UNMAPPED              _Method_248h;
  USER_INTERFACE_UNMAPPED              _Method_250h;
  USER_INTERFACE_UNMAPPED              _Method_258h;
  USER_INTERFACE_UNMAPPED              _Method_260h;
  USER_INTERFACE_UNMAPPED              _Method_268h;
  USER_INTERFACE_UNMAPPED              _Method_270h;
  USER_INTERFACE_UNMAPPED              _Method_278h;
  USER_INTERFACE_UNMAPPED              _Method_280h;
  USER_INTERFACE_UNMAPPED              _Method_288h;
  USER_INTERFACE_UNMAPPED              _Method_290h;
  USER_INTERFACE_UNMAPPED              _Method_298h;
  USER_INTERFACE_UNMAPPED              _Method_2A0h;
  USER_INTERFACE_UNMAPPED              _Method_2A8h;
  USER_INTERFACE_UNMAPPED              _Method_2B0h;
  USER_INTERFACE_UNMAPPED              _Method_2B8h;
  USER_INTERFACE_UNMAPPED              _Method_2C0h;
  USER_INTERFACE_UNMAPPED              _Method_2C8h;
  USER_INTERFACE_UNMAPPED              _Method_2D0h;
  USER_INTERFACE_UNMAPPED              _Method_2D8h;
  USER_INTERFACE_UNMAPPED              _Method_2E0h;
  USER_INTERFACE_UNMAPPED              _Method_2E8h;
  USER_INTERFACE_UNMAPPED              _Method_2F0h;
  USER_INTERFACE_UNMAPPED              _Method_2F8h;
  USER_INTERFACE_COMBINE_RGBA          CombineRgba; ///< Check argument order
  USER_INTERFACE_UNMAPPED              _Method_308h;
  USER_INTERFACE_UNMAPPED              _Method_310h;
  USER_INTERFACE_UNMAPPED              _Method_318h;
  USER_INTERFACE_UNMAPPED              _Method_320h;
  USER_INTERFACE_UNMAPPED              _Method_328h;
  USER_INTERFACE_UNMAPPED              _Method_330h;
  USER_INTERFACE_UNMAPPED              _Method_338h;
  USER_INTERFACE_UNMAPPED              _Method_340h;
  USER_INTERFACE_UNMAPPED              _Method_348h;
  USER_INTERFACE_UNMAPPED              _Method_350h;
  USER_INTERFACE_UNMAPPED              _Method_358h;
  USER_INTERFACE_UNMAPPED              _Method_360h;
};

extern EFI_GUID  gAppleFirmwareUserInterfaceProtocolGuid;

#endif // APPLE_FIRMWARE_USER_INTERFACE_PROTOCOL_H
