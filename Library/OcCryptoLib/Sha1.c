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

/*********************************************************************
* Filename:   sha1.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA1 hashing algorithm.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
*********************************************************************/

#include <Library/BaseMemoryLib.h>
#include <Library/OcCryptoLib.h>

#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

VOID
Sha1Transform (
  SHA1_CONTEXT *Ctx,
  CONST UINT8  *Data
  )
{
  UINT32 A, B, C, D, E, Index1, Index2, T, M[80];

  for (Index1 = 0, Index2 = 0; Index1 < 16; ++Index1, Index2 += 4) {
    M[Index1] = (Data[Index2] << 24) + (Data[Index2 + 1] << 16)
                + (Data[Index2 + 2] << 8) + (Data[Index2 + 3]);
  }

  for ( ; Index1 < 80; ++Index1) {
    M[Index1] = (M[Index1 - 3] ^ M[Index1 - 8] ^ M[Index1 - 14] ^ M[Index1 - 16]);
    M[Index1] = (M[Index1] << 1) | (M[Index1] >> 31);
  }

  A = Ctx->State[0];
  B = Ctx->State[1];
  C = Ctx->State[2];
  D = Ctx->State[3];
  E = Ctx->State[4];

  for (Index1 = 0; Index1 < 20; ++Index1) {
    T = ROTLEFT (A, 5) + ((B & C) ^ (~B & D)) + E + Ctx->K[0] + M[Index1];
    E = D;
    D = C;
    C = ROTLEFT (B, 30);
    B = A;
    A = T;
  }
  for ( ; Index1 < 40; ++Index1) {
    T = ROTLEFT (A, 5) + (B ^ C ^ D) + E + Ctx->K[1] + M[Index1];
    E = D;
    D = C;
    C = ROTLEFT (B, 30);
    B = A;
    A = T;
  }
  for ( ; Index1 < 60; ++Index1) {
    T = ROTLEFT (A, 5) + ((B & C) ^ (B & D) ^ (C & D))  + E + Ctx->K[2] + M[Index1];
    E = D;
    D = C;
    C = ROTLEFT (B, 30);
    B = A;
    A = T;
  }
  for ( ; Index1 < 80; ++Index1) {
    T = ROTLEFT (A, 5) + (B ^ C ^ D) + E + Ctx->K[3] + M[Index1];
    E = D;
    D = C;
    C = ROTLEFT (B, 30);
    B = A;
    A = T;
  }

  Ctx->State[0] += A;
  Ctx->State[1] += B;
  Ctx->State[2] += C;
  Ctx->State[3] += D;
  Ctx->State[4] += E;
}

VOID
Sha1Init (
  SHA1_CONTEXT  *Ctx
  )
{
  Ctx->DataLen = 0;
  Ctx->BitLen = 0;
  Ctx->State[0] = 0x67452301;
  Ctx->State[1] = 0xEFCDAB89;
  Ctx->State[2] = 0x98BADCFE;
  Ctx->State[3] = 0x10325476;
  Ctx->State[4] = 0xC3D2E1F0;
  Ctx->K[0] = 0x5A827999;
  Ctx->K[1] = 0x6ED9EBA1;
  Ctx->K[2] = 0x8F1BBCDC;
  Ctx->K[3] = 0xCA62C1D6;
}

VOID
Sha1Update (
  SHA1_CONTEXT *Ctx,
  CONST UINT8  *Data,
  UINTN        Len
  )
{
  UINTN Index = 0;

  for (Index = 0; Index < Len; ++Index) {
    Ctx->Data[Ctx->DataLen] = Data[Index];
    Ctx->DataLen++;
    if (Ctx->DataLen == 64) {
      Sha1Transform (Ctx, Ctx->Data);
      Ctx->BitLen += 512;
      Ctx->DataLen = 0;
    }
  }
}

VOID
Sha1Final (
  SHA1_CONTEXT  *Ctx,
  UINT8         *Hash
  )
{
  UINT32 Index = Ctx->DataLen;

  //
  // Pad whatever Data is left in the buffer.
  //
  if (Ctx->DataLen < 56) {
    Ctx->Data[Index++] = 0x80;
    ZeroMem (Ctx->Data + Index, 56-Index);
  } else {
    Ctx->Data[Index++] = 0x80;
    ZeroMem (Ctx->Data + Index, 64-Index);
    Sha1Transform (Ctx, Ctx->Data);
    ZeroMem (Ctx->Data, 56);
  }

  //
  // Append to the padding the total message's length in bits and transform.
  //
  Ctx->BitLen += Ctx->DataLen * 8;
  Ctx->Data[63] = (UINT8) (Ctx->BitLen);
  Ctx->Data[62] = (UINT8) (Ctx->BitLen >> 8);
  Ctx->Data[61] = (UINT8) (Ctx->BitLen >> 16);
  Ctx->Data[60] = (UINT8) (Ctx->BitLen >> 24);
  Ctx->Data[59] = (UINT8) (Ctx->BitLen >> 32);
  Ctx->Data[58] = (UINT8) (Ctx->BitLen >> 40);
  Ctx->Data[57] = (UINT8) (Ctx->BitLen >> 48);
  Ctx->Data[56] = (UINT8) (Ctx->BitLen >> 56);
  Sha1Transform (Ctx, Ctx->Data);

  //
  // Since this implementation uses little endian byte ordering and MD uses big endian,
  // reverse all the bytes when copying the final State to the output Hash.
  //
  for (Index = 0; Index < 4; ++Index) {
    Hash[Index]      = (UINT8) ((Ctx->State[0] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 4]  = (UINT8) ((Ctx->State[1] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 8]  = (UINT8) ((Ctx->State[2] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 12] = (UINT8) ((Ctx->State[3] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 16] = (UINT8) ((Ctx->State[4] >> (24 - Index * 8)) & 0x000000FF);
  }
}

VOID
Sha1 (
  UINT8  *Hash,
  UINT8  *Data,
  UINTN  Len
  )
{
  SHA1_CONTEXT Ctx;

  Sha1Init (&Ctx);
  Sha1Update (&Ctx, Data, Len);
  Sha1Final (&Ctx,Hash);
  SecureZeroMem (&Ctx, sizeof (Ctx));
}
