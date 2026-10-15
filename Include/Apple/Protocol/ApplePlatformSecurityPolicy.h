/** @file
  Apple Platform Security Policy protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_PLATFORM_SECURITY_POLICY_H
#define APPLE_PLATFORM_SECURITY_POLICY_H

/**
  Apple Platform Security Policy protocol GUID.
  E4518E76-19D8-4475-9094-73BDABDC3B0C
**/
#define APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL_GUID \
  { 0xE4518E76, 0x19D8, 0x4475,              \
    { 0x90, 0x94, 0x73, 0xBD, 0xAB, 0xDC, 0x3B, 0x0C }}

typedef struct APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL_ APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_PLATFORM_SECURITY_POLICY_GET_POLICY)(
  IN  APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL *This,
  OUT UINT32                                  *Policy
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_PLATFORM_SECURITY_POLICY_GET_POLICY_DATA)(
  IN  APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL *This,
  IN     UINTN                                Index,
  IN     UINTN                                Unused,
  IN OUT UINTN                                *Size,
  OUT    VOID                                 *Buffer OPTIONAL
  );

/**
  Apple Platform Security Policy protocol instance.
**/
struct APPLE_PLATFORM_SECURITY_POLICY_PROTOCOL_ {
  APPLE_PLATFORM_SECURITY_POLICY_GET_POLICY         GetPolicy;
  APPLE_PLATFORM_SECURITY_POLICY_GET_POLICY_DATA    GetPolicyData;
};

extern EFI_GUID  gApplePlatformSecurityPolicyProtocolGuid;

#endif // APPLE_PLATFORM_SECURITY_POLICY_H
