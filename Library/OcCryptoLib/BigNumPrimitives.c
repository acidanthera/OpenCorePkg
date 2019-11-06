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

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcGuardLib.h>

#include "BigNumLibInternal.h"

#define OC_BN_MAX_VAL  ((OC_BN_WORD)0U - 1U)

OC_STATIC_ASSERT (
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

  CopyMem (&Result[Exponent], A, (NumWordsResult - Exponent) * OC_BN_WORD_SIZE);
  ZeroMem (Result, Exponent * OC_BN_WORD_SIZE);
  if (NumWordsResult - Exponent > NumWordsA) {
    ZeroMem (
      &Result[NumWordsA + Exponent],
      (NumWordsResult - Exponent - NumWordsA) * OC_BN_WORD_SIZE
      );
  }
}

/**
  Shifts A left by Exponent Bits for 0 < Exponent < #Bits(Word).

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
  UINTN Index;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (NumWordsResult >= NumWords);
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
  ASSERT (NumWordsResult - NumWords > NumWordsA);
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
  Result[NumWords] = A[0] << NumBits;
  Result[NumWordsA + NumWords] = A[NumWordsA - 1] >> (OC_BN_WORD_NUM_BITS - NumBits);
  //
  // Zero everything outside of the previously set ranges.
  //
  ZeroMem (&Result[NumWordsA + NumWords + 1], (NumWordsResult - NumWords - NumWordsA - 1) * OC_BN_WORD_SIZE);
  ZeroMem (Result, NumWords * OC_BN_WORD_SIZE);
}

/**
  Shifts A left by Exponent Bits for 0 < Exponent < #Bits(Word).

  @param[in,out] A         The base.
  @param[in]     NumWords  The number of Words of A.
  @param[in]     Exponent  The Bit shift exponent.

**/
STATIC
VOID
BigNumLeftShiftBitsSmall (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     UINT8            Exponent
  )
{
  UINTN Index;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);
  //
  // Exponent must not be 0 because a shift of a Word by its Bit width or
  // larger is Undefined Behaviour.
  //
  ASSERT (Exponent > 0);
  ASSERT (Exponent < OC_BN_WORD_NUM_BITS);

  for (Index = (NumWords - 1); Index > 0; --Index) {
    A[Index] = (A[Index] << Exponent) | (A[Index - 1] >> (OC_BN_WORD_NUM_BITS - Exponent));
  }
  A[0] <<= Exponent;
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
  UINTN Index;

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
  )
{
  UINT64 Result64;

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

/**
  Assigns A to Result.

  @param[in,out] Result          The buffer to store the result in.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in]     A               The number to assign.
  @param[in]     NumWordsA       The number of Words of A.

**/
STATIC
VOID
BigNumAssign (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA
  )
{
  UINTN NumWordsCopy;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);

  if (NumWordsResult > NumWordsA) {
    ZeroMem (
      &Result[NumWordsA],
      (NumWordsResult - NumWordsA) * OC_BN_WORD_SIZE
      );
    NumWordsCopy = NumWordsA;
  } else {
    NumWordsCopy = NumWordsResult;
  }

  CopyMem (Result, A, NumWordsCopy * OC_BN_WORD_SIZE);
}

VOID
BigNumAssign0 (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords
  )
{
  ASSERT (A != NULL);
  ASSERT (NumWords > 0);

  ZeroMem (A, NumWords * OC_BN_WORD_SIZE);
}

VOID
BigNumSub (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWords,
  IN     CONST OC_BN_WORD  *A,
  IN     CONST OC_BN_WORD  *B
  )
{
  OC_BN_WORD TmpResult;
  OC_BN_WORD Tmp1;
  OC_BN_WORD Tmp2;
  UINTN      Index;
  UINT8      Borrow;

  ASSERT (Result != NULL);
  ASSERT (NumWords > 0);
  ASSERT (A != NULL);
  ASSERT (B != NULL);
  //
  // As the same indices are ever accessed at a step, the index is always
  // increased per step, the preexisting values in c are unused and all are
  // are set, it is safe to call this function with c = a or c = b
  // ATTENTION: This might conflict with future "top" optimizations
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
    Borrow = (Tmp2 < Borrow) | (Tmp1 < TmpResult);
    Result[Index] = TmpResult;
  }
}

/**
  Propagates multiplicative Carry from the result at Index - 1 within Result.

  @param[in,out] A         The number to propagate Carry in.
  @param[in]     NumWords  The number of Words of A.
  @param[in]     Index     The index from which on to add Carry.
  @param[in]     Carry     The carry from the multiplication of Index - 1.

**/
STATIC
VOID
BigNumMulPropagateCarry (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     UINTN            Index,
  IN     OC_BN_WORD       Carry
  )
{
  OC_BN_WORD Tmp;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);

  for (; Index < NumWords && Carry != 0; ++Index) {
    Tmp = A[Index] + Carry;
    //
    // When an addition wraps around, the result must be smaller than either
    // operand.
    //
    Carry    = (Tmp < Carry);
    A[Index] = Tmp;
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
  UINT8      NumBits;
  OC_BN_WORD Mask;
  //
  // The values we are receiving are very likely large, thus this approach
  // should be reasonably fast.
  //
  NumBits = OC_BN_WORD_NUM_BITS;
  Mask    = (OC_BN_WORD)1U << (OC_BN_WORD_NUM_BITS - 1);
  while ((Word & Mask) == 0) {
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
  OC_BN_NUM_WORDS Index;

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
  IN UINTN             NumWords
  )
{
  OC_BN_NUM_BITS Index;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);

  Index = BigNumMostSignificantWord (A, NumWords);
  return ((Index * OC_BN_WORD_NUM_BITS) + BigNumSignificantBitsWord (A[Index]));
}

/**
  Calculates the product of A and B.

  @param[in,out] Result          The buffer to store the result in.
  @param[in]     NumWordsResult  The number of Words of Result.
  @param[in]     A               The multiplicant.
  @param[in]     NumWordsA       The number of Words of A.
  @param[in]     B               The multiplier.
  @param[in]     NumWordsB       The number of Words of B.

**/
STATIC
VOID
BigNumMul (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsResult,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     CONST OC_BN_WORD  *B,
  IN     OC_BN_NUM_WORDS   NumWordsB
  )
{
  //
  // Given a better modulo function, this is subject for removal.
  // This algorithm is based on a space-optimised version of the conventional
  // Long Multiplication.
  // https://en.wikipedia.org/wiki/Multiplication_algorithm#Optimizing_space_complexity
  //
  OC_BN_WORD CurWord;
  OC_BN_WORD MulHi;
  OC_BN_WORD MulLo;
  OC_BN_WORD CurCarry;

  UINTN      LengthA;
  UINTN      LengthB;
  UINTN      LengthTmp;

  UINTN      IndexRes;
  UINTN      IndexA;
  UINTN      IndexB;

  ASSERT (Result != NULL);
  ASSERT (NumWordsResult > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (B != NULL);
  ASSERT (NumWordsB > 0);
  ASSERT (A != Result);
  ASSERT (B != Result);

  BigNumAssign0 (Result, NumWordsResult);
  //
  // These additions cannot overflow because NumWords fits UINTN.
  //
  LengthA = BigNumMostSignificantWord (A, NumWordsA) + 1;
  LengthB = BigNumMostSignificantWord (B, NumWordsB) + 1;
  //
  // This cannot overflow for sane outputs due to the address space limitation.
  //
  ASSERT (LengthA + LengthB > LengthA);
  //
  // This is required to avoid overflows, which purely internal calls should
  // never produce.
  //
  ASSERT (LengthA + LengthB - 1 < NumWordsResult);

  CurCarry = 0;
  for (IndexRes = 0; IndexRes < LengthA + LengthB - 1; ++IndexRes) {
    //
    // Add the carry from the last iteration.
    //
    CurWord  = Result[IndexRes] + CurCarry;
    CurCarry = (CurWord < CurCarry);
    //
    // When IndexB is out of bounds for B, the value at the requested position
    // would be 0 and hence no calculation would be performed.
    //
    if (IndexRes < LengthB) {
      IndexA    = 0;
      LengthTmp = IndexRes + 1;
      IndexB    = IndexRes;
    } else {
      IndexA    = IndexRes - LengthB + 1;
      LengthTmp = LengthA;
      IndexB    = LengthB - 1;
    }

    for (; IndexA < LengthTmp; ++IndexA, --IndexB) {
      //
      // No arithmetics need to be performed when both operands are 0.
      //
      if ((A[IndexA] | B[IndexB]) == 0) {
        continue;
      }

      MulLo = BigNumWordMul (&MulHi, A[IndexA], B[IndexB]);
      //
      // FIXME:
      // This is hard to read and probably not optimal, however during
      // simplification of various operations, it turned out multiplication
      // itself is only required as part of the current modulo implementation.
      // Instead of cleaning and tweaking this algorithm, an optimised modulo
      // algorithm that does not depend on this multiplication algorithm should
      // be found and imported, rendering this function subject for removal.
      //
      CurCarry += MulHi;
      if (CurCarry < MulHi) {
        //
        // If the current Carry overflows, propagate a carry of maximum value
        // upwards and start counting anew.
        //
        BigNumMulPropagateCarry (
          Result,
          NumWordsResult,
          IndexRes + 1,
          OC_BN_MAX_VAL
          );
        ++CurCarry;
      }

      CurWord += MulLo;
      if (CurWord < MulLo) {
        CurCarry += 1;
        if (CurCarry < 1) {
          //
          // If the current Carry overflows, propagate a carry of maximum value
          // upwards and start counting anew.
          //
          BigNumMulPropagateCarry (
            Result,
            NumWordsResult,
            IndexRes + 1,
            OC_BN_MAX_VAL
            );
          ++CurCarry;
        }
      }
    }

    Result[IndexRes] = CurWord;
  }
  //
  // Set the MSB to the carry of the last iteration.
  //
  Result[IndexRes] = CurCarry;
}

VOID
BigNumOrWord (
  IN OUT OC_BN_WORD       *A,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     OC_BN_WORD       Value,
  IN     UINTN            Exponent
  )
{
  UINTN WordIndex;
  UINT8 NumBits;

  ASSERT (A != NULL);
  ASSERT (NumWords > 0);
  ASSERT (Exponent / OC_BN_WORD_NUM_BITS < NumWords);

  WordIndex = Exponent / OC_BN_WORD_NUM_BITS;
  if (WordIndex < NumWords) {
    NumBits = Exponent % OC_BN_WORD_NUM_BITS;
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
  UINTN Index;

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
  UINTN NumWords;
  UINT8 NumBits;

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
  Calculates the quotient of A and B.

  @param[in,out] Result        The buffer to return the result into.
  @param[in]     NumWordsRest  The number of Words of Result, A, DenonBuf and
                               DividenBuf.
  @param[in]     A             The dividend.
  @param[in]     B             The divisor.
  @param[in]     NumWordsB     The number of Words of B.
  @param[in]     DenomBuf      A temporary denominator buffer.
  @param[in]     DividendBuf   A temporary dividend buffer.

  @returns  Whether the operation was completes successfully.

**/
STATIC
VOID
BigNumDiv (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsRest,
  IN     CONST OC_BN_WORD  *A,
  IN     CONST OC_BN_WORD  *B,
  IN     OC_BN_NUM_WORDS   NumWordsB,
  IN     OC_BN_WORD        *DenomBuf,
  IN     OC_BN_WORD        *DividendBuf
  )
{
  //
  // As for multiplication, this is subject for removal.
  //
  UINTN   CurBitIndex;
  BOOLEAN Overflow;

  UINT32  NumBitsA;
  UINT32  NumBitsB;

  ASSERT (Result != NULL);
  ASSERT (NumWordsRest > 0);
  ASSERT (A != NULL);
  ASSERT (B != NULL);
  ASSERT (NumWordsB > 0);
  ASSERT (DenomBuf != NULL);
  ASSERT (DividendBuf != NULL);
  //
  // Use an integer of natural size to store the current Bit index as 'current'
  // is always a 2's potency. While a BIGNUM can theoretically hold a
  // 2's potency eight times larger than what can represent as Bit index with a
  // natural integer (Bytes vs Bits), this cannot happen within this function
  // as 'a' aligned to the next 2's potency would need to be just as big for
  // this to be the case. This cannot happen due to the address space
  // limitation.
  //
  CurBitIndex = 0;
  Overflow    = FALSE;
  //
  // Shift b to the left so it has the same amount of significant bits as a.
  // This would, without this speedup, be done on per-bit basis by the loop
  // below.
  //
  NumBitsA = BigNumSignificantBits (A, NumWordsRest);
  NumBitsB = BigNumSignificantBits (B, NumWordsB);
  if (NumBitsA > NumBitsB) {
    CurBitIndex = NumBitsA - NumBitsB;                                   // int Current = 1 << (numBitsA - numBitsB);
    BigNumLeftShift (DenomBuf, NumWordsRest, B, NumWordsB, CurBitIndex); // Denom = B << CurBitIndex
  } else {
    CurBitIndex = 0;                                                     // int Current = 1;
    BigNumAssign (DenomBuf, NumWordsRest, B, NumWordsB);                 // Denom = B
  }

  while (BigNumCmp (DenomBuf, NumWordsRest, A) <= 0) {                   // while (Denom <= a) {
    if (DenomBuf[NumWordsRest - 1] > (OC_BN_MAX_VAL / 2U)) {
      Overflow = TRUE;
      break;
    }
    ++CurBitIndex;                                                       //   Current <<= 1;                 
    BigNumLeftShiftBitsSmall (DenomBuf, NumWordsRest, 1);                //   Denom   <<= 1;
  }
  if (!Overflow) {
    BigNumRightShiftBitsSmall (DenomBuf, NumWordsRest, 1);               // Denom   >>= 1;
    --CurBitIndex;                                                       // Current >>= 1;                 
  }
  BigNumAssign0 (Result, NumWordsRest);                                  // int Result = 0;
  BigNumAssign (DividendBuf, NumWordsRest, A, NumWordsRest);             // Dividend = A
  //
  // currentBitIndex cannot add-wraparound to reach this value as reasoned in
  // the comment before.
  //
  while (CurBitIndex != (0ULL - 1ULL)) {                                 // while (Current != 0)
    if (BigNumCmp (DividendBuf, NumWordsRest, DenomBuf) >= 0) {          //   if (Dividend >= Denom)
      BigNumSub (DividendBuf, NumWordsRest, DividendBuf, DenomBuf);      //     Dividend -= denom;            
      BigNumOrWord (Result, NumWordsRest, 1, CurBitIndex);               //     Result |= current;
    }
    --CurBitIndex;                                                       //   Current >>= 1;
    BigNumRightShiftBitsSmall (DenomBuf, NumWordsRest, 1);               //   Denom >>= 1;
  }                                                                      // return Result;
}

BOOLEAN
BigNumMod (
  IN OUT OC_BN_WORD        *Result,
  IN     OC_BN_NUM_WORDS   NumWordsRest,
  IN     CONST OC_BN_WORD  *A,
  IN     OC_BN_NUM_WORDS   NumWordsA,
  IN     CONST OC_BN_WORD  *B
  )
{
  //
  // FIXME:
  // The algorithm is rather expensive and slow. It utilitises the current
  // suboptimal multiplication and division algorithms. An optimised algorithm
  // should be imported and formerly mentioned functions be removed as they'd
  // be dead code.
  //
  UINTN TempsBnSize;

  VOID       *Memory;
  OC_BN_WORD *TmpDiv;
  OC_BN_WORD *TmpMod;
  OC_BN_WORD *TmpDenom;
  OC_BN_WORD *TmpDividend;

  ASSERT (Result != NULL);
  ASSERT (NumWordsRest > 0);
  ASSERT (A != NULL);
  ASSERT (NumWordsA > 0);
  ASSERT (B != NULL);
  ASSERT (NumWordsA >= NumWordsRest);

  OC_STATIC_ASSERT (
    OC_BN_MAX_SIZE <= MAX_UINTN / 4,
    "An overflow verification must be added"
    );

  TempsBnSize = NumWordsA * OC_BN_WORD_SIZE;
  Memory      = AllocatePool (4 * TempsBnSize);
  if (Memory == NULL) {
    return FALSE;
  }

  TmpDiv      = (OC_BN_WORD *)Memory;
  TmpMod      = (OC_BN_WORD *)((UINTN)TmpDiv   + TempsBnSize);
  TmpDenom    = (OC_BN_WORD *)((UINTN)TmpMod   + TempsBnSize);
  TmpDividend = (OC_BN_WORD *)((UINTN)TmpDenom + TempsBnSize);

  BigNumDiv (TmpDiv, NumWordsA, A, B, NumWordsRest, TmpDenom, TmpDividend);
  BigNumMul (TmpMod, NumWordsA, TmpDiv, NumWordsA, B, NumWordsRest);
  BigNumSub (Result, NumWordsRest, A, TmpMod);

  FreePool (Memory);
  return TRUE;
}

VOID
BigNumParseBuffer (
  IN OUT OC_BN_WORD       *Result,
  IN     OC_BN_NUM_WORDS  NumWords,
  IN     CONST UINT8      *Buffer,
  IN     UINTN            BufferSize
  )
{
  UINTN      Index;
  OC_BN_WORD Tmp;

  ASSERT (Result != NULL);
  ASSERT (NumWords * OC_BN_WORD_SIZE == BufferSize);
  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);
  ASSERT ((BufferSize % OC_BN_WORD_SIZE) == 0);

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
