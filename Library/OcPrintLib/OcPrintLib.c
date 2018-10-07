/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/OcLog.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcPrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "GuidTable.h"

// OcFillBuffer
/** Internal function that places the character into the Buffer.

  Internal function that places ASCII or Unicode character into the Buffer.

  @param[in, out] Buffer     Buffer to place the Unicode or ASCII string.
  @param[in]      EndBuffer  The end of the input Buffer. No characters will be placed after that.
  @param[in]      Length     Count of character to be placed into Buffer.
  @param[in]      Character  Character to be placed into Buffer.
  @param[in]      Increment  Character increment in Buffer.

  @return  Number of characters printed.
**/
CHAR8 *
OcFillBuffer (
  IN OUT CHAR8  *Buffer,
  IN     CHAR8  *EndBuffer,
  IN     INTN   Length,
  IN     UINTN  Character,
  IN     UINTN   Increment
  )
{
  INTN Index;

  for (Index = 0; (Index < Length) && (Buffer < EndBuffer); ++Index) {
    Buffer[0] = (CHAR8) Character;
    Buffer[1] = (CHAR8) (Character >> 8);
    Buffer   += Increment;
  }

  return Buffer;
}

// OcValueToString
/** Internal function that convert a decimal number to a string in Buffer.

  Print worker function that convert a decimal number to a string in Buffer.

  @param[in, out] Buffer  Location to place the Unicode or ASCII string of Value.
  @param[in]      Value   Value to convert to a Decimal or Hexidecimal string in Buffer.
  @param[in]      Radix   Radix of the value

  @return  Number of characters printed.
**/
UINTN
OcValueToString (
  IN OUT CHAR8    *Buffer,
  IN     INT64    Value,
  IN     UINTN    Radix,
  IN     BOOLEAN  UpperCase
  )
{
  UINTN  Digits;

  UINT32 Remainder;
  CHAR8  Digit;

  // Loop to convert one digit at a time in reverse order

  *Buffer = 0;
  Digits  = 0;

  ++Buffer;

  do {
    Value   = (INT64)DivU64x32Remainder ((UINT64)Value, (UINT32)Radix, &Remainder);
    Digit   = mOcPrintLibHexStr[Remainder];
    *Buffer = (CHAR8)(UpperCase ? (((Digit >= 'A') && (Digit <= 'Z')) ? (Digit + ('a' - 'A')) : Digit) : Digit);

    ++Buffer;
    ++Digits;
  } while (Value != 0);

  return Digits;
}

// OcVSPrint
/**

  @param[out] Buffer      A pointer to the character buffer to print the results of the parsing of Format into.
  @param[in]  BufferSize  Maximum number of characters to put into buffer. Zero means no limit.
  @param[in]  Flags       Intial flags value. Can only have FORMAT_UNICODE and OUTPUT_UNICODE set.
  @param[in]  Format      A pointer a Null-terminated format string which describes the VA_ARGS list.
  @param[in]  Marker      A pointer to the VA_ARGS list

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
UINTN
EFIAPI
OcVSPrint (
  OUT CHAR8        *Buffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      Marker
  )
{
  UINTN       Result;

  CHAR8       *OriginalBuffer;
  CHAR8       *EndBuffer;
  CHAR8       ValueBuffer[MAXIMUM_VALUE_CHARACTERS];
  UINTN       BytesPerOutputCharacter;
  UINTN       BytesPerFormatCharacter;
  UINTN       FormatMask;
  UINTN       FormatCharacter;
  UINTN       Width;
  UINTN       Precision;
  INT64       Value;
  CONST CHAR8 *ArgumentString;
  UINTN       Character;
  EFI_GUID    *TmpGuid;
  EFI_TIME    *TmpTime;
  UINTN       Count;
  UINTN       ArgumentMask;
  INTN        BytesPerArgumentCharacter;
  UINTN       ArgumentCharacter;
  BOOLEAN     Done;
  UINTN       Index;
  CHAR8       Prefix;
  BOOLEAN     ZeroPad;
  BOOLEAN     Comma;
  UINTN       Digits;
  UINTN       Radix;
  EFI_STATUS  Status;

  Result = 0;

  if (BufferSize != 0) {
    //ASSERT (Buffer != NULL);

    BytesPerOutputCharacter = (((Flags & OUTPUT_UNICODE) != 0) ? 2 : 1);

    // Reserve space for the Null terminator.

    --BufferSize;
    OriginalBuffer = Buffer;

    // Set the tag for the end of the input Buffer.
    EndBuffer = (Buffer + (BufferSize * BytesPerOutputCharacter));

    if ((Flags & FORMAT_UNICODE) != 0) {
      // Make sure format string cannot contain more than PcdMaximumUnicodeStringLength
      // Unicode characters if PcdMaximumUnicodeStringLength is not zero.

      //ASSERT (StrSize ((CHAR16 *) Format) != 0);
      BytesPerFormatCharacter = sizeof (CHAR16);
      FormatMask              = MAX_UINT16;
    } else {
      // Make sure format string cannot contain more than PcdMaximumAsciiStringLength
      // Ascii characters if PcdMaximumAsciiStringLength is not zero.

      //ASSERT (AsciiStrSize (Format) != 0);
      BytesPerFormatCharacter = sizeof (CHAR8);
      FormatMask              = MAX_UINT8;
    }

    // Get the first character from the format string
    FormatCharacter = ((*Format | (*(Format + 1) << 8)) & FormatMask);

    // Loop until the end of the format string is reached or the output buffer is full
    while ((FormatCharacter != 0) && (Buffer < EndBuffer)) {
      // Clear all the flag bits except those that may have been passed in
      Flags &= (OUTPUT_UNICODE | FORMAT_UNICODE);

      // Set the default width to zero, and the default precision to 1

      Width     = 0;
      Precision = 1;
      Prefix    = 0;
      Comma     = FALSE;
      ZeroPad   = FALSE;
      Count     = 0;
      Digits    = 0;

      switch (FormatCharacter) {
        case '%':
        {
          // Parse Flags and Width

          Done = FALSE;

          while (!Done) {
            Format         += BytesPerFormatCharacter;
            FormatCharacter = ((*Format | (*(Format + 1) << 8)) & FormatMask);

            switch (FormatCharacter) {
              case '.':
              {
                Flags |= PRECISION;
                break;
              }

              case '-':
              {
                Flags |= LEFT_JUSTIFY;
                break;
              }

              case '+':
              {
                Flags |= PREFIX_SIGN;
                break;
              }

              case ' ':
              {
                Flags |= PREFIX_BLANK;
                break;
              }

              case ',':
              {
                Flags |= COMMA_TYPE;
                break;
              }

              case 'L':
              case 'l':
              {
                Flags |= LONG_TYPE;
                break;
              }

              case '*':
              {
                if ((Flags & PRECISION) == 0) {
                  Flags |= PAD_TO_WIDTH;
                  Width  = VA_ARG (Marker, UINTN);
                } else {
                  Precision = VA_ARG (Marker, UINTN);
                }

                break;
              }

              case '0':
              {
                if ((Flags & PRECISION) == 0) {
                  Flags |= PREFIX_ZERO;
                }
              }

              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
              {
                Count = 0;

                while ((FormatCharacter >= '0') && (FormatCharacter <= '9')) {
                  Count           = ((Count * 10) + (FormatCharacter - '0'));
                  Format         += BytesPerFormatCharacter;
                  FormatCharacter = (*Format | (*(Format + 1) << 8)) & FormatMask;
                }

                Format -= BytesPerFormatCharacter;

                if ((Flags & PRECISION) == 0) {
                  Flags |= PAD_TO_WIDTH;
                  Width  = Count;
                } else {
                  Precision = Count;
                }

                break;
              }

              case '\0':
              {
                // Make no output if Format string terminates unexpectedly when
                // looking up for flag, width, precision and type.

                Format   -= BytesPerFormatCharacter;
                Precision = 0;

                // break skipped on purpose.
              }

              default:
              {
                Done = TRUE;
                break;
              }
            }
          }

          // Handle each argument type

          switch (FormatCharacter) {
            case 'p':
            {
              // Flag space, +, 0, L & l are invalid for type p.
              Flags &= ~(PREFIX_BLANK | PREFIX_SIGN | PREFIX_ZERO | LONG_TYPE);

              if (sizeof (VOID *) > sizeof (UINT32)) {
                Flags |= LONG_TYPE;
              }
            }

            case 'X':
            {
              Flags |= UPPER_CASE_HEX;

              // break skipped on purpose
            }

            case 'x':
            {
              Flags |= RADIX_HEX;

              // break skipped on purpose
            }

            case 'd':
            {
              if ((Flags & LONG_TYPE) == 0) {
                Value = (VA_ARG (Marker, INT32));
              } else {
                Value = VA_ARG (Marker, INT64);
              }

              if ((Flags & PREFIX_BLANK) != 0) {
                Prefix = ' ';
              }

              if ((Flags & PREFIX_SIGN) != 0) {
                Prefix = '+';
              }

              if ((Flags & COMMA_TYPE) != 0) {
                Comma = TRUE;
              }

              if ((Flags & RADIX_HEX) == 0) {
                Radix = 10;

                if (Comma) {
                  Flags    &= (~PREFIX_ZERO);
                  Precision = 1;
                }

                if (Value < 0) {
                  Flags |= PREFIX_SIGN;
                  Prefix = '-';
                  Value  = -Value;
                }
              } else {
                Radix = 16;
                Comma = FALSE;

                if (((Flags & LONG_TYPE) == 0) && (Value < 0)) {
                  Value = (UINT32)Value;
                }
              }

              // Convert Value to a reversed string

              Count = OcValueToString (ValueBuffer, Value, Radix, ((Flags & UPPER_CASE_HEX) == 0));

              if ((Value == 0) && (Precision == 0)) {
                Count = 0;
              }

              ArgumentString = ((CHAR8 *)ValueBuffer + Count);
              Digits         = (Count % 3);

              if (Digits != 0) {
                Digits = (3 - Digits);
              }

              if (Comma && (Count != 0)) {
                Count += ((Count - 1) / 3);
              }

              if (Prefix != 0) {
                ++Count;
                ++Precision;
              }

              Flags  |= ARGUMENT_REVERSED;
              ZeroPad = TRUE;

              if (((Flags & (PREFIX_ZERO)) != 0)
               && ((Flags & (PAD_TO_WIDTH)) != 0)
               && ((Flags & (LEFT_JUSTIFY | PRECISION)) == 0)) {
                Precision = Width;
              }

              break;
            }

            case 'b':
            {
              if ((Flags & LONG_TYPE) == 0) {
                Value = (VA_ARG (Marker, INT32));
              } else {
                Value = VA_ARG (Marker, INT64);
              }

              // Calculate Precision

              if ((Flags & PRECISION) == 0) {
                if ((Value > MAX_UINT32) || ((Flags & LONG_TYPE) != 0)) {
                  Precision = 64;
                } else if (Value > MAX_UINT16) {
                  Precision = 32;
                } else if (Value > MAX_UINT8) {
                  Precision = 16;
                } else {
                  Precision = 8;
                }
              }

              Precision = MIN (64, Precision);

              // Convert Value to a reversed binary string

              for (Count = 0; Count < Precision; ++Count) {
                ValueBuffer[Count] = ((Value & 1) != 0)  ? '1' : '0';
                Value            >>= 1;
              }

              if ((Value == 0) && (Precision == 0)) {
                Count = 0;
              }

              ArgumentString = (((CHAR8 *)ValueBuffer + Count) - 1);
              Digits         = (Count % 3);

              if (Digits != 0) {
                Digits = (3 - Digits);
              }

              Flags |= ARGUMENT_REVERSED;

              break;
            }

            case 's':
            case 'S':
            {
              Flags |= ARGUMENT_UNICODE;

              // break skipped on purpose
            }

            case 'a':
            {
              ArgumentString = (CHAR8 *)VA_ARG (Marker, CHAR8 *);

              if (ArgumentString == NULL) {
                Flags         &= (~ARGUMENT_UNICODE);
                ArgumentString = "<null>";
              }

              break;
            }

            case 'c':
            {
              Character      = (VA_ARG (Marker, UINTN) & MAX_UINT16);
              ArgumentString = (CHAR8 *)&Character;
              Flags         |= ARGUMENT_UNICODE;

              break;
            }

            case 'G':
            case 'g':
            {
              TmpGuid = VA_ARG (Marker, EFI_GUID *);

              if (TmpGuid == NULL) {
                ArgumentString = "<null>";
              } else {
                if (TmpGuid != NULL) {
                  OcSPrint (
                    ValueBuffer,
                    MAXIMUM_VALUE_CHARACTERS,
                    0,
                    "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                    TmpGuid->Data1,
                    TmpGuid->Data2,
                    TmpGuid->Data3,
                    TmpGuid->Data4[0],
                    TmpGuid->Data4[1],
                    TmpGuid->Data4[2],
                    TmpGuid->Data4[3],
                    TmpGuid->Data4[4],
                    TmpGuid->Data4[5],
                    TmpGuid->Data4[6],
                    TmpGuid->Data4[7]
                    );
                }

                ArgumentString = ValueBuffer;
              }

              break;
            }

            case 't':
            {
              TmpTime = VA_ARG (Marker, EFI_TIME *);

              if (TmpTime == NULL) {
                ArgumentString = "<null>";
              } else {
                OcSPrint (
                  ValueBuffer,
                  MAXIMUM_VALUE_CHARACTERS,
                  0,
                  "%04d-%02d-%02d %02d:%02d:%02d",
                  TmpTime->Year,
                  TmpTime->Month,
                  TmpTime->Day,
                  TmpTime->Hour,
                  TmpTime->Minute,
                  TmpTime->Second
                  );

                ArgumentString = ValueBuffer;
              }

              break;
            }

            case 'r':
            {
              Status         = VA_ARG (Marker, EFI_STATUS);
              ArgumentString = ValueBuffer;

              // Only convert the status code to a string for debug builds.

              #if defined (MDEPKG_NDEBUG)
              #else

              if (EFI_ERROR (Status)) {
                // Clear error bit

                Index = (Status & ~MAX_BIT);

                if ((Index > 0) && (Index <= ERROR_STATUS_NUMBER)) {
                  ArgumentString = mOcStatusString[Index + WARNING_STATUS_NUMBER];
                }
              } else {
                Index = Status;

                if (Index <= WARNING_STATUS_NUMBER) {
                  ArgumentString = mOcStatusString[Index];
                }
              }

              #endif

              if (ArgumentString == ValueBuffer) {
                OcSPrint ((CHAR8 *)ValueBuffer, MAXIMUM_VALUE_CHARACTERS, 0, "%08X", Status);
              }

              break;
            }

            case '\n':
            {
              ArgumentString = (((Flags & OUTPUT_CONSOLE) == 0) ? "\n\r" : "\n");

              break;
            }

            case '%':
            default:
            {
              // if the type is '%' or unknown, then print it to the screen

              ArgumentString = (CHAR8 *)&FormatCharacter;
              Flags         |= ARGUMENT_UNICODE;

              break;
            }
          }

          break;
        }
        case '\n':
          ArgumentString = (((Flags & OUTPUT_CONSOLE) == 0) ? "\n\r" : "\n");

          break;

        default:
          ArgumentString = (CHAR8 *)&FormatCharacter;
          Flags         |= ARGUMENT_UNICODE;

          break;
      }

      // Retrieve the ArgumentString attriubutes

      if ((Flags & ARGUMENT_UNICODE) != 0) {
        ArgumentMask              = MAX_UINT16;
        BytesPerArgumentCharacter = sizeof (CHAR16);
      } else {
        ArgumentMask              = MAX_UINT8;
        BytesPerArgumentCharacter = sizeof (CHAR8);
      }

      if ((Flags & ARGUMENT_REVERSED) != 0) {
        BytesPerArgumentCharacter = -BytesPerArgumentCharacter;
      } else {
        // Compute the number of characters in ArgumentString and store it in Count
        // ArgumentString is either null-terminated, or it contains Precision characters

        for (Count = 0; (Count < Precision) || ((Flags & PRECISION) == 0); ++Count) {
          ArgumentCharacter = ((ArgumentString[Count * BytesPerArgumentCharacter]
                             | ((ArgumentString[(Count * BytesPerArgumentCharacter) + 1]) << 8)) & ArgumentMask);

          if (ArgumentCharacter == 0) {
            break;
          }
        }
      }

      if (Precision < Count) {
        Precision = Count;
      }

      // Pad before the string
      if ((Flags & (PAD_TO_WIDTH | LEFT_JUSTIFY)) == (PAD_TO_WIDTH)) {
        Buffer = OcFillBuffer (Buffer, EndBuffer, (Width - Precision), ' ', BytesPerOutputCharacter);
      }

      if (ZeroPad) {
        if (Prefix != 0) {
          Buffer = OcFillBuffer (Buffer, EndBuffer, sizeof (*Buffer), Prefix, BytesPerOutputCharacter);
        }

        Buffer = OcFillBuffer (Buffer, EndBuffer, (Precision - Count), '0', BytesPerOutputCharacter);
      } else {
        Buffer = OcFillBuffer (Buffer, EndBuffer, (Precision - Count), ' ', BytesPerOutputCharacter);

        if (Prefix != 0) {
          Buffer = OcFillBuffer (Buffer, EndBuffer, sizeof (*Buffer), Prefix, BytesPerOutputCharacter);
        }
      }

      // Output the Prefix character if it is present

      Index = 0;

      if (Prefix != 0) {
        ++Index;
      }

      // Copy the string into the output buffer performing the required type conversions

      while (Index < Count) {
        ArgumentCharacter = ((ArgumentString[0] | (ArgumentString[1] << 8)) & ArgumentMask);
        Buffer            = OcFillBuffer (Buffer, EndBuffer, sizeof (*Buffer), ArgumentCharacter, BytesPerOutputCharacter);
        ArgumentString   += BytesPerArgumentCharacter;
        ++Index;

        if (Comma) {
          ++Digits;

          if (Digits == 3) {
            Digits = 0;
            ++Index;

            if (Index < Count) {
              Buffer = OcFillBuffer (Buffer, EndBuffer, sizeof (*Buffer), ',', BytesPerOutputCharacter);
            }
          }
        }
      }

      // Pad after the string

      if (((Flags & PAD_TO_WIDTH) != 0)
       && ((Flags & LEFT_JUSTIFY) != 0)) {
        Buffer = OcFillBuffer (Buffer, EndBuffer, (Width - Precision), ' ', BytesPerOutputCharacter);
      }

      // Get the next character from the format string
      Format += BytesPerFormatCharacter;

      // Get the next character from the format string
      FormatCharacter = ((Format[0] | (Format[1] << 8)) & FormatMask);
    }

    // Null terminate the Unicode or ASCII string
    OcFillBuffer (Buffer, EndBuffer + BytesPerOutputCharacter, 1, 0, BytesPerOutputCharacter);

    // Make sure output buffer cannot contain more than PcdMaximumUnicodeStringLength
    // Unicode characters if PcdMaximumUnicodeStringLength is not zero.

    //ASSERT ((((Flags & OUTPUT_UNICODE) == 0)) || (StrSize ((CHAR16 *) OriginalBuffer) != 0));

    // Make sure output buffer cannot contain more than PcdMaximumAsciiStringLength
    // ASCII characters if PcdMaximumAsciiStringLength is not zero.

    //ASSERT ((((Flags & OUTPUT_UNICODE) != 0)) || (AsciiStrSize (OriginalBuffer) != 0));

    Result = (UINTN)((Buffer - OriginalBuffer) / BytesPerOutputCharacter);
  }

  return Result;
}

// OcSPrint
/**

  @param[out] StartOfBuffer  A pointer to the character buffer to print the results of the parsing of Format into.
  @param[in]  BufferSize     Maximum number of characters to put into buffer. Zero means no limit.
  @param[in]  Flags          Intial flags value. Can only have FORMAT_UNICODE and OUTPUT_UNICODE set.
  @param[in]  FormatString   A pointer a Null-terminated format string which describes the VA_ARGS list.
  @param[in]  ...            VA_ARGS list

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
UINTN
EFIAPI
OcSPrint (
  OUT VOID   *StartOfBuffer,
  IN  UINTN  BufferSize,
  IN  UINTN  Flags,
  IN  VOID   *FormatString,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, FormatString);

  return OcVSPrint (StartOfBuffer, BufferSize, Flags, FormatString, Marker);
}

// OcLog
/**

  @param[in] ErrorLevel  The firmware allocated handle for the EFI image.
  @param[in] Format      A pointer to the string format list which describes the VA_ARGS list
  @param[in] ...         VA_ARGS list

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
VOID
EFIAPI
OcLog (
  IN UINTN        ErrorLevel,
  IN CONST CHAR8  *Format,
  ...
  )
{
  STATIC OC_LOG_PROTOCOL *OcLog = NULL;

  VA_LIST    Marker;
  EFI_STATUS Status;

  VA_START (Marker, Format);

  // Locate OcLog Protocol Once

  if (OcLog == NULL) {
    Status = gBS->LocateProtocol (
                    &gOcLogProtocolGuid,
                    NULL,
                    (VOID **)&OcLog
                    );

    if (EFI_ERROR (Status)) {
      goto Return;
    }
  }

  if (OcLog != NULL) {
    OcLog->AddEntry (OcLog, ErrorLevel, Format, Marker);
  }

Return:
  VA_END (Marker);
}
