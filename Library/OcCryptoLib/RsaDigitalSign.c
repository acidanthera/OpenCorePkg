/** @file
  This library performs RSA-based cryptography operations.

  SECURITY: Currently, no security measures have been taken. This code is
            vulnerable to both timing and side channel attacks for value
            leakage. However, its current purpose is the verification of public
            binaries with public certificates, for which this is perfectly
            acceptable, especially in regards to performance.

  Copyright (C) 2019, Download-Fritz. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BigNumLib.h"

//
// RFC 3447, 9.2 EMSA-PKCS1-v1_5, Notes 1.
//

#ifdef OC_CRYPTO_SUPPORTS_SHA256
STATIC CONST UINT8 mPkcsDigestEncodingPrefixSha256[] = {
  0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
  0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA384
STATIC CONST UINT8 mPkcsDigestEncodingPrefixSha384[] = {
  0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
  0x02, 0x02, 0x05, 0x00, 0x04, 0x30
};
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA512
STATIC CONST UINT8 mPkcsDigestEncodingPrefixSha512[] = {
  0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
  0x02, 0x03, 0x05, 0x00, 0x04, 0x40
};
#endif

/**
  Returns whether the RSA modulus size is allowed.

  @param[in]  ModulusSize  The size, in bytes, of the RSA modulus.

**/
STATIC
BOOLEAN
InternalRsaModulusSizeIsAllowed (
  IN UINTN  ModulusSize
  )
{
  //
  // Verify ModulusSize is a two's potency.
  //
  if ((ModulusSize & (ModulusSize - 1U)) != 0) {
    return FALSE;
  }

  return (PcdGet16 (PcdOcCryptoAllowedRsaModuli) & ModulusSize) != 0;
}

/**
  Returns whether the signature hashing algorithm is allowed.

  @param[in]  Type  The signature hashing algorithm type.

**/
STATIC
BOOLEAN
InternalSigHashTypeIsAllowed (
  IN OC_SIG_HASH_TYPE  Type
  )
{
  return (PcdGet16 (PcdOcCryptoAllowedSigHashTypes) & (1U << Type)) != 0;
}

INTN
SigVerifyShaHashBySize (
  IN CONST VOID   *Data,
  IN UINTN        DataSize,
  IN CONST UINT8  *Hash,
  IN UINTN        HashSize
  )
{
  OC_SIG_HASH_TYPE Hashtype;
  UINT8            DataDigest[OC_MAX_SHA_DIGEST_SIZE];

  ASSERT (Data != NULL);
  ASSERT (DataSize > 0);
  ASSERT (Hash != NULL);
  ASSERT (HashSize > 0);
  ASSERT (HashSize <= sizeof (DataDigest));

  switch (HashSize) {
#ifdef OC_CRYPTO_SUPPORTS_SHA512
    case SHA512_DIGEST_SIZE:
    {
      Hashtype = OcSigHashTypeSha512;
      Sha512 (DataDigest, Data, DataSize);
      break;
    }
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA384
    case SHA384_DIGEST_SIZE:
    {
      Hashtype = OcSigHashTypeSha384;
      Sha384 (DataDigest, Data, DataSize);
      break;
    }
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA256
    case SHA256_DIGEST_SIZE:
    {
      Hashtype = OcSigHashTypeSha256;
      Sha256 (DataDigest, Data, DataSize);
      break;
    }
#endif

    default:
    {
      return -1;
    }
  }

  if (!InternalSigHashTypeIsAllowed (Hashtype)) {
    return -1;
  }

  return CompareMem (DataDigest, Hash, HashSize);
}

/**
  Verify a RSA PKCS1.5 signature against an expected hash.

  @param[in] N              The RSA modulus.
  @param[in] N0Inv          The Montgomery Inverse of N.
  @param[in] RSqrMod        Montgomery's R^2 mod N.
  @param[in] NumWords       The number of Words of N and RSqrMod.
  @param[in] Exponent       The RSA exponent.
  @param[in] Signature      The RSA signature to be verified.
  @param[in] SignatureSize  Size, in bytes, of Signature.
  @param[in] Hash           The Hash digest of the signed data.
  @param[in] HashSize       Size, in bytes, of Hash.
  @param[in] Algorithm      The RSA algorithm used.
  @param[in] Scratch        Scratch buffer 3xModulo.

  @returns  Whether the signature has been successfully verified as valid.

**/
STATIC
BOOLEAN
RsaVerifySigHashFromProcessed (
  IN CONST OC_BN_WORD  *N,
  IN UINTN             NumWords,
  IN OC_BN_WORD        N0Inv,
  IN CONST OC_BN_WORD  *RSqrMod,
  IN UINT32            Exponent,
  IN CONST UINT8       *Signature,
  IN UINTN             SignatureSize,
  IN CONST UINT8       *Hash,
  IN UINTN             HashSize,
  IN OC_SIG_HASH_TYPE  Algorithm,
  IN OC_BN_WORD        *Scratch
  )
{
  BOOLEAN     Result;
  INTN        CmpResult;

  UINTN       ModulusSize;

  OC_BN_WORD  *EncryptedSigNum;
  OC_BN_WORD  *DecryptedSigNum;
  OC_BN_WORD  *PowScratchNum;

  CONST UINT8 *Padding;
  UINTN       PaddingSize;
  UINTN       DigestSize;
  UINTN       Index;

  OC_BN_WORD  Tmp;

  ASSERT (N != NULL);
  ASSERT (NumWords > 0);
  ASSERT (RSqrMod != NULL);
  ASSERT (Signature != NULL);
  ASSERT (SignatureSize > 0);
  ASSERT (Hash != NULL);
  ASSERT (HashSize > 0);

  STATIC_ASSERT (
    OcSigHashTypeSha512 == OcSigHashTypeMax - 1,
    "New switch cases have to be added for every introduced algorithm."
    );

  if (NumWords > OC_BN_MAX_LEN) {
    return FALSE;
  }

  if (!InternalSigHashTypeIsAllowed (Algorithm)) {
    return FALSE;
  }

  switch (Algorithm) {

#ifdef OC_CRYPTO_SUPPORTS_SHA256
    case OcSigHashTypeSha256:
    {
      ASSERT (HashSize == SHA256_DIGEST_SIZE);

      Padding     = mPkcsDigestEncodingPrefixSha256;
      PaddingSize = sizeof (mPkcsDigestEncodingPrefixSha256);
      break;
    }
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA384
    case OcSigHashTypeSha384:
    {
      ASSERT (HashSize == SHA384_DIGEST_SIZE);

      Padding     = mPkcsDigestEncodingPrefixSha384;
      PaddingSize = sizeof (mPkcsDigestEncodingPrefixSha384);
      break;
    }
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA512
    case OcSigHashTypeSha512:
    {
      ASSERT (HashSize == SHA512_DIGEST_SIZE);

      Padding     = mPkcsDigestEncodingPrefixSha512;
      PaddingSize = sizeof (mPkcsDigestEncodingPrefixSha512);
      break;
    }
#endif

    default:
    {
      ASSERT (FALSE);
      Padding     = NULL;
      PaddingSize = 0;
    }
  }
  //
  // Verify the Signature size matches the Modulus size.
  // This implicitly verifies it's a multiple of the Word size.
  //
  ModulusSize = NumWords * OC_BN_WORD_SIZE;
  if (!InternalRsaModulusSizeIsAllowed (ModulusSize)) {
    return FALSE;
  }

  if (SignatureSize != ModulusSize) {
    DEBUG ((DEBUG_INFO, "OCCR: Signature length does not match key length\n"));
    return FALSE;
  }

  EncryptedSigNum = Scratch;
  DecryptedSigNum = EncryptedSigNum + NumWords;
  PowScratchNum   = DecryptedSigNum + NumWords;

  BigNumParseBuffer (
    EncryptedSigNum,
    (OC_BN_NUM_WORDS)NumWords,
    Signature,
    SignatureSize
    );

  Result = BigNumPowMod (
             DecryptedSigNum,
             (OC_BN_NUM_WORDS)NumWords,
             EncryptedSigNum,
             Exponent,
             N,
             N0Inv,
             RSqrMod,
             PowScratchNum
             );
  if (!Result) {
    return FALSE;
  }
  //
  // Convert the result to a big-endian byte array.
  // Re-use EncryptedSigNum as it is not required anymore.
  // FIXME: Doing this as part of the comparison could speed up the process
  //        and clean up the code.
  //
  Index = NumWords;
  while (Index > 0) {
    --Index;
    //
    // Note: This does work with Big Endian systems.
    //
    Tmp = BigNumSwapWord (
            DecryptedSigNum[NumWords - 1 - Index]
            );
    EncryptedSigNum[Index] = Tmp;
  }
  Signature = (UINT8 *)EncryptedSigNum;

  //
  // From RFC 3447, 9.2 EMSA-PKCS1-v1_5:
  //
  // 5. Concatenate PS, the DER encoding T, and other padding to form the
  //    encoded message EM as
  //
  //     EM = 0x00 || 0x01 || PS || 0x00 || T.
  //

  //
  // 3. If emLen < tLen + 11, output "intended encoded message length too
  //    short" and stop.
  //
  // The additions cannot overflow because both PaddingSize and HashSize are
  // sane at this point.
  //
  DigestSize = PaddingSize + HashSize;
  if (SignatureSize < DigestSize + 11) {
    return FALSE;
  }

  if (Signature[0] != 0x00 || Signature[1] != 0x01) {
    return FALSE;
  }
  //
  // 4. Generate an octet string PS consisting of emLen - tLen - 3 octets with
  //    hexadecimal value 0xff.  The length of PS will be at least 8 octets.
  //
  // The additions and subtractions cannot overflow as per 3.
  //
  for (Index = 2; Index < SignatureSize - DigestSize - 3 + 2; ++Index) {
    if (Signature[Index] != 0xFF) {
      return FALSE;
    }
  }

  if (Signature[Index] != 0x00) {
    return FALSE;
  }

  ++Index;

  CmpResult = CompareMem (&Signature[Index], Padding, PaddingSize);
  if (CmpResult != 0) {
    return FALSE;
  }

  Index += PaddingSize;

  CmpResult = CompareMem (&Signature[Index], Hash, HashSize);
  if (CmpResult != 0) {
    return FALSE;
  }
  //
  // The code above must have covered the entire Signature range.
  //
  ASSERT (Index + HashSize == SignatureSize);

  return TRUE;
}

/**
  Verify RSA PKCS1.5 signed data against its signature.
  The modulus' size must be a multiple of the configured BIGNUM word size.
  This will be true for any conventional RSA, which use two's potencies.

  @param[in] N              The RSA modulus.
  @param[in] NumWords       The number of Words of N and RSqrMod.
  @param[in] N0Inv          The Montgomery Inverse of N.
  @param[in] RSqrMod        Montgomery's R^2 mod N.
  @param[in] Exponent       The RSA exponent.
  @param[in] Signature      The RSA signature to be verified.
  @param[in] SignatureSize  Size, in bytes, of Signature.
  @param[in] Data           The signed data to verify.
  @param[in] DataSize       Size, in bytes, of Data.
  @param[in] Algorithm      The RSA algorithm used.
  @param[in] Scratch        Scratch buffer 3xModulo.

  @returns  Whether the signature has been successfully verified as valid.

**/
STATIC
BOOLEAN
RsaVerifySigDataFromProcessed (
  IN CONST OC_BN_WORD  *N,
  IN OC_BN_NUM_WORDS   NumWords,
  IN OC_BN_WORD        N0Inv,
  IN CONST OC_BN_WORD  *RSqrMod,
  IN UINT32            Exponent,
  IN CONST UINT8       *Signature,
  IN UINTN             SignatureSize,
  IN CONST UINT8       *Data,
  IN UINTN             DataSize,
  IN OC_SIG_HASH_TYPE  Algorithm,
  IN OC_BN_WORD        *Scratch
  )
{
  UINT8 Hash[OC_MAX_SHA_DIGEST_SIZE];
  UINTN HashSize;

  ASSERT (N != NULL);
  ASSERT (NumWords > 0);
  ASSERT (RSqrMod != NULL);
  ASSERT (Exponent > 0);
  ASSERT (Signature != NULL);
  ASSERT (SignatureSize > 0);
  ASSERT (Data != NULL);
  ASSERT (DataSize > 0);

  STATIC_ASSERT (
    OcSigHashTypeSha512 == OcSigHashTypeMax - 1,
    "New switch cases have to be added for every introduced algorithm."
    );

  switch (Algorithm) {

#ifdef OC_CRYPTO_SUPPORTS_SHA256
    case OcSigHashTypeSha256:
    {
      Sha256 (Hash, Data, DataSize);
      HashSize = SHA256_DIGEST_SIZE;
      break;
    }
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA384
    case OcSigHashTypeSha384:
    {
      Sha384 (Hash, Data, DataSize);
      HashSize = SHA384_DIGEST_SIZE;
      break;
    }
#endif

#ifdef OC_CRYPTO_SUPPORTS_SHA512
    case OcSigHashTypeSha512:
    {
      Sha512 (Hash, Data, DataSize);
      HashSize = SHA512_DIGEST_SIZE;
      break;
    }
#endif

    default:
    {
      //
      // New switch cases have to be added for every introduced algorithm.
      //
      ASSERT (FALSE);
      return FALSE;
    }
  }

  return RsaVerifySigHashFromProcessed (
           N,
           NumWords,
           N0Inv,
           RSqrMod,
           Exponent,
           Signature,
           SignatureSize,
           Hash,
           HashSize,
           Algorithm,
           Scratch
           );
}

#ifndef OC_CRYPTO_STATIC_MEMORY_ALLOCATION

BOOLEAN
RsaVerifySigDataFromData (
  IN CONST UINT8       *Modulus,
  IN UINTN             ModulusSize,
  IN UINT32            Exponent,
  IN CONST UINT8       *Signature,
  IN UINTN             SignatureSize,
  IN CONST UINT8       *Data,
  IN UINTN             DataSize,
  IN OC_SIG_HASH_TYPE  Algorithm
  )
{
  UINTN           ModulusNumWordsTmp;
  OC_BN_NUM_WORDS ModulusNumWords;

  VOID            *Memory;
  VOID            *Mont;
  OC_BN_WORD      *N;
  OC_BN_WORD      *RSqrMod;
  VOID            *Scratch;

  OC_BN_WORD      N0Inv;
  BOOLEAN         Result;

  ASSERT (Modulus != NULL);
  ASSERT (ModulusSize > 0);
  ASSERT (Exponent > 0);
  ASSERT (Signature != NULL);
  ASSERT (SignatureSize > 0);
  ASSERT (Data != NULL);
  ASSERT (DataSize > 0);

  ModulusNumWordsTmp = ModulusSize / OC_BN_WORD_SIZE;
  if (ModulusNumWordsTmp > OC_BN_MAX_LEN
   || (ModulusSize % OC_BN_WORD_SIZE) != 0) {
    return FALSE;
  }

  ModulusNumWords = (OC_BN_NUM_WORDS)ModulusNumWordsTmp;

  STATIC_ASSERT (
    OC_BN_MAX_SIZE <= MAX_UINTN / 2,
    "An overflow verification must be added"
    );

  Memory = AllocatePool (
    2 * ModulusSize + BIG_NUM_MONT_PARAMS_SCRATCH_SIZE (ModulusNumWords)
    );
  if (Memory == NULL) {
    return FALSE;
  }

  N       = (OC_BN_WORD *)Memory;
  RSqrMod = (OC_BN_WORD *)((UINTN)N + ModulusSize);
  Mont    = (UINT8 *)Memory + 2 * ModulusSize;

  BigNumParseBuffer (N, ModulusNumWords, Modulus, ModulusSize);

  N0Inv = BigNumCalculateMontParams (RSqrMod, ModulusNumWords, N, Mont);
  if (N0Inv == 0) {
    FreePool (Memory);
    return FALSE;
  }

  Scratch = AllocatePool (RSA_SCRATCH_BUFFER_SIZE (ModulusSize));
  if (Scratch == NULL) {
    FreePool (Memory);
    return FALSE;
  }

  Result = RsaVerifySigDataFromProcessed (
             N,
             ModulusNumWords,
             N0Inv,
             RSqrMod,
             Exponent,
             Signature,
             SignatureSize,
             Data,
             DataSize,
             Algorithm,
             Scratch
             );

  FreePool (Scratch);
  FreePool (Memory);
  return Result;
}

#endif

BOOLEAN
RsaVerifySigHashFromKey (
  IN CONST OC_RSA_PUBLIC_KEY  *Key,
  IN CONST UINT8              *Signature,
  IN UINTN                    SignatureSize,
  IN CONST UINT8              *Hash,
  IN UINTN                    HashSize,
  IN OC_SIG_HASH_TYPE         Algorithm,
  IN VOID                     *Scratch
  )
{
  ASSERT (Key != NULL);
  ASSERT (Signature != NULL);
  ASSERT (SignatureSize > 0);
  ASSERT (Hash != NULL);
  ASSERT (HashSize > 0);

  STATIC_ASSERT (
    OC_BN_WORD_SIZE <= 8,
    "The parentheses need to be changed to avoid truncation."
    );
  //
  // When OC_BN_WORD is not UINT64, this violates the strict aliasing rule.
  // However, due to packed-ness and byte order, this is perfectly safe.
  //
  return RsaVerifySigHashFromProcessed (
           (OC_BN_WORD *)Key->Data,
           Key->Hdr.NumQwords * (8 / OC_BN_WORD_SIZE),
           (OC_BN_WORD)Key->Hdr.N0Inv,
           (OC_BN_WORD *)&Key->Data[Key->Hdr.NumQwords],
           0x10001,
           Signature,
           SignatureSize,
           Hash,
           HashSize,
           Algorithm,
           Scratch
           );
}

BOOLEAN
RsaVerifySigDataFromKey (
  IN CONST OC_RSA_PUBLIC_KEY  *Key,
  IN CONST UINT8              *Signature,
  IN UINTN                    SignatureSize,
  IN CONST UINT8              *Data,
  IN UINTN                    DataSize,
  IN OC_SIG_HASH_TYPE         Algorithm,
  IN VOID                     *Scratch
  )
{
  ASSERT (Key != NULL);
  ASSERT (Signature != NULL);
  ASSERT (SignatureSize > 0);
  ASSERT (Data != NULL);
  ASSERT (DataSize > 0);

  STATIC_ASSERT (
    OC_BN_WORD_SIZE <= 8,
    "The parentheses need to be changed to avoid truncation."
    );
  //
  // When OC_BN_WORD is not UINT64, this violates the strict aliasing rule.
  // However, due to packed-ness and byte order, this is perfectly safe.
  //
  return RsaVerifySigDataFromProcessed (
           (OC_BN_WORD *)Key->Data,
           Key->Hdr.NumQwords * (8 / OC_BN_WORD_SIZE),
           (OC_BN_WORD)Key->Hdr.N0Inv,
           (OC_BN_WORD *)&Key->Data[Key->Hdr.NumQwords],
           0x10001,
           Signature,
           SignatureSize,
           Data,
           DataSize,
           Algorithm,
           Scratch
           );
}
