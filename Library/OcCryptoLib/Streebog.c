/** @file
  Copyright (c) 2013, Alexey Degtyarev <alexey@renatasystems.org>. All rights reserved.<BR>
  Copyright (c) 2022 Maxim Kuznetsov. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifdef OC_CRYPTO_SUPPORTS_STREEBOG

#include "CryptoInternal.h"
#include "Streebog.h"

#define BSWAP64(X) \
    (((X & 0xFF00000000000000ULL) >> 56) | \
     ((X & 0x00FF000000000000ULL) >> 40) | \
     ((X & 0x0000FF0000000000ULL) >> 24) | \
     ((X & 0x000000FF00000000ULL) >>  8) | \
     ((X & 0x00000000FF000000ULL) <<  8) | \
     ((X & 0x0000000000FF0000ULL) << 24) | \
     ((X & 0x000000000000FF00ULL) << 40) | \
     ((X & 0x00000000000000FFULL) << 56))

STATIC
VOID
GOST34112012Cleanup (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < STREEBOG_BYTE_COUNT; ++Index) {
    Context->Buffer[Index] = 0;
  }

  for (Index = 0; Index < STREEBOG_QWORD_COUNT; ++Index) {
    Context->Hash.QWORD[Index]  = 0;
    Context->H.QWORD[Index]     = 0;
    Context->N.QWORD[Index]     = 0;
    Context->Sigma.QWORD[Index] = 0;
  }

  Context->BufSize    = 0;
  Context->DigestSize = 0;
}

STATIC
VOID
GOST34112012Init (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT32          DigestSize
  )
{
  UINTN  Index;

  GOST34112012Cleanup (Context);
  Context->DigestSize = DigestSize;

  for (Index = 0; Index < STREEBOG_QWORD_COUNT; Index++) {
    if (DigestSize == STREEBOG256_DIGEST_SIZE) {
      Context->H.QWORD[Index] = 0x0101010101010101ULL;
    } else {
      Context->H.QWORD[Index] = 0x00ULL;
    }
  }
}

STATIC
VOID
Pad (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  UINTN  Index;

  if (Context->BufSize > STREEBOG_BYTE_COUNT - 1) {
    return;
  }

  for (Index = 0; Index < sizeof (Context->Buffer) - Context->BufSize; ++Index) {
    Context->Buffer[Context->BufSize + Index] = 0;
  }

  Context->Buffer[Context->BufSize] = 0x01;
}

STATIC
VOID
Add512 (
  IN  CONST UINT512  *X,
  IN  CONST UINT512  *Y,
  OUT UINT512        *R
  )
{
  UINTN  Index;

 #if STREEBOG_LITTLE_ENDIAN
  UINT32  CF;

  CF = 0;
  for (Index = 0; Index < STREEBOG_QWORD_COUNT; Index++) {
    CONST UINT64  Left = X->QWORD[Index];
    UINT64        Sum;

    Sum = Left + Y->QWORD[Index] + CF;
    if (Sum != Left) {
      CF = (Sum < Left);
    }

    R->QWORD[Index] = Sum;
  }

 #else // STREEBOG_BIG_ENDIAN
  CONST UINT8  *Xp, *Yp;
  UINT8        *Rp;
  INT32        Buf;

  Xp = (CONST UINT8 *)X;
  Yp = (CONST UINT8 *)Y;
  Rp = (UINT8 *)R;

  Buf = 0;
  for (Index = 0; Index < STREEBOG_BYTE_COUNT; Index++) {
    Buf       = Xp[Index] + Yp[Index] + (Buf >> 8);
    Rp[Index] = (UINT8)Buf & 0xFF;
  }

 #endif
}

STATIC
VOID
G (
  IN OUT UINT512    *H,
  IN CONST UINT512  *N,
  IN CONST UINT8    *M
  )
{
  UINT512  Ki, Data;
  UINTN    Index;

  XLPS (H, N, (&Data));

  Ki = Data;
  XLPS ((&Ki), ((CONST UINT512 *)&M[0]), (&Data));

  for (Index = 0; Index < 11; Index++) {
    ROUND (Index, (&Ki), (&Data));
  }

  XLPS ((&Ki), (&C[11]), (&Ki));
  X ((&Ki), (&Data), (&Data));

  X ((&Data), H, (&Data));
  X ((&Data), ((CONST UINT512 *)&M[0]), H);
}

STATIC
VOID
MasCpy (
  OUT UINT8       *To,
  IN CONST UINT8  *From
  )
{
  UINTN  Index;

  for (Index = 0; Index < STREEBOG_BYTE_COUNT; ++Index) {
    To[Index] = From[Index];
  }
}

STATIC
VOID
Uint512uCpy (
  OUT UINT512       *To,
  IN CONST UINT512  *From
  )
{
  UINTN  Index;

  for (Index = 0; Index < STREEBOG_QWORD_COUNT; ++Index) {
    To->QWORD[Index] = From->QWORD[Index];
  }
}

STATIC
VOID
Stage2 (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT8           *Data
  )
{
  UINT512  M;

  MasCpy ((UINT8 *)&M, Data);
  G (&(Context->H), &(Context->N), (CONST UINT8 *)&M);

  Add512 (&(Context->N), &Buffer512, &(Context->N));
  Add512 (&(Context->Sigma), &M, &(Context->Sigma));
}

STATIC
VOID
Stage3 (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  UINT512  Buf = {
    { 0 }
  };

 #if STREEBOG_LITTLE_ENDIAN
  Buf.QWORD[0] = Context->BufSize << 3;
 #else // STREEBOG_BIG_ENDIAN
  Buf.QWORD[0] = BSWAP64 (Context->BufSize << 3);
 #endif

  Pad (Context);

  G (&Context->H, &Context->N, (CONST UINT8 *)Context->Buffer);

  Add512 (&(Context->N), &Buf, &(Context->N));
  Add512 (
    &(Context->Sigma),
    (CONST UINT512 *)&Context->Buffer[0],
    &(Context->Sigma)
    );

  G (&Context->H, &Buffer0, (CONST UINT8 *)&Context->N);

  G (&Context->H, &Buffer0, (CONST UINT8 *)&Context->Sigma);
  Uint512uCpy (&(Context->Hash), &(Context->H));
}

STATIC
VOID
GOST34112012Update (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT8           *Data,
  IN UINT32                Length
  )
{
  UINT32  ChunkSize;
  UINTN   Index;

  if (Context->BufSize) {
    ChunkSize = STREEBOG_BYTE_COUNT - Context->BufSize;
    if (ChunkSize > Length) {
      ChunkSize = Length;
    }

    for (Index = 0; Index < ChunkSize; ++Index) {
      Context->Buffer[Context->BufSize + Index] = Data[Index];
    }

    Context->BufSize += ChunkSize;
    Length           -= ChunkSize;
    Data             += ChunkSize;

    if (Context->BufSize == STREEBOG_BYTE_COUNT) {
      Stage2 (Context, Context->Buffer);

      Context->BufSize = 0;
    }
  }

  while (Length > STREEBOG_BYTE_COUNT - 1) {
    Stage2 (Context, Data);

    Data   += STREEBOG_BYTE_COUNT;
    Length -= STREEBOG_BYTE_COUNT;
  }

  if (Length) {
    for (Index = 0; Index < Length; ++Index) {
      Context->Buffer[Index] = Data[Index];
    }

    Context->BufSize = Length;
  }
}

STATIC
VOID
GOST34112012Final (
  IN OUT STREEBOG_CONTEXT  *Context,
  OUT UINT8                *Digest
  )
{
  UINTN  Index;

  Stage3 (Context);

  Context->BufSize = 0;

  if (Context->DigestSize == STREEBOG256_DIGEST_SIZE) {
    for (Index = 0; Index < STREEBOG_BYTE_COUNT / 2; ++Index) {
      Digest[Index] = ((UINT8 *)&(Context->Hash.QWORD[4]))[Index];
    }
  } else {
    for (Index = 0; Index < STREEBOG_BYTE_COUNT; ++Index) {
      Digest[Index] = ((UINT8 *)&(Context->Hash.QWORD[0]))[Index];
    }
  }
}

VOID
Streebog256Init (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  GOST34112012Init (Context, STREEBOG256_DIGEST_SIZE);
}

VOID
Streebog256Update (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT8           *Data,
  IN UINT32                Length
  )
{
  GOST34112012Update (Context, Data, Length);
}

VOID
Streebog256Final (
  IN OUT STREEBOG_CONTEXT  *Context,
  OUT UINT8                *Digest
  )
{
  GOST34112012Final (Context, Digest);
}

VOID
Streebog256 (
  IN CONST UINT8  *Data,
  OUT UINT8       *Digest,
  IN UINT32       Length
  )
{
  STREEBOG_CONTEXT  Context;

  Streebog256Init (&Context);
  Streebog256Update (&Context, Data, Length);
  Streebog256Final (&Context, Digest);
  SecureZeroMem (&Context, sizeof (Context));
}

VOID
Streebog512Init (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  GOST34112012Init (Context, STREEBOG512_DIGEST_SIZE);
}

VOID
Streebog512Update (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT8           *Data,
  IN UINT32                Length
  )
{
  GOST34112012Update (Context, Data, Length);
}

VOID
Streebog512Final (
  IN OUT STREEBOG_CONTEXT  *Context,
  OUT UINT8                *Digest
  )
{
  GOST34112012Final (Context, Digest);
}

VOID
Streebog512 (
  IN CONST UINT8  *Data,
  OUT UINT8       *Digest,
  IN UINT32       Length
  )
{
  STREEBOG_CONTEXT  Context;

  Streebog512Init (&Context);
  Streebog512Update (&Context, Data, Length);
  Streebog512Final (&Context, Digest);
  SecureZeroMem (&Context, sizeof (Context));
}

#endif
