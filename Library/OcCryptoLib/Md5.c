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
* Filename:   md5.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the MD5 hashing algorithm.
          Algorithm specification can be found here:
           * http://tools.ietf.org/html/rfc1321
          This implementation uses little endian byte order.
*********************************************************************/

#include <Library/BaseMemoryLib.h>
#include <Library/OcCryptoLib.h>

#define ROTLEFT(A,B) ((A << B) | (A >> (32 - B)))

#define F(X,Y,Z) ((X & Y) | (~X & Z))
#define G(X,Y,Z) ((X & Z) | (Y & ~Z))
#define H(X,Y,Z) (X ^ Y ^ Z)
#define I(X,Y,Z) (Y ^ (X | ~Z))

#define FF(A,B,C,D,M,S,T) do { A += F(B,C,D) + M + T; \
                            A = B + ROTLEFT(A,S); } while (0)
#define GG(A,B,C,D,M,S,T) do { A += G(B,C,D) + M + T; \
                            A = B + ROTLEFT(A,S); } while (0)
#define HH(A,B,C,D,M,S,T) do { A += H(B,C,D) + M + T; \
                            A = B + ROTLEFT(A,S); } while (0)
#define II(A,B,C,D,M,S,T) do { A += I(B,C,D) + M + T; \
                            A = B + ROTLEFT(A,S); } while (0)
VOID
Md5Transform (
  MD5_CONTEXT  *Ctx,
  CONST UINT8  *Data
  )
{
  UINT32 A, B, C, D, M[16], Index1, Index2;

  //
  // MD5 specifies big endian byte order, but this implementation assumes a little
  // endian byte order CPU. Reverse all the bytes upon input, and re-reverse them
  // on output (in md5_final()).
  //
  for (Index1 = 0, Index2 = 0; Index1 < 16; ++Index1, Index2 += 4) {
    M[Index1] = (Data[Index2]) + (Data[Index2 + 1] << 8)
                + (Data[Index2 + 2] << 16) + (Data[Index2 + 3] << 24);
  }
  A = Ctx->State[0];
  B = Ctx->State[1];
  C = Ctx->State[2];
  D = Ctx->State[3];

  FF (A, B, C, D, M[0],   7, 0xD76AA478);
  FF (D, A, B, C, M[1],  12, 0xE8C7B756);
  FF (C, D, A, B, M[2],  17, 0x242070DB);
  FF (B, C, D, A, M[3],  22, 0xC1BDCEEE);
  FF (A, B, C, D, M[4],   7, 0xF57C0FAF);
  FF (D, A, B, C, M[5],  12, 0x4787C62A);
  FF (C, D, A, B, M[6],  17, 0xA8304613);
  FF (B, C, D, A, M[7],  22, 0xFD469501);
  FF (A, B, C, D, M[8],   7, 0x698098D8);
  FF (D, A, B, C, M[9],  12, 0x8B44F7AF);
  FF (C, D, A, B, M[10], 17, 0xFFFF5BB1);
  FF (B, C, D, A, M[11], 22, 0x895CD7BE);
  FF (A, B, C, D, M[12],  7, 0x6B901122);
  FF (D, A, B, C, M[13], 12, 0xFD987193);
  FF (C, D, A, B, M[14], 17, 0xA679438E);
  FF (B, C, D, A, M[15], 22, 0x49B40821);

  GG (A, B, C, D, M[1],   5, 0xF61E2562);
  GG (D, A, B, C, M[6],   9, 0xC040B340);
  GG (C, D, A, B, M[11], 14, 0x265E5A51);
  GG (B, C, D, A, M[0],  20, 0xE9B6C7AA);
  GG (A, B, C, D, M[5],   5, 0xD62F105D);
  GG (D, A, B, C, M[10],  9, 0x02441453);
  GG (C, D, A, B, M[15], 14, 0xD8A1E681);
  GG (B, C, D, A, M[4],  20, 0xE7D3FBC8);
  GG (A, B, C, D, M[9],   5, 0x21E1CDE6);
  GG (D, A, B, C, M[14],  9, 0xC33707D6);
  GG (C, D, A, B, M[3],  14, 0xF4D50D87);
  GG (B, C, D, A, M[8],  20, 0x455A14ED);
  GG (A, B, C, D, M[13],  5, 0xA9E3E905);
  GG (D, A, B, C, M[2],   9, 0xFCEFA3F8);
  GG (C, D, A, B, M[7],  14, 0x676F02D9);
  GG (B, C, D, A, M[12], 20, 0x8D2A4C8A);

  HH (A, B, C, D, M[5],   4, 0xFFFA3942);
  HH (D, A, B, C, M[8],  11, 0x8771F681);
  HH (C, D, A, B, M[11], 16, 0x6D9D6122);
  HH (B, C, D, A, M[14], 23, 0xFDE5380C);
  HH (A, B, C, D, M[1],   4, 0xA4BEEA44);
  HH (D, A, B, C, M[4],  11, 0x4BDECFA9);
  HH (C, D, A, B, M[7],  16, 0xF6BB4B60);
  HH (B, C, D, A, M[10], 23, 0xBEBFBC70);
  HH (A, B, C, D, M[13],  4, 0x289B7EC6);
  HH (D, A, B, C, M[0],  11, 0xEAA127FA);
  HH (C, D, A, B, M[3],  16, 0xD4EF3085);
  HH (B, C, D, A, M[6],  23, 0x04881D05);
  HH (A, B, C, D, M[9],   4, 0xD9D4D039);
  HH (D, A, B, C, M[12], 11, 0xE6DB99E5);
  HH (C, D, A, B, M[15], 16, 0x1FA27CF8);
  HH (B, C, D, A, M[2],  23, 0xC4AC5665);

  II (A, B, C, D, M[0],   6, 0xF4292244);
  II (D, A, B, C, M[7],  10, 0x432AFF97);
  II (C, D, A, B, M[14], 15, 0xAB9423A7);
  II (B, C, D, A, M[5],  21, 0xFC93A039);
  II (A, B, C, D, M[12],  6, 0x655B59C3);
  II (D, A, B, C, M[3],  10, 0x8F0CCC92);
  II (C, D, A, B, M[10], 15, 0xFFEFF47D);
  II (B, C, D, A, M[1],  21, 0x85845DD1);
  II (A, B, C, D, M[8],   6, 0x6FA87E4F);
  II (D, A, B, C, M[15], 10, 0xFE2CE6E0);
  II (C, D, A, B, M[6],  15, 0xA3014314);
  II (B, C, D, A, M[13], 21, 0x4E0811A1);
  II (A, B, C, D, M[4],   6, 0xF7537E82);
  II (D, A, B, C, M[11], 10, 0xBD3AF235);
  II (C, D, A, B, M[2],  15, 0x2AD7D2BB);
  II (B, C, D, A, M[9],  21, 0xEB86D391);

  Ctx->State[0] += A;
  Ctx->State[1] += B;
  Ctx->State[2] += C;
  Ctx->State[3] += D;
}

VOID
Md5Init (
  MD5_CONTEXT  *Ctx
  )
{
  Ctx->DataLen = 0;
  Ctx->BitLen = 0;
  Ctx->State[0] = 0x67452301;
  Ctx->State[1] = 0xEFCDAB89;
  Ctx->State[2] = 0x98BADCFE;
  Ctx->State[3] = 0x10325476;
}

VOID
Md5Update (
  MD5_CONTEXT  *Ctx,
  CONST UINT8  *Data,
  UINTN        Len
  )
{
  UINTN Index;

  for (Index = 0; Index < Len; ++Index) {
    Ctx->Data[Ctx->DataLen] = Data[Index];
    Ctx->DataLen++;
    if (Ctx->DataLen == 64) {
      Md5Transform (Ctx, Ctx->Data);
      Ctx->BitLen += 512;
      Ctx->DataLen = 0;
    }
  }
}

VOID
Md5Final (
  MD5_CONTEXT  *Ctx,
  UINT8        *Hash
  )
{
  UINTN Index = Ctx->DataLen;

  //
  // Pad whatever Data is left in the buffer.
  //
  if (Ctx->DataLen < 56) {
    Ctx->Data[Index++] = 0x80;
    ZeroMem (Ctx->Data + Index, 56-Index);
  } else if (Ctx->DataLen >= 56) {
    Ctx->Data[Index++] = 0x80;
    ZeroMem (Ctx->Data + Index, 64-Index);
    Md5Transform (Ctx, Ctx->Data);
    ZeroMem (Ctx->Data, 56);
  }

  //
  // Append to the padding the total message's length in bits and transform.
  //
  Ctx->BitLen += Ctx->DataLen * 8;
  Ctx->Data[56] = (UINT8) (Ctx->BitLen);
  Ctx->Data[57] = (UINT8) (Ctx->BitLen >> 8);
  Ctx->Data[58] = (UINT8) (Ctx->BitLen >> 16);
  Ctx->Data[59] = (UINT8) (Ctx->BitLen >> 24);
  Ctx->Data[60] = (UINT8) (Ctx->BitLen >> 32);
  Ctx->Data[61] = (UINT8) (Ctx->BitLen >> 40);
  Ctx->Data[62] = (UINT8) (Ctx->BitLen >> 48);
  Ctx->Data[63] = (UINT8) (Ctx->BitLen >> 56);
  Md5Transform (Ctx, Ctx->Data);

  //
  // Since this implementation uses little endian byte ordering and MD uses big endian,
  // reverse all the bytes when copying the final State to the output Hash.
  //
  for (Index = 0; Index < 4; ++Index) {
    Hash[Index]      = (UINT8) ((Ctx->State[0] >> (Index * 8)) & 0x000000FF);
    Hash[Index + 4]  = (UINT8) ((Ctx->State[1] >> (Index * 8)) & 0x000000FF);
    Hash[Index + 8]  = (UINT8) ((Ctx->State[2] >> (Index * 8)) & 0x000000FF);
    Hash[Index + 12] = (UINT8) ((Ctx->State[3] >> (Index * 8)) & 0x000000FF);
  }
}

VOID
Md5 (
  UINT8  *Hash,
  UINT8  *Data,
  UINTN  Len
  )
{
  MD5_CONTEXT Ctx;

  Md5Init (&Ctx);
  Md5Update (&Ctx, Data, Len);
  Md5Final (&Ctx,Hash);
  SecureZeroMem (&Ctx, sizeof (Ctx));
}
