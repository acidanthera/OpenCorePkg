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

#ifndef OC_VARIABLE_LIB_H_
#define OC_VARIABLE_LIB_H_

// CheckVariableBoolean
/** Check the boolean-value of an EFI Variable.

  @param[in] Name        The firmware allocated handle for the EFI image.
  @param[in] VendorGuid  A unique identifier for the vendor.

  @retval  Boolean value indicating TRUE, if variable exists.
**/
BOOLEAN
CheckVariableBoolean (
  IN CHAR16    *Name,
  IN EFI_GUID  *VendorGuid
  );

// DeleteVariables
/** Deletes all EFI Variables matching the pattern.

  @param[in] VendorGuid  A unique identifier for the vendor.
  @param[in] SearchName  A Null-terminated Unicode string that is the name of the vendor’s variable. Each SearchName is
                         unique for each VendorGuid.
                         SearchName must contain 1 or more Unicode characters. If SearchName is an empty Unicode
                         string, then EFI_INVALID_PARAMETER is returned.

  @retval EFI_SUCCESS  The variable was deleted successfully.
**/
EFI_STATUS
DeleteVariables (
  IN EFI_GUID  *VendorGuid,
  IN CHAR16    *SearchName
  );

// OcGetVariable
/** Return the variables value in buffer.

  @param[in] Name           A Null-terminated Ascii string that is the name of the vendor’s variable. Each Name is
                            unique for each VendorGuid.
                            Name must contain 1 or more Ascii characters. If Name is an empty Ascii string, then
                            EFI_INVALID_PARAMETER is returned.
  @param[in] VendorGuid     A unique identifier for the vendor.
  @param[in,out] BufferPtr  Pointer to store the buffer.
  @param[in,out] BufferSize Size of the variables data buffer.

  @retval  A pointer to the variables data
**/
EFI_STATUS
OcGetVariable (
  IN  CHAR8       *Name,
  IN  EFI_GUID    *VendorGuid,
  IN OUT VOID     **BufferPtr,
  IN OUT UINTN    *BufferSize
  );

// SetVariable
/** Sets an EFI Variable.

  @param[in] Name            A Null-terminated Ascii string that is the name of the vendor’s variable. Each Name is
                             unique for each VendorGuid.
                             Name must contain 1 or more Ascii characters. If Name is an empty Ascii string, then
                             EFI_INVALID_PARAMETER is returned.
  @param[in] VendorGuid      A unique identifier for the vendor.
  @param[in] Attributes      Attributes bitmask to set for the variable. Refer to the GetVariable() function
                             description.
  @param[in] DataSize        The size in bytes of the Data buffer. A size of zero causes the variable to be deleted.
  @param[in] Data            The contents for the variable.
  @param[in] OverideDefault  A boolean flag which enables updating a previously set value.

  @retval EFI_SUCCESS  The variable was set successfully.
**/
EFI_STATUS
SetVariable (
  IN CHAR8     *Name,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN BOOLEAN   OverideDefault
  );

#endif // OC_VARIABLE_LIB_H_
