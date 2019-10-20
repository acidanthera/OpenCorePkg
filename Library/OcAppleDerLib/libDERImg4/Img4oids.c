/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Img4oids.h"

#define OID_ISO_MEMBER  42                     
#define OID_US          OID_ISO_MEMBER, 134, 72
#define OID_RSA         OID_US, 134, 247, 13
#define OID_PKCS        OID_RSA, 1
#define OID_PKCS_1      OID_PKCS, 1
#define APPLE_OID       OID_US, 0x86, 0xf7, 0x63
#define APPLE_ADS_OID   APPLE_OID, 0x64
#define APPLE_CERT_EXT  APPLE_ADS_OID, 6

#define APPLE_CERT_EXT_CODESIGN            APPLE_CERT_EXT, 1
#define APPLE_IMG4_MANIFEST_CERT_SPEC_OID  APPLE_CERT_EXT_CODESIGN, 15

static const DERByte _oidAppleImg4ManifestCertSpec[] = {
  APPLE_IMG4_MANIFEST_CERT_SPEC_OID
};

const DERItem oidAppleImg4ManifestCertSpec = {
  (DERByte *)_oidAppleImg4ManifestCertSpec,
  sizeof (_oidAppleImg4ManifestCertSpec)
};

static const DERByte _oidSha384Rsa[] = { OID_PKCS_1, 12 };
const DERItem
  oidSha384Rsa = { (DERByte *)_oidSha384Rsa, sizeof (_oidSha384Rsa) };

static const DERByte _oidSha512Rsa[] = { OID_PKCS_1, 13 };
const DERItem
  oidSha512Rsa = { (DERByte *)_oidSha512Rsa, sizeof (_oidSha512Rsa) };
