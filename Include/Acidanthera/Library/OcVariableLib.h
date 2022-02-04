/** @file
  OpenCore Variable library.

  Copyright (c) 2021, Marvin Häuser. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_VARIABLE_LIB_H
#define OC_VARIABLE_LIB_H

/**
  Initialize the OpenCore variable library. No other function may be called
  before this function has returned.

  @param[in] ForceOcWriteFlash  Whether OC system variables should be forced to
                                be written to flash.
**/
VOID
OcVariableInit (
  IN BOOLEAN  ForceOcWriteFlash
  );

/**
  Sets the value of an OpenCore system variable to the OpenCore vendor GUID.
  If the write is to be performed non-volatile, this function guarantees to not
  request a write of the existing data is identical to the data requested to be
  written.

  @param[in]  VariableName       A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE or
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to
                                 the variable value (the timestamp associated with the variable may be updated however
                                 even if no new data value is provided,see the description of the
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param[in]  Data               The contents for the variable.
  @param[in]  VendorGuid         Variable GUID, defaults to gOcVendorVariableGuid if NULL.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set,
                                 but the AuthInfo does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
OcSetSystemVariable (
  IN CHAR16    *VariableName,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN EFI_GUID  *VendorGuid  OPTIONAL
  );

#endif // OC_VARIABLE_LIB_H
