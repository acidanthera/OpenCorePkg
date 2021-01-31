/*
 * File: HdaCodecInfo.c
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

#include "HdaCodec.h"

EFI_STATUS
EFIAPI
HdaCodecInfoGetAddress (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT8                        *Address
  )
{
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  if (This == NULL || Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS (This);

  return HdaPrivateData->HdaCodecDev->HdaIo->GetAddress (HdaPrivateData->HdaCodecDev->HdaIo, Address);
}

/**
  Gets the codec's name.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] CodecName         A pointer to the buffer to return the codec name.

  @retval EFI_SUCCESS           The codec name was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetCodecName(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT CONST CHAR16 **CodecName) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetCodecName(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (CodecName == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill codec name parameter.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  *CodecName = HdaPrivateData->HdaCodecDev->Name;
  return EFI_SUCCESS;
}

/**
  Gets the codec's vendor and device ID.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] VendorId          The vendor and device ID of the codec.

  @retval EFI_SUCCESS           The vendor and device ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetVendorId(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *VendorId) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetVendorId(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (VendorId == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill vendor ID parameter.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  *VendorId = HdaPrivateData->HdaCodecDev->VendorId;
  return EFI_SUCCESS;
}

/**
  Gets the codec's revision ID.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] RevisionId        The revision ID of the codec.

  @retval EFI_SUCCESS           The revision ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetRevisionId(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *RevisionId) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetRevisionId(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (RevisionId == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill revision ID parameter.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  *RevisionId = HdaPrivateData->HdaCodecDev->RevisionId;
  return EFI_SUCCESS;
}

/**
  Gets the node ID of the codec's audio function.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] AudioFuncId       The node ID of the codec's audio function.

  @retval EFI_SUCCESS           The node ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetAudioFuncId(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT8 *AudioFuncId,
  OUT BOOLEAN *UnsolCapable) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetAudioFuncId(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (AudioFuncId == NULL) || (UnsolCapable == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill node ID parameter.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  *AudioFuncId = HdaPrivateData->HdaCodecDev->AudioFuncGroup->NodeId;
  *UnsolCapable = HdaPrivateData->HdaCodecDev->AudioFuncGroup->UnsolCapable;
  return EFI_SUCCESS;
}

/**
  Gets the codec's default supported stream rates and formats.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] Rates             The default supported rates.
  @param[out] Formats           The default supported formats.

  @retval EFI_SUCCESS           The stream rates and formats were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetDefaultRatesFormats(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *Rates,
  OUT UINT32 *Formats) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetDefaultRatesFormats(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (Rates == NULL) || (Formats == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill rates and formats parameters.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  *Rates = HdaPrivateData->HdaCodecDev->AudioFuncGroup->SupportedPcmRates;
  *Formats = HdaPrivateData->HdaCodecDev->AudioFuncGroup->SupportedFormats;
  return EFI_SUCCESS;
}

/**
  Gets the codec's default amp capabilities.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] AmpInCaps         The default input amp capabilities.
  @param[out] AmpOutCaps        The default output amp capabilities.

  @retval EFI_SUCCESS           The default amp capabilities were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetDefaultAmpCaps(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *AmpInCaps,
  OUT UINT32 *AmpOutCaps) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetDefaultAmpCaps(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

  // If parameters are null, fail.
  if ((This == NULL) || (AmpInCaps == NULL) || (AmpOutCaps == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and fill amp caps parameters.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  *AmpInCaps = HdaPrivateData->HdaCodecDev->AudioFuncGroup->AmpInCapabilities;
  *AmpOutCaps = HdaPrivateData->HdaCodecDev->AudioFuncGroup->AmpOutCapabilities;
  return EFI_SUCCESS;
}

/**
  Gets the codec's widgets.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] Widgets           A pointer to the buffer to return the requested array of widgets.
  @param[out] WidgetCount       The number of widgets returned in Widgets.

  @retval EFI_SUCCESS           The widgets were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  A buffer couldn't be allocated.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetWidgets(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT HDA_WIDGET **Widgets,
  OUT UINTN *WidgetCount) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoGetWidgets(): start\n"));

  // Create variables.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;
  HDA_WIDGET_DEV *HdaWidgetDev;
  UINT32 AmpInCount;
  HDA_WIDGET *HdaWidgets;
  UINTN HdaWidgetsCount;

  // If parameters are null, fail.
  if ((This == NULL) || (Widgets == NULL) || (WidgetCount == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data and allocate widgets array.
  HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
  HdaWidgetsCount = HdaPrivateData->HdaCodecDev->AudioFuncGroup->WidgetsCount;
  HdaWidgets = AllocateZeroPool(sizeof(HDA_WIDGET) * HdaWidgetsCount);
  if (HdaWidgets == NULL)
    return EFI_OUT_OF_RESOURCES;

  // Populate widgets array.
  for (UINTN w = 0; w < HdaWidgetsCount; w++) {
    // Get widget.
    HdaWidgetDev = HdaPrivateData->HdaCodecDev->AudioFuncGroup->Widgets + w;

    // Get basic data.
    HdaWidgets[w].NodeId = HdaWidgetDev->NodeId;
    HdaWidgets[w].Capabilities = HdaWidgetDev->Capabilities;
    HdaWidgets[w].DefaultUnSol = HdaWidgetDev->DefaultUnSol;
    HdaWidgets[w].DefaultEapd = HdaWidgetDev->DefaultEapd;

    // Get connections.
    HdaWidgets[w].ConnectionCount = HdaWidgetDev->ConnectionCount;
    HdaWidgets[w].Connections = AllocateZeroPool(sizeof(UINT16) * HdaWidgetDev->ConnectionCount);
    if (HdaWidgets[w].Connections == NULL)
      goto FREE_WIDGETS;
    CopyMem(HdaWidgets[w].Connections, HdaWidgetDev->Connections, sizeof(UINT16) * HdaWidgetDev->ConnectionCount);

    // Get power info.
    HdaWidgets[w].SupportedPowerStates = HdaWidgetDev->SupportedPowerStates;
    HdaWidgets[w].DefaultPowerState = HdaWidgetDev->DefaultPowerState;

    // Get input amps.
    HdaWidgets[w].AmpInCapabilities = HdaWidgetDev->AmpInCapabilities;
    if (HdaWidgetDev->Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) {
      // Determine number of input amps.
      AmpInCount = HdaWidgetDev->ConnectionCount;
      if (AmpInCount < 1)
        AmpInCount = 1;

      // Allocate arrays.
      HdaWidgets[w].AmpInLeftDefaultGainMute = AllocateZeroPool(sizeof(UINT8) * AmpInCount);
      HdaWidgets[w].AmpInRightDefaultGainMute = AllocateZeroPool(sizeof(UINT8) * AmpInCount);
      if ((HdaWidgets[w].AmpInLeftDefaultGainMute == NULL) ||
        (HdaWidgets[w].AmpInRightDefaultGainMute == NULL))
        goto FREE_WIDGETS;

      // Copy arrays.
      CopyMem(HdaWidgets[w].AmpInLeftDefaultGainMute, HdaWidgetDev->AmpInLeftDefaultGainMute,
        sizeof(UINT8) * AmpInCount);
      CopyMem(HdaWidgets[w].AmpInRightDefaultGainMute, HdaWidgetDev->AmpInRightDefaultGainMute,
        sizeof(UINT8) * AmpInCount);
    }

    // Get output amp.
    HdaWidgets[w].AmpOutCapabilities = HdaWidgetDev->AmpOutCapabilities;
    HdaWidgets[w].AmpOutLeftDefaultGainMute = HdaWidgetDev->AmpOutLeftDefaultGainMute;
    HdaWidgets[w].AmpOutRightDefaultGainMute = HdaWidgetDev->AmpOutRightDefaultGainMute;

    // Get input/output data.
    HdaWidgets[w].SupportedPcmRates = HdaWidgetDev->SupportedPcmRates;
    HdaWidgets[w].SupportedFormats = HdaWidgetDev->SupportedFormats;
    HdaWidgets[w].DefaultConvFormat = HdaWidgetDev->DefaultConvFormat;
    HdaWidgets[w].DefaultConvStreamChannel = HdaWidgetDev->DefaultConvStreamChannel;
    HdaWidgets[w].DefaultConvChannelCount = HdaWidgetDev->DefaultConvChannelCount;

    // Get pin complex data.
    HdaWidgets[w].PinCapabilities = HdaWidgetDev->PinCapabilities;
    HdaWidgets[w].DefaultPinControl = HdaWidgetDev->DefaultPinControl;
    HdaWidgets[w].DefaultConfiguration = HdaWidgetDev->DefaultConfiguration;

    // Get volume knob data.
    HdaWidgets[w].VolumeCapabilities = HdaWidgetDev->VolumeCapabilities;
    HdaWidgets[w].DefaultVolume = HdaWidgetDev->DefaultVolume;
  }

  // Fill parameters.
  *WidgetCount = HdaWidgetsCount;
  *Widgets = HdaWidgets;
  return EFI_SUCCESS;

FREE_WIDGETS:
  HdaCodecInfoFreeWidgetsBuffer(HdaWidgets, HdaWidgetsCount);
  return EFI_OUT_OF_RESOURCES;
}

/**
  Frees an array of HDA_WIDGET.

  @param[in] Widgets            A pointer to the buffer array of widgets that is to be freed.
  @param[in] WidgetCount        The number of widgets in Widgets.

  @retval EFI_SUCCESS           The buffer was freed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecInfoFreeWidgetsBuffer(
  IN  HDA_WIDGET *Widgets,
  IN  UINTN WidgetCount) {
  //DEBUG((DEBUG_INFO, "HdaCodecInfoFreeWidgetsBuffer(): start\n"));

  // If parameter is null, fail.
  if (Widgets == NULL)
    return EFI_INVALID_PARAMETER;

  // Free pool buffers.
  for (UINTN w = 0; w < WidgetCount; w++) {
    if (Widgets[w].Connections != NULL)
      FreePool(Widgets[w].Connections);
    if (Widgets[w].AmpInLeftDefaultGainMute != NULL)
      FreePool(Widgets[w].AmpInLeftDefaultGainMute);
    if (Widgets[w].AmpInRightDefaultGainMute != NULL)
      FreePool(Widgets[w].AmpInRightDefaultGainMute);
  }
  FreePool(Widgets);
  return EFI_SUCCESS;
}
