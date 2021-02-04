/*
 * File: HdaControllerInfo.h
 *
 * Copyright (c) 2018 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EFI_HDA_CONTROLLER_INFO_H
#define EFI_HDA_CONTROLLER_INFO_H

#include <Uefi.h>

/**
  HDA Controller Info protocol GUID.
**/
#define EFI_HDA_CONTROLLER_INFO_PROTOCOL_GUID \
  { 0xE5FC2CAF, 0x0291, 0x46F2,               \
    { 0x87, 0xF8, 0x10, 0xC7, 0x58, 0x72, 0x58, 0x04 } }

typedef struct EFI_HDA_CONTROLLER_INFO_PROTOCOL_ EFI_HDA_CONTROLLER_INFO_PROTOCOL;

/**
  Gets the controller's name.

  @param[in]  This              A pointer to the EFI_HDA_CONTROLLER_INFO_PROTOCOL instance.
  @param[out] Name              A pointer to the buffer to return the controller name.

  @retval EFI_SUCCESS           The controller name was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CONTROLLER_INFO_GET_NAME) (
  IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL  *This,
  OUT CONST CHAR16                      **Name
  );

/**
  Gets the controller's vendor and device ID.

  @param[in]  This              A pointer to the EFI_HDA_CONTROLLER_INFO_PROTOCOL instance.
  @param[out] VendorId          The vendor and device ID of the controller.

  @retval EFI_SUCCESS           The vendor and device ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CONTROLLER_INFO_GET_VENDOR_ID) (
  IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL  *This,
  OUT UINT32                            *VendorId
  );

/**
  Protocol struct.
**/
struct EFI_HDA_CONTROLLER_INFO_PROTOCOL_ {
  EFI_HDA_CONTROLLER_INFO_GET_NAME        GetName;
  EFI_HDA_CONTROLLER_INFO_GET_VENDOR_ID   GetVendorId;
};

extern EFI_GUID gEfiHdaControllerInfoProtocolGuid;

#endif // EFI_HDA_CONTROLLER_INFO_H
