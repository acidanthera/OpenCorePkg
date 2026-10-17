/** @file
  AppleEg2Info protocol.

  Copyright (c) 2020, vit9696. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_EG2_INFO_H
#define APPLE_EG2_INFO_H

///
/// The GUID of the APPLE_EG2_INFO_PROTOCOL_GUID.
/// 63FAECF2-E7EE-4CB9-8A0C-11CE5E89E33C
///
#define APPLE_EG2_INFO_PROTOCOL_GUID          \
  { 0x63FAECF2, 0xE7EE, 0x4CB9,               \
    { 0x8A, 0x0C, 0x11, 0xCE, 0x5E, 0x89, 0xE3, 0x3C } }

#define APPLE_EG2_INFO_PROTOCOL_REVISION  0x10003

typedef struct APPLE_EG2_INFO_PROTOCOL_ APPLE_EG2_INFO_PROTOCOL;

enum {
  AppleDisplayRotate0   = 0,
  AppleDisplayRotate90  = 1,
  AppleDisplayRotate180 = 2,
  AppleDisplayRotate270 = 3,
  AppleDisplayRotateMax = 4
};

/**
  Returns EFI_UNSUPPORTED for Apple.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_EG2_INFO_UNKNOWN1)(
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  IN   EFI_HANDLE                   GpuHandle
  );

/**
  Used for e.g. ATY,PlatformInfo.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_EG2_INFO_GET_PLATFORM_INFO)(
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  IN   EFI_HANDLE                   GpuHandle,
  OUT  VOID                         *Data,
  OUT  UINTN                        *Size
  );

/**
  Seems to enable display over a specified connector.
  Can return EFI_NOT_FOUND or EFI_UNSUPPORTED.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_EG2_INFO_STARTUP_DISPLAY)(
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  IN   EFI_HANDLE                   GpuHandle,
  OUT  VOID                         *Unk1,
  OUT  VOID                         *Unk2,
  OUT  VOID                         *Unk3
  );

/**
  Used for e.g. EFI,hibwake.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_EG2_INFO_GET_HIBERNATION)(
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  OUT  BOOLEAN                      *Hibernated
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_EG2_INFO_GET_ROTATION)(
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  OUT  UINT32                       *Rotation
  );

/**
  Disable display polling timer.
  Note: Available starting from revision 0x10003.

  @param[in]  This               A pointer to the protocol instance.
  @param[in]  Disable            If TRUE display polling timer will be disabled.

  @retval EFI_NOT_FOUND          No display in the system.
  @retval EFI_SUCCESS            Display polling timer enabled/disabled.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_EG2_INFO_DISABLE_DISPLAY_POLLING)(
  IN APPLE_EG2_INFO_PROTOCOL *This,
  IN BOOLEAN                 Disable
  );

struct APPLE_EG2_INFO_PROTOCOL_ {
  UINT32                                    Revision;
  VOID                                      *Unused;
  APPLE_EG2_INFO_UNKNOWN1                   Unknown1;
  APPLE_EG2_INFO_GET_PLATFORM_INFO          GetPlatformInfo;
  APPLE_EG2_INFO_STARTUP_DISPLAY            StartupDisplay;
  APPLE_EG2_INFO_GET_HIBERNATION            GetHibernation;
  APPLE_EG2_INFO_GET_ROTATION               GetRotation;
  APPLE_EG2_INFO_DISABLE_DISPLAY_POLLING    DisableDisplayPolling;
};

extern EFI_GUID  gAppleEg2InfoProtocolGuid;

#endif // APPLE_EG2_INFO_H
