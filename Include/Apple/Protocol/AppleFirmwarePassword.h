/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FIRMWARE_PASSWORD_PROTOCOL_H
#define APPLE_FIRMWARE_PASSWORD_PROTOCOL_H

#define APPLE_FIRMWARE_PASSWORD_PROTOCOL_GUID \
  { \
  0x8FFEEB3A, 0x4C98, 0x4630, { 0x80, 0x3F, 0x74, 0x0F, 0x95, 0x67, 0x09, 0x1D } \
  }

// APPLE_FIRMWARE_PASSWORD_PROTOCOL
/// The forward declaration for the protocol for the APPLE_FIRMWARE_PASSWORD_PROTOCOL.

typedef struct APPLE_FIRMWARE_PASSWORD_PROTOCOL_ APPLE_FIRMWARE_PASSWORD_PROTOCOL;

// AppleFirmwarePasswordCheck
///
///
/// @param[in]     This         This protocol.
/// @param[in,out] Arg1
///
/// @retval EFI_SUCCESS         The log was saved successfully.
/// @retval other

typedef
EFI_STATUS
(EFIAPI *APPLE_FIRMWARE_PASSWORD_CHECK) (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This,
  IN OUT UINTN                             *Arg1
  );

// _APPLE_FIRMWARE_PASSWORD_PROTOCOL
/// The structure exposed by the APPLE_FIRMWARE_PASSWORD_PROTOCOL.

struct APPLE_FIRMWARE_PASSWORD_PROTOCOL_ {
  UINTN Revision;
  UINTN Unknown01;
  UINTN Unknown02;
  UINTN Unknown03;
  APPLE_FIRMWARE_PASSWORD_CHECK Check;
};

//
// gAppleFirmwarePasswordProtocolGuid
/// A global variable storing the GUID of the _APPLE_FIRMWARE_PASSWORD_PROTOCOL.

extern EFI_GUID gAppleFirmwarePasswordProtocolGuid;

#endif /* APPLE_FIRMWARE_PASSWORD_PROTOCOL_H_ */

