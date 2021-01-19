/*
 * File: HdaControllerInfo.c
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

#include "HdaController.h"

EFI_STATUS
EFIAPI
HdaControllerInfoGetName (
  IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL  *This,
  OUT CONST CHAR16                      **Name
  )
{
  HDA_CONTROLLER_INFO_PRIVATE_DATA    *HdaPrivateData;

  if (This == NULL || Name == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HdaPrivateData = HDA_CONTROLLER_INFO_PRIVATE_DATA_FROM_THIS (This);
  *Name          = HdaPrivateData->HdaControllerDev->Name;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerInfoGetVendorId (
  IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL  *This,
  OUT UINT32                            *VendorId
  )
{
  HDA_CONTROLLER_INFO_PRIVATE_DATA    *HdaPrivateData;

  if (This == NULL || VendorId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HdaPrivateData = HDA_CONTROLLER_INFO_PRIVATE_DATA_FROM_THIS (This);
  *VendorId      = HdaPrivateData->HdaControllerDev->VendorId;

  return EFI_SUCCESS;
}
