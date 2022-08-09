/*
 * Copyright (c) 2013, Alexey Degtyarev <alexey@renatasystems.org>. 
 * All rights reserved.
 *
 * $Id$
 */


#include"CryptoInternal.h"

#include "gost3411-2012-ref.h"


union uint512_u
{
  UINT64    QWORD[8];
};

#include "gost3411-2012-const.h"
#include "gost3411-2012-precalc.h"

typedef struct GOST34112012Context
{
  UINT8              buffer[64];
  union uint512_u    hash;
  union uint512_u    h;
  union uint512_u    N;
  union uint512_u    Sigma;
  UINT32             bufsize;
  UINT32             digest_size;
} GOST34112012Context;

VOID
GOST34112012Init(
  GOST34112012Context  *CTX,
  CONST UINT32         digest_size
  );

VOID
GOST34112012Update(
  GOST34112012Context  *CTX, 
  CONST UINT8          *data,
  UINT32               len
  ); 

VOID
GOST34112012Final(
  GOST34112012Context  *CTX, 
  UINT8                *digest
  ); 

VOID
GOST34112012Cleanup(
  GOST34112012Context  *CTX
  );
  
VOID
Streebog(
  CONST UINT8  *data, 
  UINT32       len, 
  UINT8        *digest, 
  UINT32       digest_size
  );
