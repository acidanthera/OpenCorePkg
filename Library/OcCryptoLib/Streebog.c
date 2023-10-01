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

VOID
GOST34112012Cleanup (
  STREEBOG_CONTEXT  *Context
  )
{
  for (INT32 i = 0; i < 64; ++i) {
    Context->buffer[i] = 0;
  }

  for (INT32 i = 0; i < 8; ++i) {
    Context->hash.QWORD[i]  = 0;
    Context->h.QWORD[i]     = 0;
    Context->N.QWORD[i]     = 0;
    Context->Sigma.QWORD[i] = 0;
  }

  Context->bufsize     = 0;
  Context->digest_size = 0;
}

VOID
GOST34112012Init (
  STREEBOG_CONTEXT  *Context,
  CONST UINT32      digest_size
  )
{
  UINT32  i;

  GOST34112012Cleanup (Context);
  Context->digest_size = digest_size;

  for (i = 0; i < 8; i++) {
    if (digest_size == 256) {
      Context->h.QWORD[i] = 0x0101010101010101ULL;
    } else {
      Context->h.QWORD[i] = 0x00ULL;
    }
  }
}

STATIC
VOID
Pad (
  STREEBOG_CONTEXT  *Context
  )
{
  if (Context->bufsize > 63) {
    return;
  }

  for (UINT32 i = 0; i < sizeof (Context->buffer) - Context->bufsize; ++i) {
    Context->buffer[Context->bufsize + i] = 0;
  }

  Context->buffer[Context->bufsize] = 0x01;
}

STATIC
VOID
Add512 (
  CONST UINT512  *x,
  CONST UINT512  *y,
  UINT512        *r
  )
{
#if (defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
  UINT32  CF;
  UINT32  i;

  CF = 0;
  for (i = 0; i < 8; i++) {
    CONST UINT64  left = x->QWORD[i];
    UINT64        sum;

    sum = left + y->QWORD[i] + CF;
    if (sum != left) {
      CF = (sum < left);
    }

    r->QWORD[i] = sum;
  }

#elif defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  CONST UINT8  *xp, *yp;
  UINT8        *rp;
  UINT32       i;
  INT32        buf;

  xp = (CONST UINT8 *)&x[0];
  yp = (CONST UINT8 *)&y[0];
  rp = (UINT8 *)&r[0];

  buf = 0;
  for (i = 0; i < 64; i++) {
    buf   = xp[i] + yp[i] + (buf >> 8);
    rp[i] = (UINT8)buf & 0xFF;
  }

#else
  #error Byte order is undefined
#endif
}

STATIC
VOID
g (
  UINT512        *h,
  CONST UINT512  *N,
  CONST UINT8            *m
  )
{
  UINT512  Ki, data;
  UINT32           i;

  XLPS (h, N, (&data));

  Ki = data;
  XLPS ((&Ki), ((const UINT512 *)&m[0]), (&data));

  for (i = 0; i < 11; i++) {
    ROUND (i, (&Ki), (&data));
  }

  XLPS ((&Ki), (&C[11]), (&Ki));
  X ((&Ki), (&data), (&data));

  X ((&data), h, (&data));
  X ((&data), ((const UINT512 *)&m[0]), h);
}

STATIC
VOID
MasCpy (
  UINT8        *To,
  CONST UINT8  *From
  )
{
  for (INT32 i = 0; i < 64; ++i) {
    To[i] = From[i];
  }
}

STATIC
VOID
Uint512uCpy (
  UINT512        *to,
  CONST UINT512  *from
  )
{
  for (INT32 i = 0; i < 8; ++i) {
    to->QWORD[i] = from->QWORD[i];
  }
}

STATIC
VOID
Stage2 (
  STREEBOG_CONTEXT  *Context,
  CONST UINT8       *Data
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
  STREEBOG_CONTEXT  *Context
  )
{
  UINT512  buf = {
    { 0 }
  };

#if (defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
  buf.QWORD[0] = Context->bufsize << 3;
#elif defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  buf.QWORD[0] = BSWAP64 (Context->bufsize << 3);
#else
  #error Byte order is undefined
#endif

  Pad (Context);

  g (&(Context->h), &(Context->N), (const unsigned char *)&(Context->buffer));

  Add512 (&(Context->N), &buf, &(Context->N));
  Add512 (
    &(Context->Sigma),
    (const UINT512 *)&Context->buffer[0],
    &(Context->Sigma)
    );

  g (&(Context->h), &buffer0, (const unsigned char *)&(Context->N));

  g (&(Context->h), &buffer0, (const unsigned char *)&(Context->Sigma));
  Uint512uCpy (&(Context->hash), &(Context->h));
}

VOID
GOST34112012Update (
  STREEBOG_CONTEXT  *Context,
  CONST UINT8       *Data,
  UINT32            Length
  )
{
  UINT32  chunksize;

  if (Context->bufsize) {
    chunksize = 64 - Context->bufsize;
    if (chunksize > Length) {
      chunksize = Length;
    }

    for (UINT32 i = 0; i < chunksize; ++i) {
      ((UINT8 *)(&(Context->buffer[Context->bufsize])))[i] = Data[i];
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

    Data += 64;
    Length  -= 64;
  }

  if (Length) {
    for (UINT32 i = 0; i < Length; ++i) {
      ((UINT8 *)(&Context->buffer))[i] = Data[i];
    }

    Context->bufsize = Length;
  }
}

VOID
GOST34112012Final (
  STREEBOG_CONTEXT  *Context,
  UINT8            *Digest
  )
{
  Stage3 (Context);

  Context->bufsize = 0;

  if (Context->digest_size == 256) {
    for (INT32 i = 0; i < 32; ++i) {
      Digest[i] = ((UINT8 *)&(Context->hash.QWORD[4]))[i];
    }
  } else {
    for (INT32 i = 0; i < 64; ++i) {
      Digest[i] = ((UINT8 *)&(Context->hash.QWORD[0]))[i];
    }
  }
}

VOID
Streebog256Init (
  STREEBOG_CONTEXT  *Context
  )
{
  GOST34112012Init (Context, 256);
}

VOID
Streebog256Update (
  STREEBOG_CONTEXT  *Context,
  CONST UINT8       *Data,
  UINT32            Length
  )
{
  GOST34112012Update (Context, Data, Length);
}

VOID
Streebog256Final (
  STREEBOG_CONTEXT  *Context,
  UINT8            *Digest
  )
{
  GOST34112012Final (Context, Digest);
}

VOID
Streebog256 (
  CONST UINT8  *Data,
  UINT8        *Digest,
  UINT32       Length
  )
{
  STREEBOG_CONTEXT  Context;

  Streebog256Init (&Context);
  Streebog256Update (&Context, Data, Length);
  Streebog256Final (&Context, Digest);
  SecureZeroMem (&Context, sizeof(Context));
}

VOID
Streebog512Init (
  STREEBOG_CONTEXT  *Context
  )
{
  GOST34112012Init (Context, 512);
}

VOID
Streebog512Update (
  STREEBOG_CONTEXT  *Context,
  CONST UINT8      *Data,
  UINT32           Length
  )
{
  GOST34112012Update (Context, Data, Length);
}

VOID
Streebog512Final (
  STREEBOG_CONTEXT  *Context,
  UINT8            *Digest
  )
{
  GOST34112012Final (Context, Digest);
}

VOID
Streebog512 (
  CONST UINT8  *Data,
  UINT8        *Digest,
  UINT32       Length
  )
{
  STREEBOG_CONTEXT  Context;

  Streebog512Init (&Context);
  Streebog512Update (&Context, Data, Length);
  Streebog512Final (&Context, Digest);
  SecureZeroMem (&Context, sizeof(Context));
}

#endif
