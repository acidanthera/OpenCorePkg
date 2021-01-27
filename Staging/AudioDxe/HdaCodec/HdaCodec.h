/*
 * File: HdaCodec.h
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

#ifndef _EFI_HDA_CODEC_H_
#define _EFI_HDA_CODEC_H_

#include "AudioDxe.h"

typedef struct _HDA_CODEC_DEV HDA_CODEC_DEV;
typedef struct _HDA_FUNC_GROUP HDA_FUNC_GROUP;
typedef struct _HDA_WIDGET_DEV HDA_WIDGET_DEV;
typedef struct _HDA_CODEC_INFO_PRIVATE_DATA HDA_CODEC_INFO_PRIVATE_DATA;
typedef struct _AUDIO_IO_PRIVATE_DATA AUDIO_IO_PRIVATE_DATA;
#define HDA_CODEC_PRIVATE_DATA_SIGNATURE SIGNATURE_32('H','D','C','O')

struct _HDA_WIDGET_DEV {
  HDA_FUNC_GROUP *FuncGroup;
  UINT8 NodeId;
  UINT8 Type;

  // General widgets.
  UINT32 Capabilities;
  UINT8 DefaultUnSol;

  // Connections.
  UINT32 ConnectionListLength;
  UINT16 *Connections;
  HDA_WIDGET_DEV **WidgetConnections;
  UINT32 ConnectionCount;
  HDA_WIDGET_DEV *UpstreamWidget;
  UINT8 UpstreamIndex;

  // Power.
  UINT32 SupportedPowerStates;
  UINT32 DefaultPowerState;

  // Amps.
  BOOLEAN AmpOverride;
  UINT32 AmpInCapabilities;
  UINT32 AmpOutCapabilities;
  UINT8 *AmpInLeftDefaultGainMute;
  UINT8 *AmpInRightDefaultGainMute;
  UINT8 AmpOutLeftDefaultGainMute;
  UINT8 AmpOutRightDefaultGainMute;

  // Input/Output.
  UINT32 SupportedPcmRates;
  UINT32 SupportedFormats;
  UINT16 DefaultConvFormat;
  UINT8 DefaultConvStreamChannel;
  UINT8 DefaultConvChannelCount;

  // Pin Complex.
  UINT32 PinCapabilities;
  UINT8 DefaultEapd;
  UINT8 DefaultPinControl;
  UINT32 DefaultConfiguration;

  // Volume Knob.
  UINT32 VolumeCapabilities;
  UINT8 DefaultVolume;
};

struct _HDA_FUNC_GROUP {
  HDA_CODEC_DEV *HdaCodecDev;
  UINT8 NodeId;
  BOOLEAN UnsolCapable;
  UINT8 Type;

  // Capabilities.
  UINT32 Capabilities;
  UINT32 SupportedPcmRates;
  UINT32 SupportedFormats;
  UINT32 AmpInCapabilities;
  UINT32 AmpOutCapabilities;
  UINT32 SupportedPowerStates;
  UINT32 GpioCapabilities;

  HDA_WIDGET_DEV *Widgets;
  UINT8 WidgetsCount;
};

struct _HDA_CODEC_DEV {
  // Signature.
  UINTN Signature;

  // Protocols.
  EFI_HDA_IO_PROTOCOL *HdaIo;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_HANDLE ControllerHandle;

  // Published protocols.
  HDA_CODEC_INFO_PRIVATE_DATA *HdaCodecInfoData;
  AUDIO_IO_PRIVATE_DATA *AudioIoData;

  // Codec information.
  UINT32 VendorId;
  UINT32 RevisionId;
  CHAR16 *Name;

  HDA_FUNC_GROUP *FuncGroups;
  UINTN FuncGroupsCount;
  HDA_FUNC_GROUP *AudioFuncGroup;

  // Output and input ports.
  HDA_WIDGET_DEV **OutputPorts;
  HDA_WIDGET_DEV **InputPorts;
  UINTN OutputPortsCount;
  UINTN InputPortsCount;
};

// HDA Codec Info private data.
struct _HDA_CODEC_INFO_PRIVATE_DATA {
  // Signature.
  UINTN Signature;

  // HDA Codec Info protocol and codec device.
  EFI_HDA_CODEC_INFO_PROTOCOL HdaCodecInfo;
  HDA_CODEC_DEV *HdaCodecDev;
};

#define HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This) CR(This, HDA_CODEC_INFO_PRIVATE_DATA, HdaCodecInfo, HDA_CODEC_PRIVATE_DATA_SIGNATURE)

// Audio I/O private data.
struct _AUDIO_IO_PRIVATE_DATA {
  // Signature.
  UINTN Signature;

  // Audio I/O protocol.
  EFI_AUDIO_IO_PROTOCOL AudioIo;
  UINT8 SelectedOutputIndex;
  UINT8 SelectedInputIndex;

  // Codec device.
  HDA_CODEC_DEV *HdaCodecDev;
};

#define AUDIO_IO_PRIVATE_DATA_FROM_THIS(This) CR(This, AUDIO_IO_PRIVATE_DATA, AudioIo, HDA_CODEC_PRIVATE_DATA_SIGNATURE)

//
// HDA Codec Info protocol functions.
//
EFI_STATUS
EFIAPI
HdaCodecInfoGetAddress (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT8                        *Address
  );

EFI_STATUS
EFIAPI
HdaCodecInfoGetCodecName(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT CONST CHAR16 **CodecName);

EFI_STATUS
EFIAPI
HdaCodecInfoGetVendorId(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *VendorId);

EFI_STATUS
EFIAPI
HdaCodecInfoGetRevisionId(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *RevisionId);

EFI_STATUS
EFIAPI
HdaCodecInfoGetAudioFuncId(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT8 *AudioFuncId,
  OUT BOOLEAN *UnsolCapable);

EFI_STATUS
EFIAPI
HdaCodecInfoGetDefaultRatesFormats(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *Rates,
  OUT UINT32 *Formats);

EFI_STATUS
EFIAPI
HdaCodecInfoGetDefaultAmpCaps(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT UINT32 *AmpInCaps,
  OUT UINT32 *AmpOutCaps);

EFI_STATUS
EFIAPI
HdaCodecInfoGetWidgets(
  IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
  OUT HDA_WIDGET **Widgets,
  OUT UINTN *WidgetCount);

EFI_STATUS
EFIAPI
HdaCodecInfoFreeWidgetsBuffer(
  IN  HDA_WIDGET *Widgets,
  IN  UINTN WidgetCount);

//
// Audio I/O protocol functions.
//
EFI_STATUS
EFIAPI
HdaCodecAudioIoGetOutputs(
  IN  EFI_AUDIO_IO_PROTOCOL *This,
  OUT EFI_AUDIO_IO_PROTOCOL_PORT **OutputPorts,
  OUT UINTN *OutputPortsCount);

EFI_STATUS
EFIAPI
HdaCodecAudioIoSetupPlayback(
  IN EFI_AUDIO_IO_PROTOCOL *This,
  IN UINT8 OutputIndex,
  IN UINT8 Volume,
  IN EFI_AUDIO_IO_PROTOCOL_FREQ Freq,
  IN EFI_AUDIO_IO_PROTOCOL_BITS Bits,
  IN UINT8 Channels);

EFI_STATUS
EFIAPI
HdaCodecAudioIoStartPlayback(
  IN EFI_AUDIO_IO_PROTOCOL *This,
  IN VOID *Data,
  IN UINTN DataLength,
  IN UINTN Position OPTIONAL);

EFI_STATUS
EFIAPI
HdaCodecAudioIoStartPlaybackAsync(
  IN EFI_AUDIO_IO_PROTOCOL *This,
  IN VOID *Data,
  IN UINTN DataLength,
  IN UINTN Position OPTIONAL,
  IN EFI_AUDIO_IO_CALLBACK Callback OPTIONAL,
  IN VOID *Context OPTIONAL);

EFI_STATUS
EFIAPI
HdaCodecAudioIoStopPlayback(
  IN EFI_AUDIO_IO_PROTOCOL *This);

//
// HDA Codec internal functions.
//
EFI_STATUS
EFIAPI
HdaCodecPrintDefaults(
  HDA_CODEC_DEV *HdaCodecDev);

EFI_STATUS
EFIAPI
HdaCodecGetOutputDac(
  IN  HDA_WIDGET_DEV *HdaWidget,
  OUT HDA_WIDGET_DEV **HdaOutputWidget);

EFI_STATUS
EFIAPI
HdaCodecGetSupportedPcmRates(
  IN  HDA_WIDGET_DEV *HdaPinWidget,
  OUT UINT32 *SupportedRates);

EFI_STATUS
EFIAPI
HdaCodecDisableWidgetPath(
  IN HDA_WIDGET_DEV *HdaWidget);

EFI_STATUS
EFIAPI
HdaCodecEnableWidgetPath(
  IN HDA_WIDGET_DEV *HdaWidget,
  IN UINT8 Volume,
  IN UINT8 StreamId,
  IN UINT16 StreamFormat);

VOID
EFIAPI
HdaCodecCleanup(
  IN HDA_CODEC_DEV *HdaCodecDev);

//
// Driver Binding protocol functions.
//
EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStop(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE *ChildHandleBuffer OPTIONAL);

#endif
