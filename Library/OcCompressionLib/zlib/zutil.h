/* zutil.h -- internal interface and configuration of the compression library
 * Copyright (C) 1995-2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id$ */

#ifndef ZUTIL_H
#define ZUTIL_H

#define ZLIB_INTERNAL

#include "zlib.h"

#define local static

/* since "static" is used to mean two completely different things in C, we
   define "local" for the non-static meaning of "static", for readability
   (compile with -Dlocal if your debugger can't find static symbols) */

typedef unsigned char  uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long  ulg;

#define ERR_MSG(err) NULL

#define ERR_RETURN(strm,err) \
  return (strm->msg = NULL, (err))
/* To be used only when the state is known to be valid */

/* common constants */

#define DEF_WBITS MAX_WBITS
/* default windowBits for decompression. MAX_WBITS is for compression only */

#define DEF_MEM_LEVEL 8
/* default memLevel */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header */

/* target dependencies */

#define OS_CODE  3     /* assume Unix */

#define zmemcpy(Dst, Src, Size) CopyMem ((Dst), (Src), (Size))
#define zmemcmp(Ptr1, Ptr2, Size) CompareMem ((Ptr1), (Ptr2), (Size))
#define zmemzero(Dst, Size) ZeroMem ((Dst), (Size))

/* Diagnostic functions */
#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

voidpf ZLIB_INTERNAL zcalloc OF((voidpf opaque, unsigned items,
                                    unsigned size));
void ZLIB_INTERNAL zcfree  OF((voidpf opaque, voidpf ptr));

#define ZALLOC(strm, items, size) \
           (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}

/* Reverse the bytes in a 32-bit value */
#define ZSWAP32(q) ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
                    (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

#endif /* ZUTIL_H */
