/** @file

OcCryptoLib

Copyright (c) 2020, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/*
 chacha-merged.c version 20080118
 D. J. Bernstein
 Public domain.
 */

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcCryptoLib.h>

#define U32V(v) ((UINT32)(v) & 0xFFFFFFFFU)
#define ROTATE(v, c) (LRotU32 ((v), (c)))
#define XOR(v, w) ((v) ^ (w))
#define PLUS(v, w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v), 1))
#define LOAD32_LE(a) (ReadUnaligned32 ((UINT32 *)(a)))
#define STORE32_LE(a, v) (WriteUnaligned32 ((UINT32 *)(a), (v)))

#define QUARTERROUND(a, b, c, d) \
  a = PLUS(a, b);                \
  d = ROTATE(XOR(d, a), 16);     \
  c = PLUS(c, d);                \
  b = ROTATE(XOR(b, c), 12);     \
  a = PLUS(a, b);                \
  d = ROTATE(XOR(d, a), 8);      \
  c = PLUS(c, d);                \
  b = ROTATE(XOR(b, c), 7);

VOID
ChaChaInitCtx (
  OUT CHACHA_CONTEXT  *Context,
  IN  CONST UINT8     *Key,
  IN  CONST UINT8     *Iv,
  IN  UINT32          Counter
  )
{
  Context->Input[0]  = 0x61707865U;
  Context->Input[1]  = 0x3320646EU;
  Context->Input[2]  = 0x79622d32U;
  Context->Input[3]  = 0x6B206574U;
  Context->Input[4]  = LOAD32_LE (Key + 0);
  Context->Input[5]  = LOAD32_LE (Key + 4);
  Context->Input[6]  = LOAD32_LE (Key + 8);
  Context->Input[7]  = LOAD32_LE (Key + 12);
  Context->Input[8]  = LOAD32_LE (Key + 16);
  Context->Input[9]  = LOAD32_LE (Key + 20);
  Context->Input[10] = LOAD32_LE (Key + 24);
  Context->Input[11] = LOAD32_LE (Key + 28);
  Context->Input[12] = Counter;
  Context->Input[13] = LOAD32_LE (Iv + 0);
  Context->Input[14] = LOAD32_LE (Iv + 4);
  Context->Input[15] = LOAD32_LE (Iv + 8);
}

VOID
ChaChaCryptBuffer (
  IN OUT CHACHA_CONTEXT  *Context,
  IN     CONST UINT8     *Source,
     OUT UINT8           *Destination,
  IN     UINT32          Length
  )
{
  UINT32  X0;
  UINT32  X1;
  UINT32  X2;
  UINT32  X3;
  UINT32  X4;
  UINT32  X5;
  UINT32  X6;
  UINT32  X7;
  UINT32  X8;
  UINT32  X9;
  UINT32  X10;
  UINT32  X11;
  UINT32  X12;
  UINT32  X13;
  UINT32  X14;
  UINT32  X15;
  UINT32  J0;
  UINT32  J1;
  UINT32  J2;
  UINT32  J3;
  UINT32  J4;
  UINT32  J5;
  UINT32  J6;
  UINT32  J7;
  UINT32  J8;
  UINT32  J9;
  UINT32  J10;
  UINT32  J11;
  UINT32  J12;
  UINT32  J13;
  UINT32  J14;
  UINT32  J15;
  UINT8   *Ctarget;
  UINT8   Tmp[64];
  UINT32  Index;

  Ctarget = NULL;

  if (Length == 0) {
    return;
  }

  J0  = Context->Input[0];
  J1  = Context->Input[1];
  J2  = Context->Input[2];
  J3  = Context->Input[3];
  J4  = Context->Input[4];
  J5  = Context->Input[5];
  J6  = Context->Input[6];
  J7  = Context->Input[7];
  J8  = Context->Input[8];
  J9  = Context->Input[9];
  J10 = Context->Input[10];
  J11 = Context->Input[11];
  J12 = Context->Input[12];
  J13 = Context->Input[13];
  J14 = Context->Input[14];
  J15 = Context->Input[15];

  while (TRUE) {
    if (Length < sizeof (Tmp)) {
      CopyMem (Tmp, Source, Length);
      ZeroMem (Tmp + Length, sizeof (Tmp) - Length);
      Source      = Tmp;
      Ctarget     = Destination;
      Destination = Tmp;
    }

    X0  = J0;
    X1  = J1;
    X2  = J2;
    X3  = J3;
    X4  = J4;
    X5  = J5;
    X6  = J6;
    X7  = J7;
    X8  = J8;
    X9  = J9;
    X10 = J10;
    X11 = J11;
    X12 = J12;
    X13 = J13;
    X14 = J14;
    X15 = J15;

    for (Index = 20; Index > 0; Index -= 2) {
      QUARTERROUND (X0, X4, X8,  X12)
      QUARTERROUND (X1, X5, X9,  X13)
      QUARTERROUND (X2, X6, X10, X14)
      QUARTERROUND (X3, X7, X11, X15)
      QUARTERROUND (X0, X5, X10, X15)
      QUARTERROUND (X1, X6, X11, X12)
      QUARTERROUND (X2, X7, X8,  X13)
      QUARTERROUND (X3, X4, X9,  X14)
    }

    X0  = PLUS (X0,  J0);
    X1  = PLUS (X1,  J1);
    X2  = PLUS (X2,  J2);
    X3  = PLUS (X3,  J3);
    X4  = PLUS (X4,  J4);
    X5  = PLUS (X5,  J5);
    X6  = PLUS (X6,  J6);
    X7  = PLUS (X7,  J7);
    X8  = PLUS (X8,  J8);
    X9  = PLUS (X9,  J9);
    X10 = PLUS (X10, J10);
    X11 = PLUS (X11, J11);
    X12 = PLUS (X12, J12);
    X13 = PLUS (X13, J13);
    X14 = PLUS (X14, J14);
    X15 = PLUS (X15, J15);

    X0  = XOR (X0,  LOAD32_LE (Source + 0));
    X1  = XOR (X1,  LOAD32_LE (Source + 4));
    X2  = XOR (X2,  LOAD32_LE (Source + 8));
    X3  = XOR (X3,  LOAD32_LE (Source + 12));
    X4  = XOR (X4,  LOAD32_LE (Source + 16));
    X5  = XOR (X5,  LOAD32_LE (Source + 20));
    X6  = XOR (X6,  LOAD32_LE (Source + 24));
    X7  = XOR (X7,  LOAD32_LE (Source + 28));
    X8  = XOR (X8,  LOAD32_LE (Source + 32));
    X9  = XOR (X9,  LOAD32_LE (Source + 36));
    X10 = XOR (X10, LOAD32_LE (Source + 40));
    X11 = XOR (X11, LOAD32_LE (Source + 44));
    X12 = XOR (X12, LOAD32_LE (Source + 48));
    X13 = XOR (X13, LOAD32_LE (Source + 52));
    X14 = XOR (X14, LOAD32_LE (Source + 56));
    X15 = XOR (X15, LOAD32_LE (Source + 60));

    J12 = PLUSONE (J12);

    if (J12 == 0) {
      J13 = PLUSONE (J13);
    }

    STORE32_LE (Destination + 0,  X0);
    STORE32_LE (Destination + 4,  X1);
    STORE32_LE (Destination + 8,  X2);
    STORE32_LE (Destination + 12, X3);
    STORE32_LE (Destination + 16, X4);
    STORE32_LE (Destination + 20, X5);
    STORE32_LE (Destination + 24, X6);
    STORE32_LE (Destination + 28, X7);
    STORE32_LE (Destination + 32, X8);
    STORE32_LE (Destination + 36, X9);
    STORE32_LE (Destination + 40, X10);
    STORE32_LE (Destination + 44, X11);
    STORE32_LE (Destination + 48, X12);
    STORE32_LE (Destination + 52, X13);
    STORE32_LE (Destination + 56, X14);
    STORE32_LE (Destination + 60, X15);

    if (Length <= sizeof (Tmp)) {
      if (Length < sizeof (Tmp)) {
        CopyMem (Ctarget, Destination, Length);
      }

      Context->Input[12] = J12;
      Context->Input[13] = J13;

      return;
    }

    Length      -= sizeof (Tmp);
    Destination += sizeof (Tmp);
    Source      += sizeof (Tmp);
  }
}
