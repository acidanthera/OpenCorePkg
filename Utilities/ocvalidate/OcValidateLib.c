/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "ocvalidate.h"

INT64
GetCurrentTimestamp (
  VOID
  )
{
  struct timeval Time;
  //
  // Get current time.
  //
  gettimeofday (&Time, NULL);
  //
  // Return milliseconds.
  //
  return Time.tv_sec * 1000LL + Time.tv_usec / 1000LL;
}

CHAR8 *
AsciiGetFilenameSuffix (
  IN  CONST CHAR8  *FileName
  )
{
  CONST CHAR8  *SuffixDot;

  //
  // Find the last dot by reverse order.
  //
  SuffixDot = OcAsciiStrrChr (FileName, '.');

  //
  // In some weird cases the filename can be crazily like
  // "a.suffix." (note that '.') or "..." (extremely abnormal).
  // We do not support such.
  //
  if (!SuffixDot || SuffixDot == FileName) {
    return (CHAR8 *) "";
  }

  //
  // In most normal cases, return whatever following the dot.
  //
  return (CHAR8 *) SuffixDot + 1;
}

BOOLEAN
AsciiFileNameHasSuffix (
  IN  CONST CHAR8  *FileName,
  IN  CONST CHAR8  *Suffix
  )
{
  //
  // Ensure non-empty strings.
  //
  if (FileName[0] == '\0' || Suffix[0] == '\0') {
    return FALSE;
  }

  return AsciiStriCmp (AsciiGetFilenameSuffix (FileName), Suffix) == 0;
}

BOOLEAN
AsciiFileSystemPathIsLegal (
  IN  CONST CHAR8  *Path
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (Path); ++Index) {
    //
    // Skip allowed characters (0-9, A-Z, a-z, '_', '-', '.', '/', and '\').
    //
    if (IsAsciiNumber (Path[Index])
      || IsAsciiAlpha (Path[Index])
      || Path[Index] == '_'
      || Path[Index] == '-'
      || Path[Index] == '.'
      || Path[Index] == '/'
      || Path[Index] == '\\') {
      continue;
    }

    //
    // Disallowed characters matched.
    //
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
AsciiCommentIsLegal (
  IN  CONST CHAR8  *Comment
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (Comment); ++Index) {
    //
    // Unprintable characters matched.
    //
    if (IsAsciiPrint (Comment[Index]) == 0) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
AsciiIdentifierIsLegal (
  IN  CONST CHAR8    *Identifier,
  IN        BOOLEAN  IsKernelPatchIdentifier
  )
{
  UINTN  Index;

  //
  // Special check for Kernel->Patch->Identifier.
  //
  if (IsKernelPatchIdentifier) {
    //
    // There must be one dot character unless it is a kernel patch.
    //
    if (AsciiStrCmp (Identifier, "kernel") == 0) {
      return TRUE;
    } else {
      if (OcAsciiStrChr (Identifier, '.') == NULL) {
        return FALSE;
      }

      return TRUE;
    }
  }

  for (Index = 0; Index < AsciiStrLen (Identifier); ++Index) {
    //
    // Skip allowed characters (0-9, A-Z, a-z, '_', '-', and '.').
    // FIXME: Discuss what exactly is legal for identifiers.
    //
    if (IsAsciiNumber (Identifier[Index])
      || IsAsciiAlpha (Identifier[Index])
      || Identifier[Index] == '_'
      || Identifier[Index] == '-'
      || Identifier[Index] == '.') {
      continue;
    }

    //
    // Disallowed characters matched.
    //
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
AsciiArchIsLegal (
  IN  CONST CHAR8  *Arch
  )
{
  //
  // Only allow Any, i386 (32-bit), and x86_64 (64-bit).
  // FIXME: Do not allow empty string in OC.
  //
  if (AsciiStrCmp (Arch, "Any") != 0
    && AsciiStrCmp (Arch, "i386") != 0
    && AsciiStrCmp (Arch, "x86_64") != 0) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
AsciiDevicePathIsLegal (
  IN  CONST CHAR8  *DevicePath
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (DevicePath); ++Index) {
    //
    // Skip allowed characters (0-9, A-Z, a-z, '/', '(', ')', and ',').
    // FIXME: Discuss whether more/less should be allowed for a legal device path.
    //
    if (IsAsciiNumber (DevicePath[Index])
      || IsAsciiAlpha (DevicePath[Index])
      || DevicePath[Index] == '/'
      || DevicePath[Index] == '('
      || DevicePath[Index] == ')'
      || DevicePath[Index] == ',') {
      continue;
    }

    //
    // Disallowed characters matched.
    //
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
AsciiDevicePropertyIsLegal (
  IN  CONST CHAR8  *DeviceProperty
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (DeviceProperty); ++Index) {
    //
    // Skip allowed characters (0-9, A-Z, a-z, '-').
    // FIXME: Discuss whether more/less should be allowed for a legal device property.
    //
    if (IsAsciiNumber (DeviceProperty[Index])
      || IsAsciiAlpha (DeviceProperty[Index])
      || DeviceProperty[Index] == '-') {
      continue;
    }

    //
    // Disallowed characters matched.
    //
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
AsciiUefiDriverIsLegal (
  IN  CONST CHAR8  *Driver
  )
{
  UINTN  Index;

  //
  // If an EFI driver does not contain .efi suffix,
  // then it must be illegal.
  //
  if (!AsciiFileNameHasSuffix (Driver, "efi")) {
    return FALSE;
  }

  for (Index = 0; Index < AsciiStrLen (Driver); ++Index) {
    //
    // NOTE: Skip '#' as it is treated as comments and thus is legal.
    //
    if (Driver[0] == '#') {
      continue;
    }

    //
    // Skip allowed characters (0-9, A-Z, a-z, '_', '-', '.', '/').
    // FIXME: Discuss whether more/less should be allowed for a UEFI driver.
    //
    if (IsAsciiNumber (Driver[Index])
      || IsAsciiAlpha (Driver[Index])
      || Driver[Index] == '_'
      || Driver[Index] == '-'
      || Driver[Index] == '.'
      || Driver[Index] == '/') {
      continue;
    }

    //
    // Disallowed characters matched.
    //
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
DataHasProperMasking (
  IN  CONST VOID   *Data,
  IN  CONST VOID   *Mask,
  IN        UINTN  Size
  )
{
  CONST UINT8  *ByteData;
  CONST UINT8  *ByteMask;
  UINTN        Index;

  ByteData = Data;
  ByteMask = Mask;

  for (Index = 0; Index < Size; ++Index) {
    if ((ByteData[Index] & ~ByteMask[Index]) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

UINT32
ReportError (
  IN  CONST CHAR8  *FuncName,
  IN  UINT32       ErrorCount
  )
{
  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", FuncName, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  } else {
    DEBUG ((DEBUG_VERBOSE, "%a returns no errors!\n", FuncName));
  }

  return ErrorCount;
}
