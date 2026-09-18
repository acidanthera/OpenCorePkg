/** @file
  Copyright (c) 2013, Alexey Degtyarev <alexey@renatasystems.org>. All rights reserved.<BR>
  Copyright (c) 2022 Maxim Kuznetsov. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifdef OC_CRYPTO_SUPPORTS_STREEBOG

#include "CryptoInternal.h"
#include "Streebog.h"

#define BSWAP64(x) \
    (((x & 0xFF00000000000000ULL) >> 56) | \
     ((x & 0x00FF000000000000ULL) >> 40) | \
     ((x & 0x0000FF0000000000ULL) >> 24) | \
     ((x & 0x000000FF00000000ULL) >>  8) | \
     ((x & 0x00000000FF000000ULL) <<  8) | \
     ((x & 0x0000000000FF0000ULL) << 24) | \
     ((x & 0x000000000000FF00ULL) << 40) | \
     ((x & 0x00000000000000FFULL) << 56))

STATIC
VOID
GOST34112012Cleanup (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < 64; ++Index) {
    Context->buffer[Index] = 0;
  }

  for (Index = 0; Index < STREEBOG_QWORD_COUNT; ++Index) {
    Context->hash.QWORD[Index]  = 0;
    Context->h.QWORD[Index]     = 0;
    Context->N.QWORD[Index]     = 0;
    Context->Sigma.QWORD[Index] = 0;
  }

  Context->bufsize     = 0;
  Context->digest_size = 0;
}

STATIC
VOID
GOST34112012Init (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT32          digest_size
  )
{
  UINTN  Index;

  GOST34112012Cleanup (Context);
  Context->digest_size = digest_size;

  for (Index = 0; Index < STREEBOG_QWORD_COUNT; Index++) {
    if (digest_size == 256) {
      Context->h.QWORD[Index] = 0x0101010101010101ULL;
    } else {
      Context->h.QWORD[Index] = 0x00ULL;
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

  if (Context->bufsize > 63) {
    return;
  }

  for (Index = 0; Index < sizeof (Context->buffer) - Context->bufsize; ++Index) {
    Context->buffer[Context->bufsize + Index] = 0;
  }

  Context->buffer[Context->bufsize] = 0x01;
}

STATIC
VOID
Add512 (
  IN  CONST UINT512  *x,
  IN  CONST UINT512  *y,
  OUT UINT512        *r
  )
{
  UINTN  Index;

 #if STREEBOG_LITTLE_ENDIAN
  UINT32  CF;

  CF = 0;
  for (Index = 0; Index < STREEBOG_QWORD_COUNT; Index++) {
    CONST UINT64  left = x->QWORD[Index];
    UINT64        sum;

    sum = left + y->QWORD[Index] + CF;
    if (sum != left) {
      CF = (sum < left);
    }

    r->QWORD[Index] = sum;
  }

 #else // STREEBOG_BIG_ENDIAN
  CONST UINT8  *xp, *yp;
  UINT8        *rp;
  INT32        buf;

  xp = (CONST UINT8 *)x;
  yp = (CONST UINT8 *)y;
  rp = (UINT8 *)r;

  buf = 0;
  for (Index = 0; Index < STREEBOG_BYTE_COUNT; Index++) {
    buf       = xp[Index] + yp[Index] + (buf >> 8);
    rp[Index] = (UINT8)buf & 0xFF;
  }

 #endif
}

STATIC
VOID
g (
  IN OUT UINT512    *h,
  IN CONST UINT512  *N,
  IN CONST UINT8    *m
  )
{
  UINT512  Ki, data;
  UINTN    Index;

  XLPS (h, N, (&data));

  Ki = data;
  XLPS ((&Ki), ((CONST UINT512 *)&m[0]), (&data));

  for (Index = 0; Index < 11; Index++) {
    ROUND (Index, (&Ki), (&data));
  }

  XLPS ((&Ki), (&C[11]), (&Ki));
  X ((&Ki), (&data), (&data));

  X ((&data), h, (&data));
  X ((&data), ((CONST UINT512 *)&m[0]), h);
}

STATIC
VOID
MasCpy (
  OUT UINT8       *To,
  IN CONST UINT8  *From
  )
{
  UINTN  Index;

  for (Index = 0; Index < 64; ++Index) {
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
  UINT512  m;

  MasCpy ((UINT8 *)&m, Data);
  g (&(Context->h), &(Context->N), (CONST UINT8 *)&m);

  Add512 (&(Context->N), &buffer512, &(Context->N));
  Add512 (&(Context->Sigma), &m, &(Context->Sigma));
}

STATIC
VOID
Stage3 (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  UINT512  buf = {
    { 0 }
  };

 #if STREEBOG_LITTLE_ENDIAN
  buf.QWORD[0] = Context->bufsize << 3;
 #else // STREEBOG_BIG_ENDIAN
  buf.QWORD[0] = BSWAP64 (Context->bufsize << 3);
 #endif

  Pad (Context);

  g (&Context->h, &Context->N, (CONST UINT8 *)Context->buffer);

  Add512 (&(Context->N), &buf, &(Context->N));
  Add512 (
    &(Context->Sigma),
    (CONST UINT512 *)&Context->buffer[0],
    &(Context->Sigma)
    );

  g (&Context->h, &buffer0, (CONST UINT8 *)&Context->N);

  g (&Context->h, &buffer0, (CONST UINT8 *)&Context->Sigma);
  Uint512uCpy (&(Context->hash), &(Context->h));
}

STATIC
VOID
GOST34112012Update (
  IN OUT STREEBOG_CONTEXT  *Context,
  IN CONST UINT8           *Data,
  IN UINT32                Length
  )
{
  UINT32  chunksize;
  UINTN   Index;

  if (Context->bufsize) {
    chunksize = 64 - Context->bufsize;
    if (chunksize > Length) {
      chunksize = Length;
    }

    for (Index = 0; Index < chunksize; ++Index) {
      Context->buffer[Context->bufsize + Index] = Data[Index];
    }

    Context->bufsize += chunksize;
    Length           -= chunksize;
    Data             += chunksize;

    if (Context->bufsize == 64) {
      Stage2 (Context, Context->buffer);

      Context->bufsize = 0;
    }
  }

  while (Length > 63) {
    Stage2 (Context, Data);

    Data   += 64;
    Length -= 64;
  }

  if (Length) {
    for (Index = 0; Index < Length; ++Index) {
      Context->buffer[Index] = Data[Index];
    }

    Context->bufsize = Length;
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

  Context->bufsize = 0;

  if (Context->digest_size == 256) {
    for (Index = 0; Index < 32; ++Index) {
      Digest[Index] = ((UINT8 *)&(Context->hash.QWORD[4]))[Index];
    }
  } else {
    for (Index = 0; Index < 64; ++Index) {
      Digest[Index] = ((UINT8 *)&(Context->hash.QWORD[0]))[Index];
    }
  }
}

VOID
Streebog256Init (
  IN OUT STREEBOG_CONTEXT  *Context
  )
{
  GOST34112012Init (Context, 256);
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
  GOST34112012Init (Context, 512);
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
