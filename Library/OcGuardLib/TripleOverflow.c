/** @file

OcGuardLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcGuardLib.h>

BOOLEAN
OcOverflowTriAddU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  )
{
  UINT32   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddU32(A, B, &OcTmp);
  OcSecond = OcOverflowAddU32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriMulU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  )
{
  UINT32   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulU32(A, B, &OcTmp);
  OcSecond = OcOverflowMulU32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowAddMulU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  )
{
  UINT32   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddU32(A, B, &OcTmp);
  OcSecond = OcOverflowMulU32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowMulAddU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  )
{
  UINT32   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulU32(A, B, &OcTmp);
  OcSecond = OcOverflowAddU32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriAddS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  )
{
  INT32    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddS32(A, B, &OcTmp);
  OcSecond = OcOverflowAddS32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriMulS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  )
{
  INT32    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulS32(A, B, &OcTmp);
  OcSecond = OcOverflowMulS32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowAddMulS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  )
{
  INT32    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddS32(A, B, &OcTmp);
  OcSecond = OcOverflowMulS32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowMulAddS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  )
{
  INT32    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulS32(A, B, &OcTmp);
  OcSecond = OcOverflowAddS32(OcTmp, C, Result);
  return OcFirst | OcSecond;
}


BOOLEAN
OcOverflowTriAddU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  )
{
  UINT64   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddU64(A, B, &OcTmp);
  OcSecond = OcOverflowAddU64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriMulU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  )
{
  UINT64   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulU64(A, B, &OcTmp);
  OcSecond = OcOverflowMulU64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowAddMulU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  )
{
  UINT64   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddU64(A, B, &OcTmp);
  OcSecond = OcOverflowMulU64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowMulAddU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  )
{
  UINT64   OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulU64(A, B, &OcTmp);
  OcSecond = OcOverflowAddU64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriAddS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  )
{
  INT64    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddS64(A, B, &OcTmp);
  OcSecond = OcOverflowAddS64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriMulS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  )
{
  INT64    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulS64(A, B, &OcTmp);
  OcSecond = OcOverflowMulS64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowAddMulS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  )
{
  INT64    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddS64(A, B, &OcTmp);
  OcSecond = OcOverflowMulS64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowMulAddS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  )
{
  INT64    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulS64(A, B, &OcTmp);
  OcSecond = OcOverflowAddS64(OcTmp, C, Result);
  return OcFirst | OcSecond;
}


BOOLEAN
OcOverflowTriAddUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  )
{
  UINTN    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddUN(A, B, &OcTmp);
  OcSecond = OcOverflowAddUN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriMulUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  )
{
  UINTN    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulUN(A, B, &OcTmp);
  OcSecond = OcOverflowMulUN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowAddMulUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  )
{
  UINTN    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddUN(A, B, &OcTmp);
  OcSecond = OcOverflowMulUN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowMulAddUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  )
{
  UINTN    OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulUN(A, B, &OcTmp);
  OcSecond = OcOverflowAddUN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriAddSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  )
{
  INTN     OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddSN(A, B, &OcTmp);
  OcSecond = OcOverflowAddSN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowTriMulSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  )
{
  INTN     OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulSN(A, B, &OcTmp);
  OcSecond = OcOverflowMulSN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowAddMulSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  )
{
  INTN     OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowAddSN(A, B, &OcTmp);
  OcSecond = OcOverflowMulSN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}

BOOLEAN
OcOverflowMulAddSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  )
{
  INTN     OcTmp;
  BOOLEAN  OcFirst;
  BOOLEAN  OcSecond;

  OcFirst  = OcOverflowMulSN(A, B, &OcTmp);
  OcSecond = OcOverflowAddSN(OcTmp, C, Result);
  return OcFirst | OcSecond;
}
