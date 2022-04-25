/**
  This library performs unsigned arbitrary precision arithmetic operations.
  All results are returned into caller-provided buffers. The caller is
  responsible to ensure the buffers can hold a value of the precision it
  desires. Too large results will be truncated without further notification for
  public APIs.

  https://github.com/kokke/tiny-bignum-c has served as a template for several
  algorithmic ideas.

  This code is not to be considered general-purpose but solely to support
  cryptographic operations such as RSA encryption. As such, there are arbitrary
  limitations, such as requirement of equal precision, to limit the complexity
  of the operations to the bare minimum required to support such use caes.

  SECURITY: Currently, no security measures have been taken. This code is
            vulnerable to both timing and side channel attacks for value
            leakage. However, its current purpose is the verification of public
            binaries with public certificates, for which this is perfectly
            acceptable, especially in regards to performance.

  Copyright (C) 2019, Download-Fritz. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BigNumLibInternal.h"

#define OC_BN_MAX_VAL  ((OC_BN_WORD)0U - 1U)

STATIC_ASSERT (
  OC_BN_WORD_SIZE == sizeof (UINT32) || OC_BN_WORD_SIZE == sizeof (UINT64),
  "OC_BN_WORD_SIZE and OC_BN_WORD_NUM_BITS usages must be adapted."
  );

OC_BN_WORD
BigNumSwapWord (
  IN OC_BN_WORD  Word
  )
{
  if (OC_BN_WORD_SIZE == sizeof (UINT32)) {
    return (OC_BN_WORD)SwapBytes32 ((UINT32)Word);
  }

  if (OC_BN_WORD_SIZE == sizeof (UINT64)) {
    return (OC_BN_WORD)SwapBytes64 ((UINT64)Word);
  }

  ASSERT (FALSE);
}

/**
  Shifts A left by Exponent Words.

  @param[in,out] Result          The buffer to return the result into.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in,out] A               The number to be word-shifted.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     Exponent        The Word shift exponent.

**/
STATIC
VOID
BigNumLeftShiftWords (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     UINTN             Exponent
  )
{
  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (Exponent < NumWordsResult);
  ASSERT (NumWordsResult - Exponent >= NumWordsA);

  CopyMem (&Result[Exponent], A, OC_BN_SIZE (NumWordsResult - Exponent));
  ZeroMem (Result, OC_BN_SIZE (Exponent));
  if (NumWordsResult - Exponent > NumWordsA) {
    ZeroMem (
      &Result[NumWordsA + Exponent],
      OC_BN_SIZE (NumWordsResult - Exponent - NumWordsA)
      );
  }
}

/**
  Shifts A left by Exponent Bits for 0 < Exponent < #Bits(Word).
  Result must have the exact precision to carry the result.

  @param[in,out] Result          The buffer to return the result into.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in]     A               The base.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     NumWords        The Word shift exponent.
  @param[in]     NumBits         The Bit shift exponent.

**/
STATIC
VOID
BigNumLeftShiftWordsAndBits (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     UINTN             NumWords,
  IN     UINT8             NumBits
  )
{
  OC_BN_NUM_WORDS  Index;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (NumWordsResult == NumWordsA + NumWords + 1);
  //
  // NumBits must not be 0 because a shift of a Word by its Bit width or
  // larger is Undefined Behaviour.
  //
  ASSERT (NumBits > 0);
  ASSERT (NumBits < OC_BN_WORD_NUM_BITS);
  //
  // This is not an algorithmic requirement, but BigNumLeftShiftWords shall be
  // called if TRUE.
  //
  ASSERT (NumWords > 0);

  for (Index = (NumWordsA - 1); Index > 0; --Index) {
    Result[Index + NumWords] = (A[Index] << NumBits) | (A[Index - 1] >> (OC_BN_WORD_NUM_BITS - NumBits));
  }

  //
  // Handle the edge-cases at the beginning and the end of the value.
  //
  Result[NumWords]             = A[0] << NumBits;
  Result[NumWordsA + NumWords] = A[NumWordsA - 1] >> (OC_BN_WORD_NUM_BITS - NumBits);
  //
  // Zero everything outside of the previously set ranges.
  //
  ZeroMem (Result, OC_BN_SIZE (NumWords));
}

/**
  Calculates the left-shift of A by Exponent Bits.

  @param[in,out] Result          The buffer to return the result into.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in]     A               The number to shift.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     Exponent        The amount of Bits to shift by.

**/
STATIC
VOID
BigNumLeftShift (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     UINTN             Exponent
  )
{
  UINTN  NumWords;
  UINT8  NumBits;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);

  NumWords = Exponent / OC_BN_WORD_NUM_BITS;
  NumBits  = Exponent % OC_BN_WORD_NUM_BITS;

  if (NumBits != 0) {
    BigNumLeftShiftWordsAndBits (
      Result,
      NumWordsResult,
      A,
      NumWordsA,
      NumWords,
      NumBits
      );
  } else {
    BigNumLeftShiftWords (Result, NumWordsResult, A, NumWordsA, NumWords);
  }
}

/**
  Shifts A right by Exponent Words.

  @param[in,out] Result          The buffer to return the result into.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in,out] A               The number to be word-shifted.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     Exponent        The Word shift exponent.

**/
STATIC
VOID
BigNumRightShiftWords (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     UINTN             Exponent
  )
{
  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (Exponent < NumWordsResult);
  ASSERT (NumWordsResult - Exponent >= NumWordsA);

  CopyMem (Result, &A[Exponent], OC_BN_SIZE (NumWordsResult - Exponent));
  ZeroMem (&Result[NumWordsResult - Exponent], OC_BN_SIZE (Exponent));
}

/**
  Shifts A right by Exponent Bits for 0 < Exponent < #Bits(Word).

  @param[in,out] A         The base.
  @param[in]     NumWords  The number of Words of A.
  @param[in]     Exponent  The Bit shift exponent.

**/
STATIC
VOID
BigNumRightShiftBitsSmall (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     UINT8            Exponent
  )
{
  UINTN  Index;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);
  //
  // Exponent must not be 0 because a shift of a Word by its Bit width or
  // larger is Undefined Behaviour.
  //
  ASSERT (Exponent > 0);
  ASSERT (Exponent < OC_BN_WORD_NUM_BITS);

  for (Index = 0; Index < (NumWords - 1U); ++Index) {
    A[Index] = (A[Index] >> Exponent) | (A[Index + 1] << (OC_BN_WORD_NUM_BITS - Exponent));
  }

  A[Index] >>= Exponent;
}

/**
  Shifts A right by Exponent Bits for 0 < Exponent < #Bits(Word).

  @param[in,out] Result          The buffer to return the result into.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in]     A               The base.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     NumWords        The Word shift exponent.
  @param[in]     NumBits         The Bit shift exponent.

**/
STATIC
VOID
BigNumRightShiftWordsAndBits (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     UINTN             NumWords,
  IN     UINT8             NumBits
  )
{
  UINTN  Index;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (NumWordsA >= NumWords);
  //
  // NumBits must not be 0 because a shift of a Word by its Bit width or
  // larger is Undefined Behaviour.
  //
  ASSERT (NumBits > 0);
  ASSERT (NumBits < OC_BN_WORD_NUM_BITS);
  //
  // This, assuming below, is required to avoid overflows, which purely
  // internal calls should never produce.
  //
  ASSERT (NumWordsA - NumWords >= NumWordsResult);
  //
  // This is not an algorithmic requirement, but BigNumRightShiftWords shall be
  // called if FALSE.
  //
  // ASSERT (NumWords > 0);

  for (Index = NumWords; Index < (UINTN)(NumWordsA - 1); ++Index) {
    Result[Index - NumWords] = (A[Index] >> NumBits) | (A[Index + 1] << (OC_BN_WORD_NUM_BITS - NumBits));
  }

  //
  // Handle the edge-cases at the beginning and the end of the value.
  //
  Result[Index - NumWords] = (A[NumWordsA - 1] >> NumBits);
  //
  // Zero everything outside of the previously set ranges.
  //
  ZeroMem (&Result[Index - NumWords + 1], OC_BN_SIZE (NumWordsResult - (Index - NumWords + 1)));
}

/**
  Calculates the right-shift of A by Exponent Bits.

  @param[in,out] Result          The buffer to return the result into.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in]     A               The number to shift.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     Exponent        The amount of Bits to shift by.

**/
STATIC
VOID
BigNumRightShift (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     UINTN             Exponent
  )
{
  UINTN  NumWords;
  UINT8  NumBits;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);

  NumWords = Exponent / OC_BN_WORD_NUM_BITS;
  NumBits  = Exponent % OC_BN_WORD_NUM_BITS;

  if (NumBits != 0) {
    BigNumRightShiftWordsAndBits (
      Result,
      NumWordsResult,
      A,
      NumWordsA,
      NumWords,
      NumBits
      );
  } else {
    BigNumRightShiftWords (Result, NumWordsResult, A, NumWordsA, NumWords);
  }
}

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
  )
{
  UINT64  Result64;

  ASSERT (Hi != NULL);

  if (OC_BN_WORD_SIZE == sizeof (UINT32)) {
    Result64 = (UINT64)A * B;
    //
    // FIXME: The subtraction in the shift is a dirty hack to shut up MSVC.
    //
    *Hi = (OC_BN_WORD)(RShiftU64 (Result64, OC_BN_WORD_NUM_BITS));
    return (OC_BN_WORD)Result64;
  }

  if (OC_BN_WORD_SIZE == sizeof (UINT64)) {
    return BigNumWordMul64 (Hi, A, B);
  }
}

VOID
BigNumSub (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWords,
  IN     CONST OC_BN_WORD  *A,
  IN     CONST OC_BN_WORD  *B
  )
{
  OC_BN_WORD  TmpResult;
  OC_BN_WORD  Tmp1;
  OC_BN_WORD  Tmp2;
  UINTN       Index;
  UINT8       Borrow;

  ASSERT (Result != NULL);
  ASSERT (NumWords > 0);
  ASSERT (A != NULL);
  ASSERT (B != NULL);
  //
  // As only the same indices are ever accessed at a step, it is safe to call
  // this function with Result = A or Result = B.
  //
  Borrow = 0;
  for (Index = 0; Index < NumWords; ++Index) {
    Tmp1      = A[Index];
    Tmp2      = B[Index] + Borrow;
    TmpResult = (Tmp1 - Tmp2);
    //
    // When a subtraction wraps around, the result must be bigger than either
    // operand.
    //
    Borrow        = (Tmp2 < Borrow) | (Tmp1 < TmpResult);
    Result[Index] = TmpResult;
  }
}

/**
  Returns the number of significant bits in a Word.

  @param[in] Word  The word to gather the number of significant bits of.

  @returns  The number of significant bits in Word.

**/
STATIC
UINT8
BigNumSignificantBitsWord (
  IN OC_BN_WORD  Word
  )
{
  UINT8       NumBits;
  OC_BN_WORD  Mask;

  //
  // The values we are receiving are very likely large, thus this approach
  // should be reasonably fast.
  //
  NumBits = OC_BN_WORD_NUM_BITS;
  Mask    = (OC_BN_WORD)1U << (OC_BN_WORD_NUM_BITS - 1);
  while (Mask != 0 && (Word & Mask) == 0) {
    --NumBits;
    Mask >>= 1U;
  }

  return NumBits;
}

/**
  Returns the most significant word index of A.

  @param[in] A         The number to gather the most significant Word index of.
  @param[in] NumWords  The number of Words of A.

  @returns  The index of the most significant Word in A.

**/
STATIC
OC_BN_NUM_WORDS
BigNumMostSignificantWord (
  IN CONST OC_BN_WORD  *A,
  IN OC_BN_NUM_WORDS   NumWords
  )
{
  OC_BN_NUM_WORDS  Index;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);

  Index = NumWords;
  do {
    --Index;
    if (A[Index] != 0) {
      return Index;
    }
  } while (Index != 0);

  return 0;
}

OC_BN_NUM_BITS
BigNumSignificantBits (
  IN CONST OC_BN_WORD  *A,
  IN OC_BN_NUM_WORDS   NumWords
  )
{
  OC_BN_NUM_BITS  Index;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);

  Index = BigNumMostSignificantWord (A, NumWords);
  return ((Index * OC_BN_WORD_NUM_BITS) + BigNumSignificantBitsWord (A[Index]));
}

VOID
BigNumOrWord (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     OC_BN_WORD       Value,
  IN     UINTN            Exponent
  )
{
  UINTN  WordIndex;
  UINT8  NumBits;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);
  ASSERT (Exponent / OC_BN_WORD_NUM_BITS < NumWords);

  WordIndex = Exponent / OC_BN_WORD_NUM_BITS;
  if (WordIndex < NumWords) {
    NumBits       = Exponent % OC_BN_WORD_NUM_BITS;
    A[WordIndex] |= (Value << NumBits);
  }
}

INTN
BigNumCmp (
  IN CONST OC_BN_WORD  *A,
  IN OC_BN_NUM_WORDS   NumWords,
  IN CONST OC_BN_WORD  *B
  )
{
  UINTN  Index;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);
  ASSERT (B != NULL);

  Index = NumWords;
  do {
    --Index;
    if (A[Index] > B[Index]) {
      return 1;
    } else if (A[Index] < B[Index]) {
      return -1;
    }
  } while (Index != 0);

  return 0;
}

VOID
BigNumMod (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsRest,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     CONST OC_BN_WORD  *B,
  IN     OC_BN_WORD        *Scratch
  )
{
  INTN  CmpResult;

  OC_BN_NUM_BITS   SigBitsModTmp;
  OC_BN_NUM_WORDS  SigWordsModTmp;
  OC_BN_WORD       *ModTmp;
  OC_BN_NUM_BITS   BigDivExp;
  OC_BN_WORD       *BigDiv;
  OC_BN_NUM_BITS   SigBitsBigDiv;
  OC_BN_NUM_WORDS  SigWordsBigDiv;

  OC_BN_NUM_BITS  DeltaBits;

  ASSERT (Result != NULL);
  ASSERT (NumWordsRest > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (B != NULL);
  ASSERT (NumWordsA >= NumWordsRest);
  //
  // SigBitsModTmp is calculated manually to avoid calculating SigWordsModTmp
  // by modulo.
  //
  SigWordsModTmp = BigNumMostSignificantWord (A, NumWordsA);
  SigBitsModTmp  = SigWordsModTmp * OC_BN_WORD_NUM_BITS + BigNumSignificantBitsWord (A[SigWordsModTmp]);
  ++SigWordsModTmp;
  ASSERT (SigBitsModTmp == BigNumSignificantBits (A, SigWordsModTmp));

  ModTmp         = &Scratch[0];
  BigDiv         = &Scratch[SigWordsModTmp];
  SigWordsBigDiv = SigWordsModTmp;
  //
  // Invariant: BigDiv > ModTmp / 2
  // The invariant implies ModTmp / BigDiv <= 1 (latter in the strictest case).
  //
  // This loop iteratively subtracts multiples BigDiv of B from ModTmp := A [1],
  // otherwise ModTmp is not modified. BigDiv is iteratively reduced such that
  // ModTmp >= BigDiv [2]. The loop terminates once BigDiv < B yields true [3].
  //
  CopyMem (ModTmp, A, OC_BN_SIZE (SigWordsModTmp));
  //
  // Initialisation:
  // A bit-shift by x is equal to multiplying the operand with 2^x.
  // BigDiv := B << x so that its MSB is equal to the MSB of A implies:
  // 2*BigDiv > A <=> BigDiv > ModTmp / 2 with ModTmp = A.
  //
  SigBitsBigDiv = BigNumSignificantBits (B, NumWordsRest);
  ASSERT (SigBitsModTmp >= SigBitsBigDiv);
  BigDivExp = SigBitsModTmp - SigBitsBigDiv;

  BigNumLeftShift (BigDiv, SigWordsBigDiv, B, NumWordsRest, BigDivExp);
  SigBitsBigDiv = SigBitsModTmp;
  ASSERT (SigBitsBigDiv == BigNumSignificantBits (BigDiv, SigWordsBigDiv));

  while (TRUE) {
    //
    // Because the invariant is maintained optimally, the MSB of BigDiv is
    // either the MSB of ModTmp or one below. SigWords* are maintained
    // precisely during the loop's execution, so when SigWordsModTmp is larger
    // than SigWordsBigDiv, ModTmp is larger than BigDiv.
    //
    ASSERT (SigWordsModTmp == SigWordsBigDiv || SigWordsModTmp == SigWordsBigDiv + 1);
    if (SigWordsModTmp > SigWordsBigDiv) {
      ASSERT (ModTmp[SigWordsModTmp - 1] != 0);
      CmpResult = 1;
    } else {
      CmpResult = BigNumCmp (ModTmp, SigWordsBigDiv, BigDiv);
    }

    if (CmpResult >= 0) {
      //
      // Iteration 1: [1] ModTmp >= BigDiv means ModTmp is reduced.
      //
      // Subtract SigWordsModTmp words because the current divisor value may be
      // shorter than the current modulus value. As both reside in buffers of
      // equal length and no high word is stripped without being set to 0, this
      // is safe.
      //
      BigNumSub (ModTmp, SigWordsModTmp, ModTmp, BigDiv);

      if (BigDivExp == 0) {
        //
        // Iteration 1: [3] BigDiv = B implies BigDiv < B would yield true
        //                  after executing the reduction below.
        //
        ASSERT (BigNumCmp (BigDiv, SigWordsBigDiv, B) == 0);
        break;
      }

      //
      // SigBitsModTmp is calculated manually to avoid calculating
      // SigWordsModTmp by modulo.
      //
      SigWordsModTmp = BigNumMostSignificantWord (ModTmp, SigWordsModTmp);
      SigBitsModTmp  = SigWordsModTmp * OC_BN_WORD_NUM_BITS + BigNumSignificantBitsWord (ModTmp[SigWordsModTmp]);
      ++SigWordsModTmp;
      ASSERT (SigBitsModTmp == BigNumSignificantBits (ModTmp, SigWordsModTmp));

      ASSERT (SigBitsBigDiv >= SigBitsModTmp);
      DeltaBits = SigBitsBigDiv - SigBitsModTmp;
      if (DeltaBits > BigDivExp) {
        //
        // Iteration 1: [3] This implies BigDiv < B would yield true after
        //                  executing the reduction below.
        //
        break;
      }

      //
      // Iteration 1: [2] Please refer to Initialisation.
      //
      BigNumRightShift (BigDiv, SigWordsBigDiv, BigDiv, SigWordsBigDiv, DeltaBits);

      SigWordsBigDiv = (OC_BN_NUM_WORDS)((SigBitsModTmp + (OC_BN_WORD_NUM_BITS - 1U)) / OC_BN_WORD_NUM_BITS);
      SigBitsBigDiv  = SigBitsModTmp;

      BigDivExp -= DeltaBits;
    } else {
      //
      // Iteration 2: [1] BigDiv > ModTmp means ModTmp will be reduced next
      //                  iteration.
      //
      if (BigDivExp == 0) {
        //
        // Iteration 2: [3] BigDiv = B implies BigDiv < B would yield true
        //                  after executing the reduction below.
        //
        ASSERT (BigNumCmp (BigDiv, SigWordsBigDiv, B) == 0);
        break;
      }

      //
      // Iteration 2: [2] BigDiv > ModTmp means BigDiv is reduced to more
      //                  strictly maintain the invariant BigDiv / 2 > ModTmp.
      //
      BigNumRightShiftBitsSmall (BigDiv, SigWordsBigDiv, 1);

      --SigBitsBigDiv;
      --BigDivExp;

      if (SigBitsBigDiv % OC_BN_WORD_NUM_BITS == 0) {
        //
        // Every time the subtraction by 1 yields a multiplie of the word
        // length, the most significant Byte has become zero and is stripped.
        //
        ASSERT (BigDiv[SigWordsBigDiv - 1] == 0);
        --SigWordsBigDiv;
      }
    }

    //
    // ASSERT both branches maintain SigBitsBigDiv correctly.
    //
    ASSERT (SigBitsBigDiv == BigNumSignificantBits (BigDiv, SigWordsBigDiv));
  }

  //
  // Termination:
  // Because BigDiv = B and, by invariant, BigDiv > ModTmp / 2 are true in the
  // last iteration, B > ModTmp / 2 <=> 2 * B > ModTmp is true and thus
  // conditionally subtracting B from ModTmp once more yields B > ModTmp, at
  // which point ModTmp must carry the modulus of A / B. The final reduction
  // of BigDiv yields BigDiv < B and thus the loop is terminated without
  // further effects.
  //

  //
  // Assuming correctness, the modulus cannot be larger than the divisor.
  //
  ASSERT (BigNumMostSignificantWord (ModTmp, SigWordsModTmp) + 1 <= NumWordsRest);
  CopyMem (Result, ModTmp, OC_BN_SIZE (NumWordsRest));
}

VOID
BigNumParseBuffer (
  IN OUT OC_BN_WORD       *Result,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     CONST UINT8      *Buffer,
  IN     UINTN            BufferSize
  )
{
  UINTN       Index;
  OC_BN_WORD  Tmp;

  ASSERT (Result != NULL);
  ASSERT (OC_BN_SIZE (NumWords) == BufferSize);
  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);

  for (Index = OC_BN_WORD_SIZE; Index <= BufferSize; Index += OC_BN_WORD_SIZE) {
    if (OC_BN_WORD_SIZE == sizeof (UINT32)) {
      Tmp = (OC_BN_WORD)(
                         ((UINT32)Buffer[(BufferSize - Index) + 0] << 24U) |
                         ((UINT32)Buffer[(BufferSize - Index) + 1] << 16U) |
                         ((UINT32)Buffer[(BufferSize - Index) + 2] << 8U) |
                         ((UINT32)Buffer[(BufferSize - Index) + 3] << 0U));
    } else if (OC_BN_WORD_SIZE == sizeof (UINT64)) {
      Tmp = (OC_BN_WORD)(
                         ((UINT64)Buffer[(BufferSize - Index) + 0] << 56U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 1] << 48U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 2] << 40U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 3] << 32U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 4] << 24U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 5] << 16U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 6] << 8U) |
                         ((UINT64)Buffer[(BufferSize - Index) + 7] << 0U));
    }

    Result[(Index / OC_BN_WORD_SIZE) - 1] = Tmp;
  }
}
