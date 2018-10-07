/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>

STATIC CONST INT8 mDecoding[] = {
  62,
  -1,
  -1,
  -1,
  63,
  52,
  53,
  54,
  55,
  56,
  57,
  58,
  59,
  60,
  61,
  -1,
  -1,
  -1,
  -2,
  -1,
  -1,
  -1,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  26,
  27,
  28,
  29,
  30,
  31,
  32,
  33,
  34,
  35,
  36,
  37,
  38,
  39,
  40,
  41,
  42,
  43,
  44,
  45,
  46,
  47,
  48,
  49,
  50,
  51
};

// BASE64_DECODE_STEP
typedef enum {
  StepA,
  StepB,
  StepC,
  StepD
} BASE64_DECODE_STEP;

// BASE64_DECODE_STATE
typedef struct {
  BASE64_DECODE_STEP Step;
  UINT8              PlainChar;
} BASE64_DECODE_STATE;

INT32
Base64DecodeValue (
  IN UINT8  Value
  )
{
  INT32 Result;

  Value -= 43;
  Result = -1;

  if ((Value >= 0) && (Value <= sizeof (mDecoding))) {
    Result =mDecoding[Value];
  }

  return Result;
}

VOID
Base64InitDecodeState (
  IN BASE64_DECODE_STATE  *StateIn
  )
{
  StateIn->Step      = StepA;
  StateIn->PlainChar = 0;
}

UINTN
Base64DecodeBlock (
  IN CONST UINT8          *Code,
  IN UINTN                Size,
  IN UINT8                *PlainText,
  IN BASE64_DECODE_STATE  *State
  )
{
  CONST UINT8 *codechar = Code;

  UINT8       *plainchar = PlainText;
  INT32       fragment;

  *plainchar = State->PlainChar;

  switch (State->Step) {
    while (TRUE) {
      case StepA:
      {
        do {
          if (codechar == Code + Size) {
            State->Step      = StepA;
            State->PlainChar = *plainchar;

            return (plainchar - PlainText);
          }

          fragment = Base64DecodeValue (*codechar++);
        } while (fragment < 0);

        *plainchar = ((fragment & 0x3F) << 2);
      }

      case StepB:
      {
        do {
          if (codechar == Code + Size) {
            State->Step      = StepB;
            State->PlainChar = *plainchar;

            return (plainchar - PlainText);
          }
          fragment = Base64DecodeValue (*codechar++);
        } while (fragment < 0);

        *plainchar++ |= (fragment & 0x030) >> 4;
        *plainchar = (fragment & 0x00f) << 4;
      }

      case StepC:
      {
        do {
          if (codechar == Code + Size) {
            State->Step = StepC;
            State->PlainChar = *plainchar;

            return (plainchar - PlainText);
          }

          fragment = Base64DecodeValue (*codechar++);
        } while (fragment < 0);

        *plainchar++ |= (fragment & 0x03c) >> 2;
        *plainchar = (fragment & 0x003) << 6;
      }

      case StepD:
      {
        do {
          if (codechar == (Code + Size)) {
            State->Step      = StepD;
            State->PlainChar = *plainchar;

            return (plainchar - PlainText);
          }

          fragment = Base64DecodeValue (*codechar++);
        } while (fragment < 0);

        *plainchar++ |= (fragment & 0x03f);
      }
    }
  }

  // control should not reach here
  return plainchar - PlainText;
}

/** UEFI interface to base64 decode.

  Decodes EncodedData into a new allocated buffer and returns it. Caller is responsible to FreePool() it.
  If DecodedSize != NULL, then size of decoded data is put there.
**/
UINT8 *
Base64Decode (
  IN  UINT8  *EncodedData,
  IN  UINTN  EncodedDataLength,
  OUT UINTN  *DecodedSize
  )
{
  UINT8               *DecodedData;

  BASE64_DECODE_STATE DecodeState;

  DecodedData = NULL;

  if ((EncodedData != NULL) && (EncodedDataLength != 0)) {
    // to simplify, we'll allocate the same size, although smaller size is needed
    DecodedData = AllocateZeroPool (EncodedDataLength);

    Base64InitDecodeState (&DecodeState);

    *DecodedSize = Base64DecodeBlock (
                     EncodedData,
                     EncodedDataLength,
                     DecodedData,
                     &DecodeState
                     );

    if (*DecodedSize == 0) {
      DecodedData  = NULL;
    }
  }

  return DecodedData;
}
