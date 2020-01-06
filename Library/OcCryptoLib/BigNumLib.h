/**
  This library performs arbitrary precision arithmetic operations.
  For more details, please refer to the source files and function headers.

  Copyright (C) 2019, Download-Fritz. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef BIG_NUM_LIB_H
#define BIG_NUM_LIB_H

#include <Library/OcMiscLib.h>

///
/// A BIGNUM word. This is at best an integer of the platform's natural size
/// to optimize memory accesses and arithmetic operation count.
///
typedef UINTN OC_BN_WORD;
//
// Declarations regarding the Word size.
//
#define OC_BN_WORD_SIZE      (sizeof (OC_BN_WORD))
#define OC_BN_WORD_NUM_BITS  ((OC_BN_NUM_BITS) (OC_BN_WORD_SIZE * OC_CHAR_BIT))
//
// Declarations regarding the maximum size of OC_BN structures.
//
typedef UINT16 OC_BN_NUM_WORDS;
typedef UINT32 OC_BN_NUM_BITS;
#define OC_BN_MAX_SIZE  MAX_UINT16
#define OC_BN_MAX_LEN   (OC_BN_MAX_SIZE / OC_BN_WORD_SIZE)

//
// Primitives
//

/**
  Parses a data array into a number. The buffer size must be a multiple of the
  Word size. The length of Result must precisely fit the required size.

  @param[in,out] Result      The buffer to store the result in.
  @param[in]     NumWords    The number of Words of Result.
  @param[in]     Buffer      The buffer to parse.
  @param[in]     BufferSize  The size, in bytes, of Buffer.

**/
VOID
BigNumParseBuffer (
  IN OUT OC_BN_WORD       *Result,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     CONST UINT8      *Buffer,
  IN     UINTN            BufferSize
  );

/**
  Swaps the byte order of Word.

  @param[in] Word  The Word to swap.

  @returns  The byte-swapped value of Word.

**/
OC_BN_WORD
BigNumSwapWord (
  IN OC_BN_WORD  Word
  );

//
// Montgomery arithmetics
//

/**
  Calculates the Montgomery Inverse and R^2 mod N.

  @param[in,out] RSqrMod   The buffer to return R^2 mod N into.
  @param[in]     NumWords  The number of Words of RSqrMod and N.
  @param[in]     N         The Montgomery Modulus.

  @returns  The Montgomery Inverse of N.

**/
OC_BN_WORD
BigNumCalculateMontParams (
  IN OUT OC_BN_WORD        *RSqrMod,
  IN     OC_BN_NUM_WORDS   NumWords,
  IN     CONST OC_BN_WORD  *N
  );

/**
  Caulculates the exponentiation of A with B mod N.

  @param[in,out] Result    The buffer to return the result into.
  @param[in]     NumWords  The number of Words of Result, A, N and RSqrMod.
  @param[in]     A         The base.
  @param[in]     B         The exponent.
  @param[in]     N         The modulus.
  @param[in]     N0Inv     The Montgomery Inverse of N.
  @param[in]     RSqrMod   Montgomery's R^2 mod N.

  @returns  Whether the operation was completes successfully.

**/
BOOLEAN
BigNumPowMod (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWords,
  IN     CONST OC_BN_WORD  *A,
  IN     UINT32            B,
  IN     CONST OC_BN_WORD  *N,
  IN     OC_BN_WORD        N0Inv,
  IN     CONST OC_BN_WORD  *RSqrMod
  );

#endif // BIG_NUM_LIB_H
