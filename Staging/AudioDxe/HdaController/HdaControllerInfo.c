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

/**
  Gets the controller's name.

  @param[in]  This              A pointer to the EFI_HDA_CONTROLLER_INFO_PROTOCOL instance.
  @param[out] CodecName         A pointer to the buffer to return the codec name.

  @retval EFI_SUCCESS           The controller name was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaControllerInfoGetName(
  IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL *This,
  OUT CONST CHAR16 **ControllerName) {
  //DEBUG((DEBUG_INFO, "HdaControllerInfoGetName(): start\n"));

  // Create variables.
  HDA_CONTROLLER_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (ControllerName == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill node ID parameter.
  HdaPrivateData = HDA_CONTROLLER_INFO_PRIVATE_DATA_FROM_THIS(This);
  *ControllerName = HdaPrivateData->HdaControllerDev->Name;
  return EFI_SUCCESS;
}
