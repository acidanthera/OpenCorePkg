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

#ifndef OC_GUARD_LIB_H
#define OC_GUARD_LIB_H

//
// The macros below provide pointer alignment checking interfaces.
// TypedPtr - pointer of a dedicated type, which alignment is to be checked.
// Align    - valid alignment for the target platform (power of two so far).
// Type     - valid complete typename.
// Ptr      - raw pointer value, must fit into UINTN, meant to be uintptr_t equivalent.
//

#define OC_ALIGNOF(Type)           (_Alignof (Type))
#define OC_POT_ALIGNED(Align, Ptr) (0ULL == (((UINTN) (Ptr)) & (Align-1U)))
#define OC_TYPE_ALIGNED(Type, Ptr) (OC_POT_ALIGNED (OC_ALIGNOF (Type), Ptr))

//
// Force member alignment for the structure.
//
#if (defined(__STDC__) && __STDC_VERSION__ >= 201112L) || defined(__GNUC__) || defined(__clang__)
#define OC_ALIGNAS(Alignment) _Alignas(Alignment)
#else
#define OC_ALIGNAS(Alignment)
#endif

/**
 Return the result of (Multiplicand * Multiplier / Divisor).

 @param Multiplicand A 64-bit unsigned value.
 @param Multiplier   A 64-bit unsigned value.
 @param Divisor      A 32-bit unsigned value.
 @param Remainder    A pointer to a 32-bit unsigned value. This parameter is
 optional and may be NULL.

 @return Multiplicand * Multiplier / Divisor.
 **/
UINT64
MultThenDivU64x64x32 (
  IN  UINT64  Multiplicand,
  IN  UINT64  Multiplier,
  IN  UINT32  Divisor,
  OUT UINT32  *Remainder  OPTIONAL
  );

//
// The interfaces below provide base safe arithmetics, reporting
// signed integer overflow and unsigned integer wraparound similarly to
// os/overflow.h in macOS SDK.
//
// Each interface may be implemented not only as an actual function, but
// a macro as well. Macro implementations are allowed to evaluate the
// expressions no more than once, and are supposed to provide faster
// compiler builtins if available.
//
// Each interface returns FALSE when the the stored result is equal to
// the infinite precision result, otherwise TRUE. The operands should
// be read left to right with the last argument representing a non-NULL
// pointer to the resulting value of the same type.
//
// More information could be found in Clang Extensions documentation:
// http://releases.llvm.org/7.0.0/tools/clang/docs/LanguageExtensions.html#checked-arithmetic-builtins
//

//
// 32-bit integer addition, subtraction, multiplication, triple addition (A+B+C),
// triple multiplication (A*B*C), addition with multiplication ((A+B)*C),
// and multiplication with addition (A*B+C) support.
//

BOOLEAN
OcOverflowAddU16 (
  UINT16  A,
  UINT16  B,
  UINT16  *Result
  );

BOOLEAN
OcOverflowSubU16 (
  UINT16  A,
  UINT16  B,
  UINT16  *Result
  );

BOOLEAN
OcOverflowMulU16 (
  UINT16  A,
  UINT16  B,
  UINT16  *Result
  );

BOOLEAN
OcOverflowAddU32 (
  UINT32  A,
  UINT32  B,
  UINT32  *Result
  );

BOOLEAN
OcOverflowSubU32 (
  UINT32  A,
  UINT32  B,
  UINT32  *Result
  );

BOOLEAN
OcOverflowMulU32 (
  UINT32  A,
  UINT32  B,
  UINT32  *Result
  );

BOOLEAN
OcOverflowTriAddU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  );

BOOLEAN
OcOverflowTriMulU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  );

BOOLEAN
OcOverflowAddMulU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  );

BOOLEAN
OcOverflowMulAddU32 (
  UINT32  A,
  UINT32  B,
  UINT32  C,
  UINT32  *Result
  );

BOOLEAN
OcOverflowAlignUpU32 (
  UINT32  Value,
  UINT32  Alignment,
  UINT32  *Result
  );

BOOLEAN
OcOverflowAddS32 (
  INT32  A,
  INT32  B,
  INT32  *Result
  );

BOOLEAN
OcOverflowSubS32 (
  INT32  A,
  INT32  B,
  INT32  *Result
  );

BOOLEAN
OcOverflowMulS32 (
  INT32  A,
  INT32  B,
  INT32  *Result
  );

BOOLEAN
OcOverflowTriAddS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  );

BOOLEAN
OcOverflowTriMulS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  );

BOOLEAN
OcOverflowAddMulS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  );

BOOLEAN
OcOverflowMulAddS32 (
  INT32  A,
  INT32  B,
  INT32  C,
  INT32  *Result
  );

//
// 64-bit integer addition, subtraction, multiplication, triple addition (A+B+C),
// triple multiplication (A*B*C), addition with multiplication ((A+B)*C),
// and multiplication with addition (A*B+C) support.
//

BOOLEAN
OcOverflowAddU64 (
  UINT64  A,
  UINT64  B,
  UINT64  *Result
  );

BOOLEAN
OcOverflowSubU64 (
  UINT64  A,
  UINT64  B,
  UINT64  *Result
  );

BOOLEAN
OcOverflowMulU64 (
  UINT64  A,
  UINT64  B,
  UINT64  *Result
  );

BOOLEAN
OcOverflowTriAddU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  );

BOOLEAN
OcOverflowTriMulU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  );

BOOLEAN
OcOverflowAddMulU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  );

BOOLEAN
OcOverflowMulAddU64 (
  UINT64  A,
  UINT64  B,
  UINT64  C,
  UINT64  *Result
  );


BOOLEAN
OcOverflowAddS64 (
  INT64  A,
  INT64  B,
  INT64  *Result
  );

BOOLEAN
OcOverflowSubS64 (
  INT64  A,
  INT64  B,
  INT64  *Result
  );

BOOLEAN
OcOverflowMulS64 (
  INT64  A,
  INT64  B,
  INT64  *Result
  );

BOOLEAN
OcOverflowTriAddS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  );

BOOLEAN
OcOverflowTriMulS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  );

BOOLEAN
OcOverflowAddMulS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  );

BOOLEAN
OcOverflowMulAddS64 (
  INT64  A,
  INT64  B,
  INT64  C,
  INT64  *Result
  );

//
// Native integer addition, subtraction, multiplication, triple addition (A+B+C),
// triple multiplication (A*B*C), addition with multiplication ((A+B)*C),
// and multiplication with addition (A*B+C) support.
//

BOOLEAN
OcOverflowAddUN (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  );

BOOLEAN
OcOverflowSubUN (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  );

BOOLEAN
OcOverflowMulUN (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  );

BOOLEAN
OcOverflowTriAddUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  );

BOOLEAN
OcOverflowTriMulUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  );

BOOLEAN
OcOverflowAddMulUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  );

BOOLEAN
OcOverflowMulAddUN (
  UINTN  A,
  UINTN  B,
  UINTN  C,
  UINTN  *Result
  );

BOOLEAN
OcOverflowAddSN (
  INTN  A,
  INTN  B,
  INTN  *Result
  );

BOOLEAN
OcOverflowSubSN (
  INTN  A,
  INTN  B,
  INTN  *Result
  );

BOOLEAN
OcOverflowMulSN (
  INTN  A,
  INTN  B,
  INTN  *Result
  );

BOOLEAN
OcOverflowTriAddSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  );

BOOLEAN
OcOverflowTriMulSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  );

BOOLEAN
OcOverflowAddMulSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  );

BOOLEAN
OcOverflowMulAddSN (
  INTN  A,
  INTN  B,
  INTN  C,
  INTN  *Result
  );

#endif // OC_GUARD_LIB_H
