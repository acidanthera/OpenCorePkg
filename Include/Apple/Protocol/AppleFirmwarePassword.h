/** @file
  Apple Firmware Password protocol.

  Copyright (c) 2017, mhaeuser. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_FIRMWARE_PASSWORD_H
#define APPLE_FIRMWARE_PASSWORD_H

/**
  Apple Firmware Password protocol GUID.
  8FFEEB3A-4C98-4630-803F-740F9567091D
**/
#define APPLE_FIRMWARE_PASSWORD_PROTOCOL_GUID  \
  { 0x8FFEEB3A, 0x4C98, 0x4630,     \
     { 0x80, 0x3F, 0x74, 0x0F, 0x95, 0x67, 0x09, 0x1D }}

/**
  Apple Firmware Password protocol signature FWPASSWD.
**/
#define APPLE_FIRMWARE_PASSWORD_PROTOCOL_SIGNATURE  0x4457535341505746ULL

/**
  Apple Firmware Password protocol revision.
**/
#define APPLE_FIRMWARE_PASSWORD_PROTOCOL_REVISION  1

typedef struct APPLE_FIRMWARE_PASSWORD_PROTOCOL_ APPLE_FIRMWARE_PASSWORD_PROTOCOL;

/**
  Apple Firmware Password mode info.
**/
typedef struct {
  ///
  /// If password is on - 1, if off - 0.
  ///
  UINT8    Mode;
  ///
  /// Always 0 and changes only if unknown protocol is present in system.
  /// The producer of this unknown protocol has not been found, its only
  /// consumer is FirmwarePasswordDxe.
  ///
  UINT8    Flags;
} APPLE_FIRMWARE_PASSWORD_MODE_INFO;

/**
  Check whether the system has a firmware password using TamperResistantBoot.

  @param[in] This                A pointer to the protocol instance.

  @retval EFI_INVALID_PARAMETER  This is NULL or has a wrong signature.
  @retval EFI_SUCCESS            Password is set.
  @retval EFI_NOT_FOUND          There is no password set in the system.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_FIRMWARE_PASSWORD_IS_SET)(
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This
  );

/**
  Check that the entered password is correct using TamperResistantBoot.

  @param[in] This                A pointer to the protocol instance.
  @param[in] Password            Password to check.

  @retval EFI_ACCESS_DENIED      Entered password is incorrect.
  @retval EFI_INVALID_PARAMETER  This is NULL or has a wrong signature.
  @retval EFI_NOT_FOUND          There is no password set in the system.
  @retval EFI_SUCCESS            Entered password is correct.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_FIRMWARE_PASSWORD_CHECK)(
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This,
  IN     CONST CHAR16                      *Password
  );

/**
  Get the firmware password mode and an additional flag.

  @param[in]  This               A pointer to the protocol instance.
  @param[out] Info               Pointer to APPLE_FIRMWARE_PASSWORD_MODE_INFO.

  @retval EFI_INVALID_PARAMETER  This is NULL or has a wrong signature.
  @retval EFI_SUCCESS            Info was populated.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_FIRMWARE_PASSWORD_GET_MODE)(
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL   *This,
  OUT    APPLE_FIRMWARE_PASSWORD_MODE_INFO  *Info
  );

/**
  Apple Firmware Password protocol instance.
**/
struct APPLE_FIRMWARE_PASSWORD_PROTOCOL_ {
  UINT64                              Signature;
  UINT64                              Revision;
  APPLE_FIRMWARE_PASSWORD_IS_SET      IsSet;
  APPLE_FIRMWARE_PASSWORD_CHECK       Check;
  APPLE_FIRMWARE_PASSWORD_GET_MODE    GetMode;
};

extern EFI_GUID  gAppleFirmwarePasswordProtocolGuid;

#endif // APPLE_FIRMWARE_PASSWORD_H
