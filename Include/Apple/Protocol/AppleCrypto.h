/** @file
  Apple Crypto protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_CRYPTO_H
#define APPLE_CRYPTO_H

/**
  Apple Crypto protocol GUID.
  9D7B16E6-3473-4035-BA63-7D6A3BA39498
**/
#define APPLE_CRYPTO_PROTOCOL_GUID  \
  { 0x9D7B16E6, 0x3473, 0x4035,     \
     { 0xBA, 0x63, 0x7D, 0x6A, 0x3B, 0xA3, 0x94, 0x98 }}

typedef struct APPLE_CRYPTO_PROTOCOL_ APPLE_CRYPTO_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_CRYPTO_VERIFY_SIGNATURE)(
  IN APPLE_CRYPTO_PROTOCOL  *This,
  IN CONST UINT8            *Hash,
  IN UINTN                  HashSize,
  IN CONST UINT8            *Signature,
  IN CONST EFI_GUID         *CertType
  );

/**
  Apple Crypto protocol instance.
**/
struct APPLE_CRYPTO_PROTOCOL_ {
  APPLE_CRYPTO_VERIFY_SIGNATURE    VerifySignature;
};

extern EFI_GUID  gAppleCryptoProtocolGuid;

#endif // APPLE_CRYPTO_H
