/*
 * Copyright (c) 2013, Alexey Degtyarev <alexey@renatasystems.org>. 
 * All rights reserved.
 *
 * GOST R 34.11-2012 core and API functions.
 *
 * $Id$
 */

#include "gost3411-2012-core.h"

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
GOST34112012Cleanup(
  GOST34112012Context *CTX
  )
{
    for (INT32 i = 0; i < 64; ++i)
        CTX->buffer[i] = 0;
    for (INT32 i = 0; i < 8; ++i) {
        CTX->hash.QWORD[i] = 0;
        CTX->h.QWORD[i] = 0;
        CTX->N.QWORD[i] = 0;
        CTX->Sigma.QWORD[i] = 0;
    }
    CTX->bufsize = 0;
    CTX->digest_size = 0;
}

VOID
GOST34112012Init(
  GOST34112012Context *CTX, 
  CONST UINT32 digest_size
  )
{
    unsigned int i;

    GOST34112012Cleanup(CTX);
    CTX->digest_size = digest_size;

    for (i = 0; i < 8; i++)
    {
        if (digest_size == 256)
            CTX->h.QWORD[i] = 0x0101010101010101ULL;
        else
            CTX->h.QWORD[i] = 0x00ULL;
    }
}

static inline void
pad(
  GOST34112012Context *CTX
  )
{
    if (CTX->bufsize > 63)
        return;

    //memset(CTX->buffer + CTX->bufsize,
    //   0x00, sizeof(CTX->buffer) - CTX->bufsize);

    for (int i = 0; i < sizeof(CTX->buffer) - CTX->bufsize; ++i)
        CTX->buffer[CTX->bufsize + i] = 0;

    CTX->buffer[CTX->bufsize] = 0x01;
}

static inline void
add512(
  CONST union uint512_u *x, 
  CONST union uint512_u *y, 
  union uint512_u *r
  )
{
#ifndef __GOST3411_BIG_ENDIAN__
    UINT32 CF;
    UINT32 i;

    CF = 0;
    for (i = 0; i < 8; i++)
    {
        CONST UINT64 left = x->QWORD[i];
        UINT64 sum;

        sum = left + y->QWORD[i] + CF;
        if (sum != left)
            CF = (sum < left);
        r->QWORD[i] = sum;
    }
#else
    CONST UINT8 *xp, *yp;
    UINT8 *rp;
    UINT32 i;
    INT32 buf;

    xp = (CONST UINT8 *) &x[0];
    yp = (CONST UINT8 *) &y[0];
    rp = (UINT8 *) &r[0];

    buf = 0;
    for (i = 0; i < 64; i++)
    {
        buf = xp[i] + yp[i] + (buf >> 8);
        rp[i] = (UINT8) buf & 0xFF;
    }
#endif
}

static void
g(
  union uint512_u *h, 
  CONST union uint512_u *N, 
  CONST UINT8 *m)
{
    union uint512_u Ki, data;
    UINT32 i;

    XLPS(h, N, (&data));

    /* Starting E() */
    Ki = data;
    XLPS((&Ki), ((const union uint512_u *) &m[0]), (&data));

    for (i = 0; i < 11; i++)
        ROUND(i, (&Ki), (&data));

    XLPS((&Ki), (&C[11]), (&Ki));
    X((&Ki), (&data), (&data));
    /* E() done */

    X((&data), h, (&data));
    X((&data), ((const union uint512_u *) &m[0]), h);
}

void masCpy(
  UINT8 *to, 
  CONST UINT8 *from
  )
{
    for (INT32 i = 0; i < 64; ++i)
        to[i] = from[i];
}

void uint512uCpy(
  union uint512_u *to, 
  CONST union uint512_u *from
  )
{
    for (INT32 i = 0; i < 8; ++i)
        to->QWORD[i] = from->QWORD[i];
}

static inline void
stage2(
  GOST34112012Context *CTX, 
  CONST UINT8 *data
  )
{
    union uint512_u m;

    masCpy((UINT8 *)&m, data);
    g(&(CTX->h), &(CTX->N), (CONST UINT8 *)&m);

    add512(&(CTX->N), &buffer512, &(CTX->N));
    add512(&(CTX->Sigma), &m, &(CTX->Sigma));
}

static inline void
stage3(
  GOST34112012Context *CTX
  )
{
    union uint512_u buf = {{ 0 }};

#ifndef __GOST3411_BIG_ENDIAN__
    buf.QWORD[0] = CTX->bufsize << 3;
#else
    buf.QWORD[0] = BSWAP64(CTX->bufsize << 3);
#endif

    pad(CTX);

    g(&(CTX->h), &(CTX->N), (const unsigned char *) &(CTX->buffer));

    add512(&(CTX->N), &buf, &(CTX->N));
    add512(&(CTX->Sigma), (const union uint512_u *) &CTX->buffer[0],
           &(CTX->Sigma));

    g(&(CTX->h), &buffer0, (const unsigned char *) &(CTX->N));

    g(&(CTX->h), &buffer0, (const unsigned char *) &(CTX->Sigma));
    //memcpy(&(CTX->hash), &(CTX->h), sizeof uint512_u);
    uint512uCpy(&(CTX->hash), &(CTX->h));
}

VOID
GOST34112012Update(
  GOST34112012Context *CTX, 
  CONST UINT8 *data, 
  UINT32 len)
{
    UINT32 chunksize;

    if (CTX->bufsize) {
        chunksize = 64 - CTX->bufsize;
        if (chunksize > len)
            chunksize = len;

        //memcpy(&CTX->buffer[CTX->bufsize], data, chunksize);
        for (int i = 0; i < chunksize; ++i)
            ((UINT8 *)(&(CTX->buffer[CTX->bufsize])))[i] = data[i];

        CTX->bufsize += chunksize;
        len -= chunksize;
        data += chunksize;
        
        if (CTX->bufsize == 64)
        {
            stage2(CTX, CTX->buffer);

            CTX->bufsize = 0;
        }
    }

    while (len > 63)
    {
        stage2(CTX, data);

        data += 64;
        len  -= 64;
    }

    if (len) {
        //memcpy(&CTX->buffer, data, len);
        for (INT32 i = 0; i < len; ++i)
            ((UINT8 *)(&CTX->buffer))[i] = data[i];
        CTX->bufsize = len;
    }
}

VOID
GOST34112012Final(
  GOST34112012Context *CTX, 
  UINT8 *digest
  )
{
    stage3(CTX);

    CTX->bufsize = 0;

    if (CTX->digest_size == 256)
        //memcpy(digest, &(CTX->hash.QWORD[4]), 32);
        for (INT32 i = 0; i < 32; ++i)
            digest[i] = ((UINT8 *)&(CTX->hash.QWORD[4]))[i];
    else
        //memcpy(digest, &(CTX->hash.QWORD[0]), 64);
        for (INT32 i = 0; i < 64; ++i)
            digest[i] = ((UINT8 *)&(CTX->hash.QWORD[0]))[i];
}

VOID Streebog(
  CONST UINT8 *data, 
  UINT32 len, 
  UINT8 *digest, 
  UINT32 digest_size
  )
{
    GOST34112012Context CTX;
    GOST34112012Init(&CTX, digest_size);
    GOST34112012Update(&CTX, data, len);
    GOST34112012Final(&CTX, digest);
}
