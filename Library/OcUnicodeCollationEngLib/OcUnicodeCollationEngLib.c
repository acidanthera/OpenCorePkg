/** @file
  Driver to implement English version of Unicode Collation Protocol.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <Library/BaseLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiLib.h>

#include "OcUnicodeCollationEngLibInternal.h"

CHAR8 mEngUpperMap[MAP_TABLE_SIZE];
CHAR8 mEngLowerMap[MAP_TABLE_SIZE];
CHAR8 mEngInfoMap[MAP_TABLE_SIZE];

CHAR8 mOtherChars[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  '\\',
  '.',
  '_',
  '^',
  '$',
  '~',
  '!',
  '#',
  '%',
  '&',
  '-',
  '{',
  '}',
  '(',
  ')',
  '@',
  '`',
  '\'',
  '\0'
};

//
// Supported language list, meant to contain one extra language if necessary.
//
STATIC CHAR8  UnicodeLanguages[6] = "en";

//
// EFI Unicode Collation2 Protocol supporting RFC 4646 language code
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_COLLATION_PROTOCOL  Unicode2Eng = {
  EngStriColl,
  EngMetaiMatch,
  EngStrLwr,
  EngStrUpr,
  EngFatToStr,
  EngStrToFat,
  UnicodeLanguages
};

/**
  The user Entry Point for English module.

  This function initializes unicode character mapping and then installs Unicode
  Collation & Unicode Collation 2 Protocols based on the feature flags.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_UNICODE_COLLATION_PROTOCOL *
OcUnicodeCollationEngInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         OverwroteLang = FALSE;
  UINTN                           Index         = 0;
  UINTN                           Index2        = 0;
  EFI_UNICODE_COLLATION_PROTOCOL  *Existing     = NULL;
  CHAR8                           *PlatformLang = NULL;
  UINTN                           Size          = 0;
  EFI_HANDLE                      NewHandle     = NULL;

  //
  // On several platforms EFI_PLATFORM_LANG_VARIABLE_NAME is not available.
  // Fallback to "en-US" which is supported by this driver and the wide majority of others.
  //
  Status = GetVariable2 (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **) &PlatformLang, &Size);
  //
  // No value or something broken, discard and fallback.
  //
  if (EFI_ERROR (Status) || AsciiStrLen (PlatformLang) < 2) {
    gRT->SetVariable (
           EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid,
           EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
           6, "en-US");
    OverwroteLang = TRUE;
  }

  if (Reinstall) {
    Status = UninstallAllProtocolInstances (&gEfiUnicodeCollation2ProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCUC: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gEfiUnicodeCollation2ProtocolGuid,
                    NULL,
                    (VOID **) &Existing);

    if (!EFI_ERROR (Status)) {
      //
      // We do not need to install an existing collation.
      //
      return Existing;
    }
  }

  //
  // On some platforms with missing gEfiUnicodeCollation2ProtocolGuid EFI_PLATFORM_LANG_VARIABLE_NAME is set
  // to the value different from "en" or "en-...". This is not going to work with our driver UEFI Shell load failures:
  // we did not overwrite EFI_PLATFORM_LANG_VARIABLE_NAME, but it uses some other language.
  // Workaround by appending the other language to the list of supported ones.
  //
  if (!OverwroteLang && AsciiStrnCmp (PlatformLang, "en", 2)) {
    OC_INLINE_STATIC_ASSERT (sizeof (UnicodeLanguages) == 6 && sizeof (UnicodeLanguages[0]) == 1,
      "UnicodeLanguages must contain one extra language");
    UnicodeLanguages[2] = ';';
    UnicodeLanguages[3] = PlatformLang[0];
    UnicodeLanguages[4] = PlatformLang[1];
    UnicodeLanguages[5] = '\0';
  }

  //
  // Free allocated memory for platform languages
  //
  if (PlatformLang) {
    gBS->FreePool (PlatformLang);
  }

  //
  // Initialize mapping tables for the supported languages
  //
  for (Index = 0; Index < MAP_TABLE_SIZE; Index++) {
    mEngUpperMap[Index] = (CHAR8) Index;
    mEngLowerMap[Index] = (CHAR8) Index;
    mEngInfoMap[Index]  = 0;

    if ((Index >= 'a' && Index <= 'z') || (Index >= 0xe0 && Index <= 0xf6) || (Index >= 0xf8 && Index <= 0xfe)) {

      Index2                = Index - 0x20;
      mEngUpperMap[Index]   = (CHAR8) Index2;
      mEngLowerMap[Index2]  = (CHAR8) Index;

      mEngInfoMap[Index] |= CHAR_FAT_VALID;
      mEngInfoMap[Index2] |= CHAR_FAT_VALID;
    }
  }

  for (Index = 0; mOtherChars[Index] != 0; Index++) {
    Index2 = mOtherChars[Index];
    mEngInfoMap[Index2] |= CHAR_FAT_VALID;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiUnicodeCollation2ProtocolGuid,
                  &Unicode2Eng,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &Unicode2Eng;
}


/**
  Performs a case-insensitive comparison of two Null-terminated strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated string.
  @param  Str2 A pointer to a Null-terminated string.

  @retval 0   Str1 is equivalent to Str2
  @retval > 0 Str1 is lexically greater than Str2
  @retval < 0 Str1 is lexically less than Str2

**/
INTN
EFIAPI
EngStriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *Str1,
  IN CHAR16                           *Str2
  )
{
  while (*Str1 != 0) {
    if (TO_UPPER (*Str1) != TO_UPPER (*Str2)) {
      break;
    }

    Str1 += 1;
    Str2 += 1;
  }

  return TO_UPPER (*Str1) - TO_UPPER (*Str2);
}


/**
  Converts all the characters in a Null-terminated string to
  lower case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
EngStrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
{
  while (*Str != 0) {
    *Str = TO_LOWER (*Str);
    Str += 1;
  }
}


/**
  Converts all the characters in a Null-terminated string to upper
  case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
EngStrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
{
  while (*Str != 0) {
    *Str = TO_UPPER (*Str);
    Str += 1;
  }
}

/**
  Performs a case-insensitive comparison of a Null-terminated
  pattern string and a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string.
  @param  Pattern A pointer to a Null-terminated pattern string.

  @retval TRUE    Pattern was found in String.
  @retval FALSE   Pattern was not found in String.

**/
BOOLEAN
EFIAPI
EngMetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  )
{
  CHAR16  CharC;
  CHAR16  CharP;
  CHAR16  Index3;

  for (;;) {
    CharP = *Pattern;
    Pattern += 1;

    switch (CharP) {
    case 0:
      //
      // End of pattern.  If end of string, TRUE match
      //
      if (*String != 0) {
        return FALSE;
      } else {
        return TRUE;
      }

    case '*':
      //
      // Match zero or more chars
      //
      while (*String != 0) {
        if (EngMetaiMatch (This, String, Pattern)) {
          return TRUE;
        }

        String += 1;
      }

      return EngMetaiMatch (This, String, Pattern);

    case '?':
      //
      // Match any one char
      //
      if (*String == 0) {
        return FALSE;
      }

      String += 1;
      break;

    case '[':
      //
      // Match char set
      //
      CharC = *String;
      if (CharC == 0) {
        //
        // syntax problem
        //
        return FALSE;
      }

      Index3  = 0;
      CharP   = *Pattern++;
      while (CharP != 0) {
        if (CharP == ']') {
          return FALSE;
        }

        if (CharP == '-') {
          //
          // if range of chars, get high range
          //
          CharP = *Pattern;
          if (CharP == 0 || CharP == ']') {
            //
            // syntax problem
            //
            return FALSE;
          }

          if (TO_UPPER (CharC) >= TO_UPPER (Index3) && TO_UPPER (CharC) <= TO_UPPER (CharP)) {
            //
            // if in range, it's a match
            //
            break;
          }
        }

        Index3 = CharP;
        if (TO_UPPER (CharC) == TO_UPPER (CharP)) {
          //
          // if char matches
          //
          break;
        }

        CharP = *Pattern++;
      }
      //
      // skip to end of match char set
      //
      while ((CharP != 0) && (CharP != ']')) {
        CharP = *Pattern;
        Pattern += 1;
      }

      String += 1;
      break;

    default:
      CharC = *String;
      if (TO_UPPER (CharC) != TO_UPPER (CharP)) {
        return FALSE;
      }

      String += 1;
      break;
    }
  }
}


/**
  Converts an 8.3 FAT file name in an OEM character set to a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an 8-bit OEM character set.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.

**/
VOID
EFIAPI
EngFatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN UINTN                            FatSize,
  IN CHAR8                            *Fat,
  OUT CHAR16                          *String
  )
{
  //
  // No DBCS issues, just expand and add null terminate to end of string
  //
  while ((*Fat != 0) && (FatSize != 0)) {
    *String = *Fat;
    String += 1;
    Fat += 1;
    FatSize -= 1;
  }

  *String = 0;
}


/**
  Converts a Null-terminated string to legal characters in a FAT
  filename using an OEM character set.

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.

  @retval TRUE    Fat is a Long File Name
  @retval FALSE   Fat is an 8.3 file name

**/
BOOLEAN
EFIAPI
EngStrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN UINTN                            FatSize,
  OUT CHAR8                           *Fat
  )
{
  BOOLEAN SpecialCharExist;

  SpecialCharExist = FALSE;
  while ((*String != 0) && (FatSize != 0)) {
    //
    // Skip '.' or ' ' when making a fat name
    //
    if (*String != '.' && *String != ' ') {
      //
      // If this is a valid fat char, move it.
      // Otherwise, move a '_' and flag the fact that the name needs a long file name.
      //
      if (*String < MAP_TABLE_SIZE && ((mEngInfoMap[*String] & CHAR_FAT_VALID) != 0)) {
        *Fat = mEngUpperMap[*String];
      } else {
        *Fat              = '_';
        SpecialCharExist  = TRUE;
      }

      Fat += 1;
      FatSize -= 1;
    }

    String += 1;
  }
  //
  // Do not terminate that fat string
  //
  return SpecialCharExist;
}
