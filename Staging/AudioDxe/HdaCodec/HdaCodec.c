/*
 * File: HdaCodec.c
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
#include "HdaCodecComponentName.h"

#include <Library/OcHdaDevicesLib.h>
#include <Library/OcStringLib.h>

EFI_STATUS
EFIAPI
HdaCodecProbeWidget(
  IN HDA_WIDGET_DEV *HdaWidget) {
  //DEBUG((DEBUG_INFO, "HdaCodecProbeWidget(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo = HdaWidget->FuncGroup->HdaCodecDev->HdaIo;
  UINT32 Response;
  UINT8 ConnectionListThresh;
  UINT8 AmpInCount;

  Response = 0;

  // Get widget capabilities.
  Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_WIDGET_CAPS), &HdaWidget->Capabilities);
  if (EFI_ERROR(Status))
    return Status;
  HdaWidget->Type = HDA_PARAMETER_WIDGET_CAPS_TYPE(HdaWidget->Capabilities);
  HdaWidget->AmpOverride = HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_AMP_OVERRIDE;
  //DEBUG((DEBUG_INFO, "Widget @ 0x%X type: 0x%X\n", HdaWidget->NodeId, HdaWidget->Type));
  //DEBUG((DEBUG_INFO, "Widget @ 0x%X capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->Capabilities));

  // Get default unsolicitation.
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_UNSOL_CAPABLE) {
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_UNSOL_RESPONSE, 0), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->DefaultUnSol = (UINT8)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X unsolicitation: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultUnSol));
  }

  // Get connections.
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_CONN_LIST) {
    // Get connection list length.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_CONN_LIST_LENGTH), &HdaWidget->ConnectionListLength);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->ConnectionCount = HDA_PARAMETER_CONN_LIST_LENGTH_LEN(HdaWidget->ConnectionListLength);
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X connection list length: 0x%X\n", HdaWidget->NodeId, HdaWidget->ConnectionListLength));

    // Get connections.
    HdaWidget->Connections = AllocateZeroPool(sizeof(UINT16) * HdaWidget->ConnectionCount);
    if (HdaWidget->Connections == NULL)
      return EFI_OUT_OF_RESOURCES;
    ConnectionListThresh = (HdaWidget->ConnectionListLength & HDA_PARAMETER_CONN_LIST_LENGTH_LONG) ? 2 : 4;
    for (UINT8 c = 0; c < HdaWidget->ConnectionCount; c++) {
      // Do we need to get entries?
      if (!(c % ConnectionListThresh)) {
        // Get connection entries.
        Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
          HDA_CODEC_VERB(HDA_VERB_GET_CONN_LIST_ENTRY, c), &Response);
        if (EFI_ERROR(Status))
          return Status;
      }

      // Populate entry list.
      if (HdaWidget->ConnectionListLength & HDA_PARAMETER_CONN_LIST_LENGTH_LONG)
        HdaWidget->Connections[c] = HDA_VERB_GET_CONN_LIST_ENTRY_LONG(Response, c % 2);
      else
        HdaWidget->Connections[c] = HDA_VERB_GET_CONN_LIST_ENTRY_SHORT(Response, c % 4);
    }
  }

  // Print connections.
  //DEBUG((DEBUG_INFO, "Widget @ 0x%X connections (%u):", HdaWidget->NodeId, HdaWidget->ConnectionCount));
  //for (UINT8 c = 0; c < HdaWidget->ConnectionCount; c++)
    //DEBUG((DEBUG_INFO, " 0x%X", HdaWidget->Connections[c]));
  //DEBUG((DEBUG_INFO, "\n"));

  // Does the widget support power management?
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_POWER_CNTRL) {
    // Get supported power states.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_POWER_STATES), &HdaWidget->SupportedPowerStates);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X supported power states: 0x%X\n", HdaWidget->NodeId, HdaWidget->SupportedPowerStates));

    // Get default power state.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_POWER_STATE, 0), &HdaWidget->DefaultPowerState);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X power state: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultPowerState));
  }

  // Do we have input amps?
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) {
    // Get input amp capabilities.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), &HdaWidget->AmpInCapabilities);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X input amp capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->AmpInCapabilities));

    // Determine number of input amps and allocate arrays.
    AmpInCount = HdaWidget->ConnectionCount;
    if (AmpInCount < 1)
      AmpInCount = 1;
    HdaWidget->AmpInLeftDefaultGainMute = AllocateZeroPool(sizeof(UINT8) * AmpInCount);
    HdaWidget->AmpInRightDefaultGainMute = AllocateZeroPool(sizeof(UINT8) * AmpInCount);
    if ((HdaWidget->AmpInLeftDefaultGainMute == NULL) || (HdaWidget->AmpInRightDefaultGainMute == NULL))
      return EFI_OUT_OF_RESOURCES;

    // Get default gain/mute for input amps.
    for (UINT8 i = 0; i < AmpInCount; i++) {
      // Get left.
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_AMP_GAIN_MUTE,
      HDA_VERB_GET_AMP_GAIN_MUTE_PAYLOAD(i, TRUE, FALSE)), &Response);
      if (EFI_ERROR(Status))
        return Status;
      HdaWidget->AmpInLeftDefaultGainMute[i] = (UINT8)Response;

      // Get right.
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_AMP_GAIN_MUTE,
      HDA_VERB_GET_AMP_GAIN_MUTE_PAYLOAD(i, FALSE, FALSE)), &Response);
      if (EFI_ERROR(Status))
        return Status;
      HdaWidget->AmpInRightDefaultGainMute[i] = (UINT8)Response;
      //DEBUG((DEBUG_INFO, "Widget @ 0x%X input amp %u defaults: 0x%X 0x%X\n", HdaWidget->NodeId, i,
      //    HdaWidget->AmpInLeftDefaultGainMute[i], HdaWidget->AmpInRightDefaultGainMute[i]));
    }
  }

  // Do we have an output amp?
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) {
    // Get output amp capabilities.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), &HdaWidget->AmpOutCapabilities);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X output amp capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->AmpOutCapabilities));

    // Get left.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_AMP_GAIN_MUTE,
    HDA_VERB_GET_AMP_GAIN_MUTE_PAYLOAD(0, TRUE, TRUE)), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->AmpOutLeftDefaultGainMute = (UINT8)Response;

    // Get right.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_AMP_GAIN_MUTE,
    HDA_VERB_GET_AMP_GAIN_MUTE_PAYLOAD(0, FALSE, TRUE)), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->AmpOutRightDefaultGainMute = (UINT8)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X output amp defaults: 0x%X 0x%X\n", HdaWidget->NodeId,
    //    HdaWidget->AmpOutLeftDefaultGainMute, HdaWidget->AmpOutRightDefaultGainMute));
  }

  // Is the widget an Input or Output?
  if (HdaWidget->Type == HDA_WIDGET_TYPE_INPUT || HdaWidget->Type == HDA_WIDGET_TYPE_OUTPUT) {
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_FORMAT_OVERRIDE) {
      // Get supported PCM sizes/rates.
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
        HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES), &HdaWidget->SupportedPcmRates);
      if (EFI_ERROR(Status))
        return Status;
      //DEBUG((DEBUG_INFO, "Widget @ 0x%X supported PCM sizes/rates: 0x%X\n", HdaWidget->NodeId, HdaWidget->SupportedPcmRates));

      // Get supported stream formats.
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
        HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_STREAM_FORMATS), &HdaWidget->SupportedFormats);
      if (EFI_ERROR(Status))
        return Status;
      //DEBUG((DEBUG_INFO, "Widget @ 0x%X supported formats: 0x%X\n", HdaWidget->NodeId, HdaWidget->SupportedFormats));
    }

    // Get default converter format.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_CONVERTER_FORMAT, 0), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->DefaultConvFormat = (UINT16)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X default format: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultConvFormat));

    // Get default converter stream/channel.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_CONVERTER_STREAM_CHANNEL, 0), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->DefaultConvStreamChannel = (UINT8)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X default stream/channel: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultConvStreamChannel));

    // Get default converter channel count.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_CONVERTER_CHANNEL_COUNT, 0), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->DefaultConvChannelCount = (UINT8)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X default channel count: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultConvChannelCount));
  } else if (HdaWidget->Type == HDA_WIDGET_TYPE_PIN_COMPLEX) { // Is the widget a Pin Complex?
    // Get pin capabilities.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_PIN_CAPS), &HdaWidget->PinCapabilities);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X pin capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->PinCapabilities));

    // Get default EAPD.
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) {
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
        HDA_CODEC_VERB(HDA_VERB_GET_EAPD_BTL_ENABLE, 0), &Response);
      if (EFI_ERROR(Status))
        return Status;
      HdaWidget->DefaultEapd = (UINT8)Response;
      HdaWidget->DefaultEapd &= 0x7;
      HdaWidget->DefaultEapd |= HDA_EAPD_BTL_ENABLE_EAPD;
      //DEBUG((DEBUG_INFO, "Widget @ 0x%X EAPD: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultEapd));
    }

    // Get default pin control.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->DefaultPinControl = (UINT8)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X default pin control: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultPinControl));

    // Get default pin configuration.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_CONFIGURATION_DEFAULT, 0), &HdaWidget->DefaultConfiguration);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X default pin configuration: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultConfiguration));
  } else if (HdaWidget->Type == HDA_WIDGET_TYPE_VOLUME_KNOB) { // Is the widget a Volume Knob?
    // Get volume knob capabilities.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VOLUME_KNOB_CAPS), &HdaWidget->VolumeCapabilities);
    if (EFI_ERROR(Status))
      return Status;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X volume knob capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->VolumeCapabilities));

    // Get default volume.
    Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId,
      HDA_CODEC_VERB(HDA_VERB_GET_VOLUME_KNOB, 0), &Response);
    if (EFI_ERROR(Status))
      return Status;
    HdaWidget->DefaultVolume = (UINT8)Response;
    //DEBUG((DEBUG_INFO, "Widget @ 0x%X default volume: 0x%X\n", HdaWidget->NodeId, HdaWidget->DefaultVolume));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecProbeFuncGroup(
  IN HDA_FUNC_GROUP *FuncGroup) {
  //DEBUG((DEBUG_INFO, "HdaCodecProbeFuncGroup(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo = FuncGroup->HdaCodecDev->HdaIo;
  UINT32 Response;

  UINT8 WidgetStart;
  UINT8 WidgetEnd;
  UINT8 WidgetCount;
  HDA_WIDGET_DEV *HdaWidget;
  HDA_WIDGET_DEV *HdaConnectedWidget;

  // Get function group type.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_TYPE), &Response);
  if (EFI_ERROR(Status))
    return Status;
  FuncGroup->Type = HDA_PARAMETER_FUNC_GROUP_TYPE_NODETYPE(Response);
  FuncGroup->UnsolCapable = (Response & HDA_PARAMETER_FUNC_GROUP_TYPE_UNSOL) != 0;

  // Determine if function group is an audio one. If not, we cannot support it.
  DEBUG((DEBUG_INFO, "Function group @ 0x%X is of type 0x%X\n", FuncGroup->NodeId, FuncGroup->Type));
  if (FuncGroup->Type != HDA_FUNC_GROUP_TYPE_AUDIO)
    return EFI_UNSUPPORTED;

  // Get function group capabilities.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_CAPS), &FuncGroup->Capabilities);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->Capabilities));

  // Get default supported PCM sizes/rates.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES), &FuncGroup->SupportedPcmRates);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X supported PCM sizes/rates: 0x%X\n", FuncGroup->NodeId, FuncGroup->SupportedPcmRates));

  // Get default supported stream formats.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_STREAM_FORMATS), &FuncGroup->SupportedFormats);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X supported formats: 0x%X\n", FuncGroup->NodeId, FuncGroup->SupportedFormats));

  // Get default input amp capabilities.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), &FuncGroup->AmpInCapabilities);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X input amp capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->AmpInCapabilities));

  // Get default output amp capabilities.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), &FuncGroup->AmpOutCapabilities);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X output amp capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->AmpOutCapabilities));

  // Get supported power states.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_POWER_STATES), &FuncGroup->SupportedPowerStates);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X supported power states: 0x%X\n", FuncGroup->NodeId, FuncGroup->SupportedPowerStates));

  // Get GPIO capabilities.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_GPIO_COUNT), &FuncGroup->GpioCapabilities);
  if (EFI_ERROR(Status))
    return Status;
  //DEBUG((DEBUG_INFO, "Function group @ 0x%X GPIO capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->GpioCapabilities));

  // Get number of widgets in function group.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), &Response);
  if (EFI_ERROR(Status))
    return Status;
  WidgetStart = HDA_PARAMETER_SUBNODE_COUNT_START(Response);
  WidgetCount = HDA_PARAMETER_SUBNODE_COUNT_TOTAL(Response);
  WidgetEnd = WidgetStart + WidgetCount - 1;
  DEBUG((DEBUG_INFO, "Function group @ 0x%X contains %u widgets, start @ 0x%X, end @ 0x%X\n",
    FuncGroup->NodeId, WidgetCount, WidgetStart, WidgetEnd));

  // Power up.
  Status = HdaIo->SendCommand(HdaIo, FuncGroup->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_POWER_STATE, 0), &Response);
  ASSERT_EFI_ERROR(Status);

  // Ensure there are widgets.
  if (WidgetCount == 0)
    return EFI_UNSUPPORTED;

  // Allocate space for widgets.
  FuncGroup->Widgets = AllocateZeroPool(sizeof(HDA_WIDGET_DEV) * WidgetCount);
  if (FuncGroup->Widgets == NULL)
    return EFI_OUT_OF_RESOURCES;
  FuncGroup->WidgetsCount = WidgetCount;

  // Probe widgets.
  DEBUG((DEBUG_INFO, "HdaCodecProbeFuncGroup(): probing widgets\n"));
  for (UINT8 w = 0; w < WidgetCount; w++) {
    // Get widget.
    HdaWidget = FuncGroup->Widgets + w;

    // Probe widget.
    HdaWidget->FuncGroup = FuncGroup;
    HdaWidget->NodeId = WidgetStart + w;
    Status = HdaCodecProbeWidget(HdaWidget);
    ASSERT_EFI_ERROR(Status);

    // Power up.
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_POWER_CNTRL) {
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_POWER_STATE, 0), &Response);
      ASSERT_EFI_ERROR(Status);
    }
  }

  // Probe widget connections.
  DEBUG((DEBUG_INFO, "HdaCodecProbeFuncGroup(): probing widget connections\n"));
  for (UINT8 w = 0; w < WidgetCount; w++) {
    // Get widget.
    HdaWidget = FuncGroup->Widgets + w;

    // Get connections.
    if (HdaWidget->ConnectionCount > 0) {
      // Allocate array of widget pointers.
      HdaWidget->WidgetConnections = AllocateZeroPool(sizeof(HDA_WIDGET_DEV*) * HdaWidget->ConnectionCount);
      if (HdaWidget->WidgetConnections == NULL)
        return EFI_OUT_OF_RESOURCES;

      // Populate array.
      for (UINT8 c = 0; c < HdaWidget->ConnectionCount; c++) {
        // Get widget index.
        // This can be gotten using the node ID of the connection minus our starting node ID.
        UINT16 WidgetIndex = HdaWidget->Connections[c] - WidgetStart;
        if (WidgetIndex < 0) {
          DEBUG((DEBUG_INFO, "Widget @ 0x%X error connection to index %u (0x%X) is invalid\n", WidgetIndex, HdaWidget->Connections[c]));
          continue;
        }

        // Save pointer to widget.
        HdaConnectedWidget = FuncGroup->Widgets + WidgetIndex;
        //DEBUG((DEBUG_INFO, "Widget @ 0x%X found connection to index %u (0x%X, type 0x%X)\n",
        //    HdaWidget->NodeId, WidgetIndex, HdaConnectedWidget->NodeId, HdaConnectedWidget->Type));
        HdaWidget->WidgetConnections[c] = HdaConnectedWidget;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecProbeCodec(
  IN HDA_CODEC_DEV *HdaCodecDev) {
  //DEBUG((DEBUG_INFO, "HdaCodecProbeCodec(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo = HdaCodecDev->HdaIo;
  UINT32 Response;
  UINT8 FuncStart;
  UINT8 FuncEnd;
  UINT8 FuncCount;

  // Get vendor and device ID.
  Status = HdaIo->SendCommand(HdaIo, HDA_NID_ROOT,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID), &HdaCodecDev->VendorId);
  if (EFI_ERROR(Status))
    return Status;
  DEBUG((DEBUG_INFO, "Codec ID: 0x%X:0x%X\n", HDA_PARAMETER_VENDOR_ID_VEN(HdaCodecDev->VendorId), HDA_PARAMETER_VENDOR_ID_DEV(HdaCodecDev->VendorId)));

  // Get revision ID.
  Status = HdaIo->SendCommand(HdaIo, HDA_NID_ROOT,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_REVISION_ID), &HdaCodecDev->RevisionId);
  if (EFI_ERROR(Status))
    return Status;

  // Try to match codec name.
  HdaCodecDev->Name = AsciiStrCopyToUnicode (OcHdaCodecGetName (HdaCodecDev->VendorId, (UINT16) HdaCodecDev->RevisionId), 0);
  DEBUG((DEBUG_INFO, "Codec name: %s\n", HdaCodecDev->Name));

  // Get function group count.
  Status = HdaIo->SendCommand(HdaIo, HDA_NID_ROOT,
    HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), &Response);
  if (EFI_ERROR(Status))
    return Status;
  FuncStart = HDA_PARAMETER_SUBNODE_COUNT_START(Response);
  FuncCount = HDA_PARAMETER_SUBNODE_COUNT_TOTAL(Response);
  FuncEnd = FuncStart + FuncCount - 1;
  DEBUG((DEBUG_INFO, "Codec contains %u function groups, start @ 0x%X, end @ 0x%X\n", FuncCount, FuncStart, FuncEnd));

  // Ensure there are functions.
  if (FuncCount == 0)
    return EFI_UNSUPPORTED;

  // Allocate space for function groups.
  HdaCodecDev->FuncGroups = AllocateZeroPool(sizeof(HDA_FUNC_GROUP) * FuncCount);
  if (HdaCodecDev->FuncGroups == NULL)
    return EFI_OUT_OF_RESOURCES;
  HdaCodecDev->FuncGroupsCount = FuncCount;
  HdaCodecDev->AudioFuncGroup = NULL;

  // Probe functions.
  for (UINT8 i = 0; i < FuncCount; i++) {
    HdaCodecDev->FuncGroups[i].HdaCodecDev = HdaCodecDev;
    HdaCodecDev->FuncGroups[i].NodeId = FuncStart + i;
    Status = HdaCodecProbeFuncGroup(HdaCodecDev->FuncGroups + i);
    if (!(EFI_ERROR(Status)) && (HdaCodecDev->AudioFuncGroup == NULL))
      HdaCodecDev->AudioFuncGroup = HdaCodecDev->FuncGroups + i;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecFindUpstreamOutput(
  IN HDA_WIDGET_DEV *HdaWidget,
  IN UINT8 Level) {
  //DEBUG((DEBUG_INFO, "HdaCodecFindUpstreamOutput(): start\n"));

  // If level is above 15, we may have entered an infinite loop so just give up.
  if (Level > 15)
    return EFI_ABORTED;

  // Create variables.
  EFI_STATUS Status;
  HDA_WIDGET_DEV *HdaConnectedWidget;

  // Go through connections and check for Output widgets.
  for (UINT8 c = 0; c < HdaWidget->ConnectionCount; c++) {
    // Get connected widget.
    HdaConnectedWidget = HdaWidget->WidgetConnections[c];
    DEBUG((DEBUG_INFO, "%*aWidget @ 0x%X (type 0x%X)\n",
      Level, " ", HdaConnectedWidget->NodeId, HdaConnectedWidget->Type));

    // If this is an Output, we are done.
    if (HdaConnectedWidget->Type == HDA_WIDGET_TYPE_OUTPUT) {
      HdaWidget->UpstreamWidget = HdaConnectedWidget;
      HdaWidget->UpstreamIndex = c;
      return EFI_SUCCESS;
    }

    // Check connections of connected widget.
    // If a success status is returned, that means an Output widget was found and we are done.
    Status = HdaCodecFindUpstreamOutput(HdaConnectedWidget, Level + 1);
    if (Status == EFI_SUCCESS) {
      HdaWidget->UpstreamWidget = HdaConnectedWidget;
      HdaWidget->UpstreamIndex = c;
      return EFI_SUCCESS;
    }
  }

  // We didn't find an Output if we got here (probably zero connections).
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
HdaCodecParsePorts(
  IN HDA_CODEC_DEV *HdaCodecDev) {
  //DEBUG((DEBUG_INFO, "HdaCodecParsePorts(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo = HdaCodecDev->HdaIo;
  HDA_FUNC_GROUP *HdaFuncGroup;
  HDA_WIDGET_DEV *HdaWidget;
  UINT8 DefaultDeviceType;
  UINT32 Response;

  // Loop through each function group.
  for (UINT8 f = 0; f < HdaCodecDev->FuncGroupsCount; f++) {
    // Get function group.
    HdaFuncGroup = HdaCodecDev->FuncGroups + f;

    // Loop through each widget.
    for (UINT8 w = 0; w < HdaFuncGroup->WidgetsCount; w++) {
      // Get widget.
      HdaWidget = HdaFuncGroup->Widgets + w;

      // Is the widget a pin complex? If not, ignore it.
      // If this is a pin complex but it has no connection to a port, also ignore it.
      // If the default association for the pin complex is zero, also ignore it.
      if ((HdaWidget->Type != HDA_WIDGET_TYPE_PIN_COMPLEX) ||
        (HDA_VERB_GET_CONFIGURATION_DEFAULT_PORT_CONN(HdaWidget->DefaultConfiguration) == HDA_CONFIG_DEFAULT_PORT_CONN_NONE) ||
        (HDA_VERB_GET_CONFIGURATION_DEFAULT_ASSOCIATION(HdaWidget->DefaultConfiguration) == 0))
        continue;

      // Determine if port is an output based on the device type.
      DefaultDeviceType = HDA_VERB_GET_CONFIGURATION_DEFAULT_DEVICE(HdaWidget->DefaultConfiguration);
      if ((DefaultDeviceType == HDA_CONFIG_DEFAULT_DEVICE_LINE_OUT) || (DefaultDeviceType == HDA_CONFIG_DEFAULT_DEVICE_SPEAKER) ||
        (DefaultDeviceType == HDA_CONFIG_DEFAULT_DEVICE_HEADPHONE_OUT) || (DefaultDeviceType == HDA_CONFIG_DEFAULT_DEVICE_SPDIF_OUT) ||
        (DefaultDeviceType == HDA_CONFIG_DEFAULT_DEVICE_OTHER_DIGITAL_OUT)) {

        // Try to get upstream output.
        DEBUG((DEBUG_INFO, "Port widget @ 0x%X is an output (pin defaults 0x%X)\n", HdaWidget->NodeId, HdaWidget->DefaultConfiguration));
        Status = HdaCodecFindUpstreamOutput(HdaWidget, 0);
        if (EFI_ERROR(Status))
          continue;

        // Enable output amp.
        Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_PIN_WIDGET_CONTROL,
          HDA_VERB_SET_PIN_WIDGET_CONTROL_PAYLOAD(0, FALSE, FALSE, TRUE, FALSE)), &Response);
        if (EFI_ERROR(Status))
          continue;

        // If EAPD is present, enable.
        if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) {
          // Get current EAPD setting.
          Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_GET_EAPD_BTL_ENABLE, 0), &Response);
          if (EFI_ERROR(Status))
            return Status;

          // If the EAPD is not set, set it.
          if (!(Response & HDA_EAPD_BTL_ENABLE_EAPD)) {
            Response |= HDA_EAPD_BTL_ENABLE_EAPD;
            Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_EAPD_BTL_ENABLE,
              (UINT8)Response), &Response);
            if (EFI_ERROR(Status))
              return Status;
          }
        }

        // If the output amp supports muting, unmute.
        if (HdaWidget->AmpOutCapabilities & HDA_PARAMETER_AMP_CAPS_MUTE) {
          UINT8 offset = HDA_PARAMETER_AMP_CAPS_OFFSET(HdaWidget->AmpOutCapabilities); // TODO set volume.

          // If there are no overriden amp capabilities, check function group.
          if (!(HdaWidget->AmpOverride))
            offset = HDA_PARAMETER_AMP_CAPS_OFFSET(HdaWidget->FuncGroup->AmpOutCapabilities);

          // Unmute amp.
          Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_AMP_GAIN_MUTE,
            HDA_VERB_SET_AMP_GAIN_MUTE_PAYLOAD(0, offset, FALSE, TRUE, TRUE, FALSE, TRUE)), &Response);
          if (EFI_ERROR(Status))
            return Status;
        }

        // Reallocate output array.
        HdaCodecDev->OutputPorts = ReallocatePool(sizeof(HDA_WIDGET_DEV*) * HdaCodecDev->OutputPortsCount, sizeof(HDA_WIDGET_DEV*) * (HdaCodecDev->OutputPortsCount + 1), HdaCodecDev->OutputPorts);
        if (HdaCodecDev->OutputPorts == NULL)
          return EFI_OUT_OF_RESOURCES;
        HdaCodecDev->OutputPortsCount++;

        // Add widget to output array.
        HdaCodecDev->OutputPorts[HdaCodecDev->OutputPortsCount - 1] = HdaWidget;
      }
    }
  }

  // Wait 1000ms for all widgets to fully come on.
  gBS->Stall(MS_TO_MICROSECOND(1000));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecInstallProtocols(
  IN HDA_CODEC_DEV *HdaCodecDev) {
  DEBUG((DEBUG_INFO, "HdaCodecInstallProtocols(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  HDA_CODEC_INFO_PRIVATE_DATA *HdaCodecInfoData;
  AUDIO_IO_PRIVATE_DATA *AudioIoData;

  // Allocate space for protocol data.
  HdaCodecInfoData = AllocateZeroPool(sizeof(HDA_CODEC_INFO_PRIVATE_DATA));
  AudioIoData = AllocateZeroPool(sizeof(AUDIO_IO_PRIVATE_DATA));
  if ((HdaCodecInfoData == NULL) || (AudioIoData == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_POOLS;
  }

  // Populate info protocol data.
  HdaCodecInfoData->Signature = HDA_CODEC_PRIVATE_DATA_SIGNATURE;
  HdaCodecInfoData->HdaCodecDev = HdaCodecDev;
  HdaCodecInfoData->HdaCodecInfo.GetName = HdaCodecInfoGetCodecName;
  HdaCodecInfoData->HdaCodecInfo.GetVendorId = HdaCodecInfoGetVendorId;
  HdaCodecInfoData->HdaCodecInfo.GetRevisionId = HdaCodecInfoGetRevisionId;
  HdaCodecInfoData->HdaCodecInfo.GetAudioFuncId = HdaCodecInfoGetAudioFuncId;
  HdaCodecInfoData->HdaCodecInfo.GetDefaultRatesFormats = HdaCodecInfoGetDefaultRatesFormats;
  HdaCodecInfoData->HdaCodecInfo.GetDefaultAmpCaps = HdaCodecInfoGetDefaultAmpCaps;
  HdaCodecInfoData->HdaCodecInfo.GetWidgets = HdaCodecInfoGetWidgets;
  HdaCodecInfoData->HdaCodecInfo.FreeWidgetsBuffer = HdaCodecInfoFreeWidgetsBuffer;
  HdaCodecDev->HdaCodecInfoData = HdaCodecInfoData;

  // Populate I/O protocol data.
  AudioIoData->Signature = HDA_CODEC_PRIVATE_DATA_SIGNATURE;
  AudioIoData->HdaCodecDev = HdaCodecDev;
  AudioIoData->AudioIo.GetOutputs = HdaCodecAudioIoGetOutputs;
  AudioIoData->AudioIo.SetupPlayback = HdaCodecAudioIoSetupPlayback;
  AudioIoData->AudioIo.StartPlayback = HdaCodecAudioIoStartPlayback;
  AudioIoData->AudioIo.StartPlaybackAsync = HdaCodecAudioIoStartPlaybackAsync;
  AudioIoData->AudioIo.StopPlayback = HdaCodecAudioIoStopPlayback;
  HdaCodecDev->AudioIoData = AudioIoData;

  // Install protocols.
  Status = gBS->InstallMultipleProtocolInterfaces(&HdaCodecDev->ControllerHandle,
    &gEfiHdaCodecInfoProtocolGuid, &HdaCodecInfoData->HdaCodecInfo,
    &gEfiAudioIoProtocolGuid, &AudioIoData->AudioIo,
    &gEfiCallerIdGuid, HdaCodecDev, NULL);
  if (EFI_ERROR(Status))
    goto FREE_POOLS;
  return EFI_SUCCESS;

FREE_POOLS:
  if (HdaCodecInfoData != NULL)
    FreePool(HdaCodecInfoData);
  if (AudioIoData != NULL)
    FreePool(AudioIoData);
  return Status;
}

EFI_STATUS
EFIAPI
HdaCodecGetOutputDac(
  IN  HDA_WIDGET_DEV *HdaWidget,
  OUT HDA_WIDGET_DEV **HdaOutputWidget) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecGetOutputDac(): start\n"));

  // Check that parameters are valid.
  if ((HdaWidget == NULL) || (HdaOutputWidget == NULL))
    return EFI_INVALID_PARAMETER;

  // Crawl through widget path looking for output DAC.
  while (HdaWidget != NULL) {
    // Is this widget an output DAC?
    if (HdaWidget->Type == HDA_WIDGET_TYPE_OUTPUT) {
      *HdaOutputWidget = HdaWidget;
      return EFI_SUCCESS;
    }

    // Move to upstream widget.
    HdaWidget = HdaWidget->UpstreamWidget;
  }

  // If we get here, we couldn't find the DAC.
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
HdaCodecGetSupportedPcmRates(
  IN  HDA_WIDGET_DEV *HdaPinWidget,
  OUT UINT32 *SupportedRates) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecGetSupportedPcmRates(): start\n"));

  // Check that parameters are valid.
  if ((HdaPinWidget == NULL) || (SupportedRates == NULL))
    return EFI_INVALID_PARAMETER;

  // Create variables.
  EFI_STATUS Status;
  HDA_WIDGET_DEV *HdaOutputWidget;

  // Get output DAC widget.
  Status = HdaCodecGetOutputDac(HdaPinWidget, &HdaOutputWidget);
  if (EFI_ERROR(Status))
    return Status;

  // Does the widget specify format info?
  if (HdaOutputWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_FORMAT_OVERRIDE) {
    // Check widget for PCM support.
    if (!(HdaOutputWidget->SupportedFormats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_PCM))
      return EFI_UNSUPPORTED;
    *SupportedRates = HdaOutputWidget->SupportedPcmRates;
  } else {
    // Check function group for PCM support.
    if (!(HdaOutputWidget->FuncGroup->SupportedFormats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_PCM))
      return EFI_UNSUPPORTED;
    *SupportedRates = HdaOutputWidget->FuncGroup->SupportedPcmRates;
  }

  DEBUG((DEBUG_VERBOSE, "HdaCodecGetSupportedPcmRates(): supported rates - 0x%X\n", *SupportedRates));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecDisableWidgetPath(
  IN HDA_WIDGET_DEV *HdaWidget) {
  //DEBUG((DEBUG_INFO, "HdaCodecDisableWidgetPath(): start\n"));

  // Check if widget is valid.
  if (HdaWidget == NULL)
    return EFI_INVALID_PARAMETER;

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo = HdaWidget->FuncGroup->HdaCodecDev->HdaIo;
  UINT32 Response;

  // Crawl through widget path.
  while (HdaWidget != NULL) {
    // If Output, disable stream.
    if (HdaWidget->Type == HDA_WIDGET_TYPE_OUTPUT) {
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL,
        HDA_VERB_SET_CONVERTER_STREAM_PAYLOAD(0, 0)), &Response);
      if (EFI_ERROR(Status))
        return Status;
    }

    // If widget is a pin complex, disable output.
    if (HdaWidget->Type == HDA_WIDGET_TYPE_PIN_COMPLEX) {
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_PIN_WIDGET_CONTROL,
        HDA_VERB_SET_PIN_WIDGET_CONTROL_PAYLOAD(0, FALSE, FALSE, FALSE, FALSE)), &Response);
      if (EFI_ERROR(Status))
        return Status;
    }

    // Move to upstream widget.
    HdaWidget = HdaWidget->UpstreamWidget;
  }

  // Path disabled.
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecEnableWidgetPath(
  IN HDA_WIDGET_DEV *HdaWidget,
  IN UINT8 Volume,
  IN UINT8 StreamId,
  IN UINT16 StreamFormat) {
  //DEBUG((DEBUG_INFO, "HdaCodecEnableWidgetPath(): start\n"));

  // Check if widget is valid.
  if ((HdaWidget == NULL) || (Volume > EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME))
    return EFI_INVALID_PARAMETER;

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo = HdaWidget->FuncGroup->HdaCodecDev->HdaIo;
  UINT32 Response;

  // Crawl through widget path.
  while (HdaWidget != NULL) {
    DEBUG((DEBUG_INFO, "Widget @ 0x%X setting up\n", HdaWidget->NodeId));

    // If pin complex, set as output.
    if (HdaWidget->Type == HDA_WIDGET_TYPE_PIN_COMPLEX) {
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_PIN_WIDGET_CONTROL,
        HDA_VERB_SET_PIN_WIDGET_CONTROL_PAYLOAD(0, FALSE, FALSE, TRUE, FALSE)), &Response);
      if (EFI_ERROR(Status))
        return Status;

      // If EAPD, enable.
      if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) {
        // Get current EAPD setting.
        Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_GET_EAPD_BTL_ENABLE, 0), &Response);
        if (EFI_ERROR(Status))
          return Status;

        // If the EAPD is not set, set it.
        if (!(Response & HDA_EAPD_BTL_ENABLE_EAPD)) {
          Response |= HDA_EAPD_BTL_ENABLE_EAPD;
          Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_EAPD_BTL_ENABLE,
            (UINT8)Response), &Response);
          if (EFI_ERROR(Status))
            return Status;
        }
      }
    }

    // If this is a digital widget, enable digital output.
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_DIGITAL) {
      // Enable digital output.
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_DIGITAL_CONV_CONTROL1,
        HDA_DIGITAL_CONV_CONTROL_DIGEN), &Response);
      if (EFI_ERROR(Status))
        return Status;

      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_ASP_MAPPING,
        0x00), &Response);
      if (EFI_ERROR(Status))
        return Status;
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_ASP_MAPPING,
        0x11), &Response);
      if (EFI_ERROR(Status))
        return Status;
    }

    // If there is an output amp, unmute.
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) {
      UINT8 offset = HDA_PARAMETER_AMP_CAPS_OFFSET(HdaWidget->AmpOutCapabilities); // TODO set volume.

      // If there are no overriden amp capabilities, check function group.
      if (!(HdaWidget->AmpOverride))
        offset = HDA_PARAMETER_AMP_CAPS_OFFSET(HdaWidget->FuncGroup->AmpOutCapabilities);

      // Calculate offset.
      offset = (offset * Volume) / EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME;
      DEBUG((DEBUG_INFO, "HdaCodecEnableWidgetPath(): Amp out offset 0x%X\n", offset));
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_AMP_GAIN_MUTE,
        HDA_VERB_SET_AMP_GAIN_MUTE_PAYLOAD(0, offset, FALSE, TRUE, TRUE, FALSE, TRUE)), &Response);
      if (EFI_ERROR(Status))
        return Status;
    }

    // If there are input amps, mute all but the upstream.
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) {
      DEBUG((DEBUG_INFO, "Widget @ 0x%X in amp\n", HdaWidget->NodeId));
      for (UINT8 c = 0; c < HdaWidget->ConnectionCount; c++) {
        if (HdaWidget->UpstreamIndex == c) {
          UINT8 offset = HDA_PARAMETER_AMP_CAPS_OFFSET(HdaWidget->AmpInCapabilities);
          // If there are no overriden amp capabilities, check function group.
          if (!(HdaWidget->AmpOverride))
            offset = HDA_PARAMETER_AMP_CAPS_OFFSET(HdaWidget->FuncGroup->AmpInCapabilities);
          Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_AMP_GAIN_MUTE,
            HDA_VERB_SET_AMP_GAIN_MUTE_PAYLOAD(c, offset, FALSE, TRUE, TRUE, TRUE, FALSE)), &Response);
          if (EFI_ERROR(Status))
            return Status;
        } else {
          Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_AMP_GAIN_MUTE,
            HDA_VERB_SET_AMP_GAIN_MUTE_PAYLOAD(c, 0, TRUE, TRUE, TRUE, TRUE, FALSE)), &Response);
          if (EFI_ERROR(Status))
            return Status;
        }
      }
    }

    // If there is more than one connection, select our upstream.
    if (HdaWidget->ConnectionCount > 1) {
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_CONN_SELECT_CONTROL,
        HdaWidget->UpstreamIndex), &Response);
      if (EFI_ERROR(Status))
        return Status;
    }

    // If Output, set up stream.
    if (HdaWidget->Type == HDA_WIDGET_TYPE_OUTPUT) {
      DEBUG((DEBUG_INFO, "Widget @ 0x%X output\n", HdaWidget->NodeId));
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_CONVERTER_FORMAT,
        StreamFormat), &Response);
      if (EFI_ERROR(Status))
        return Status;
      Status = HdaIo->SendCommand(HdaIo, HdaWidget->NodeId, HDA_CODEC_VERB(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL,
        HDA_VERB_SET_CONVERTER_STREAM_PAYLOAD(0, StreamId)), &Response);
      if (EFI_ERROR(Status))
        return Status;
    }

    // Move to upstream widget.
    HdaWidget = HdaWidget->UpstreamWidget;
  }
  return EFI_SUCCESS;
}

VOID
EFIAPI
HdaCodecCleanup(
  IN HDA_CODEC_DEV *HdaCodecDev) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecCleanup(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  HDA_FUNC_GROUP *HdaFuncGroup;
  HDA_WIDGET_DEV *HdaWidget;

  // If codec is already clear, we are done.
  if (HdaCodecDev == NULL)
    return;

  // Clean HDA Codec Info protocol.
  if (HdaCodecDev->HdaCodecInfoData != NULL) {
    // Uninstall protocol.
    DEBUG((DEBUG_VERBOSE, "HdaCodecCleanup(): clean Hda Codec Info\n"));
    Status = gBS->UninstallProtocolInterface(HdaCodecDev->ControllerHandle,
      &gEfiHdaCodecInfoProtocolGuid, &HdaCodecDev->HdaCodecInfoData->HdaCodecInfo);
    ASSERT_EFI_ERROR(Status);

    // Free data.
    FreePool(HdaCodecDev->HdaCodecInfoData);
  }

  // Clean Audio I/O protocol.
  if (HdaCodecDev->AudioIoData != NULL) {
    // Uninstall protocol.
    DEBUG((DEBUG_VERBOSE, "HdaCodecCleanup(): clean Audio I/O\n"));
    Status = gBS->UninstallProtocolInterface(HdaCodecDev->ControllerHandle,
      &gEfiAudioIoProtocolGuid, &HdaCodecDev->AudioIoData->AudioIo);
    ASSERT_EFI_ERROR(Status);

    // Free data.
    FreePool(HdaCodecDev->AudioIoData);
  }

  // Clean up input and output port arrays.
  if (HdaCodecDev->OutputPorts != NULL)
    FreePool(HdaCodecDev->OutputPorts);
  if (HdaCodecDev->InputPorts != NULL)
    FreePool(HdaCodecDev->InputPorts);

  // Clean function groups.
  if (HdaCodecDev->FuncGroups != NULL) {
    // Clean each function group.
    for (UINT8 f = 0; f < HdaCodecDev->FuncGroupsCount; f++) {
      HdaFuncGroup = HdaCodecDev->FuncGroups + f;

      // Clean widgets in function group.
      if (HdaFuncGroup->Widgets != NULL) {
        for (UINT8 w = 0; w < HdaFuncGroup->WidgetsCount; w++) {
          HdaWidget = HdaFuncGroup->Widgets + w;

          // Clean input amp default arrays.
          if (HdaWidget->AmpInLeftDefaultGainMute != NULL)
            FreePool(HdaWidget->AmpInLeftDefaultGainMute);
          if (HdaWidget->AmpInRightDefaultGainMute != NULL)
            FreePool(HdaWidget->AmpInRightDefaultGainMute);

          // Clean connections array.
          if (HdaWidget->WidgetConnections != NULL)
            FreePool(HdaWidget->WidgetConnections);
          if (HdaWidget->Connections != NULL)
            FreePool(HdaWidget->Connections);
        }

        // Free widgets array.
        FreePool(HdaFuncGroup->Widgets);
      }
    }

    // Free function group array.
    FreePool(HdaCodecDev->FuncGroups);
  }

  // Free codec device.
  gBS->UninstallProtocolInterface(HdaCodecDev->ControllerHandle,
    &gEfiCallerIdGuid, HdaCodecDev);
  FreePool(HdaCodecDev);
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo;
  UINT8 CodecAddress;

  // Attempt to open the HDA codec protocol. If it can be opened, we can support it.
  Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaIoProtocolGuid, (VOID**)&HdaIo,
    This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR(Status))
    return Status;

  // Get address of codec.
  Status = HdaIo->GetAddress(HdaIo, &CodecAddress);
  if (EFI_ERROR(Status))
    goto CLOSE_CODEC;

  // Codec can be supported.
  DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported(): attaching to codec 0x%X\n", CodecAddress));
  Status = EFI_SUCCESS;

CLOSE_CODEC:
  // Close protocol.
  gBS->CloseProtocol(ControllerHandle, &gEfiHdaIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
  return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecDriverBindingStart(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_HDA_IO_PROTOCOL *HdaIo;
  EFI_DEVICE_PATH_PROTOCOL *HdaCodecDevicePath;
  HDA_CODEC_DEV *HdaCodecDev;

  // Open HDA I/O protocol.
  Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaIoProtocolGuid, (VOID**)&HdaIo,
    This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR(Status))
    return Status;

  // Open Device Path protocol.
  Status = gBS->OpenProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&HdaCodecDevicePath,
    This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status))
    goto CLOSE_CODEC;

  // Allocate codec device.
  HdaCodecDev = AllocateZeroPool(sizeof(HDA_CODEC_DEV));
  if (HdaCodecDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_CODEC;
  }

  // Fill codec device data.
  HdaCodecDev->Signature = HDA_CODEC_PRIVATE_DATA_SIGNATURE;
  HdaCodecDev->HdaIo = HdaIo;
  HdaCodecDev->DevicePath = HdaCodecDevicePath;
  HdaCodecDev->ControllerHandle = ControllerHandle;

  // Probe codec.
  Status = HdaCodecProbeCodec(HdaCodecDev);
  if (EFI_ERROR (Status))
    goto FREE_CODEC;

  // Get ports.
  Status = HdaCodecParsePorts(HdaCodecDev);
  if (EFI_ERROR (Status))
    goto FREE_CODEC;

  // Publish protocols.
  Status = HdaCodecInstallProtocols(HdaCodecDev);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR (Status))
    goto FREE_CODEC;

  // Success.
  return EFI_SUCCESS;

FREE_CODEC:
  // Cleanup codec.
  HdaCodecCleanup(HdaCodecDev);

CLOSE_CODEC:
  // Close protocols.
  gBS->CloseProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, This->DriverBindingHandle, ControllerHandle);
  gBS->CloseProtocol(ControllerHandle, &gEfiHdaIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
  return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStop(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecDriverBindingStop(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  HDA_CODEC_DEV *HdaCodecDev;

  // Get codec device.
  Status = gBS->OpenProtocol(ControllerHandle, &gEfiCallerIdGuid, (VOID**)&HdaCodecDev,
    This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!(EFI_ERROR(Status))) {
    // Ensure codec device is valid.
    if (HdaCodecDev->Signature != HDA_CODEC_PRIVATE_DATA_SIGNATURE)
      return EFI_INVALID_PARAMETER;

    // Cleanup codec.
    HdaCodecCleanup(HdaCodecDev);
  }

  // Close protocols.
  gBS->CloseProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, This->DriverBindingHandle, ControllerHandle);
  gBS->CloseProtocol(ControllerHandle, &gEfiHdaIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
  return EFI_SUCCESS;
}
