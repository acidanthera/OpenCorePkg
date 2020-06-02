/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef LIB_DER_IMG4_CONFIG_H
#define LIB_DER_IMG4_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DER_IMG4_MAX_DIGEST_SIZE  64

extern const UINT8 *DERImg4RootCertificate;
extern const UINTN  *DERImg4RootCertificateSize;

bool
DERImg4VerifySignature (
  DERByte        *Modulus,
  DERSize        ModulusSize,
  uint32_t       Exponent,
  const uint8_t  *Signature,
  size_t         SignatureSize,
  uint8_t        *Data,
  size_t         DataSize,
  const DERItem  *AlgoOid
  );

/* ---------------------- Do not edit below this line ---------------------- */

#ifndef DER_IMG4_MAN_CERT_CHAIN_MAX
#define DER_IMG4_MAN_CERT_CHAIN_MAX  5
#endif

#ifndef DER_IMG4_MAX_DIGEST_SIZE
#define DER_IMG4_MAX_DIGEST_SIZE  48
#endif

#ifndef DER_IMG4_MAX_CERT_SIZE
#define DER_IMG4_MAX_CERT_SIZE  0x1FFFFU
#endif

#ifdef __cplusplus
}
#endif

#endif // LIB_DER_IMG4_CONFIG_H
