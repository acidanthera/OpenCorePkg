/** @file

OcCryptoLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**

Copyright (c) 2014-2018, kokke

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

**/

/**

This is an implementation of the AES algorithm, specifically CTR and CBC mode.
Block size can be chosen in OcCryptoLib.h.

The implementation is verified against the test vectors in:
  National Institute of Standards and Technology Special Publication 800-38A 2001 ED


NOTE:   String length must be evenly divisible by 16byte (str_len % 16 == 0)
        You should pad the end of the string with zeros if this is not the case.
        For AES192/256 the key size is proportionally larger.

**/

#include <Library/BaseMemoryLib.h>
#include <Library/OcCryptoLib.h>

//
// The number of columns comprising a state in AES (Nb). This is a CONSTant in AES. Value=4
// The number of 32 bit words in a key (Nk).
// The number of rounds in AES Cipher (Nr).
//

#define Nb 4

#if CONFIG_AES_KEY_SIZE == 32
#define Nk 8
#define Nr 14
#elif CONFIG_AES_KEY_SIZE == 24
#define Nk 6
#define Nr 12
#elif CONFIG_AES_KEY_SIZE == 16
#define Nk 4
#define Nr 10
#endif

//
// state - array holding the intermediate results during decryption.
//
typedef UINT8 AES_INTERNAL_STATE[4][4];

//
// The lookup-tables are marked CONST so they can be placed in read-only storage instead of RAM
// The numbers below can be computed dynamically trading ROM for RAM -
// This can be useful in (embedded) bootloader applications, where ROM is often limited.
//
STATIC CONST UINT8 Sbox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

STATIC CONST UINT8 RsBox[256] = {
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

//
// The round CONSTant word array, Rcon[i], contains the values given by
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)
//
STATIC CONST UINT8 Rcon[11] = {
  0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/*
 * Jordan Goulder points out in PR #12 (https://github.com/kokke/tiny-AES-C/pull/12),
 * that you can remove most of the elements in the Rcon array, because they are unused.
 *
 * From Wikipedia's article on the Rijndael key schedule @ https://en.wikipedia.org/wiki/Rijndael_key_schedule#Rcon
 *
 * "Only the first some of these CONSTants are actually used – up to rcon[10] for AES-128 (as 11 round keys are needed),
 *  up to rcon[8] for AES-192, up to rcon[7] for AES-256. rcon[0] is not used in AES algorithm."
 */


//
// Private functions:
//
#define GetSboxValue(num) (Sbox[(num)])
#define GetSBoxInvert(num) (RsBox[(num)])

//
// This function produces Nb(Nr+1) round keys. The round keys are used in each
// round to decrypt the states.
//
STATIC
VOID
KeyExpansion (
  OUT UINT8        *RoundKey,
  IN  CONST UINT8  *Key
  )
{
  UINT32 Index, J, K;
  //
  // Used for the column/row operations
  //
  UINT8 TempA[4];

  //
  // The first round key is the key itself.
  //
  for (Index = 0; Index < Nk; ++Index) {
    RoundKey[(Index * 4) + 0] = Key[(Index * 4) + 0];
    RoundKey[(Index * 4) + 1] = Key[(Index * 4) + 1];
    RoundKey[(Index * 4) + 2] = Key[(Index * 4) + 2];
    RoundKey[(Index * 4) + 3] = Key[(Index * 4) + 3];
  }

  //
  // All other round keys are found from the previous round keys.
  //
  for (Index = Nk; Index < Nb * (Nr + 1); ++Index) {
    K = (Index - 1) * 4;
    TempA[0] = RoundKey[K + 0];
    TempA[1] = RoundKey[K + 1];
    TempA[2] = RoundKey[K + 2];
    TempA[3] = RoundKey[K + 3];

    if (Index % Nk == 0) {
      //
      // This function shifts the 4 bytes in a word to the left once.
      // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]
      //

      //
      // Function RotWord()
      //
      K = TempA[0];
      TempA[0] = TempA[1];
      TempA[1] = TempA[2];
      TempA[2] = TempA[3];
      TempA[3] = (UINT8) K;

      //
      // SubWord() is a function that takes a four-byte input word and
      // applies the S-box to each of the four bytes to produce an output word.
      //

      //
      // Function Subword()
      //
      TempA[0] = GetSboxValue (TempA[0]);
      TempA[1] = GetSboxValue (TempA[1]);
      TempA[2] = GetSboxValue (TempA[2]);
      TempA[3] = GetSboxValue (TempA[3]);

      TempA[0] = TempA[0] ^ Rcon[Index / Nk];
    }

#if CONFIG_AES_KEY_SIZE == 32
    if (Index % Nk == 4) {
      //
      // Function Subword()
      //
      TempA[0] = GetSboxValue (TempA[0]);
      TempA[1] = GetSboxValue (TempA[1]);
      TempA[2] = GetSboxValue (TempA[2]);
      TempA[3] = GetSboxValue (TempA[3]);
    }
#endif

    J = Index * 4; K = (Index - Nk) * 4;
    RoundKey[J + 0] = RoundKey[K + 0] ^ TempA[0];
    RoundKey[J + 1] = RoundKey[K + 1] ^ TempA[1];
    RoundKey[J + 2] = RoundKey[K + 2] ^ TempA[2];
    RoundKey[J + 3] = RoundKey[K + 3] ^ TempA[3];
  }
}

VOID
AesInitCtxIv (
  OUT AES_CONTEXT  *Context,
  IN  CONST UINT8  *Key,
  IN  CONST UINT8  *Iv
  )
{
  KeyExpansion (Context->RoundKey, Key);
  CopyMem (Context->Iv, Iv, AES_BLOCK_SIZE);
}

VOID
AesSetCtxIv (
  OUT AES_CONTEXT  *Context,
  IN  CONST UINT8  *Iv
  )
{
  CopyMem (Context->Iv, Iv, AES_BLOCK_SIZE);
}

//
// This function adds the round key to state.
// The round key is added to the state by an XOR function.
//
STATIC
VOID
AddRoundKey (
  IN     UINT8               Round,
  IN OUT AES_INTERNAL_STATE  *State,
  IN     CONST UINT8         *RoundKey
  )
{
  UINT8  I, J;

  for (I = 0; I < 4; ++I) {
    for (J = 0; J < 4; ++J) {
      (*State)[I][J] ^= RoundKey[(Round * Nb * 4) + (I * Nb) + J];
    }
  }
}

//
// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
//
STATIC 
VOID
SubBytes (
  IN OUT AES_INTERNAL_STATE  *State
  )
{
  UINT8  I, J;

  for (I = 0; I < 4; ++I) {
    for (J = 0; J < 4; ++J) {
      (*State)[J][I] = GetSboxValue((*State)[J][I]);
    }
  }
}

//
// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
//
STATIC 
VOID
ShiftRows (
  IN OUT AES_INTERNAL_STATE  *State
  )
{
  UINT8  Temp;

  //
  // Rotate first row 1 columns to left
  //
  Temp           = (*State)[0][1];
  (*State)[0][1] = (*State)[1][1];
  (*State)[1][1] = (*State)[2][1];
  (*State)[2][1] = (*State)[3][1];
  (*State)[3][1] = Temp;

  //
  // Rotate second row 2 columns to left
  //
  Temp           = (*State)[0][2];
  (*State)[0][2] = (*State)[2][2];
  (*State)[2][2] = Temp;

  Temp           = (*State)[1][2];
  (*State)[1][2] = (*State)[3][2];
  (*State)[3][2] = Temp;

  //
  // Rotate third row 3 columns to left
  //
  Temp           = (*State)[0][3];
  (*State)[0][3] = (*State)[3][3];
  (*State)[3][3] = (*State)[2][3];
  (*State)[2][3] = (*State)[1][3];
  (*State)[1][3] = Temp;
}

STATIC 
UINT8
XTime (
  IN UINT8 X
  )
{
  return (UINT8) (((UINT32) X << 1u) ^ ((((UINT32) X >> 7u) & 1u) * 0x1bu));
}

//
// MixColumns function mixes the columns of the state matrix
//
STATIC 
VOID
MixColumns (
  IN OUT AES_INTERNAL_STATE  *State
  )
{
  UINT8  I, Tmp, Tm, T;

  for (I = 0; I < 4; ++I) {
    T   = (*State)[I][0];
    Tmp = (UINT8) ((UINT32) ((*State) [I][0]) ^ (UINT32) ((*State) [I][1])
          ^ (UINT32) ((*State) [I][2]) ^ (UINT32) ((*State) [I][3]));
    Tm  = (*State) [I][0] ^ (*State) [I][1];
    Tm = XTime (Tm);
    (*State) [I][0] ^= Tm ^ Tmp;
    Tm  = (*State)[I][1] ^ (*State)[I][2];
    Tm = XTime (Tm);
    (*State) [I][1] ^= Tm ^ Tmp;
    Tm  = (*State) [I][2] ^ (*State) [I][3];
    Tm = XTime (Tm);
    (*State) [I][2] ^= Tm ^ Tmp;
    Tm  = (*State)[I][3] ^ T ;
    Tm = XTime (Tm);
    (*State) [I][3] ^= Tm ^ Tmp ;
  }
}

//
// Multiply is used to multiply numbers in the field GF(2^8)
// Note: The last call to XTime() is unneeded, but often ends up generating a smaller binary
//       The compiler seems to be able to vectorize the operation better this way.
//       See https://github.com/kokke/tiny-AES-c/pull/34
//
#define Multiply(x, y)                                    \
      (   (((y) & 1u) * (x)) ^                            \
      (((y)>>1u & 1u) * XTime(x)) ^                       \
      (((y)>>2u & 1u) * XTime(XTime(x))) ^                \
      (((y)>>3u & 1u) * XTime(XTime(XTime(x)))) ^         \
      (((y)>>4u & 1u) * XTime(XTime(XTime(XTime(x))))))   \

//
// MixColumns function mixes the columns of the state matrix.
// The method used to multiply may be difficult to understand for the inexperienced.
// Please use the references to gain more information.
//
STATIC 
VOID
InvMixColumns (
  IN OUT AES_INTERNAL_STATE  *State
  )
{
  UINT8  I, A, B, C, D;

  for (I = 0; I < 4; ++I) {
    A = (*State) [I][0];
    B = (*State) [I][1];
    C = (*State) [I][2];
    D = (*State) [I][3];

    (*State)[I][0] = (UINT8) ((UINT32) Multiply(A, 0x0eu) ^ (UINT32) Multiply(B, 0x0bu)
                     ^ (UINT32) Multiply(C, 0x0du) ^ (UINT32) Multiply(D, 0x09u));
    (*State)[I][1] = (UINT8) ((UINT32) Multiply(A, 0x09u) ^ (UINT32) Multiply(B, 0x0eu)
                     ^ (UINT32) Multiply(C, 0x0bu) ^ (UINT32) Multiply(D, 0x0du));
    (*State)[I][2] = (UINT8) ((UINT32) Multiply(A, 0x0du) ^ (UINT32) Multiply(B, 0x09u)
                     ^ (UINT32) Multiply(C, 0x0eu) ^ (UINT32) Multiply(D, 0x0bu));
    (*State)[I][3] = (UINT8) ((UINT32) Multiply(A, 0x0bu) ^ (UINT32) Multiply(B, 0x0du)
                     ^ (UINT32) Multiply(C, 0x09u) ^ (UINT32) Multiply(D, 0x0eu));
  }
}

//
// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
//
STATIC
VOID
InvSubBytes (
  IN OUT AES_INTERNAL_STATE  *State
  )
{
  UINT8  I, J;

  for (I = 0; I < 4; ++I) {
    for (J = 0; J < 4; ++J) {
      (*State)[J][I] = GetSBoxInvert ((*State)[J][I]);
    }
  }
}

STATIC
VOID
InvShiftRows (
  IN OUT AES_INTERNAL_STATE  *State
  )
{
  UINT8  Temp;

  //
  // Rotate first row 1 columns to right
  //
  Temp = (*State)[3][1];
  (*State)[3][1] = (*State)[2][1];
  (*State)[2][1] = (*State)[1][1];
  (*State)[1][1] = (*State)[0][1];
  (*State)[0][1] = Temp;

  //
  // Rotate second row 2 columns to right
  //
  Temp = (*State)[0][2];
  (*State)[0][2] = (*State)[2][2];
  (*State)[2][2] = Temp;

  Temp = (*State)[1][2];
  (*State)[1][2] = (*State)[3][2];
  (*State)[3][2] = Temp;

  //
  // Rotate third row 3 columns to right
  //
  Temp = (*State)[0][3];
  (*State)[0][3] = (*State)[1][3];
  (*State)[1][3] = (*State)[2][3];
  (*State)[2][3] = (*State)[3][3];
  (*State)[3][3] = Temp;
}

//
// Cipher is the main function that encrypts the PlainText.
//
STATIC
VOID
Cipher (
  IN OUT AES_INTERNAL_STATE  *State,
  IN     CONST UINT8         *RoundKey
  )
{
  UINT8  Round;

  //
  // Add the First round key to the state before starting the rounds.
  //
  AddRoundKey(0, State, RoundKey);

  //
  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  //
  for (Round = 1; Round < Nr; ++Round) {
    SubBytes (State);
    ShiftRows (State);
    MixColumns (State);
    AddRoundKey (Round, State, RoundKey);
  }

  //
  // The last round is given below.
  // The MixColumns function is not here in the last round.
  //
  SubBytes (State);
  ShiftRows (State);
  AddRoundKey (Nr, State, RoundKey);
}

STATIC
VOID
InvCipher (
  IN OUT AES_INTERNAL_STATE  *State,
  IN     CONST UINT8         *RoundKey
  )
{
  UINT8  Round;

  //
  // Add the First round key to the state before starting the rounds.
  //
  AddRoundKey (Nr, State, RoundKey);

  //
  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  //
  for (Round = (Nr - 1); Round > 0; --Round) {
    InvShiftRows (State);
    InvSubBytes (State);
    AddRoundKey (Round, State, RoundKey);
    InvMixColumns (State);
  }

  //
  // The last round is given below.
  // The MixColumns function is not here in the last round.
  //
  InvShiftRows (State);
  InvSubBytes (State);
  AddRoundKey (0, State, RoundKey);
}

STATIC
VOID
XorWithIv (
  IN OUT UINT8       *Buf,
  IN     CONST UINT8 *Iv
  )
{
  UINT8  I;

  //
  // The block in AES is always 128bit no matter the key size
  //
  for (I = 0; I < AES_BLOCK_SIZE; ++I) {
    Buf[I] ^= Iv[I];
  }
}

//
// Public functions
//

VOID
AesCbcEncryptBuffer (
  IN OUT AES_CONTEXT  *Context,
  IN OUT UINT8        *Data,
  IN     UINT32       Len
  )
{
  UINT32  I;
  UINT8   *Iv;

  Iv = Context->Iv;

  for (I = 0; I < Len; I += AES_BLOCK_SIZE) {
    XorWithIv (Data, Iv);
    Cipher ((AES_INTERNAL_STATE *) Data, Context->RoundKey);
    Iv = Data;
    Data += AES_BLOCK_SIZE;
  }

  //
  // Store Iv in Context for next call
  //
  CopyMem (Context->Iv, Iv, AES_BLOCK_SIZE);
}

VOID
AesCbcDecryptBuffer (
  IN OUT AES_CONTEXT  *Context,
  IN OUT UINT8        *Data,
  IN     UINT32       Len
  )
{
  UINT32  I;
  UINT8   StoreNextIv[AES_BLOCK_SIZE];

  for (I = 0; I < Len; I += AES_BLOCK_SIZE) {
    CopyMem (StoreNextIv, Data, AES_BLOCK_SIZE);
    InvCipher ((AES_INTERNAL_STATE *) Data, Context->RoundKey);
    XorWithIv (Data, Context->Iv);
    CopyMem (Context->Iv, StoreNextIv, AES_BLOCK_SIZE);
    Data += AES_BLOCK_SIZE;
  }
}

//
// Symmetrical operation: same function for encrypting as for decrypting.
// Note any IV/nonce should never be reused with the same key
//
VOID
AesCtrXcryptBuffer (
  IN OUT AES_CONTEXT  *Context,
  IN OUT UINT8        *Data,
  IN     UINT32       Len
  )
{
  UINT8  Buffer[AES_BLOCK_SIZE];
  UINT32 I;
  INT32  Bi;

  for (I = 0, Bi = AES_BLOCK_SIZE; I < Len; ++I, ++Bi) {
    //
    // We need to regen xor compliment in buffer
    //
    if (Bi == AES_BLOCK_SIZE) {
      CopyMem (Buffer, Context->Iv, AES_BLOCK_SIZE);
      Cipher ((AES_INTERNAL_STATE *) Buffer, Context->RoundKey);

      //
      // Increment Iv and handle overflow
      //
      for (Bi = (AES_BLOCK_SIZE - 1); Bi >= 0; --Bi) {
        //
        // Inc will owerflow
        //
        if (Context->Iv[Bi] == 255) {
          Context->Iv[Bi] = 0;
          continue;
        }

        Context->Iv[Bi] += 1;
        break;
      }

      Bi = 0;
    }

    Data[I] = (Data[I] ^ Buffer[Bi]);
  }
}
