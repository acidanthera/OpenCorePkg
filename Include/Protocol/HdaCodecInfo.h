/*
 * File: HdaCodecInfo.h
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

#ifndef EFI_HDA_CODEC_INFO_H
#define EFI_HDA_CODEC_INFO_H

#include <Uefi.h>

/**
  HDA Codec Info protocol GUID.
**/
#define EFI_HDA_CODEC_INFO_PROTOCOL_GUID \
  { 0x6C9CDDE1, 0xE8A5, 0x43E5,          \
    { 0xBE, 0x88, 0xDA, 0x15, 0xBC, 0x1C, 0x02, 0x50 } }

typedef struct EFI_HDA_CODEC_INFO_PROTOCOL_ EFI_HDA_CODEC_INFO_PROTOCOL;

/**
  Widget structure.
**/

typedef struct {
  UINT8   NodeId;
  ///
  /// General widgets.
  ///
  UINT32  Capabilities;
  UINT8   DefaultUnSol;
  UINT8   DefaultEapd;
  ///
  /// Connections.
  ///
  UINT32  ConnectionListLength;
  UINT16  *Connections;
  ///
  /// Power.
  ///
  UINT32  SupportedPowerStates;
  UINT32  DefaultPowerState;
  ///
  /// Amps.
  ///
  UINT32  AmpInCapabilities;
  UINT32  AmpOutCapabilities;
  UINT8   *AmpInLeftDefaultGainMute;
  UINT8   *AmpInRightDefaultGainMute;
  UINT8   AmpOutLeftDefaultGainMute;
  UINT8   AmpOutRightDefaultGainMute;
  ///
  /// Input/Output.
  ///
  UINT32  SupportedPcmRates;
  UINT32  SupportedFormats;
  UINT16  DefaultConvFormat;
  UINT8   DefaultConvStreamChannel;
  UINT8   DefaultConvChannelCount;
  ///
  /// Pin Complex.
  ///
  UINT32  PinCapabilities;
  UINT8   DefaultPinControl;
  UINT32  DefaultConfiguration;
  ///
  /// Volume Knob.
  ///
  UINT32  VolumeCapabilities;
  UINT8   DefaultVolume;
} HDA_WIDGET;

/**
  Gets the codec's name.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] CodecName         A pointer to the buffer to return the codec name.

  @retval EFI_SUCCESS           The codec name was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_NAME) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT CONST CHAR16                 **CodecName
  );

/**
  Gets the codec's vendor and device ID.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] VendorId          The vendor and device ID of the codec.

  @retval EFI_SUCCESS           The vendor and device ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_VENDOR_ID) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT32                       *VendorId
  );

/**
  Gets the codec's revision ID.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] RevisionId        The revision ID of the codec.

  @retval EFI_SUCCESS           The revision ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_REVISION_ID) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT32                       *RevisionId
  );

/**
  Gets the node ID of the codec's audio function.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] AudioFuncId       The node ID of the codec's audio function.
  @param[out] UnsolCapable      Whether or not the function supports unsolicitation.

  @retval EFI_SUCCESS           The node ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_AUDIO_FUNC_ID) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT8                        *AudioFuncId,
  OUT BOOLEAN                      *UnsolCapable
  );

/**
  Gets the codec's default supported stream rates and formats.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] Rates             The default supported rates.
  @param[out] Formats           The default supported formats.

  @retval EFI_SUCCESS           The stream rates and formats were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_DEFAULT_RATES_FORMATS) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT32                       *Rates,
  OUT UINT32                       *Formats
  );

/**
  Gets the codec's default amp capabilities.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] AmpInCaps         The default input amp capabilities.
  @param[out] AmpOutCaps        The default output amp capabilities.

  @retval EFI_SUCCESS           The default amp capabilities were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_DEFAULT_AMP_CAPS) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT UINT32                       *AmpInCaps,
  OUT UINT32                       *AmpOutCaps
  );

/**
  Gets the codec's widgets.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] Widgets           A pointer to the buffer to return the requested array of widgets.
  @param[out] WidgetCount       The number of widgets returned in Widgets.

  @retval EFI_SUCCESS           The widgets were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_GET_WIDGETS) (
  IN  EFI_HDA_CODEC_INFO_PROTOCOL  *This,
  OUT HDA_WIDGET                   **Widgets,
  OUT UINTN                        *WidgetCount
  );

/**
  Frees an array of HDA_WIDGET.

  @param[in] Widgets            A pointer to the buffer array of widgets that is to be freed.
  @param[in] WidgetCount        The number of widgets in Widgets.

  @retval EFI_SUCCESS           The buffer was freed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_INFO_FREE_WIDGETS_BUFFER) (
  IN  HDA_WIDGET                   *Widgets,
  IN  UINTN                        WidgetCount
  );

/**
  Protocol struct.
**/
struct EFI_HDA_CODEC_INFO_PROTOCOL_ {
  EFI_HDA_CODEC_INFO_GET_NAME                     GetName;
  EFI_HDA_CODEC_INFO_GET_VENDOR_ID                GetVendorId;
  EFI_HDA_CODEC_INFO_GET_REVISION_ID              GetRevisionId;
  EFI_HDA_CODEC_INFO_GET_AUDIO_FUNC_ID            GetAudioFuncId;
  EFI_HDA_CODEC_INFO_GET_DEFAULT_RATES_FORMATS    GetDefaultRatesFormats;
  EFI_HDA_CODEC_INFO_GET_DEFAULT_AMP_CAPS         GetDefaultAmpCaps;
  EFI_HDA_CODEC_INFO_GET_WIDGETS                  GetWidgets;
  EFI_HDA_CODEC_INFO_FREE_WIDGETS_BUFFER          FreeWidgetsBuffer;
};

extern EFI_GUID gEfiHdaCodecInfoProtocolGuid;

#endif // EFI_HDA_CODEC_INFO_H
