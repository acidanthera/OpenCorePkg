/** @file

GUID for UEFI APPLE_CERTIFICATE structure

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_CERTIFICATE_H
#define APPLE_CERTIFICATE_H

#include <Guid/WinCertificate.h>

//
// Apple Certificate GUID
//
#define APPLE_EFI_CERTIFICATE_GUID \
  { 0x45E7BC51, 0x913C, 0x42AC,    \
    { 0x96, 0xA2, 0x10, 0x71, 0x2F, 0xFB, 0xEB, 0xA7 } }

//
// Must be set to CertType
//
#define APPLE_EFI_CERTIFICATE_TYPE  0x0EF1

//
// Apple Signature directory header
//
typedef struct APPLE_EFI_CERTIFICATE_INFO_ {
  UINT32    CertOffset;
  UINT32    CertSize;
} APPLE_EFI_CERTIFICATE_INFO;

//
// Certificate which encapsulates a GUID-specific digital signature
//
typedef struct APPLE_EFI_CERTIFICATE_ {
  UINT32                          CertSize;
  UINT16                          CompressionType;
  UINT16                          CertType;
  EFI_GUID                        AppleSignatureGuid;
  EFI_CERT_BLOCK_RSA_2048_SHA256  CertData;
} APPLE_EFI_CERTIFICATE;

extern EFI_GUID gAppleEfiCertificateGuid;

#endif // APPLE_CERTIFICATE_H
