/** @file
  Apple System Info protocol.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_SYSTEM_INFO_H
#define APPLE_SYSTEM_INFO_H

/**
  Apple System Info protocol GUID.
  This protocol is present on Gibraltar Macs (ones with T1/T2).
  82ED9A9E-CCBB-4CD2-8A94-F4E3559AF911
**/
#define APPLE_SYSTEM_INFO_PROTOCOL_GUID \
  { 0x82ED9A9E, 0xCCBB, 0x4CD2          \
    { 0x8A, 0x94, 0xF4, 0xE3, 0x55, 0x9A, 0xF9, 0x11 } }

typedef struct APPLE_SYSTEM_INFO_PROTOCOL_ APPLE_SYSTEM_INFO_PROTOCOL;

/**
  Check whether platform is Gibraltar or not.

  @retval TRUE on Gibraltar platform (T2 machine).
  @retval FALSE on legacy platform.
**/
typedef
BOOLEAN
(EFIAPI *APPLE_SYSTEM_INFO_IS_GIBRALTAR) (
  VOID
  );

struct APPLE_SYSTEM_INFO_PROTOCOL_ {
  UINTN                                         Revision;
  APPLE_SYSTEM_INFO_IS_GIBRALTAR                IsGibraltar;
};

extern EFI_GUID gAppleSystemInfoProtocolGuid;

#endif // APPLE_SYSTEM_INFO_H
