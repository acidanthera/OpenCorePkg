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

#include <Protocol/PciRootBridgeIo.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/PrintLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Macros.h>

#include "OcSmbiosInternal.h"
#include "DebugSmbios.h"

// SmbiosGetString
/**
  @param[in] SmbiosTable
  @param[in] StringIndex  String Index to retrieve

  @retval
**/
CHAR8 *
SmbiosGetString (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING            String
  )
{
  CHAR8      *AString = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Standard.Hdr->Length);
  UINT8      Index = 1;

  if (String == 0)
    return NULL;

  while (Index != String) {
    while (*AString != 0) {
      AString ++;
    }
    AString ++;
    if (*AString == 0) {
      return AString;
    }
    Index ++;
  }

  return AString;
}

// SmbiosSetString
/**
  @param[in] Buffer        Pointer to location containing the current address within the buffer.
  @param[in] StringIndex   String Index to retrieve
  @param[in] String        Buffer containing the null terminated ascii string

  @retval
**/
UINT8
SmbiosSetString (
  IN  CHAR8     **Buffer,
  IN  CHAR8     *String,
  IN  UINT8     *Index
  )
{
  UINTN   Length  = 0;
  VOID    *Target = (VOID *)*((UINTN *)Buffer);

  if (String == NULL) {
    return 0;
  }

  Length = AsciiStrLen (String);

  // Remove any spaces found at the end
  while ((Length != 0) && (String[Length - 1] == ' '))
  {
    Length--;
  }

  if (Length == 0)
  {
    return 0;
  }

  CopyMem (Target, String, Length);

  *Buffer += (Length + 1);

  return *Index += 1;
}

// SmbiosSetStringHex
/**
  @param[in] Buffer        Pointer to location containing the current address within the buffer.
  @param[in] StringIndex   String Index to retrieve
  @param[in] String        Buffer containing the null terminated ascii string

  @retval
**/
UINT8
SmbiosSetStringHex (
  IN  CHAR8     **Buffer,
  IN  CHAR8     *String,
  IN  UINT8     *Index
  )
{
  UINTN   Length  = 0;
  CHAR8   Digit;
  CHAR8   *Target = (VOID *)*((UINTN *)Buffer);

  if (String == NULL) {
    return 0;
  }

  Length = AsciiStrLen (String);

  if (Length == 0) {
    return 0;
  }

  // Remove any spaces found at the end
  while ((Length != 0) && (String[Length - 1] == ' '))
  {
    Length--;
  }

  if (Length == 0)
  {
    return 0;
  }

  *Target++ = '0';
  *Target++ = 'x';

  while ((Length != 0) && (*String) != 0)
  {
    Digit = *String++;
    *Target++ = "0123456789ABCDEF"[((Digit >> 4) & 0xF)];
    *Target++ = "0123456789ABCDEF"[(Digit & 0xF)];
    Length--;
  }

  *Buffer += (Target - *Buffer) + 1;

  return *Index += 1;
}

// SmbiosSetOverrideString
/**
  @param[in] Buffer        Pointer to location containing the current address within the buffer.
  @param[in] StringIndex   String Index to retrieve
  @param[in] String        Buffer containing the null terminated ascii string

  @retval
**/
UINT8
SmbiosSetOverrideString (
  IN  CHAR8   **Buffer,
  IN  CHAR8   *VariableName,
  IN  UINT8   *Index
  )
{
  EFI_STATUS  Status;
  CHAR8       *Data;
  UINTN       Length;

  if ((Buffer != NULL) && (VariableName != NULL)) {

    Data   = *Buffer;
    Length = SMBIOS_STRING_MAX_LENGTH;

    //FIXME: BROKEN

    Status = EFI_INVALID_PARAMETER; /*OcGetVariable (
              VariableName,
              &gOpenCoreOverridesGuid,
              (VOID **)&Data,
              &Length
              );*/

    if (!EFI_ERROR (Status)) {
      *Buffer += (Length + 1);
      return *Index += 1;
    }

  }
  return 0;
}

// SmbiosGetTableLength
/**

  @retval
**/
UINTN
SmbiosGetTableLength (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable
  )
{
  CHAR8  *AChar;
  UINTN  Length;

  AChar = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Standard.Hdr->Length);
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++;
  }
  Length = ((UINTN)AChar - (UINTN)SmbiosTable.Raw + 2);

  return Length;
}

// SmbiosGetTableFromType
/**

  @param[in] Smbios        Pointer to smbios entry point structure.
  @param[in] Type
  @param[in] Index

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromType (
  IN  SMBIOS_TABLE_ENTRY_POINT  *Smbios,
  IN  SMBIOS_TYPE               Type,
  IN  UINTN                     Index
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER SmbiosTable;
  UINTN                         SmbiosTypeIndex;

  SmbiosTypeIndex = 1;
  SmbiosTable.Raw = (UINT8 *)(UINTN)Smbios->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return SmbiosTable;
  }

  while ((SmbiosTypeIndex != Index) || (SmbiosTable.Standard.Hdr->Type != Type)) {
    if (SmbiosTable.Standard.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      SmbiosTable.Raw = NULL;
      break;
    }
    if (SmbiosTable.Standard.Hdr->Type == Type) {
      SmbiosTypeIndex ++;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosGetTableLength (SmbiosTable));
    if (SmbiosTable.Raw > (UINT8 *)(UINTN)(Smbios->TableAddress + Smbios->TableLength)) {
      SmbiosTable.Raw = NULL;
      break;
    }
  }

  return SmbiosTable;
}

// SmbiosGetTableFromHandle
/**

  @param[in] Smbios        Pointer to smbios entry point structure.
  @param[in] Handle

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromHandle (
  IN  SMBIOS_TABLE_ENTRY_POINT    *Smbios,
  IN  SMBIOS_HANDLE               Handle
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER    SmbiosTable;

  SmbiosTable.Raw = (UINT8 *)(UINTN)Smbios->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return SmbiosTable;
  }

  while (SmbiosTable.Standard.Hdr->Handle != Handle) {
    if (SmbiosTable.Standard.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      SmbiosTable.Raw = NULL;
      break;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosGetTableLength (SmbiosTable));
    if (SmbiosTable.Raw > (UINT8 *)(UINTN)(Smbios->TableAddress + Smbios->TableLength)) {
      SmbiosTable.Raw = NULL;
      break;
    }
  }

  return SmbiosTable;
}

// SmbiosGetTableCount
/**

  @param[in] Smbios        Pointer to smbios entry point structure.
  @param[in] Type

  @retval
**/
UINTN
SmbiosGetTableCount (
  IN  SMBIOS_TABLE_ENTRY_POINT    *Smbios,
  IN  SMBIOS_TYPE                 Type
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER    SmbiosTable;
  UINTN                       SmbiosTypeIndex;

  SmbiosTypeIndex = 1;
  SmbiosTable.Raw = (UINT8 *)(UINTN)Smbios->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return SmbiosTypeIndex;
  }

  while (SmbiosTable.Standard.Hdr->Type != SMBIOS_TYPE_END_OF_TABLE) {
    if (SmbiosTable.Standard.Hdr->Type == Type) {
      SmbiosTypeIndex ++;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosGetTableLength (SmbiosTable));
    if (SmbiosTable.Raw > (UINT8 *)(UINTN)(Smbios->TableAddress + Smbios->TableLength)) {
      break;
    }
  }
  return SmbiosTypeIndex;
}
