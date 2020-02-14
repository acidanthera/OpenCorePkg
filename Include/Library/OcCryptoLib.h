/** @file

OcCryptoLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_CRYPTO_LIB_H
#define OC_CRYPTO_LIB_H

#include <Library/OcGuardLib.h>

//
// Default to 128-bit key length for AES.
//
#ifndef CONFIG_AES_KEY_SIZE
#define CONFIG_AES_KEY_SIZE 16
#endif

//
// Digest sizes.
//
#define MD5_DIGEST_SIZE     16
#define SHA1_DIGEST_SIZE    20
#define SHA256_DIGEST_SIZE  32
#define SHA384_DIGEST_SIZE  48
#define SHA512_DIGEST_SIZE  64

#define OC_MAX_SHA_DIGEST_SIZE  SHA512_DIGEST_SIZE

//
// Block sizes.
//
#define SHA256_BLOCK_SIZE  64
#define SHA512_BLOCK_SIZE  128
#define SHA384_BLOCK_SIZE  SHA512_BLOCK_SIZE

//
// Derived parameters.
//
#define AES_BLOCK_SIZE 16

//
// Support all AES key sizes.
//
#if CONFIG_AES_KEY_SIZE == 32
#define AES_KEY_EXP_SIZE 240
#elif CONFIG_AES_KEY_SIZE == 24
#define AES_KEY_EXP_SIZE 208
#elif CONFIG_AES_KEY_SIZE == 16
#define AES_KEY_EXP_SIZE 176
#else
#error "Only AES-128, AES-192, and AES-256 are supported!"
#endif

//
// ChaCha key size.
//
#define CHACHA_KEY_SIZE 32

//
// ChaCha IV size.
//
#define CHACHA_IV_SIZE  12

//
// Possible RSA algorithm types supported by OcCryptoLib
// for RSA digital signature verification
// PcdOcCryptoAllowedSigHashTypes MUST be kept in sync with changes!
//
typedef enum OC_SIG_HASH_TYPE_ {
  OcSigHashTypeSha256,
  OcSigHashTypeSha384, 
  OcSigHashTypeSha512,
  OcSigHashTypeMax
} OC_SIG_HASH_TYPE;

typedef struct AES_CONTEXT_ {
  UINT8 RoundKey[AES_KEY_EXP_SIZE];
  UINT8 Iv[AES_BLOCK_SIZE];
} AES_CONTEXT;

typedef struct CHACHA_CONTEXT_ {
  UINT32 Input[16];
} CHACHA_CONTEXT;

typedef struct MD5_CONTEXT_ {
  UINT8   Data[64];
  UINT32  DataLen;
  UINT64  BitLen;
  UINT32  State[4];
} MD5_CONTEXT;

typedef struct SHA1_CONTEXT_ {
  UINT8   Data[64];
  UINT32  DataLen;
  UINT64  BitLen;
  UINT32  State[5];
  UINT32  K[4];
} SHA1_CONTEXT;

typedef struct SHA256_CONTEXT_ {
  UINT8   Data[64];
  UINT32  DataLen;
  UINT64  BitLen;
  UINT32  State[8];
} SHA256_CONTEXT;

typedef struct SHA512_CONTEXT_ {
  UINT64 TotalLength;
  UINTN  Length;
  UINT8  Block[2 * SHA512_BLOCK_SIZE];
  UINT64 State[8];
} SHA512_CONTEXT;

typedef SHA512_CONTEXT SHA384_CONTEXT;

#pragma pack(push, 1)

///
/// The structure describing the RSA Public Key format.
/// The exponent is always 65537.
///
typedef PACKED struct {
  ///
  /// The number of 64-bit values of N and RSqrMod each.
  ///
  UINT16 NumQwords;
  ///
  /// Padding for 64-bit alignment. Must be 0 to allow future extensions.
  ///
  UINT8  Reserved[6];
  ///
  /// The Montgomery Inverse in 64-bit space: -1 / N[0] mod 2^64.
  ///
  UINT64 N0Inv;
} OC_RSA_PUBLIC_KEY_HDR;

typedef PACKED struct {
  ///
  /// The RSA Public Key header structure.
  ///
  OC_RSA_PUBLIC_KEY_HDR Hdr;
  ///
  /// The Modulus and Montgomery's R^2 mod N in little endian byte order.
  ///
  UINT64                Data[];
} OC_RSA_PUBLIC_KEY;

#pragma pack(pop)

//
// Functions prototypes
//

VOID
AesInitCtxIv (
  OUT AES_CONTEXT  *Context,
  IN  CONST UINT8  *Key,
  IN  CONST UINT8  *Iv
  );

VOID
AesSetCtxIv (
  OUT AES_CONTEXT  *Context,
  IN  CONST UINT8  *Iv
  );

//
// Data size MUST be mutiple of AES_BLOCK_SIZE;
// Suggest https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for Padding scheme
// NOTES: you need to set Iv in Context via AesInitCtxIv() or AesSetCtxIv()
//        no Iv should ever be reused with the same key
//
VOID
AesCbcEncryptBuffer (
  IN OUT AES_CONTEXT  *Context,
  IN OUT UINT8        *Data,
  IN     UINT32       Len
  );

VOID
AesCbcDecryptBuffer (
  IN OUT AES_CONTEXT  *Context,
  IN OUT UINT8        *Data,
  IN     UINT32       Len
  );

//
// Same function for encrypting as for decrypting.
// Iv is incremented for every block, and used after encryption as XOR-compliment for output
// Suggesting https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for Padding scheme
// NOTES: you need to set Iv in Context via AesInitCtxIv() or AesSetCtxIv()
//        no Iv should ever be reused with the same key
//
VOID
AesCtrXcryptBuffer (
  IN OUT AES_CONTEXT  *Context,
  IN OUT UINT8        *Data,
  IN     UINT32       Len
  );

/**
  Setup ChaCha context (IETF variant).

  @param[out] Context  ChaCha context.
  @param[in]  Key      ChaCha 256-bit key.
  @param[in]  Iv       ChaCha 96-bit initialisation vector.
  @param[in]  Counter  ChaCha 32-bit counter.
**/
VOID
ChaChaInitCtx (
  OUT CHACHA_CONTEXT  *Context,
  IN  CONST UINT8     *Key,
  IN  CONST UINT8     *Iv,
  IN  UINT32          Counter
  );

/**
  Perform ChaCha encryption/decryption.
  Source may match Destination.

  @param[in,out] Context     ChaCha context.
  @param[in]     Source      Data for transformation.
  @param[out]    Destination Resulting data.
  @param[in]     Length      Data and ciphertext lengths.
**/
VOID
ChaChaCryptBuffer (
  IN OUT CHACHA_CONTEXT  *Context,
  IN     CONST UINT8     *Source,
     OUT UINT8           *Destination,
  IN     UINT32          Length
  );

VOID
Md5Init (
  MD5_CONTEXT  *Context
  );

VOID
Md5Update (
  MD5_CONTEXT  *Context,
  CONST UINT8  *Data,
  UINTN        Len
  );

VOID
Md5Final (
  MD5_CONTEXT  *Context,
  UINT8        *Hash
  );

VOID
Md5 (
  UINT8  *Hash,
  UINT8  *Data,
  UINTN  Len
  );

VOID
Sha1Init (
  SHA1_CONTEXT  *Context
  );

VOID
Sha1Update (
  SHA1_CONTEXT  *Context,
  CONST UINT8   *Data,
  UINTN         Len
  );

VOID
Sha1Final (
  SHA1_CONTEXT  *Context,
  UINT8         *Hash
  );

VOID
Sha1 (
  UINT8  *Hash,
  UINT8  *Data,
  UINTN  Len
  );

VOID
Sha256Init (
  SHA256_CONTEXT  *Context
  );

VOID
Sha256Update (
  SHA256_CONTEXT  *Context,
  CONST UINT8     *Data,
  UINTN           Len
  );

VOID
Sha256Final (
  SHA256_CONTEXT  *Context,
  UINT8           *HashDigest
  );

VOID
Sha256 (
  UINT8        *Hash,
  CONST UINT8  *Data,
  UINTN        Len
  );

VOID
Sha512Init (
  SHA512_CONTEXT  *Context
  );

VOID
Sha512Update (
  SHA512_CONTEXT  *Context,
  CONST UINT8     *Data,
  UINTN           Len
  );

VOID
Sha512Final (
  SHA512_CONTEXT  *Context,
  UINT8           *HashDigest
  );

VOID
Sha512 (
  UINT8        *Hash,
  CONST UINT8  *Data,
  UINTN        Len
  );

VOID
Sha384Init (
  SHA384_CONTEXT  *Context
  );

VOID 
Sha384Update (
  SHA384_CONTEXT  *Context,
  CONST UINT8     *Data,
  UINTN           Len
  );

VOID 
Sha384Final (
  SHA384_CONTEXT  *Context,
  UINT8           *HashDigest
  );

VOID 
Sha384 (
  UINT8        *Hash,
  CONST UINT8  *Data,
  UINTN        Len
  );

/**
  Verifies Data against Hash with the appropiate SHA2 algorithm for HashSize.

  @param[in] Data      The data to verify the hash of.
  @param[in] DataSize  The, in bytes, of Data.
  @param[in] Hash      The reference hash to verify against.
  @param[in] HashSize  The size, in bytes, of Hash.

  @return 0         All HashSize bytes of the two buffers are identical.
  @retval Non-zero  If HashSize is not a valid SHA2 digest size, -1. Otherwise,
                    the first mismatched byte in Data's hash subtracted from
                    the first mismatched byte in Hash.

**/
INTN
SigVerifyShaHashBySize (
  IN CONST VOID   *Data,
  IN UINTN        DataSize,
  IN CONST UINT8  *Hash,
  IN UINTN        HashSize
  );

/**
  Verify a RSA PKCS1.5 signature against an expected hash.
  The exponent is always 65537 as per the format specification.

  @param[in] Key            The RSA Public Key.
  @param[in] Signature      The RSA signature to be verified.
  @param[in] SignatureSize  Size, in bytes, of Signature.
  @param[in] Hash           The Hash digest of the signed data.
  @param[in] HashSize       Size, in bytes, of Hash.
  @param[in] Algorithm      The RSA algorithm used.

  @returns  Whether the signature has been successfully verified as valid.

**/
BOOLEAN
RsaVerifySigHashFromKey (
  IN CONST OC_RSA_PUBLIC_KEY  *Key,
  IN CONST UINT8              *Signature,
  IN UINTN                    SignatureSize,
  IN CONST UINT8              *Hash,
  IN UINTN                    HashSize,
  IN OC_SIG_HASH_TYPE         Algorithm
  );

/**
  Verify RSA PKCS1.5 signed data against its signature.
  The modulus' size must be a multiple of the configured BIGNUM word size.
  This will be true for any conventional RSA, which use two's potencies.

  @param[in] Modulus        The RSA modulus byte array.
  @param[in] ModulusSize    The size, in bytes, of Modulus.
  @param[in] Exponent       The RSA exponent.
  @param[in] Signature      The RSA signature to be verified.
  @param[in] SignatureSize  Size, in bytes, of Signature.
  @param[in] Data           The signed data to verify.
  @param[in] DataSize       Size, in bytes, of Data.
  @param[in] Algorithm      The RSA algorithm used.

  @returns  Whether the signature has been successfully verified as valid.

**/
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
  );

/**
  Verify RSA PKCS1.5 signed data against its signature.
  The modulus' size must be a multiple of the configured BIGNUM word size.
  This will be true for any conventional RSA, which use two's potencies.
  The exponent is always 65537 as per the format specification.

  @param[in] Key            The RSA Public Key.
  @param[in] Signature      The RSA signature to be verified.
  @param[in] SignatureSize  Size, in bytes, of Signature.
  @param[in] Data           The signed data to verify.
  @param[in] DataSize       Size, in bytes, of Data.
  @param[in] Algorithm      The RSA algorithm used.

  @returns  Whether the signature has been successfully verified as valid.

**/
BOOLEAN
RsaVerifySigDataFromKey (
  IN CONST OC_RSA_PUBLIC_KEY  *Key,
  IN CONST UINT8              *Signature,
  IN UINTN                    SignatureSize,
  IN CONST UINT8              *Data,
  IN UINTN                    DataSize,
  IN OC_SIG_HASH_TYPE         Algorithm
  );

/**
  Performs a cryptographically secure comparison of the contents of two
  buffers.

  This function compares Length bytes of SourceBuffer to Length bytes of
  DestinationBuffer in a cryptographically secure fashion. This especially
  implies that different lengths of the longest shared prefix do not change
  execution time in a way relevant to security.

  If Length > 0 and DestinationBuffer is NULL, then ASSERT().
  If Length > 0 and SourceBuffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  DestinationBuffer The pointer to the destination buffer to compare.
  @param  SourceBuffer      The pointer to the source buffer to compare.
  @param  Length            The number of bytes to compare.

  @return 0                 All Length bytes of the two buffers are identical.
  @retval -1                The two buffers are not identical within Length
                            bytes.
**/
INTN
SecureCompareMem (
  IN CONST VOID  *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  );

/**
  Performs a cryptographically secure memory zeroing.

  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param[out]  Buffer   The pointer to the destination buffer to zero.
  @param[in]   Length   The number of bytes to zero.

  @return Buffer.
**/
VOID *
SecureZeroMem (
  OUT VOID   *Buffer,
  IN  UINTN  Length
  );

/**
  Verify Password and Salt against RefHash.  The used hash function is SHA-512,
  thus the caller must ensure RefHash is at least 64 bytes in size.

  @param[in] Password      The entered password to verify.
  @param[in] PasswordSize  The size, in bytes, of Password.
  @param[in] Salt          The cryptographic salt appended to Password on hash.
  @param[in] SaltSize      The size, in bytes, of Salt.
  @param[in] RefHash       The SHA-512 hash of the reference password and Salt.

  @returns Whether Password and Salt cryptographically match RefHash.

**/
BOOLEAN
OcVerifyPasswordSha512 (
  IN CONST UINT8  *Password,
  IN UINT32       PasswordSize,
  IN CONST UINT8  *Salt,
  IN UINT32       SaltSize,
  IN CONST UINT8  *RefHash
  );

#endif // OC_CRYPTO_LIB_H
