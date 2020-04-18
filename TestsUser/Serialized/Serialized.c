/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcConfigurationLib.h>

#include <sys/time.h>

EFI_STATUS
EFIAPI
Base64Decode (
  IN     CONST CHAR8 *Source          OPTIONAL,
  IN     UINTN       SourceSize,
  OUT    UINT8       *Destination     OPTIONAL,
  IN OUT UINTN       *DestinationSize
  )
{
  BOOLEAN PaddingMode;
  UINTN   SixBitGroupsConsumed;
  UINT32  Accumulator;
  UINTN   OriginalDestinationSize;
  UINTN   SourceIndex;
  CHAR8   SourceChar;
  UINT32  Base64Value;
  UINT8   DestinationOctet;

  if (DestinationSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check Source array validity.
  //
  if (Source == NULL) {
    if (SourceSize > 0) {
      //
      // At least one CHAR8 element at NULL Source.
      //
      return EFI_INVALID_PARAMETER;
    }
  } else if (SourceSize > MAX_ADDRESS - (UINTN)Source) {
    //
    // Non-NULL Source, but it wraps around.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check Destination array validity.
  //
  if (Destination == NULL) {
    if (*DestinationSize > 0) {
      //
      // At least one UINT8 element at NULL Destination.
      //
      return EFI_INVALID_PARAMETER;
    }
  } else if (*DestinationSize > MAX_ADDRESS - (UINTN)Destination) {
    //
    // Non-NULL Destination, but it wraps around.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for overlap.
  //
  if (Source != NULL && Destination != NULL) {
    //
    // Both arrays have been provided, and we know from earlier that each array
    // is valid in itself.
    //
    if ((UINTN)Source + SourceSize <= (UINTN)Destination) {
      //
      // Source array precedes Destination array, OK.
      //
    } else if ((UINTN)Destination + *DestinationSize <= (UINTN)Source) {
      //
      // Destination array precedes Source array, OK.
      //
    } else {
      //
      // Overlap.
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Decoding loop setup.
  //
  PaddingMode             = FALSE;
  SixBitGroupsConsumed    = 0;
  Accumulator             = 0;
  OriginalDestinationSize = *DestinationSize;
  *DestinationSize        = 0;

  //
  // Decoding loop.
  //
  for (SourceIndex = 0; SourceIndex < SourceSize; SourceIndex++) {
    SourceChar = Source[SourceIndex];

    //
    // Whitespace is ignored at all positions (regardless of padding mode).
    //
    if (SourceChar == '\t' || SourceChar == '\n' || SourceChar == '\v' ||
        SourceChar == '\f' || SourceChar == '\r' || SourceChar == ' ') {
      continue;
    }

    //
    // If we're in padding mode, accept another padding character, as long as
    // that padding character completes the quantum. This completes case (2)
    // from RFC4648, Chapter 4. "Base 64 Encoding":
    //
    // (2) The final quantum of encoding input is exactly 8 bits; here, the
    //     final unit of encoded output will be two characters followed by two
    //     "=" padding characters.
    //
    if (PaddingMode) {
      if (SourceChar == '=' && SixBitGroupsConsumed == 3) {
        SixBitGroupsConsumed = 0;
        continue;
      }
      return EFI_INVALID_PARAMETER;
    }

    //
    // When not in padding mode, decode Base64Value based on RFC4648, "Table 1:
    // The Base 64 Alphabet".
    //
    if ('A' <= SourceChar && SourceChar <= 'Z') {
      Base64Value = SourceChar - 'A';
    } else if ('a' <= SourceChar && SourceChar <= 'z') {
      Base64Value = 26 + (SourceChar - 'a');
    } else if ('0' <= SourceChar && SourceChar <= '9') {
      Base64Value = 52 + (SourceChar - '0');
    } else if (SourceChar == '+') {
      Base64Value = 62;
    } else if (SourceChar == '/') {
      Base64Value = 63;
    } else if (SourceChar == '=') {
      //
      // Enter padding mode.
      //
      PaddingMode = TRUE;

      if (SixBitGroupsConsumed == 2) {
        //
        // If we have consumed two 6-bit groups from the current quantum before
        // encountering the first padding character, then this is case (2) from
        // RFC4648, Chapter 4. "Base 64 Encoding". Bump SixBitGroupsConsumed,
        // and we'll enforce another padding character.
        //
        SixBitGroupsConsumed = 3;
      } else if (SixBitGroupsConsumed == 3) {
        //
        // If we have consumed three 6-bit groups from the current quantum
        // before encountering the first padding character, then this is case
        // (3) from RFC4648, Chapter 4. "Base 64 Encoding". The quantum is now
        // complete.
        //
        SixBitGroupsConsumed = 0;
      } else {
        //
        // Padding characters are not allowed at the first two positions of a
        // quantum.
        //
        return EFI_INVALID_PARAMETER;
      }

      //
      // Wherever in a quantum we enter padding mode, we enforce the padding
      // bits pending in the accumulator -- from the last 6-bit group just
      // preceding the padding character -- to be zero. Refer to RFC4648,
      // Chapter 3.5. "Canonical Encoding".
      //
      if (Accumulator != 0) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // Advance to the next source character.
      //
      continue;
    } else {
      //
      // Other characters outside of the encoding alphabet are rejected.
      //
      return EFI_INVALID_PARAMETER;
    }

    //
    // Feed the bits of the current 6-bit group of the quantum to the
    // accumulator.
    //
    Accumulator = (Accumulator << 6) | Base64Value;
    SixBitGroupsConsumed++;
    switch (SixBitGroupsConsumed) {
    case 1:
      //
      // No octet to spill after consuming the first 6-bit group of the
      // quantum; advance to the next source character.
      //
      continue;
    case 2:
      //
      // 12 bits accumulated (6 pending + 6 new); prepare for spilling an
      // octet. 4 bits remain pending.
      //
      DestinationOctet = (UINT8)(Accumulator >> 4);
      Accumulator &= 0xF;
      break;
    case 3:
      //
      // 10 bits accumulated (4 pending + 6 new); prepare for spilling an
      // octet. 2 bits remain pending.
      //
      DestinationOctet = (UINT8)(Accumulator >> 2);
      Accumulator &= 0x3;
      break;
    default:
      ASSERT (SixBitGroupsConsumed == 4);
      //
      // 8 bits accumulated (2 pending + 6 new); prepare for spilling an octet.
      // The quantum is complete, 0 bits remain pending.
      //
      DestinationOctet = (UINT8)Accumulator;
      Accumulator = 0;
      SixBitGroupsConsumed = 0;
      break;
    }

    //
    // Store the decoded octet if there's room left. Increment
    // (*DestinationSize) unconditionally.
    //
    if (*DestinationSize < OriginalDestinationSize) {
      ASSERT (Destination != NULL);
      Destination[*DestinationSize] = DestinationOctet;
    }
    (*DestinationSize)++;

    //
    // Advance to the next source character.
    //
  }

  //
  // If Source terminates mid-quantum, then Source is invalid.
  //
  if (SixBitGroupsConsumed != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Done.
  //
  if (*DestinationSize <= OriginalDestinationSize) {
    return EFI_SUCCESS;
  }
  return EFI_BUFFER_TOO_SMALL;
}

/*
 clang -g -fsanitize=undefined,address -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h Serialized.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcConfigurationLib/OcConfigurationLib.c -o Serialized

 for fuzzing:
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h Serialized.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcConfigurationLib/OcConfigurationLib.c -o Serialized
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp Serialized.plist DICT ; ./Serialized -jobs=4 DICT

 rm -rf Serialized.dSYM DICT fuzz*.log Serialized
*/


long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

uint8_t *readFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = 0;
  *size = fsize;

  return string;
}

int main(int argc, char** argv) {
  uint32_t f;
  uint8_t *b;
  if ((b = readFile(argc > 1 ? argv[1] : "Serialized.plist", &f)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  long long a = current_timestamp();

  OC_GLOBAL_CONFIG   Config;
  EFI_STATUS Status = OcConfigurationInit (&Config, b, f);

  DEBUG((EFI_D_ERROR, "Done in %llu ms\n", current_timestamp() - a));

  OcConfigurationFree (&Config);

  free(b);

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  VOID *NewData = AllocatePool (Size);
  if (NewData) {
    CopyMem (NewData, Data, Size);
    OC_GLOBAL_CONFIG   Config;
    OcConfigurationInit (&Config, NewData, Size);
    OcConfigurationFree (&Config);
    FreePool (NewData);
  }
  return 0;
}

