/*
 * File: HdaControllerComponentName.h
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

#ifndef _EFI_HDA_CONTROLLER_COMPONENT_NAME_H_
#define _EFI_HDA_CONTROLLER_COMPONENT_NAME_H_

#include "AudioDxe.h"
#include <Protocol/ComponentName.h>
#include "HdaController.h"

EFI_STATUS
EFIAPI
HdaControllerComponentNameGetDriverName(
  IN EFI_COMPONENT_NAME_PROTOCOL *This,
  IN CHAR8 *Language,
  OUT CHAR16 **DriverName);

EFI_STATUS
EFIAPI
HdaControllerComponentNameGetControllerName(
  IN EFI_COMPONENT_NAME_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_HANDLE ChildHandle OPTIONAL,
  IN CHAR8 *Language,
  OUT CHAR16 **ControllerName);

extern EFI_COMPONENT_NAME_PROTOCOL gHdaControllerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gHdaControllerComponentName2;

#endif
