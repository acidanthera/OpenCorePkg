/*
 * File: AudioDxe.c
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

#include "AudioDxe.h"
#include "HdaController/HdaController.h"
#include "HdaController/HdaControllerComponentName.h"
#include "HdaCodec/HdaCodec.h"
#include "HdaCodec/HdaCodecComponentName.h"

/**
  HdaController Driver Binding.
**/
EFI_DRIVER_BINDING_PROTOCOL
gHdaControllerDriverBinding = {
  HdaControllerDriverBindingSupported,
  HdaControllerDriverBindingStart,
  HdaControllerDriverBindingStop,
  AUDIODXE_VERSION,
  NULL,
  NULL
};

/**
  HdaCodec Driver Binding.
**/
EFI_DRIVER_BINDING_PROTOCOL
gHdaCodecDriverBinding = {
  HdaCodecDriverBindingSupported,
  HdaCodecDriverBindingStart,
  HdaCodecDriverBindingStop,
  AUDIODXE_VERSION,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
AudioDxeInit(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Register HdaController Driver Binding.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
    ImageHandle,
    SystemTable,
    &gHdaControllerDriverBinding,
    ImageHandle,
    &gHdaControllerComponentName,
    &gHdaControllerComponentName2
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register HdaCodec Driver Binding.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
    ImageHandle,
    SystemTable,
    &gHdaCodecDriverBinding,
    NULL,
    &gHdaCodecComponentName,
    &gHdaCodecComponentName2
    );

  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
