/**
  Copyright (C) 2019, Download-Fritz. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef BIG_NUM_LIB_INTERNAL_H
#define BIG_NUM_LIB_INTERNAL_H

#include "BigNumLib.h"

/**
  Calculates the product of A and B.

  @param[out] Hi  Buffer in which the high Word of the result is returned.
  @param[in]  A   The multiplicant.
  @param[in]  B   The multiplier.

  @returns  The low Word of the result.

**/
OC_BN_WORD
BigNumWordMul64 (
  OUT OC_BN_WORD  *Hi,
  IN  OC_BN_WORD  A,
  IN  OC_BN_WORD  B
  );

/**
  Calculates the product of A and B.

  @param[out] Hi  Buffer in which the high Word of the result is returned.
  @param[in]  A   The multiplicant.
  @param[in]  B   The multiplier.

  @returns  The low Word of the result.

**/
OC_BN_WORD
BigNumWordMul (
  OUT OC_BN_WORD  *Hi,
  IN  OC_BN_WORD  A,
  IN  OC_BN_WORD  B
  );

/**
  Calulates the difference of A and B.
  A must have the same precision as B. Result must have a precision at most as
  bit as A and B.

  @param[in,out] Result    The buffer to return the result into.
  @param[in]     NumWords  The number of Words of Result, minimum number of
                           Words of A and B.
  @param[in]     A         The minuend.
  @param[in]     B         The subtrahend.

**/
VOID
BigNumSub (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWords,
  IN     CONST OC_BN_WORD  *A,
  IN     CONST OC_BN_WORD  *B
  );

/**
  Calculates the binary union of A and (Value << Exponent).

  @param[in,out] A         The number to OR with and store the result into.
  @param[in]     NumWords  The number of Words of A.
  @param[in]     Value     The Word value to OR with.
  @param[in]     Exponent  The Word shift exponent.

**/
VOID
BigNumOrWord (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     OC_BN_WORD       Value,
  IN     UINTN            Exponent
  );

/**
  Calculates the remainder of A and B.

  @param[in,out] Result        The buffer to return the result into.
  @param[in]     NumWordsRest  The number of Words of Result and B.
  @param[in]     A             The dividend.
  @param[in]     NumWordsA     The number of Words of A.
  @param[in]     B             The divisor.

  @returns  Whether the operation was completes successfully.

**/
BOOLEAN
BigNumMod (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsRest,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     CONST OC_BN_WORD  *B
  );

/**
  Returns the relative order of A and B. A and B must have the same precision.

  @param[in] A         The first number to compare.
  @param[in] NumWords  The number of Words to compare.
  @param[in] B         The second number to compare.

  @retval < 0  A is lower than B.
  @retval 0    A is as big as B.
  @retval > 0  A is greater than B.

**/
INTN
BigNumCmp (
  IN CONST OC_BN_WORD  *A,
  IN OC_BN_NUM_WORDS   NumWords,
  IN CONST OC_BN_WORD  *B
  );

/**
  Returns the number of significant bits in a number.
  Logically matches OpenSSL's BN_num_bits.

  @param[in] A         The number to gather the number of significant bits of.
  @param[in] NumWords  The number of Words of A.

  @returns  The number of significant bits in A.

**/
OC_BN_NUM_BITS
BigNumSignificantBits (
  IN CONST OC_BN_WORD  *A,
  IN OC_BN_NUM_WORDS   NumWords
  );

#endif // BIG_NUM_LIB_INTERNAL_H
