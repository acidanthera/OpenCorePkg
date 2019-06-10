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

//
// Base64 decoding algorithm implementation in C.
// Courtesy of wikibooks, licensed under Creative Commons license:
// https://creativecommons.org/licenses/by-sa/3.0/
//
// See: https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C
//
// Additional modifications include permitting tabulation and other whitespace
// characters to appear in the encoded data. The intention of those is to support
// Base64 data from property lists.
//

#define WHITESPACE 64
#define EQUALS     65
#define INVALID    66

STATIC CONST UINT8 D[] = {
  66,66,66,66,66,66,66,66,66,64,64,64,64,64,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,64,66,66,66,66,66,66,66,66,66,66,62,66,66,66,63,52,53,
  54,55,56,57,58,59,60,61,66,66,66,65,66,66,66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,66,66,66,66,66,66,26,27,28,
  29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66
};

RETURN_STATUS
EFIAPI
OcBase64Decode (
  IN     CONST CHAR8  *EncodedData,
  IN     UINTN        EncodedLength,
     OUT UINT8        *DecodedData,
  IN OUT UINTN        *DecodedLength
  )
{
  CONST CHAR8 *End = EncodedData + EncodedLength;
  CHAR8 Iter = 0;
  UINT32 Buf = 0;
  UINTN Len = 0;

  while (EncodedData < End) {
    UINT8 C = D[(UINT8)(*EncodedData++)];

    switch (C) {
      case WHITESPACE:
        continue;       /* skip whitespace */
      case INVALID:
        return RETURN_INVALID_PARAMETER;   /* invalid input, return error */
      case EQUALS:      /* pad character, end of data */
        EncodedData = End;
        continue;
      default:
        Buf = Buf << 6U | C;
        Iter++; /* increment the number of iteration */
        /* If the buffer is full, split it into bytes */
        if (Iter == 4) {
          if ((Len += 3) > *DecodedLength) return RETURN_BUFFER_TOO_SMALL; /* buffer overflow */
          *(DecodedData++) = (Buf >> 16U) & 255U;
          *(DecodedData++) = (Buf >> 8U) & 255U;
          *(DecodedData++) = Buf & 255U;
          Buf = 0; Iter = 0;
        }
    }
  }

  if (Iter == 3) {
    if ((Len += 2) > *DecodedLength) return RETURN_BUFFER_TOO_SMALL; /* buffer overflow */
    *(DecodedData++) = (Buf >> 10U) & 255U;
    *(DecodedData++) = (Buf >> 2U) & 255U;
  } else if (Iter == 2) {
    if (++Len > *DecodedLength) return RETURN_BUFFER_TOO_SMALL; /* buffer overflow */
    *(DecodedData++) = (Buf >> 4U) & 255U;
  }

  *DecodedLength = Len; /* modify to reflect the actual output size */
  return EFI_SUCCESS;
}
