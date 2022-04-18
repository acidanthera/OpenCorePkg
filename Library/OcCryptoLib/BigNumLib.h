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

#include "CryptoInternal.h"

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
typedef UINT32 OC_BN_SIZE;
typedef UINT32 OC_BN_NUM_BITS;
#define OC_BN_MAX_SIZE  ((OC_BN_SIZE) SIZE_64KB)
#define OC_BN_MAX_LEN   ((OC_BN_NUM_WORDS) (OC_BN_MAX_SIZE / OC_BN_WORD_SIZE))

/**
  The size, in Bytes, required to store a BigNum with NumWords words.
 
  @param[in] NumWords  The number of Words. Must be at most OC_BN_MAX_LEN.
**/
#define OC_BN_SIZE(NumWords)  \
  ((OC_BN_SIZE) (NumWords) * OC_BN_WORD_SIZE)

STATIC_ASSERT (
  OC_BN_MAX_LEN <= MAX_UINTN / OC_BN_WORD_SIZE,
  "The definition of OC_BN_SIZE may cause an overflow"
  );

/**
  The size, in Bits, required to store a BigNum with NumWords words.
 
  @param[in] NumWords  The number of Words. Must be at most OC_BN_MAX_LEN.
**/
#define OC_BN_BITS(NumWords)  \
  ((OC_BN_NUM_BITS) OC_BN_SIZE (NumWords) * OC_CHAR_BIT)

STATIC_ASSERT (
  OC_BN_BITS (OC_BN_MAX_LEN) <= MAX_UINTN / OC_CHAR_BIT,
  "The definition of OC_BN_BITS may cause an overflow"
  );

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

#define OC_BN_MONT_MAX_LEN   (OC_BN_MAX_LEN / 2U - 1U)
#define OC_BN_MONT_MAX_SIZE  (OC_BN_SIZE (OC_BN_MONT_MAX_LEN))

/**
  The length, in BigNum words, of RSqr.
 
  @param[in] NumWords  The number of Words of N. Must be at most
                       OC_BN_MONT_MAX_LEN.
**/
#define OC_BN_MONT_RSQR_LEN(NumWords)  ((OC_BN_NUM_WORDS) (((NumWords) + 1U) * 2U))

STATIC_ASSERT (
  OC_BN_MONT_RSQR_LEN (OC_BN_MONT_MAX_LEN) == OC_BN_MAX_LEN,
  "The definition of OC_BN_MONT_RSQR_LEN is faulty"
  );

/**
  The size, in Bytes, of RSqr.
 
  @param[in] NumWords  The number of Words of N. Must be at most
                       OC_BN_MONT_MAX_LEN.
**/
#define OC_BN_MONT_RSQR_SIZE(NumWords)  (OC_BN_SIZE (OC_BN_MONT_RSQR_LEN (NumWords)))

/**
  1 + 2 * NumWords for RSqr, and then twice more than that for Mod.
 
  @param[in] NumWords  The number of Words of RSqrMod and N. Must be at most
                       OC_BN_MONT_MAX_LEN.
**/
#define BIG_NUM_MONT_PARAMS_SCRATCH_SIZE(NumWords) \
  (OC_BN_MONT_RSQR_SIZE (NumWords) * 3U)

STATIC_ASSERT (
  OC_BN_MONT_RSQR_SIZE (OC_BN_MONT_MAX_SIZE) <= MAX_UINTN / 3,
  "The definition of BIG_NUM_MONT_PARAMS_SCRATCH_SIZE may cause an overflow"
  );

/**
  Calculates the Montgomery Inverse and R^2 mod N.

  @param[in,out] RSqrMod   The buffer to return R^2 mod N into.
  @param[in]     NumWords  The number of Words of RSqrMod and N. Must be at most
                           OC_BN_MONT_MAX_LEN.
  @param[in]     N         The Montgomery Modulus.
  @param[in]     Scratch   Scratch buffer BIG_NUM_MONT_PARAMS_SCRATCH_SIZE(NumWords).

  @returns  The Montgomery Inverse of N.

**/
OC_BN_WORD
BigNumCalculateMontParams (
  IN OUT OC_BN_WORD        *RSqrMod,
  IN     OC_BN_NUM_WORDS   NumWords,
  IN     CONST OC_BN_WORD  *N,
  IN     OC_BN_WORD        *Scratch
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
  @param[in]     ATmp      Scratch buffer of NumWords.

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
  IN     CONST OC_BN_WORD  *RSqrMod,
  IN     OC_BN_WORD        *ATmp
  );

#endif // BIG_NUM_LIB_H
