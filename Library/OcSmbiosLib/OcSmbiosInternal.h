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

#ifndef OC_SMBIOS_INTERNAL_H
#define OC_SMBIOS_INTERNAL_H

#include <IndustryStandard/AppleSmBios.h>

// SmbiosGetString
/**

  @retval
**/
CHAR8 *
SmbiosGetString (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING            String
  );

// SmbiosSetString
/**

  @param[in] Buffer        Pointer to the buffer where to create the smbios string.
  @param[in] String        Pointer to the null terminated ascii string to set.
  @param[in] Index         Pointer to byte index.

  @retval
**/
UINT8
SmbiosSetString (
  IN  CHAR8     **Buffer,
  IN  CHAR8     *String,
  IN  UINT8     *Index
  );

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
  );

// SmbiosSetOverrideString
/**

  @param[in] Buffer        Pointer to the buffer where to create the smbios string.
  @param[in] VariableName  Pointer to the null terminated ascii variable name.
  @param[in] Index         Pointer to byte index.

  @retval
**/
UINT8
SmbiosSetOverrideString (
  IN  CHAR8   **Buffer,
  IN  CHAR8   *VariableName,
  IN  UINT8   *Index
  );

// SmbiosGetTableLength
/**

  @retval
**/
UINTN
SmbiosGetTableLength (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable
  );

// SmbiosGetTableFromType
/**

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromType (
  IN  SMBIOS_TABLE_ENTRY_POINT  *Smbios,
  IN  SMBIOS_TYPE               Type,
  IN  UINTN                     Index
  );

// SmbiosGetTableFromHandle
/**

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromHandle (
  IN  SMBIOS_TABLE_ENTRY_POINT    *Smbios,
  IN  SMBIOS_HANDLE               Handle
  );

// SmbiosGetTableCount
/**

  @retval
**/
UINTN
SmbiosGetTableCount (
  IN  SMBIOS_TABLE_ENTRY_POINT    *Smbios,
  IN  UINT8                       Type
  );

// CreateSmBios
/**

  @retval EFI_SUCCESS     The smbios structure was created and installed successfully.
**/
EFI_STATUS
CreateSmBios (
  VOID
  );

#endif // OC_SMBIOS_INTERNAL_H
