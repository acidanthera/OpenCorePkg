/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#ifndef ZCONF_H
#define ZCONF_H

#include <Library/BaseMemoryLib.h>

//
// EDIT (Download-Fritz): Disable MSVC warnings preventing compilation.
//                        Include stddef.h for its wchar_t definition.
//
#if defined(_MSC_VER)
  #include <stddef.h>
  #pragma warning ( disable : 4131 )
  #pragma warning ( disable : 4244 )
#endif

#define NO_GZIP 1

/* Maximum value for memLevel in deflateInit2 */
#define MAX_MEM_LEVEL 9

/* Maximum value for windowBits in deflateInit2 and inflateInit2.
 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
 * created by gzip. (Files created by minigzip can still be extracted by
 * gzip.)
 */
#define MAX_WBITS   15 /* 32K LZ77 window */

#define Z_LARGE64 1

/* Type declarations */

#define OF(args)     args
#define Z_ARG(args)  args /* function prototypes for stdarg */

#define ZEXTERN extern
#define ZEXPORT
#define ZEXPORTVA
#define FAR

#define z_const const

typedef unsigned char   Byte;  /* 8 bits */
typedef unsigned int    uInt;  /* 16 bits or more */
typedef unsigned long   uLong; /* 32 bits or more */
typedef Byte            Bytef;
typedef char            charf;
typedef int             intf;
typedef uInt            uIntf;
typedef uLong           uLongf;
typedef void const      *voidpc;
typedef void            *voidpf;
typedef void            *voidp;

typedef UINTN     z_size_t;
typedef UINT32    z_crc_t;
#define z_off_t   INTN
#define z_off64_t INTN
#define Z_U4      UINT32
#define Z_U8      UINT64

#ifdef MIN
#undef MIN
#endif

#ifdef _WIN32
#undef _WIN32
#endif

#ifdef _WIN64
#undef _WIN64
#endif

#endif /* ZCONF_H */
