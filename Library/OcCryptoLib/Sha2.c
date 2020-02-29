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

/**
* Filename:   sha256.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA-256 hashing algorithm.
              SHA-256 is one of the three algorithms in the SHA2
              specification. The others, SHA-384 and SHA-512, are not
              offered in this implementation.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
**/

#ifdef EFIAPI
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#endif

#include <Library/OcCryptoLib.h>


#define UNPACK64(x, str)                         \
  do {                                           \
    *((str) + 7) = (UINT8) (x);                  \
    *((str) + 6) = (UINT8) RShiftU64 ((x),  8);  \
    *((str) + 5) = (UINT8) RShiftU64 ((x), 16);  \
    *((str) + 4) = (UINT8) RShiftU64 ((x), 24);  \
    *((str) + 3) = (UINT8) RShiftU64 ((x), 32);  \
    *((str) + 2) = (UINT8) RShiftU64 ((x), 40);  \
    *((str) + 1) = (UINT8) RShiftU64 ((x), 48);  \
    *((str) + 0) = (UINT8) RShiftU64 ((x), 56);  \
  } while(0)

#define PACK64(str, x)                           \
  do {                                           \
    *(x) =    ((UINT64) *((str) + 7))            \
           | LShiftU64 (*((str) + 6),  8)        \
           | LShiftU64 (*((str) + 5), 16)        \
           | LShiftU64 (*((str) + 4), 24)        \
           | LShiftU64 (*((str) + 3), 32)        \
           | LShiftU64 (*((str) + 2), 40)        \
           | LShiftU64 (*((str) + 1), 48)        \
           | LShiftU64 (*((str) + 0), 56);       \
  } while (0)


#define SHFR(a, b)    (a >> b)
#define ROTLEFT(a, b) ((a << b) | (a >> ((sizeof(a) << 3) - b)))
#define ROTRIGHT(a, b) ((a >> b) | (a << ((sizeof(a) << 3) - b)))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

//
// Sha 256
//
#define SHA256_EP0(x)  (ROTRIGHT(x, 2)  ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define SHA256_EP1(x)  (ROTRIGHT(x, 6)  ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SHA256_SIG0(x) (ROTRIGHT(x, 7)  ^ ROTRIGHT(x, 18) ^ SHFR(x, 3))
#define SHA256_SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ SHFR(x, 10))

//
// Sha 512
//
#define SHA512_EP0(x)  (ROTRIGHT(x, 28) ^ ROTRIGHT(x, 34) ^ ROTRIGHT(x, 39))
#define SHA512_EP1(x)  (ROTRIGHT(x, 14) ^ ROTRIGHT(x, 18) ^ ROTRIGHT(x, 41))
#define SHA512_SIG0(x) (ROTRIGHT(x,  1) ^ ROTRIGHT(x,  8) ^ SHFR(x,  7))
#define SHA512_SIG1(x) (ROTRIGHT(x, 19) ^ ROTRIGHT(x, 61) ^ SHFR(x,  6))

#define SHA512_SCR(Index)                                   \
  do {                                                      \
    W[Index] =  SHA512_SIG1(W[Index -  2]) + W[Index -  7]  \
          + SHA512_SIG0(W[Index - 15]) + W[Index - 16];     \
  } while(0)



STATIC CONST UINT32 SHA256_K[64] = {
  0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
  0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
  0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
  0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
  0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
  0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
  0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
  0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};


STATIC UINT64 SHA512_K[80] = {
  0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL,
  0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
  0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL,
  0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
  0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
  0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
  0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL, 0x2de92c6f592b0275ULL,
  0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
  0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL,
  0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
  0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL,
  0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
  0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL,
  0x92722c851482353bULL, 0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
  0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
  0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
  0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL,
  0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
  0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL,
  0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
  0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL,
  0xc67178f2e372532bULL, 0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
  0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL,
  0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
  0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
  0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
  0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

//
// Sha 256 Init State
//
STATIC CONST UINT32 SHA256_H0[8] = {
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

//
// Sha 384 Init State
//
STATIC CONST UINT64 SHA384_H0[8] = {
  0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
  0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
  0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
  0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL
};

//
// Sha 512 Init State
//
STATIC CONST UINT64 SHA512_H0[8] = {
  0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
  0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
  0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
  0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

//
// Sha 256 functions
//
VOID
Sha256Transform (
  SHA256_CONTEXT  *Context,
  CONST UINT8     *Data
  )
{
  UINT32 A, B, C, D, E, F, G, H, Index1, Index2, T1, T2;
  UINT32 M[64];

  for (Index1 = 0, Index2 = 0; Index1 < 16; Index1++, Index2 += 4) {
    M[Index1] = ((UINT32)Data[Index2] << 24)
                | ((UINT32)Data[Index2 + 1] << 16)
                | ((UINT32)Data[Index2 + 2] << 8)
                | ((UINT32)Data[Index2 + 3]);
  }

  for ( ; Index1 < 64; ++Index1) {
    M[Index1] = SHA256_SIG1 (M[Index1 - 2]) + M[Index1 - 7]
      + SHA256_SIG0 (M[Index1 - 15]) + M[Index1 - 16];
  }

  A = Context->State[0];
  B = Context->State[1];
  C = Context->State[2];
  D = Context->State[3];
  E = Context->State[4];
  F = Context->State[5];
  G = Context->State[6];
  H = Context->State[7];

  for (Index1 = 0; Index1 < 64; ++Index1) {
    T1 = H + SHA256_EP1 (E) + CH (E, F, G) + SHA256_K[Index1] + M[Index1];
    T2 = SHA256_EP0 (A) + MAJ (A, B, C);
    H = G;
    G = F;
    F = E;
    E = D + T1;
    D = C;
    C = B;
    B = A;
    A = T1 + T2;
  }

  Context->State[0] += A;
  Context->State[1] += B;
  Context->State[2] += C;
  Context->State[3] += D;
  Context->State[4] += E;
  Context->State[5] += F;
  Context->State[6] += G;
  Context->State[7] += H;
}

VOID
Sha256Init (
  SHA256_CONTEXT *Context
  )
{
  UINTN Index;
  for (Index = 0; Index < 8; ++Index) {
    Context->State[Index] = SHA256_H0[Index];
  }
  Context->DataLen = 0;
  Context->BitLen = 0;
}

VOID
Sha256Update (
  SHA256_CONTEXT *Context,
  CONST UINT8    *Data,
  UINTN          Len
  )
{
  UINT32 Index;

  for (Index = 0; Index < Len; ++Index) {
    Context->Data[Context->DataLen] = Data[Index];
    Context->DataLen++;
    if (Context->DataLen == 64) {
      Sha256Transform (Context, Context->Data);
      Context->BitLen += 512;
      Context->DataLen = 0;
    }
  }
}

VOID
Sha256Final (
  SHA256_CONTEXT  *Context,
  UINT8           *HashDigest
  )
{
  UINT32 Index  = 0;

  Index = Context->DataLen;

  //
  // Pad whatever data is left in the buffer.
  //
  if (Context->DataLen < 56) {
    Context->Data[Index++] = 0x80;
    ZeroMem (Context->Data + Index, 56-Index);
  } else {
    Context->Data[Index++] = 0x80;
    ZeroMem (Context->Data + Index, 64-Index);
    Sha256Transform (Context, Context->Data);
    ZeroMem (Context->Data, 56);
  }

  //
  // Append to the padding the total Message's length in bits and transform.
  //
  Context->BitLen  += Context->DataLen * 8;
  Context->Data[63] = (UINT8) Context->BitLen;
  Context->Data[62] = (UINT8) (Context->BitLen >> 8);
  Context->Data[61] = (UINT8) (Context->BitLen >> 16);
  Context->Data[60] = (UINT8) (Context->BitLen >> 24);
  Context->Data[59] = (UINT8) (Context->BitLen >> 32);
  Context->Data[58] = (UINT8) (Context->BitLen >> 40);
  Context->Data[57] = (UINT8) (Context->BitLen >> 48);
  Context->Data[56] = (UINT8) (Context->BitLen >> 56);
  Sha256Transform (Context, Context->Data);

  //
  // Since this implementation uses little endian byte ordering and SHA uses big endian,
  // reverse all the bytes when copying the final State to the output hash.
  //
  for (Index = 0; Index < 4; ++Index) {
    HashDigest[Index]      = (UINT8) ((Context->State[0] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 4]  = (UINT8) ((Context->State[1] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 8]  = (UINT8) ((Context->State[2] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 12] = (UINT8) ((Context->State[3] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 16] = (UINT8) ((Context->State[4] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 20] = (UINT8) ((Context->State[5] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 24] = (UINT8) ((Context->State[6] >> (24 - Index * 8)) & 0x000000FF);
    HashDigest[Index + 28] = (UINT8) ((Context->State[7] >> (24 - Index * 8)) & 0x000000FF);
  }
}

VOID
Sha256 (
  UINT8        *Hash,
  CONST UINT8  *Data,
  UINTN        Len
  )
{
  SHA256_CONTEXT  Ctx;

  Sha256Init (&Ctx);
  Sha256Update (&Ctx, Data, Len);
  Sha256Final (&Ctx, Hash);
  ZeroMem (&Ctx, sizeof (Ctx));
}


//
// Sha 512 functions
//
VOID
Sha512Transform (
  SHA512_CONTEXT  *Context,
  CONST UINT8     *Data,
  UINTN           BlockNb
  )
{
  UINT64       W[80];
  UINT64       Wv[8];
  UINT64       T1;
  UINT64       T2;
  CONST UINT8  *SubBlock;
  UINTN        Index1;
  UINTN        Index2;

  for (Index1 = 0; Index1 < BlockNb; ++Index1) {
    SubBlock = Data + (Index1 << 7);

    //
    // Convert from big-endian byte order to host byte order
    //
    for (Index2 = 0; Index2 < 16; ++Index2) {
      PACK64 (&SubBlock[Index2 << 3], &W[Index2]);
    }

    //
    // Initialize the 8 working registers
    //
    for (Index2 = 0; Index2 < 8; ++Index2) {
      Wv[Index2] = Context->State[Index2];
    }

    for (Index2 = 0; Index2 < 80; ++Index2) {
      //
      // Prepare the message schedule
      //
      if (Index2 >= 16) {
        SHA512_SCR (Index2);
      }

      //
      // Calculate T1 and T2
      //
      T1 = Wv[7] + SHA512_EP1 (Wv[4])
        + CH (Wv[4], Wv[5], Wv[6]) + SHA512_K[Index2]
        + W[Index2];

      T2 = SHA512_EP0 (Wv[0]) + MAJ (Wv[0], Wv[1], Wv[2]);

      //
      // Update the working registers
      //
      Wv[7] = Wv[6];
      Wv[6] = Wv[5];
      Wv[5] = Wv[4];
      Wv[4] = Wv[3] + T1;
      Wv[3] = Wv[2];
      Wv[2] = Wv[1];
      Wv[1] = Wv[0];
      Wv[0] = T1 + T2;
    }
    //
    // Update the hash value
    //
    for (Index2 = 0; Index2 < 8; ++Index2) {
      Context->State[Index2] += Wv[Index2];
    }
  }
}

VOID
Sha512Init (
  SHA512_CONTEXT  *Context
  )
{
  UINTN  Index;

  //
  // Set initial hash value
  //
  for (Index = 0; Index < 8; ++Index) {
    Context->State[Index] = SHA512_H0[Index];
  }

  //
  // Number of bytes in the buffer
  //
  Context->Length = 0;

  //
  // Total length of the data
  //
  Context->TotalLength = 0;
}

VOID
Sha512Update (
  SHA512_CONTEXT  *Context,
  CONST UINT8     *Data,
  UINTN           Len
  )
{
  UINTN        BlockNb;
  UINTN        NewLen;
  UINTN        RemLen;
  UINTN        TmpLen;
  CONST UINT8  *ShiftedMsg;

  TmpLen = SHA512_BLOCK_SIZE - Context->Length;
  RemLen = Len < TmpLen ? Len : TmpLen;

  CopyMem (&Context->Block[Context->Length], Data, RemLen);

  if (Context->Length + Len < SHA512_BLOCK_SIZE) {
    Context->Length += Len;
    return;
  }

  NewLen = Len - RemLen;
  BlockNb = NewLen / SHA512_BLOCK_SIZE;

  ShiftedMsg = Data + RemLen;

  Sha512Transform (Context, Context->Block, 1);
  Sha512Transform (Context, ShiftedMsg, BlockNb);

  RemLen = NewLen % SHA512_BLOCK_SIZE;

  CopyMem (Context->Block, &ShiftedMsg[BlockNb << 7], RemLen);

  Context->Length = RemLen;
  Context->TotalLength += (BlockNb + 1) << 7;
}

VOID
Sha512Final (
  SHA512_CONTEXT  *Context,
  UINT8           *HashDigest
  )
{
  UINTN   BlockNb;
  UINTN   PmLen;
  UINT64  LenB;
  UINTN   Index;

  BlockNb = ((SHA512_BLOCK_SIZE - 17) < (Context->Length % SHA512_BLOCK_SIZE)) + 1;

  LenB = (Context->TotalLength + Context->Length) << 3;
  PmLen = BlockNb << 7;

  ZeroMem (Context->Block + Context->Length, PmLen - Context->Length);
  Context->Block[Context->Length] = 0x80;
  UNPACK64 (LenB, Context->Block + PmLen - 8);

  Sha512Transform (Context, Context->Block, BlockNb);

  for (Index = 0 ; Index < 8; ++Index) {
    UNPACK64 (Context->State[Index], &HashDigest[Index << 3]);
  }
}

VOID
Sha512 (
  UINT8        *Hash,
  CONST UINT8  *Data,
  UINTN        Len
  )
{
  SHA512_CONTEXT  Ctx;

  Sha512Init (&Ctx);
  Sha512Update (&Ctx, Data, Len);
  Sha512Final (&Ctx, Hash);
  ZeroMem (&Ctx, sizeof (Ctx));
}


//
// Sha 384 functions
//
VOID
Sha384Init (
  SHA384_CONTEXT  *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < 8; ++Index) {
    Context->State[Index] = SHA384_H0[Index];
  }

  Context->Length = 0;
  Context->TotalLength = 0;
}

VOID
Sha384Update (
  SHA384_CONTEXT  *Context,
  CONST UINT8     *Data,
  UINTN           Len
  )
{
  UINTN        BlockNb;
  UINTN        NewLen;
  UINTN        RemLen;
  UINTN        TmpLen;
  CONST UINT8  *ShiftedMessage;

  TmpLen = SHA384_BLOCK_SIZE - Context->Length;
  RemLen = Len < TmpLen ? Len : TmpLen;

  CopyMem (&Context->Block[Context->Length], Data, RemLen);

  if (Context->Length + Len < SHA384_BLOCK_SIZE) {
    Context->Length += Len;
    return;
  }

  NewLen = Len - RemLen;
  BlockNb = NewLen / SHA384_BLOCK_SIZE;

  ShiftedMessage = Data + RemLen;

  Sha512Transform (Context, Context->Block, 1);
  Sha512Transform (Context, ShiftedMessage, BlockNb);

  RemLen = NewLen % SHA384_BLOCK_SIZE;

  CopyMem (
    Context->Block,
    &ShiftedMessage[BlockNb << 7],
    RemLen
    );

  Context->Length = RemLen;
  Context->TotalLength += (BlockNb + 1) << 7;
}

VOID
Sha384Final (
  SHA384_CONTEXT  *Context,
  UINT8           *HashDigest
  )
{
  UINTN    BlockNb;
  UINTN    PmLen;
  UINT64   LenB;
  UINTN    Index;

  BlockNb = ((SHA384_BLOCK_SIZE - 17) < (Context->Length % SHA384_BLOCK_SIZE)) + 1;

  LenB = (Context->TotalLength + Context->Length) << 3;
  PmLen = BlockNb << 7;

  ZeroMem (Context->Block + Context->Length, PmLen - Context->Length);

  Context->Block[Context->Length] = 0x80;
  UNPACK64 (LenB, Context->Block + PmLen - 8);

  Sha512Transform (Context, Context->Block, BlockNb);

  for (Index = 0 ; Index < 6; ++Index) {
    UNPACK64 (Context->State[Index], &HashDigest[Index << 3]);
  }
}

VOID
Sha384 (
  UINT8        *Hash,
  CONST UINT8  *Data,
  UINTN        Len
  )
{
  SHA384_CONTEXT Ctx;

  Sha384Init (&Ctx);
  Sha384Update (&Ctx, Data, Len);
  Sha384Final (&Ctx, Hash);
  SecureZeroMem (&Ctx, sizeof (Ctx));
}
