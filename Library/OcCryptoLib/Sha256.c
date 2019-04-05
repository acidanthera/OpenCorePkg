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

#include <Library/BaseMemoryLib.h>
#include <Library/OcCryptoLib.h>

#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)  (ROTRIGHT(x, 2)  ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x)  (ROTRIGHT(x, 6)  ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7)  ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

STATIC CONST UINT32 K[64] = {
  0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
  0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
  0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
  0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
  0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
  0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
  0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
  0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

VOID
Sha256Transform (
  SHA256_CONTEXT  *Context,
  CONST UINT8     *Data
  )
{
  UINT32 A, B, C, D, E, F, G, H, Index1, Index2, T1, T2;
  UINT32 M[64];

  for (Index1 = 0, Index2 = 0; Index1 < 16; Index1++, Index2 += 4)
    M[Index1] = ((UINT32)Data[Index2] << 24) | ((UINT32)Data[Index2 + 1] << 16) | ((UINT32)Data[Index2 + 2] << 8) | ((UINT32)Data[Index2 + 3]);
  for ( ; Index1 < 64; Index1++)
    M[Index1] = SIG1 (M[Index1 - 2]) + M[Index1 - 7] + SIG0 (M[Index1 - 15]) + M[Index1 - 16];

  A = Context->State[0];
  B = Context->State[1];
  C = Context->State[2];
  D = Context->State[3];
  E = Context->State[4];
  F = Context->State[5];
  G = Context->State[6];
  H = Context->State[7];

  for (Index1 = 0; Index1 < 64; Index1++) {
    T1 = H + EP1 (E) + CH (E, F, G) + K[Index1] + M[Index1];
    T2 = EP0 (A) + MAJ (A, B, C);
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
  Context->DataLen = 0;
  Context->BitLen = 0;
  Context->State[0] = 0x6A09E667;
  Context->State[1] = 0xBB67AE85;
  Context->State[2] = 0x3C6EF372;
  Context->State[3] = 0xA54FF53A;
  Context->State[4] = 0x510E527F;
  Context->State[5] = 0x9B05688C;
  Context->State[6] = 0x1F83D9AB;
  Context->State[7] = 0X5BE0CD19;
}

VOID
Sha256Update (
  SHA256_CONTEXT *Context,
  CONST UINT8    *Data,
  UINTN          Len
  )
{
  UINT32 Index;

  for (Index = 0; Index < Len; Index++) {
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
  // Append to the padding the total message's length in bits and transform.
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
  for (Index = 0; Index < 4; Index++) {
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
  UINT8  *Hash,
  UINT8  *Data,
  UINTN  Len
  )
{
  SHA256_CONTEXT  Ctx;

  Sha256Init (&Ctx);
  Sha256Update (&Ctx, Data, Len);
  Sha256Final (&Ctx, Hash);
}

