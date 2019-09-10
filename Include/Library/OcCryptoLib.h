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

//
// Default to 2048-bit key length for RSA.
//
#ifndef CONFIG_RSA_KEY_BIT_SIZE
#define CONFIG_RSA_KEY_BIT_SIZE 2048
#define CONFIG_RSA_KEY_SIZE (CONFIG_RSA_KEY_BIT_SIZE / 8)
#endif

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

//
// Block sizes.
//
#define SHA256_BLOCK_SIZE  64
#define SHA512_BLOCK_SIZE  128
#define SHA384_BLOCK_SIZE  SHA512_BLOCK_SIZE

//
// Derived parameters.
//
#define RSANUMWORDS (CONFIG_RSA_KEY_SIZE / sizeof (UINT32))
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
// For now abort on anything but 2048, but we can support 1024 and 4096 at least.
//
#if CONFIG_RSA_KEY_BIT_SIZE != 2048 || CONFIG_RSA_KEY_SIZE != 256
#error "Only RSA-2048 is supported"
#endif

#pragma pack(push, 1)

typedef struct RSA_PUBLIC_KEY_ {
  UINT32  Size;
  UINT32  N0Inv;
  UINT32  N[RSANUMWORDS];
  UINT32  Rr[RSANUMWORDS];
} RSA_PUBLIC_KEY;

#pragma pack(pop)

typedef struct AES_CONTEXT_ {
  UINT8 RoundKey[AES_KEY_EXP_SIZE];
  UINT8 Iv[AES_BLOCK_SIZE];
} AES_CONTEXT;

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

//
// Functions prototypes
//
BOOLEAN
RsaVerify (
  RSA_PUBLIC_KEY  *Key,
  UINT8           *Signature,
  UINT8           *Sha256
  );

VOID
AesInitCtxIv (
  AES_CONTEXT  *Context,
  CONST UINT8  *Key,
  CONST UINT8  *Iv
  );

VOID
AesSetCtxIv (
  AES_CONTEXT  *Context,
  CONST UINT8  *Iv
  );

//
// Data size MUST be mutiple of AES_BLOCK_SIZE;
// Suggest https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for padding scheme
// NOTES: you need to set Iv in Context via AesInitCtxIv() or AesSetCtxIv()
//        no Iv should ever be reused with the same key
//
VOID
AesCbcEncryptBuffer (
  AES_CONTEXT  *Context,
  UINT8        *Data,
  UINT32       Len
  );

VOID
AesCbcDecryptBuffer (
  AES_CONTEXT  *Context,
  UINT8        *Data,
  UINT32       Len
  );

//
// Same function for encrypting as for decrypting.
// Iv is incremented for every block, and used after encryption as XOR-compliment for output
// Suggesting https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for padding scheme
// NOTES: you need to set Iv in Context via AesInitCtxIv() or AesSetCtxIv()
//        no Iv should ever be reused with the same key
//
VOID
AesCtrXcryptBuffer (
  AES_CONTEXT  *Context,
  UINT8        *Data,
  UINT32       Len
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
