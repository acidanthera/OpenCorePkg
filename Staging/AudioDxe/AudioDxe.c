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

#include <Protocol/LoadedImage.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcFlexArrayLib.h>

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
AudioDxeInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *DevicePathName;
  OC_FLEX_ARRAY              *ParsedLoadOptions;

  //
  // Parse optional driver params.
  //
  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevicePathName = NULL;

  Status = OcParseLoadOptions (LoadedImage, &ParsedLoadOptions);
  if (!EFI_ERROR (Status)) {
    gRestoreNoSnoop = OcHasParsedVar (ParsedLoadOptions, L"--restore-nosnoop", OcStringFormatUnicode);

    Status = OcParsedVarsGetInt (ParsedLoadOptions, L"--gpio-setup", &gGpioSetupStageMask, OcStringFormatUnicode);
    if ((Status == EFI_NOT_FOUND) && OcHasParsedVar (ParsedLoadOptions, L"--gpio-setup", OcStringFormatUnicode)) {
      gGpioSetupStageMask = GPIO_SETUP_STAGE_ALL;
    }

    if (gGpioSetupStageMask != GPIO_SETUP_STAGE_NONE) {
      OcParsedVarsGetInt (ParsedLoadOptions, L"--gpio-pins", &gGpioPinMask, OcStringFormatUnicode);
    }

    OcParsedVarsGetUnicodeStr (ParsedLoadOptions, L"--force-device", &DevicePathName);
    if (DevicePathName != NULL) {
      gForcedControllerDevicePath = ConvertTextToDevicePath (DevicePathName);
    }

    OcParsedVarsGetInt (ParsedLoadOptions, L"--codec-setup-delay", &gCodecSetupDelay, OcStringFormatUnicode);

    Status = OcParsedVarsGetInt (ParsedLoadOptions, L"--force-codec", &gForcedCodec, OcStringFormatUnicode);
    if (Status != EFI_NOT_FOUND) {
      gUseForcedCodec = TRUE;
    }

    gCodecUseConnNoneNode = OcHasParsedVar (ParsedLoadOptions, L"--use-conn-none", OcStringFormatUnicode);

    OcFlexArrayFree (&ParsedLoadOptions);
  } else if (Status != EFI_NOT_FOUND) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "HDA: GPIO stages 0x%X mask 0x%X%a; Restore NSNPEN %d; Force device %s codec %u(%u); Conn none %u; Delay %u\n",
    gGpioSetupStageMask,
    gGpioPinMask,
    gGpioPinMask == 0 ? " (auto)" : "",
    gRestoreNoSnoop,
    DevicePathName,
    gUseForcedCodec,
    gForcedCodec,
    gCodecUseConnNoneNode,
    gCodecSetupDelay
    ));

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

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiAudioDecodeProtocolGuid,
                  &gEfiAudioDecodeProtocol,
                  NULL
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
