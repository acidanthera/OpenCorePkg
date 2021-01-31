/** @file

OcAppleImageVerificationLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DXE_IMAGE_VERIFICATION_H
#define APPLE_DXE_IMAGE_VERIFICATION_H

#include <Library/OcPeCoffLib.h>

#define APPLE_SIGNATURE_SECENTRY_SIZE 8

//
// Signature context
//
typedef struct APPLE_SIGNATURE_CONTEXT_ {
  UINT8                            PublicKey[256];
  UINT8                            PublicKeyHash[32];
  UINT8                            Signature[256];
} APPLE_SIGNATURE_CONTEXT;

EFI_STATUS
VerifyApplePeImageSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINT32                              *ImageSize
  );

#endif //APPLE_DXE_IMAGE_VERIFICATION_H
