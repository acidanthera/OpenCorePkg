/** @file
  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_USER_UNICODE_COLLATION_H
#define OC_USER_UNICODE_COLLATION_H

#include <Uefi.h>
#include <Protocol/UnicodeCollation.h>

extern EFI_UNICODE_COLLATION_PROTOCOL  gInternalUnicode2Eng;

VOID
OcUnicodeCollationInitializeMappingTables (
  VOID
  );

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
EngStriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN CHAR16                          *Str1,
  IN CHAR16                          *Str2
  );

/**
  Converts all the characters in a Null-terminated string to
  lower case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
EngStrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN OUT CHAR16                      *Str
  );

/**
  Converts all the characters in a Null-terminated string to upper
  case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
EngStrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN OUT CHAR16                      *Str
  );

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
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN CHAR16                          *String,
  IN CHAR16                          *Pattern
  );

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
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN UINTN                           FatSize,
  IN CHAR8                           *Fat,
  OUT CHAR16                         *String
  );

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
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN CHAR16                          *String,
  IN UINTN                           FatSize,
  OUT CHAR8                          *Fat
  );

/**
  Userspace English unicode collation protocol implementation.

  This function initializes unicode character mapping and then install Unicode
  Collation protocol on given interface.

  @param[in] Interface  Interface into which protocol should be installed

**/
VOID
UserUnicodeCollationInstallProtocol (
  OUT EFI_UNICODE_COLLATION_PROTOCOL  **Interface
  );

#endif // OC_USER_UNICODE_COLLATION_H
