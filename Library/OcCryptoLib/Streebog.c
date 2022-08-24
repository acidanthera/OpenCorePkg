/** @file
  Copyright (c) 2013, Alexey Degtyarev <alexey@renatasystems.org>. All rights reserved.<BR>
  Copyright (c) 2022 Maxim Kuznetsov. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#include "BigNumLib.h"
#ifdef OC_CRYPTO_SUPPORTS_STREEBOG

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
  StreebogContext  *CTX
  )
{
  for (INT32 i = 0; i < 64; ++i) {
    CTX->buffer[i] = 0;
  }

  for (INT32 i = 0; i < 8; ++i) {
    CTX->hash.QWORD[i]  = 0;
    CTX->h.QWORD[i]     = 0;
    CTX->N.QWORD[i]     = 0;
    CTX->Sigma.QWORD[i] = 0;
  }

  CTX->bufsize     = 0;
  CTX->digest_size = 0;
}

VOID
GOST34112012Init (
  StreebogContext  *CTX,
  CONST UINT32     digest_size
  )
{
  UINT32  i;

  GOST34112012Cleanup (CTX);
  CTX->digest_size = digest_size;

  for (i = 0; i < 8; i++) {
    if (digest_size == 256) {
      CTX->h.QWORD[i] = 0x0101010101010101ULL;
    } else {
      CTX->h.QWORD[i] = 0x00ULL;
    }
  }
}

STATIC
VOID
Pad (
  StreebogContext  *CTX
  )
{
  if (CTX->bufsize > 63) {
    return;
  }

  for (UINT32 i = 0; i < sizeof (CTX->buffer) - CTX->bufsize; ++i) {
    CTX->buffer[CTX->bufsize + i] = 0;
  }

  CTX->buffer[CTX->bufsize] = 0x01;
}

STATIC
VOID
Add512 (
  CONST union uint512_u  *x,
  CONST union uint512_u  *y,
  union uint512_u        *r
  )
{
 #if defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
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
  union uint512_u        *h,
  CONST union uint512_u  *N,
  CONST UINT8            *m
  )
{
  union uint512_u  Ki, data;
  UINT32           i;

  XLPS (h, N, (&data));

  Ki = data;
  XLPS ((&Ki), ((const union uint512_u *)&m[0]), (&data));

  for (i = 0; i < 11; i++) {
    ROUND (i, (&Ki), (&data));
  }

  XLPS ((&Ki), (&C[11]), (&Ki));
  X ((&Ki), (&data), (&data));

  X ((&data), h, (&data));
  X ((&data), ((const union uint512_u *)&m[0]), h);
}

STATIC
VOID
MasCpy (
  UINT8        *to,
  CONST UINT8  *from
  )
{
  for (INT32 i = 0; i < 64; ++i) {
    to[i] = from[i];
  }
}

STATIC
VOID
Uint512uCpy (
  union uint512_u        *to,
  CONST union uint512_u  *from
  )
{
  for (INT32 i = 0; i < 8; ++i) {
    to->QWORD[i] = from->QWORD[i];
  }
}

STATIC
VOID
Stage2 (
  StreebogContext  *CTX,
  CONST UINT8      *data
  )
{
  union uint512_u  m;

  MasCpy ((UINT8 *)&m, data);
  g (&(CTX->h), &(CTX->N), (CONST UINT8 *)&m);

  Add512 (&(CTX->N), &buffer512, &(CTX->N));
  Add512 (&(CTX->Sigma), &m, &(CTX->Sigma));
}

STATIC
VOID
Stage3 (
  StreebogContext  *CTX
  )
{
  union uint512_u  buf = {
    { 0 }
  };

 #if defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  buf.QWORD[0] = CTX->bufsize << 3;
 #elif defined (__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  buf.QWORD[0] = BSWAP64 (CTX->bufsize << 3);
 #else
   #error Byte order is undefined
 #endif 

  Pad (CTX);

  g (&(CTX->h), &(CTX->N), (const unsigned char *)&(CTX->buffer));

  Add512 (&(CTX->N), &buf, &(CTX->N));
  Add512 (
    &(CTX->Sigma),
    (const union uint512_u *)&CTX->buffer[0],
    &(CTX->Sigma)
    );

  g (&(CTX->h), &buffer0, (const unsigned char *)&(CTX->N));

  g (&(CTX->h), &buffer0, (const unsigned char *)&(CTX->Sigma));
  Uint512uCpy (&(CTX->hash), &(CTX->h));
}

VOID
GOST34112012Update (
  StreebogContext  *CTX,
  CONST UINT8      *data,
  UINT32           len
  )
{
  UINT32  chunksize;

  if (CTX->bufsize) {
    chunksize = 64 - CTX->bufsize;
    if (chunksize > len) {
      chunksize = len;
    }

    for (UINT32 i = 0; i < chunksize; ++i) {
      ((UINT8 *)(&(CTX->buffer[CTX->bufsize])))[i] = data[i];
    }

    CTX->bufsize += chunksize;
    len          -= chunksize;
    data         += chunksize;

    if (CTX->bufsize == 64) {
      Stage2 (CTX, CTX->buffer);

      CTX->bufsize = 0;
    }
  }

  while (len > 63) {
    Stage2 (CTX, data);

    data += 64;
    len  -= 64;
  }

  if (len) {
    for (UINT32 i = 0; i < len; ++i) {
      ((UINT8 *)(&CTX->buffer))[i] = data[i];
    }

    CTX->bufsize = len;
  }
}

VOID
GOST34112012Final (
  StreebogContext  *CTX,
  UINT8            *digest
  )
{
  Stage3 (CTX);

  CTX->bufsize = 0;

  if (CTX->digest_size == 256) {
    for (INT32 i = 0; i < 32; ++i) {
      digest[i] = ((UINT8 *)&(CTX->hash.QWORD[4]))[i];
    }
  } else {
    for (INT32 i = 0; i < 64; ++i) {
      digest[i] = ((UINT8 *)&(CTX->hash.QWORD[0]))[i];
    }
  }
}

VOID
Streebog256Init (
  StreebogContext  *CTX
  )
{
  GOST34112012Init (CTX, 256);
}

VOID
Streebog256Update (
  StreebogContext  *CTX,
  CONST UINT8      *data,
  UINT32           len
  )
{
  GOST34112012Update (CTX, data, len);
}

VOID
Streebog256Final (
  StreebogContext  *CTX,
  UINT8            *digest
  )
{
  GOST34112012Final (CTX, digest);
}

VOID
Streebog512Init (
  StreebogContext  *CTX
  )
{
  GOST34112012Init (CTX, 512);
}

VOID
Streebog512Update (
  StreebogContext  *CTX,
  CONST UINT8      *data,
  UINT32           len
  )
{
  GOST34112012Update (CTX, data, len);
}

VOID
Streebog512Final (
  StreebogContext  *CTX,
  UINT8            *digest
  )
{
  GOST34112012Final (CTX, digest);
}

VOID
Streebog256 (
  CONST UINT8  *data,
  UINT32       len,
  UINT8        *digest
  )
{
  StreebogContext CTX;

  Streebog256Init (&CTX);
  Streebog256Update (&CTX, data, len);
  Streebog256Final (&CTX, digest);
}

VOID
Streebog512 (
  CONST UINT8  *data,
  UINT32       len,
  UINT8        *digest
  )
{
  StreebogContext CTX;

  Streebog512Init (&CTX);
  Streebog512Update (&CTX, data, len);
  Streebog512Final (&CTX, digest);
}

#endif
